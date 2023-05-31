/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "includer.hpp"
#include "../symdef.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief Opens the included file and determines
/// the including type.
///
/// \param sIncludingString const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Includer::openIncludedFile(const std::string& sIncludingString)
{
    Match _mMatch = findCommand(sIncludingString);
    std::string sIncludeFileName;

    if (sIncludingString.front() == '@' && sIncludingString[1] != ' ')
    {
        // This syntax is deprecated
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", sIncludingString));

        if (sIncludingString[1] == '"')
            sIncludeFileName = sIncludingString.substr(2, sIncludingString.find('"', 2)-2);
        else
            sIncludeFileName = sIncludingString.substr(1, sIncludingString.find(' ')-1);

        size_t offset = 4; // Corresponds to '@"C:...'

        // Determine the inclusion type
        if (sIncludingString.find(':', offset) != std::string::npos)
        {
            if (sIncludingString.find("defines", sIncludingString.find(':', offset)+1) != std::string::npos)
                m_type = INCLUDE_DEFINES;
            else if (sIncludingString.find("globals", sIncludingString.find(':', offset)+1) != std::string::npos)
                m_type = INCLUDE_GLOBALS;
            else if (sIncludingString.find("declarations", sIncludingString.find(':', offset)+1) != std::string::npos)
                m_type = INCLUDE_DECLARATIONS;
        }

        // Extract the actual file name
        if (sIncludeFileName.find(':') != std::string::npos)
        {
            for (int __i = sIncludeFileName.length()-1; __i >= 0; __i--)
            {
                if (sIncludeFileName[__i] == ':'
                    && (__i > 1
                        || (__i == 1 && sIncludeFileName.length() > (size_t)__i+1 && sIncludeFileName[__i+1] != '/')))
                {
                    sIncludeFileName.erase(__i);
                    break;
                }
            }
        }

        // Get a valid file name
        if (sIncludeFileName.length())
            sIncludeFileName = FileSystem::ValidFileName(sIncludeFileName, ".nscr");
        else
            return;
    }
    else if (_mMatch.sString == "include")
    {
        sIncludeFileName = sIncludingString.substr(_mMatch.nPos+1+_mMatch.sString.length());

        // Extract the actual file name
        if (sIncludeFileName.find("::") != std::string::npos)
            sIncludeFileName.erase(sIncludeFileName.find("::"));

        StripSpaces(sIncludeFileName);

        if (sIncludeFileName.front() == '"')
            sIncludeFileName = sIncludeFileName.substr(1, sIncludeFileName.find('"', 1)-1);

        size_t offset = _mMatch.nPos+1+_mMatch.sString.length()+3; // Corresponds to 'include "C:...'

        // Determine the inclusion type
        if (sIncludingString.find("::", offset) != std::string::npos)
        {
            if (sIncludingString.find("defines", sIncludingString.find("::", offset)+1) != std::string::npos)
                m_type |= INCLUDE_DEFINES;
            else if (sIncludingString.find("globals", sIncludingString.find("::", offset)+1) != std::string::npos)
                m_type |= INCLUDE_GLOBALS;
            else if (sIncludingString.find("declarations", sIncludingString.find("::", offset)+1) != std::string::npos)
                m_type |= INCLUDE_DECLARATIONS;
        }

        // Get a valid file name
        if (sIncludeFileName.length())
            sIncludeFileName = FileSystem::ValidFileName(sIncludeFileName+".*", ".nscr");
        else
            return;
    }

    // Open the include file
    m_include = new StyledTextFile(sIncludeFileName);

    // Ensure that the file is valid
    if (m_include->getLastPosition() == -1)
    {
        delete m_include;
        m_include = nullptr;
        throw SyntaxError(SyntaxError::INCLUDE_NOT_EXIST, sIncludingString, SyntaxError::invalid_position, sIncludeFileName);
    }
}


/////////////////////////////////////////////////
/// \brief Includer class constructor. Opens the
/// included file defined by the passed including
/// string.
///
/// \param sIncludingString const std::string&
/// \param sSearchPath const std::string&
///
/////////////////////////////////////////////////
Includer::Includer(const std::string& sIncludingString, const std::string& sSearchPath) : m_include(nullptr), nIncludeLine(-1), m_type(Includer::INCLUDE_ALL)
{
    initializeFromKernel();

    if (sIncludingString.front() == '@')
        setPath(NumeReKernel::getInstance()->getScript().getPath(), false, getProgramPath());
    else
        setPath(sSearchPath, false, getProgramPath());

    openIncludedFile(sIncludingString);
}


