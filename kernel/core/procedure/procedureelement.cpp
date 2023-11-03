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
#include "../symdef.hpp"
#include "dependency.hpp"
#include "includer.hpp"
#include "../plotting/plotting.hpp"

#include <memory>


/////////////////////////////////////////////////
/// \brief Procedure element constructor. This
/// class is always heap allocated.
///
/// \param procedureContents const StyledTextFile&
/// \param sFilePath const std::string&
///
/////////////////////////////////////////////////
ProcedureElement::ProcedureElement(const StyledTextFile& procedureContents, const std::string& sFilePath) : sFileName(sFilePath), m_dependencies(nullptr)
{
    std::string sFolderPath = sFileName.substr(0, sFileName.rfind('/'));
    std::string sProcCommandLine;
    std::string sCurrentCommand;
    std::string sCurrentLineCache;
    std::string sProcPlotCompose;

    SymDefManager _symdefs;
    std::unique_ptr<Includer> _includer;
    int i = 0;
    int currentLine = 0;
    int includeOffset = 0;

    // Examine the contents of each line
    while (i < procedureContents.getLinesCount())
    {
        if (_includer && _includer->is_open())
        {
            includeOffset++;
            currentLine = -includeOffset;
            sProcCommandLine = _includer->getNextLine();
        }
        else
        {
            if (_includer)
                _includer.reset();

            // get the current line
            sProcCommandLine = procedureContents.getStrippedLine(i);
            currentLine = i;
            i++;
        }

        StripSpaces(sProcCommandLine);

        // skip easy cases
        if (!sProcCommandLine.length())
            continue;

        if (Includer::is_including_syntax(sProcCommandLine))
        {
            _includer.reset(new Includer(sProcCommandLine, sFolderPath));
            continue;
        }

        // If the length is longer than 2, then it's possible
        // that we have a line continuation at this point
        if (sProcCommandLine.length() > 2)
        {
            if (sProcCommandLine.ends_with("\\\\"))
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

        // get the current command, if any
        sCurrentCommand = findCommand(sProcCommandLine).sString;

        // Resolve symbol macros
        if (sCurrentCommand == SYMDEF_COMMAND)
        {
            _symdefs.createSymbol(sProcCommandLine.substr(sCurrentCommand.length()));
            continue;
        }
        else
        {
            _symdefs.resolveSymbols(sProcCommandLine);
            sCurrentCommand = findCommand(sProcCommandLine).sString;
        }

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
                std::string sCommand = findCommand(sProcCommandLine).sString;

                if (Plot::isPlottingCommand(sCommand))
                {
                    sProcPlotCompose += sProcCommandLine + " <<COMPOSE>> ";
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
			throw SyntaxError(SyntaxError::INSTALL_CMD_FOUND,
                     "@ " + toString(abs(currentLine)+1) + ": " + sProcCommandLine, sCurrentCommand);

        // Ensure that the parentheses are valid
        if (sProcCommandLine.find('(') != std::string::npos || sProcCommandLine.find('{') != std::string::npos)
        {
            if (!validateParenthesisNumber(sProcCommandLine))
            {
                sProcCommandLine.insert(0, "@ " + toString(abs(currentLine)+1) + ": ");

                for (size_t j = 0; j < sProcCommandLine.length(); j++)
                {
                    if ((sProcCommandLine[j] == '(' || sProcCommandLine[j] == '{' || sProcCommandLine[j] == '[')
                         && getMatchingParenthesis(StringView(sProcCommandLine, j)) == std::string::npos)
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, j);
                }

                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, sProcCommandLine.find_last_of(")}]"));
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

                if (sFlags.find("event") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_MASK | ProcedureCommandLine::FLAG_EVENT;

                if (sFlags.find("template") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_TEMPLATE;

                if (sFlags.find("macro") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_MACRO;

                if (sFlags.find("test") != std::string::npos)
                    nFlags |= ProcedureCommandLine::FLAG_TEST;
            }

            // Extract procedure name and argument list
            if (sProcCommandLine.find('$') != std::string::npos
                && sProcCommandLine.find('(', sProcCommandLine.find('$')) != std::string::npos)
            {
                std::string sProcName = sProcCommandLine.substr(sProcCommandLine.find('$'));
                sProcName.erase(sProcName.find('('));

                if (getMatchingParenthesis(StringView(sProcCommandLine, sProcCommandLine.find('('))) == std::string::npos)
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, sProcCommandLine.find('('));

                // Ensure that the argument list is defined reasonable
                if (findCommand(sArgumentList, "var").sString == "var")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME,
                                      "@" + toString(abs(currentLine)+1) + ": " + sProcCommandLine, "var", "var");

                if (findCommand(sArgumentList, "str").sString == "str")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME,
                                      "@" + toString(abs(currentLine)+1) + ": " + sProcCommandLine, "str", "str");

                if (findCommand(sArgumentList, "tab").sString == "tab")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME,
                                      "@" + toString(abs(currentLine)+1) + ": " + sProcCommandLine, "tab", "tab");

                if (findCommand(sArgumentList, "cst").sString == "cst")
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME,
                                      "@" + toString(abs(currentLine)+1) + ": " + sProcCommandLine, "cst", "cst");

                sArgumentList = sProcCommandLine.substr(sProcCommandLine.find('(')+1);
                sArgumentList = " " + sArgumentList.erase(sArgumentList.rfind(')')) + " ";

                // Store the procedure name and the corresponding line in the
                // procedure list map
                mProcedureList[sProcName] = currentLine;
            }

            mProcedureContents.push_back(std::make_pair(currentLine, ProcedureCommandLine(nFlags,
                                                                                          ProcedureCommandLine::TYPE_PROCEDURE_HEAD,
                                                                                          sProcCommandLine,
                                                                                          sArgumentList)));
        }
        else if (sCurrentCommand == "endprocedure")
            mProcedureContents.push_back(std::make_pair(currentLine, ProcedureCommandLine(ProcedureCommandLine::FLAG_NONE,
                                                                                          ProcedureCommandLine::TYPE_PROCEDURE_FOOT,
                                                                                          sProcCommandLine)));
        else
            mProcedureContents.push_back(std::make_pair(currentLine, ProcedureCommandLine(ProcedureCommandLine::FLAG_NONE,
                                                                                          ProcedureCommandLine::TYPE_PROCEDURE_BODY,
                                                                                          sProcCommandLine)));
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
    return mProcedureContents.front();
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

    for (int i = mProcedureContents.size()-1; i >= 0; i--)
    {
        if (mProcedureContents[i].first == nCurrentLine)
            return mProcedureContents[i];

        //if (abs(mProcedureContents[i].first) < nCurrentLine)
        //    break;
    }

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

    for (int i = mProcedureContents.size()-1; i >= 0; i--)
    {
        if (mProcedureContents[i].first == nCurrentLine && (size_t)i+1 < mProcedureContents.size())
            return mProcedureContents[i+1];

        //if (abs(mProcedureContents[i].first) < nCurrentLine)
        //    break;
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
    if (mProcedureContents.back().first <= nCurrentLine)
        return true;

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
    for (int i = mProcedureContents.size()-1; i >= 0; i--)
    {
        if (mProcedureContents[i].first == nCurrentLine)
            mProcedureContents[i].second.setByteCode(_nByteCode);

        //if (abs(mProcedureContents[i].first) < nCurrentLine)
        //    break;
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

