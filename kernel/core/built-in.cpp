/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


#include "built-in.hpp"
#include "../kernel.hpp"
#include "io/fileops.hpp"
#include "datamanagement/dataops.hpp"

//static std::string evaluateParameterValues(const std::string& sCmd);
//static bool extractFirstParameterStringValue(const std::string& sCmd, std::string& sArgument);
static bool parseCmdArg(const std::string& sCmd, size_t nPos, mu::Parser& _parser, size_t& nArgument);

#include "commandfunctions.hpp"


/////////////////////////////////////////////////
/// \brief This function is the main command
/// handling function.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/// The function identifies the commands in the
/// passed string and passes control to the
/// corresponding command function registered
/// in one of the static maps.
///
/// If no command has been found, the function
/// will return \c NO_COMMAND
/////////////////////////////////////////////////
CommandReturnValues commandHandler(string& sCmd)
{
    // Get the command functions as static maps
    static map<string,CommandFunc> mCommandsWithReturnValue = getCommandFunctionsWithReturnValues();
    static map<string,CommandFunc> mCommands = getCommandFunctions();

	StripSpaces(sCmd);
	sCmd += " ";
	string sCommand = findCommand(sCmd).sString;

	// Keyword search and application documentation
	// are most important and will evaluated first
	if (sCommand == "find" || sCommand == "search")
        return cmd_find(sCmd);
    else if (sCommand == "help" || sCommand == "man" || sCommand == "doc")
        return cmd_help(sCmd);

    // Try to find any of the commands with a
    // return value. These have to be searched
    // in the command string, because they are
    // most probably not the first command
    // candidate.
    for (auto iter = mCommandsWithReturnValue.begin(); iter != mCommandsWithReturnValue.end(); ++iter)
    {
        if (findCommand(sCmd, iter->first).sString == iter->first)
            return iter->second(sCmd);
    }

    // Search the command in the list of usual
    // commands
    auto iter = mCommands.find(sCommand);

    if (iter != mCommands.end())
        return iter->second(sCmd);

    // Now chek for the too generic "get"
    if (findCommand(sCmd, "get").sString == "get")
        return cmd_get(sCmd);

    // No command found
    return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This function finds the numerical
/// argument to the selected command line
/// parameter and evaluates it.
///
/// \param sCmd const string&
/// \param nPos size_t
/// \param _parser Parser&
/// \param nArgument size_t&
/// \return bool
///
/////////////////////////////////////////////////
static bool parseCmdArg(const string& sCmd, size_t nPos, Parser& _parser, size_t& nArgument)
{
	if (!sCmd.length() || !nPos)
		return false;

    while (nPos < sCmd.length() - 1 && sCmd[nPos] == ' ')
        nPos++;

    if (sCmd[nPos] == ' ' || nPos >= sCmd.length() - 1)
        return false;

    string sArg = sCmd.substr(nPos);

    if (sArg[0] == '(')
        sArg = sArg.substr(1, getMatchingParenthesis(sArg) - 1);
    else
        sArg = sArg.substr(0, sArg.find(' '));

    _parser.SetExpr(sArg);
    mu::Array res = _parser.Eval();

    if (mu::isnan(res.front()) || mu::isinf(_parser.Eval().front().getNum().asCF64()))
        return false;

    nArgument = abs(res.getAsScalarInt());
    return true;
}

