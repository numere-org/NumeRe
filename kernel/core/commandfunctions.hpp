/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#ifndef COMMANDFUNCTIONS_HPP
#define COMMANDFUNCTIONS_HPP

#include <string>
#include <vector>
#include <map>
#include "built-in.hpp"
#include "maths/command_implementations.hpp"
#include "maths/matrixoperations.hpp"
#include "plotting/plotting.hpp"
#include "../kernel.hpp"

using namespace std;
using namespace mu;

typedef CommandReturnValues (*CommandFunc)(string&);

extern mglGraph _fontData;

string removeQuotationMarks(const string& sString);
static CommandReturnValues cmd_data(string& sCmd) __attribute__ ((deprecated));
static CommandReturnValues cmd_tableAsCommand(string& sCmd, const string& sCacheCmd) __attribute__ ((deprecated));


/////////////////////////////////////////////////
/// \brief This function returns a list of the
/// current defined variables either as strings
/// or as plain text.
///
/// \param sCmd const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return string
///
/////////////////////////////////////////////////
static string getVarList(const string& sCmd, Parser& _parser, MemoryManager& _data, Settings& _option)
{
	mu::varmap_type mNumVars = _parser.GetVar();
	map<string, string> mStringVars = NumeReKernel::getInstance()->getStringParser().getStringVars();
	map<string, int> mVars;

	string sSep = ", ";
	string sReturn = "";

	// Fill the vars map with the names and the
	// types of the variables
	for (auto iter = mNumVars.begin(); iter != mNumVars.end(); ++iter)
		mVars[iter->first] = 0;

	for (auto iter = mStringVars.begin(); iter != mStringVars.end(); ++iter)
		mVars[iter->first] = 1;

    // Change the separation characters, if the user
    // wants the return value to be a string
	if (findParameter(sCmd, "asstr"))
	{
		sSep = "\", \"";
		sReturn = "\"";
	}

	// Return all variables, when "vars" was passed
	if (findCommand(sCmd).sString == "vars")
	{
		for (auto iter = mVars.begin(); iter != mVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";

			if (iter->second)
			{
				if (findParameter(sCmd, "asstr"))
					sReturn += "\\\"" + mStringVars[iter->first] + "\\\"";
				else
					sReturn += "\"" + mStringVars[iter->first] + "\"";
			}
			else
				sReturn += toString(*mNumVars[iter->first], _option);

			sReturn += sSep;
		}
	}

	// Return only string variables, if "strings" was
	// passed
	if (findCommand(sCmd).sString == "strings")
	{
		for (auto iter = mStringVars.begin(); iter != mStringVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";

			if (findParameter(sCmd, "asstr"))
				sReturn += "\\\"" + iter->second + "\\\"";
			else
				sReturn += "\"" + iter->second + "\"";

			sReturn += sSep;
		}

		if (sReturn == "\"")
			return "\"\"";
	}

	// Return only numerical variables, if "nums"
	// was passed
	if (findCommand(sCmd).sString == "nums")
	{
		for (auto iter = mNumVars.begin(); iter != mNumVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";
			sReturn += toString(*iter->second, _option);
			sReturn += sSep;
		}
	}

	// Remove the trailing separation character
	if (findParameter(sCmd, "asstr") && sReturn.length() > 2)
		sReturn.erase(sReturn.length() - 3);
	else if (!findParameter(sCmd, "asstr") && sReturn.length() > 1)
		sReturn.erase(sReturn.length() - 2);

	return sReturn;
}


/////////////////////////////////////////////////
/// \brief This static function handles the
/// undefinition process of custom defined
/// functions.
///
/// \param sFunctionList string
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool undefineFunctions(string sFunctionList, FunctionDefinitionManager& _functions, const Settings& _option)
{
    string sSuccessFulRemoved;

    // As long as the list of passed functions has a length,
    // undefine the current first argument of the list
    while (sFunctionList.length())
    {
        string sFunction = getNextArgument(sFunctionList, true);

        // Try to undefine the functions
        if (!_functions.undefineFunc(sFunction))
            NumeReKernel::issueWarning(_lang.get("BUILTIN_CHECKKEYWORD_UNDEF_FAIL", sFunction));
        else
            sSuccessFulRemoved += sFunction + ", ";
    }

    // Inform the user that (some) of the functions were undefined
    if (_option.getSystemPrintStatus() && sSuccessFulRemoved.length())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_UNDEF_SUCCESS", sSuccessFulRemoved.substr(0, sSuccessFulRemoved.length()-2)));

    return true;
}


/////////////////////////////////////////////////
/// \brief This function creates new objects:
/// files, directories, procedures and tables
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool newObject(string& sCmd, Parser& _parser, MemoryManager& _data, Settings& _option)
{
	int nType = 0;
	string sObject = "";
	vector<string> vTokens;
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());

	if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
		NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    // Evaluate and prepare the passed parameters
	if (findParameter(sCmd, "dir", '='))
	{
		nType = 1;
		addArgumentQuotes(sCmd, "dir");
	}
	else if (findParameter(sCmd, "script", '='))
	{
		nType = 2;
		addArgumentQuotes(sCmd, "script");
	}
	else if (findParameter(sCmd, "proc", '='))
	{
		nType = 3;
		addArgumentQuotes(sCmd, "proc");
	}
	else if (findParameter(sCmd, "file", '='))
	{
		nType = 4;
		addArgumentQuotes(sCmd, "file");
	}
	else if (findParameter(sCmd, "plugin", '='))
	{
		nType = 5;
		addArgumentQuotes(sCmd, "plugin");
	}
	else if (findParameter(sCmd, "cache", '='))
	{
	    // DEPRECATED: Declared at v1.1.2rc2
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

		string sReturnVal = "";

		if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		{
			if (!extractFirstParameterStringValue(sCmd, sObject))
				return false;
		}
		else
			sObject = sCmd.substr(findParameter(sCmd, "cache", '=') + 5);

		StripSpaces(sObject);

		if (findParameter(sObject, "free"))
			eraseToken(sObject, "free", false);

		if (sObject.rfind('-') != string::npos)
			sObject.erase(sObject.rfind('-'));

		if (!sObject.length() || !getNextArgument(sObject, false).length())
			return false;

		while (sObject.length() && getNextArgument(sObject, false).length())
		{
			if (_data.isTable(getNextArgument(sObject, false)))
			{
				if (findParameter(sCmd, "free"))
				{
					string sTemp = getNextArgument(sObject, false);
					sTemp.erase(sTemp.find('('));
					_data.deleteBulk(sTemp, 0, _data.getLines(sTemp) - 1, 0, _data.getCols(sTemp) - 1);

					if (sReturnVal.length())
						sReturnVal += ", ";

					sReturnVal += "\"" + getNextArgument(sObject, false) + "\"";
				}

				getNextArgument(sObject, true);
				continue;
			}

			if (_data.addTable(getNextArgument(sObject, false), _option))
			{
				if (sReturnVal.length())
					sReturnVal += ", ";

				sReturnVal += "\"" + getNextArgument(sObject, true) + "\"";
				continue;
			}
			else
				return false;
		}

		if (sReturnVal.length() && _option.getSystemPrintStatus())
		{
			if (findParameter(sCmd, "free"))
				NumeReKernel::print(_lang.get("BUILTIN_NEW_FREE_CACHES", sReturnVal));
			else
				NumeReKernel::print(_lang.get("BUILTIN_NEW_CACHES", sReturnVal));
		}

		return true;
	}
	else if (sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3) != string::npos)
	{
		if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3)] == '$')
		{
		    // Insert the parameter for the new procedure
			nType = 3;
			sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3), "-proc=");
			addArgumentQuotes(sCmd, "proc");
		}
		else if (sCmd.find("()", findCommand(sCmd).nPos + 3) != string::npos)
		{
		    // Create new tables
			string sReturnVal = "";

			if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
			{
				if (!extractFirstParameterStringValue(sCmd, sObject))
					return false;
			}
			else
				sObject = sCmd.substr(findCommand(sCmd).nPos + 3);

			StripSpaces(sObject);

			if (findParameter(sObject, "free"))
				eraseToken(sObject, "free", false);

			if (sObject.rfind('-') != string::npos)
				sObject.erase(sObject.rfind('-'));

			if (!sObject.length())
				return false;

            // Create the tables
			while (sObject.length())
			{
			    string sTableName = getNextArgument(sObject, true);

			    // Does the table already exist?
				if (_data.isTable(sTableName))
				{
					if (findParameter(sCmd, "free"))
					{
						_data.deleteBulk(sTableName.substr(0, sTableName.find('(')), 0, _data.getLines(sTableName.substr(0, sTableName.find('('))) - 1, 0, _data.getCols(sTableName.substr(0, sTableName.find('('))) - 1);

						if (sReturnVal.length())
							sReturnVal += ", ";

						sReturnVal += "\"" + sTableName + "\"";
					}

					continue;
				}

				// Create a new table
				if (_data.addTable(sTableName, _option))
				{
					if (sReturnVal.length())
						sReturnVal += ", ";

					sReturnVal += "\"" + sTableName + "\"";
					continue;
				}
				else
					return false;
			}

			if (sReturnVal.length() && _option.getSystemPrintStatus())
			{
				if (findParameter(sCmd, "free"))
					NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_FREE_CACHES", sReturnVal), _option) );
				else
					NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_CACHES", sReturnVal), _option) );
			}

			return true;
		}
	}

	if (!nType)
		return false;

	extractFirstParameterStringValue(sCmd, sObject);
	StripSpaces(sObject);

	if (!sObject.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

	if (_option.getbDebug())
		NumeReKernel::print("DEBUG: sObject = " + sObject );

    // Create the objects
	if (nType == 1) // Directory
	{
		int nReturn = _fSys.setPath(sObject, true, _option.getExePath());

		if (nReturn == 1 && _option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_FOLDERCREATED", sObject), _option) );
	}
	else if (nType == 2) // Script template
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;

			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}

			_fSys.setPath(sPath, true, _option.getScriptPath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());

		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<scriptpath>/" + sObject;

		sObject = _fSys.ValidFileName(sObject, ".nscr");
		vTokens.push_back(sObject.substr(sObject.rfind('/') + 1, sObject.rfind('.') - sObject.rfind('/') - 1));
		vTokens.push_back(getTimeStamp(false));

		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_script.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_script.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sObject, sObject);
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_script.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sObject, sObject);
		}

		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_SCRIPTCREATED", sObject), _option) );
	}
	else if (nType == 3) // Procedure template
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos || sObject.find('~') != string::npos)
		{
			string sPath = sObject;

			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/' || sPath[i] == '~')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}

			while (sPath.find('~') != string::npos)
				sPath[sPath.find('~')] = '/';

			while (sPath.find('$') != string::npos)
				sPath.erase(sPath.find('$'), 1);

			_fSys.setPath(sPath, true, _option.getProcsPath());
		}
		else
			_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());

		string sProcedure = sObject;

		if (sProcedure.find('$') != string::npos)
		{
			sProcedure = sProcedure.substr(sProcedure.rfind('$'));

			if (sProcedure.find('~') != string::npos)
				sProcedure.erase(1, sProcedure.rfind('~'));
		}
		else
		{
			if (sProcedure.find('~') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('~') + 1);

			if (sProcedure.find('\\') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('\\') + 1);

			if (sProcedure.find('/') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('/') + 1);

			StripSpaces(sProcedure);
			sProcedure = "$" + sProcedure;
		}

		if (sProcedure.find('.') != string::npos)
			sProcedure = sProcedure.substr(0, sProcedure.rfind('.'));

		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<procpath>/" + sObject;

		while (sObject.find('~') != string::npos)
			sObject[sObject.find('~')] = '/';

		while (sObject.find('$') != string::npos)
			sObject.erase(sObject.find('$'), 1);

		sObject = _fSys.ValidFileName(sObject, ".nprc");

		vector<string> vTokens;
		vTokens.push_back(sProcedure.substr(1));
		vTokens.push_back(getTimeStamp(false));

		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_procedure.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_procedure.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_PROCEDURE, sCmd, SyntaxError::invalid_position, sObject);
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_procedure.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_PROCEDURE, sCmd, SyntaxError::invalid_position, sObject);
		}

		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_PROCCREATED", sObject), _option) );
	}
	else if (nType == 4) // Arbitrary file template
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;

			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}

			_fSys.setPath(sPath, true, _option.getExePath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());

		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<>/" + sObject;

		sObject = _fSys.ValidFileName(sObject, ".txt");

		if (sObject.substr(sObject.rfind('.')) == ".nprc"
				|| sObject.substr(sObject.rfind('.')) == ".nscr"
				|| sObject.substr(sObject.rfind('.')) == ".ndat")
			sObject.replace(sObject.rfind('.'), 5, ".txt");

		vTokens.push_back(sObject.substr(sObject.rfind('/') + 1, sObject.rfind('.') - sObject.rfind('/') - 1));
		vTokens.push_back(getTimeStamp(false));

		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_file.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_file.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, sCmd, SyntaxError::invalid_position, sObject);
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_file.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, sCmd, SyntaxError::invalid_position, sObject);
		}

		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_FILECREATED", sObject), _option) );
	}
	else if (nType == 5) // Plugin template
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;
			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}

			_fSys.setPath(sPath, true, _option.getScriptPath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());

		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<scriptpath>/" + sObject;

		sObject = _fSys.ValidFileName(sObject, ".nscr");

		if (sObject.substr(sObject.rfind('/') + 1, 5) != "plgn_")
			sObject.insert(sObject.rfind('/') + 1, "plgn_");

		while (sObject.find(' ', sObject.rfind('/')) != string::npos)
			sObject.erase(sObject.find(' ', sObject.rfind('/')), 1);

		string sPluginName = sObject.substr(sObject.rfind("plgn_") + 5, sObject.rfind('.') - sObject.rfind("plgn_") - 5);
		vTokens.push_back(sPluginName);
		vTokens.push_back(getTimeStamp(false));

		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_plugin.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_plugin.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, SyntaxError::invalid_position, sObject);
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_plugin.nlng", vTokens, _option))
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, SyntaxError::invalid_position, sObject);
		}

		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_PLUGINCREATED", sPluginName, sObject), _option) );
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This function opens the object in the
/// editor to edit its contents.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool editObject(string& sCmd, Parser& _parser, MemoryManager& _data, Settings& _option)
{
	int nType = 0;
	int nFileOpenFlag = 0;

	if (findParameter(sCmd, "norefresh"))
		nFileOpenFlag = 1;

	if (findParameter(sCmd, "refresh"))
		nFileOpenFlag = 2 | 4;

	string sObject;

	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		extractFirstParameterStringValue(sCmd, sObject);
	else
	{
		sObject = sCmd.substr(findCommand(sCmd).sString.length());

		// remove flags from object
		if (nFileOpenFlag)
			sObject.erase(sObject.rfind('-'));
	}

	StripSpaces(sObject);
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());

	if (sObject.find('.') != string::npos)
		_fSys.declareFileType(sObject.substr(sObject.rfind('.')));

	if (!sObject.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

	if (sObject[0] == '$'  && sObject[1] != '\'')
		sObject = "<procpath>/" + sObject.substr(1);
	else if (sObject[0] == '$')
		sObject.erase(0, 1);

	while (sObject.find('~') != string::npos)
		sObject[sObject.find('~')] = '/';

	while (sObject.find('$') != string::npos)
		sObject.erase(sObject.find('$'), 1);

	if (sObject[0] == '\'' && sObject[sObject.length() - 1] == '\'')
		sObject = sObject.substr(1, sObject.length() - 2);

    // Resolve the paths
	if (sObject.find("<loadpath>") != string::npos || sObject.find(_option.getLoadPath()) != string::npos)
	{
		_fSys.setPath(_option.getLoadPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (sObject.find("<savepath>") != string::npos || sObject.find(_option.getSavePath()) != string::npos)
	{
		_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (sObject.find("<scriptpath>") != string::npos || sObject.find(_option.getScriptPath()) != string::npos)
	{
		_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nscr");
	}
	else if (sObject.find("<plotpath>") != string::npos || sObject.find(_option.getPlotOutputPath()) != string::npos)
	{
		_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".png");
	}
	else if (sObject.find("<procpath>") != string::npos || sObject.find(_option.getProcsPath()) != string::npos)
	{
		_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nprc");
	}
	else if (sObject.find("<wp>") != string::npos || sObject.find(_option.getWorkPath()) != string::npos)
	{
		_fSys.setPath(_option.getWorkPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nprc");
	}
	else if (sObject.find("<>") != string::npos || sObject.find("<this>") != string::npos || sObject.find(_option.getExePath()) != string::npos)
	{
		_fSys.setPath(_option.getExePath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (!_data.containsTablesOrClusters(sObject))
	{
	    // Is probably a folder path to be edited in the Windows Explorer
		if (sObject.find('.') == string::npos && (sObject.find('/') != string::npos || sObject.find('\\') != string::npos))
		{
			ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL);
			return true;
		}

		// Append a wildcard at the end of the path if necessary
		if (sObject[sObject.length() - 1] != '*' && sObject.find('.') == string::npos)
			sObject += "*";

        // Try to determine the path based upon the file extension, where we might find the
        // file, if the user did not supply the path to the file
		if (sObject.find('.') != string::npos)
		{
			if (sObject.substr(sObject.rfind('.')) == ".dat" || sObject.substr(sObject.rfind('.')) == ".txt")
			{
				_fSys.setPath(_option.getLoadPath(), false, _option.getExePath());
				string sTemporaryObjectName = _fSys.ValidFileName(sObject, ".dat");

				if (!fileExists(sTemporaryObjectName))
					_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
			}
			else if (sObject.substr(sObject.rfind('.')) == ".nscr")
				_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".nprc")
				_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".png"
					 || sObject.substr(sObject.rfind('.')) == ".gif"
					 || sObject.substr(sObject.rfind('.')) == ".svg"
					 || sObject.substr(sObject.rfind('.')) == ".eps")
				_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".tex")
			{
				_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
				string sTemporaryObjectName = _fSys.ValidFileName(sObject, ".tex");

				if (!fileExists(sTemporaryObjectName))
					_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
			}
			else if (sObject.substr(sObject.rfind('.')) == ".nhlp")
				_fSys.setPath(_option.getExePath() + "/docs", false, _option.getExePath());
			else
				_fSys.setPath(_option.getExePath(), false, _option.getExePath());
		}
		else
			_fSys.setPath(_option.getExePath(), false, _option.getExePath());

		sObject = _fSys.ValidFileName(sObject, ".dat");
	}

	// Is probably a folder path
	if (!_data.containsTablesOrClusters(sObject) && sObject.find('.') == string::npos && (sObject.find('/') != string::npos || sObject.find('\\') != string::npos))
	{
		ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}

	// Open the table for editing
	if (_data.containsTables(sObject))
	{
		StripSpaces(sObject);
		string sTableName = sObject.substr(0, sObject.find('('));

		NumeReKernel::showTable(_data.extractTable(sTableName), sTableName, true);
		NumeReKernel::printPreFmt("|-> " + _lang.get("BUILTIN_WAITINGFOREDIT") + " ... ");

		NumeRe::Table _table = NumeReKernel::getTable();
		NumeReKernel::printPreFmt(_lang.get("COMMON_DONE") + ".\n");

		if (_table.isEmpty())
            return true;

        _data.importTable(_table, sTableName);
        return true;
	}

	// Could be a folder -> open it in the Windows Explorer
	if (!fileExists(sObject) || sObject.find('.') == string::npos)
	{
		sObject.erase(sObject.rfind('.'));

		if (sObject.find('*') != string::npos)
			sObject.erase(sObject.rfind('*'));

		if ((int)ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32)
			return true;

		throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, SyntaxError::invalid_position, sObject);
	}

	// Determine the file type of the file to be edited
	if (sObject.substr(sObject.rfind('.')) == ".dat"
			|| sObject.substr(sObject.rfind('.')) == ".txt"
			|| sObject.substr(sObject.rfind('.')) == ".tex"
			|| sObject.substr(sObject.rfind('.')) == ".csv"
			|| sObject.substr(sObject.rfind('.')) == ".labx"
			|| sObject.substr(sObject.rfind('.')) == ".jdx"
			|| sObject.substr(sObject.rfind('.')) == ".jcm"
			|| sObject.substr(sObject.rfind('.')) == ".dx"
			|| sObject.substr(sObject.rfind('.')) == ".nscr"
			|| sObject.substr(sObject.rfind('.')) == ".nprc"
			|| sObject.substr(sObject.rfind('.')) == ".nhlp"
			|| sObject.substr(sObject.rfind('.')) == ".png"
			|| sObject.substr(sObject.rfind('.')) == ".gif"
			|| sObject.substr(sObject.rfind('.')) == ".m"
			|| sObject.substr(sObject.rfind('.')) == ".cpp"
			|| sObject.substr(sObject.rfind('.')) == ".cxx"
			|| sObject.substr(sObject.rfind('.')) == ".c"
			|| sObject.substr(sObject.rfind('.')) == ".hpp"
			|| sObject.substr(sObject.rfind('.')) == ".hxx"
			|| sObject.substr(sObject.rfind('.')) == ".h"
			|| sObject.substr(sObject.rfind('.')) == ".log")
		nType = 1;
	else if (sObject.substr(sObject.rfind('.')) == ".svg"
			 || sObject.substr(sObject.rfind('.')) == ".eps")
		nType = 2;

	if (!nType)
		throw SyntaxError(SyntaxError::CANNOT_EDIT_FILE_TYPE, sCmd, SyntaxError::invalid_position, sObject);

	if (nType == 1)
	{
		NumeReKernel::nOpenFileFlag = nFileOpenFlag;
		NumeReKernel::gotoLine(sObject);
	}
	else if (nType == 2)
		openExternally(sObject, _option.getViewerPath(), _option.getExePath());

	return true;
}


