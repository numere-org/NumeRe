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
    else if (sCommand == "help" || sCommand == "man")
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

    // Try to find "data" in the command string
    // DECLARED AS DEPRECATED
    if (findCommand(sCmd, "data").sString == "data")
        return cmd_data(sCmd);

    // Get a reference to the datafile object
    Datafile& _data = NumeReKernel::getInstance()->getData();

    // Try to find any other table in the command
    // string
    // DECLARED AS DEPRECATED
	for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
	{
		if (findCommand(sCmd, iter->first).sString == iter->first)
            return cmd_tableAsCommand(sCmd, iter->first);
	}

	// No command found
	return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This function returns the string
/// argument for a single parameter in the
/// command line.
///
/// \param sCmd const string&
/// \param sArgument string&
/// \return bool
///
/// It will also parse it directly, which means
/// that it won't contain further string operations.
/////////////////////////////////////////////////
bool extractFirstParameterStringValue(const string& sCmd, string& sArgument)
{
    // Don't do anything, if no string is found in this expression
	if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		return false;

    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

	string sTemp = sCmd;

    // Get the contents of the contained data tables
	if (sTemp.find("data(") != string::npos || _data.containsTablesOrClusters(sTemp))
		getDataElements(sTemp, _parser, _data, _option);

	//
	for (unsigned int i = 0; i < sTemp.length(); i++)
	{
	    // Jump over this parenthesis, if its contents don't contain
	    // strings or string variables
		if (sTemp[i] == '('
            && !NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(i, getMatchingParenthesis(sTemp.substr(i))))
            && !NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(0, i)))
			i += getMatchingParenthesis(sTemp.substr(i));

		// Evaluate parameter starts, i.e. the minus sign of the command line
		if (sTemp[i] == '-'	&& !NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(0, i)))
		{
		    // No string left of the minus sign, erase this part
		    // and break the loop
			sTemp.erase(0, i);
			break;
		}
		else if (sTemp[i] == '-' && NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(0, i)))
		{
		    // There are strings left of the minus sign
		    // Find now the last string element in this part of the expression
			for (int j = (int)i; j >= 0; j--)
			{
			    // Find the start of this function or data element, which
			    // ends at this character
				if (sTemp[j] == '(' && j && (isalnum(sTemp[j - 1]) || sTemp[j - 1] == '_'))
				{
					while (j && (isalnum(sTemp[j - 1]) || sTemp[j - 1] == '_'))
						j--;
				}

				// This is now the location, where all string-related stuff is
				// to the right and everything else is to the left
				if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(0, j)) && NumeReKernel::getInstance()->getStringParser().isStringExpression(sTemp.substr(j, i - j)))
				{
				    // Erase the left part and break the loop
					sTemp.erase(0, j);
					break;
				}
			}

			break;
		}
	}

	// If there are no strings, sTemp will be empty
	if (!sTemp.length())
		return false;

    // Get the string variable values
	if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sTemp))
		NumeReKernel::getInstance()->getStringParser().getStringValues(sTemp);

    // Get now the string argument, which may contain pure
    // strings, string functions and concatenations
	if (!getStringArgument(sTemp, sArgument))
		return false;

    // If there are path tokens in the string part, ensure that
    // they are valid. Additionally, replace the "<this>" path token
	if (sArgument.find('<') != string::npos && sArgument.find('>', sArgument.find('<')) != string::npos)
	{
		for (unsigned int i = 0; i < sArgument.length(); i++)
		{
			if (sArgument.find('<', i) == string::npos)
				break;

			if (sArgument[i] == '<' && sArgument.find('>', i) != string::npos)
			{
				string sToken = sArgument.substr(i, sArgument.find('>', i) + 1 - i);

				if (sToken == "<this>")
					sToken = _option.getExePath();

				if (sToken.find('/') == string::npos)
				{
				    // Is the token valid?
					if (_option.getTokenPaths().find(sToken) == string::npos)
						throw SyntaxError(SyntaxError::UNKNOWN_PATH_TOKEN, sCmd, sToken, sToken);
				}

				i = sArgument.find('>', i);
			}
		}
	}

	// Clear the temporary variable
	sTemp.clear();

	// Parse the string expression
    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sArgument, sTemp, true);
    sArgument = sArgument.substr(1, sArgument.length() - 2);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function evaluates a passed
