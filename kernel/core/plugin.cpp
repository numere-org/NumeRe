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



#include "plugin.hpp"


Plugin::Plugin() : FileSystem()
{
    nPlugins = 0;
    sPlugins = "<>/numere.plugins";
    sPluginProcName = "";
    sPluginVarList = "";
}

Plugin::Plugin(const Plugin& _plugin) : Plugin()
{
    assign(_plugin);
}

void Plugin::assign(const Plugin& _plugin)
{
    nPlugins = _plugin.nPlugins;
    if (nPlugins)
    {
        vPluginInfo.resize(nPlugins, vector<string>(8,""));
        for (unsigned int i = 0; i < nPlugins; i++)
        {
            for (unsigned int j = 0; j < 8; j++)
                vPluginInfo[i][j] = _plugin.vPluginInfo[i][j];
        }
    }
    return;
}

Plugin::~Plugin()
{
    if (fPlugins.is_open())
        fPlugins.close();
}

Plugin& Plugin::operator=(const Plugin& _plugin)
{
    assign(_plugin);
    return *this;
}

void Plugin::updatePluginFile()
{
    if (sPlugins.find("<>/") != string::npos)
        sPlugins = FileSystem::ValidFileName(sPlugins, ".plugins");

    fPlugins.open(sPlugins.c_str(), ios_base::trunc | ios_base::out);
    if (fPlugins.fail())
    {
        //sErrorToken = sPlugins;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sPlugins);
    }
    for (unsigned int i = 0; i < nPlugins; i++)
    {
        for (unsigned int j = 0; j < vPluginInfo[i].size(); j++)
            fPlugins << vPluginInfo[i][j] << ",";
        fPlugins << endl;
    }
    fPlugins.close();
    return;
}

bool Plugin::loadPlugins()
{
    string sLine = "";
    vector<string> vLine;
    //vLine.resize(10,"");
    if (sPlugins.find("<>/") != string::npos)
        sPlugins = FileSystem::ValidFileName(sPlugins, ".plugins");

    fPlugins.open(sPlugins.c_str());
    if (fPlugins.fail())
    {
        //sErrorToken = sPlugins;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sPlugins);
    }

    while (!fPlugins.eof())
    {
        getline(fPlugins, sLine);
        StripSpaces(sLine);
        if (sLine.length())
        {
            while (sLine.length())
            {
                if (getNextArgument(sLine, false).length())
                    vLine.push_back(getNextArgument(sLine, true));
                else
                    break;
            }
            vPluginInfo.push_back(vLine);
            nPlugins++;
            vLine.clear();
        }
    }

    fPlugins.close();

    return true;
}

bool Plugin::evalPluginCmd(string& sCmd)
{
    string sExpr = "";
    string sParams = "";
    string sCommand = "";
    string sCommandLine = "";

    unsigned int nPos = 0;
    unsigned int nthPlugin = string::npos;
    if (!nPlugins)
        return false;

    for (unsigned int i = 0; i < nPlugins; i++)
    {
        if (findCommand(sCmd, vPluginInfo[i][0]).sString == vPluginInfo[i][0])
        {
            nthPlugin = i;
            break;
        }
    }
    if (nthPlugin == string::npos)
        return false;

    nPos = findCommand(sCmd, vPluginInfo[nthPlugin][0]).nPos;
    sCommandLine = extractCommandString(sCmd, findCommand(sCmd, vPluginInfo[nthPlugin][0]));

    sPluginProcName = vPluginInfo[nthPlugin][1];
    sPluginVarList = vPluginInfo[nthPlugin][2];

    if (sPluginVarList[0] == '(' && sPluginVarList[sPluginVarList.length()-1] == ')')
        sPluginVarList = sPluginVarList.substr(1,sPluginVarList.length()-2);

    if (sPluginVarList.find("<EXPRESSION>") != string::npos)
    {
        if (sCommandLine.find("-set") != string::npos || sCommandLine.find("--") != string::npos)
        {
            if (sCommandLine.find("-set") != string::npos)
                sParams = sCommandLine.substr(sCommandLine.find("-set"));
            else
                sParams = sCommandLine.substr(sCommandLine.find("--"));
        }
        if (sParams.length())
        {
            sExpr = sCommandLine.substr(sCommandLine.find(vPluginInfo[nthPlugin][0])+vPluginInfo[nthPlugin][0].length());
            sExpr.erase(sExpr.find(sParams));
            StripSpaces(sParams);
        }
        else
            sExpr = sCommandLine.substr(sCommandLine.find(vPluginInfo[nthPlugin][0])+vPluginInfo[nthPlugin][0].length());
        StripSpaces(sExpr);
        sParams = "\"" + sParams + "\"";
    }
    else if (sPluginVarList.find("<PARAMSTRING>") != string::npos)
    {
        if (sCommandLine.find('-') != string::npos)
            sParams = sCommandLine.substr(sCommandLine.find('-', sCommandLine.find(vPluginInfo[nthPlugin][0])));
        StripSpaces(sParams);
        sParams = "\"" + sParams + "\"";
    }
    sCommand = "\"" + sCommandLine + "\"";
    for (unsigned int i = 1; i < sCommandLine.length()-1; i++)
    {
        if (sCommandLine[i] == '"' && sCommandLine[i-1] != '\\')
            sCommandLine.insert(i,1,'\\');
    }
    for (unsigned int i = 1; i < sParams.length()-1; i++)
    {
        if (sParams[i] == '"' && sParams[i-1] != '\\')
            sParams.insert(i,1,'\\');
    }

    while (sPluginVarList.find("<CMDSTRING>") != string::npos)
        sPluginVarList.replace(sPluginVarList.find("<CMDSTRING>"),11,sCommand);
    while (sPluginVarList.find("<EXPRESSION>") != string::npos)
        sPluginVarList.replace(sPluginVarList.find("<EXPRESSION>"),12,sExpr);
    while (sPluginVarList.find("<PARAMSTRING>") != string::npos)
        sPluginVarList.replace(sPluginVarList.find("<PARAMSTRING>"),13,sParams);

    if (vPluginInfo[nthPlugin][3].find("TYPE_PLUGIN_WITH_RETURN_VALUE") != string::npos)
    {
        sCmd.replace(nPos, sCommandLine.length(), "<<RETURNVAL>>");
    }
    else
        sCmd.clear();

    return true;
}