/////////////////////////////////////////////////
/// \brief This function lists all internal
/// (kernel) settings.
///
/// \param _option Settings&
/// \return void
/// \deprecated Will be removed at v1.1.3rc1
///
/////////////////////////////////////////////////
static void listOptions(Settings& _option)
{
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("BUILTIN_LISTOPT_SETTINGS")) );
	make_hline();
	NumeReKernel::print(  toSystemCodePage(_lang.get("BUILTIN_LISTOPT_1")) + "\n|" );

	// List the path settings
	NumeReKernel::printPreFmt(sectionHeadline(_lang.get("BUILTIN_LISTOPT_2")));
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_3", _option.getSavePath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_4", _option.getLoadPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_5", _option.getScriptPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_6", _option.getProcsPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_7", _option.getPlotOutputPath()), _option, true, 0, 25) + "\n" );
	if (_option.getViewerPath().length())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_8", _option.getViewerPath()), _option, true, 0, 25) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_8", _lang.get("BUILTIN_LISTOPT_NOVIEWER")), _option, true, 0, 25) + "\n");
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_9", _option.getEditorPath()), _option, true, 0, 25) + "\n");
	NumeReKernel::printPreFmt("|\n" );

	// List all other settings
	NumeReKernel::printPreFmt(sectionHeadline(_lang.get("BUILTIN_LISTOPT_10")));

	// Autosaveintervall
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_11", toString(_option.getAutoSaveInterval())), _option) + "\n");

	// Greeting
	if (_option.getbGreeting())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_12", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_12", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Buffer
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_13", toString(_option.getBuffer(1))), _option) + "\n");

	// Draftmode
	if (_option.getbUseDraftMode())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_15", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_15", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Extendedfileinfo
	if (_option.getbShowExtendedFileInfo())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_16", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_16", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// ESC in Scripts
	if (_option.getbUseESCinScripts())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_17", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_17", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Defcontrol
	if (_option.getbDefineAutoLoad())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_19", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_19", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Compact table view in the terminal
	if (_option.getbCompact())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_20", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_20", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Loading empty columns
	if (_option.getbLoadEmptyCols())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_21", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_21", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Precision
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_22", toString(_option.getPrecision())), _option) + "\n");

	// Create a logfile of the terminal inputs
	if (_option.getbUseLogFile())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_23", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_23", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Default Plotfont
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_25", _option.getDefaultPlotFont()), _option) + "\n");

	// Display Hints
	if (_option.getbShowHints())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_26", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_26", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Use UserLangFiles
	if (_option.getUseCustomLanguageFiles())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_27", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_27", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	//  Use the ExternalDocViewer
	if (_option.getUseExternalViewer())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_28", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_28", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	NumeReKernel::printPreFmt("|\n" );
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LISTOPT_FOOTNOTE"), _option) );
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function displays the contents of
/// a single directory directly in the terminal.
///
/// \param sDir const string&
/// \param sParams const string&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool listDirectory(const string& sDir, const string& sParams, const Settings& _option)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	LARGE_INTEGER Filesize;
	double dFilesize = 0.0;
	double dFilesizeTotal = 0.0;
	string sConnect = "";
	string sPattern = "*";
	string sFilesize = " Bytes";
	string sFileName = "";
	string sDirectory = "";
	int nLength = 0;
	int nCount[2] = {0, 0};
	unsigned int nFirstColLength = _option.getWindow() / 2 - 6;
	bool bOnlyDir = false;

	if (findParameter(sParams, "dir"))
		bOnlyDir = true;

	if (findParameter(sParams, "pattern", '=') || findParameter(sParams, "p", '='))
	{
		int nPos = 0;

		if (findParameter(sParams, "pattern", '='))
			nPos = findParameter(sParams, "pattern", '=') + 7;
		else
			nPos = findParameter(sParams, "p", '=') + 1;

		sPattern = getArgAtPos(sParams, nPos);
		StripSpaces(sPattern);

		if (!sPattern.length())
			sPattern = "*";
	}

	for (int n = 0; n < 2; n++)
	{
		if (bOnlyDir && n)
			break;

		if (sDir == "LOADPATH")
		{
			hFind = FindFirstFile((_option.getLoadPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getLoadPath();
		}
		else if (sDir == "SAVEPATH")
		{
			hFind = FindFirstFile((_option.getSavePath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getSavePath();
		}
		else if (sDir == "PLOTPATH")
		{
			hFind = FindFirstFile((_option.getPlotOutputPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getPlotOutputPath();
		}
		else if (sDir == "SCRIPTPATH")
		{
			hFind = FindFirstFile((_option.getScriptPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getScriptPath();
		}
		else if (sDir == "PROCPATH")
		{
			hFind = FindFirstFile((_option.getProcsPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getProcsPath();
		}
		else if (sDir == "WORKPATH")
		{
			hFind = FindFirstFile((_option.getWorkPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getWorkPath();
		}
		else
		{
			if (sDir[0] == '.')
			{
				hFind = FindFirstFile((_option.getExePath() + "\\" + sDir + "\\" + sPattern).c_str(), &FindFileData);
				sDirectory = _option.getExePath() + "/" + sDir;
			}
			else if (sDir[0] == '<')
			{
				if (sDir.substr(0, 10) == "<loadpath>")
				{
					hFind = FindFirstFile((_option.getLoadPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getLoadPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 10) == "<savepath>")
				{
					hFind = FindFirstFile((_option.getSavePath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getSavePath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 12) == "<scriptpath>")
				{
					hFind = FindFirstFile((_option.getScriptPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getScriptPath() + sDir.substr(12);
				}
				else if (sDir.substr(0, 10) == "<plotpath>")
				{
					hFind = FindFirstFile((_option.getPlotOutputPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getPlotOutputPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 10) == "<procpath>")
				{
					hFind = FindFirstFile((_option.getProcsPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getProcsPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 4) == "<wp>")
				{
					hFind = FindFirstFile((_option.getWorkPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getWorkPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 2) == "<>" || sDir.substr(0, 6) == "<this>")
				{
					hFind = FindFirstFile((_option.getExePath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getExePath() + sDir.substr(sDir.find('>') + 1);
				}
			}
			else
			{
				hFind = FindFirstFile((sDir + "\\" + sPattern).c_str(), &FindFileData);
				sDirectory = sDir;
			}
		}

		if (hFind == INVALID_HANDLE_VALUE)
			return false;

		do
		{
			sFilesize = " Bytes";
			sConnect = "|   ";
			sConnect += FindFileData.cFileName;
			sFileName = sDirectory + "/" + FindFileData.cFileName;

			if (sConnect.length() + 3 > nFirstColLength) //31
				sConnect = sConnect.substr(0, nFirstColLength - 14) + "..." + sConnect.substr(sConnect.length() - 8); //20

			nLength = sConnect.length();

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (n)
					continue;

                // Ignore parent and current directory placeholders
				if (sConnect.substr(sConnect.length() - 2) == ".." || sConnect.substr(sConnect.length() - 1) == ".")
					continue;

				nCount[1]++;
				sConnect += "  (...)";
				sConnect.append(nFirstColLength - 1 - nLength, ' ');
				sConnect += "<" + _lang.get("BUILTIN_LISTFILES_CUSTOMPATH") + ">";
			}
			else if (!bOnlyDir && n)
			{
				nCount[0]++;
				Filesize.LowPart = FindFileData.nFileSizeLow;
				Filesize.HighPart = FindFileData.nFileSizeHigh;
				string sExt = "";

				if (sConnect.find('.') != string::npos)
					sExt = toLowerCase(sConnect.substr(sConnect.rfind('.'), sConnect.find(' ', sConnect.rfind('.')) - sConnect.rfind('.')));

				sConnect.append(nFirstColLength + 7 - nLength, ' ');

				// Get the language string for the current file type
				if (!sExt.length())
					sConnect += _lang.get("COMMON_FILETYPE_NOEXT");
				else if (sExt == ".dx" || sExt == ".jcm")
					sConnect += _lang.get("COMMON_FILETYPE_JDX");
				else if (sExt == ".wave")
					sConnect += _lang.get("COMMON_FILETYPE_WAV");
                else
                {
                    sExt = _lang.get("COMMON_FILETYPE_" + toUpperCase(sExt.substr(1)));

                    if (sExt.find("COMMON_FILETYPE_") != string::npos)
                        sConnect += sExt.substr(sExt.rfind('_')+1) + "-" + _lang.get("COMMON_FILETYPE_NOEXT");
                    else
                        sConnect += sExt;
                }

                // Create the file size string
				dFilesize = (double)Filesize.QuadPart;
				dFilesizeTotal += dFilesize;

				if (dFilesize / 1000.0 >= 1)
				{
					dFilesize /= 1024.0;
					sFilesize = "KBytes";

					if (dFilesize / 1000.0 >= 1)
					{
						dFilesize /= 1024.0;
						sFilesize = "MBytes";

						if (dFilesize / 1000.0 >= 1)
						{
							dFilesize /= 1024.0;
							sFilesize = "GBytes";
						}
					}
				}

				sFilesize = toString(dFilesize, 3) + " " + sFilesize;
				sConnect.append(_option.getWindow() - sConnect.length() - sFilesize.length(), ' ');
				sConnect += sFilesize;

				if (sExt == ".ndat" && _option.getbShowExtendedFileInfo())
				{
					sConnect += "$     ";
					sConnect += getFileInfo(sFileName);
				}
			}
			else
				continue;

			NumeReKernel::printPreFmt(sConnect + "\n");
		}
		while (FindNextFile(hFind, &FindFileData) != 0);
	}

	FindClose(hFind);

    // Create the byte sum string for the whole list
	if (nCount[0])
	{
		sFilesize = " Bytes";

		if (dFilesizeTotal / 1000.0 >= 1)
		{
			dFilesizeTotal /= 1024.0;
			sFilesize = "KBytes";

			if (dFilesizeTotal / 1000.0 >= 1)
			{
				dFilesizeTotal /= 1024.0;
				sFilesize = "MBytes";

				if (dFilesizeTotal / 1000.0 >= 1)
				{
					dFilesizeTotal /= 1024.0;
					sFilesize = "GBytes";
				}
			}
		}

		sFilesize = "Total: " + toString(dFilesizeTotal, 3) + " " + sFilesize;
	}
	else
		sFilesize = "";

	string sSummary = "-- " + _lang.get("BUILTIN_LISTFILES_SUMMARY", toString(nCount[0]), toString(nCount[1])) + " --";
	sSummary.append(_option.getWindow() - sSummary.length() - 4 - sFilesize.length(), ' ');
	sSummary += sFilesize;

	if (bOnlyDir)
	{
		if (nCount[1])
			NumeReKernel::printPreFmt("|   -- " + _lang.get("BUILTIN_LISTFILES_DIR_SUMMARY", toString(nCount[1])) + " --\n");
		else
			NumeReKernel::printPreFmt("|   -- " + _lang.get("BUILTIN_LISTFILES_NODIRS") + " --\n");
	}
	else
		NumeReKernel::printPreFmt("|   " + sSummary + "\n");

	return true;
}


/////////////////////////////////////////////////
/// \brief This static function draws the headers
/// for the listed directories.
///
/// \param sPathName const string&
/// \param sLangString const string&
/// \param nWindowLength size_t
/// \return string
///
/////////////////////////////////////////////////
static string createListDirectoryHeader(const string& sPathName, const string& sLangString, size_t nWindowLength)
{
    size_t nFirstColLength = nWindowLength / 2 - 6;
    string sHeader = sPathName + "  ";

    if (sHeader.length() > nFirstColLength)
    {
        sHeader += "$";
        sHeader.append(nFirstColLength, '-');
    }
    else
        sHeader.append(nFirstColLength - sHeader.length(), '-');

    sHeader += "  <" + toUpperCase(sLangString) + ">  ";

    if (sHeader.find('$') != string::npos)
        sHeader.append(nWindowLength - 4 - sHeader.length() + sHeader.rfind('$'), '-');
    else
        sHeader.append(nWindowLength - 4 - sHeader.length(), '-');

    return sHeader;
}


/////////////////////////////////////////////////
/// \brief This function handles the display of
/// the contents of the selected folders directly
/// in the terminal.
///
/// \param sCmd const string&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool listFiles(const string& sCmd, const Settings& _option)
{
	string sConnect = "";
	string sSpecified = "";
	string __sCmd = sCmd + " ";
	string sPattern = "";
	unsigned int nFirstColLength = _option.getWindow() / 2 - 6;
	bool bFreePath = false;

	// Extract a search pattern
	if (findParameter(__sCmd, "pattern", '=') || findParameter(__sCmd, "p", '='))
	{
		int nPos = 0;

		if (findParameter(__sCmd, "pattern", '='))
			nPos = findParameter(__sCmd, "pattern", '=') + 7;
		else
			nPos = findParameter(__sCmd, "p", '=') + 1;

		sPattern = getArgAtPos(__sCmd, nPos);
		StripSpaces(sPattern);

		if (sPattern.length())
			sPattern = _lang.get("BUILTIN_LISTFILES_FILTEREDFOR", sPattern);
	}

	// Write the headline
	make_hline();
	sConnect = "NUMERE: " + toUpperCase(_lang.get("BUILTIN_LISTFILES_EXPLORER"));

	if (sConnect.length() > nFirstColLength + 6)
		sConnect += "    ";
	else
		sConnect.append(nFirstColLength + 6 - sConnect.length(), ' ');

	NumeReKernel::print(LineBreak(sConnect + sPattern, _option, true, 0, sConnect.length()) );
	make_hline();

	// Find the specified folder
	if (findParameter(__sCmd, "files", '='))
	{
		int nPos = findParameter(__sCmd, "files", '=') + 5;
		sSpecified = getArgAtPos(__sCmd, nPos);
		StripSpaces(sSpecified);

		if (sSpecified[0] == '<' && sSpecified[sSpecified.length() - 1] == '>' && sSpecified != "<>" && sSpecified != "<this>")
		{
			sSpecified = sSpecified.substr(1, sSpecified.length() - 2);
			sSpecified = toLowerCase(sSpecified);

			if (sSpecified != "loadpath" && sSpecified != "savepath" && sSpecified != "plotpath" && sSpecified != "scriptpath" && sSpecified != "procpath" && sSpecified != "wp")
				sSpecified = "";
		}
		else
			bFreePath = true;
	}

	// Write the headers and list the directories
	if (!bFreePath)
	{
		if (!sSpecified.length() || sSpecified == "loadpath")
		{
		    NumeReKernel::print(createListDirectoryHeader(_option.getLoadPath(), _lang.get("BUILTIN_LISTFILES_LOADPATH"), _option.getWindow()));

			if (!listDirectory("LOADPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}

		if (!sSpecified.length() || sSpecified == "savepath")
		{
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );

		    NumeReKernel::print(createListDirectoryHeader(_option.getSavePath(), _lang.get("BUILTIN_LISTFILES_SAVEPATH"), _option.getWindow()));

			if (!listDirectory("SAVEPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}

		if (!sSpecified.length() || sSpecified == "scriptpath")
		{
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );

		    NumeReKernel::print(createListDirectoryHeader(_option.getScriptPath(), _lang.get("BUILTIN_LISTFILES_SCRIPTPATH"), _option.getWindow()));

			if (!listDirectory("SCRIPTPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}

		if (!sSpecified.length() || sSpecified == "procpath")
		{
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );

		    NumeReKernel::print(createListDirectoryHeader(_option.getProcsPath(), _lang.get("BUILTIN_LISTFILES_PROCPATH"), _option.getWindow()));

			if (!listDirectory("PROCPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}

		if (!sSpecified.length() || sSpecified == "plotpath")
		{
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );

		    NumeReKernel::print(createListDirectoryHeader(_option.getPlotOutputPath(), _lang.get("BUILTIN_LISTFILES_PLOTPATH"), _option.getWindow()));

			if (!listDirectory("PLOTPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}

		if (sSpecified == "wp")
		{
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );

		    NumeReKernel::print(createListDirectoryHeader(_option.getWorkPath(), _lang.get("BUILTIN_LISTFILES_WORKPATH"), _option.getWindow()));

			if (!listDirectory("WORKPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
	}
	else
	{
		sSpecified = fromSystemCodePage(sSpecified);

	    if (sSpecified == "<>" || sSpecified == "<this>")
            NumeReKernel::print(createListDirectoryHeader(_option.getExePath(), _lang.get("BUILTIN_LISTFILES_ROOTPATH"), _option.getWindow()));
	    else
            NumeReKernel::print(createListDirectoryHeader(sSpecified, _lang.get("BUILTIN_LISTFILES_CUSTOMPATH"), _option.getWindow()));

		if (!listDirectory(sSpecified, __sCmd, _option))
			NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
	}

	make_hline();
	return true;
}


/////////////////////////////////////////////////
/// \brief This function lists all known
/// functions in the terminal.
///
/// \param _option const Settings&
/// \param sType const string&
/// \return void
///
/// It is more or less a legacy function, because
/// the functions are now listed in the sidebar.
/////////////////////////////////////////////////
static void listFunctions(const Settings& _option, const string& sType) //PRSRFUNC_LISTFUNC_[TYPES]_*
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::printPreFmt("|-> NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_HEADLINE")));
	if (sType != "all")
	{
		NumeReKernel::printPreFmt("  [" + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_TYPE_" + toUpperCase(sType))) + "]");
	}
	NumeReKernel::printPreFmt("\n");
	make_hline();
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTFUNC_TABLEHEAD"), _option, false, 0, 28) + "\n|\n");
	vector<string> vFuncs;

	// Get the list of functions from the language file
	// depending on the selected type
	if (sType == "all")
		vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*");
	else
		vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_[" + toUpperCase(sType) + "]");

    // Print the obtained function list on the terminal
	for (unsigned int i = 0; i < vFuncs.size(); i++)
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + vFuncs[i], _option, false, 0, 60) + "\n");
	}
	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all custom defined
/// functions.
///
/// \param _functions const Define&
/// \param _option const Settings&
/// \return void
///
/// It is more or less also a legacy function,
/// because the custom defined functions are also
/// listed in the sidebar.
/////////////////////////////////////////////////
static void listDefinitions(const FunctionDefinitionManager& _functions, const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTDEFINE_HEADLINE")));
	make_hline();
	if (!_functions.getDefinedFunctions())
	{
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_EMPTY")));
	}
	else
	{
	    // Print all custom defined functions on the terminal
		for (unsigned int i = 0; i < _functions.getDefinedFunctions(); i++)
		{
		    // Print first the name of the function
			NumeReKernel::printPreFmt(sectionHeadline(_functions.getFunctionSignature(i).substr(0, _functions.getFunctionSignature(i).rfind('('))));

			// Print the comment, if it is available
			if (_functions.getComment(i).length())
			{
				NumeReKernel::printPreFmt(LineBreak("|       " + _lang.get("PARSERFUNCS_LISTDEFINE_DESCRIPTION", _functions.getComment(i)), _option, true, 0, 25) + "\n"); //10
			}

			// Print the actual implementation of the function
			NumeReKernel::printPreFmt(LineBreak("|       " + _lang.get("PARSERFUNCS_LISTDEFINE_DEFINITION", _functions.getFunctionSignature(i), _functions.getImplementation(i)), _option, false, 0, 29) + "\n"); //14
        }
		NumeReKernel::printPreFmt("|   -- " + toString((int)_functions.getDefinedFunctions()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_FUNCTIONS"))  + " --\n");
	}
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all logical
/// expressions.
///
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void listLogicalOperators(const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTLOGICAL_HEADLINE"))));
	make_hline();
	NumeReKernel::printPreFmt(toSystemCodePage("|   " + _lang.get("PARSERFUNCS_LISTLOGICAL_TABLEHEAD")) + "\n|\n");

	// Get the list of all logical expressions
	vector<string> vLogicals = _lang.getList("PARSERFUNCS_LISTLOGICAL_ITEM*");

	// Print the list on the terminal
	for (unsigned int i = 0; i < vLogicals.size(); i++)
		NumeReKernel::printPreFmt(toSystemCodePage("|   " + vLogicals[i]) + "\n");

	NumeReKernel::printPreFmt(toSystemCodePage("|\n"));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all declared
/// variables, which are known by the numerical
/// and the string parser as well as the current
/// declared data tables and clusters.
///
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _data const Datafile&
/// \return void
///
/// It is more or less also a legacy function,
/// because the declared variables are also
/// listed in the variables widget.
/////////////////////////////////////////////////
static void listDeclaredVariables(Parser& _parser, const Settings& _option, const MemoryManager& _data)
{
	int nDataSetNum = 1;
	map<string, int> VarMap;
	int nBytesSum = 0;

	// Query the used variables
	//
	// Get the numerical variables
	mu::varmap_type variables = _parser.GetVar();

	// Get the string variables
	map<string, string> StringMap = NumeReKernel::getInstance()->getStringParser().getStringVars();

	// Get the current defined data tables
	map<string, long long int> CacheMap = _data.getTableMap();

	const map<string, NumeRe::Cluster>& mClusterMap = _data.getClusterMap();

	// Combine string and numerical variables to have
	// them sorted after their name
	for (auto iter = variables.begin(); iter != variables.end(); ++iter)
	{
		VarMap[iter->first] = 0;
	}
	for (auto iter = StringMap.begin(); iter != StringMap.end(); ++iter)
	{
		VarMap[iter->first] = 1;
	}

	// Get data table and string table sizes
	string sStringSize = toString((int)_data.getStringElements()) + " x " + toString((int)_data.getStringCols());

	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_HEADLINE"))));
	make_hline();

	// Print all defined caches first
	for (auto iter = CacheMap.begin(); iter != CacheMap.end(); ++iter)
	{
		string sCacheSize = toString(_data.getLines(iter->first, false)) + " x " + toString(_data.getCols(iter->first, false));
		NumeReKernel::printPreFmt("|   " + iter->first + "()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - (iter->first).length() + _option.getWindow(0) % 2) + strfill(sCacheSize, (_option.getWindow(0) - 50) / 2) + strfill("[double x double]", 19));

		if (_data.getSize(iter->second) >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second) / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (_data.getSize(iter->second) >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second) / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second)), 9) + "  Bytes\n");

		nBytesSum += _data.getSize(iter->second);
	}

	NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");

	// Print all defined cluster
	for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
	{
		string sClusterSize = toString(iter->second.size()) + " x 1";
		NumeReKernel::printPreFmt("|   " + iter->first + "{}" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - (iter->first).length() + _option.getWindow(0) % 2) + strfill(sClusterSize, (_option.getWindow(0) - 50) / 2) + strfill("[cluster]", 19));

		if (iter->second.getBytes() >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(iter->second.getBytes() / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (iter->second.getBytes() >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(iter->second.getBytes() / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(iter->second.getBytes()), 9) + "  Bytes\n");

		nBytesSum += iter->second.getBytes();
	}

	if (mClusterMap.size())
        NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");


	// Print now the dimension of the string table
	if (_data.getStringElements())
	{
		NumeReKernel::printPreFmt("|   string()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - 6 + _option.getWindow(0) % 2) + strfill(sStringSize, (_option.getWindow(0) - 50) / 2) + strfill("[string x string]", 19));
		if (_data.getStringSize() >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize() / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (_data.getStringSize() >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize() / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize()), 9) + "  Bytes\n");
		nBytesSum += _data.getStringSize();

		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");
    }

    // Print now the set of variables
	for (auto item = VarMap.begin(); item != VarMap.end(); ++item)
	{
	    // The second member indicates, whether a
	    // variable is a string or a numerical variable
		if (item->second)
		{
		    // This is a string
			NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0) - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow(0) % 2));
			if (StringMap[item->first].length() > (unsigned int)_option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 4)
				NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first].substr(0, _option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 7) + "...\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
			else
				NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first] + "\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
			NumeReKernel::printPreFmt(strfill("[string]", 19) + strfill(toString((int)StringMap[item->first].size()), 9) + "  Bytes\n");
			nBytesSum += StringMap[item->first].size();
		}
		else
		{
		    // This is a numerical variable
			NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0) - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow(0) % 2) + strfill(toString(*variables[item->first], _option), (_option.getWindow(0) - 60) / 2 + _option.getPrecision()) + strfill("[double]", 19) + strfill("8", 9) + "  Bytes\n");
			nBytesSum += sizeof(double);
		}
	}

	// Create now the footer of the list:
	// Combine the number of variables and data
	// tables first
	NumeReKernel::printPreFmt("|   -- " + toString((int)VarMap.size()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_VARS_AND")) + " ");
	if (_data.isValid() || _data.getStringElements())
	{
		if (_data.isValid() && _data.getStringElements())
		{
			NumeReKernel::printPreFmt(toString(2 + CacheMap.size()));
			nDataSetNum = CacheMap.size() + 1;
		}
		else if (_data.isValid())
		{
			NumeReKernel::printPreFmt(toString(1 + CacheMap.size()));
			nDataSetNum = CacheMap.size();
		}
		else
			NumeReKernel::printPreFmt("1");
	}
	else
		NumeReKernel::printPreFmt("0");
	NumeReKernel::printPreFmt(" " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_DATATABLES")) + " --");

	// Calculate now the needed memory for the stored values and print it at the
	// end of the footer line
	if (VarMap.size() > 9 && nDataSetNum > 9)
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 32 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	else if (VarMap.size() > 9 || nDataSetNum > 9)
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 31 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	else
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 30 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	if (nBytesSum >= 1024 * 1024)
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum / (1024.0 * 1024.0), 4), 8) + " MBytes\n");
	else if (nBytesSum >= 1024)
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum / (1024.0), 4), 8) + " KBytes\n");
	else
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum), 8) + "  Bytes\n");
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all known
/// constants.
///
/// \param _parser const Parser&
/// \param _option const Settings&
/// \return void
/// It is more or less a legacy function, because
/// the constants are now listed in the sidebar.
/////////////////////////////////////////////////
static void listConstants(const Parser& _parser, const Settings& _option)
{
	const int nUnits = 20;
	// Define a set of units including a simple
	// heuristic, which defines, which constant
	// needs which unit
	static string sUnits[nUnits] =
	{
		"_G[m^3/(kg s^2)]",
		"_R[J/(mol K)]",
		"_coul_norm[V m/(A s)]",
		"_c[m/s]",
		"_elek[A s/(V m)]",
		"_elem[A s]",
		"_gamma[1/(T s)]",
		"_g[m/s^2]",
		"_hartree[J]",
		"_h[J s]",
		"_k[J/K]",
		"_m_[kg]",
		"_magn[V s/(A m)]",
		"_mu_[J/T]",
		"_n[1/mol]",
		"_rydberg[1/m]",
		"_r[m]",
		"_stefan[J/(m^2 s K^4)]",
		"_wien[m K]",
		"_[---]"
	};
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCONST_HEADLINE"))));
	make_hline();

	// Get the map of all defined constants from the parser
	mu::valmap_type cmap = _parser.GetConst();
    valmap_type::const_iterator item = cmap.begin();

    // Print all constants, their values and their unit on
    // the terminal
    for (; item != cmap.end(); ++item)
    {
        if (item->first[0] != '_')
            continue;
        NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow() - 10) / 2 + 2 - _option.getPrecision() - (item->first).length() + _option.getWindow() % 2) + strfill(toString(item->second, _option), _option.getPrecision() + (_option.getWindow() - 50) / 2));
        for (int i = 0; i < nUnits; i++)
        {
            if (sUnits[i].substr(0, sUnits[i].find('[')) == (item->first).substr(0, sUnits[i].find('[')))
            {
                NumeReKernel::printPreFmt(strfill(sUnits[i].substr(sUnits[i].find('[')), 24) + "\n");
                break;
            }
        }
    }
    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE1"), _option));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all defined
/// commands.
///
/// \param _option const Settings&
/// \return void
///
/// It is more or less a legacy function, because
/// the commands are now listed in the sidebar.
/////////////////////////////////////////////////
static void listCommands(const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCMD_HEADLINE")))); //PRSRFUNC_LISTCMD_*
	make_hline();
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTCMD_TABLEHEAD"), _option, 0) + "\n|\n");

    // Get the list of all defined commands
    // from the language files
	vector<string> vCMDList = _lang.getList("PARSERFUNCS_LISTCMD_CMD_*");

	// Print the complete list on the terminal
	for (unsigned int i = 0; i < vCMDList.size(); i++)
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + vCMDList[i], _option, false, 0, 42) + "\n");
	}

	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
}


/////////////////////////////////////////////////
/// \brief This static function prints the
/// selected unit, its description, its dimension
/// and its value conversion to the terminal.
///
/// \param sUnit const string&
/// \param sDesc const string&
/// \param sDim const string&
/// \param sValues const string&
/// \param nWindowsize unsigned int
/// \return void
///
/////////////////////////////////////////////////
static void printUnits(const string& sUnit, const string& sDesc, const string& sDim, const string& sValues, unsigned int nWindowsize)
{
	NumeReKernel::printPreFmt("|     " + strlfill(sUnit, 11) + strlfill(sDesc, (nWindowsize - 17) / 3 + (nWindowsize + 1) % 3) + strlfill(sDim, (nWindowsize - 35) / 3) + "=" + strfill(sValues, (nWindowsize - 2) / 3) + "\n");
	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all unit
/// conversions and their result, if applied on 1.
///
/// \param _option const Settings&
/// \return void
///
/// The units are partly physcially units, partly
/// magnitudes.
/////////////////////////////////////////////////
static void listUnitConversions(const Settings& _option) //PRSRFUNC_LISTUNITS_*
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTUNITS_HEADLINE")))); //(_option.getWindow()-x)/3
	make_hline(); // 11       21  x=17             15   x=35      1               x=2      26
	printUnits(_lang.get("PARSERFUNCS_LISTUNITS_SYMBOL"), _lang.get("PARSERFUNCS_LISTUNITS_DESCRIPTION"), _lang.get("PARSERFUNCS_LISTUNITS_DIMENSION"), _lang.get("PARSERFUNCS_LISTUNITS_UNIT"), _option.getWindow());
	NumeReKernel::printPreFmt("|\n");
	printUnits("1'A",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ANGSTROEM"),        "L",           "1e-10      [m]", _option.getWindow());
	printUnits("1'AU",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ASTRO_UNIT"),       "L",           "1.4959787e11      [m]", _option.getWindow());
	printUnits("1'b",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_BARN"),             "L^2",         "1e-28    [m^2]", _option.getWindow());
	printUnits("1'cal", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CALORY"),           "M L^2 / T^2", "4.1868      [J]", _option.getWindow());
	printUnits("1'Ci",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CURIE"),            "1 / T",       "3.7e10     [Bq]", _option.getWindow());
	printUnits("1'eV",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ELECTRONVOLT"),     "M L^2 / T^2", "1.60217657e-19      [J]", _option.getWindow());
	printUnits("1'fm",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FERMI"),            "L",           "1e-15      [m]", _option.getWindow());
	printUnits("1'ft",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FOOT"),             "L",           "0.3048      [m]", _option.getWindow());
	printUnits("1'Gs",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_GAUSS"),            "M / (T^2 I)", "1e-4      [T]", _option.getWindow());
	printUnits("1'in",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_INCH"),             "L",           "0.0254      [m]", _option.getWindow());
	printUnits("1'kmh", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.2777777...    [m/s]", _option.getWindow());
	printUnits("1'kn",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_KNOTS"),            "L / T",       "0.5144444...    [m/s]", _option.getWindow());
	printUnits("1'l",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LITERS"),           "L^3",         "1e-3    [m^3]", _option.getWindow());
	printUnits("1'ly",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LIGHTYEAR"),        "L",           "9.4607305e15      [m]", _option.getWindow());
	printUnits("1'mile", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_MILE"),             "L",           "1609.344      [m]", _option.getWindow());
	printUnits("1'mol", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_MOL"),              "N",           "6.022140857e23      ---", _option.getWindow());
	printUnits("1'mph", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.44703722    [m/s]", _option.getWindow());
	printUnits("1'Ps",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_POISE"),            "M / (L T)",   "0.1   [Pa s]", _option.getWindow());
	printUnits("1'pc",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PARSEC"),           "L",           "3.0856776e16      [m]", _option.getWindow());
	printUnits("1'psi", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PSI"),              "M / (L T^2)", "6894.7573     [Pa]", _option.getWindow());
	printUnits("1'TC",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CELSIUS"),          "Theta",       "274.15      [K]", _option.getWindow());
	printUnits("1'TF",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FAHRENHEIT"),       "Theta",       "255.92778      [K]", _option.getWindow());
	printUnits("1'Torr", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_TORR"),             "M / (L T^2)", "133.322     [Pa]", _option.getWindow());
	printUnits("1'yd",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_YARD"),             "L",           "0.9144      [m]", _option.getWindow());
	NumeReKernel::printPreFmt("|\n");
	printUnits("1'G",   "(giga)",             "---",           "1e9      ---", _option.getWindow());
	printUnits("1'M",   "(mega)",             "---",           "1e6      ---", _option.getWindow());
	printUnits("1'k",   "(kilo)",             "---",           "1e3      ---", _option.getWindow());
	printUnits("1'm",   "(milli)",            "---",           "1e-3      ---", _option.getWindow());
	printUnits("1'mu",  "(micro)",            "---",           "1e-6      ---", _option.getWindow());
	printUnits("1'n",   "(nano)",             "---",           "1e-9      ---", _option.getWindow());

	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTUNITS_FOOTNOTE"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();

	return;
}


/////////////////////////////////////////////////
/// \brief This function lists all declared
/// plugins including their name, their command
/// and their description.
///
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return void
///
/// It is more or less a legacy function, because
/// the plugins are also listed in the sidebar.
/////////////////////////////////////////////////
static void listInstalledPlugins(Parser& _parser, MemoryManager& _data, const Settings& _option)
{
	string sDummy = "";
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTPLUGINS_HEADLINE"))));
	make_hline();
	Procedure& _procedure = NumeReKernel::getInstance()->getProcedureInterpreter();

	// Probably there's no plugin defined
	if (!_procedure.getPluginCount())
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTPLUGINS_EMPTY")));
	else
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTPLUGINS_TABLEHEAD"), _option, 0) + "\n");
		NumeReKernel::printPreFmt("|\n");

		// Print all plugins (name, command and description)
		// on the terminal
		for (unsigned int i = 0; i < _procedure.getPluginCount(); i++)
		{
			string sLine = "|   ";

			if (_procedure.getPluginCommand(i).length() > 18)
				sLine += _procedure.getPluginCommand(i).substr(0, 15) + "...";
			else
				sLine += _procedure.getPluginCommand(i);

			sLine.append(23 - sLine.length(), ' ');

			// Print basic information about the plugin
			sLine += _lang.get("PARSERFUNCS_LISTPLUGINS_PLUGININFO", _procedure.getPluginName(i), _procedure.getPluginVersion(i), _procedure.getPluginAuthor(i));

			// Print the description
			if (_procedure.getPluginDesc(i).length())
				sLine += "$" + _procedure.getPluginDesc(i);

			sLine = '"' + sLine + "\" -nq";
			NumeReKernel::getInstance()->getStringParser().evalAndFormat(sLine, sDummy, true);
			NumeReKernel::printPreFmt(LineBreak(sLine, _option, true, 0, 25) + "\n");
		}
	}

	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// possibility to call the Windows shell directly
/// from the code.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/// For security reasons, this command may be
/// disabled in the settings. It may only be
/// enabled, if the user clicks the corresponding
/// chekbox. There's no command available to enable
/// this command.
/////////////////////////////////////////////////
static bool executeCommand(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
	if (!_option.getUseExecuteCommand())
		throw SyntaxError(SyntaxError::EXECUTE_COMMAND_DISABLED, sCmd, "execute");

	sCmd = evaluateParameterValues(sCmd);
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());
	_fSys.setPath(_option.getExePath(), false, _option.getExePath());
	_fSys.declareFileType(".exe");
	string sParams = "";
	string sWorkpath = "";
	string sObject = "";
	int nRetVal = 0;
	bool bWaitForTermination = false;

	// Extract command line parameters
	if (findParameter(sCmd, "params", '='))
		sParams = "\"" + getArgAtPos(sCmd, findParameter(sCmd, "params", '=') + 6) + "\"";

    // Extract target working path
	if (findParameter(sCmd, "wp", '='))
		sWorkpath = "\"" + getArgAtPos(sCmd, findParameter(sCmd, "wp", '=') + 2) + "\"";

    // Extract, whether we shall wait for the process to terminate
	if (findParameter(sCmd, "wait"))
		bWaitForTermination = true;

    // Extract the actual command line command
	sObject = sCmd.substr(findCommand(sCmd).sString.length());

	if (sParams.length() || bWaitForTermination || sWorkpath.length())
	{
		if (sCmd.find("-set") != string::npos && sObject.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")))
			sObject.erase(sObject.find("-set"));
		else if (sCmd.find("--") != string::npos && sObject.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")))
			sObject.erase(sObject.find("--"));
		else
			throw SyntaxError(SyntaxError::EXECUTE_COMMAND_UNSUCCESSFUL, sCmd, "execute"); // throw an unsuccessful, if the parameters are not clearly identified
	}

	// Resolve strings in the extracted command line objects
	NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();

	if (_stringParser.isStringExpression(sObject))
	{
		string sDummy = "";
		_stringParser.evalAndFormat(sObject, sDummy, true);
	}

	if (_stringParser.isStringExpression(sParams))
	{
		string sDummy = "";
		sParams += " -nq";
		_stringParser.evalAndFormat(sParams, sDummy, true);
	}

	if (_stringParser.isStringExpression(sWorkpath))
	{
		string sDummy = "";
		sWorkpath += " -nq";
		_stringParser.evalAndFormat(sWorkpath, sDummy, true);
	}

	// Resolve path placeholders
	if (sObject.find('<') != string::npos && sObject.find('>', sObject.find('<') + 1) != string::npos)
		sObject = _fSys.ValidFileName(sObject, ".exe");

	if (sParams.find('<') != string::npos && sParams.find('>', sParams.find('<') + 1) != string::npos)
	{
		if (sParams.front() == '"')
			sParams = "\"" + _fSys.ValidFileName(sParams.substr(1));
		else
			sParams = _fSys.ValidFileName(sParams);

	}

	if (sWorkpath.find('<') != string::npos && sWorkpath.find('>', sWorkpath.find('<') + 1) != string::npos)
	{
		if (sWorkpath.front() == '"')
			sWorkpath = "\"" + _fSys.ValidFileName(sWorkpath.substr(1));
		else
			sWorkpath = _fSys.ValidFileName(sWorkpath);

		if (sWorkpath.rfind(".dat") != string::npos)
			sWorkpath.erase(sWorkpath.rfind(".dat"), 4);
	}

	StripSpaces(sObject);

	// Prepare the shell execution information
	// structure
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = sObject.c_str();
	ShExecInfo.lpParameters = sParams.c_str();
	ShExecInfo.lpDirectory = sWorkpath.c_str();
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;

	nRetVal = ShellExecuteEx(&ShExecInfo);

	if (!nRetVal)
		throw SyntaxError(SyntaxError::EXECUTE_COMMAND_UNSUCCESSFUL, sCmd, "execute");

    // Do we have to wait for termination?
	if (bWaitForTermination)
	{
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("|-> " + _lang.get("COMMON_EVALUATING") + " ... ");

		while (bWaitForTermination)
		{
			// wait 1sec and check, whether the user pressed the ESC key
			if (WaitForSingleObject(ShExecInfo.hProcess, 1000) == WAIT_OBJECT_0)
				break;

			if (NumeReKernel::GetAsyncCancelState())
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_CANCEL") + "\n");

				throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
			}
		}

		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt(_lang.get("COMMON_DONE") + ".\n");
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This function performs the autosave at
/// the application termination.
///
/// \param _data Datafile&
/// \param _out Output&
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
static void autoSave(MemoryManager& _data, Output& _out, Settings& _option)
{
    // Only do something, if there's unsaved and valid data
	if (_data.isValid() && !_data.getSaveStatus())
	{
	    // Inform the user
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt(toSystemCodePage(  _lang.get("BUILTIN_AUTOSAVE") + " ... "));

		// Try to save the cache
		if (_data.saveToCacheFile())
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS") + ".") );
		}
		else
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt("\n");
			throw SyntaxError(SyntaxError::CANNOT_SAVE_CACHE, "", SyntaxError::invalid_position);
		}
	}
	return;
}


/////////////////////////////////////////////////
/// \brief This static function extracts the path
/// from the selected parameter. It is only used
/// by cmd_set().
///
/// \param sCmd string&
/// \param sPathParameter const string&
/// \return string
///
/////////////////////////////////////////////////
static string getPathForSetting(string& sCmd, const string& sPathParameter)
{
    string sPath;

    if (findParameter(sCmd, sPathParameter, '='))
        addArgumentQuotes(sCmd, sPathParameter);

    while (sCmd.find('\\') != string::npos)
        sCmd[sCmd.find('\\')] = '/';

    if (!extractFirstParameterStringValue(sCmd, sPath))
    {
        NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );

        do
        {
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(sPath);
        }
        while (!sPath.length());
    }

    return sPath;
}


/////////////////////////////////////////////////
/// \brief This static function copies the contents
/// of the selected table to the provided temporary
/// Datafile object. This function evaluates user-
/// provided indices during the copy process.
///
/// \param sCmd const string&
/// \param _accessParser DataAccessParser&
/// \param _data Datafile&
/// \param _cache Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void copyDataToTemporaryTable(const string& sCmd, DataAccessParser& _accessParser, MemoryManager& _data, MemoryManager& _cache)
{
    // Validize the obtained index sets
    if (!isValidIndexSet(_accessParser.getIndices()))
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

    // Copy the target data to a new table
    if (_accessParser.getIndices().row.isOpenEnd())
        _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

    if (_accessParser.getIndices().col.isOpenEnd())
        _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

    _cache.resizeTable(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size(), "table");

    for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
    {
        for (size_t j = 0; j < _accessParser.getIndices().col.size(); j++)
        {
            if (!i)
                _cache.setHeadLineElement(j, "table", _data.getHeadLineElement(_accessParser.getIndices().col[j], _accessParser.getDataObject()));

            if (_data.isValidElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()))
                _cache.writeToTable(i, j, "table", _data.getElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()));
        }
    }
}


/////////////////////////////////////////////////
/// \brief This static function handles the
/// swapping of the data of the values of two
/// tables.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _option Settings&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues swapTables(string& sCmd, MemoryManager& _data, Settings& _option)
{
    string sArgument;

    // If the current command line contains strings
    // handle them here
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        sCmd = evaluateParameterValues(sCmd);

    // Handle legacy and new syntax in these two cases
    if (_data.matchTableAsParameter(sCmd, '=').length())
    {
        // Legacy syntax: swap -cache1=cache2
        //
        // Get the option value of the parameter "cache1"
        sArgument = getArgAtPos(sCmd, findParameter(sCmd, _data.matchTableAsParameter(sCmd, '='), '=') + _data.matchTableAsParameter(sCmd, '=').length());

        // Swap the caches
        _data.swapTables(_data.matchTableAsParameter(sCmd, '='), sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", _data.matchTableAsParameter(sCmd, '='), sArgument), _option) );
    }
    else if (sCmd.find("()") != string::npos && sCmd.find(',') != string::npos)
    {
        // New syntax: swap cache1(), cache2()
        //
        // Extract the first of the two arguments
        // (length of command = 4)
        sCmd.erase(0, 4);
        sArgument = getNextArgument(sCmd, true);

        if (!sCmd.length())
            return COMMAND_PROCESSED;

        // Remove parentheses, if available
        if (sArgument.find('(') != string::npos)
            sArgument.erase(sArgument.find('('));

        if (sCmd.find('(') != string::npos)
            sCmd.erase(sCmd.find('('));

        // Remove not necessary white spaces
        StripSpaces(sCmd);
        StripSpaces(sArgument);

        // Swap the caches
        _data.swapTables(sCmd, sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", sCmd, sArgument), _option) );
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function handles the
/// saving and exporting of data into files
/// (internally, there's no real difference
/// between those two actions).
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues saveDataObject(string& sCmd)
{
    // Get references to the main objects
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

	string sArgument;

    size_t nPrecision = _option.getPrecision();

    // Update the precision, if the user selected any
    if (findParameter(sCmd, "precision", '='))
    {
        _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "precision", '=')));
        nPrecision = _parser.Eval();

        if (nPrecision < 0 || nPrecision > 14)
            nPrecision = _option.getPrecision();
    }

    // Copy the selected data into another datafile instance and
    // save the copied data
    DataAccessParser _access(sCmd);

    if (_access.getDataObject().length())
    {
        // Create the new instance
        MemoryManager _cache;

        // Update the necessary parameters
        _cache.setTokens(_option.getTokenPaths());
        _cache.setPath(_data.getPath(), false, _option.getExePath());

        copyDataToTemporaryTable(sCmd, _access, _data, _cache);

        // Update the name of the cache table (force it)
        if (_access.getDataObject() != "table")
            _cache.renameTable("table", _access.getDataObject(), true);

        // If the command line contains string variables
        // get those values here
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "file", '='))
            addArgumentQuotes(sCmd, "file");

        // Try to extract the file name, if it was passed
        if (containsStrings(sCmd) && extractFirstParameterStringValue(sCmd.substr(findParameter(sCmd, "file", '=')), sArgument))
        {
            if (_cache.saveFile(_access.getDataObject(), sArgument, nPrecision))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()), _option) );

                return COMMAND_PROCESSED;
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
        }
        else
            _cache.setPrefix(_access.getDataObject());

        // Auto-generate a file name during saving
        if (_cache.saveFile(_access.getDataObject(), "", nPrecision))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()), _option) );
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);

        return COMMAND_PROCESSED;
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

    return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "find" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_find(string& sCmd)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.length() > 6 && sCmd.find("-") != string::npos)
        doc_SearchFct(sCmd.substr(sCmd.find('-', findCommand(sCmd).nPos) + 1), _option);
    else if (sCmd.length() > 6)
        doc_SearchFct(sCmd.substr(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + findCommand(sCmd).sString.length())), _option);
    else
    {
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CHECKKEYWORD_FIND_CANNOT_READ"), _option));
        doc_Help("find", _option);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "integrate" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_integrate(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "integrate").nPos;
    vector<double> vIntegrate;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "integrate"));
        sArgument.replace(nPos, sCmd.length(), "<<ANS>>");
    }
    else
        sArgument = "<<ANS>>";

    sCmd = evaluateParameterValues(sCmd);

    StripSpaces(sCmd);

    if ((findCommand(sCmd, "integrate").sString.length() >= 10 && findCommand(sCmd, "integrate").sString.substr(0, 10) == "integrate2")
        || (findParameter(sCmd, "x", '=') && findParameter(sCmd, "y", '=')))
    {
        vIntegrate = integrate2d(sCmd, _data, _parser, _option, _functions);
        sCmd = sArgument;
        sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~integrate2[~_~]");
        _parser.SetVectorVar("_~integrate2[~_~]", vIntegrate);
        return COMMAND_HAS_RETURNVALUE;
    }
    else
    {
        vIntegrate = integrate(sCmd, _data, _parser, _option, _functions);
        sCmd = sArgument;
        sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~integrate[~_~]");
        _parser.SetVectorVar("_~integrate[~_~]", vIntegrate);
        return COMMAND_HAS_RETURNVALUE;
    }
    /*else
    {
        doc_Help("integrate", _option);
        return COMMAND_PROCESSED;
    }*/
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "diff" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_diff(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "diff").nPos;
    vector<double> vDiff;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "diff"));
        sArgument.replace(nPos, sCmd.length(), "<<ANS>>");
    }
    else
        sArgument = "<<ANS>>";

    if (sCmd.length() > 5)
    {
        vDiff = differentiate(sCmd, _parser, _data, _option, _functions);
        sCmd = sArgument;
        sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~diff[~_~]");
        _parser.SetVectorVar("_~diff[~_~]", vDiff);
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("diff", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "extrema" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_extrema(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "extrema").nPos;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "extrema"));
        sArgument.replace(nPos, sCmd.length(), "<<ans>>");
    }
    else
        sArgument = "<<ans>>";

    if (sCmd.length() > 8)
    {
        if (findExtrema(sCmd, _data, _parser, _option, _functions))
        {
            if (sCmd[0] != '"')
            {
                sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
                sCmd = sArgument;
            }

            return COMMAND_HAS_RETURNVALUE;
        }
        else
            doc_Help("extrema", _option);

        return COMMAND_PROCESSED;
    }
    else
        doc_Help("extrema", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "pulse" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_pulse(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!analyzePulse(sCmd, _parser, _data, _functions, _option))
    {
        doc_Help("pulse", _option);
        return COMMAND_PROCESSED;
    }

    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "eval" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_eval(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "eval").nPos;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "eval"));
        sArgument.replace(nPos, sCmd.length(), "<<ans>>");
    }
    else
        sArgument = "<<ans>>";

    if (evalPoints(sCmd, _data, _parser, _option, _functions))
    {
        if (sCmd[0] != '"')
        {
            sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
            sCmd = sArgument;
        }

        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("eval", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "zeroes" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_zeroes(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "zeroes").nPos;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "zeroes"));
        sArgument.replace(nPos, sCmd.length(), "<<ans>>");
    }
    else
        sArgument = "<<ans>>";

    if (sCmd.length() > 7)
    {
        if (findZeroes(sCmd, _data, _parser, _option, _functions))
        {
            if (sCmd[0] != '"')
            {
                sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
                sCmd = sArgument;
            }

            return COMMAND_HAS_RETURNVALUE;
        }
        else
            doc_Help("zeroes", _option);
    }
    else
        doc_Help("zeroes", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "sort" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_sort(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    size_t nPos = findCommand(sCmd, "sort").nPos;
    string sArgument;

    if (nPos)
    {
        sArgument = sCmd;
        sCmd = extractCommandString(sCmd, findCommand(sCmd, "sort"));
        sArgument.replace(nPos, sCmd.length(), "<<ans>>");
    }
    else
        sArgument = "<<ans>>";

    sortData(sCmd, _parser, _data, _functions, _option);

    if (sCmd.length())
    {
        sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
        sCmd = sArgument;
        return COMMAND_HAS_RETURNVALUE;
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function handles the
/// displaying of user interaction dialogs.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/// This includes message boxes, file and
/// directory pickers, text entries, list and
/// selection dialogs.
/////////////////////////////////////////////////
static CommandReturnValues cmd_dialog(string& sCmd)
{
    size_t position = findCommand(sCmd, "dialog").nPos;
    string sDialogSettings = sCmd.substr(position+7);
    string sMessage;
    string sTitle = "NumeRe: Window";
    string sExpression;
    int nControls = NumeRe::CTRL_NONE;
    NumeReKernel* kernel = NumeReKernel::getInstance();

    // If the current command line contains strings in the option values
    // handle them here
    if (kernel->getStringParser().isStringExpression(sDialogSettings))
        sDialogSettings = evaluateParameterValues(sDialogSettings);

    // Extract the message for the user
    if (findParameter(sDialogSettings, "msg", '='))
        sMessage = getArgAtPos(sDialogSettings, findParameter(sDialogSettings, "msg", '=')+3);

    // Extract the window title
    if (findParameter(sDialogSettings, "title", '='))
        sTitle = getArgAtPos(sDialogSettings, findParameter(sDialogSettings, "title", '=')+5);

    // Extract the selected dialog type if available, otherwise
    // use the message box as default value
    if (findParameter(sDialogSettings, "type", '='))
    {
        string sType = getArgAtPos(sDialogSettings, findParameter(sDialogSettings, "type", '=')+4);

        if (sType == "filedialog")
            nControls = NumeRe::CTRL_FILEDIALOG;
        else if (sType == "dirdialog")
            nControls = NumeRe::CTRL_FOLDERDIALOG;
        else if (sType == "listdialog")
            nControls = NumeRe::CTRL_LISTDIALOG;
        else if (sType == "selectiondialog")
            nControls = NumeRe::CTRL_SELECTIONDIALOG;
        else if (sType == "messagebox")
            nControls = NumeRe::CTRL_MESSAGEBOX;
        else if (sType == "textentry")
            nControls = NumeRe::CTRL_TEXTENTRY;
    }
    else
        nControls = NumeRe::CTRL_MESSAGEBOX;

    // Extract the button information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (findParameter(sDialogSettings, "buttons", '='))
    {
        string sButtons = getArgAtPos(sDialogSettings, findParameter(sDialogSettings, "buttons", '=')+7);

        if (sButtons == "ok")
            nControls |= NumeRe::CTRL_OKBUTTON;
        else if (sButtons == "okcancel")
            nControls |= NumeRe::CTRL_OKBUTTON | NumeRe::CTRL_CANCELBUTTON;
        else if (sButtons == "yesno")
            nControls |= NumeRe::CTRL_YESNOBUTTON;
    }

    // Extract the icon information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (findParameter(sDialogSettings, "icon", '='))
    {
        string sIcon = getArgAtPos(sDialogSettings, findParameter(sDialogSettings, "icon", '=')+4);

        if (sIcon == "erroricon")
            nControls |= NumeRe::CTRL_ICONERROR;
        else if (sIcon == "warnicon")
            nControls |= NumeRe::CTRL_ICONWARNING;
        else if (sIcon == "infoicon")
            nControls |= NumeRe::CTRL_ICONINFORMATION;
        else if (sIcon == "questionicon")
            nControls |= NumeRe::CTRL_ICONQUESTION;
    }

    // Extract the default values for the dialog. First,
    // erase the appended parameter list
    if (sDialogSettings.find("-set") != string::npos)
        sDialogSettings.erase(sDialogSettings.find("-set"));
    else if (sDialogSettings.find("--") != string::npos)
        sDialogSettings.erase(sDialogSettings.find("--"));

    // Strip spaces and assign the value
    StripSpaces(sDialogSettings);
    sExpression = sDialogSettings;

    // Handle strings in the default value
    // expression. This will include also possible path
    // tokens
    if (kernel->getStringParser().isStringExpression(sExpression))
    {
        string sDummy;
        kernel->getStringParser().evalAndFormat(sExpression, sDummy, true);
    }

    // Ensure that default values are available, if the user
    // selected either a list or a selection dialog
    if ((nControls & NumeRe::CTRL_LISTDIALOG || nControls & NumeRe::CTRL_SELECTIONDIALOG) && (!sExpression.length() || sExpression == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, sCmd, "dialog");
    }

    // Use the default expression as message for the message
    // box as a fallback solution
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
        sMessage = getNextArgument(sExpression, false);

    // Ensure that the message box has at least a message,
    // because the message box is the default value
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, sCmd, "dialog");
    }

    // Ensure that the path for the file and the directory
    // dialog is a valid path and replace all placeholders
    if ((nControls & NumeRe::CTRL_FILEDIALOG || nControls & NumeRe::CTRL_FOLDERDIALOG) && sExpression.length() && sExpression != "\"\"")
    {
        sExpression = kernel->getMemoryManager().ValidFolderName(removeQuotationMarks(sExpression));
    }

    // Get the window manager, create the modal window and
    // wait until the user interacted with the dialog
    NumeRe::WindowManager& manager = kernel->getWindowManager();
    size_t winid = manager.createWindow(NumeRe::WINDOW_MODAL, NumeRe::WindowSettings(nControls, true, sMessage, sTitle, sExpression));
    NumeRe::WindowInformation wininfo = manager.getWindowInformationModal(winid);

    // Insert the return value as a string into the command
    // line and inform the command handler, that a value
    // has to be evaluated
    sCmd = sCmd.substr(0, position) + "\"" + replacePathSeparator(wininfo.sReturn) + "\"";

    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements all
/// plotting commands.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_plotting(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    string sCommand = findCommand(sCmd).sString;

    if (sCmd.length() > sCommand.length() + 1)
    {
        if (sCommand == "graph")
            sCmd.replace(findCommand(sCmd).nPos, 5, "plot");

        if (sCommand == "graph3d")
            sCmd.replace(findCommand(sCmd).nPos, 7, "plot3d");

        if (sCmd.find("--") != string::npos || sCmd.find("-set") != string::npos)
        {
            string sCmdSubstr;

            if (sCmd.find("--") != string::npos)
                sCmdSubstr = sCmd.substr(4, sCmd.find("--") - 4);
            else
                sCmdSubstr = sCmd.substr(4, sCmd.find("-set") - 4);

            if (!isNotEmptyExpression(sCmdSubstr))
            {
                if (sCmd.find("--") != string::npos)
                    _pData.setParams(sCmd.substr(sCmd.find("--")), _parser, _option);
                else
                    _pData.setParams(sCmd.substr(sCmd.find("-set")), _parser, _option);

                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_PLOTPARAMS")));

            }
            else
                createPlot(sCmd, _data, _parser, _option, _functions, _pData);
        }
        else
            createPlot(sCmd, _data, _parser, _option, _functions, _pData);

    }
    else
        doc_Help(sCommand, _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "fit" and "fitw" commands.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_fit(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (_data.isValid())
        fitDataSet(sCmd, _parser, _data, _functions, _option);
    else
        doc_Help("fit", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "fft" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_fft(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    fastFourierTransform(sCmd, _parser, _data, _option);
    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "fwt" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_fwt(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    fastWaveletTransform(sCmd, _parser, _data, _option);
    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "get" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_get(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    size_t nPos = findCommand(sCmd, "get").nPos;
    string sCommand = extractCommandString(sCmd, findCommand(sCmd, "get"));

    if (findParameter(sCmd, "savepath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getSavePath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getSavePath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("SAVEPATH: \"" + _option.getSavePath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "loadpath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getLoadPath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getLoadPath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("LOADPATH: \"" + _option.getLoadPath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "workpath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getWorkPath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getWorkPath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("WORKPATH: \"" + _option.getWorkPath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "viewer"))
    {
        if (_option.getViewerPath().length())
        {
            if (findParameter(sCmd, "asstr"))
            {
                if (_option.getViewerPath()[0] == '"' && _option.getViewerPath()[_option.getViewerPath().length() - 1] == '"')
                {
                    if (!nPos)
                        sCmd = _option.getViewerPath();
                    else
                        sCmd.replace(nPos, sCommand.length(), _option.getViewerPath());
                }
                else
                {
                    if (!nPos)
                        sCmd = "\"" + _option.getViewerPath() + "\"";
                    else
                        sCmd.replace(nPos, sCommand.length(), "\"" + _option.getViewerPath() + "\"");
                }

                return COMMAND_HAS_RETURNVALUE;
            }
            if (_option.getViewerPath()[0] == '"' && _option.getViewerPath()[_option.getViewerPath().length() - 1] == '"')
                NumeReKernel::print(LineBreak("IMAGEVIEWER: " + _option.getViewerPath(), _option));
            else
                NumeReKernel::print(LineBreak("|-> IMAGEVIEWER: \"" + _option.getViewerPath() + "\"", _option));
        }
        else
        {
            if (findParameter(sCmd, "asstr"))
            {
                if (!nPos)
                    sCmd = "\"\"";
                else
                    sCmd.replace(nPos, sCommand.length(), "\"\"");

                return COMMAND_HAS_RETURNVALUE;
            }
            else
                NumeReKernel::print("Kein Imageviewer deklariert!");
        }

        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "editor"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (_option.getEditorPath()[0] == '"' && _option.getEditorPath()[_option.getEditorPath().length() - 1] == '"')
            {
                if (!nPos)
                    sCmd = _option.getEditorPath();
                else
                    sCmd.replace(nPos, sCommand.length(), _option.getEditorPath());
            }
            else
            {
                if (!nPos)
                    sCmd = "\"" + _option.getEditorPath() + "\"";
                else
                    sCmd.replace(nPos, sCommand.length(), "\"" + _option.getEditorPath() + "\"");
            }

            return COMMAND_HAS_RETURNVALUE;
        }

        if (_option.getEditorPath()[0] == '"' && _option.getEditorPath()[_option.getEditorPath().length() - 1] == '"')
            NumeReKernel::print(LineBreak("TEXTEDITOR: " + _option.getEditorPath(), _option));
        else
            NumeReKernel::print(LineBreak("TEXTEDITOR: \"" + _option.getEditorPath() + "\"", _option));

        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "scriptpath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getScriptPath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getScriptPath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("SCRIPTPATH: \"" + _option.getScriptPath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "procpath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getProcsPath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getProcsPath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("PROCPATH: \"" + _option.getProcsPath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "plotfont"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getDefaultPlotFont() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getDefaultPlotFont() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("PLOTFONT: \"" + _option.getDefaultPlotFont() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "precision"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getPrecision());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getPrecision()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getPrecision()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getPrecision()) + "\"");
            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("PRECISION = " + toString(_option.getPrecision()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "faststart"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbFastStart());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbFastStart()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbFastStart()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbFastStart()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("FASTSTART: " + toString(_option.getbFastStart()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "compact"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbCompact());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbCompact()));

            return COMMAND_HAS_RETURNVALUE;
        }
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbCompact()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbCompact()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("COMPACT-MODE: " + toString(_option.getbCompact()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "autosave"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getAutoSaveInterval());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getAutoSaveInterval()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getAutoSaveInterval()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getAutoSaveInterval()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("AUTOSAVE-INTERVAL: " + toString(_option.getAutoSaveInterval()) + " [sec]");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "plotparams"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = _pData.getParams(_option, true);
            else
                sCmd.replace(nPos, sCommand.length(), _pData.getParams(_option, true));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("PLOTPARAMS: " + _pData.getParams(_option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "varlist"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = getVarList("vars -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("vars -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("VARS: " + getVarList("vars", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "stringlist"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = getVarList("strings -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("strings -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("STRINGS: " + getVarList("strings", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "numlist"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = getVarList("nums -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("nums -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("NUMS: " + getVarList("nums", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "plotpath"))
    {
        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + _option.getPlotOutputPath() + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + _option.getPlotOutputPath() + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("PLOTPATH: \"" + _option.getPlotOutputPath() + "\"");
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "greeting"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbGreeting());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbGreeting()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbGreeting()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbGreeting()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("GREETING: " + toString(_option.getbGreeting()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "hints"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbShowHints());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowHints()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbShowHints()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbGreeting()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("HINTS: " + toString(_option.getbGreeting()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "useescinscripts"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseESCinScripts());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseESCinScripts()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbUseESCinScripts()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseESCinScripts()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("USEESCINSCRIPTS: " + toString(_option.getbUseESCinScripts()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "usecustomlang"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getUseCustomLanguageFiles());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getUseCustomLanguageFiles()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getUseCustomLanguageFiles()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getUseCustomLanguageFiles()) + "\"");
            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("USECUSTOMLANG: " + toString(_option.getUseCustomLanguageFiles()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "externaldocwindow"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getUseExternalViewer());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getUseExternalViewer()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getUseExternalViewer()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getUseExternalViewer()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("EXTERNALDOCWINDOW: " + toString(_option.getUseExternalViewer()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "draftmode"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseDraftMode());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseDraftMode()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbUseDraftMode()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseDraftMode()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("DRAFTMODE: " + toString(_option.getbUseDraftMode()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "extendedfileinfo"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbShowExtendedFileInfo());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowExtendedFileInfo()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbShowExtendedFileInfo()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbShowExtendedFileInfo()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("EXTENDED FILEINFO: " + toString(_option.getbShowExtendedFileInfo()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "loademptycols"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbLoadEmptyCols());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbLoadEmptyCols()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbLoadEmptyCols()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbLoadEmptyCols()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("LOAD EMPTY COLS: " + toString(_option.getbLoadEmptyCols()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "logfile"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseLogFile());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseLogFile()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbUseLogFile()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseLogFile()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("EXTENDED FILEINFO: " + toString(_option.getbUseLogFile()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "defcontrol"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbDefineAutoLoad());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbDefineAutoLoad()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString(_option.getbDefineAutoLoad()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbDefineAutoLoad()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("DEFCONTROL: " + toString(_option.getbDefineAutoLoad()));
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "buffersize"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString((int)_option.getBuffer(1));
            else
                sCmd.replace(nPos, sCommand.length(), toString((int)_option.getBuffer(1)));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString((int)_option.getBuffer(1)) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString((int)_option.getBuffer(1)) + "\"");
            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("BUFFERSIZE: " + _option.getBuffer(1) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "windowsize"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = "x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1);
            else
                sCmd.replace(nPos, sCommand.length(), "{" + toString((int)_option.getWindow() + 1) + ", " + toString((int)_option.getWindow(1) + 1) + "}");

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("WINDOWSIZE: x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "colortheme"))
    {
        if (findParameter(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString((int)_option.getColorTheme());
            else
                sCmd.replace(nPos, sCommand.length(), toString((int)_option.getColorTheme()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (findParameter(sCmd, "asstr"))
        {
            if (!nPos)
                sCmd = "\"" + toString((int)_option.getColorTheme()) + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + toString((int)_option.getColorTheme()) + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("COLORTHEME: " + _option.getColorTheme() );
        return COMMAND_PROCESSED;
    }
    else
    {
        doc_Help("get", _option);
        return COMMAND_PROCESSED;
    }
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "undefine" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_undefine(string& sCmd)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    size_t nPos = findCommand(sCmd).nPos;

    if (sCmd.length() > 7)
        undefineFunctions(sCmd.substr(sCmd.find(' ', nPos) + 1), _functions, _option);
    else
        doc_Help("define", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "readline" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_readline(string& sCmd)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    size_t nPos = findCommand(sCmd, "readline").nPos;
    string sCommand = extractCommandString(sCmd, findCommand(sCmd, "readline"));
    string sDefault = "";
    string sArgument;

    if (findParameter(sCmd, "msg", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd, nPos);

        sCmd = sCmd.replace(nPos, sCommand.length(), evaluateParameterValues(sCommand));
        sCommand = evaluateParameterValues(sCommand);
    }

    if (findParameter(sCmd, "dflt", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd, nPos);

        sCmd = sCmd.replace(nPos, sCommand.length(), evaluateParameterValues(sCommand));
        sCommand = evaluateParameterValues(sCommand);
        sDefault = getArgAtPos(sCmd, findParameter(sCmd, "dflt", '=') + 4);
    }

    while (!sArgument.length())
    {
        string sLastLine = "";
        NumeReKernel::printPreFmt("|-> ");

        if (findParameter(sCmd, "msg", '='))
        {
            sLastLine = LineBreak(getArgAtPos(sCmd, findParameter(sCmd, "msg", '=') + 3), _option, false, 4);
            NumeReKernel::printPreFmt(sLastLine);

            if (sLastLine.find('\n') != string::npos)
                sLastLine.erase(0, sLastLine.rfind('\n'));

            if (sLastLine.substr(0, 4) == "|   " || sLastLine.substr(0, 4) == "|<- " || sLastLine.substr(0, 4) == "|-> ")
                sLastLine.erase(0, 4);

            StripSpaces(sLastLine);
        }

        NumeReKernel::getline(sArgument);

        if (sLastLine.length() && sArgument.find(sLastLine) != string::npos)
            sArgument.erase(0, sArgument.find(sLastLine) + sLastLine.length());

        StripSpaces(sArgument);

        if (!sArgument.length() && sDefault.length())
            sArgument = sDefault;
    }

    if (findParameter(sCmd, "asstr") && sArgument[0] != '"' && sArgument[sArgument.length() - 1] != '"')
        sCmd = sCmd.replace(nPos, sCommand.length(), "\"" + sArgument + "\"");
    else
        sCmd = sCmd.replace(nPos, sCommand.length(), sArgument);

    GetAsyncKeyState(VK_ESCAPE);
    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "read" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_read(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    size_t nPos = findCommand(sCmd, "read").nPos;
    string sArgument = extractCommandString(sCmd, findCommand(sCmd, "read"));
    string sCommand = sArgument;

    if (sArgument.length() > 5)
    {
        readFromFile(sArgument, _parser, _data, _option);
        sCmd.replace(nPos, sCommand.length(), sArgument);
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("read", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "data" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_data(string& sCmd)
{
    // DEPRECATED: Declared at v1.1.2rc1
    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    string sArgument;
    int nArgument;
    size_t nPos;
    string sCommand = findCommand(sCmd).sString;

    if (findParameter(sCmd, "clear"))
    {
        if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            remove_data(_data, _option, true);
        else
            remove_data(_data, _option);

        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "load") || findParameter(sCmd, "load", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "load", '='))
            addArgumentQuotes(sCmd, "load");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (findParameter(sCmd, "slices", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slices", '=') + 6) == "xz")
                nArgument = -1;
            else if (findParameter(sCmd, "slices", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slices", '=') + 6) == "yz")
                nArgument = -2;
            else
                nArgument = 0;
            if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            {
                if (_data.isValid())
                {
                    if (_option.getSystemPrintStatus())
                        _data.removeData(false);
                    else
                        _data.removeData(true);
                }
                if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                    {
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    }
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);

                    for (unsigned int i = 0; i < vFilelist.size(); i++)
                    {
                        _data.openFile(sPath + vFilelist[i], false, nArgument);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der " + toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                {
                    _data.openFile(sArgument, false, nArgument);
                }
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
            }
            else if (!_data.isValid())
            {
                if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    //NumeReKernel::print(sArgument );
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                    {
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    }
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
                    for (unsigned int i = 0; i < vFilelist.size(); i++)
                    {
                        _data.openFile(sPath + vFilelist[i], false, nArgument);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der " +toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument, false, nArgument);
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else
            load_data(_data, _option, _parser);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "paste") || findParameter(sCmd, "pasteload"))
    {
        PasteHandler _handler;
        _data.melt(_handler.pasteLoad(_option), "data");
        if (_data.isValid())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_PASTE_SUCCESS", toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
        //NumeReKernel::print(LineBreak("|-> Die Daten wurden erfolgreich eingefügt: Der Datensatz besteht nun aus "+toString(_data.getLines("data"))+" Zeile(n) und "+toString(_data.getCols("data"))+" Spalte(n).", _option) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "reload") || findParameter(sCmd, "reload", '='))
    {
        if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !findParameter(sCmd, "reload", '='))
            //throw CANNOT_RELOAD_DATA;
            throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, "", SyntaxError::invalid_position);
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "reload", '='))
            addArgumentQuotes(sCmd, "reload");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (_data.isValid())
            {
                _data.removeData(false);
                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument);
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich aktualisiert.", _option) );
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else if (_data.isValid())
        {
            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            sArgument = _data.getDataFileName("data");
            _data.removeData(false);
            _data.openFile(sArgument);
            if (_data.isValid() && _option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_SUCCESS"), _option) );
            //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich aktualisiert.", _option) );
        }
        else
            load_data(_data, _option, _parser);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "app") || findParameter(sCmd, "app", '='))
    {
        append_data(sCmd, _data, _option);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "showf"))
    {
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "show"))
    {
        _out.setCompact(_option.getbCompact());
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
        return COMMAND_PROCESSED;
    }
    else if (sCmd.substr(0, 5) == "data(")
    {
        return NO_COMMAND;
    }
    else if (findParameter(sCmd, "stats"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValid())
            plugin_statistics(sArgument, _data, _out, _option, false, true);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "hist"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValid())
            plugin_histogram(sArgument, _data, _data, _out, _option, _pData, false, true);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "save") || findParameter(sCmd, "save", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "save", '='))
            addArgumentQuotes(sCmd, "save");
        _data.setPrefix("data");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (_data.saveFile("data", sArgument))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
                //throw CANNOT_SAVE_FILE;
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
            //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
        }
        else
        {
            sArgument = _data.getDataFileName("data");
            if (sArgument.find('\\') != string::npos)
                sArgument = sArgument.substr(sArgument.rfind('\\') + 1);
            if (sArgument.find('/') != string::npos)
                sArgument = sArgument.substr(sArgument.rfind('/') + 1);
            if (sArgument.substr(sArgument.rfind('.')) != ".ndat")
                sArgument = sArgument.substr(0, sArgument.rfind('.')) + ".ndat";
            if (_data.saveFile("data", "copy_of_" + sArgument))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
                //throw CANNOT_SAVE_FILE;
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
            //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
        }
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "sort", '=') || findParameter(sCmd, "sort"))
    {
        _data.sortElements(sCmd);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "export") || findParameter(sCmd, "export", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "export", '='))
            addArgumentQuotes(sCmd, "export");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            _out.setFileName(sArgument);
            show_data(_data, _out, _option, "data", _option.getPrecision(), true, false, true, false);
        }
        else
            show_data(_data, _out, _option, "data", _option.getPrecision(), true, false, true);
        return COMMAND_PROCESSED;
    }
    else if ((findParameter(sCmd, "avg")
              || findParameter(sCmd, "sum")
              || findParameter(sCmd, "min")
              || findParameter(sCmd, "max")
              || findParameter(sCmd, "norm")
              || findParameter(sCmd, "std")
              || findParameter(sCmd, "prd")
              || findParameter(sCmd, "num")
              || findParameter(sCmd, "cnt")
              || findParameter(sCmd, "and")
              || findParameter(sCmd, "or")
              || findParameter(sCmd, "xor")
              || findParameter(sCmd, "med"))
             && (findParameter(sCmd, "lines") || findParameter(sCmd, "cols")))
    {
        if (!_data.isValid())
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        string sEvery = "";
        if (findParameter(sCmd, "every", '='))
        {
            value_type* v = 0;
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "every", '=') + 5));
            v = _parser.Eval(nArgument);
            if (nArgument > 1)
            {
                sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
            }
            else
                sEvery = "every=" + toString((int)v[0]) + " ";
        }
        nPos = findCommand(sCmd, "data").nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, "data"));
        sCommand = sArgument;
        if (findParameter(sCmd, "grid"))
            sArgument = "grid";
        else
            sArgument.clear();
        if (findParameter(sCmd, "avg"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[avg_lines]");
                _parser.SetVectorVar("_~data[avg_lines]", _data.avg("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[avg_cols]");
                _parser.SetVectorVar("_~data[avg_cols]", _data.avg("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "sum"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[sum_lines]");
                _parser.SetVectorVar("_~data[sum_lines]", _data.sum("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[sum_cols]");
                _parser.SetVectorVar("_~data[sum_cols]", _data.sum("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "min"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[min_lines]");
                _parser.SetVectorVar("_~data[min_lines]", _data.min("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[min_cols]");
                _parser.SetVectorVar("_~data[min_cols]", _data.min("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "max"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[max_lines]");
                _parser.SetVectorVar("_~data[max_lines]", _data.max("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[max_cols]");
                _parser.SetVectorVar("_~data[max_cols]", _data.max("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "norm"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[norm_lines]");
                _parser.SetVectorVar("_~data[norm_lines]", _data.norm("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[norm_cols]");
                _parser.SetVectorVar("_~data[norm_cols]", _data.norm("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "std"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[std_lines]");
                _parser.SetVectorVar("_~data[std_lines]", _data.std("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[std_cols]");
                _parser.SetVectorVar("_~data[std_cols]", _data.std("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "prd"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[prd_lines]");
                _parser.SetVectorVar("_~data[prd_lines]", _data.prd("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[prd_cols]");
                _parser.SetVectorVar("_~data[prd_cols]", _data.prd("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "num"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[num_lines]");
                _parser.SetVectorVar("_~data[num_lines]", _data.num("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[num_cols]");
                _parser.SetVectorVar("_~data[num_cols]", _data.num("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "cnt"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[cnt_lines]");
                _parser.SetVectorVar("_~data[cnt_lines]", _data.cnt("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[cnt_cols]");
                _parser.SetVectorVar("_~data[cnt_cols]", _data.cnt("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "med"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[med_lines]");
                _parser.SetVectorVar("_~data[med_lines]", _data.med("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[med_cols]");
                _parser.SetVectorVar("_~data[med_cols]", _data.med("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "and"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[and_lines]");
                _parser.SetVectorVar("_~data[and_lines]", _data.and_func("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[and_cols]");
                _parser.SetVectorVar("_~data[and_cols]", _data.and_func("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "or"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[or_lines]");
                _parser.SetVectorVar("_~data[or_lines]", _data.or_func("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[or_cols]");
                _parser.SetVectorVar("_~data[or_cols]", _data.or_func("data", sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "xor"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[xor_lines]");
                _parser.SetVectorVar("_~data[xor_lines]", _data.xor_func("data", sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~data[xor_cols]");
                _parser.SetVectorVar("_~data[xor_cols]", _data.xor_func("data", sArgument + "cols" + sEvery));
            }
        }

        return COMMAND_HAS_RETURNVALUE;
    }
    else if ((findParameter(sCmd, "avg")
              || findParameter(sCmd, "sum")
              || findParameter(sCmd, "min")
              || findParameter(sCmd, "max")
              || findParameter(sCmd, "norm")
              || findParameter(sCmd, "std")
              || findParameter(sCmd, "prd")
              || findParameter(sCmd, "num")
              || findParameter(sCmd, "cnt")
              || findParameter(sCmd, "and")
              || findParameter(sCmd, "or")
              || findParameter(sCmd, "xor")
              || findParameter(sCmd, "med"))
            )
    {
        if (!_data.isValid())
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        nPos = findCommand(sCmd, "data").nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, "data"));
        sCommand = sArgument;
        if (findParameter(sCmd, "grid") && _data.getCols("data") < 3)
            //throw TOO_FEW_COLS;
            throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, "data", "data");
        else if (findParameter(sCmd, "grid"))
            nArgument = 2;
        else
            nArgument = 0;
        if (findParameter(sCmd, "avg"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "sum"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "min"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "max"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "norm"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "std"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "prd"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "num"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "cnt"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "med"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "and"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "or"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (findParameter(sCmd, "xor"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.xor_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));

        return COMMAND_HAS_RETURNVALUE;
    }
    else if (sCommand == "data")
    {
        doc_Help("data", _option);
        return COMMAND_PROCESSED;
    }
    else
        return NO_COMMAND;

}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "new" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_new(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (findParameter(sCmd, "dir", '=')
            || findParameter(sCmd, "script", '=')
            || findParameter(sCmd, "proc", '=')
            || findParameter(sCmd, "file", '=')
            || findParameter(sCmd, "plugin", '=')
            || findParameter(sCmd, "cache", '=')
            || sCmd.find("()", findCommand(sCmd).nPos + 3) != string::npos
            || sCmd.find('$', findCommand(sCmd).nPos + 3) != string::npos)
    {
        _data.setUserdefinedFuncs(_functions.getNamesOfDefinedFunctions());

        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
            sCmd = evaluateParameterValues(sCmd);

        if (!newObject(sCmd, _parser, _data, _option))
            doc_Help("new", _option);
    }
    else
        doc_Help("new", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "edit" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_edit(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.length() > 5)
    {
        string sArgument;
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        {
            extractFirstParameterStringValue(sCmd, sArgument);
            sArgument = "edit " + sArgument;
            editObject(sArgument, _parser, _data, _option);
        }
        else
            editObject(sCmd, _parser, _data, _option);
    }
    else
        doc_Help("edit", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "taylor" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_taylor(string& sCmd)
{
    //Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (sCmd.length() > 7)
        taylor(sCmd, _parser, _option, _functions);
    else
        doc_Help("taylor", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "quit" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_quit(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "as"))
        autoSave(_data, _out, _option);

    if (findParameter(sCmd, "i"))
        _data.setSaveStatus(true);

    return NUMERE_QUIT;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "firststart" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_firststart(string& sCmd)
{
    // DEPRECATED: Declared at v1.1.2rc1
    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
    doc_FirstStart(NumeReKernel::getInstance()->getSettings());
    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "odesolve" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_odesolve(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (sCmd.length() > 9)
    {
        Odesolver _solver(&_parser, &_data, &_functions, &_option);
        _solver.solve(sCmd);
    }
    else
        doc_Help("odesolver", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// commands originating from using table names
/// as commands.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_tableAsCommand(string& sCmd, const string& sCacheCmd)
{
    // DEPRECATED: Declared at v1.1.2rc1
    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    //Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;
    int nArgument;
    size_t nPos;
    string sCommand = findCommand(sCmd).sString;

    if (findParameter(sCmd, "showf"))
    {
        show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "show"))
    {
        _out.setCompact(_option.getbCompact());
        show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "clear"))
    {
        if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            clear_cache(_data, _option, true);
        else
            clear_cache(_data, _option);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "hist"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValid())
            plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "stats"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (findParameter(sCmd, "save", '='))
        {
            if (sCmd[sCmd.find("save=") + 5] == '"' || sCmd[sCmd.find("save=") + 5] == '#')
            {
                if (!extractFirstParameterStringValue(sCmd, sArgument))
                    sArgument = "";
            }
            else
                sArgument = sCmd.substr(sCmd.find("save=") + 5, sCmd.find(' ', sCmd.find("save=") + 5) - sCmd.find("save=") - 5);
        }

        if (_data.isValid())
            plugin_statistics(sArgument, _data, _out, _option, true, false);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "save") || findParameter(sCmd, "save", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "save", '='))
            addArgumentQuotes(sCmd, "save");
        _data.setPrefix(sCommand);
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (_data.saveFile(sCommand, sArgument))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
            {
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
                //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
            }
        }
        else
        {
            if (_data.saveFile(sCommand, ""))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
            {
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
                //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
            }
        }
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "sort") || findParameter(sCmd, "sort", '='))
    {
        _data.sortElements(sCmd);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "export") || findParameter(sCmd, "export", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "export", '='))
            addArgumentQuotes(sCmd, "export");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            _out.setFileName(sArgument);
            show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true, true, false);
        }
        else
            show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true, true);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "rename", '=')) //CACHE -rename=NEWNAME
    {
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
            sCmd = evaluateParameterValues(sCmd);

        sArgument = getArgAtPos(sCmd, findParameter(sCmd, "rename", '=') + 6);
        _data.renameTable(sCommand, sArgument);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sArgument), _option) );
        //NumeReKernel::print(LineBreak("|-> Der Cache wurde erfolgreich zu \""+sArgument+"\" umbenannt.", _option) );
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "swap", '=')) //CACHE -swap=NEWCACHE
    {
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
            sCmd = evaluateParameterValues(sCmd);

        sArgument = getArgAtPos(sCmd, findParameter(sCmd, "swap", '=') + 4);
        _data.swapTables(sCommand, sArgument);
        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", sCommand, sArgument), _option) );
        //NumeReKernel::print(LineBreak("|-> Der Inhalt von \""+sCommand+"\" wurde erfolgreich mit dem Inhalt von \""+sArgument+"\" getauscht.", _option) );
        return COMMAND_PROCESSED;
    }
    else if ((findParameter(sCmd, "avg")
              || findParameter(sCmd, "sum")
              || findParameter(sCmd, "min")
              || findParameter(sCmd, "max")
              || findParameter(sCmd, "norm")
              || findParameter(sCmd, "std")
              || findParameter(sCmd, "prd")
              || findParameter(sCmd, "num")
              || findParameter(sCmd, "cnt")
              || findParameter(sCmd, "and")
              || findParameter(sCmd, "or")
              || findParameter(sCmd, "xor")
              || findParameter(sCmd, "med"))
             && (findParameter(sCmd, "lines") || findParameter(sCmd, "cols")))
    {
        if (!_data.isValid() || !_data.getCols(sCacheCmd, false))
            //throw NO_CACHED_DATA;
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
        string sEvery = "";
        if (findParameter(sCmd, "every", '='))
        {
            value_type* v = 0;
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "every", '=') + 5));
            v = _parser.Eval(nArgument);
            if (nArgument > 1)
            {
                sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
            }
            else
                sEvery = "every=" + toString((int)v[0]) + " ";
        }
        nPos = findCommand(sCmd, sCacheCmd).nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, sCacheCmd));
        sCommand = sArgument;
        if (findParameter(sCmd, "grid"))
            sArgument = "grid";
        else
            sArgument.clear();

        if (findParameter(sCmd, "avg"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[avg_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[avg_lines]", _data.avg(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[avg_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[avg_cols]", _data.avg(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "sum"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[sum_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[sum_lines]", _data.sum(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[sum_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[sum_cols]", _data.sum(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "min"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[min_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[min_lines]", _data.min(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[min_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[min_cols]", _data.min(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "max"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[max_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[max_lines]", _data.max(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[max_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[max_cols]", _data.max(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "norm"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[norm_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[norm_lines]", _data.norm(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[norm_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[norm_cols]", _data.norm(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "std"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[std_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[std_lines]", _data.std(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[std_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[std_cols]", _data.std(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "prd"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[prd_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[prd_lines]", _data.prd(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[prd_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[prd_cols]", _data.prd(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "num"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[num_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[num_lines]", _data.num(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[num_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[num_cols]", _data.num(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "cnt"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[cnt_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[cnt_lines]", _data.cnt(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[cnt_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[cnt_cols]", _data.cnt(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "med"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[med_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[med_lines]", _data.med(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[med_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[med_cols]", _data.med(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "and"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[and_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[and_lines]", _data.and_func(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[and_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[and_cols]", _data.and_func(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "or"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[or_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[or_lines]", _data.or_func(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[or_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[or_cols]", _data.or_func(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }
        else if (findParameter(sCmd, "xor"))
        {
            if (findParameter(sCmd, "lines"))
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[xor_lines]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[xor_lines]", _data.med(sCacheCmd, sArgument + "lines" + sEvery));
            }
            else
            {
                sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[xor_cols]");
                _parser.SetVectorVar("_~" + sCacheCmd + "[xor_cols]", _data.med(sCacheCmd, sArgument + "cols" + sEvery));
            }
        }

        return COMMAND_HAS_RETURNVALUE;
    }
    else if ((findParameter(sCmd, "avg")
              || findParameter(sCmd, "sum")
              || findParameter(sCmd, "min")
              || findParameter(sCmd, "max")
              || findParameter(sCmd, "norm")
              || findParameter(sCmd, "std")
              || findParameter(sCmd, "prd")
              || findParameter(sCmd, "num")
              || findParameter(sCmd, "cnt")
              || findParameter(sCmd, "and")
              || findParameter(sCmd, "or")
              || findParameter(sCmd, "xor")
              || findParameter(sCmd, "med"))
            )
    {
        if (!_data.isValid() || !_data.getCols(sCacheCmd, false))
            //throw NO_CACHED_DATA;
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
        nPos = findCommand(sCmd, sCacheCmd).nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, sCacheCmd));
        sCommand = sArgument;
        if (findParameter(sCmd, "grid") && _data.getCols(sCacheCmd, false) < 3)
            //throw TOO_FEW_COLS;
            throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, sCacheCmd, sCacheCmd);
        else if (findParameter(sCmd, "grid"))
            nArgument = 2;
        else
            nArgument = 0;
        if (findParameter(sCmd, "avg"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "sum"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "min"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "max"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "norm"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "std"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "prd"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "num"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "cnt"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "med"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "and"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "or"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (findParameter(sCmd, "xor"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.xor_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));

        return COMMAND_HAS_RETURNVALUE;
    }
    else if (sCommand == "cache")
    {
        doc_Help("cache", _option);
        return COMMAND_PROCESSED;
    }
    else
        return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "clear" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/// \todo Evaluate, whether the "clear" command
/// actual fits the current design.
/////////////////////////////////////////////////
static CommandReturnValues cmd_clear(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "data") || sCmd.find(" data()", findCommand(sCmd).nPos) != string::npos)
    {
        if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            remove_data(_data, _option, true);
        else
            remove_data(_data, _option);
    }
    else if (_data.matchTableAsParameter(sCmd).length() || _data.containsTablesOrClusters(sCmd.substr(findCommand(sCmd).nPos)))
    {
        if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            clear_cache(_data, _option, true);
        else
            clear_cache(_data, _option);
    }
    else if (findParameter(sCmd, "string") || sCmd.find(" string()", findCommand(sCmd).nPos) != string::npos)
    {
        if (_data.clearStringElements())
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_SUCCESS"));
        }
        else
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_EMPTY"));
        }

        return COMMAND_PROCESSED;
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "ifndefined" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_ifndefined(string& sCmd)
{
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.find(' ') != string::npos)
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "comment", '='))
            addArgumentQuotes(sCmd, "comment");

        string sArgument = sCmd.substr(sCmd.find(' '));
        StripSpaces(sArgument);

        if (!_functions.isDefined(sArgument.substr(0, sArgument.find(":="))))
        {
            if (_functions.defineFunc(sArgument))
                NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
            else
                NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
        }
    }
    else
        doc_Help("ifndef", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "install" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_install(string& sCmd)
{
    Script& _script = NumeReKernel::getInstance()->getScript();

    string sArgument;

    if (!_script.isOpen())
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        _script.setInstallProcedures();

        if (containsStrings(sCmd))
            extractFirstParameterStringValue(sCmd, sArgument);
        else
            sArgument = sCmd.substr(findCommand(sCmd).nPos + 8);

        StripSpaces(sArgument);
        _script.openScript(sArgument);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "copy" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_copy(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (_data.containsTablesOrClusters(sCmd))
    {
        if (CopyData(sCmd, _parser, _data, _option))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYDATA_SUCCESS"));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, sCmd, SyntaxError::invalid_position);
    }
    else if ((findParameter(sCmd, "target", '=') || findParameter(sCmd, "t", '=')) && sCmd.length() > 5)
    {
        int nArgument;

        if (findParameter(sCmd, "all") || findParameter(sCmd, "a"))
            nArgument = 1;
        else
            nArgument = 0;

        if (copyFile(sCmd, _parser, _data, _option))
        {
            if (_option.getSystemPrintStatus())
            {
                if (nArgument)
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_ALL_SUCCESS", sCmd));
                else
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_SUCCESS", sCmd));
            }
        }
        else
        {
            throw SyntaxError(SyntaxError::CANNOT_COPY_FILE, sCmd, SyntaxError::invalid_position, sCmd);
        }
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "credits" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_credits(string& sCmd)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::printPreFmt("|-> ");
	NumeReKernel::getInstance()->displaySplash();
	NumeReKernel::printPreFmt("\n");
	make_hline();
	NumeReKernel::printPreFmt("|-> Version: " + sVersion);
	NumeReKernel::printPreFmt(" | " + _lang.get("BUILTIN_CREDITS_BUILD") + ": " + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + "\n");
	NumeReKernel::print("Copyright (c) 2013-" + (AutoVersion::YEAR + toSystemCodePage(", Erik HÄNEL et al.")) );
	NumeReKernel::printPreFmt("|   <numere.developer@gmail.com>\n" );
	NumeReKernel::print(_lang.get("BUILTIN_CREDITS_VERSIONINFO"));
	make_hline(-80);
	NumeReKernel::print(_lang.get("BUILTIN_CREDITS_LICENCE_1"));
	NumeReKernel::print(_lang.get("BUILTIN_CREDITS_LICENCE_2"));
	NumeReKernel::print(_lang.get("BUILTIN_CREDITS_LICENCE_3"));
	NumeReKernel::toggleTableStatus();
	make_hline();

	return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "append" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_append(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "data") || findParameter(sCmd, "data", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        sCmd.replace(sCmd.find("data"), 4, "app");
        append_data(sCmd, _data, _option);
    }
    else if (sCmd.length() > findCommand(sCmd, "append").nPos + 7 && sCmd.find_first_not_of(' ', findCommand(sCmd, "append").nPos + 7) != string::npos)
    {
        NumeReKernel::printPreFmt("\r");

        Match _match = findCommand(sCmd, "append");
        double j1 = _data.getCols("data") + 1;
        string sExpr = sCmd;

        sExpr.replace(_match.nPos, string::npos, "_~append[~_~]");
        sCmd.erase(0, _match.nPos);

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7)] != '"' && sCmd.find("string(") == string::npos)
        {
            sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7), 1, '"');

            if (findParameter(sCmd, "slice")
                    || findParameter(sCmd, "keepdim")
                    || findParameter(sCmd, "complete")
                    || findParameter(sCmd, "ignore")
                    || findParameter(sCmd, "i")
                    || findParameter(sCmd, "head")
                    || findParameter(sCmd, "h")
                    || findParameter(sCmd, "all"))
            {
                size_t nPos = string::npos;

                while (sCmd.find_last_of('-', nPos) != string::npos
                        && sCmd.find_last_of('-', nPos) > sCmd.find_first_of(' ', sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7)))
                    nPos = sCmd.find_last_of('-', nPos) - 1;

                nPos = sCmd.find_last_not_of(' ', nPos);
                sCmd.insert(nPos + 1, 1, '"');
            }
            else
                sCmd.insert(sCmd.find_last_not_of(' ') + 1, 1, '"');
        }

        sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7), "-app=");
        append_data(sCmd, _data, _option);

        sCmd = sExpr;
        vector<double> vIndices;
        vIndices.push_back(1);
        vIndices.push_back(_data.getLines("data", true) - _data.getAppendedZeroes(j1, "data"));
        vIndices.push_back(j1);
        vIndices.push_back(_data.getCols("data"));

        _parser.SetVectorVar("_~append[~_~]", vIndices);
        return COMMAND_HAS_RETURNVALUE;
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "audio" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_audio(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!writeAudioFile(sCmd, _parser, _data, _functions, _option))
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);
    else if (_option.getSystemPrintStatus())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_AUDIO_SUCCESS"));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "imread" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_imread(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    readImage(sCmd, _parser, _data, _option);
    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "write" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_write(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    //Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.length() > 6 && findParameter(sCmd, "file", '='))
        writeToFile(sCmd, _data, _option);
    else
        doc_Help("write", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "workpath" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_workpath(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;

    if (sCmd.length() <= 8)
        return NO_COMMAND;

    if (sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 8) == string::npos)
        return NO_COMMAND;

    if (sCmd.find('"') == string::npos)
    {
        sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 8), 1, '"');
        StripSpaces(sCmd);
        sCmd += '"';
    }

    while (sCmd.find('\\') != string::npos)
        sCmd[sCmd.find('\\')] = '/';

    if (!extractFirstParameterStringValue(sCmd, sArgument))
        return COMMAND_PROCESSED;

    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(sArgument, true, _data.getProgramPath());
    _option.setWorkPath(_fSys.getPath());

    if (_option.getSystemPrintStatus())
        NumeReKernel::print(toSystemCodePage(_lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "warn" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_warn(string& sCmd)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;

    if (sCmd.length() > 5)
    {
        if (!extractFirstParameterStringValue(sCmd, sArgument))
        {
            sArgument = sCmd.substr(sCmd.find("warn")+5);
            _parser.SetExpr(sArgument);
            int nResults = 0;
            value_type* v = _parser.Eval(nResults);

            if (nResults > 1)
            {
                sArgument = "{";
                for (int i = 0; i < nResults; i++)
                    sArgument += " " + toString(v[i], _option) + ",";

                sArgument.pop_back();
                sArgument += "}";
            }
            else
                sArgument = toString(v[0], _option);
        }

        NumeReKernel::issueWarning(sArgument);
    }
    else
        doc_Help("warn", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "stats" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_stats(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();

    string sArgument = evaluateParameterValues(sCmd);

    if (findParameter(sCmd, "data") && !_data.isEmpty("data"))
    {
        // DEPRECATED: Declared at v1.1.2rc2
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        plugin_statistics(sArgument, _data, _out, _option, false, true);
    }
    else if (_data.matchTableAsParameter(sCmd).length() && _data.isValid())
    {
        // DEPRECATED: Declared at v1.1.2rc2
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        plugin_statistics(sArgument, _data, _out, _option, true, false);
    }
    else
    {
        DataAccessParser _accessParser(sCmd);

        if (_accessParser.getDataObject().length())
        {
            MemoryManager _cache;

            copyDataToTemporaryTable(sCmd, _accessParser, _data, _cache);

            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (findParameter(sCmd, "export", '='))
                addArgumentQuotes(sCmd, "export");

            sArgument = "stats -cache " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));
            sArgument = evaluateParameterValues(sArgument);
            plugin_statistics(sArgument, _cache, _out, _option, true, false);

            return COMMAND_PROCESSED;
        }
        else
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "stfa" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_stfa(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;

    if (!shortTimeFourierAnalysis(sCmd, sArgument, _parser, _data, _functions, _option))
        doc_Help("stfa", _option);
    else
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_STFA_SUCCESS", sArgument));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "spline" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_spline(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!calculateSplines(sCmd, _parser, _data, _functions, _option))
        doc_Help("spline", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "save" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_save(string& sCmd)
{
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "define"))
    {
        _functions.save(_option);
        return COMMAND_PROCESSED;
    }
    else if (findParameter(sCmd, "set") || findParameter(sCmd, "settings"))
    {
        _option.save(_option.getExePath());
        return COMMAND_PROCESSED;
    }
    else
        return saveDataObject(sCmd);
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "set" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_set(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Script& _script = NumeReKernel::getInstance()->getScript();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    int nArgument;
    string sArgument;

    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
        NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    if (findParameter(sCmd, "savepath") || findParameter(sCmd, "savepath", '='))
    {
        sArgument = getPathForSetting(sCmd, "savepath");
        _out.setPath(sArgument, true, _out.getProgramPath());
        _option.setSavePath(_out.getPath());
        _data.setSavePath(_option.getSavePath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "loadpath") || findParameter(sCmd, "loadpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "loadpath");
        _data.setPath(sArgument, true, _data.getProgramPath());
        _option.setLoadPath(_data.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "workpath") || findParameter(sCmd, "workpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "workpath");
        FileSystem _fSys;
        _fSys.setTokens(_option.getTokenPaths());
        _fSys.setPath(sArgument, true, _data.getProgramPath());
        _option.setWorkPath(_fSys.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "viewer") || findParameter(sCmd, "viewer", '='))
    {
        sArgument = getPathForSetting(sCmd, "viewer");
        _option.setViewerPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Imageviewer")) );
    }
    else if (findParameter(sCmd, "editor") || findParameter(sCmd, "editor", '='))
    {
        sArgument = getPathForSetting(sCmd, "editor");
        _option.setEditorPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Texteditor")) );
    }
    else if (findParameter(sCmd, "scriptpath") || findParameter(sCmd, "scriptpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "scriptpath");
        _script.setPath(sArgument, true, _script.getProgramPath());
        _option.setScriptPath(_script.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "plotpath") || findParameter(sCmd, "plotpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "plotpath");
        _pData.setPath(sArgument, true, _pData.getProgramPath());
        _option.setPlotOutputPath(_pData.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "procpath") || findParameter(sCmd, "procpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "procpath");
        _option.setProcPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "plotfont") || findParameter(sCmd, "plotfont", '='))
    {
        if (findParameter(sCmd, "plotfont", '='))
            addArgumentQuotes(sCmd, "plotfont");

        if (!extractFirstParameterStringValue(sCmd, sArgument))
        {
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_DEFAULTFONT"))) );

            do
            {
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sArgument);
            }
            while (!sArgument.length());
        }

        if (sArgument[0] == '"')
            sArgument.erase(0, 1);

        if (sArgument[sArgument.length() - 1] == '"')
            sArgument.erase(sArgument.length() - 1);

        _option.setDefaultPlotFont(sArgument);
        _fontData.LoadFont(_option.getDefaultPlotFont().c_str(), _option.getExePath().c_str());
        _pData.setFont(_option.getDefaultPlotFont());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_DEFAULTFONT"))) );
    }
    else if (findParameter(sCmd, "precision") || findParameter(sCmd, "precision", '='))
    {
        if (!parseCmdArg(sCmd, "precision", _parser, nArgument) || (!nArgument || nArgument > 14))
        {
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_PRECISION")) + " (1-14)") );

            do
            {
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sArgument);
                nArgument = StrToInt(sArgument);
            }
            while (!nArgument || nArgument > 14);
        }

        _option.setprecision(nArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_PRECISION"))) );
    }
    else if (findParameter(sCmd, "draftmode") || findParameter(sCmd, "draftmode", '='))
    {
        if (!parseCmdArg(sCmd, "draftmode", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbUseDraftMode();

        _option.setbUseDraftMode((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DRAFTMODE"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DRAFTMODE"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "extendedfileinfo") || findParameter(sCmd, "extendedfileinfo", '='))
    {
        if (!parseCmdArg(sCmd, "extendedfileinfo", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbShowExtendedFileInfo();

        _option.setbExtendedFileInfo((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_EXTENDEDINFO"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_EXTENDEDINFO"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "loademptycols") || findParameter(sCmd, "loademptycols", '='))
    {
        if (!parseCmdArg(sCmd, "loademptycols", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbLoadEmptyCols();

        _option.setbLoadEmptyCols((bool)nArgument);
        _data.setbLoadEmptyCols((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOADEMPTYCOLS"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOADEMPTYCOLS"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "logfile") || findParameter(sCmd, "logfile", '='))
    {
        if (!parseCmdArg(sCmd, "logfile", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbUseLogFile();

        _option.setbUseLogFile((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOGFILE"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOGFILE"), _lang.get("COMMON_INACTIVE")), _option) );

            NumeReKernel::print(LineBreak("|   (" + _lang.get("BUILTIN_CHECKKEYWORD_SET_RESTART_REQUIRED") + ")", _option) );
        }
    }
    else if (findParameter(sCmd, "mode") || findParameter(sCmd, "mode", '='))
    {
        if (findParameter(sCmd, "mode", '='))
            addArgumentQuotes(sCmd, "mode");

        extractFirstParameterStringValue(sCmd, sArgument);

        if (sArgument.length() && sArgument == "debug")
        {
            if (_option.getUseDebugger())
            {
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEBUGGER"), _lang.get("COMMON_INACTIVE")), _option) );
                _option.setDebbuger(false);
                NumeReKernel::getInstance()->getDebugger().setActive(false);
            }
            else
            {
                _option.setDebbuger(true);
                NumeReKernel::getInstance()->getDebugger().setActive(true);
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEBUGGER"), _lang.get("COMMON_ACTIVE")), _option) );
            }
        }
        else if (sArgument.length() && sArgument == "developer")
        {
            if (_option.getbDebug())
            {
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_INACTIVE"), _option) );
                _option.setbDebug(false);
                _parser.EnableDebugDump(false, false);
            }
            else
            {
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_ACTIVE"), _option) );
                sArgument = "";

                do
                {
                    NumeReKernel::printPreFmt("|\n|<- ");
                    NumeReKernel::getline(sArgument);
                }
                while (!sArgument.length());

                if (sArgument == AutoVersion::STATUS)
                {
                    _option.setbDebug(true);
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_SUCCESS"), _option) );
                    _parser.EnableDebugDump(true, true);
                }
                else
                    NumeReKernel::print(toSystemCodePage( _lang.get("COMMON_CANCEL")) );
            }
        }
    }
    else if (findParameter(sCmd, "compact") || findParameter(sCmd, "compact", '='))
    {
        if (!parseCmdArg(sCmd, "compact", _parser, nArgument) || !(nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbCompact();

        _option.setbCompact((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_COMPACT"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_COMPACT"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "greeting") || findParameter(sCmd, "greeting", '='))
    {
        if (!parseCmdArg(sCmd, "greeting", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbGreeting();

        _option.setbGreeting((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_GREETING"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_GREETING"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "hints") || findParameter(sCmd, "hints", '='))
    {
        if (!parseCmdArg(sCmd, "hints", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbShowHints();

        _option.setbShowHints((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_HINTS"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_HINTS"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "useescinscripts") || findParameter(sCmd, "useescinscripts", '='))
    {
        if (!parseCmdArg(sCmd, "useescinscripts", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbUseESCinScripts();

        _option.setbUseESCinScripts((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_ESC_IN_SCRIPTS"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_ESC_IN_SCRIPTS"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "usecustomlang") || findParameter(sCmd, "usecustomlang", '='))
    {
        if (!parseCmdArg(sCmd, "usecustomlang", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getUseCustomLanguageFiles();

        _option.setUserLangFiles((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_CUSTOM_LANG"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_CUSTOM_LANG"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "externaldocwindow") || findParameter(sCmd, "externaldocwindow", '='))
    {
        if (!parseCmdArg(sCmd, "externaldocwindow", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getUseExternalViewer();

        _option.setExternalDocViewer((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DOC_VIEWER"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DOC_VIEWER"), _lang.get("COMMON_INACTIVE")), _option) );
        }
    }
    else if (findParameter(sCmd, "defcontrol") || findParameter(sCmd, "defcontrol", '='))
    {
        if (!parseCmdArg(sCmd, "defcontrol", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
            nArgument = !_option.getbDefineAutoLoad();

        _option.setbDefineAutoLoad((bool)nArgument);

        if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEFCONTROL"), _lang.get("COMMON_ACTIVE")), _option) );
            else
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEFCONTROL"), _lang.get("COMMON_INACTIVE")), _option) );
        }

        if (_option.getbDefineAutoLoad() && !_functions.getDefinedFunctions() && fileExists(_option.getExePath() + "\\functions.def"))
            _functions.load(_option);
    }
    else if (findParameter(sCmd, "autosave") || findParameter(sCmd, "autosave", '='))
    {
        if (!parseCmdArg(sCmd, "autosave", _parser, nArgument) && !nArgument)
        {
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_AUTOSAVE") + "? [sec]") );

            do
            {
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sArgument);
                nArgument = StrToInt(sArgument);
            }
            while (!nArgument);
        }

        _option.setAutoSaveInterval(nArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_AUTOSAVE"))) );
    }
    else if (findParameter(sCmd, "buffersize") || findParameter(sCmd, "buffersize", '='))
    {
        if (!parseCmdArg(sCmd, "buffersize", _parser, nArgument) || nArgument < 300)
        {
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_BUFFERSIZE") + "? (>= 300)") );

            do
            {
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sArgument);
                nArgument = StrToInt(sArgument);
            }
            while (nArgument < 300);
        }

        _option.setWindowBufferSize(0, (unsigned)nArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_BUFFERSIZE"))) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (findParameter(sCmd, "windowsize"))
    {
        if (findParameter(sCmd, "x", '='))
        {
            parseCmdArg(sCmd, "x", _parser, nArgument);
            _option.setWindowSize((unsigned)nArgument, 0);
            _option.setWindowBufferSize(_option.getWindow() + 1, 0);
            NumeReKernel::nLINE_LENGTH = _option.getWindow();
        }

        if (findParameter(sCmd, "y", '='))
        {
            parseCmdArg(sCmd, "y", _parser, nArgument);
            _option.setWindowSize(0, (unsigned)nArgument);

            if (_option.getWindow(1) + 1 > _option.getBuffer(1))
                _option.setWindowBufferSize(0, _option.getWindow(1) + 1);
        }

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_WINDOWSIZE"))) );
    }
    else if (findParameter(sCmd, "save"))
        _option.save(_option.getExePath());
    else
        doc_Help("set", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "start" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_start(string& sCmd)
{
    Script& _script = NumeReKernel::getInstance()->getScript();

    string sArgument;

    if (_script.isOpen())
        throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, "start");

    if (findParameter(sCmd, "script") || findParameter(sCmd, "script", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "install"))
            _script.setInstallProcedures();

        if (findParameter(sCmd, "script", '='))
            addArgumentQuotes(sCmd, "script");

        if (extractFirstParameterStringValue(sCmd, sArgument))
            _script.openScript(sArgument);
        else
            _script.openScript();
    }
    else
    {
        if (!_script.isOpen())
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (containsStrings(sCmd))
                extractFirstParameterStringValue(sCmd, sArgument);
            else
                sArgument = sCmd.substr(findCommand(sCmd).nPos + 6);

            StripSpaces(sArgument);

            if (!sArgument.length())
            {
                if (_script.getScriptFileName().length())
                    _script.openScript();
                else
                    throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sArgument, "[" + _lang.get("BUILTIN_CHECKKEYWORD_START_ERRORTOKEN") + "]");

                return COMMAND_PROCESSED;
            }

            _script.openScript(sArgument);
        }
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "script" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_script(string& sCmd)
{
    Script& _script = NumeReKernel::getInstance()->getScript();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;

    if (findParameter(sCmd, "load") || findParameter(sCmd, "load", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

        if (!_script.isOpen())
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (findParameter(sCmd, "load", '='))
                addArgumentQuotes(sCmd, "load");

            if (!extractFirstParameterStringValue(sCmd, sArgument))
            {
                do
                {
                    NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTNAME"))) );
                    NumeReKernel::printPreFmt("|<- ");
                    NumeReKernel::getline(sArgument);
                }
                while (!sArgument.length());
            }

            _script.setScriptFileName(sArgument);

            if (fileExists(_script.getScriptFileName()))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SCRIPTLOAD_SUCCESS", _script.getScriptFileName()));
            }
            else
            {
                string sErrorToken = _script.getScriptFileName();
                sArgument = "";
                _script.setScriptFileName(sArgument);
                throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sErrorToken, sErrorToken);
            }
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, "script");
    }
    else if (findParameter(sCmd, "start") || findParameter(sCmd, "start", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

        if (_script.isOpen())
            throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, "script");

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "install"))
            _script.setInstallProcedures();

        if (findParameter(sCmd, "start", '='))
            addArgumentQuotes(sCmd, "start");

        if (extractFirstParameterStringValue(sCmd, sArgument))
            _script.openScript(sArgument);
        else
            _script.openScript();
    }
    else
        doc_Help("script", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "show" command. Editing of tables is not
/// supplied by this function.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_show(string& sCmd)
{
    // Get references to the main objects
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Handle the compact mode (probably not needed any more)
    if (sCmd.substr(0, 5) == "showf")
        _out.setCompact(false);
    else
        _out.setCompact(_option.getbCompact());

    // Determine the correct data object
    if (_data.matchTableAsParameter(sCmd).length())
    {
        // a cache as object, passed as parameter
        show_data(_data, _out, _option, _data.matchTableAsParameter(sCmd), _option.getPrecision(), false, true);
    }
    else
    {
        DataAccessParser _accessParser(sCmd);

        if (_accessParser.getDataObject().length())
        {
            if (_accessParser.isCluster())
            {
                NumeRe::Cluster& cluster = _data.getCluster(_accessParser.getDataObject());

                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, cluster.size()-1);

                // Create the target container
                NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), 1);

                // Copy the data to the new container
                for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    if (cluster.getType(i) == NumeRe::ClusterItem::ITEMTYPE_STRING)
                        _stringTable.set(i, 0, cluster.getString(_accessParser.getIndices().row[i]));
                    else
                        _stringTable.set(i, 0, toString(cluster.getDouble(_accessParser.getIndices().row[i]), 5));
                }

                // Redirect control
                NumeReKernel::showStringTable(_stringTable, _accessParser.getDataObject() + "{}");

                return COMMAND_PROCESSED;
            }
            else if (_accessParser.getDataObject() == "string")
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getStringElements()-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getStringCols()-1);

                // Create the target container
                NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size());

                // Copy the data to the new container and add surrounding
                // quotation marks
                for (size_t j = 0; j < _accessParser.getIndices().col.size(); j++)
                {
                    for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
                    {
                        if (_data.getStringElements(_accessParser.getIndices().col[j]) <= _accessParser.getIndices().row[i])
                            break;

                        _stringTable.set(i, j, "\"" + _data.readString(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j]) + "\"");
                    }
                }

                // Redirect control
                NumeReKernel::showStringTable(_stringTable, "string()");
            }
            else
            {
                MemoryManager _cache;

                // Validize the obtained index sets
                if (!isValidIndexSet(_accessParser.getIndices()))
                    throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

                // Copy the target data to a new table
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

                _cache.resizeTable(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size(), "table");
                _cache.renameTable("table", "*" + _accessParser.getDataObject(), true);

                for (unsigned int i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    for (unsigned int j = 0; j < _accessParser.getIndices().col.size(); j++)
                    {
                        if (!i)
                        {
                            _cache.setHeadLineElement(j, "*" + _accessParser.getDataObject(), _data.getHeadLineElement(_accessParser.getIndices().col[j], _accessParser.getDataObject()));
                        }

                        if (_data.isValidElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()))
                            _cache.writeToTable(i, j, "*" + _accessParser.getDataObject(), _data.getElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()));
                    }
                }

                if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                    NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

                // Redirect the control
                show_data(_cache, _out, _option, "*" + _accessParser.getDataObject(), _option.getPrecision(), false, true);
                return COMMAND_PROCESSED;
            }
        }
        else
        {
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
        }

    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "smooth" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_smooth(string& sCmd)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nWindowSize = 1;
    double dAlpha = NAN;
    NumeRe::FilterSettings::FilterType _type = NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR;
    MemoryManager::AppDir dir = MemoryManager::ALL;

    // Find the window size
    if (findParameter(sCmd, "order", '='))
    {
        nWindowSize = findParameter(sCmd, "order", '=') + 5;

        if (_data.containsTablesOrClusters(sCmd.substr(nWindowSize)))
        {
            sArgument = sCmd.substr(nWindowSize);
            getDataElements(sArgument, _parser, _data, _option);

            if (sArgument.find("{") != string::npos)
                convertVectorToExpression(sArgument, _option);

            sCmd = sCmd.substr(0, nWindowSize) + sArgument;
        }

        _parser.SetExpr(getArgAtPos(sCmd, nWindowSize));
        nWindowSize = intCast(_parser.Eval());
    }

    // Find the window shape (used for type=gaussian)
    if (findParameter(sCmd, "alpha", '='))
    {
        size_t pos = findParameter(sCmd, "alpha", '=') + 5;

        if (_data.containsTablesOrClusters(sCmd.substr(pos)))
        {
            sArgument = sCmd.substr(pos);
            getDataElements(sArgument, _parser, _data, _option);

            if (sArgument.find("{") != string::npos)
                convertVectorToExpression(sArgument, _option);

            sCmd = sCmd.substr(0, pos) + sArgument;
        }

        _parser.SetExpr(getArgAtPos(sCmd, pos));
        dAlpha = _parser.Eval();
    }

    // Find the smoothing filter type
    if (findParameter(sCmd, "type", '='))
    {
        string sFilterType = getArgAtPos(sCmd, findParameter(sCmd, "type", '=')+4);

        if (sFilterType == "weightedlinear")
            _type = NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR;
        else if (sFilterType == "gaussian")
            _type = NumeRe::FilterSettings::FILTER_GAUSSIAN;
        else if (sFilterType == "savitzkygolay")
            _type = NumeRe::FilterSettings::FILTER_SAVITZKY_GOLAY;
    }

    // Find the app dir
    if (findParameter(sCmd, "grid"))
        dir = MemoryManager::GRID;
    else if (findParameter(sCmd, "lines"))
        dir = MemoryManager::LINES;
    else if (findParameter(sCmd, "cols"))
        dir = MemoryManager::COLS;

    if (!_data.containsTablesOrClusters(sCmd))
        return COMMAND_PROCESSED;

    DataAccessParser _access(sCmd);

    if (_access.getDataObject().length())
    {
        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject());

        if (_access.getIndices().row.isOpenEnd())
            _access.getIndices().row.setRange(0, _data.getLines(_access.getDataObject(), false)-1);

        if (_access.getIndices().col.isOpenEnd())
            _access.getIndices().col.setRange(0, _data.getCols(_access.getDataObject())-1);

        bool success = false;

        // Apply the smoothing filter
        switch (dir)
        {
            case MemoryManager::GRID:
            case MemoryManager::ALL:
                success = _data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, NumeRe::FilterSettings(_type, nWindowSize, nWindowSize, dAlpha), dir);
                break;
            case MemoryManager::LINES:
            case MemoryManager::COLS:
                success = _data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, NumeRe::FilterSettings(_type, nWindowSize, 1u, dAlpha), dir);
                break;
        }

        if (success)
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", "\"" + _access.getDataObject() + "\""));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "string" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_string(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "clear"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));

        if (_data.clearStringElements())
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_SUCCESS"));
        }
        else
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_EMPTY"));
        }

        return COMMAND_PROCESSED;
    }

    return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "swap" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_swap(string& sCmd)
{
    return swapTables(sCmd, NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "hist" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_hist(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    string sArgument = evaluateParameterValues(sCmd);
    string sCommand = findCommand(sCmd).sString;

    if (findParameter(sCmd, "data") && !_data.isEmpty("data"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRACATED"));
        plugin_histogram(sArgument, _data, _data, _out, _option, _pData, false, true);
    }
    else if (_data.matchTableAsParameter(sCmd).length())
    {
        // a cache as object, passed as parameter
        // DEPRECATED: Declared at v1.1.2rc2
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
    }
    else
    {
        DataAccessParser _accessParser(sCmd);

        if (_accessParser.getDataObject().length())
        {
            MemoryManager _cache;

            copyDataToTemporaryTable(sCmd, _accessParser, _data, _cache);

            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (findParameter(sCmd, "export", '='))
                addArgumentQuotes(sCmd, "export");

            if (sCommand == "hist2d")
                sArgument = "hist2d -cache c=1:inf " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));
            else
                sArgument = "hist -cache c=1:inf " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));

            sArgument = evaluateParameterValues(sArgument);
            plugin_histogram(sArgument, _cache, _data, _out, _option, _pData, true, false);
            return COMMAND_PROCESSED;
        }
        else
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "help" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_help(string& sCmd)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findCommand(sCmd).nPos + findCommand(sCmd).sString.length() < sCmd.length())
        doc_Help(sCmd.substr(findCommand(sCmd).nPos + findCommand(sCmd).sString.length()), _option);
    else
        doc_Help("brief", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "move" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_move(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    int nArgument;

    if (sCmd.length() > 5)
    {
        if (_data.containsTablesOrClusters(sCmd) && (findParameter(sCmd, "target", '=') || findParameter(sCmd, "t", '=')))
        {
            if (moveData(sCmd, _parser, _data, _option))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_MOVEDATA_SUCCESS"), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_MOVE_DATA, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            if (findParameter(sCmd, "all") || findParameter(sCmd, "a"))
                nArgument = 1;
            else
                nArgument = 0;

            if (moveFile(sCmd, _parser, _data, _option))
            {
                if (_option.getSystemPrintStatus())
                {
                    if (nArgument)
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_ALL_SUCCESS", sCmd));
                    else
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_SUCCESS", sCmd));
                }
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_MOVE_FILE, sCmd, SyntaxError::invalid_position, sCmd);
        }
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "hline" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_hline(string& sCmd)
{
    if (findParameter(sCmd, "single"))
        make_hline(-2);
    else
        make_hline();

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "matop" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_matop(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    performMatrixOperation(sCmd, _parser, _data, _functions, _option);
    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "random" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_random(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "o"))
        plugin_random(sCmd, _data, _out, _option, true);
    else
        plugin_random(sCmd, _data, _out, _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "redefine" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_redefine(string& sCmd)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (sCmd.length() > findCommand(sCmd).sString.length() + 1)
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "comment", '='))
            addArgumentQuotes(sCmd, "comment");

        if (_functions.defineFunc(sCmd.substr(sCmd.find(' ') + 1), true))
            NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
        else
            NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
    }
    else
        doc_Help("define", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "resample" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_resample(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (!_data.containsTablesOrClusters(sCmd))
        return COMMAND_PROCESSED;

    DataAccessParser _access(sCmd);

    if (_access.getDataObject().length())
    {
        if (findParameter(sCmd, "samples", '='))
        {
            nArgument = findParameter(sCmd, "samples", '=') + 7;

            if (_data.containsTablesOrClusters(getArgAtPos(sCmd, nArgument)))
            {
                sArgument = getArgAtPos(sCmd, nArgument);
                getDataElements(sArgument, _parser, _data, _option);

                if (sArgument.find("{") != string::npos)
                    convertVectorToExpression(sArgument, _option);

                sCmd.replace(nArgument, getArgAtPos(sCmd, nArgument).length(), sArgument);
            }

            _parser.SetExpr(getArgAtPos(sCmd, nArgument));
            nArgument = intCast(_parser.Eval());
        }
        else
            nArgument = _data.getLines(_access.getDataObject(), false);

        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject(), _access.getDataObject());

        if (_access.getIndices().row.isOpenEnd())
            _access.getIndices().row.setRange(0, _data.getLines(_access.getDataObject(), false)-1);

        if (_access.getIndices().col.isOpenEnd())
            _access.getIndices().col.setRange(0, _data.getCols(_access.getDataObject())-1);

        if (findParameter(sCmd, "grid"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::GRID))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (!findParameter(sCmd, "lines") && !findParameter(sCmd, "cols"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::ALL))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (findParameter(sCmd, "cols"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::COLS))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_COLS")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (findParameter(sCmd, "lines"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::LINES))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_LINES")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "remove" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_remove(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (_data.containsTablesOrClusters(sCmd))
    {
        while (_data.containsTablesOrClusters(sCmd))
        {
            for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
            {
                if (sCmd.find(iter->first + "()") != string::npos && iter->first != "table")
                {
                    string sObj = iter->first;
                    if (_data.deleteTable(iter->first))
                    {
                        if (sArgument.length())
                            sArgument += ", ";

                        sArgument += "\"" + sObj + "()\"";
                        break;
                    }
                }
            }
        }

        if (sArgument.length() && _option.getSystemPrintStatus())
            NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_REMOVECACHE", sArgument));
    }
    else if (sCmd.length() > 7)
    {
        if (findParameter(sCmd, "all") || findParameter(sCmd, "a"))
            nArgument = 1;
        else
            nArgument = 0;

        if (!removeFile(sCmd, _parser, _data, _option))
            throw SyntaxError(SyntaxError::CANNOT_REMOVE_FILE, sCmd, SyntaxError::invalid_position, sCmd);
        else if (_option.getSystemPrintStatus())
        {
            if (nArgument)
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_REMOVE_ALL_FILE"));
            else
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_REMOVE_FILE"));
        }
    }
    else
        throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "rename" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_rename(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;

    // If the current command line contains strings
    // handle them here
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        sCmd = evaluateParameterValues(sCmd);

    // Handle legacy and new syntax in these two cases
    if (_data.matchTableAsParameter(sCmd, '=').length())
    {
        // Legacy syntax: rename -cache1=cache2
        //
        // Get the option value of the parameter "cache1"
        sArgument = getArgAtPos(sCmd, findParameter(sCmd, _data.matchTableAsParameter(sCmd, '='), '=') + _data.matchTableAsParameter(sCmd, '=').length());

        // Rename the cache
        _data.renameTable(_data.matchTableAsParameter(sCmd, '='), sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sArgument), _option) );
    }
    else if (sCmd.find("()") != string::npos && sCmd.find(',') != string::npos)
    {
        // New syntax: rename cache1(), cache2()
        //
        // Extract the first of the two arguments
        // (length of command = 6)
        sCmd.erase(0, 6);
        sArgument = getNextArgument(sCmd, true);

        if (!sCmd.length())
            return COMMAND_PROCESSED;

        // Remove parentheses, if available
        if (sArgument.find('(') != string::npos)
            sArgument.erase(sArgument.find('('));

        if (sCmd.find('(') != string::npos)
            sCmd.erase(sCmd.find('('));

        // Remove not necessary white spaces
        StripSpaces(sArgument);
        StripSpaces(sCmd);

        // Rename the cache
        _data.renameTable(sArgument, sCmd);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sCmd), _option) );
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "reload" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_reload(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (findParameter(sCmd, "data") || findParameter(sCmd, "data", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "data", '='))
            addArgumentQuotes(sCmd, "data");

        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            if (!_data.isEmpty("data"))
            {
                _data.removeData(false);

                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;

                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument);

                if (!_data.isEmpty("data") && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else if (!_data.isEmpty("data"))
        {
            if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !findParameter(sCmd, "data", '='))
                throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, sCmd, SyntaxError::invalid_position);

            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            sArgument = _data.getDataFileName("data");
            _data.removeData(false);
            _data.openFile(sArgument);

            if (!_data.isEmpty("data") && _option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_RELOAD_SUCCESS"));
        }
        else
            load_data(_data, _option, _parser);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "retouch" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_retouch(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!_data.containsTablesOrClusters(sCmd))
        return COMMAND_PROCESSED;

    // DEPRECATED: Declared at v1.1.2rc1
    if (findCommand(sCmd).sString == "retoque")
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

    DataAccessParser _access(sCmd);

    if (_access.getDataObject().length())
    {
        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject(), _access.getDataObject());

        if (_access.getIndices().row.isOpenEnd())
            _access.getIndices().row.setRange(0, _data.getLines(_access.getDataObject(), false)-1);

        if (_access.getIndices().col.isOpenEnd())
            _access.getIndices().col.setRange(0, _data.getCols(_access.getDataObject())-1);

        if (findParameter(sCmd, "grid"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::GRID))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (!findParameter(sCmd, "lines") && !findParameter(sCmd, "cols"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::ALL))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (findParameter(sCmd, "lines"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::LINES))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_LINES")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (findParameter(sCmd, "cols"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::COLS))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_COLS")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "regularize" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_regularize(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!regularizeDataSet(sCmd, _parser, _data, _functions, _option))
        throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, SyntaxError::invalid_position);
    else if (_option.getSystemPrintStatus())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_REGULARIZE"));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "define" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_define(string& sCmd)
{
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    if (sCmd.length() > 8)
    {
        _functions.setTableList(_data.getTableNames());

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (findParameter(sCmd, "comment", '='))
            addArgumentQuotes(sCmd, "comment");

        if (findParameter(sCmd, "save"))
        {
            _functions.save(_option);
            return COMMAND_PROCESSED;
        }
        else if (findParameter(sCmd, "load"))
        {
            if (fileExists(_option.getExePath() + "\\functions.def"))
                _functions.load(_option);
            else
                NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY")) );

            return COMMAND_PROCESSED;
        }
        else
        {
            if (_functions.defineFunc(sCmd.substr(7)))
                NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
            else
                NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
        }
    }
    else
        doc_Help("define", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "delete" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_delete(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (_data.containsTablesOrClusters(sCmd))
    {
        if (findParameter(sCmd, "ignore") || findParameter(sCmd, "i"))
        {
            if (deleteCacheEntry(sCmd, _parser, _data, _option))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_SUCCESS"));
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_DELETE_ELEMENTS, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_CONFIRM"), _option));

            do
            {
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sArgument);
                StripSpaces(sArgument);
            }
            while (!sArgument.length());

            if (sArgument.substr(0, 1) == _lang.YES())
            {
                if (deleteCacheEntry(sCmd, _parser, _data, _option))
                {
                    if (_option.getSystemPrintStatus())
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_SUCCESS"));
                }
                else
                    throw SyntaxError(SyntaxError::CANNOT_DELETE_ELEMENTS, sCmd, SyntaxError::invalid_position);
            }
            else
            {
                NumeReKernel::print(_lang.get("COMMON_CANCEL") );
                return COMMAND_PROCESSED;
            }
        }
    }
    else if (sCmd.find("string()") != string::npos || sCmd.find("string(:)") != string::npos)
    {
        if (_data.removeStringElements(0))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_SUCCESS", "1"));
        }
        else
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_EMPTY", "1"));
        }

        return COMMAND_PROCESSED;
    }
    else if (sCmd.find(" string(", findCommand(sCmd).nPos) != string::npos)
    {
        _parser.SetExpr(sCmd.substr(sCmd.find(" string(", findCommand(sCmd).nPos) + 8, getMatchingParenthesis(sCmd.substr(sCmd.find(" string(", findCommand(sCmd).nPos) + 7)) - 1));
        nArgument = (int)_parser.Eval() - 1;

        if (_data.removeStringElements(nArgument))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_SUCCESS", toString(nArgument + 1)));
        }
        else
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_EMPTY", toString(nArgument + 1)));
        }

        return COMMAND_PROCESSED;

    }
    else
        doc_Help("delete", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "datagrid" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_datagrid(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument = "grid";

    if (!createDatagrid(sCmd, sArgument, _parser, _data, _functions, _option))
        doc_Help("datagrid", _option);
    else if (_option.getSystemPrintStatus())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DATAGRID_SUCCESS", sArgument));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "list" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_list(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;

    if (findParameter(sCmd, "files") || (findParameter(sCmd, "files", '=')))
        listFiles(sCmd, _option);
    else if (findParameter(sCmd, "var"))
        listDeclaredVariables(_parser, _option, _data);
    else if (findParameter(sCmd, "const"))
        listConstants(_parser, _option);
    else if ((findParameter(sCmd, "func") || findParameter(sCmd, "func", '=')))
    {
        if (findParameter(sCmd, "func", '='))
            sArgument = getArgAtPos(sCmd, findParameter(sCmd, "func", '=') + 4);
        else
            listFunctions(_option, "all");

        if (sArgument == "num" || sArgument == "numerical")
            listFunctions(_option, "num");
        else if (sArgument == "mat" || sArgument == "matrix" || sArgument == "vec" || sArgument == "vector")
            listFunctions(_option, "mat");
        else if (sArgument == "string")
            listFunctions(_option, "string");
        else if (sArgument == "trigonometric")
            listFunctions(_option, "trigonometric");
        else if (sArgument == "hyperbolic")
            listFunctions(_option, "hyperbolic");
        else if (sArgument == "logarithmic")
            listFunctions(_option, "logarithmic");
        else if (sArgument == "polynomial")
            listFunctions(_option, "polynomial");
        else if (sArgument == "stats" || sArgument == "statistical")
            listFunctions(_option, "stats");
        else if (sArgument == "angular")
            listFunctions(_option, "angular");
        else if (sArgument == "physics" || sArgument == "physical")
            listFunctions(_option, "physics");
        else if (sArgument == "logic" || sArgument == "logical")
            listFunctions(_option, "logic");
        else if (sArgument == "time")
            listFunctions(_option, "time");
        else if (sArgument == "distrib")
            listFunctions(_option, "distrib");
        else if (sArgument == "random")
            listFunctions(_option, "random");
        else if (sArgument == "coords")
            listFunctions(_option, "coords");
        else if (sArgument == "draw")
            listFunctions(_option, "draw");
        else
            listFunctions(_option, "all");

    }
    else if (findParameter(sCmd, "logic"))
        listLogicalOperators(_option);
    else if (findParameter(sCmd, "cmd"))
        listCommands(_option);
    else if (findParameter(sCmd, "define"))
        listDefinitions(_functions, _option);
    else if (findParameter(sCmd, "settings"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        listOptions(_option);
    }
    else if (findParameter(sCmd, "units"))
        listUnitConversions(_option);
    else if (findParameter(sCmd, "plugins"))
        listInstalledPlugins(_parser, _data, _option);
    else
        doc_Help("list", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "load" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_load(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Script& _script = NumeReKernel::getInstance()->getScript();

    string sArgument;
    int nArgument;

    if (findParameter(sCmd, "define"))
    {
        if (fileExists("functions.def"))
            _functions.load(_option);
        else
            NumeReKernel::print( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY") );
    }
    else if (findParameter(sCmd, "data") || findParameter(sCmd, "data", '=')) // deprecated
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (findParameter(sCmd, "data", '='))
            addArgumentQuotes(sCmd, "data");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (findParameter(sCmd, "slice", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slice", '=') + 5) == "xz")
                nArgument = -1;
            else if (findParameter(sCmd, "slice", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slice", '=') + 5) == "yz")
                nArgument = -2;
            else
                nArgument = 0;
            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (findParameter(sCmd, "tocache") && !findParameter(sCmd, "all"))
            {
                MemoryManager _cache;
                _cache.setTokens(_option.getTokenPaths());
                _cache.setPath(_option.getLoadPath(), false, _option.getExePath());
                _cache.openFile(sArgument, false, nArgument);
                sArgument = generateCacheName(sArgument, _option);
                if (!_data.isTable(sArgument + "()"))
                    _data.addTable(sArgument + "()", _option);
                nArgument = _data.getCols(sArgument, false);
                for (long long int i = 0; i < _cache.getLines("data", false); i++)
                {
                    for (long long int j = 0; j < _cache.getCols("data", false); j++)
                    {
                        if (!i)
                            _data.setHeadLineElement(j + nArgument, sArgument, _cache.getHeadLineElement(j, "data"));
                        if (_cache.isValidElement(i, j, "data"))
                        {
                            _data.writeToTable(i, j + nArgument, sArgument, _cache.getElement(i, j, "data"));
                        }
                    }
                }
                if (!_data.isEmpty(sArgument))
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines(sArgument, false)), toString(_data.getCols(sArgument, false))), _option) );
                return COMMAND_PROCESSED;
            }
            else if (findParameter(sCmd, "tocache") && findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
            {
                if (sArgument.find('/') == string::npos)
                    sArgument = "<loadpath>/" + sArgument;
                vector<string> vFilelist = getFileList(sArgument, _option);
                if (!vFilelist.size())
                    throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                string sPath = "<loadpath>/";
                if (sArgument.find('/') != string::npos)
                    sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
                string sTarget = generateCacheName(sPath + vFilelist[0], _option);
                MemoryManager _cache;
                _cache.setTokens(_option.getTokenPaths());
                _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                for (unsigned int i = 0; i < vFilelist.size(); i++)
                {
                    _cache.openFile(sPath + vFilelist[i], false, nArgument);
                    sTarget = generateCacheName(sPath + vFilelist[i], _option);
                    if (!_data.isTable(sTarget + "()"))
                        _data.addTable(sTarget + "()", _option);
                    nArgument = _data.getCols(sTarget, false);
                    for (long long int i = 0; i < _cache.getLines("data", false); i++)
                    {
                        for (long long int j = 0; j < _cache.getCols("data", false); j++)
                        {
                            if (!i)
                                _data.setHeadLineElement(j + nArgument, sTarget, _cache.getHeadLineElement(j, "data"));
                            if (_cache.isValidElement(i, j, "data"))
                            {
                                _data.writeToTable(i, j + nArgument, sTarget, _cache.getElement(i, j, "data"));
                            }
                        }
                    }
                    _cache.removeData(false);
                    nArgument = -1;
                }
                if (!_data.isEmpty("data"))
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument), _option) );
                //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                return COMMAND_PROCESSED;
            }
            if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore"))
            {
                if (!_data.isEmpty("data"))
                {
                    if (_option.getSystemPrintStatus())
                        _data.removeData(false);
                    else
                        _data.removeData(true);
                }
                if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);

                    for (unsigned int i = 0; i < vFilelist.size(); i++)
                    {
                        _data.openFile(sPath + vFilelist[i], false, nArgument);
                    }
                    if (!_data.isEmpty("data"))
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }

                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument, false, nArgument);
                if (!_data.isEmpty("data") && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

            }
            else if (_data.isEmpty("data"))
            {
                if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);

                    for (unsigned int i = 0; i < vFilelist.size(); i++)
                    {
                        _data.openFile(sPath + vFilelist[i], false, nArgument);
                    }
                    if (!_data.isEmpty("data"))
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument, false, nArgument);
                if (!_data.isEmpty("data") && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else
            load_data(_data, _option, _parser);
    }
    else if (findParameter(sCmd, "script") || findParameter(sCmd, "script", '=')) // deprecated
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        if (!_script.isOpen())
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
            if (findParameter(sCmd, "script", '='))
                addArgumentQuotes(sCmd, "script");
            if (!extractFirstParameterStringValue(sCmd, sArgument))
            {
                do
                {
                    NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTNAME"))) );
                    //NumeReKernel::print("|-> Dateiname des Scripts angeben:" );
                    NumeReKernel::printPreFmt("|<- ");
                    NumeReKernel::getline(sArgument);
                }
                while (!sArgument.length());
            }
            _script.setScriptFileName(sArgument);
            if (fileExists(_script.getScriptFileName()))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTLOAD_SUCCESS", _script.getScriptFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Script \"" + _script.getScriptFileName() + "\" wurde erfolgreich geladen!", _option) );
            }
            else
            {
                string sErrorToken = _script.getScriptFileName();
                sArgument = "";
                _script.setScriptFileName(sArgument);
                throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sErrorToken, sErrorToken);
            }
        }
        return COMMAND_PROCESSED;
    }
    else if (sCmd.length() > findCommand(sCmd, "load").nPos + 5 && sCmd.find_first_not_of(' ', findCommand(sCmd, "load").nPos + 5) != string::npos)
    {
        Match _match = findCommand(sCmd, "load");
        string sExpr = sCmd;

        sExpr.replace(_match.nPos, string::npos, "_~load[~_~]");
        sCmd.erase(0, _match.nPos);

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        // Add quotation marks around the object, if there aren't any
        if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5)] != '"' && sCmd.find("string(") == string::npos)
        {
            if (findParameter(sCmd, "slice")
                    || findParameter(sCmd, "keepdim")
                    || findParameter(sCmd, "complete")
                    || findParameter(sCmd, "ignore")
                    || findParameter(sCmd, "tocache")
                    || findParameter(sCmd, "i")
                    || findParameter(sCmd, "head")
                    || findParameter(sCmd, "h")
                    || findParameter(sCmd, "app")
                    || findParameter(sCmd, "all"))
            {
                sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), 1, '"');
                nArgument = string::npos;

                while (sCmd.find_last_of('-', nArgument) != string::npos
                    && sCmd.find_last_of('-', nArgument) > sCmd.find_first_of(' ', sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5)))
                    nArgument = sCmd.find_last_of('-', nArgument) - 1;

                nArgument = sCmd.find_last_not_of(' ', nArgument);
                sCmd.insert(nArgument + 1, 1, '"');
            }
            else
            {
                sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), 1, '"');
                sCmd.insert(sCmd.find_last_not_of(' ') + 1, 1, '"');
            }
        }

        if (findParameter(sCmd, "app"))
        {
            double j1 = _data.getCols("data") + 1;

            sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), "-app=");
            append_data(sCmd, _data, _option);

            sCmd = sExpr;
            vector<double> vIndices;
            vIndices.push_back(1);
            vIndices.push_back(_data.getLines("data", true) - _data.getAppendedZeroes(j1, "data"));
            vIndices.push_back(j1);
            vIndices.push_back(_data.getCols("data"));

            _parser.SetVectorVar("_~load[~_~]", vIndices);

            return COMMAND_HAS_RETURNVALUE;
        }

        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (findParameter(sCmd, "slice", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slice", '=') + 5) == "xz")
                nArgument = -1;
            else if (findParameter(sCmd, "slice", '=') && getArgAtPos(sCmd, findParameter(sCmd, "slice", '=') + 5) == "yz")
                nArgument = -2;
            else
                nArgument = 0;

            if (findParameter(sCmd, "keepdim") || findParameter(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            if (findParameter(sCmd, "tocache") && !findParameter(sCmd, "all"))
            {
                // Single file directly to cache
                NumeRe::FileHeaderInfo info = _data.openFile(sArgument, true);

                if (!_data.isEmpty(info.sTableName))
                {
                    NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", info.sTableName + "()", toString(_data.getLines(info.sTableName, false)), toString(_data.getCols(info.sTableName, false))));
                    sCmd = sExpr;
                    vector<double> vIndices;
                    vIndices.push_back(1);
                    vIndices.push_back(info.nRows);
                    vIndices.push_back(_data.getCols(info.sTableName)-info.nCols+1);
                    vIndices.push_back(_data.getCols(info.sTableName));

                    _parser.SetVectorVar("_~load[~_~]", vIndices);

                    return COMMAND_HAS_RETURNVALUE;
                }

                return COMMAND_PROCESSED;
            }
            else if (findParameter(sCmd, "tocache") && findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
            {
                // multiple files directly to cache
                if (sArgument.find('/') == string::npos)
                    sArgument = "<loadpath>/" + sArgument;

                vector<string> vFilelist = getFileList(sArgument, _option, 1);

                if (!vFilelist.size())
                    throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);

                for (size_t i = 0; i < vFilelist.size(); i++)
                    _data.openFile(vFilelist[i], true);

                if (!_data.isEmpty(vFilelist.front()))
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument));

                return COMMAND_PROCESSED;
            }

            if (findParameter(sCmd, "i") || findParameter(sCmd, "ignore") || _data.isEmpty("data"))
            {
                if (!_data.isEmpty("data"))
                    _data.removeData();

                // multiple files
                if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;

                    vector<string> vFilelist = getFileList(sArgument, _option, 1);

                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);

                    for (size_t i = 0; i < vFilelist.size(); i++)
                    {
                        // Melting is done automatically
                        _data.openFile(vFilelist[i], false, nArgument);
                    }

                    if (!_data.isEmpty("data"))
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", false)), toString(_data.getCols("data", false))));

                    vector<double> vIndices = {1, _data.getLines("data", false), 1, _data.getCols("data", false)};
                    _parser.SetVectorVar("_~load[~_~]", vIndices);
                    sCmd = sExpr;

                    return COMMAND_HAS_RETURNVALUE;
                }

                // Provide headline
                if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
                {
                    if (findParameter(sCmd, "head", '='))
                        nArgument = findParameter(sCmd, "head", '=') + 4;
                    else
                        nArgument = findParameter(sCmd, "h", '=') + 1;

                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, false, nArgument);
                }
                else
                    _data.openFile(sArgument, false, nArgument);

                if (!_data.isEmpty("data"))
                {
                    if (_option.getSystemPrintStatus())
                        NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", false)), toString(_data.getCols("data", false))));

                    vector<double> vIndices = {1, _data.getLines("data", false), 1, _data.getCols("data", false)};
                    _parser.SetVectorVar("_~load[~_~]", vIndices);
                    sCmd = sExpr;

                    return COMMAND_HAS_RETURNVALUE;
                }
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else
            load_data(_data, _option, _parser);
    }
    else
        doc_Help("load", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "execute" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_execute(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    executeCommand(sCmd, _parser, _data, _functions, _option);
    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "paste" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
/// \deprecated Will be removed with v1.1.3rc1
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_paste(string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (findParameter(sCmd, "data"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        PasteHandler _handler;
        _data.melt(_handler.pasteLoad(_option), "data");
        if (!_data.isEmpty("data"))
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_PASTE_SUCCESS", toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );

        return COMMAND_PROCESSED;

    }

    return NO_COMMAND;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "progress" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_progress(string& sCmd)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    string sArgument;
    int nArgument;
    value_type* vVals = 0;
    string sExpr;

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        sCmd = evaluateParameterValues(sCmd);

    if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
    {
        if (sCmd.find("-set") != string::npos)
            sArgument = sCmd.substr(sCmd.find("-set"));
        else
            sArgument = sCmd.substr(sCmd.find("--"));

        sCmd.erase(sCmd.find(sArgument));

        if (findParameter(sArgument, "first", '='))
            sExpr = getArgAtPos(sArgument, findParameter(sArgument, "first", '=') + 5) + ",";
        else
            sExpr = "1,";

        if (findParameter(sArgument, "last", '='))
            sExpr += getArgAtPos(sArgument, findParameter(sArgument, "last", '=') + 4);
        else
            sExpr += "100";

        if (findParameter(sArgument, "type", '='))
        {
            sArgument = getArgAtPos(sArgument, findParameter(sArgument, "type", '=') + 4);

            if (containsStrings(sArgument))
            {
                if (sArgument.front() != '"')
                    sArgument = "\"" + sArgument + "\" -nq";

                string sDummy;
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sArgument, sDummy, true);
            }
        }
        else
            sArgument = "std";
    }
    else
    {
        sArgument = "std";
        sExpr = "1,100";
    }

    while (sCmd.length() && (sCmd.back() == ' ' || sCmd.back() == '-'))
        sCmd.pop_back();

    if (!sCmd.length())
        return COMMAND_PROCESSED;

    _parser.SetExpr(sCmd.substr(findCommand(sCmd).nPos + 8) + "," + sExpr);
    vVals = _parser.Eval(nArgument);
    make_progressBar((int)vVals[0], (int)vVals[1], (int)vVals[2], sArgument);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "print" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_print(string& sCmd)
{
    string sArgument = sCmd.substr(findCommand(sCmd).nPos + 6) + " -print";
    sCmd.replace(findCommand(sCmd).nPos, string::npos, sArgument);

    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function returns a map of
/// commands linked to their function
/// implementation.
///
/// \return map<string,CommandFunc>
///
/////////////////////////////////////////////////
static map<string,CommandFunc> getCommandFunctions()
{
    map<string, CommandFunc> mCommandFuncMap;

    mCommandFuncMap["about"] = cmd_credits;
    mCommandFuncMap["audio"] = cmd_audio;
    mCommandFuncMap["clear"] = cmd_clear;
    mCommandFuncMap["cont"] = cmd_plotting;
    mCommandFuncMap["cont3d"] = cmd_plotting;
    mCommandFuncMap["contour"] = cmd_plotting;
    mCommandFuncMap["contour3d"] = cmd_plotting;
    mCommandFuncMap["copy"] = cmd_copy;
    mCommandFuncMap["credits"] = cmd_credits;
    mCommandFuncMap["datagrid"] = cmd_datagrid;
    mCommandFuncMap["del"] = cmd_delete;
    mCommandFuncMap["delete"] = cmd_delete;
    mCommandFuncMap["dens"] = cmd_plotting;
    mCommandFuncMap["dens3d"] = cmd_plotting;
    mCommandFuncMap["density"] = cmd_plotting;
    mCommandFuncMap["density3d"] = cmd_plotting;
    mCommandFuncMap["draw"] = cmd_plotting;
    mCommandFuncMap["draw3d"] = cmd_plotting;
    mCommandFuncMap["define"] = cmd_define;
    mCommandFuncMap["edit"] = cmd_edit;
    mCommandFuncMap["execute"] = cmd_execute;
    mCommandFuncMap["export"] = saveDataObject;
    mCommandFuncMap["firststart"] = cmd_firststart;
    mCommandFuncMap["fit"] = cmd_fit;
    mCommandFuncMap["fitw"] = cmd_fit;
    mCommandFuncMap["fft"] = cmd_fft;
    mCommandFuncMap["fwt"] = cmd_fwt;
    mCommandFuncMap["grad"] = cmd_plotting;
    mCommandFuncMap["grad3d"] = cmd_plotting;
    mCommandFuncMap["gradient"] = cmd_plotting;
    mCommandFuncMap["gradient3d"] = cmd_plotting;
    mCommandFuncMap["graph"] = cmd_plotting;
    mCommandFuncMap["graph3d"] = cmd_plotting;
    mCommandFuncMap["hist"] = cmd_hist;
    mCommandFuncMap["hline"] = cmd_hline;
    mCommandFuncMap["ifndef"] = cmd_ifndefined;
    mCommandFuncMap["ifndefined"] = cmd_ifndefined;
    mCommandFuncMap["implot"] = cmd_plotting;
    mCommandFuncMap["info"] = cmd_credits;
    mCommandFuncMap["install"] = cmd_install;
    mCommandFuncMap["list"] = cmd_list;
    mCommandFuncMap["matop"] = cmd_matop;
    mCommandFuncMap["mesh"] = cmd_plotting;
    mCommandFuncMap["mesh3d"] = cmd_plotting;
    mCommandFuncMap["meshgrid"] = cmd_plotting;
    mCommandFuncMap["meshgrid3d"] = cmd_plotting;
    mCommandFuncMap["move"] = cmd_move;
    mCommandFuncMap["mtrxop"] = cmd_matop;
    mCommandFuncMap["new"] = cmd_new;
    mCommandFuncMap["odesolve"] = cmd_odesolve;
    mCommandFuncMap["open"] = cmd_edit;
    mCommandFuncMap["paste"] = cmd_paste;
    mCommandFuncMap["plot"] = cmd_plotting;
    mCommandFuncMap["plot3d"] = cmd_plotting;
    mCommandFuncMap["plotcompose"] = cmd_plotting;
    mCommandFuncMap["print"] = cmd_print;
    mCommandFuncMap["progress"] = cmd_progress;
    mCommandFuncMap["quit"] = cmd_quit;
    mCommandFuncMap["random"] = cmd_random;
    mCommandFuncMap["redef"] = cmd_redefine;
    mCommandFuncMap["redefine"] = cmd_redefine;
    mCommandFuncMap["regularize"] = cmd_regularize;
    mCommandFuncMap["reload"] = cmd_reload;
    mCommandFuncMap["remove"] = cmd_remove;
    mCommandFuncMap["rename"] = cmd_rename;
    mCommandFuncMap["resample"] = cmd_resample;
    mCommandFuncMap["retoque"] = cmd_retouch;
    mCommandFuncMap["retouch"] = cmd_retouch;
    mCommandFuncMap["save"] = cmd_save;
    mCommandFuncMap["script"] = cmd_script;
    mCommandFuncMap["set"] = cmd_set;
    mCommandFuncMap["show"] = cmd_show;
    mCommandFuncMap["showf"] = cmd_show;
    mCommandFuncMap["smooth"] = cmd_smooth;
    mCommandFuncMap["spline"] = cmd_spline;
    mCommandFuncMap["start"] = cmd_start;
    mCommandFuncMap["stats"] = cmd_stats;
    mCommandFuncMap["stfa"] = cmd_stfa;
    mCommandFuncMap["string"] = cmd_string;
    mCommandFuncMap["surf"] = cmd_plotting;
    mCommandFuncMap["surf3d"] = cmd_plotting;
    mCommandFuncMap["surface"] = cmd_plotting;
    mCommandFuncMap["surface3d"] = cmd_plotting;
    mCommandFuncMap["swap"] = cmd_swap;
    mCommandFuncMap["taylor"] = cmd_taylor;
    mCommandFuncMap["undef"] = cmd_undefine;
    mCommandFuncMap["undefine"] = cmd_undefine;
    mCommandFuncMap["vect"] = cmd_plotting;
    mCommandFuncMap["vect3d"] = cmd_plotting;
    mCommandFuncMap["vector"] = cmd_plotting;
    mCommandFuncMap["vector3d"] = cmd_plotting;
    mCommandFuncMap["view"] = cmd_edit;
    mCommandFuncMap["warn"] = cmd_warn;
    mCommandFuncMap["workpath"] = cmd_workpath;
    mCommandFuncMap["write"] = cmd_write;

    return mCommandFuncMap;
}


/////////////////////////////////////////////////
/// \brief This static function returns a map of
/// commands with return values linked to their
/// function implementation.
///
/// \return map<string,CommandFunc>
///
/////////////////////////////////////////////////
static map<string,CommandFunc> getCommandFunctionsWithReturnValues()
{
    map<string, CommandFunc> mCommandFuncMap;

    mCommandFuncMap["append"] = cmd_append;
    mCommandFuncMap["dialog"] = cmd_dialog;
    mCommandFuncMap["diff"] = cmd_diff;
    mCommandFuncMap["eval"] = cmd_eval;
    mCommandFuncMap["extrema"] = cmd_extrema;
    mCommandFuncMap["get"] = cmd_get;
    mCommandFuncMap["imread"] = cmd_imread;
    mCommandFuncMap["integrate"] = cmd_integrate;
    mCommandFuncMap["integrate2d"] = cmd_integrate;
    mCommandFuncMap["load"] = cmd_load;
    mCommandFuncMap["pulse"] = cmd_pulse;
    mCommandFuncMap["read"] = cmd_read;
    mCommandFuncMap["readline"] = cmd_readline;
    mCommandFuncMap["sort"] = cmd_sort;
    mCommandFuncMap["zeroes"] = cmd_zeroes;

    return mCommandFuncMap;
}










#endif // COMMANDFUNCTIONS_HPP