/////////////////////////////////////////////////
/// \brief Includer class destructor. Frees up
/// internal memory if needed.
/////////////////////////////////////////////////
Includer::~Includer()
{
    if (m_include)
        delete m_include;
}


/////////////////////////////////////////////////
/// \brief Return the next line of the included
/// string.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Includer::getNextLine()
{
    if (!is_open())
        return "";

    std::string sIncludedLine;

    // Search for the next valid and non-empty line
    // in the included script
    while (nIncludeLine < m_include->getLinesCount() && !sIncludedLine.length())
    {
        std::string sCurrentLine;

        // Compose lines, which were broken using the "\\" operator
        do
        {
            sCurrentLine = m_include->getStrippedLine(nIncludeLine);
            nIncludeLine++;
            StripSpaces(sCurrentLine);

            if (sIncludedLine.length() > 2 && sIncludedLine.substr(sIncludedLine.length()-2) == "\\\\")
                sIncludedLine.erase(sIncludedLine.length()-2);

            sIncludedLine += sCurrentLine;
        }
        while (nIncludeLine < m_include->getLinesCount()
               && sIncludedLine.length() > 2
               && sIncludedLine.substr(sIncludedLine.length()-2) == "\\\\");

        // Ignore empty lines
        if (!sIncludedLine.length())
            continue;

        // Ignore non-global installation sections
        if (sIncludedLine.substr(0,9) == "<install>"
            || (findCommand(sIncludedLine).sString == "global" && sIncludedLine.find("<install>") != std::string::npos))
        {
            while (nIncludeLine < m_include->getLinesCount())
            {
                sIncludedLine = m_include->getStrippedLine(nIncludeLine);
                nIncludeLine++;
                StripSpaces(sIncludedLine);

                if (sIncludedLine.substr(0,12) == "<endinstall>"
                    || (findCommand(sIncludedLine).sString == "global" && sIncludedLine.find("<endinstall>") != std::string::npos))
                    break;
            }

            sIncludedLine.clear();
            continue;
        }

        // Get the current command
        Match _mMatch = findCommand(sIncludedLine);

        // Ensure that the relevant commands are available
        if (_mMatch.sString != "define"
            && _mMatch.sString != "ifndef"
            && _mMatch.sString != "ifndefined"
            && _mMatch.sString != "redefine"
            && _mMatch.sString != "redef"
            && _mMatch.sString != "lclfunc"
            && _mMatch.sString != "global"
            && _mMatch.sString != SYMDEF_COMMAND)
        {
            sIncludedLine.clear();
            continue;
        }

        // Depending on the include type, only accept the
        // corresponding commands
        if (m_type)
        {
            if (!(m_type & INCLUDE_DEFINES)
                && (_mMatch.sString == "define"
                    || _mMatch.sString == "ifndef"
                    || _mMatch.sString == "ifndefined"
                    || _mMatch.sString == "redefine"
                    || _mMatch.sString == "redef"
                    || _mMatch.sString == "lclfunc"))
            {
                sIncludedLine.clear();
                continue;
            }
            else if (!(m_type & INCLUDE_GLOBALS)
                     && _mMatch.sString == "global")
            {
                sIncludedLine.clear();
                continue;
            }
            else if (!(m_type & INCLUDE_DECLARATIONS)
                     && _mMatch.sString == SYMDEF_COMMAND)
            {
                sIncludedLine.clear();
                continue;
            }
        }
    }

    // If this is the last line, close the included file
    if (nIncludeLine >= m_include->getLinesCount())
    {
        delete m_include;
        m_include = nullptr;
    }

    return sIncludedLine;
}


/////////////////////////////////////////////////
/// \brief Determine, if the internal included
/// file is open and valid.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Includer::is_open() const
{
    if (!m_include)
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Returns the embedded file name.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Includer::getIncludedFileName() const
{
    if (is_open())
        return m_include->getFileName();

    return "";
}


/////////////////////////////////////////////////
/// \brief Static member function which
/// determines, whether the passed line is
/// actually a including syntax.
///
/// \param sLine const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Includer::is_including_syntax(const std::string& sLine)
{
    if (sLine.length() > 1 && sLine.front() == '@' && sLine[1] != ' ')
        return true;

    if (findCommand(sLine).sString == "include")
        return true;

    return false;
}