bool Plugin::declareNewPlugin(const string& sInstallInfoString)
{
    string sProtectedCommands = ";quit;help;find;uninstall;install;credits;about;continue;break;var;tab;global;throw;namespace;return;abort;explicit;str;if;else;elseif;endif;while;endwhile;for;endfor;switch;case;default;endswitch;";
    vector<string> vPlugin;
    vector<string> vParams = {"plugincommand", "pluginmain", "pluginmain", "type", "name", "version", "author", "plugindesc"};
    bool bAllowOverride = false;
    vPlugin.resize(8,"");

    if (findParameter(sInstallInfoString, "flags", '='))
    {
        if (getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, "flags", '=')+5).find("ENABLE_FORCE_OVERRIDE") != string::npos)
            bAllowOverride = true;
    }

    for (unsigned int i = 0; i < 8; i++)
    {
        if (findParameter(sInstallInfoString, vParams[i], '='))
        {
            vPlugin[i] = getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, vParams[i], '=')+vParams[i].length());
            StripSpaces(vPlugin[i]);
            if (i > 3 && (vPlugin[i].find(' ') != string::npos || vPlugin[i].find(',') != string::npos))
            {
                vPlugin[i] = "(" + vPlugin[i] + ")";
            }
        }
        if (vPlugin[i] == "<AUTO>" || !vPlugin[i].length())
        {
            if (i == 0)
                throw SyntaxError(SyntaxError::PLUGIN_HAS_NO_CMD, "", SyntaxError::invalid_position);
            if (i == 1)
                throw SyntaxError(SyntaxError::PLUGIN_HAS_NO_MAIN, "", SyntaxError::invalid_position);
            if (i == 4)
                vPlugin[i] = "Plugin";
            if (i == 5)
                vPlugin[i] = "<VERSIONING>";
            if (i == 6)
                vPlugin[i] = "Benutzer";
        }
    }

    if (sProtectedCommands.find(";" + vPlugin[0] + ";") != string::npos)
    {
        //sErrorToken = vPlugin[0];
        throw SyntaxError(SyntaxError::PLUGIN_MAY_NOT_OVERRIDE, "", SyntaxError::invalid_position, vPlugin[0]);
    }

    if (vPlugin[1].find('$') == string::npos || vPlugin[1].find('(') == string::npos)
        return false;
    vPlugin[1] = vPlugin[1].substr(1,vPlugin[1].find('(')-1);
    vPlugin[2] = vPlugin[2].substr(vPlugin[2].find('('));

    if (nPlugins)
    {
        for (unsigned int i = 0; i < nPlugins; i++)
        {
            if (vPluginInfo[i][0] == vPlugin[0])
            {
                if ((vPluginInfo[i][4] == vPlugin[4] && vPluginInfo[i][6] == vPlugin[6]) || bAllowOverride)
                {
                    for (unsigned int j = i+1; j < nPlugins; j++)
                    {
                        if (vPluginInfo[j][4] == vPlugin[4] && (vPluginInfo[j][0] != vPlugin[0] || vPluginInfo[j][6] != vPlugin[6]))
                        {
                            string sErrorToken;
                            if (vPlugin[4][0] == '(' && vPlugin[4][vPlugin[4].length()-1] == ')')
                                sErrorToken = vPlugin[4].substr(1, vPlugin[4].length()-2);
                            else
                                sErrorToken = vPlugin[4];
                            throw SyntaxError(SyntaxError::PLUGINNAME_ALREADY_EXISTS, "", SyntaxError::invalid_position, sErrorToken);
                        }
                    }
                    for (unsigned int j = 0; j < 8; j++)
                    {
                        if (j == 5 && vPlugin[j] == "<VERSIONING>" && vPluginInfo[i][j].length())
                        {
                            string sTempVersion = vPluginInfo[i][j];
                            for (unsigned int n = 0; n < sTempVersion.length(); n++)
                            {
                                if (sTempVersion[n] == '.')
                                {
                                    sTempVersion.erase(n,1);
                                    n--;
                                }
                            }
                            int nVersion = StrToInt(sTempVersion);
                            nVersion++;
                            sTempVersion = toString(nVersion);
                            if (sTempVersion.length() < 3)
                                sTempVersion.insert(0,3-sTempVersion.length(), '0');
                            for (unsigned int n = 1; n < sTempVersion.length(); n++)
                            {
                                if (n % 2)
                                {
                                    sTempVersion.insert(n,1,'.');
                                }
                            }
                            vPluginInfo[i][j] = sTempVersion;
                        }
                        else
                            vPluginInfo[i][j] = vPlugin[j];
                    }
                }
                else
                {
                    //sErrorToken = vPlugin[0];
                    throw SyntaxError(SyntaxError::PLUGINCMD_ALREADY_EXISTS, "", SyntaxError::invalid_position, vPlugin[0]);
                }
                break;
            }
            else if (vPluginInfo[i][4] == vPlugin[4])
            {
                if (vPluginInfo[i][0] != vPlugin[0] || vPluginInfo[i][6] != vPlugin[6])
                {
                    string sErrorToken;
                    if (vPlugin[4][0] == '(' && vPlugin[4][vPlugin[4].length()-1] == ')')
                        sErrorToken = vPlugin[4].substr(1, vPlugin[4].length()-2);
                    else
                        sErrorToken = vPlugin[4];
                    throw SyntaxError(SyntaxError::PLUGINNAME_ALREADY_EXISTS, "", SyntaxError::invalid_position, sErrorToken);
                }
            }
            if (i == nPlugins-1)
            {
                vPluginInfo.push_back(vPlugin);
                if (vPluginInfo[nPlugins][5] == "<VERSIONING>")
                    vPluginInfo[nPlugins][5] = "0.0.1";
                nPlugins++;
                break;
            }
        }
    }
    else
    {
        vPluginInfo.push_back(vPlugin);
        if (vPluginInfo[nPlugins][5] == "<VERSIONING>")
            vPluginInfo[nPlugins][5] = "0.0.1";
        nPlugins++;
    }

    updatePluginFile();

    return true;
}

