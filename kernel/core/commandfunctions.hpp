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
static string getVarList(const string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
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
	if (matchParams(sCmd, "asstr"))
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
				if (matchParams(sCmd, "asstr"))
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

			if (matchParams(sCmd, "asstr"))
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
	if (matchParams(sCmd, "asstr") && sReturn.length() > 2)
		sReturn.erase(sReturn.length() - 3);
	else if (!matchParams(sCmd, "asstr") && sReturn.length() > 1)
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
static bool undefineFunctions(string sFunctionList, Define& _functions, const Settings& _option)
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
static bool newObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	int nType = 0;
	string sObject = "";
	vector<string> vTokens;
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());

	if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
		NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    // Evaluate and prepare the passed parameters
	if (matchParams(sCmd, "dir", '='))
	{
		nType = 1;
		addArgumentQuotes(sCmd, "dir");
	}
	else if (matchParams(sCmd, "script", '='))
	{
		nType = 2;
		addArgumentQuotes(sCmd, "script");
	}
	else if (matchParams(sCmd, "proc", '='))
	{
		nType = 3;
		addArgumentQuotes(sCmd, "proc");
	}
	else if (matchParams(sCmd, "file", '='))
	{
		nType = 4;
		addArgumentQuotes(sCmd, "file");
	}
	else if (matchParams(sCmd, "plugin", '='))
	{
		nType = 5;
		addArgumentQuotes(sCmd, "plugin");
	}
	else if (matchParams(sCmd, "cache", '='))
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
			sObject = sCmd.substr(matchParams(sCmd, "cache", '=') + 5);

		StripSpaces(sObject);

		if (matchParams(sObject, "free"))
			eraseToken(sObject, "free", false);

		if (sObject.rfind('-') != string::npos)
			sObject.erase(sObject.rfind('-'));

		if (!sObject.length() || !getNextArgument(sObject, false).length())
			return false;

		while (sObject.length() && getNextArgument(sObject, false).length())
		{
			if (_data.isTable(getNextArgument(sObject, false)))
			{
				if (matchParams(sCmd, "free"))
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
			if (matchParams(sCmd, "free"))
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

			if (matchParams(sObject, "free"))
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
					if (matchParams(sCmd, "free"))
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
				if (matchParams(sCmd, "free"))
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
static bool editObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	int nType = 0;
	int nFileOpenFlag = 0;

	if (matchParams(sCmd, "norefresh"))
		nFileOpenFlag = 1;

	if (matchParams(sCmd, "refresh"))
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

	if (matchParams(sParams, "dir"))
		bOnlyDir = true;

	if (matchParams(sParams, "pattern", '=') || matchParams(sParams, "p", '='))
	{
		int nPos = 0;

		if (matchParams(sParams, "pattern", '='))
			nPos = matchParams(sParams, "pattern", '=') + 7;
		else
			nPos = matchParams(sParams, "p", '=') + 1;

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
	if (matchParams(__sCmd, "pattern", '=') || matchParams(__sCmd, "p", '='))
	{
		int nPos = 0;

		if (matchParams(__sCmd, "pattern", '='))
			nPos = matchParams(__sCmd, "pattern", '=') + 7;
		else
			nPos = matchParams(__sCmd, "p", '=') + 1;

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
	if (matchParams(__sCmd, "files", '='))
	{
		int nPos = matchParams(__sCmd, "files", '=') + 5;
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
static bool executeCommand(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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
	if (matchParams(sCmd, "params", '='))
		sParams = "\"" + getArgAtPos(sCmd, matchParams(sCmd, "params", '=') + 6) + "\"";

    // Extract target working path
	if (matchParams(sCmd, "wp", '='))
		sWorkpath = "\"" + getArgAtPos(sCmd, matchParams(sCmd, "wp", '=') + 2) + "\"";

    // Extract, whether we shall wait for the process to terminate
	if (matchParams(sCmd, "wait"))
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
/// \brief This static function loads a single
/// file directly to a cache and returns the name
/// of the target cache.
///
/// \param sFileName const string&
/// \param _data Datafile&
/// \param _option Settings&
/// \return string
///
/// The cache name is either extracted from the
/// file header or constructed from the file name.
/////////////////////////////////////////////////
static string loadToCache(const string& sFileName, Datafile& _data, Settings& _option)
{
    Datafile _cache;
    _cache.setTokens(_option.getTokenPaths());
    _cache.setPath(_option.getLoadPath(), false, _option.getExePath());
    NumeRe::FileHeaderInfo info = _cache.openFile(sFileName, _option, false, true);

    if (info.sTableName == "data")
        info.sTableName = "loaded_data";

    if (!_data.isTable(info.sTableName + "()"))
        _data.addTable(info.sTableName + "()", _option);

    long long int nFirstColumn = _data.getCols(info.sTableName, false);

    for (long long int i = 0; i < _cache.getLines("data", false); i++)
    {
        for (long long int j = 0; j < _cache.getCols("data", false); j++)
        {
            if (!i)
                _data.setHeadLineElement(j + nFirstColumn, info.sTableName, _cache.getHeadLineElement(j, "data"));

            if (_cache.isValidEntry(i, j, "data"))
                _data.writeToTable(i, j + nFirstColumn, info.sTableName, _cache.getElement(i, j, "data"));
        }
    }

    return info.sTableName;
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
static void autoSave(Datafile& _data, Output& _out, Settings& _option)
{
    // Only do something, if there's unsaved and valid data
	if (_data.isValidCache() && !_data.getSaveStatus())
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

    if (matchParams(sCmd, sPathParameter, '='))
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
static void copyDataToTemporaryTable(const string& sCmd, DataAccessParser& _accessParser, Datafile& _data, Datafile& _cache)
{
    // Validize the obtained index sets
    if (!isValidIndexSet(_accessParser.getIndices()))
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

    // Copy the target data to a new table
    if (_accessParser.getIndices().row.isOpenEnd())
        _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

    if (_accessParser.getIndices().col.isOpenEnd())
        _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

    _cache.setCacheSize(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size(), "cache");

    for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
    {
        for (size_t j = 0; j < _accessParser.getIndices().col.size(); j++)
        {
            if (!i)
                _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_accessParser.getIndices().col[j], _accessParser.getDataObject()));

            if (_data.isValidEntry(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()))
                _cache.writeToTable(i, j, "cache", _data.getElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()));
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
static CommandReturnValues swapTables(string& sCmd, Datafile& _data, Settings& _option)
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
        sArgument = getArgAtPos(sCmd, matchParams(sCmd, _data.matchTableAsParameter(sCmd, '='), '=') + _data.matchTableAsParameter(sCmd, '=').length());

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

	string sArgument;

    size_t nPrecision = _option.getPrecision();

    // Update the precision, if the user selected any
    if (matchParams(sCmd, "precision", '='))
    {
        _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "precision", '=')));
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
        Datafile _cache;

        // Update the necessary parameters
        _cache.setTokens(_option.getTokenPaths());
        _cache.setPath(_data.getPath(), false, _option.getExePath());
        _cache.setCacheStatus(true);

        copyDataToTemporaryTable(sCmd, _access, _data, _cache);

        // Update the name of the  cache table (force it)
        if (_access.getDataObject() != "cache")
            _cache.renameTable("cache", (_access.getDataObject() == "data" ? "copy_of_data" : _access.getDataObject()), true);

        // If the command line contains string variables
        // get those values here
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "file", '='))
            addArgumentQuotes(sCmd, "file");

        // Try to extract the file name, if it was passed
        if (containsStrings(sCmd) && extractFirstParameterStringValue(sCmd.substr(matchParams(sCmd, "file", '=')), sArgument))
        {
            if (_cache.saveFile(_access.getDataObject() == "data" ? "copy_of_data" : _access.getDataObject(), sArgument, nPrecision))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()), _option) );

                return COMMAND_PROCESSED;
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
        }
        else
            _cache.setPrefix(_access.getDataObject() == "data" ? "copy_of_data" : _access.getDataObject());

        // Auto-generate a file name during saving
        if (_cache.saveFile(_access.getDataObject() == "data" ? "copy_of_data" : _access.getDataObject(), "", nPrecision))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
        || (matchParams(sCmd, "x", '=') && matchParams(sCmd, "y", '=')))
    {
        vIntegrate = parser_Integrate_2(sCmd, _data, _parser, _option, _functions);
        sCmd = sArgument;
        sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~integrate2[~_~]");
        _parser.SetVectorVar("_~integrate2[~_~]", vIntegrate);
        return COMMAND_HAS_RETURNVALUE;
    }
    else
    {
        vIntegrate = parser_Integrate(sCmd, _data, _parser, _option, _functions);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
        vDiff = parser_Diff(sCmd, _parser, _data, _option, _functions);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
        if (parser_findExtrema(sCmd, _data, _parser, _option, _functions))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!parser_pulseAnalysis(sCmd, _parser, _data, _functions, _option))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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

    if (parser_evalPoints(sCmd, _data, _parser, _option, _functions))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
        if (parser_findZeroes(sCmd, _data, _parser, _option, _functions))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
    if (matchParams(sDialogSettings, "msg", '='))
        sMessage = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "msg", '=')+3);

    // Extract the window title
    if (matchParams(sDialogSettings, "title", '='))
        sTitle = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "title", '=')+5);

    // Extract the selected dialog type if available, otherwise
    // use the message box as default value
    if (matchParams(sDialogSettings, "type", '='))
    {
        string sType = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "type", '=')+4);

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
    if (matchParams(sDialogSettings, "buttons", '='))
    {
        string sButtons = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "buttons", '=')+7);

        if (sButtons == "ok")
            nControls |= NumeRe::CTRL_OKBUTTON;
        else if (sButtons == "okcancel")
            nControls |= NumeRe::CTRL_OKBUTTON | NumeRe::CTRL_CANCELBUTTON;
        else if (sButtons == "yesno")
            nControls |= NumeRe::CTRL_YESNOBUTTON;
    }

    // Extract the icon information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (matchParams(sDialogSettings, "icon", '='))
    {
        string sIcon = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "icon", '=')+4);

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
        sExpression = kernel->getData().ValidFolderName(removeQuotationMarks(sExpression));
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
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
                parser_Plot(sCmd, _data, _parser, _option, _functions, _pData);
        }
        else
            parser_Plot(sCmd, _data, _parser, _option, _functions, _pData);

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (_data.isValid() || _data.isValidCache())
        parser_fit(sCmd, _parser, _data, _functions, _option);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    parser_fft(sCmd, _parser, _data, _option);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    parser_wavelet(sCmd, _parser, _data, _option);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    size_t nPos = findCommand(sCmd, "get").nPos;
    string sCommand = extractCommandString(sCmd, findCommand(sCmd, "get"));

    if (matchParams(sCmd, "savepath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "loadpath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "workpath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "viewer"))
    {
        if (_option.getViewerPath().length())
        {
            if (matchParams(sCmd, "asstr"))
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
            if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "editor"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "scriptpath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "procpath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "plotfont"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "precision"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getPrecision());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getPrecision()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "faststart"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbFastStart());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbFastStart()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "compact"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbCompact());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbCompact()));

            return COMMAND_HAS_RETURNVALUE;
        }
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "autosave"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getAutoSaveInterval());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getAutoSaveInterval()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "plotparams"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "varlist"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "stringlist"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "numlist"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "plotpath"))
    {
        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "greeting"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbGreeting());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbGreeting()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "hints"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbShowHints());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowHints()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "useescinscripts"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseESCinScripts());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseESCinScripts()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "usecustomlang"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getUseCustomLanguageFiles());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getUseCustomLanguageFiles()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "externaldocwindow"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getUseExternalViewer());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getUseExternalViewer()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "draftmode"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseDraftMode());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseDraftMode()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "extendedfileinfo"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbShowExtendedFileInfo());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowExtendedFileInfo()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "loademptycols"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbLoadEmptyCols());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbLoadEmptyCols()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "logfile"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbUseLogFile());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseLogFile()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "defcontrol"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString(_option.getbDefineAutoLoad());
            else
                sCmd.replace(nPos, sCommand.length(), toString(_option.getbDefineAutoLoad()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "buffersize"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString((int)_option.getBuffer(1));
            else
                sCmd.replace(nPos, sCommand.length(), toString((int)_option.getBuffer(1)));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "windowsize"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = "x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1);
            else
                sCmd.replace(nPos, sCommand.length(), "{" + toString((int)_option.getWindow() + 1) + ", " + toString((int)_option.getWindow(1) + 1) + "}");

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    else if (matchParams(sCmd, "colortheme"))
    {
        if (matchParams(sCmd, "asval"))
        {
            if (!nPos)
                sCmd = toString((int)_option.getColorTheme());
            else
                sCmd.replace(nPos, sCommand.length(), toString((int)_option.getColorTheme()));

            return COMMAND_HAS_RETURNVALUE;
        }

        if (matchParams(sCmd, "asstr"))
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
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
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

    if (matchParams(sCmd, "msg", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd, nPos);

        sCmd = sCmd.replace(nPos, sCommand.length(), evaluateParameterValues(sCommand));
        sCommand = evaluateParameterValues(sCommand);
    }

    if (matchParams(sCmd, "dflt", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd, nPos);

        sCmd = sCmd.replace(nPos, sCommand.length(), evaluateParameterValues(sCommand));
        sCommand = evaluateParameterValues(sCommand);
        sDefault = getArgAtPos(sCmd, matchParams(sCmd, "dflt", '=') + 4);
    }

    while (!sArgument.length())
    {
        string sLastLine = "";
        NumeReKernel::printPreFmt("|-> ");

        if (matchParams(sCmd, "msg", '='))
        {
            sLastLine = LineBreak(getArgAtPos(sCmd, matchParams(sCmd, "msg", '=') + 3), _option, false, 4);
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

    if (matchParams(sCmd, "asstr") && sArgument[0] != '"' && sArgument[sArgument.length() - 1] != '"')
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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

    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;
    int nArgument;
    size_t nPos;
    string sCommand = findCommand(sCmd).sString;

    if (matchParams(sCmd, "clear"))
    {
        if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            remove_data(_data, _option, true);
        else
            remove_data(_data, _option);

        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "load") || matchParams(sCmd, "load", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "load", '='))
            addArgumentQuotes(sCmd, "load");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (matchParams(sCmd, "slices", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slices", '=') + 6) == "xz")
                nArgument = -1;
            else if (matchParams(sCmd, "slices", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slices", '=') + 6) == "yz")
                nArgument = -2;
            else
                nArgument = 0;
            if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            {
                if (_data.isValid())
                {
                    if (_option.getSystemPrintStatus())
                        _data.removeData(false);
                    else
                        _data.removeData(true);
                }
                if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
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
                    _data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
                    Datafile _cache;
                    _cache.setTokens(_option.getTokenPaths());
                    _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                    for (unsigned int i = 1; i < vFilelist.size(); i++)
                    {
                        _cache.removeData(false);
                        _cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
                        _data.melt(_cache);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der " + toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                {
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
            }
            else if (!_data.isValid())
            {
                if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
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
                    _data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
                    Datafile _cache;
                    _cache.setTokens(_option.getTokenPaths());
                    _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                    for (unsigned int i = 1; i < vFilelist.size(); i++)
                    {
                        _cache.removeData(false);
                        _cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
                        _data.melt(_cache);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der " +toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option, false, false, nArgument);
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
    else if (matchParams(sCmd, "paste") || matchParams(sCmd, "pasteload"))
    {
        _data.pasteLoad(_option);
        if (_data.isValid())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_PASTE_SUCCESS", toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
        //NumeReKernel::print(LineBreak("|-> Die Daten wurden erfolgreich eingefügt: Der Datensatz besteht nun aus "+toString(_data.getLines("data"))+" Zeile(n) und "+toString(_data.getCols("data"))+" Spalte(n).", _option) );
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "reload") || matchParams(sCmd, "reload", '='))
    {
        if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !matchParams(sCmd, "reload", '='))
            //throw CANNOT_RELOAD_DATA;
            throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, "", SyntaxError::invalid_position);
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "reload", '='))
            addArgumentQuotes(sCmd, "reload");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (_data.isValid())
            {
                _data.removeData(false);
                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option);
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich aktualisiert.", _option) );
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else if (_data.isValid())
        {
            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            sArgument = _data.getDataFileName("data");
            _data.removeData(false);
            _data.openFile(sArgument, _option, false, true);
            if (_data.isValid() && _option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_SUCCESS"), _option) );
            //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich aktualisiert.", _option) );
        }
        else
            load_data(_data, _option, _parser);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "app") || matchParams(sCmd, "app", '='))
    {
        append_data(sCmd, _data, _option);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "showf"))
    {
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "show"))
    {
        _out.setCompact(_option.getbCompact());
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
        return COMMAND_PROCESSED;
    }
    else if (sCmd.substr(0, 5) == "data(")
    {
        return NO_COMMAND;
    }
    else if (matchParams(sCmd, "stats"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValid())
            plugin_statistics(sArgument, _data, _out, _option, false, true);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "hist"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValid())
            plugin_histogram(sArgument, _data, _data, _out, _option, _pData, false, true);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "save") || matchParams(sCmd, "save", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "save", '='))
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
    else if (matchParams(sCmd, "sort", '=') || matchParams(sCmd, "sort"))
    {
        _data.sortElements(sCmd);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "export") || matchParams(sCmd, "export", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "export", '='))
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
    else if ((matchParams(sCmd, "avg")
              || matchParams(sCmd, "sum")
              || matchParams(sCmd, "min")
              || matchParams(sCmd, "max")
              || matchParams(sCmd, "norm")
              || matchParams(sCmd, "std")
              || matchParams(sCmd, "prd")
              || matchParams(sCmd, "num")
              || matchParams(sCmd, "cnt")
              || matchParams(sCmd, "and")
              || matchParams(sCmd, "or")
              || matchParams(sCmd, "xor")
              || matchParams(sCmd, "med"))
             && (matchParams(sCmd, "lines") || matchParams(sCmd, "cols")))
    {
        if (!_data.isValid())
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        string sEvery = "";
        if (matchParams(sCmd, "every", '='))
        {
            value_type* v = 0;
            _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "every", '=') + 5));
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
        if (matchParams(sCmd, "grid"))
            sArgument = "grid";
        else
            sArgument.clear();
        if (matchParams(sCmd, "avg"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "sum"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "min"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "max"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "norm"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "std"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "prd"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "num"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "cnt"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "med"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "and"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "or"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "xor"))
        {
            if (matchParams(sCmd, "lines"))
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
    else if ((matchParams(sCmd, "avg")
              || matchParams(sCmd, "sum")
              || matchParams(sCmd, "min")
              || matchParams(sCmd, "max")
              || matchParams(sCmd, "norm")
              || matchParams(sCmd, "std")
              || matchParams(sCmd, "prd")
              || matchParams(sCmd, "num")
              || matchParams(sCmd, "cnt")
              || matchParams(sCmd, "and")
              || matchParams(sCmd, "or")
              || matchParams(sCmd, "xor")
              || matchParams(sCmd, "med"))
            )
    {
        if (!_data.isValid())
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        nPos = findCommand(sCmd, "data").nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, "data"));
        sCommand = sArgument;
        if (matchParams(sCmd, "grid") && _data.getCols("data") < 3)
            //throw TOO_FEW_COLS;
            throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, "data", "data");
        else if (matchParams(sCmd, "grid"))
            nArgument = 2;
        else
            nArgument = 0;
        if (matchParams(sCmd, "avg"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "sum"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "min"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "max"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "norm"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "std"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "prd"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "num"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "cnt"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "med"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "and"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "or"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
        else if (matchParams(sCmd, "xor"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (matchParams(sCmd, "dir", '=')
            || matchParams(sCmd, "script", '=')
            || matchParams(sCmd, "proc", '=')
            || matchParams(sCmd, "file", '=')
            || matchParams(sCmd, "plugin", '=')
            || matchParams(sCmd, "cache", '=')
            || sCmd.find("()", findCommand(sCmd).nPos + 3) != string::npos
            || sCmd.find('$', findCommand(sCmd).nPos + 3) != string::npos)
    {
        _data.setUserdefinedFuncs(_functions.getDefinesName());

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (sCmd.length() > 7)
        parser_Taylor(sCmd, _parser, _option, _functions);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "as"))
        autoSave(_data, _out, _option);

    if (matchParams(sCmd, "i"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;
    int nArgument;
    size_t nPos;
    string sCommand = findCommand(sCmd).sString;

    if (matchParams(sCmd, "showf"))
    {
        show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "show"))
    {
        _out.setCompact(_option.getbCompact());
        show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "clear"))
    {
        if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            clear_cache(_data, _option, true);
        else
            clear_cache(_data, _option);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "hist"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (_data.isValidCache())
            plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "stats"))
    {
        sArgument = evaluateParameterValues(sCmd);
        if (matchParams(sCmd, "save", '='))
        {
            if (sCmd[sCmd.find("save=") + 5] == '"' || sCmd[sCmd.find("save=") + 5] == '#')
            {
                if (!extractFirstParameterStringValue(sCmd, sArgument))
                    sArgument = "";
            }
            else
                sArgument = sCmd.substr(sCmd.find("save=") + 5, sCmd.find(' ', sCmd.find("save=") + 5) - sCmd.find("save=") - 5);
        }

        if (_data.isValidCache())
            plugin_statistics(sArgument, _data, _out, _option, true, false);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "save") || matchParams(sCmd, "save", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "save", '='))
            addArgumentQuotes(sCmd, "save");
        _data.setPrefix(sCommand);
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            _data.setCacheStatus(true);
            if (_data.saveFile(sCommand, sArgument))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
            {
                _data.setCacheStatus(false);
                //throw CANNOT_SAVE_FILE;
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
                //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
            }
            _data.setCacheStatus(false);
        }
        else
        {
            _data.setCacheStatus(true);
            if (_data.saveFile(sCommand, ""))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
            }
            else
            {
                _data.setCacheStatus(false);
                //throw CANNOT_SAVE_FILE;
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
                //NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
            }
            _data.setCacheStatus(false);
        }
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "sort") || matchParams(sCmd, "sort", '='))
    {
        _data.sortElements(sCmd);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "export") || matchParams(sCmd, "export", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "export", '='))
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
    else if (matchParams(sCmd, "rename", '=')) //CACHE -rename=NEWNAME
    {
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
            sCmd = evaluateParameterValues(sCmd);

        sArgument = getArgAtPos(sCmd, matchParams(sCmd, "rename", '=') + 6);
        _data.renameTable(sCommand, sArgument);
        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sArgument), _option) );
        //NumeReKernel::print(LineBreak("|-> Der Cache wurde erfolgreich zu \""+sArgument+"\" umbenannt.", _option) );
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "swap", '=')) //CACHE -swap=NEWCACHE
    {
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
            sCmd = evaluateParameterValues(sCmd);

        sArgument = getArgAtPos(sCmd, matchParams(sCmd, "swap", '=') + 4);
        _data.swapTables(sCommand, sArgument);
        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", sCommand, sArgument), _option) );
        //NumeReKernel::print(LineBreak("|-> Der Inhalt von \""+sCommand+"\" wurde erfolgreich mit dem Inhalt von \""+sArgument+"\" getauscht.", _option) );
        return COMMAND_PROCESSED;
    }
    else if ((matchParams(sCmd, "avg")
              || matchParams(sCmd, "sum")
              || matchParams(sCmd, "min")
              || matchParams(sCmd, "max")
              || matchParams(sCmd, "norm")
              || matchParams(sCmd, "std")
              || matchParams(sCmd, "prd")
              || matchParams(sCmd, "num")
              || matchParams(sCmd, "cnt")
              || matchParams(sCmd, "and")
              || matchParams(sCmd, "or")
              || matchParams(sCmd, "xor")
              || matchParams(sCmd, "med"))
             && (matchParams(sCmd, "lines") || matchParams(sCmd, "cols")))
    {
        if (!_data.isValidCache() || !_data.getTableCols(sCacheCmd, false))
            //throw NO_CACHED_DATA;
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
        string sEvery = "";
        if (matchParams(sCmd, "every", '='))
        {
            value_type* v = 0;
            _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "every", '=') + 5));
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
        if (matchParams(sCmd, "grid"))
            sArgument = "grid";
        else
            sArgument.clear();

        if (matchParams(sCmd, "avg"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "sum"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "min"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "max"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "norm"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "std"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "prd"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "num"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "cnt"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "med"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "and"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "or"))
        {
            if (matchParams(sCmd, "lines"))
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
        else if (matchParams(sCmd, "xor"))
        {
            if (matchParams(sCmd, "lines"))
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
    else if ((matchParams(sCmd, "avg")
              || matchParams(sCmd, "sum")
              || matchParams(sCmd, "min")
              || matchParams(sCmd, "max")
              || matchParams(sCmd, "norm")
              || matchParams(sCmd, "std")
              || matchParams(sCmd, "prd")
              || matchParams(sCmd, "num")
              || matchParams(sCmd, "cnt")
              || matchParams(sCmd, "and")
              || matchParams(sCmd, "or")
              || matchParams(sCmd, "xor")
              || matchParams(sCmd, "med"))
            )
    {
        if (!_data.isValidCache() || !_data.getTableCols(sCacheCmd, false))
            //throw NO_CACHED_DATA;
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
        nPos = findCommand(sCmd, sCacheCmd).nPos;
        sArgument = extractCommandString(sCmd, findCommand(sCmd, sCacheCmd));
        sCommand = sArgument;
        if (matchParams(sCmd, "grid") && _data.getTableCols(sCacheCmd, false) < 3)
            //throw TOO_FEW_COLS;
            throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, sCacheCmd, sCacheCmd);
        else if (matchParams(sCmd, "grid"))
            nArgument = 2;
        else
            nArgument = 0;
        if (matchParams(sCmd, "avg"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "sum"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "min"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "max"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "norm"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "std"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "prd"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "num"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "cnt"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "med"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "and"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "or"))
            sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
        else if (matchParams(sCmd, "xor"))
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
/////////////////////////////////////////////////
static CommandReturnValues cmd_clear(string& sCmd)
{
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "data") || sCmd.find(" data()", findCommand(sCmd).nPos) != string::npos)
    {
        if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            remove_data(_data, _option, true);
        else
            remove_data(_data, _option);
    }
    else if (_data.matchTableAsParameter(sCmd).length() || _data.containsTablesOrClusters(sCmd.substr(findCommand(sCmd).nPos)))
    {
        if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            clear_cache(_data, _option, true);
        else
            clear_cache(_data, _option);
    }
    else if (matchParams(sCmd, "string") || sCmd.find(" string()", findCommand(sCmd).nPos) != string::npos)
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
    {
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
    }
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
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.find(' ') != string::npos)
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "comment", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (_data.containsTablesOrClusters(sCmd) || sCmd.find("data(", 5) != string::npos)
    {
        if (CopyData(sCmd, _parser, _data, _option))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYDATA_SUCCESS"));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, sCmd, SyntaxError::invalid_position);
    }
    else if ((matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '=')) && sCmd.length() > 5)
    {
        int nArgument;

        if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        sCmd.replace(sCmd.find("data"), 4, "app");
        append_data(sCmd, _data, _option);
    }
    else if (sCmd.length() > findCommand(sCmd).nPos + 7 && sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7) != string::npos)
    {
        NumeReKernel::printPreFmt("\r");

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7)] != '"' && sCmd.find("string(") == string::npos)
        {
            sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7), 1, '"');

            if (matchParams(sCmd, "slice")
                    || matchParams(sCmd, "keepdim")
                    || matchParams(sCmd, "complete")
                    || matchParams(sCmd, "ignore")
                    || matchParams(sCmd, "i")
                    || matchParams(sCmd, "head")
                    || matchParams(sCmd, "h")
                    || matchParams(sCmd, "all"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!parser_writeAudio(sCmd, _parser, _data, _functions, _option))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    readImage(sCmd, _parser, _data, _option);
    return COMMAND_PROCESSED;
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.length() > 6 && matchParams(sCmd, "file", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();

    string sArgument = evaluateParameterValues(sCmd);

    if (matchParams(sCmd, "data") && _data.isValid())
    {
        // DEPRECATED: Declared at v1.1.2rc2
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        plugin_statistics(sArgument, _data, _out, _option, false, true);
    }
    else if (_data.matchTableAsParameter(sCmd).length() && _data.isValidCache())
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
            Datafile _cache;

            copyDataToTemporaryTable(sCmd, _accessParser, _data, _cache);

            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (matchParams(sCmd, "export", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;

    if (!parser_stfa(sCmd, sArgument, _parser, _data, _functions, _option))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!parser_spline(sCmd, _parser, _data, _functions, _option))
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
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "define"))
    {
        _functions.save(_option);
        return COMMAND_PROCESSED;
    }
    else if (matchParams(sCmd, "set") || matchParams(sCmd, "settings"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Script& _script = NumeReKernel::getInstance()->getScript();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    int nArgument;
    string sArgument;

    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
        NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    if (matchParams(sCmd, "savepath") || matchParams(sCmd, "savepath", '='))
    {
        sArgument = getPathForSetting(sCmd, "savepath");
        _out.setPath(sArgument, true, _out.getProgramPath());
        _option.setSavePath(_out.getPath());
        _data.setSavePath(_option.getSavePath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (matchParams(sCmd, "loadpath") || matchParams(sCmd, "loadpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "loadpath");
        _data.setPath(sArgument, true, _data.getProgramPath());
        _option.setLoadPath(_data.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (matchParams(sCmd, "workpath") || matchParams(sCmd, "workpath", '='))
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
    else if (matchParams(sCmd, "viewer") || matchParams(sCmd, "viewer", '='))
    {
        sArgument = getPathForSetting(sCmd, "viewer");
        _option.setViewerPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Imageviewer")) );
    }
    else if (matchParams(sCmd, "editor") || matchParams(sCmd, "editor", '='))
    {
        sArgument = getPathForSetting(sCmd, "editor");
        _option.setEditorPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Texteditor")) );
    }
    else if (matchParams(sCmd, "scriptpath") || matchParams(sCmd, "scriptpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "scriptpath");
        _script.setPath(sArgument, true, _script.getProgramPath());
        _option.setScriptPath(_script.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (matchParams(sCmd, "plotpath") || matchParams(sCmd, "plotpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "plotpath");
        _pData.setPath(sArgument, true, _pData.getProgramPath());
        _option.setPlotOutputPath(_pData.getPath());

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (matchParams(sCmd, "procpath") || matchParams(sCmd, "procpath", '='))
    {
        sArgument = getPathForSetting(sCmd, "procpath");
        _option.setProcPath(sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );

        NumeReKernel::modifiedSettings = true;
    }
    else if (matchParams(sCmd, "plotfont") || matchParams(sCmd, "plotfont", '='))
    {
        if (matchParams(sCmd, "plotfont", '='))
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
    else if (matchParams(sCmd, "precision") || matchParams(sCmd, "precision", '='))
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
    else if (matchParams(sCmd, "draftmode") || matchParams(sCmd, "draftmode", '='))
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
    else if (matchParams(sCmd, "extendedfileinfo") || matchParams(sCmd, "extendedfileinfo", '='))
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
    else if (matchParams(sCmd, "loademptycols") || matchParams(sCmd, "loademptycols", '='))
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
    else if (matchParams(sCmd, "logfile") || matchParams(sCmd, "logfile", '='))
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
    else if (matchParams(sCmd, "mode") || matchParams(sCmd, "mode", '='))
    {
        if (matchParams(sCmd, "mode", '='))
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
    else if (matchParams(sCmd, "compact") || matchParams(sCmd, "compact", '='))
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
    else if (matchParams(sCmd, "greeting") || matchParams(sCmd, "greeting", '='))
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
    else if (matchParams(sCmd, "hints") || matchParams(sCmd, "hints", '='))
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
    else if (matchParams(sCmd, "useescinscripts") || matchParams(sCmd, "useescinscripts", '='))
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
    else if (matchParams(sCmd, "usecustomlang") || matchParams(sCmd, "usecustomlang", '='))
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
    else if (matchParams(sCmd, "externaldocwindow") || matchParams(sCmd, "externaldocwindow", '='))
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
    else if (matchParams(sCmd, "defcontrol") || matchParams(sCmd, "defcontrol", '='))
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
    else if (matchParams(sCmd, "autosave") || matchParams(sCmd, "autosave", '='))
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
    else if (matchParams(sCmd, "buffersize") || matchParams(sCmd, "buffersize", '='))
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
    else if (matchParams(sCmd, "windowsize"))
    {
        if (matchParams(sCmd, "x", '='))
        {
            parseCmdArg(sCmd, "x", _parser, nArgument);
            _option.setWindowSize((unsigned)nArgument, 0);
            _option.setWindowBufferSize(_option.getWindow() + 1, 0);
            NumeReKernel::nLINE_LENGTH = _option.getWindow();
        }

        if (matchParams(sCmd, "y", '='))
        {
            parseCmdArg(sCmd, "y", _parser, nArgument);
            _option.setWindowSize(0, (unsigned)nArgument);

            if (_option.getWindow(1) + 1 > _option.getBuffer(1))
                _option.setWindowBufferSize(0, _option.getWindow(1) + 1);
        }

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_WINDOWSIZE"))) );
    }
    else if (matchParams(sCmd, "save"))
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

    if (matchParams(sCmd, "script") || matchParams(sCmd, "script", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "install"))
            _script.setInstallProcedures();

        if (matchParams(sCmd, "script", '='))
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

    if (matchParams(sCmd, "load") || matchParams(sCmd, "load", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

        if (!_script.isOpen())
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (matchParams(sCmd, "load", '='))
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
    else if (matchParams(sCmd, "start") || matchParams(sCmd, "start", '='))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

        if (_script.isOpen())
            throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, "script");

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "install"))
            _script.setInstallProcedures();

        if (matchParams(sCmd, "start", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Handle the compact mode (probably not needed any more)
    if (sCmd.substr(0, 5) == "showf")
        _out.setCompact(false);
    else
        _out.setCompact(_option.getbCompact());

    // Determine the correct data object
    if (matchParams(sCmd, "data") || sCmd.find(" data()") != string::npos)
    {
        // data as object, passed as parameter
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
    }
    else if (_data.matchTableAsParameter(sCmd).length())
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
                Datafile _cache;

                // Validize the obtained index sets
                if (!isValidIndexSet(_accessParser.getIndices()))
                    throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

                // Copy the target data to a new table
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

                _cache.setCacheSize(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size(), "cache");
                _cache.renameTable("cache", "*" + _accessParser.getDataObject(), true);

                for (unsigned int i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    for (unsigned int j = 0; j < _accessParser.getIndices().col.size(); j++)
                    {
                        if (!i)
                        {
                            _cache.setHeadLineElement(j, "*" + _accessParser.getDataObject(), _data.getHeadLineElement(_accessParser.getIndices().col[j], _accessParser.getDataObject()));
                        }

                        if (_data.isValidEntry(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    // smooth cache(i1:i2,j1:j2) -order=1
    if (matchParams(sCmd, "order", '='))
    {
        nArgument = matchParams(sCmd, "order", '=') + 5;
        if (_data.containsTablesOrClusters(sCmd.substr(nArgument)) || sCmd.substr(nArgument).find("data(") != string::npos)
        {
            sArgument = sCmd.substr(nArgument);
            getDataElements(sArgument, _parser, _data, _option);
            if (sArgument.find("{") != string::npos)
                parser_VectorToExpr(sArgument, _option);
            sCmd = sCmd.substr(0, nArgument) + sArgument;
        }
        _parser.SetExpr(getArgAtPos(sCmd, nArgument));
        nArgument = (int)_parser.Eval();
    }
    else
        nArgument = 1;

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

        if (matchParams(sCmd, "grid"))
        {
            if (_data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::GRID))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", "\"" + _access.getDataObject() + "\""));
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
        {
            if (_data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::ALL))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", "\"" + _access.getDataObject() + "\""));
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "lines"))
        {
            if (_data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::LINES))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", _lang.get("COMMON_LINES")));
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "cols"))
        {
            if (_data.smooth(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::COLS))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", _lang.get("COMMON_COLS")));
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "clear"))
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
    return swapTables(sCmd, NumeReKernel::getInstance()->getData(), NumeReKernel::getInstance()->getSettings());
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();

    string sArgument = evaluateParameterValues(sCmd);
    string sCommand = findCommand(sCmd).sString;

    if (matchParams(sCmd, "data") && _data.isValid())
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
            Datafile _cache;

            copyDataToTemporaryTable(sCmd, _accessParser, _data, _cache);

            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

            if (matchParams(sCmd, "export", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    int nArgument;

    if (sCmd.length() > 5)
    {
        if (_data.containsTablesOrClusters(sCmd) && (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '=')))
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
            if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
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
    if (matchParams(sCmd, "single"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    parser_matrixOperations(sCmd, _parser, _data, _functions, _option);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "o"))
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
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (sCmd.length() > findCommand(sCmd).sString.length() + 1)
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "comment", '='))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (!_data.containsTablesOrClusters(sCmd))
        return COMMAND_PROCESSED;

    DataAccessParser _access(sCmd);

    if (_access.getDataObject().length())
    {
        if (matchParams(sCmd, "samples", '='))
        {
            nArgument = matchParams(sCmd, "samples", '=') + 7;

            if (_data.containsTablesOrClusters(getArgAtPos(sCmd, nArgument)) || getArgAtPos(sCmd, nArgument).find("data(") != string::npos)
            {
                sArgument = getArgAtPos(sCmd, nArgument);
                getDataElements(sArgument, _parser, _data, _option);

                if (sArgument.find("{") != string::npos)
                    parser_VectorToExpr(sArgument, _option);

                sCmd.replace(nArgument, getArgAtPos(sCmd, nArgument).length(), sArgument);
            }

            _parser.SetExpr(getArgAtPos(sCmd, nArgument));
            nArgument = intCast(_parser.Eval());
        }
        else
            nArgument = _data.getTableLines(_access.getDataObject(), false);

        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject(), _access.getDataObject());

        if (_access.getIndices().row.isOpenEnd())
            _access.getIndices().row.setRange(0, _data.getLines(_access.getDataObject(), false)-1);

        if (_access.getIndices().col.isOpenEnd())
            _access.getIndices().col.setRange(0, _data.getCols(_access.getDataObject())-1);

        if (matchParams(sCmd, "grid"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::GRID))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::ALL))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "cols"))
        {
            if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, nArgument, MemoryManager::COLS))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_COLS")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "lines"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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
                if (sCmd.find(iter->first + "()") != string::npos && iter->first != "cache")
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
        if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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
        sArgument = getArgAtPos(sCmd, matchParams(sCmd, _data.matchTableAsParameter(sCmd, '='), '=') + _data.matchTableAsParameter(sCmd, '=').length());

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "data", '='))
            addArgumentQuotes(sCmd, "data");

        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            if (_data.isValid())
            {
                _data.removeData(false);

                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;

                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option);

                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else if (_data.isValid())
        {
            if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !matchParams(sCmd, "data", '='))
                throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, sCmd, SyntaxError::invalid_position);

            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            sArgument = _data.getDataFileName("data");
            _data.removeData(false);
            _data.openFile(sArgument, _option, false, true);

            if (_data.isValid() && _option.getSystemPrintStatus())
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
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

        if (matchParams(sCmd, "grid"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::GRID))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::ALL))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + _access.getDataObject() + "\""), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "lines"))
        {
            if (_data.retoque(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, MemoryManager::LINES))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_LINES")), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
        }
        else if (matchParams(sCmd, "cols"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!parser_regularize(sCmd, _parser, _data, _functions, _option))
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
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Datafile& _data = NumeReKernel::getInstance()->getData();

    if (sCmd.length() > 8)
    {
        _functions.setCacheList(_data.getTableNames());

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (matchParams(sCmd, "comment", '='))
            addArgumentQuotes(sCmd, "comment");

        if (matchParams(sCmd, "save"))
        {
            _functions.save(_option);
            return COMMAND_PROCESSED;
        }
        else if (matchParams(sCmd, "load"))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    if (_data.containsTablesOrClusters(sCmd))
    {
        if (matchParams(sCmd, "ignore") || matchParams(sCmd, "i"))
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
            NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_CONFIRM"));

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument = "grid";

    if (!parser_datagrid(sCmd, sArgument, _parser, _data, _functions, _option))
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

    string sArgument;

    if (matchParams(sCmd, "files") || (matchParams(sCmd, "files", '=')))
        listFiles(sCmd, _option);
    else if (matchParams(sCmd, "var"))
        parser_ListVar(_parser, _option, _data);
    else if (matchParams(sCmd, "const"))
        parser_ListConst(_parser, _option);
    else if ((matchParams(sCmd, "func") || matchParams(sCmd, "func", '=')))
    {
        if (matchParams(sCmd, "func", '='))
            sArgument = getArgAtPos(sCmd, matchParams(sCmd, "func", '=') + 4);
        else
            parser_ListFunc(_option, "all");

        if (sArgument == "num" || sArgument == "numerical")
            parser_ListFunc(_option, "num");
        else if (sArgument == "mat" || sArgument == "matrix" || sArgument == "vec" || sArgument == "vector")
            parser_ListFunc(_option, "mat");
        else if (sArgument == "string")
            parser_ListFunc(_option, "string");
        else if (sArgument == "trigonometric")
            parser_ListFunc(_option, "trigonometric");
        else if (sArgument == "hyperbolic")
            parser_ListFunc(_option, "hyperbolic");
        else if (sArgument == "logarithmic")
            parser_ListFunc(_option, "logarithmic");
        else if (sArgument == "polynomial")
            parser_ListFunc(_option, "polynomial");
        else if (sArgument == "stats" || sArgument == "statistical")
            parser_ListFunc(_option, "stats");
        else if (sArgument == "angular")
            parser_ListFunc(_option, "angular");
        else if (sArgument == "physics" || sArgument == "physical")
            parser_ListFunc(_option, "physics");
        else if (sArgument == "logic" || sArgument == "logical")
            parser_ListFunc(_option, "logic");
        else if (sArgument == "time")
            parser_ListFunc(_option, "time");
        else if (sArgument == "distrib")
            parser_ListFunc(_option, "distrib");
        else if (sArgument == "random")
            parser_ListFunc(_option, "random");
        else if (sArgument == "coords")
            parser_ListFunc(_option, "coords");
        else if (sArgument == "draw")
            parser_ListFunc(_option, "draw");
        else
            parser_ListFunc(_option, "all");

    }
    else if (matchParams(sCmd, "logic"))
        parser_ListLogical(_option);
    else if (matchParams(sCmd, "cmd"))
        parser_ListCmd(_option);
    else if (matchParams(sCmd, "define"))
        parser_ListDefine(_functions, _option);
    else if (matchParams(sCmd, "settings"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        listOptions(_option);
    }
    else if (matchParams(sCmd, "units"))
        parser_ListUnits(_option);
    else if (matchParams(sCmd, "plugins"))
        parser_ListPlugins(_parser, _data, _option);
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Script& _script = NumeReKernel::getInstance()->getScript();

    string sArgument;
    int nArgument;

    if (matchParams(sCmd, "define"))
    {
        if (fileExists("functions.def"))
            _functions.load(_option);
        else
            NumeReKernel::print( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY") );
    }
    else if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '=')) // deprecated
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
        if (matchParams(sCmd, "data", '='))
            addArgumentQuotes(sCmd, "data");
        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "xz")
                nArgument = -1;
            else if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "yz")
                nArgument = -2;
            else
                nArgument = 0;
            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);
            if (matchParams(sCmd, "tocache") && !matchParams(sCmd, "all"))
            {
                Datafile _cache;
                _cache.setTokens(_option.getTokenPaths());
                _cache.setPath(_option.getLoadPath(), false, _option.getExePath());
                _cache.openFile(sArgument, _option, false, true, nArgument);
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
                        if (_cache.isValidEntry(i, j, "data"))
                        {
                            _data.writeToTable(i, j + nArgument, sArgument, _cache.getElement(i, j, "data"));
                        }
                    }
                }
                if (_data.isValidCache() && _data.getCols(sArgument, false))
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines(sArgument, false)), toString(_data.getCols(sArgument, false))), _option) );
                return COMMAND_PROCESSED;
            }
            else if (matchParams(sCmd, "tocache") && matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
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
                Datafile _cache;
                _cache.setTokens(_option.getTokenPaths());
                _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                for (unsigned int i = 0; i < vFilelist.size(); i++)
                {
                    _cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
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
                            if (_cache.isValidEntry(i, j, "data"))
                            {
                                _data.writeToTable(i, j + nArgument, sTarget, _cache.getElement(i, j, "data"));
                            }
                        }
                    }
                    _cache.removeData(false);
                    nArgument = -1;
                }
                if (_data.isValidCache())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument), _option) );
                //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                return COMMAND_PROCESSED;
            }
            if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
            {
                if (_data.isValid())
                {
                    if (_option.getSystemPrintStatus())
                        _data.removeData(false);
                    else
                        _data.removeData(true);
                }
                if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
                    _data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
                    Datafile _cache;
                    _cache.setTokens(_option.getTokenPaths());
                    _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                    for (unsigned int i = 1; i < vFilelist.size(); i++)
                    {
                        _cache.removeData(false);
                        _cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
                        _data.melt(_cache);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }

                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option, false, true, nArgument);
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

            }
            else if (!_data.isValid())
            {
                if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;
                    vector<string> vFilelist = getFileList(sArgument, _option);
                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
                    string sPath = "<loadpath>/";
                    if (sArgument.find('/') != string::npos)
                        sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
                    _data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
                    Datafile _cache;
                    _cache.setTokens(_option.getTokenPaths());
                    _cache.setPath(_data.getPath(), false, _data.getProgramPath());
                    for (unsigned int i = 1; i < vFilelist.size(); i++)
                    {
                        _cache.removeData(false);
                        _cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
                        _data.melt(_cache);
                    }
                    if (_data.isValid())
                        NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                    //NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
                    return COMMAND_PROCESSED;
                }
                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;
                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option, false, false, nArgument);
                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

            }
            else
                load_data(_data, _option, _parser, sArgument);
        }
        else
            load_data(_data, _option, _parser);
    }
    else if (matchParams(sCmd, "script") || matchParams(sCmd, "script", '=')) // deprecated
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        if (!_script.isOpen())
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);
            if (matchParams(sCmd, "script", '='))
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
    else if (sCmd.length() > findCommand(sCmd).nPos + 5 && sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5) != string::npos)
    {
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        // Add quotation marks around the object, if there aren't any
        if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5)] != '"' && sCmd.find("string(") == string::npos)
        {
            if (matchParams(sCmd, "slice")
                    || matchParams(sCmd, "keepdim")
                    || matchParams(sCmd, "complete")
                    || matchParams(sCmd, "ignore")
                    || matchParams(sCmd, "tocache")
                    || matchParams(sCmd, "i")
                    || matchParams(sCmd, "head")
                    || matchParams(sCmd, "h")
                    || matchParams(sCmd, "app")
                    || matchParams(sCmd, "all"))
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

        if (matchParams(sCmd, "app"))
        {
            sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), "-app=");
            append_data(sCmd, _data, _option);
            return COMMAND_PROCESSED;
        }

        if (extractFirstParameterStringValue(sCmd, sArgument))
        {
            if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "xz")
                nArgument = -1;
            else if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "yz")
                nArgument = -2;
            else
                nArgument = 0;

            if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
                _data.setbLoadEmptyColsInNextFile(true);

            if (matchParams(sCmd, "tocache") && !matchParams(sCmd, "all"))
            {
                // Single file directly to cache
                sArgument = loadToCache(sArgument, _data, _option);

                if (_data.isValidCache() && _data.getCols(sArgument, false))
                    NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", sArgument + "()", toString(_data.getLines(sArgument, false)), toString(_data.getCols(sArgument, false))));

                return COMMAND_PROCESSED;
            }
            else if (matchParams(sCmd, "tocache") && matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
            {
                // multiple files directly to cache
                if (sArgument.find('/') == string::npos)
                    sArgument = "<loadpath>/" + sArgument;

                vector<string> vFilelist = getFileList(sArgument, _option, 1);

                if (!vFilelist.size())
                    throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);

                for (size_t i = 0; i < vFilelist.size(); i++)
                    loadToCache(vFilelist[i], _data, _option);

                if (_data.isValidCache())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument));

                return COMMAND_PROCESSED;
            }

            if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore") || !_data.isValid())
            {
                if (_data.isValid())
                    _data.removeData();

                // multiple files
                if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
                {
                    if (sArgument.find('/') == string::npos)
                        sArgument = "<loadpath>/" + sArgument;

                    vector<string> vFilelist = getFileList(sArgument, _option, 1);

                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);

                    _data.openFile(vFilelist[0], _option, false, true, nArgument);
                    Datafile _cache;
                    _cache.setTokens(_option.getTokenPaths());
                    _cache.setPath(_data.getPath(), false, _data.getProgramPath());

                    for (size_t i = 1; i < vFilelist.size(); i++)
                    {
                        _cache.removeData(false);
                        _cache.openFile(vFilelist[i], _option, false, true, nArgument);
                        _data.melt(_cache);
                    }

                    if (_data.isValid())
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", false)), toString(_data.getCols("data", false))));

                    return COMMAND_PROCESSED;
                }

                // Provide headline
                if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
                {
                    if (matchParams(sCmd, "head", '='))
                        nArgument = matchParams(sCmd, "head", '=') + 4;
                    else
                        nArgument = matchParams(sCmd, "h", '=') + 1;

                    nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                    _data.openFile(sArgument, _option, false, true, nArgument);
                }
                else
                    _data.openFile(sArgument, _option, false, true, nArgument);

                if (_data.isValid() && _option.getSystemPrintStatus())
                    NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", false)), toString(_data.getCols("data", false))));
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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();

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
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (matchParams(sCmd, "data"))
    {
        // DEPRECATED: Declared at v1.1.2rc1
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
        _data.pasteLoad(_option);
        if (_data.isValid())
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

        if (matchParams(sArgument, "first", '='))
            sExpr = getArgAtPos(sArgument, matchParams(sArgument, "first", '=') + 5) + ",";
        else
            sExpr = "1,";

        if (matchParams(sArgument, "last", '='))
            sExpr += getArgAtPos(sArgument, matchParams(sArgument, "last", '=') + 4);
        else
            sExpr += "100";

        if (matchParams(sArgument, "type", '='))
        {
            sArgument = getArgAtPos(sArgument, matchParams(sArgument, "type", '=') + 4);

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
    mCommandFuncMap["append"] = cmd_append;
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
    mCommandFuncMap["imread"] = cmd_imread;
    mCommandFuncMap["info"] = cmd_credits;
    mCommandFuncMap["install"] = cmd_install;
    mCommandFuncMap["list"] = cmd_list;
    mCommandFuncMap["load"] = cmd_load;
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

    mCommandFuncMap["dialog"] = cmd_dialog;
    mCommandFuncMap["diff"] = cmd_diff;
    mCommandFuncMap["eval"] = cmd_eval;
    mCommandFuncMap["extrema"] = cmd_extrema;
    mCommandFuncMap["get"] = cmd_get;
    mCommandFuncMap["integrate"] = cmd_integrate;
    mCommandFuncMap["integrate2d"] = cmd_integrate;
    mCommandFuncMap["pulse"] = cmd_pulse;
    mCommandFuncMap["read"] = cmd_read;
    mCommandFuncMap["readline"] = cmd_readline;
    mCommandFuncMap["sort"] = cmd_sort;
    mCommandFuncMap["zeroes"] = cmd_zeroes;

    return mCommandFuncMap;
}










#endif // COMMANDFUNCTIONS_HPP

