/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#include "procedureelement.hpp"
#include "../utils/tools.hpp"
#include "dependency.hpp"


/////////////////////////////////////////////////
/// \brief Procedure element constructor. This
/// class is always heap allocated.
///
/// \param vProcedureContents const std::vector<std::string>&
/// \param sFilePath const std::string&
///
/////////////////////////////////////////////////
ProcedureElement::ProcedureElement(const std::vector<std::string>& vProcedureContents, const std::string& sFilePath) : sFileName(sFilePath), m_dependencies(nullptr)
{
    std::string sFolderPath = sFileName.substr(0, sFileName.rfind('/'));
    std::string sProcCommandLine;
    std::string sCurrentCommand;
    std::string sCurrentLineCache;
    std::string sProcPlotCompose;
    bool bBlockComment = false;

    // Examine the contents of each line
    for (size_t i = 0; i < vProcedureContents.size(); i++)
    {
        // get the current line
        sProcCommandLine = vProcedureContents[i];
        StripSpaces(sProcCommandLine);

        // skip easy cases
        if (!sProcCommandLine.length())
            continue;

        if (sProcCommandLine.substr(0,2) == "##")
            continue;

        // Already inside of a block comment?
        if (bBlockComment)
        {
            if (sProcCommandLine.find("*#") != std::string::npos)
            {
                sProcCommandLine.erase(0, sProcCommandLine.find("*#")+2);
                bBlockComment = false;
            }
            else
                continue;
        }

        // examine the string: consider also quotation marks
        int nQuotes = 0;

        for (size_t j = 0; j < sProcCommandLine.length(); j++)
        {
            // count the quotation marks
            if (sProcCommandLine[j] == '"' && (!j || sProcCommandLine[j-1] != '\\'))
                nQuotes++;

            if (!(nQuotes % 2) && sProcCommandLine.substr(j,2) == "##")
            {
                // that's a standard line comment
                sProcCommandLine.erase(j);
                break;
            }

            if (!(nQuotes % 2) && sProcCommandLine.substr(j,2) == "#*")
            {
                // this is a block comment
                if (sProcCommandLine.find("*#", j+2) != std::string::npos)
                {
                    sProcCommandLine.erase(j, sProcCommandLine.find("*#", j+2)-j+2);
                }
                else
                {
                    sProcCommandLine.erase(j);
                    bBlockComment = true;
                    break;
                }
            }
        }

        // get the current command, if any
        sCurrentCommand = findCommand(sProcCommandLine).sString;

        // clean the current line
        cleanCurrentLine(sProcCommandLine, sCurrentCommand, sFolderPath);

        // Handle composition blocks in advance
        if ((sCurrentCommand == "compose"
			|| sCurrentCommand == "endcompose"
			|| sProcPlotCompose.length())
			&& sCurrentCommand != "quit")
        {
            if (!sProcPlotCompose.length() && sCurrentCommand == "compose")
            {
                // Start of the plot composition block
                sProcPlotCompose = "plotcompose ";

                if (findParameter(sProcCommandLine, "multiplot", '='))
                {
                    sProcPlotCompose += "-multiplot=" + getArgAtPos(sProcCommandLine, findParameter(sProcCommandLine, "multiplot", '=') + 9) + " <<COMPOSE>> ";
                }

                continue;
            }
            else if (sCurrentCommand == "abort")
            {
                // Manual abort
                sProcPlotCompose.clear();
                continue;
            }
            else if (sCurrentCommand != "endcompose")
            {
                // An arbitrary command
                string sCommand = findCommand(sProcCommandLine).sString;

                if (sCommand.substr(0, 4) == "plot"
                        || sCommand.substr(0, 7) == "subplot"
                        || sCommand.substr(0, 5) == "graph"
                        || sCommand.substr(0, 4) == "grad"
                        || sCommand.substr(0, 4) == "draw"
                        || sCommand.substr(0, 4) == "dens"
                        || sCommand.substr(0, 4) == "vect"
                        || sCommand.substr(0, 4) == "cont"
                        || sCommand.substr(0, 4) == "surf"
                        || sCommand.substr(0, 4) == "mesh")
                    sProcPlotCompose += sProcCommandLine + " <<COMPOSE>> ";
                {
                    continue;
                }
            }
            else
            {
                // The actual end of the compose block
                sProcCommandLine = sProcPlotCompose;
                sProcPlotCompose.clear();
            }
        }

        // Avoid "install" and "script" calls
        if (sCurrentCommand == "install"
            || sCurrentCommand == "script")
		{
			throw SyntaxError(SyntaxError::INSTALL_CMD_FOUND, sProcCommandLine, SyntaxError::invalid_position);
		}

        // skip empty lines
        if (!sProcCommandLine.length())
            continue;

        // If the length is longer than 2, then it's possible
        // that we have a line continuation at this point
        if (sProcCommandLine.length() > 2)
        {
            if (sProcCommandLine.substr(sProcCommandLine.length() - 2, 2) == "\\\\")
            {
                // Add the current line to the current line cache
                // and continue
                sCurrentLineCache += sProcCommandLine.substr(0, sProcCommandLine.length() - 2);
                continue;
            }
        }

        // If the current line cache is not empty then simply
        // append the current line and use the whole line as input
        if (sCurrentLineCache.length())
        {
            sProcCommandLine = sCurrentLineCache + sProcCommandLine;
            sCurrentLineCache.clear();
        }

        // Ensure that the parentheses are valid
        if (sProcCommandLine.find('(') != std::string::npos || sProcCommandLine.find('{') != std::string::npos)
        {
            if (!validateParenthesisNumber(sProcCommandLine))
            {
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, SyntaxError::invalid_position);
            }
        }

        // Determine the type of the actual type of the procedure command line
        if (sCurrentCommand == "procedure")
        {
            int nFlags = ProcedureCommandLine::FLAG_NONE;
            std::string sArgumentList;

            // Extract the flags
            if (sProcCommandLine.rfind("::") != std::string::npos)
            {
                std::string sFlags = sProcCommandLine.substr(sProcCommandLine.rfind("::"));

                if (sFlags.find("private") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_PRIVATE;

                if (sFlags.find("explicit") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_EXPLICIT;

                if (sFlags.find("inline") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_INLINE;

                if (sFlags.find("mask") != std::string::npos || sFlags.find("silent") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_MASK;

                if (sFlags.find("template") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_TEMPLATE;
            }

            // Extract procedure name and argument list
            if (sProcCommandLine.find('$') != std::string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$')) != std::string::npos)
            {
                std::string sProcName = sProcCommandLine.substr(sProcCommandLine.find('$'));
                sProcName.erase(sProcName.find('('));

                if (getMatchingParenthesis(sProcCommandLine.substr(sProcCommandLine.find('('))) == std::string::npos)
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, sProcCommandLine.find('('));

                // Ensure that the argument list is defined reasonable
                if (findCommand(sArgumentList, "var").sString == "var")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "var");

                if (findCommand(sArgumentList, "str").sString == "str")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "str");

                if (findCommand(sArgumentList, "tab").sString == "tab")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "tab");

                sArgumentList = sProcCommandLine.substr(sProcCommandLine.find('(')+1);
                sArgumentList = " " + sArgumentList.erase(sArgumentList.rfind(')')) + " ";

                // Store the procedure name and the corresponding line in the
                // procedure list map
                mProcedureList[sProcName] = i;
            }

            mProcedureContents[i] = ProcedureCommandLine(nFlags, ProcedureCommandLine::TYPE_PROCEDURE_HEAD, sProcCommandLine, sArgumentList);
        }
        else if (sCurrentCommand == "endprocedure")
            mProcedureContents[i] = ProcedureCommandLine(ProcedureCommandLine::FLAG_NONE, ProcedureCommandLine::TYPE_PROCEDURE_FOOT, sProcCommandLine);
        else
            mProcedureContents[i] = ProcedureCommandLine(ProcedureCommandLine::FLAG_NONE, ProcedureCommandLine::TYPE_PROCEDURE_BODY, sProcCommandLine);
    }
}


/////////////////////////////////////////////////
/// \brief Destructor. Cleares the dependency
/// list.
/////////////////////////////////////////////////
ProcedureElement::~ProcedureElement()
{
    if (m_dependencies)
        delete m_dependencies;
}


/////////////////////////////////////////////////
/// \brief This member function does the hard
/// work on cleaning the current procedure
/// command line. This includes removing
/// tabulators, definition operators and
/// replacing the "<this>" path placeholder.
///
/// \param sProcCommandLine std::string&
/// \param sCurrentCommand const std::string&
/// \param sFolderPath const std::string&
/// \return void
///
/////////////////////////////////////////////////
void ProcedureElement::cleanCurrentLine(std::string& sProcCommandLine, const std::string& sCurrentCommand, const std::string& sFolderPath)
{
    // Replace the "this" path place holder
    while (sProcCommandLine.find("<this>") != std::string::npos)
        sProcCommandLine.replace(sProcCommandLine.find("<this>"), 6, sFolderPath);

    // Remove the "global" command, if it is available
    if (sCurrentCommand == "global")
    {
        sProcCommandLine = sProcCommandLine.substr(findCommand(sProcCommandLine).nPos + 6);
        StripSpaces(sProcCommandLine);
    }

    // replace tabulator characters with whitespaces
    for (size_t i = 0; i < sProcCommandLine.length(); i++)
    {
        if (sProcCommandLine[i] == '\t')
            sProcCommandLine[i] = ' ';
    }

    // remove definition operator, where needed
    if (sCurrentCommand != "define"
        && sCurrentCommand != "ifndefined"
        && sCurrentCommand != "redefine"
        && sCurrentCommand != "ifndef"
        && sCurrentCommand != "redef"
        && sCurrentCommand != "lclfunc")
    {
        while (sProcCommandLine.find(":=") != std::string::npos)
            sProcCommandLine.erase(sProcCommandLine.find(":="), 1);
    }

    // remove whitespaces
    StripSpaces(sProcCommandLine);
}


/////////////////////////////////////////////////
/// \brief This function returns the first line
/// of the stored file.
///
/// \return std::pair<int, ProcedureCommandLine>
///
/////////////////////////////////////////////////
std::pair<int, ProcedureCommandLine> ProcedureElement::getFirstLine()
{
    return *mProcedureContents.begin();
}


/////////////////////////////////////////////////
/// \brief This function returns the selected
/// line of the stored file. This member function
/// will be used in combination with the the
/// member function "gotoProcedure()"
///
/// \param nCurrentLine int
/// \return std::pair<int, ProcedureCommandLine>
///
/////////////////////////////////////////////////
std::pair<int, ProcedureCommandLine> ProcedureElement::getCurrentLine(int nCurrentLine)
{
    std::pair<int, ProcedureCommandLine> currentLine;
    auto iter = mProcedureContents.find(nCurrentLine);

    if (iter != mProcedureContents.end())
        currentLine = *iter;

    return currentLine;
}


/////////////////////////////////////////////////
/// \brief This member function returns the line
/// after the current selected line. This is
/// probably not the same as nCurrentLine++,
/// because line comments and empty lines were
/// omitted during the pre-parsing step.
///
/// \param nCurrentLine int
/// \return std::pair<int, ProcedureCommandLine>
///
/////////////////////////////////////////////////
std::pair<int, ProcedureCommandLine> ProcedureElement::getNextLine(int nCurrentLine)
{
    std::pair<int, ProcedureCommandLine> currentLine;
    auto iter = mProcedureContents.find(nCurrentLine);

    if (iter != mProcedureContents.end())
    {
        iter++;

        if (iter != mProcedureContents.end())
            currentLine = *iter;
    }

    return currentLine;
}


/////////////////////////////////////////////////
/// \brief This member function determines,
/// whether the current line is the last line of
/// the stored procedure file.
///
/// \param nCurrentLine int
/// \return bool
///
/////////////////////////////////////////////////
bool ProcedureElement::isLastLine(int nCurrentLine)
{
    auto iter = mProcedureContents.find(nCurrentLine);

    if (iter != mProcedureContents.end())
    {
        iter++;

        if (iter != mProcedureContents.end())
            return false;

        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function returns the line
/// of the stored file, where the desired
/// procedure may be found or -1 if the procedure
/// is not found in the current file.
///
/// \param sProcedureName const std::string&
/// \return int
///
/// \remark The procedure name has to be passed
/// with the dollar sign but without the
/// parentheses.
///
/////////////////////////////////////////////////
int ProcedureElement::gotoProcedure(const std::string& sProcedureName)
{
    auto iter = mProcedureList.find(sProcedureName);

    if (iter != mProcedureList.end())
        return iter->second;

    return -1;
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// store the created byte code in the current
/// procedure command line.
///
/// Note that it is not reasonable to store the
/// byte code in the already obtained procedure
/// command line, because this object will be
/// destroyed after its evaluation.
///
/// \param _nByteCode int
/// \param nCurrentLine int
/// \return void
///
/////////////////////////////////////////////////
void ProcedureElement::setByteCode(int _nByteCode, int nCurrentLine)
{
    auto iter = mProcedureContents.find(nCurrentLine);

    if (iter != mProcedureContents.end())
    {
        iter->second.setByteCode(_nByteCode);
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// first-level dependencies of the current
/// procedure file. The dependencies are only
/// calculated once and refreshed, if the current
/// procedure has been saved.
///
/// \return Dependencies*
///
/////////////////////////////////////////////////
Dependencies* ProcedureElement::getDependencies()
{
    if (!m_dependencies)
        m_dependencies = new Dependencies(this);

    return m_dependencies;
}