void Plugin::addHelpIndex(const string& _sPluginName, string _sHelpId)
{
    StripSpaces(_sHelpId);
    string sPluginName = _sPluginName;
    StripSpaces(sPluginName);
    if (!vPluginInfo.size() || !_sHelpId.length())
        return;
    if (sPluginName.length())
    {
        for (unsigned int i = 0; i < vPluginInfo.size(); i++)
        {
            if (vPluginInfo[i][4] == sPluginName)
            {
                if (vPluginInfo[i].size() > 8)
                {
                    if (vPluginInfo[i][8] != _sHelpId
                        && vPluginInfo[i][8].find(";"+_sHelpId) == string::npos
                        && vPluginInfo[i][8].find(_sHelpId+";") == string::npos
                        && vPluginInfo[i][8].find(";"+_sHelpId+";") == string::npos)
                    vPluginInfo[i][8] += ";" + _sHelpId;
                }
                else
                    vPluginInfo[i].push_back(_sHelpId);
                updatePluginFile();
                break;
            }
        }
    }
    else
    {
        if (vPluginInfo[nPlugins-1].size() > 8)
            vPluginInfo[nPlugins-1][8] += ";" + _sHelpId;
        else
            vPluginInfo[nPlugins-1].push_back(_sHelpId);
        updatePluginFile();
    }
    return;
}

bool Plugin::isPluginCmd(const string& sCmd) const
{
    if (findCommand(sCmd, "explicit").sString == "explicit")
    {
        return false;
    }
    if (nPlugins)
    {
        for (unsigned int i = 0; i < nPlugins; i++)
        {
            if (findCommand(sCmd, vPluginInfo[i][0]).sString == vPluginInfo[i][0])
                return true;
        }
    }
    return false;
}

string Plugin::deletePlugin(const string& sPlugin)
{
    std::vector<vector<string> >::iterator vIter = vPluginInfo.begin();
    string sHLPIDs = "<<NO_HLP_ENTRY>>";
    for (; vIter != vPluginInfo.end(); ++vIter)
    {
        if ((*vIter)[4] == sPlugin || (*vIter)[4] == "("+sPlugin+")")
        {
            if ((*vIter).size() > 8)
                sHLPIDs = (*vIter)[8];
            vPluginInfo.erase(vIter);
            nPlugins--;

            updatePluginFile();

            return sHLPIDs;
        }
    }
    return "";
}

string Plugin::getPluginInfoPath()
{
    if (sPlugins.find("<>/") != string::npos)
        sPlugins = FileSystem::ValidFileName(sPlugins, ".plugins");
    return sPlugins;
}

