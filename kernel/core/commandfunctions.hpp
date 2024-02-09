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
#include "ui/winlayout.hpp"
#include "io/logger.hpp"
#include "utils/tools.hpp"
#include "io/archive.hpp"
#include "io/qrcode.hpp"

#include "commandlineparser.hpp"

using namespace std;
using namespace mu;

enum WindowType
{
    WT_ALL,
    WT_GRAPH,
    WT_TABLEVIEWER,
    WT_IMAGEVIEWER,
    WT_DOCVIEWER,
    WT_CUSTOM
};


typedef CommandReturnValues (*CommandFunc)(string&);

extern mglGraph _fontData;

string removeQuotationMarks(const string& sString);
static size_t findSettingOption(const std::string& sCmd, const std::string& sOption);

/////////////////////////////////////////////////
/// \brief Performs the operation confirmation
/// loop, if the user did not supply the ignore
/// command line option.
///
/// \param sMessage const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool confirmOperation(const std::string& sMessage)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    NumeReKernel::print(LineBreak(sMessage, _option));
    std::string sArgument;

    do
    {
        NumeReKernel::printPreFmt("|\n|<- ");
        NumeReKernel::getline(sArgument);
        StripSpaces(sArgument);
    }
    while (!sArgument.length());

    if (sArgument.substr(0, 1) != _lang.YES())
    {
        NumeReKernel::print(_lang.get("COMMON_CANCEL"));
        return false;
    }

    return true;
}


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
                sReturn += toString(*mNumVars[iter->first], _option.getPrecision());

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
            sReturn += toString(*iter->second, _option.getPrecision());
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
    if (_option.systemPrints() && sSuccessFulRemoved.length())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_UNDEF_SUCCESS", sSuccessFulRemoved.substr(0, sSuccessFulRemoved.length() - 2)));

    return true;
}