/// parameter string, so that the values of the
/// parameters are only values. No expressions
/// exist after this call anymore.
///
/// \param sCmd const string&
/// \return string
///
/////////////////////////////////////////////////
string evaluateParameterValues(const string& sCmd)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

	string sReturn = sCmd;
	string sTemp = "";
	string sDummy = "";
	unsigned int nPos = 0;
	unsigned int nLength = 0;
	vector<double> vInterval;

	// Add a whitespace character at the end
	if (sReturn.back() != ' ')
		sReturn += " ";

    // Try to detect the interval syntax
	if (sReturn.find('-') != string::npos
        && (sReturn.find('[') != string::npos
            || matchParams(sReturn, "x", '=')
            || matchParams(sReturn, "y", '=')
            || matchParams(sReturn, "z", '=')))
	{
	    // Get the parameter part of the string and remove
	    // the parameter string part from the original expression
		if (sReturn.find("-set") != string::npos)
		{
			sTemp = sReturn.substr(sReturn.find("-set"));
			sReturn.erase(sReturn.find("-set"));
		}
		else if (sReturn.find("--") != string::npos)
		{
			sTemp = sReturn.substr(sReturn.find("--"));
			sReturn.erase(sReturn.find("--"));
		}
		else
		{
			sTemp = sReturn.substr(sReturn.find('-'));
			sReturn.erase(sReturn.find("-"));
		}

		// Parse the interval syntax
		vInterval = parser_IntervalReader(sTemp, _parser, _data, _functions, _option, true);

		// Append the remaining part of the parameter string to the expression
		sReturn += sTemp;
	}

	// Get the string var values, if any
	if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sReturn))
		NumeReKernel::getInstance()->getStringParser().getStringValues(sReturn);

	// Repeat as long as an equal sign is found after
	// the current position in the command line
	while (sReturn.find('=', nPos) != string::npos)
	{
	    // Get the position after the equal sign
		nPos = sReturn.find('=', nPos) + 1;

		// Ignore equal signs in strings
		if (isInQuotes(sReturn, nPos))
        {
            nPos++;
            continue;
        }

		// jump over whitespaces
		while (nPos < sReturn.length() - 1 && sReturn[nPos] == ' ')
			nPos++;

        // Parse the parameter values into evaluated values for the commands
		if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos)))
		{
		    // This is a string value
			if (!getStringArgument(sReturn.substr(nPos - 1), sTemp)) // mit "=" uebergeben => fixes getStringArgument issues
				return "";

			// Get the current length of the string
			nLength = sTemp.length();
			sTemp += " -kmq";

			// Parse the string
			NumeReKernel::getInstance()->getStringParser().evalAndFormat(sTemp, sDummy, true);

            // Replace the parsed string
			sReturn.replace(nPos, nLength, sTemp);
		}
		else if ((nPos > 5 && sReturn.substr(nPos - 5, 5) == "save=")
			|| (nPos > 7 && sReturn.substr(nPos - 7, 7) == "export="))
		{
		    // This is a path value without quotation marks
		    // (otherwise it would be catched by the previous block)
			sTemp = sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos);
			nLength = sTemp.length();

			// Add quotation marks and replace the prvious path definition
			sTemp = "\"" + sTemp + "\"";
			sReturn.replace(nPos, nLength, sTemp);
		}
		else if ((nPos > 8 && sReturn.substr(nPos - 8, 8) == "tocache=")
			|| (nPos > 5 && sReturn.substr(nPos - 5, 5) == "type=")
            || (nPos > 5 && sReturn.substr(nPos - 5, 5) == "icon=")
            || (nPos > 8 && sReturn.substr(nPos - 8, 8) == "buttons="))
		{
		    // do nothing here
			nPos++;
		}
		else
		{
		    // All other cases, i.e. numerical values
		    // evaluate the value correspondingly

		    // Get the value and its length
			sTemp = sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos);
			nLength = sTemp.length();

			// Call functions
			if (!_functions.call(sTemp))
				return "";

            // Get data elements
			if (sTemp.find("data(") != string::npos || _data.containsTablesOrClusters(sTemp))
				getDataElements(sTemp, _parser, _data, _option);

            int nResult = 0;
            value_type* v = nullptr;

            // If the string contains a colon operator,
            // replace it with a comma
            if (sTemp.find(':') != string::npos)
            {
                string sTemp_2 = sTemp;
				sTemp = getNextIndex(sTemp_2, true);
				sTemp += ", " + sTemp_2;
            }

            // Set the expression and evaluate it numerically
            _parser.SetExpr(sTemp);
            v = _parser.Eval(nResult);

            // Clear the temporary variable
            sTemp.clear();

            // convert the doubles into strings and remove the trailing comma
            for (int i = 0; i < nResult; i++)
                sTemp += toString(v[i], _option) + ":";

            sTemp.pop_back();

            // Replace the string
			sReturn.replace(nPos, nLength, sTemp);
		}

	}

	// Convert the calculated intervals into their string definitions
	if (vInterval.size())
	{
	    // x interval
		if (vInterval.size() >= 2)
		{
			if (!isnan(vInterval[0]) && !isnan(vInterval[1]))
				sReturn += " -x=" + toString(vInterval[0], 7) + ":" + toString(vInterval[1], 7);
		}

		// y interval
		if (vInterval.size() >= 4)
		{
			if (!isnan(vInterval[2]) && !isnan(vInterval[3]))
				sReturn += " -y=" + toString(vInterval[2], 7) + ":" + toString(vInterval[3], 7);
		}

		// z interval
		if (vInterval.size() >= 6)
		{
			if (!isnan(vInterval[4]) && !isnan(vInterval[5]))
				sReturn += " -z=" + toString(vInterval[4], 7) + ":" + toString(vInterval[5], 7);
		}
	}

	return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function finds the numerical
/// argument to the selected command line
/// parameter and evaluates it.
///
/// \param sCmd const string&
/// \param sParam const string&
/// \param _parser Parser&
/// \param nArgument int&
/// \return bool
///
/////////////////////////////////////////////////
bool parseCmdArg(const string& sCmd, const string& sParam, Parser& _parser, int& nArgument)
{
	if (!sCmd.length() || !sParam.length())
		return false;

	unsigned int nPos = 0;

	if (matchParams(sCmd, sParam) || matchParams(sCmd, sParam, '='))
	{
		if (matchParams(sCmd, sParam))
			nPos = matchParams(sCmd, sParam) + sParam.length();
		else
			nPos = matchParams(sCmd, sParam, '=') + sParam.length();

		while (sCmd[nPos] == ' ' && nPos < sCmd.length() - 1)
			nPos++;

		if (sCmd[nPos] == ' ' || nPos >= sCmd.length() - 1)
			return false;

		string sArg = sCmd.substr(nPos);

		if (sArg[0] == '(')
			sArg = sArg.substr(1, getMatchingParenthesis(sArg) - 1);
		else
			sArg = sArg.substr(0, sArg.find(' '));

		_parser.SetExpr(sArg);

		if (isnan(_parser.Eval()) || isinf(_parser.Eval()))
			return false;

		nArgument = intCast(_parser.Eval());
		return true;
	}

	return false;
}