/////////////////////////////////////////////////
/// \brief Perpares a template based upon the
/// selected template id. The file has to exist.
///
/// \param sTemplateID const std::string&
/// \param sFileName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool prepareTemplate(const std::string& sTemplateID, const std::string& sFileName)
{
    std::vector<std::string> vTokens;
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    vTokens.push_back(sFileName.substr(sFileName.rfind('/') + 1, sFileName.rfind('.') - sFileName.rfind('/') - 1));
    vTokens.push_back(getTimeStamp(false));

    if (fileExists(_option.ValidFileName("<>/user/lang/" + sTemplateID + ".nlng", ".nlng")))
        return generateTemplate(sFileName, "<>/user/lang/" + sTemplateID + ".nlng", vTokens, _option);

    return generateTemplate(sFileName, "<>/lang/" + sTemplateID + ".nlng", vTokens, _option);
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
    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_DAT_PAR);
    std::string sFileName;
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    if (cmdParser.hasParam("dir"))
    {
        sFileName = replacePathSeparator(cmdParser.getParameterValueAsString("dir", "", true, true));
        int nReturn = _fSys.setPath(sFileName, true, _option.getExePath());

        if (nReturn == 1 && _option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_NEW_FOLDERCREATED", sFileName));

        return true;
    }
    else if (cmdParser.hasParam("script"))
    {
        sFileName = replacePathSeparator(cmdParser.getParameterValueAsString("script", "", true, true));

        if (!sFileName.length())
            return false;

        if (sFileName.find(':') == std::string::npos && sFileName.front() != '<')
            sFileName = "<scriptpath>/" + sFileName;

        sFileName = _fSys.ValidizeAndPrepareName(sFileName, ".nscr");

        if (!prepareTemplate("tmpl_script", sFileName))
            throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sFileName, sFileName);

        if (_option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_NEW_SCRIPTCREATED", sFileName));

        return true;
    }
    else if (cmdParser.hasParam("proc"))
    {
        sFileName = replacePathSeparator(cmdParser.getParameterValueAsString("proc", "", true, true));

        if (!sFileName.length())
            return false;

        replaceAll(sFileName, "~", "/");
        replaceAll(sFileName, "$", "");
        replaceAll(sFileName, "main/", "");

        if (sFileName.find(':') == std::string::npos && sFileName.front() != '<')
            sFileName = "<procpath>/" + sFileName;

        sFileName = _fSys.ValidizeAndPrepareName(sFileName, ".nprc");

        if (!prepareTemplate("tmpl_procedure", sFileName))
            throw SyntaxError(SyntaxError::CANNOT_GENERATE_PROCEDURE, sCmd, sFileName, sFileName);

        if (_option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_NEW_PROCCREATED", sFileName));

        return true;
    }
    else if (cmdParser.hasParam("file"))
    {
        sFileName = replacePathSeparator(cmdParser.getParameterValueAsString("file", "", true, true));

        if (!sFileName.length())
            return false;

        if (sFileName.find(':') == std::string::npos && sFileName.front() != '<')
            sFileName = "<>/" + sFileName;

        sFileName = _fSys.ValidizeAndPrepareName(sFileName, ".txt");

        if (sFileName.ends_with(".nprc")
                || sFileName.ends_with(".nscr")
                || sFileName.ends_with(".nlyt")
                || sFileName.ends_with(".ndat"))
            sFileName.replace(sFileName.rfind('.'), 5, ".txt");

        if (!prepareTemplate("tmpl_file", sFileName))
            throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, sCmd, sFileName, sFileName);

        if (_option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_NEW_FILECREATED", sFileName));

        return true;
    }
    else if (cmdParser.hasParam("plugin"))
    {
        sFileName = replacePathSeparator(cmdParser.getParameterValueAsString("plugin", "", true, true));

        if (!sFileName.length())
            return false;

        if (sFileName.find(':') == std::string::npos && sFileName.front() != '<')
            sFileName = "<scriptpath>/" + sFileName;

        sFileName = _fSys.ValidizeAndPrepareName(sFileName, ".nscr");

        if (!prepareTemplate("tmpl_plugin", sFileName))
            throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sFileName, sFileName);

        if (_option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_NEW_PLUGINCREATED", sFileName));

        return true;
    }
    else
    {
        if (cmdParser.getExpr().front() == '$')
        {
            sFileName = cmdParser.getExpr();

            if (!sFileName.length())
                return false;

            replaceAll(sFileName, "~", "/");
            replaceAll(sFileName, "$", "");
            replaceAll(sFileName, "main/", "");

            if (sFileName.find(':') == std::string::npos && sFileName.front() != '<')
                sFileName = "<procpath>/" + sFileName;

            sFileName = _fSys.ValidizeAndPrepareName(sFileName, ".nprc");

            if (!prepareTemplate("tmpl_procedure", sFileName))
                throw SyntaxError(SyntaxError::CANNOT_GENERATE_PROCEDURE, sCmd, sFileName, sFileName);

            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_NEW_PROCCREATED", sFileName));

            return true;
        }
        else if (cmdParser.getExpr().find("()") != std::string::npos)
        {
            // Create new tables
            std::string sReturnVal = "";
            std::string sObject = cmdParser.parseExprAsString();

            if (!sObject.length())
                return false;

            // Create the tables
            while (sObject.length())
            {
                std::string sTableName = getNextArgument(sObject, true);

                // Does the table already exist?
                if (_data.isTable(sTableName))
                {
                    if (cmdParser.hasParam("free"))
                    {
                        std::string sName = sTableName.substr(0, sTableName.find('('));
                        _data.deleteBulk(sName, 0, _data.getLines(sName) - 1, 0, _data.getCols(sName) - 1);

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

            if (sReturnVal.length() && _option.systemPrints())
            {
                if (cmdParser.hasParam("free"))
                    NumeReKernel::print(_lang.get("BUILTIN_NEW_FREE_CACHES", sReturnVal));
                else
                    NumeReKernel::print(_lang.get("BUILTIN_NEW_CACHES", sReturnVal));
            }

            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function opens the object in the
/// editor to edit its contents.
///
/// \param sCmd std::string&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool editObject(std::string& sCmd, MemoryManager& _data, Settings& _option)
{
    std::string sObject;
    int nType = 0;
    int nFileOpenFlag = 0;

    if (findParameter(sCmd, "norefresh"))
        nFileOpenFlag = 1;

    if (findParameter(sCmd, "refresh"))
        nFileOpenFlag = 2 | 4;

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

    // Open the table for editing
    if (_data.containsTables(sObject))
    {
        StripSpaces(sObject);
        std::string sTableName = sObject.substr(0, sObject.find('('));

        NumeReKernel::showTable(_data.extractTable(sTableName), sTableName, true);
        NumeReKernel::printPreFmt("|-> " + _lang.get("BUILTIN_WAITINGFOREDIT") + " ... ");

        NumeRe::Table _table = NumeReKernel::getTable();
        NumeReKernel::printPreFmt(_lang.get("COMMON_DONE") + ".\n");

        if (_table.isEmpty())
            return true;

        _data.importTable(_table, sTableName);
        return true;
    }

    std::string sDefaultExtension = ".dat";
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());

    if (sObject.find('.') != std::string::npos)
        _fSys.declareFileType(sObject.substr(sObject.rfind('.')));

    if (!sObject.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    if (sObject[0] == '$' && sObject[1] != '\'')
        sObject.insert(0, "<procpath>/");
    else if (sObject[0] == '$')
        sObject.erase(0, 1);

    replaceAll(sObject, "~", "/");
    replaceAll(sObject, "$", "");

    if (sObject.front() == '\'' && sObject.back() == '\'')
    {
        sObject.erase(0, 1);
        sObject.pop_back();
    }

    // Resolve the paths
    if (sObject.ends_with(".dat")
            || sObject.ends_with(".txt")
            || sObject.ends_with(".tex"))
    {
        _fSys.setPath(_option.getLoadPath(), false, _option.getExePath());

        std::string sTemporaryObjectName = _fSys.ValidFileName(sObject, ".dat");

        if (!fileExists(sTemporaryObjectName))
            _fSys.setPath(_option.getSavePath(), false, _option.getExePath());
    }
    else if (sObject.find("<loadpath>") != std::string::npos
             || sObject.find(_option.getLoadPath()) != std::string::npos)
        _fSys.setPath(_option.getLoadPath(), false, _option.getExePath());
    else if (sObject.find("<savepath>") != std::string::npos
             || sObject.find(_option.getSavePath()) != std::string::npos)
        _fSys.setPath(_option.getSavePath(), false, _option.getExePath());
    else if (sObject.ends_with(".nscr")
             || sObject.find("<scriptpath>") != std::string::npos
             || sObject.find(_option.getScriptPath()) != std::string::npos)
    {
        _fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
        sDefaultExtension = ".nscr";
    }
    else if (sObject.ends_with(".png")
             || sObject.ends_with(".gif")
             || sObject.ends_with(".svg")
             || sObject.ends_with(".eps")
             || sObject.find("<plotpath>") != std::string::npos
             || sObject.find(_option.getPlotPath()) != std::string::npos)
    {
        _fSys.setPath(_option.getPlotPath(), false, _option.getExePath());
        sDefaultExtension = ".png";
    }
    else if (sObject.ends_with(".nprc")
             || sObject.find("<procpath>") != std::string::npos
             || sObject.find(_option.getProcPath()) != std::string::npos)
    {
        _fSys.setPath(_option.getProcPath(), false, _option.getExePath());
        sDefaultExtension = ".nprc";
    }
    else if (sObject.find("<wp>") != std::string::npos
             || sObject.find(_option.getWorkPath()) != std::string::npos)
        _fSys.setPath(_option.getWorkPath(), false, _option.getExePath());
    else if (sObject.find("<>") != std::string::npos
             || sObject.find("<this>") != std::string::npos
             || sObject.find(_option.getExePath()) != std::string::npos)
        _fSys.setPath(_option.getExePath(), false, _option.getExePath());
    else
    {
        // Is probably a folder path to be edited in the Windows Explorer
        if (sObject.find('.') == std::string::npos
                && sObject.find_first_of("/\\") != std::string::npos)
        {
            ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL);
            return true;
        }

        // Append a wildcard at the end of the path if necessary
        if (sObject.back() != '*' && sObject.find('.') == std::string::npos)
            sObject += "*";

        // Try to determine the path based upon the file extension, where we might find the
        // file, if the user did not supply the path to the file
        if (sObject.find('.') != std::string::npos)
        {
            if (sObject.ends_with(".nhlp"))
                _fSys.setPath(_option.getExePath() + "/docs", false, _option.getExePath());
            else
                _fSys.setPath(_option.getExePath(), false, _option.getExePath());
        }
        else
            _fSys.setPath(_option.getExePath(), false, _option.getExePath());
    }

    sObject = _fSys.ValidFileName(sObject, sDefaultExtension);

    // Could be a folder -> open it in the Windows Explorer
    if (!fileExists(sObject) || sObject.find('.') == std::string::npos)
    {
        sObject.erase(sObject.rfind('.'));

        if (sObject.find('*') != std::string::npos)
            sObject.erase(sObject.rfind('*'));

        if ((long long int)ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32)
            return true;

        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, SyntaxError::invalid_position, sObject);
    }

    // Determine the file type of the file to be edited
    if (sObject.ends_with(".dat")
            || sObject.ends_with(".txt")
            || sObject.ends_with(".tex")
            || sObject.ends_with(".ini")
            || sObject.ends_with(".cfg")
            || sObject.ends_with(".conf")
            || sObject.ends_with(".xml")
            || sObject.ends_with(".yaml")
            || sObject.ends_with(".yml")
            || sObject.ends_with(".json")
            || sObject.ends_with(".csv")
            || sObject.ends_with(".labx")
            || sObject.ends_with(".jdx")
            || sObject.ends_with(".jcm")
            || sObject.ends_with(".dx")
            || sObject.ends_with(".nscr")
            || sObject.ends_with(".nprc")
            || sObject.ends_with(".nhlp")
            || sObject.ends_with(".nlyt")
            || sObject.ends_with(".png")
            || sObject.ends_with(".gif")
            || sObject.ends_with(".m")
            || sObject.ends_with(".cpp")
            || sObject.ends_with(".cxx")
            || sObject.ends_with(".c")
            || sObject.ends_with(".hpp")
            || sObject.ends_with(".hxx")
            || sObject.ends_with(".h")
            || sObject.ends_with(".log"))
        nType = 1;
    else if (sObject.ends_with(".svg")
             || sObject.ends_with(".eps"))
        nType = 2;

    if (!nType)
        throw SyntaxError(SyntaxError::CANNOT_EDIT_FILE_TYPE, sCmd, SyntaxError::invalid_position, sObject);

    if (nType == 1)
    {
        NumeReKernel::nOpenFileFlag = nFileOpenFlag;
        NumeReKernel::gotoLine(sObject);
    }
    else if (nType == 2)
        openExternally(sObject);

    return true;
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
    string sConnect;
    string sPattern = "*";
    string sFilesize = " Bytes";
    string sFileName;
    string sDirectory = "";
    int nLength = 0;
    int nCount[2] = {0, 0};
    size_t nFirstColLength = _option.getWindow() / 2 - 6;
    bool bOnlyDir = false;

    if (findSettingOption(sParams, "dir"))
        bOnlyDir = true;

    if (findSettingOption(sParams, "pattern") || findSettingOption(sParams, "p"))
    {
        int nPos = 0;

        if (findSettingOption(sParams, "pattern"))
            nPos = findSettingOption(sParams, "pattern");
        else
            nPos = findSettingOption(sParams, "p");

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
            hFind = FindFirstFile((_option.getPlotPath() + "\\" + sPattern).c_str(), &FindFileData);
            sDirectory = _option.getPlotPath();
        }
        else if (sDir == "SCRIPTPATH")
        {
            hFind = FindFirstFile((_option.getScriptPath() + "\\" + sPattern).c_str(), &FindFileData);
            sDirectory = _option.getScriptPath();
        }
        else if (sDir == "PROCPATH")
        {
            hFind = FindFirstFile((_option.getProcPath() + "\\" + sPattern).c_str(), &FindFileData);
            sDirectory = _option.getProcPath();
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
                if (sDir.starts_with("<loadpath>"))
                    sDirectory = _option.getLoadPath() + sDir.substr(10);
                else if (sDir.starts_with("<savepath>"))
                    sDirectory = _option.getSavePath() + sDir.substr(10);
                else if (sDir.starts_with("<scriptpath>"))
                    sDirectory = _option.getScriptPath() + sDir.substr(12);
                else if (sDir.starts_with("<plotpath>"))
                    sDirectory = _option.getPlotPath() + sDir.substr(10);
                else if (sDir.starts_with("<procpath>"))
                    sDirectory = _option.getProcPath() + sDir.substr(10);
                else if (sDir.starts_with("<wp>"))
                    sDirectory = _option.getWorkPath() + sDir.substr(4);
                else if (sDir.starts_with("<>") || sDir.starts_with("<this>"))
                    sDirectory = _option.getExePath() + sDir.substr(sDir.find('>') + 1);

                hFind = FindFirstFile((sDirectory + "\\" + sPattern).c_str(), &FindFileData);
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

                sConnect.append(nFirstColLength + 7 - nLength, '.');

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
                        sConnect += sExt.substr(sExt.rfind('_') + 1) + "-" + _lang.get("COMMON_FILETYPE_NOEXT");
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
                sConnect.append(_option.getWindow() - sConnect.length() - sFilesize.length(), '.');
                sConnect += sFilesize;

                if (sExt == _lang.get("COMMON_FILETYPE_NDAT") && _option.showExtendedFileInfo())
                {
                    sConnect += "\n|     : ";
                    std::string sFileInfo = getFileInfo(sFileName);
                    replaceAll(sFileInfo, "\n", "\n|     : ");
                    sConnect += sFileInfo;
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
    size_t nFirstColLength = _option.getWindow() / 2 - 6;
    bool bFreePath = false;

    // Extract a search pattern
    if (findSettingOption(__sCmd, "pattern") || findSettingOption(__sCmd, "p"))
    {
        int nPos = 0;

        if (findSettingOption(__sCmd, "pattern"))
            nPos = findSettingOption(__sCmd, "pattern");
        else
            nPos = findSettingOption(__sCmd, "p");

        sPattern = getArgAtPos(__sCmd, nPos);
        StripSpaces(sPattern);

        if (sPattern.length())
            sPattern = _lang.get("BUILTIN_LISTFILES_FILTEREDFOR", sPattern);
    }

    NumeReKernel::toggleTableStatus();

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
    size_t nPos = findSettingOption(__sCmd, "files");

    if (nPos && sCmd[nPos - 1] == '=')
    {
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

            NumeReKernel::print(createListDirectoryHeader(_option.getProcPath(), _lang.get("BUILTIN_LISTFILES_PROCPATH"), _option.getWindow()));

            if (!listDirectory("PROCPATH", __sCmd, _option))
                NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
        }

        if (!sSpecified.length() || sSpecified == "plotpath")
        {
            if (!sSpecified.length())
                NumeReKernel::printPreFmt("|\n" );

            NumeReKernel::print(createListDirectoryHeader(_option.getPlotPath(), _lang.get("BUILTIN_LISTFILES_PLOTPATH"), _option.getWindow()));

            if (!listDirectory("PLOTPATH", __sCmd, _option))
                NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
        }

        if (sSpecified == "wp")
        {
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

    NumeReKernel::toggleTableStatus();
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
    for (size_t i = 0; i < vFuncs.size(); i++)
    {
        NumeReKernel::printPreFmt(LineBreak("|   " + vFuncs[i], _option, false, 0, 75) + "\n");
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
        for (size_t i = 0; i < _functions.getDefinedFunctions(); i++)
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
        NumeReKernel::printPreFmt("|   -- " + toString(_functions.getDefinedFunctions()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_FUNCTIONS"))  + " --\n");
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
    for (size_t i = 0; i < vLogicals.size(); i++)
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
    map<string, std::pair<size_t, size_t>> CacheMap = _data.getTableMap();

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
    string sStringSize = toString(_data.getStringElements()) + " x " + toString(_data.getStringCols());

    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toUpperCase(toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_HEADLINE"))));
    make_hline();

    // Print all defined caches first
    for (auto iter = CacheMap.begin(); iter != CacheMap.end(); ++iter)
    {
        string sCacheSize = toString(_data.getLines(iter->first, false)) + " x " + toString(_data.getCols(iter->first, false));
        NumeReKernel::printPreFmt("|   " + iter->first + "()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - (iter->first).length() + _option.getWindow(0) % 2) + strfill(sCacheSize, (_option.getWindow(0) - 50) / 2) + strfill("[table]", 19));

        if (_data.getSize(iter->second.second) >= 1024 * 1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second.second) / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
        else if (_data.getSize(iter->second.second) >= 1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second.second) / (1024.0), 4), 9) + " KBytes\n");
        else
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second.second)), 9) + "  Bytes\n");

        nBytesSum += _data.getSize(iter->second.second);
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
            if (StringMap[item->first].length() > _option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 4)
                NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first].substr(0, _option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 7) + "...\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
            else
                NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first] + "\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
            NumeReKernel::printPreFmt(strfill("[string]", 19) + strfill(toString(StringMap[item->first].size()), 9) + "  Bytes\n");
            nBytesSum += StringMap[item->first].size();
        }
        else
        {
            // This is a numerical variable
            NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0) - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow(0) % 2) + strfill(toString(*variables[item->first], _option.getPrecision()), (_option.getWindow(0) - 60) / 2 + _option.getPrecision()) + (variables[item->first]->imag() ? strfill("[complex]", 19) + strfill("16", 9) + "  Bytes\n" : strfill("[double]", 19) + strfill("8", 9) + "  Bytes\n"));
            nBytesSum += variables[item->first]->imag() ? sizeof(std::complex<double>) : sizeof(double);
        }
    }

    // Create now the footer of the list:
    // Combine the number of variables and data
    // tables first
    NumeReKernel::printPreFmt("|   -- " + toString(VarMap.size()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_VARS_AND")) + " ");
    if (_data.isValid() || _data.getStringElements())
    {
        if (_data.isValid() && _data.getStringElements())
        {
            NumeReKernel::printPreFmt(toString(1 + CacheMap.size()));
            nDataSetNum = CacheMap.size() + 1;
        }
        else if (_data.isValid())
        {
            NumeReKernel::printPreFmt(toString(CacheMap.size()));
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
    const int nUnits = 21;
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
        "_elec[A s/(V m)]",
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
        NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow() - 10) / 2 + 2 - _option.getPrecision() - (item->first).length() + _option.getWindow() % 2) + strfill(toString(item->second, _option.getPrecision()), _option.getPrecision() + (_option.getWindow() - 50) / 2));
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
    for (size_t i = 0; i < vCMDList.size(); i++)
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
/// \param nWindowsize size_t
/// \return void
///
/////////////////////////////////////////////////
static void printUnits(const string& sUnit, const string& sDesc, const string& sDim, const string& sValues, size_t nWindowsize)
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
/// \brief This function returns a vector with
/// column widths for a table, depending on
/// the overall table size and the minimal
/// desired column sizes. A zero for the minimal
/// desired column width means to maximize that
/// column. If multiple columns are maximized then
/// the space is distributed equally between them.
///
/// \param maxWidth size_t
/// \param minDesiredColWidth std::vector<size_t>
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> calcTableWidth(size_t maxWidth, std::vector<size_t> minDesiredColWidth)
{
    std::vector<size_t> colWidth;

    // Get the average column width
    size_t avgWidth = maxWidth / minDesiredColWidth.size();

    // Find the column width, depending on wether this column should be maxed or not
    for (size_t i = 0; i < minDesiredColWidth.size(); i++)
    {
        if (minDesiredColWidth[i] > 0)
            colWidth.push_back(std::min(avgWidth, minDesiredColWidth[i]));
        else
            colWidth.push_back(0);
    }

    // Find the number of columns to be maximized
    size_t numOfMarkedColumns = std::count(colWidth.begin(), colWidth.end(), 0);

    // Get the current sum of chars
    size_t sumOfChars = 0;
    for (auto& n : colWidth)
        sumOfChars += n;

    // Expand the marked columns
    for (size_t i = 0; i < colWidth.size(); i++)
    {
        if (colWidth[i] == 0)
            colWidth[i] = (maxWidth - sumOfChars) / numOfMarkedColumns;
    }

    return colWidth;
}


/////////////////////////////////////////////////
/// \brief This function actually plots the table
/// with the desired colun widths. For this
/// all the content to plot and the column
/// sizes are required.
///
/// \param lineEntries std::vector<std::string>
/// \param lineColSizes std::vector<size_t>
/// \return void
///
/////////////////////////////////////////////////
void plotTableBySize(std::vector<std::string> lineEntries, std::vector<size_t> lineColSizes)
{
    // Format the string
    string sDummy = "";
    for (auto& thisEntry : lineEntries)
    {
        thisEntry = '"' + thisEntry + "\" -nq";
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(thisEntry, sDummy, true, false, true);
    }

    // Split all columns into multiple lines if necessary
    std::vector<std::vector<std::string>> splittedLineEntries;
    for (size_t i = 0; i < lineEntries.size(); i++)
    {
        splittedLineEntries.push_back(splitIntoLines(lineEntries[i], lineColSizes[i], true, 2, 2));
    }

    // Print lines as long as strings are not all empty
    while (true)
    {
        std::string sLine = "|   ";

        // Join the various columns into one line
        for (size_t i = 0; i < splittedLineEntries.size(); i++)
        {
            if (splittedLineEntries[i].size())
            {
                sLine += strlfill(splittedLineEntries[i].front(), lineColSizes[i], ' ');
                splittedLineEntries[i].erase(splittedLineEntries[i].begin());
            }
            else
                sLine += strlfill("", lineColSizes[i], ' ');
        }

        // Check if all strings have been plotted completely
        size_t totalStringElements = 0;
        for (std::vector<std::string> thisColumn : splittedLineEntries)
            totalStringElements += thisColumn.size();

        // Plot depending on last line or not
        if (totalStringElements <= 0)
        {
            NumeReKernel::printPreFmt(sLine + "\n|\n");
            break;
        }
        else
        {
            NumeReKernel::printPreFmt(sLine + "\n");
        }
    }
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

    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTPLUGINS_HEADLINE"))));
    make_hline();
    Procedure& _procedure = NumeReKernel::getInstance()->getProcedureInterpreter();

    // Probably there's no plugin defined
    if (!_procedure.getPackageCount())
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTPLUGINS_EMPTY")));
    else
    {
        size_t largewindowoffset = _option.getWindow() > 230 ? 10 : 0;
        // The info to be printed is: Package Name, Version, Command, Description, Author, License
        // The minimal desired column width is
        std::vector<size_t> minDesiredColWidth{20 + largewindowoffset,
                                               10,
                                               25 + largewindowoffset,
                                               0,
                                               20 + largewindowoffset,
                                               15 + largewindowoffset};

        //Get the terminal window width minus the 4 digit indent
        size_t maxWidth = _option.getWindow(0) - 4;

        // Set the column withs depending on number of columns and window width
        std::vector<size_t> colWidth = calcTableWidth(maxWidth, minDesiredColWidth);

        // Print the table head and an empty line
        std::vector<std::string> headerEntries;
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_PACKAGENAME"));
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_VERSION"));
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_COMMAND"));
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_DESCRIPTION"));
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_AUTHOR"));
        headerEntries.push_back(_lang.get("PARSERFUNCS_LISTPLUGINS_LICENSE"));
        plotTableBySize(headerEntries, colWidth);

        // Print all plugins (name, command and description) on the terminal
        for (size_t i = 0; i < _procedure.getPackageCount(); i++)
        {
            // Tabellenfunktion unter utils/tools.cpp oder util/stringtools.cpp
            std::vector<std::string> lineEntries;

            // Print package name
            lineEntries.push_back(_procedure.getPackageName(i));
            // Print package version
            lineEntries.push_back("v" + _procedure.getPackageVersion(i));
            // Print command info
            lineEntries.push_back(_procedure.getPluginCommand(i).length() ? _procedure.getPluginCommandSignature(i) : "---");
            // Print the description
            lineEntries.push_back(_procedure.getPackageDescription(i));
            // Print package author
            lineEntries.push_back(_procedure.getPackageAuthor(i));
            // Print package license
            lineEntries.push_back(_procedure.getPackageLicense(i));
            // Plot this line
            plotTableBySize(lineEntries, colWidth);
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
    if (!_option.executeEnabled())
        throw SyntaxError(SyntaxError::EXECUTE_COMMAND_DISABLED, sCmd, "execute");

    CommandLineParser cmdParser(sCmd, "execute", CommandLineParser::CMD_EXPR_set_PAR);

    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());
    _fSys.declareFileType(".exe");
    string sParams = cmdParser.getParameterValueAsString("params", "", true);
    string sWorkpath = cmdParser.getParameterValueAsString("wp", "");
    string sObject = cmdParser.parseExprAsString();
    int nRetVal = 0;
    bool bWaitForTermination = cmdParser.hasParam("wait");

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
        if (_option.systemPrints())
            NumeReKernel::printPreFmt("|-> " + _lang.get("COMMON_EVALUATING") + " ... ");

        while (true)
        {
            // wait 1sec and check, whether the user pressed the ESC key
            if (WaitForSingleObject(ShExecInfo.hProcess, 1000) == WAIT_OBJECT_0)
                break;

            if (NumeReKernel::GetAsyncCancelState())
            {
                if (_option.systemPrints())
                    NumeReKernel::printPreFmt(_lang.get("COMMON_CANCEL") + "\n");

                throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
            }
        }

        if (_option.systemPrints())
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
        if (_option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(  _lang.get("BUILTIN_AUTOSAVE") + " ... "));

        // Try to save the cache
        if (_data.saveToCacheFile())
        {
            if (_option.systemPrints())
                NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS") + ".") );
        }
        else
        {
            if (_option.systemPrints())
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
/// \param pos size_t
/// \return string
///
/////////////////////////////////////////////////
static string getPathForSetting(string& sCmd, size_t pos)
{
    string sPath;

    addArgumentQuotes(sCmd, pos);

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
    _accessParser.evalIndices();
    Memory* mem = _data.getTable(_accessParser.getDataObject())->extractRange(_accessParser.getIndices().row,
                  _accessParser.getIndices().col);
    _cache.melt(mem, "table", true);
}


/////////////////////////////////////////////////
/// \brief This static function is used to detect
/// a setting option independent on a leading
/// dash character in front of the option value.
///
/// \param sCmd const std::string&
/// \param sOption const std::string&
/// \return size_t
///
/////////////////////////////////////////////////
static size_t findSettingOption(const std::string& sCmd, const std::string& sOption)
{
    size_t pos = findParameter(sCmd, sOption, '=');

    if (pos)
        return pos + sOption.length();

    pos = findParameter(sCmd, sOption);

    if (pos)
        return pos - 1 + sOption.length();

    pos = sCmd.find(sOption);

    if (pos != std::string::npos
            && (!pos || sCmd[pos - 1] == ' '))
    {
        if (pos + sOption.length() == sCmd.length() || sCmd[pos + sOption.length()] == ' ')
            return pos + sOption.length();

        if (sCmd[pos + sOption.length()] == '=')
            return pos + sOption.length() + 1;
    }

    return 0u;
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

        if (_option.systemPrints())
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

        if (_option.systemPrints())
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

    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_DAT_PAR);

    size_t nPrecision = _option.getPrecision();

    // Update the precision, if the user selected any
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("precision");

    if (vParVal.size())
        nPrecision = std::min(14LL, intCast(vParVal.front()));

    // Copy the selected data into another datafile instance and
    // save the copied data
    DataAccessParser _access = cmdParser.getExprAsDataObject();

    if (_access.getDataObject().length())
    {
        // Create the new instance
        MemoryManager _cache;

        // Update the necessary parameters
        _cache.setTokens(_option.getTokenPaths());
        _cache.setPath(_option.getSavePath(), false, _option.getExePath());

        copyDataToTemporaryTable(sCmd, _access, _data, _cache);

        // Update the name of the  cache table (force it)
        if (_access.getDataObject() != "table")
            _cache.renameTable("table", _access.getDataObject(), true);

        // If the command line contains string variables
        // get those values here
        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
            NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        std::string sFileName;
        std::string sFileFormat;

        if (cmdParser.hasParam("fileformat"))
            sFileFormat = cmdParser.getParameterValueAsString("fileformat", "", true, true);

        if (cmdParser.getCommand() == "export")
            sFileName = cmdParser.getFileParameterValue(".dat", "<savepath>", "");
        else
            sFileName = cmdParser.getFileParameterValue(".ndat", "<savepath>", "");

        if (!sFileName.length())
        {
            _cache.setPrefix(_access.getDataObject());

            if (cmdParser.getCommand() == "export")
                sFileName = _cache.generateFileName(".dat");
            else
                sFileName = _cache.generateFileName(".ndat");
        }

        if (_cache.saveFile(_access.getDataObject(), sFileName, nPrecision, sFileFormat))
        {
            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sFileName, sFileName);

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
    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_DAT_PAR);

    if (cmdParser.getExpr().length())
        doc_SearchFct(cmdParser.getExpr(), _option);
    else if (cmdParser.getParameterList().length())
        doc_SearchFct(cmdParser.getParameterList(), _option);
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
    CommandLineParser cmdParser(sCmd, "integrate", CommandLineParser::CMD_EXPR_set_PAR);

    if (cmdParser.getCommand().substr(0, 10) == "integrate2"
            || cmdParser.parseIntervals().size() == 2)
    {
        integrate2d(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
    {
        integrate(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
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
    CommandLineParser cmdParser(sCmd, "diff", CommandLineParser::CMD_EXPR_set_PAR);

    if (cmdParser.getExpr().length())
    {
        differentiate(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("diff", NumeReKernel::getInstance()->getSettings());

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
    CommandLineParser cmdParser(sCmd, "extrema", CommandLineParser::CMD_EXPR_set_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (cmdParser.getExpr().length())
    {
        if (findExtrema(cmdParser))
        {
            sCmd = cmdParser.getReturnValueStatement();
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
    CommandLineParser cmdParser(sCmd, "pulse", CommandLineParser::CMD_DAT_PAR);

    if (!analyzePulse(cmdParser))
    {
        doc_Help("pulse", NumeReKernel::getInstance()->getSettings());
        return COMMAND_PROCESSED;
    }

    sCmd = cmdParser.getReturnValueStatement();

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
    CommandLineParser cmdParser(sCmd, "eval", CommandLineParser::CMD_EXPR_set_PAR);

    if (evalPoints(cmdParser))
    {
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("eval", NumeReKernel::getInstance()->getSettings());

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
    CommandLineParser cmdParser(sCmd, "zeroes", CommandLineParser::CMD_EXPR_set_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (cmdParser.getExpr().length())
    {
        if (findZeroes(cmdParser))
        {
            sCmd = cmdParser.getReturnValueStatement();
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
    CommandLineParser cmdParser(sCmd, "sort", CommandLineParser::CMD_EXPR_set_PAR);

    sortData(cmdParser);

    if (cmdParser.getReturnValueStatement().length())
    {
        sCmd = cmdParser.getReturnValueStatement();
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
    CommandLineParser cmdParser(sCmd, "dialog", CommandLineParser::CMD_EXPR_set_PAR);
    dialogCommand(cmdParser);
    sCmd = cmdParser.getReturnValueStatement();

    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// interface to the particle swarm optimizer.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_pso(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "pso", CommandLineParser::CMD_EXPR_set_PAR);

    // Call the optimizer
    particleSwarmOptimizer(cmdParser);

    sCmd = cmdParser.getReturnValueStatement();

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
                    _pData.setParams(sCmd.substr(sCmd.find("--")));
                else
                    _pData.setParams(sCmd.substr(sCmd.find("-set")));

                if (_option.systemPrints())
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
    CommandLineParser cmdParser(sCmd, "fft", CommandLineParser::CMD_DAT_PAR);

    fastFourierTransform(cmdParser);
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
    CommandLineParser cmdParser(sCmd, "fwt", CommandLineParser::CMD_DAT_PAR);

    fastWaveletTransform(cmdParser);
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

    const std::map<std::string, SettingsValue>& mSettings = _option.getSettings();

    size_t nPos = findCommand(sCmd, "get").nPos;
    string sCommand = extractCommandString(sCmd, findCommand(sCmd, "get"));

    bool asVal = findParameter(sCmd, "asval");
    bool asStr = findParameter(sCmd, "asstr");

    // Handle some special cases
    if (findSettingOption(sCmd, "windowsize"))
    {
        std::string convertedValue = "x = " + toString(mSettings.at(SETTING_V_WINDOW_X).value() + 1) + ", y = " + toString(mSettings.at(SETTING_V_WINDOW_Y).value() + 1);
        if (asVal)
        {
            if (!nPos)
                sCmd = convertedValue;
            else
                sCmd.replace(nPos, sCommand.length(), "{" + toString(mSettings.at(SETTING_V_WINDOW_X).value() + 1) + ", " + toString(mSettings.at(SETTING_V_WINDOW_Y).value() + 1) + "}");

            return COMMAND_HAS_RETURNVALUE;
        }

        if (asStr)
        {
            if (!nPos)
                sCmd = "\"" + convertedValue + "\"";
            else
                sCmd.replace(nPos, sCommand.length(), "\"" + convertedValue + "\"");

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print("WINDOWSIZE: " + convertedValue);
        return COMMAND_PROCESSED;
    }
    else if (findSettingOption(sCmd, "varlist"))
    {
        if (asStr)
        {
            if (!nPos)
                sCmd = getVarList("vars -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("vars -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("VARLIST: " + getVarList("vars", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findSettingOption(sCmd, "stringlist"))
    {
        if (asStr)
        {
            if (!nPos)
                sCmd = getVarList("strings -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("strings -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("STRINGLIST: " + getVarList("strings", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findSettingOption(sCmd, "numlist"))
    {
        if (asStr)
        {
            if (!nPos)
                sCmd = getVarList("nums -asstr", _parser, _data, _option);
            else
                sCmd.replace(nPos, sCommand.length(), getVarList("nums -asstr", _parser, _data, _option));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("NUMLIST: " + getVarList("nums", _parser, _data, _option), _option, false));
        return COMMAND_PROCESSED;
    }
    else if (findSettingOption(sCmd, "plotparams"))
    {
        if (asStr)
        {
            if (!nPos)
                sCmd = _pData.getParams(true);
            else
                sCmd.replace(nPos, sCommand.length(), _pData.getParams(true));

            return COMMAND_HAS_RETURNVALUE;
        }

        NumeReKernel::print(LineBreak("PLOTPARAMS: " + _pData.getParams(), _option, false));
        return COMMAND_PROCESSED;
    }

    // Handle generic cases
    for (auto iter = mSettings.begin(); iter != mSettings.end(); ++iter)
    {
        if (findSettingOption(sCmd, iter->first.substr(iter->first.find('.') + 1)) && !iter->second.isHidden())
        {
            std::string convertedValue;

            switch (iter->second.getType())
            {
                case SettingsValue::BOOL:
                    convertedValue = toString(iter->second.active());
                    break;
                case SettingsValue::UINT:
                    convertedValue = toString(iter->second.value());
                    break;
                case SettingsValue::STRING:
                    convertedValue = iter->second.stringval();
                    break;
            }

            switch (iter->second.getType())
            {
                case SettingsValue::BOOL:
                case SettingsValue::UINT:
                    if (asVal)
                    {
                        if (!nPos)
                            sCmd = convertedValue;
                        else
                            sCmd.replace(nPos, sCommand.length(), convertedValue);

                        break;
                    }
                // Fallthrough intended
                case SettingsValue::STRING:
                    if (asStr || asVal)
                    {
                        if (!nPos)
                            sCmd = "\"" + convertedValue + "\"";
                        else
                            sCmd.replace(nPos, sCommand.length(), "\"" + convertedValue + "\"");
                    }
                    else
                        NumeReKernel::print(toUpperCase(iter->first.substr(iter->first.find('.') + 1)) + ": " + convertedValue);

                    break;
            }

            if (asStr || asVal)
                return COMMAND_HAS_RETURNVALUE;
            else
                return COMMAND_PROCESSED;
        }
    }

    // Will only reach this point, if no setting has been found
    doc_Help("get", _option);
    return COMMAND_PROCESSED;
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
    CommandLineParser cmdParser(sCmd, "readline", CommandLineParser::CMD_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    std::string sDefault = cmdParser.getParameterValueAsString("dflt", "");
    std::string sMessage = cmdParser.getParameterValueAsString("msg", "");
    std::string sArgument;


    while (!sArgument.length())
    {
        string sLastLine = "";
        NumeReKernel::printPreFmt("|-> ");

        if (sMessage.length())
        {
            sLastLine = LineBreak(sMessage, _option, false, 4);
            NumeReKernel::printPreFmt(sLastLine);

            if (sLastLine.find('\n') != string::npos)
                sLastLine.erase(0, sLastLine.rfind('\n'));

            if (sLastLine.starts_with("|   ") || sLastLine.starts_with("|<- ") || sLastLine.starts_with("|-> "))
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

    if (cmdParser.hasParam("asstr") && sArgument[0] != '"' && sArgument[sArgument.length() - 1] != '"')
        cmdParser.setReturnValue("\"" + sArgument + "\"");
    else
        cmdParser.setReturnValue(sArgument);

    sCmd = cmdParser.getReturnValueStatement();

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
    CommandLineParser cmdParser(sCmd, "read", CommandLineParser::CMD_DAT_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (cmdParser.getExpr().length())
    {
        readFromFile(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("read", _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "window" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_window(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "window", CommandLineParser::CMD_DAT_PAR);

    if (cmdParser.getExpr().length() || cmdParser.getParameterList().length())
    {
        windowCommand(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("window", NumeReKernel::getInstance()->getSettings());

    return COMMAND_PROCESSED;
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
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (sCmd.length() > 5)
    {
        string sArgument;
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        {
            extractFirstParameterStringValue(sCmd, sArgument);
            sArgument = "edit " + sArgument;
            editObject(sArgument, _data, _option);
        }
        else
            editObject(sCmd, _data, _option);
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
    CommandLineParser cmdParser(sCmd, "taylor", CommandLineParser::CMD_EXPR_set_PAR);

    if (cmdParser.getExpr().length())
    {
        taylor(cmdParser);
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
    }
    else
        doc_Help("taylor", NumeReKernel::getInstance()->getSettings());

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
/// "delete" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_delete(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_DAT_PAR);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sArgument;
    int nArgument;

    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    if (_data.containsTablesOrClusters(cmdParser.getExpr()))
    {
        if (!cmdParser.hasParam("ignore") && !cmdParser.hasParam("i"))
        {
            if (!confirmOperation(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_CONFIRM")))
                return COMMAND_PROCESSED;
        }

        if (deleteCacheEntry(cmdParser))
        {
            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETE_SUCCESS"));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_DELETE_ELEMENTS, sCmd, SyntaxError::invalid_position);
    }
    else if (accessParser.getDataObject() == "string")
    {
        nArgument = accessParser.getIndices().row.front();

        if (_data.removeStringElements(nArgument))
        {
            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_SUCCESS", toString(nArgument + 1)));
        }

        return COMMAND_PROCESSED;
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function clears all user
/// defined variables.
///
/// \return void
///
/////////////////////////////////////////////////
static void clear_variables()
{
    NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();
    _stringParser.clearStringVar();

    static std::vector<std::string> defaultVars = {"t", "x", "y", "z", "ncols", "nlens", "nlines", "nrows", "ans"};
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    varmap_type mVariables = _parser.GetVar();

    // remove variables, which are not default variables and no temporary variables
    for (auto iter = mVariables.begin(); iter != mVariables.end(); ++iter)
    {
        bool isTemporaryVar = iter->first.starts_with("_~");
        bool isDefaultVar = std::find(defaultVars.begin(), defaultVars.end(), iter->first) != defaultVars.end();
        if (!isTemporaryVar && !isDefaultVar)
            _parser.RemoveVar(iter->first);
    }
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
    CommandLineParser cmdParser(sCmd, "clear", CommandLineParser::CMD_DAT_PAR);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Match _mMatch = findCommand(sCmd);

    if (_data.containsTables(cmdParser.getExpr()))
    {
        string sCommand = "delete " + cmdParser.getExpr();

        if (cmdParser.hasParam("ignore") || cmdParser.hasParam("i"))
            sCommand += " -ignore";

        return cmd_delete(sCommand);
    }
    else if (cmdParser.hasParam("var") && NumeReKernel::getInstance()->getDebugger().getStackSize() == 0)
    {
        clear_variables();
    }
    else if (cmdParser.hasParam("memory") && NumeReKernel::getInstance()->getDebugger().getStackSize() == 0)
    {
        // Clear all tables
        if (cmdParser.hasParam("ignore") || cmdParser.hasParam("i"))
            clear_cache(_data, _option, true);
        else
            clear_cache(_data, _option);

        // Clear also the string table
        _data.clearStringElements();

        // Clear also user-defined variables if called from terminal
        clear_variables();

        // Clear also the clusters
        _data.clearAllClusters();
        NumeRe::Cluster& ans = _data.newCluster("ans");
        ans.setDouble(0, NAN);
        NumeReKernel::getInstance()->setAns(&ans);
    }
    else if (cmdParser.getExpr() == "string()")
    {
        if (_data.clearStringElements())
        {
            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_SUCCESS"));
        }

        return COMMAND_PROCESSED;
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "close" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_close(string& sCmd)
{
    NumeReKernel* instance = NumeReKernel::getInstance();

    if (findParameter(sCmd, "all"))
        instance->closeWindows(WT_ALL);
    else if (findParameter(sCmd, "graphs"))
        instance->closeWindows(WT_GRAPH);
    else if (findParameter(sCmd, "tables"))
        instance->closeWindows(WT_TABLEVIEWER);
    else if (findParameter(sCmd, "images"))
        instance->closeWindows(WT_IMAGEVIEWER);
    else if (findParameter(sCmd, "docs"))
        instance->closeWindows(WT_DOCVIEWER);

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
                NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
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
        _script.openScript(sArgument, 0);
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
    CommandLineParser cmdParser(sCmd, "copy", CommandLineParser::CMD_DAT_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (cmdParser.exprContainsDataObjects())
    {
        if (CopyData(cmdParser))
        {
            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYDATA_SUCCESS"));
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, sCmd, SyntaxError::invalid_position);
    }
    else if ((cmdParser.hasParam("target") || cmdParser.hasParam("t")) && cmdParser.getExpr().length())
    {
        if (moveOrCopyFiles(cmdParser))
        {
            if (_option.systemPrints())
            {
                if (cmdParser.hasParam("all") || cmdParser.hasParam("a"))
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_ALL_SUCCESS", cmdParser.getReturnValueStatement()));
                else
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_SUCCESS", cmdParser.getReturnValueStatement()));
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
    CommandLineParser cmdParser(sCmd, "append", CommandLineParser::CMD_DAT_PAR);

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    if (cmdParser.getExpr().length())
    {
        NumeReKernel::printPreFmt("\r");

        double j1 = _data.getCols("data") + 1;
        append_data(cmdParser);
        cmdParser.setReturnValue(std::vector<mu::value_type>({1, _data.getColElements(VectorIndex(j1, VectorIndex::OPEN_END), "data"), j1, _data.getCols("data")}));

        sCmd = cmdParser.getReturnValueStatement();
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
    CommandLineParser cmdParser(sCmd, "audio", CommandLineParser::CMD_EXPR_set_PAR);

    if (!writeAudioFile(cmdParser))
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);
    else if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_AUDIO_SUCCESS"));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "clc" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_clc(string& sCmd)
{
    NumeReKernel::clcTerminal();

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "audioread" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_audioread(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "audioread", CommandLineParser::CMD_EXPR_set_PAR);

    if (!readAudioFile(cmdParser))
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sCmd, SyntaxError::invalid_position, cmdParser.getExprAsFileName(".wav"));
    else if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_AUDIOREAD_SUCCESS"));

    sCmd = cmdParser.getReturnValueStatement();
    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "seek" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_seek(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "seek", CommandLineParser::CMD_EXPR_set_PAR);

    if (!seekInAudioFile(cmdParser))
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sCmd, SyntaxError::invalid_position, cmdParser.getExprAsFileName(".wav"));
    else if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_SEEK_SUCCESS"));

    sCmd = cmdParser.getReturnValueStatement();
    return COMMAND_HAS_RETURNVALUE;
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
    CommandLineParser cmdParser(sCmd, "imread", CommandLineParser::CMD_DAT_PAR);

    readImage(cmdParser);

    sCmd = cmdParser.getReturnValueStatement();
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
    CommandLineParser cmdParser(sCmd, "write", CommandLineParser::CMD_EXPR_set_PAR);

    if (cmdParser.getExpr().length() && cmdParser.hasParam("file"))
        writeToFile(cmdParser);
    else
        doc_Help("write", NumeReKernel::getInstance()->getSettings());

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
    CommandLineParser cmdParser(sCmd, "workpath", CommandLineParser::CMD_EXPR_set_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!cmdParser.getExpr().length())
        return NO_COMMAND;

    _option.getSetting(SETTING_S_WORKPATH).stringval() = cmdParser.getExprAsFileName("");

    if (_option.systemPrints())
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
    CommandLineParser cmdParser(sCmd, "warn", CommandLineParser::CMD_EXPR_set_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    std::string sMessage = cmdParser.getExpr();

    if (sMessage.length())
    {
        if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sMessage))
        {
            auto vVals = cmdParser.parseExprAsNumericalValues();

            if (vVals.size() > 1)
            {
                sMessage = "{";

                for (size_t i = 0; i < vVals.size(); i++)
                    sMessage += " " + toString(vVals[i], _option.getPrecision()) + ",";

                sMessage.back() = '}';
            }
            else
                sMessage = toString(vVals.front(), _option.getPrecision());
        }
        else
            sMessage = cmdParser.parseExprAsString();

        NumeReKernel::issueWarning(sMessage);
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
    CommandLineParser cmdParser(sCmd, "stats", CommandLineParser::CMD_DAT_PAR);
    plugin_statistics(cmdParser);

    if (cmdParser.getReturnValueStatement().length())
    {
        sCmd = cmdParser.getReturnValueStatement();
        return COMMAND_HAS_RETURNVALUE;
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
    CommandLineParser cmdParser(sCmd, "stfa", CommandLineParser::CMD_DAT_PAR);

    if (!shortTimeFourierAnalysis(cmdParser))
        doc_Help("stfa", NumeReKernel::getInstance()->getSettings());
    else if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_STFA_SUCCESS", cmdParser.getReturnValueStatement()));

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
    CommandLineParser cmdParser(sCmd, "spline", CommandLineParser::CMD_DAT_PAR);

    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!calculateSplines(cmdParser))
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
    else if (findParameter(sCmd, "settings"))
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

    std::map<std::string, SettingsValue>& mSettings = _option.getSettings();

    size_t nArgument;
    size_t pos;
    string sArgument;

    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
        NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    for (auto iter = mSettings.begin(); iter != mSettings.end(); ++iter)
    {
        if (iter->second.isMutable() && (pos = findSettingOption(sCmd, iter->first.substr(iter->first.find('.') + 1))))
        {
            switch (iter->second.getType())
            {
                case SettingsValue::BOOL:
                    if (!parseCmdArg(sCmd, pos, _parser, nArgument) || (nArgument != 0 && nArgument != 1))
                        nArgument = !iter->second.active();

                    iter->second.active() = (bool)nArgument;

                    _data.setbLoadEmptyCols(mSettings[SETTING_B_LOADEMPTYCOLS].active());

                    if (iter->first == SETTING_B_DEFCONTROL && mSettings[SETTING_B_DEFCONTROL].active()
                            && !_functions.getDefinedFunctions()
                            && fileExists(_option.getExePath() + "\\functions.def"))
                        _functions.load(_option);
                    else if (iter->first == SETTING_B_DEBUGGER)
                        NumeReKernel::getInstance()->getDebugger().setActive(mSettings[SETTING_B_DEBUGGER].active());

                    if (iter->second.isUiModifying())
                        NumeReKernel::modifiedSettings = true;

                    if (_option.systemPrints())
                        NumeReKernel::print(toUpperCase(iter->first.substr(iter->first.find('.') + 1)) + ": " + toString((bool)nArgument));

                    break;
                case SettingsValue::UINT:
                    if (parseCmdArg(sCmd, pos, _parser, nArgument)
                            && nArgument >= iter->second.min()
                            && nArgument <= iter->second.max())
                    {
                        iter->second.value() = nArgument;

                        if (iter->second.isUiModifying())
                            NumeReKernel::modifiedSettings = true;

                        if (_option.systemPrints())
                            NumeReKernel::print(toUpperCase(iter->first.substr(iter->first.find('.') + 1)) + ": " + toString(nArgument));
                    }

                    break;
                case SettingsValue::STRING:
                    if (iter->second.isPath())
                    {
                        sArgument = getPathForSetting(sCmd, pos);

                        FileSystem _fSys;
                        _fSys.setTokens(_option.getTokenPaths());
                        _fSys.setPath(sArgument, true, _data.getProgramPath());

                        iter->second.stringval() = _fSys.getPath();

                        _out.setPath(mSettings[SETTING_S_SAVEPATH].stringval(), false, mSettings[SETTING_S_EXEPATH].stringval());
                        _data.setSavePath(mSettings[SETTING_S_SAVEPATH].stringval());
                        _data.setPath(mSettings[SETTING_S_LOADPATH].stringval(), false, mSettings[SETTING_S_EXEPATH].stringval());
                        _script.setPath(mSettings[SETTING_S_SCRIPTPATH].stringval(), false, mSettings[SETTING_S_EXEPATH].stringval());
                        _pData.setPath(mSettings[SETTING_S_PLOTPATH].stringval(), false, mSettings[SETTING_S_EXEPATH].stringval());

                        if (iter->second.isUiModifying())
                            NumeReKernel::modifiedSettings = true;

                        if (_option.systemPrints())
                            NumeReKernel::print(toUpperCase(iter->first.substr(iter->first.find('.') + 1)) + ": " + iter->second.stringval());
                    }
                    else
                    {
                        if (sCmd[pos] == '=')
                            addArgumentQuotes(sCmd, pos);

                        if (extractFirstParameterStringValue(sCmd, sArgument))
                        {
                            if (sArgument.front() == '"')
                                sArgument.erase(0, 1);

                            if (sArgument.back() == '"')
                                sArgument.erase(sArgument.length() - 1);

                            if (iter->first == SETTING_S_PLOTFONT)
                            {
                                _option.setDefaultPlotFont(sArgument);
                                _fontData.LoadFont(mSettings[SETTING_S_PLOTFONT].stringval().c_str(), mSettings[SETTING_S_EXEPATH].stringval().c_str());
                                _pData.setFont(mSettings[SETTING_S_PLOTFONT].stringval());
                            }
                            else
                                iter->second.stringval() = sArgument;

                            if (iter->second.isUiModifying())
                                NumeReKernel::modifiedSettings = true;

                            if (_option.systemPrints())
                                NumeReKernel::print(toUpperCase(iter->first.substr(iter->first.find('.') + 1)) + ": " + sArgument);
                        }
                    }

            }

            return COMMAND_PROCESSED;
        }
    }

    if ((pos = findSettingOption(sCmd, "mode")))
    {
        sArgument = getArgAtPos(sCmd, pos, ARGEXTRACT_STRIPPED);

        if (sArgument.length() && sArgument == "debug")
        {
            if (_option.useDebugger())
            {
                mSettings[SETTING_B_DEBUGGER].active() = false;
                NumeReKernel::getInstance()->getDebugger().setActive(false);
            }
            else
            {
                mSettings[SETTING_B_DEBUGGER].active() = true;
                NumeReKernel::getInstance()->getDebugger().setActive(true);
            }

            if (_option.systemPrints())
                NumeReKernel::print("DEBUGGER: " + toString(_option.useDebugger()));
        }
        else if (sArgument.length() && sArgument == "developer")
        {
            if (_option.isDeveloperMode())
            {
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_INACTIVE"), _option) );
                mSettings[SETTING_B_DEVELOPERMODE].active() = false;
                _parser.EnableDebugDump(false, false);
                g_logger.setLoggingLevel(Logger::LVL_INFO);
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
                    mSettings[SETTING_B_DEVELOPERMODE].active() = true;
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_SUCCESS"), _option) );
                    _parser.EnableDebugDump(true, true);
                    g_logger.setLoggingLevel(Logger::LVL_DEBUG);
                }
                else
                    NumeReKernel::print(toSystemCodePage( _lang.get("COMMON_CANCEL")) );
            }
        }
    }
    else if (findSettingOption(sCmd, "save"))
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
    CommandLineParser cmdParser(sCmd, "start", CommandLineParser::CMD_DAT_PAR);
    Script& _script = NumeReKernel::getInstance()->getScript();

    if (_script.isOpen())
        throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, "start");

    std::string sFileName = cmdParser.getExprAsFileName(".nscr", "<scriptpath>");

    if (!sFileName.length())
        throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sFileName, "[" + _lang.get("BUILTIN_CHECKKEYWORD_START_ERRORTOKEN") + "]");

    // Get the line to start from
    int nFromLine = 0;
    if (cmdParser.hasParam("fromline"))
    {
        // Get the line parameter and subtract 1 to match the internal line count
        std::vector<mu::value_type> vecFromLine = cmdParser.getParameterValueAsNumericalValue("fromline");
        nFromLine = intCast(vecFromLine.front()) - 1;
    }

    _script.openScript(sFileName, nFromLine);

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

    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_DAT_PAR);

    // Handle the compact mode (probably not needed any more)
    if (cmdParser.getCommand().starts_with("showf"))
        _out.setCompact(false);
    else
        _out.setCompact(_option.createCompactTables());

    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();
    _accessParser.evalIndices();

    if (_accessParser.getDataObject().length())
    {
        if (_accessParser.isCluster())
        {
            NumeRe::Cluster& cluster = _data.getCluster(_accessParser.getDataObject());

            // Create the target container
            NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), 1);

            // Copy the data to the new container
            for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
            {
                if (cluster.getType(i) == NumeRe::ClusterItem::ITEMTYPE_STRING)
                    _stringTable.set(i, 0, cluster.getString(_accessParser.getIndices().row[i]));
                else
                    _stringTable.set(i, 0, mu::isnan(cluster.getDouble(_accessParser.getIndices().row[i])) ? "---" : toString(cluster.getDouble(_accessParser.getIndices().row[i]), 5));
            }

            // Redirect control
            NumeReKernel::showStringTable(_stringTable, _accessParser.getDataObject() + "{}");

            return COMMAND_PROCESSED;
        }
        else if (_accessParser.getDataObject() == "string")
        {
            // Create the target container
            NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size());

            // Copy the data to the new container and add surrounding
            // quotation marks
            for (size_t j = 0; j < _accessParser.getIndices().col.size(); j++)
            {
                for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    if ((int)_data.getStringElements(_accessParser.getIndices().col[j]) <= _accessParser.getIndices().row[i])
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
            copyDataToTemporaryTable(sCmd, _accessParser, _data, _cache);
            _cache.renameTable("table", "*" + _accessParser.getDataObject(), true);

            // Redirect the control
            show_data(_cache, _out, _option, "*" + _accessParser.getDataObject(), _option.getPrecision());
            return COMMAND_PROCESSED;
        }
    }
    else
    {
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
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
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    CommandLineParser cmdParser(sCmd, "smooth", CommandLineParser::CMD_DAT_PAR);

    string sArgument;
    int nWindowSize = 1;
    double dAlpha = NAN;
    NumeRe::FilterSettings::FilterType _type = NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR;
    MemoryManager::AppDir dir = MemoryManager::ALL;

    // Find the window size
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("order");

    if (vParVal.size())
        nWindowSize = intCast(vParVal.front());

    // Ensure that the windowsize is odd (we don't need even window sizes)
    nWindowSize = 2 * nWindowSize + 1;

    // Find the window shape (used for type=gaussian)
    vParVal = cmdParser.getParameterValueAsNumericalValue("alpha");

    if (vParVal.size())
        dAlpha = vParVal.front().real();

    // Find the smoothing filter type
    std::string sFilterType = cmdParser.getParameterValue("type");

    if (!sFilterType.length())
        sFilterType = cmdParser.getParameterValue("method");

    if (sFilterType.length())
    {
        if (sFilterType == "weightedlinear")
            _type = NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR;
        else if (sFilterType == "gaussian")
            _type = NumeRe::FilterSettings::FILTER_GAUSSIAN;
        else if (sFilterType == "savitzkygolay")
            _type = NumeRe::FilterSettings::FILTER_SAVITZKY_GOLAY;
    }

    // Find the app dir
    if (cmdParser.hasParam("grid"))
        dir = MemoryManager::GRID;
    else if (cmdParser.hasParam("lines"))
        dir = MemoryManager::LINES;
    else if (cmdParser.hasParam("cols"))
        dir = MemoryManager::COLS;

    if (!cmdParser.exprContainsDataObjects())
        return COMMAND_PROCESSED;

    DataAccessParser _access = cmdParser.getExprAsDataObject();

    if (_access.getDataObject().length())
    {
        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject() + "(", _access.getIndexString());

        _access.evalIndices();

        if (!_data.isValueLike(_access.getIndices().col, _access.getDataObject()))
            throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _access.getDataObject() + "(", _access.getDataObject());

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
            if (_option.systemPrints())
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
    CommandLineParser cmdParser(sCmd, "hist", CommandLineParser::CMD_EXPR_set_PAR);

    plugin_histogram(cmdParser);

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
    Match _mMatch = findCommand(sCmd);

    if (_mMatch.nPos + _mMatch.sString.length() < sCmd.length())
        doc_Help(sCmd.substr(_mMatch.nPos + _mMatch.sString.length()), _option);
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
    CommandLineParser cmdParser(sCmd, "move", CommandLineParser::CMD_DAT_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (cmdParser.getExpr().length())
    {
        if (cmdParser.exprContainsDataObjects())
        {
            if (moveData(cmdParser))
            {
                if (_option.systemPrints())
                    NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_MOVEDATA_SUCCESS"), _option) );
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_MOVE_DATA, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            if (moveOrCopyFiles(cmdParser))
            {
                if (_option.systemPrints())
                {
                    if (cmdParser.hasParam("all") || cmdParser.hasParam("a"))
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_ALL_SUCCESS", cmdParser.getReturnValueStatement()));
                    else
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_SUCCESS", cmdParser.getReturnValueStatement()));
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
/// "pack" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_pack(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "pack", CommandLineParser::CMD_EXPR_set_PAR);
    NumeReKernel* instance = NumeReKernel::getInstance();

    if (cmdParser.getExpr().length() && cmdParser.hasParam("file"))
    {
        std::string sTargetPathName = cmdParser.getFileParameterValueForSaving("", "<savepath>", "");
        std::string sExpression = cmdParser.getExpr();
        Archive::Type type = Archive::ARCHIVE_AUTO;

        if (cmdParser.hasParam("type"))
        {
            std::string sType = cmdParser.getParameterValue("type");

            if (sType == "ziparchive")
                type = Archive::ARCHIVE_ZIP;
            else if (sType == "gzarchive")
                type = Archive::ARCHIVE_GZ;
            else if (sType == "tarchive")
                type = Archive::ARCHIVE_TAR;
        }

        if (instance->getStringParser().isStringExpression(sExpression)
                || instance->getMemoryManager().containsClusters(sExpression))
        {
            sExpression += " -komq";
            std::string sDummy = "";
            instance->getStringParser().evalAndFormat(sExpression, sDummy, true, false, true);
        }

        std::vector<std::string> vFileNames;

        while (sExpression.length())
        {
            vFileNames.push_back(instance->getFileSystem().ValidFileName(removeQuotationMarks(getNextArgument(sExpression)), "", false, false));
        }


        Archive::pack(vFileNames, sTargetPathName, type);
    }

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "unpack" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_unpack(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "unpack", CommandLineParser::CMD_EXPR_set_PAR);

    if (cmdParser.getExpr().length())
    {
        std::string sArchiveFileName = cmdParser.getExprAsFileName(".zip", "<loadpath>");
        std::string sTargetPathName = replacePathSeparator(cmdParser.getParameterValueAsString("target", ""));

        if (sTargetPathName.length())
            sTargetPathName = NumeReKernel::getInstance()->getFileSystem().ValidFolderName(sTargetPathName, true, false);

        std::vector<std::string> vFiles = Archive::unpack(sArchiveFileName, sTargetPathName);

        if (vFiles.size())
        {
            for (auto& file : vFiles)
            {
                file.insert(0, 1, '"');
                file += '"';
            }

            cmdParser.setReturnValue(vFiles);

            sCmd = cmdParser.getReturnValueStatement();
            return COMMAND_HAS_RETURNVALUE;
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
    plugin_random(sCmd);

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
            NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
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
    CommandLineParser cmdParser(sCmd, "resample", CommandLineParser::CMD_DAT_PAR);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!cmdParser.exprContainsDataObjects())
        return COMMAND_PROCESSED;

    DataAccessParser _access = cmdParser.getExprAsDataObject();

    if (_access.getDataObject().length())
    {
        auto vParVal = cmdParser.getParameterValueAsNumericalValue("samples");

        std::pair<size_t, size_t> samples;

        if (vParVal.size() > 1)
        {
            samples.first = intCast(vParVal[0]);
            samples.second = intCast(vParVal[1]);
        }
        else if (vParVal.size() == 1)
        {
            samples.first = intCast(vParVal.front());
            samples.second = intCast(vParVal.front());
        }
        else
        {
            samples.first = _data.getLines(_access.getDataObject(), false);
            samples.second = _data.getCols(_access.getDataObject(), false);
        }

        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject() + "(", _access.getIndexString());

        _access.evalIndices();

        if (!_data.isValueLike(_access.getIndices().col, _access.getDataObject()))
            throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _access.getDataObject() + "(", _access.getDataObject());

        MemoryManager::AppDir dir = MemoryManager::ALL;

        if (cmdParser.hasParam("grid"))
            dir = MemoryManager::GRID;
        else if (cmdParser.hasParam("cols"))
            dir = MemoryManager::COLS;
        else if (cmdParser.hasParam("lines"))
            dir = MemoryManager::LINES;

        std::string sFilter = "lanczos3";

        if (cmdParser.hasParam("method"))
            sFilter = cmdParser.getParameterValue("method");

        if (_data.resample(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, samples, dir, sFilter))
        {
            if (_option.systemPrints())
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_LINES")), _option) );
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
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
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    CommandLineParser cmdParser(sCmd, "remove", CommandLineParser::CMD_DAT_PAR);

    std::string sReturnStatement;

    if (cmdParser.exprContainsDataObjects())
    {
        std::string sTableList = cmdParser.getExpr();

        while (_data.containsTables(sTableList) && sTableList.length())
        {
            std::string sTable = getNextArgument(sTableList, true);

            for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
            {
                size_t nPos = sTable.find(iter->first + "()");

                if (nPos != string::npos && (!nPos || isDelimiter(sTable[nPos - 1])) && iter->first != "table")
                {
                    sTable = iter->first;

                    if (_data.deleteTable(iter->first))
                    {
                        if (sReturnStatement.length())
                            sReturnStatement += ", ";

                        sReturnStatement += "\"" + sTable + "()\"";
                        break;
                    }
                }
            }
        }

        if (sReturnStatement.length() && _option.systemPrints())
            NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_REMOVECACHE", sReturnStatement));
    }
    else if (cmdParser.getExpr().length())
    {
        if (!removeFile(cmdParser))
            throw SyntaxError(SyntaxError::CANNOT_REMOVE_FILE, sCmd, SyntaxError::invalid_position, cmdParser.parseExprAsString());
        else if (_option.systemPrints())
        {
            if (cmdParser.hasParam("all") || cmdParser.hasParam("a"))
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

        if (_option.systemPrints())
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

        if (_option.systemPrints())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sCmd), _option) );
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
        NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED", sCmd));

    DataAccessParser _access(sCmd, false);

    if (_access.getDataObject().length())
    {
        if (!isValidIndexSet(_access.getIndices()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, _access.getDataObject() + "(", _access.getIndexString());

        if (_access.getIndices().row.isOpenEnd())
            _access.getIndices().row.setRange(0, _data.getLines(_access.getDataObject(), false) - 1);

        if (_access.getIndices().col.isOpenEnd())
            _access.getIndices().col.setRange(0, _data.getCols(_access.getDataObject()) - 1);

        if (!_data.isValueLike(_access.getIndices().col, _access.getDataObject()))
            throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, sCmd, _access.getDataObject() + "(", _access.getDataObject());

        MemoryManager::AppDir dir = MemoryManager::ALL;

        if (findParameter(sCmd, "grid"))
            dir = MemoryManager::GRID;
        else if (findParameter(sCmd, "lines"))
            dir = MemoryManager::LINES;
        else if (findParameter(sCmd, "cols"))
            dir = MemoryManager::COLS;

        if (_data.retouch(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col, dir))
        {
            if (_option.systemPrints())
                NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_COLS")), _option) );
        }
        else
            throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, _access.getDataObject(), _access.getDataObject());
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
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    CommandLineParser cmdParser(sCmd, "regularize", CommandLineParser::CMD_DAT_PAR);

    if (!regularizeDataSet(cmdParser))
        throw SyntaxError(SyntaxError::CANNOT_REGULARIZE_CACHE, sCmd, SyntaxError::invalid_position);
    else if (_option.systemPrints())
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
                NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
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
/// "datagrid" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_datagrid(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "datagrid", CommandLineParser::CMD_EXPR_set_PAR);
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!createDatagrid(cmdParser))
        doc_Help("datagrid", _option);
    else if (_option.systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DATAGRID_SUCCESS", cmdParser.getReturnValueStatement()));

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "detect" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_detect(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "detect", CommandLineParser::CMD_EXPR_set_PAR);
    boneDetection(cmdParser);

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

    if (findSettingOption(sCmd, "files"))
        listFiles(sCmd, _option);
    else if (findSettingOption(sCmd, "var"))
        listDeclaredVariables(_parser, _option, _data);
    else if (findSettingOption(sCmd, "const"))
        listConstants(_parser, _option);
    else if (findSettingOption(sCmd, "func"))
    {
        sArgument = getArgAtPos(sCmd, findSettingOption(sCmd, "func"));

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
    else if (findSettingOption(sCmd, "logic"))
        listLogicalOperators(_option);
    else if (findSettingOption(sCmd, "cmd"))
        listCommands(_option);
    else if (findSettingOption(sCmd, "define"))
        listDefinitions(_functions, _option);
    else if (findSettingOption(sCmd, "units"))
        listUnitConversions(_option);
    else if (findSettingOption(sCmd, "plugins") || findSettingOption(sCmd, "packages"))
        listInstalledPlugins(_parser, _data, _option);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief Simple handler function for cmd_load.
///
/// \param sCmd const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getTargetTable(const std::string& sCmd)
{
    std::string sTargetTable;

    if (findParameter(sCmd, "totable", '='))
        sTargetTable = getArgAtPos(sCmd, findParameter(sCmd, "totable", '=') + 7);
    else if (findParameter(sCmd, "tocache", '='))
        sTargetTable = getArgAtPos(sCmd, findParameter(sCmd, "tocache", '=') + 7);
    else if (findParameter(sCmd, "target", '='))
        sTargetTable = getArgAtPos(sCmd, findParameter(sCmd, "target", '=') + 6);

    if (sTargetTable.find('(') != std::string::npos)
        return sTargetTable.substr(0, sTargetTable.find('('));

    return sTargetTable;
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

    CommandLineParser cmdParser(sCmd, "load", CommandLineParser::CMD_DAT_PAR);

    int nArgument;

    if (findParameter(sCmd, "define"))
    {
        if (fileExists("functions.def"))
            _functions.load(_option);
        else
            NumeReKernel::print( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY") );
    }
    else if (cmdParser.getExpr().length())
    {
        if (cmdParser.hasParam("app"))
        {
            double j1 = _data.getCols("data") + 1;
            append_data(cmdParser);
            cmdParser.setReturnValue(std::vector<mu::value_type>({1, _data.getLines("data"), j1, _data.getCols("data")}));
            sCmd = cmdParser.getReturnValueStatement();

            return COMMAND_HAS_RETURNVALUE;
        }

        std::string sFileName = cmdParser.parseExprAsString();

        if (sFileName.length())
        {
            std::string sSlicingParam = cmdParser.getParameterValue("slice");
            std::string sFileFormat;

            if (sSlicingParam == "xz")
                nArgument = -1;
            else if (sSlicingParam == "yz")
                nArgument = -2;
            else
                nArgument = 0;

            if (cmdParser.hasParam("fileformat"))
                sFileFormat = cmdParser.getParameterValueAsString("fileformat", "", true, true);

            _data.setbLoadEmptyColsInNextFile(cmdParser.hasParam("keepdim") || cmdParser.hasParam("complete"));

            if ((cmdParser.hasParam("tocache") || cmdParser.hasParam("totable") || cmdParser.hasParam("target"))
                    && !cmdParser.hasParam("all"))
            {
                // Single file directly to cache
                std::string sTargetTable = getTargetTable(cmdParser.getParameterList());

                NumeRe::FileHeaderInfo info = _data.openFile(sFileName, true, cmdParser.hasParam("ignore") || cmdParser.hasParam("i"),
                                                             nArgument, sTargetTable, sFileFormat);

                if (!_data.isEmpty(info.sTableName))
                {
                    if (_option.systemPrints())
                        NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", info.sTableName + "()", toString(_data.getLines(info.sTableName, false)), toString(_data.getCols(info.sTableName, false))));

                    cmdParser.setReturnValue(std::vector<mu::value_type>({1, info.nRows, _data.getCols(info.sTableName) - info.nCols + 1, _data.getCols(info.sTableName)}));
                    sCmd = cmdParser.getReturnValueStatement();

                    return COMMAND_HAS_RETURNVALUE;
                }

                return COMMAND_PROCESSED;
            }
            else if ((cmdParser.hasParam("tocache") || cmdParser.hasParam("totable") || cmdParser.hasParam("target"))
                     && cmdParser.hasParam("all")
                     && (sFileName.find('*') != string::npos || sFileName.find('?') != string::npos))
            {
                // multiple files directly to cache
                if (sFileName.find('/') == string::npos)
                    sFileName = "<loadpath>/" + sFileName;

                std::vector<std::string> vFilelist = NumeReKernel::getInstance()->getFileSystem().getFileList(sFileName, FileSystem::FULLPATH);

                if (!vFilelist.size())
                    throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sFileName, sFileName);

                for (size_t i = 0; i < vFilelist.size(); i++)
                    vFilelist[i] = _data.openFile(vFilelist[i], true, cmdParser.hasParam("ignore") || cmdParser.hasParam("i"), nArgument,
                                                  getTargetTable(cmdParser.getParameterList()), sFileFormat).sTableName;

                if (!_data.isEmpty(vFilelist.front()) && _option.systemPrints())
                    NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString(vFilelist.size()), sFileName));

                // Returning of indices not possible due to multiple
                // table targets

                return COMMAND_PROCESSED;
            }

            if (cmdParser.hasParam("i") || cmdParser.hasParam("ignore") || _data.isEmpty("data"))
            {
                if (!_data.isEmpty("data"))
                    _data.removeData();

                // multiple files
                if (cmdParser.hasParam("all") && (sFileName.find('*') != string::npos || sFileName.find('?') != string::npos))
                {
                    if (sFileName.find('/') == string::npos)
                        sFileName = "<loadpath>/" + sFileName;

                    std::vector<std::string> vFilelist = NumeReKernel::getInstance()->getFileSystem().getFileList(sFileName, FileSystem::FULLPATH);

                    if (!vFilelist.size())
                        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sFileName, sFileName);

                    for (size_t i = 0; i < vFilelist.size(); i++)
                    {
                        // Melting is done automatically
                        _data.openFile(vFilelist[i], false, false, nArgument, "", sFileFormat);
                    }

                    if (!_data.isEmpty("data") && _option.systemPrints())
                        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString(vFilelist.size()), sFileName, toString(_data.getLines("data", false)), toString(_data.getCols("data", false))));

                    cmdParser.setReturnValue(std::vector<mu::value_type>({1, _data.getLines("data", false), 1, _data.getCols("data", false)}));
                    sCmd = cmdParser.getReturnValueStatement();

                    return COMMAND_HAS_RETURNVALUE;
                }

                NumeRe::FileHeaderInfo info;

                // Provide headline
                auto vParList = cmdParser.getParameterValueAsNumericalValue("head");

                if (vParList.size())
                    nArgument = intCast(vParList.front());
                else
                {
                    vParList = cmdParser.getParameterValueAsNumericalValue("h");

                    if (vParList.size())
                        nArgument = intCast(vParList.front());
                }

                info = _data.openFile(sFileName, false, false, nArgument, "", sFileFormat);

                if (!_data.isEmpty("data"))
                {
                    if (_option.systemPrints())
                        NumeReKernel::print(_lang.get("BUILTIN_LOADDATA_SUCCESS", info.sFileName, toString(info.nRows), toString(info.nCols)));

                    cmdParser.setReturnValue(std::vector<mu::value_type>({1, _data.getLines("data", false), 1, _data.getCols("data", false)}));
                    sCmd = cmdParser.getReturnValueStatement();

                    return COMMAND_HAS_RETURNVALUE;
                }
            }
            else
                load_data(_data, _option, _parser, sFileName, sFileFormat);
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

    string sArgument;
    auto _filenames = getAllSemiColonSeparatedTokens(_data.getDataFileName("data"));
    Match _mMatch = findCommand(sCmd, "reload");

    if (!_filenames.size())
        return COMMAND_PROCESSED;

    _data.removeData();

    for (size_t i = 0; i < _filenames.size(); i++)
    {
        if (sCmd.find_first_not_of(' ', _mMatch.nPos + 6) != string::npos)
        {
            // Seems not to contain any valid file name
            if (sCmd[sCmd.find_first_not_of(' ', _mMatch.nPos + 6)] == '-')
                sArgument = "load " + _filenames[i] + " " + sCmd.substr(sCmd.find_first_not_of(' ', _mMatch.nPos + 6)) + " -app";
            else
                sArgument = sCmd.substr(_mMatch.nPos + 2) + " -app";
        }
        else
            sArgument = "load " + _filenames[i] + " -app";

        cmd_load(sArgument);
    }

    _parser.SetVectorVar("_~load[~_~]", {1, _data.getLines("data", false), 1, _data.getCols("data", false)});
    sCmd.replace(_mMatch.nPos, string::npos, "_~load[~_~]");

    return COMMAND_HAS_RETURNVALUE;
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
/// "progress" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_progress(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "progress", CommandLineParser::CMD_EXPR_set_PAR);

    if (!cmdParser.getExpr().length())
        return COMMAND_PROCESSED;

    string sArgument;
    int frst = 1, lst = 100;

    auto vParVal = cmdParser.getParameterValueAsNumericalValue("first");

    if (vParVal.size())
        frst = intCast(vParVal.front());

    vParVal = cmdParser.getParameterValueAsNumericalValue("last");

    if (vParVal.size())
        lst = intCast(vParVal.front());

    sArgument = cmdParser.getParameterValue("type");

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sArgument))
        sArgument = cmdParser.getParameterValueAsString("type", "std");

    auto vVal = cmdParser.parseExprAsNumericalValues();

    if (vVal.size())
        make_progressBar(intCast(vVal.front()), frst, lst, sArgument);

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
    string sArgument = sCmd.substr(findCommand(sCmd).nPos + 6);
    string sDummy;

    if (!NumeReKernel::getInstance()->getDefinitions().call(sArgument))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sArgument);

    if (sArgument.find("??") != string::npos)
        sArgument = promptForUserInput(sArgument);

    sArgument += " -print";

    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sArgument, sDummy, false, false, true);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements all
/// "*rot" commands.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_rotate(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_EXPR_set_PAR);
    rotateTable(cmdParser);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "url" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_url(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "url", CommandLineParser::CMD_DAT_PAR);
    urlExecute(cmdParser);
    sCmd = cmdParser.getReturnValueStatement();

    return COMMAND_HAS_RETURNVALUE;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "qrcode" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_qrcode(std::string& sCmd)
{
    CommandLineParser cmdParser(sCmd, "qrcode", CommandLineParser::CMD_EXPR_set_PAR);
    createQrCode(cmdParser);

    return COMMAND_PROCESSED;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "include" command.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_include(string& sCmd)
{
    // Command is not usable in this context
    throw SyntaxError(SyntaxError::INVALID_COMMAND, sCmd, "include", "include");
}


/////////////////////////////////////////////////
/// \brief This static function catches context
/// specific commands and throws an error.
///
/// \param sCmd string&
/// \return CommandReturnValues
///
/////////////////////////////////////////////////
static CommandReturnValues cmd_context_specific(string& sCmd)
{
    CommandLineParser cmdParser(sCmd, CommandLineParser::CMD_EXPR_set_PAR);
    // Command is not usable in this context
    throw SyntaxError(SyntaxError::INVALID_COMMAND, sCmd, cmdParser.getCommand(), cmdParser.getCommand());
}


/////////////////////////////////////////////////
/// \brief This static function returns a map of
/// commands linked to their function
/// implementation.
///
/// \return std::map<std::string,CommandFunc>
///
/////////////////////////////////////////////////
static std::map<std::string, CommandFunc> getCommandFunctions()
{
    std::map<std::string, CommandFunc> mCommandFuncMap;

    mCommandFuncMap["about"] = cmd_credits;
    mCommandFuncMap["audio"] = cmd_audio;
    mCommandFuncMap["clc"] = cmd_clc;
    mCommandFuncMap["clear"] = cmd_clear;
    mCommandFuncMap["close"] = cmd_close;
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
    mCommandFuncMap["detect"] = cmd_detect;
    mCommandFuncMap["edit"] = cmd_edit;
    mCommandFuncMap["execute"] = cmd_execute;
    mCommandFuncMap["export"] = saveDataObject;
    mCommandFuncMap["fit"] = cmd_fit;
    mCommandFuncMap["fitw"] = cmd_fit;
    mCommandFuncMap["fft"] = cmd_fft;
    mCommandFuncMap["fft2d"] = cmd_fft;
    mCommandFuncMap["fwt"] = cmd_fwt;
    mCommandFuncMap["grad"] = cmd_plotting;
    mCommandFuncMap["grad3d"] = cmd_plotting;
    mCommandFuncMap["gradient"] = cmd_plotting;
    mCommandFuncMap["gradient3d"] = cmd_plotting;
    mCommandFuncMap["graph"] = cmd_plotting;
    mCommandFuncMap["graph3d"] = cmd_plotting;
    mCommandFuncMap["hist"] = cmd_hist;
    mCommandFuncMap["hist2d"] = cmd_hist;
    mCommandFuncMap["hline"] = cmd_hline;
    mCommandFuncMap["ifndef"] = cmd_ifndefined;
    mCommandFuncMap["ifndefined"] = cmd_ifndefined;
    mCommandFuncMap["implot"] = cmd_plotting;
    mCommandFuncMap["include"] = cmd_include;
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
    mCommandFuncMap["pack"] = cmd_pack;
    mCommandFuncMap["plot"] = cmd_plotting;
    mCommandFuncMap["plot3d"] = cmd_plotting;
    mCommandFuncMap["plotcompose"] = cmd_plotting;
    mCommandFuncMap["print"] = cmd_print;
    mCommandFuncMap["progress"] = cmd_progress;
    mCommandFuncMap["qrcode"] = cmd_qrcode;
    mCommandFuncMap["quit"] = cmd_quit;
    mCommandFuncMap["random"] = cmd_random;
    mCommandFuncMap["redef"] = cmd_redefine;
    mCommandFuncMap["redefine"] = cmd_redefine;
    mCommandFuncMap["regularize"] = cmd_regularize;
    mCommandFuncMap["remove"] = cmd_remove;
    mCommandFuncMap["rename"] = cmd_rename;
    mCommandFuncMap["resample"] = cmd_resample;
    mCommandFuncMap["retoque"] = cmd_retouch;
    mCommandFuncMap["retouch"] = cmd_retouch;
    mCommandFuncMap["save"] = cmd_save;
    mCommandFuncMap["set"] = cmd_set;
    mCommandFuncMap["show"] = cmd_show;
    mCommandFuncMap["showf"] = cmd_show;
    mCommandFuncMap["smooth"] = cmd_smooth;
    mCommandFuncMap["spline"] = cmd_spline;
    mCommandFuncMap["start"] = cmd_start;
    mCommandFuncMap["stfa"] = cmd_stfa;
    mCommandFuncMap["surf"] = cmd_plotting;
    mCommandFuncMap["surf3d"] = cmd_plotting;
    mCommandFuncMap["surface"] = cmd_plotting;
    mCommandFuncMap["surface3d"] = cmd_plotting;
    mCommandFuncMap["swap"] = cmd_swap;
    mCommandFuncMap["tabrot"] = cmd_rotate;
    mCommandFuncMap["imrot"] = cmd_rotate;
    mCommandFuncMap["gridrot"] = cmd_rotate;
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

    // Invalid, i.e. context specific commands, which cannot and shall
    // not appear here
    mCommandFuncMap["tab"] = cmd_context_specific;
    mCommandFuncMap["var"] = cmd_context_specific;
    mCommandFuncMap["str"] = cmd_context_specific;
    mCommandFuncMap["cst"] = cmd_context_specific;
    mCommandFuncMap["namespace"] = cmd_context_specific;
    mCommandFuncMap["layout"] = cmd_context_specific;
    mCommandFuncMap["endlayout"] = cmd_context_specific;
    mCommandFuncMap["group"] = cmd_context_specific;
    mCommandFuncMap["endgroup"] = cmd_context_specific;
    mCommandFuncMap["break"] = cmd_context_specific;
    mCommandFuncMap["continue"] = cmd_context_specific;
    mCommandFuncMap["leave"] = cmd_context_specific;
    mCommandFuncMap["else"] = cmd_context_specific;
    mCommandFuncMap["elseif"] = cmd_context_specific;
    mCommandFuncMap["endif"] = cmd_context_specific;
    mCommandFuncMap["endfor"] = cmd_context_specific;
    mCommandFuncMap["endwhile"] = cmd_context_specific;
    mCommandFuncMap["case"] = cmd_context_specific;
    mCommandFuncMap["default"] = cmd_context_specific;
    mCommandFuncMap["endswitch"] = cmd_context_specific;
    mCommandFuncMap["catch"] = cmd_context_specific;
    mCommandFuncMap["endtry"] = cmd_context_specific;
    mCommandFuncMap["explicit"] = cmd_context_specific;
    mCommandFuncMap["throw"] = cmd_context_specific;
    mCommandFuncMap["rethrow"] = cmd_context_specific;
    mCommandFuncMap["button"] = cmd_context_specific;
    mCommandFuncMap["statictext"] = cmd_context_specific;
    mCommandFuncMap["checkbox"] = cmd_context_specific;
    mCommandFuncMap["radio"] = cmd_context_specific;
    mCommandFuncMap["textfield"] = cmd_context_specific;
    mCommandFuncMap["dropdown"] = cmd_context_specific;
    mCommandFuncMap["combobox"] = cmd_context_specific;
    mCommandFuncMap["gauge"] = cmd_context_specific;
    mCommandFuncMap["bitmap"] = cmd_context_specific;
    mCommandFuncMap["spinbut"] = cmd_context_specific;
    mCommandFuncMap["slider"] = cmd_context_specific;
    mCommandFuncMap["tablegrid"] = cmd_context_specific;
    mCommandFuncMap["grapher"] = cmd_context_specific;
    mCommandFuncMap["treelist"] = cmd_context_specific;
    mCommandFuncMap["menuitem"] = cmd_context_specific;
    mCommandFuncMap["separator"] = cmd_context_specific;
    mCommandFuncMap["prop"] = cmd_context_specific;

    return mCommandFuncMap;
}


/////////////////////////////////////////////////
/// \brief This static function returns a map of
/// commands with return values linked to their
/// function implementation.
///
/// \return std::map<std::string,CommandFunc>
///
/////////////////////////////////////////////////
static std::map<std::string, CommandFunc> getCommandFunctionsWithReturnValues()
{
    std::map<std::string, CommandFunc> mCommandFuncMap;

    mCommandFuncMap["append"] = cmd_append;
    mCommandFuncMap["audioread"] = cmd_audioread;
    mCommandFuncMap["dialog"] = cmd_dialog;
    mCommandFuncMap["diff"] = cmd_diff;
    mCommandFuncMap["eval"] = cmd_eval;
    mCommandFuncMap["extrema"] = cmd_extrema;
    mCommandFuncMap["imread"] = cmd_imread;
    mCommandFuncMap["integrate"] = cmd_integrate;
    mCommandFuncMap["integrate2d"] = cmd_integrate;
    mCommandFuncMap["load"] = cmd_load;
    mCommandFuncMap["pso"] = cmd_pso;
    mCommandFuncMap["pulse"] = cmd_pulse;
    mCommandFuncMap["read"] = cmd_read;
    mCommandFuncMap["readline"] = cmd_readline;
    mCommandFuncMap["reload"] = cmd_reload;
    mCommandFuncMap["seek"] = cmd_seek;
    mCommandFuncMap["sort"] = cmd_sort;
    mCommandFuncMap["stats"] = cmd_stats;
    mCommandFuncMap["taylor"] = cmd_taylor;
    mCommandFuncMap["unpack"] = cmd_unpack;
    mCommandFuncMap["url"] = cmd_url;
    mCommandFuncMap["window"] = cmd_window;
    mCommandFuncMap["zeroes"] = cmd_zeroes;

    return mCommandFuncMap;
}










#endif // COMMANDFUNCTIONS_HPP
