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





// CLASS Plugin


#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include <iostream>
#include <fstream>
#include <string>

#include "filesystem.hpp"
#include "utils/tools.hpp"
#include "ui/error.hpp"

using namespace std;

class Plugin : public FileSystem
{
    private:
        fstream fPlugins;
        vector<vector<string> > vPluginInfo;
        unsigned int nPlugins;
        string sPlugins;

        string sPluginProcName;
        string sPluginVarList;
        void assign(const Plugin& _plugin);
        void updatePluginFile();

    public:
        Plugin();
        Plugin(const Plugin& _plugin);
        ~Plugin();
        Plugin& operator= (const Plugin& _plugin);

        bool loadPlugins();
        bool evalPluginCmd(string& sCmd);
        bool declareNewPlugin(const string& sInstallInfoString);
        bool isPluginCmd(const string& sCmd) const;
        string deletePlugin(const string& sPlugin);

        inline unsigned int getPluginCount() const
            {return nPlugins;}

        string getPluginInfoPath();
        void addHelpIndex(const string& _sPluginName, string _sHelpId);
        inline string getPluginNames() const
            {
                string sReturn = ";";
                for (unsigned int i = 0; i < vPluginInfo.size(); i++)
                {
                    sReturn += vPluginInfo[i][0] + ";";
                }
                return sReturn;
            }
        inline string getPluginProcName() const
            {return sPluginProcName;}
        inline string getPluginVarList() const
            {return sPluginVarList;}
        inline string getPluginCommand(unsigned int i = 0) const
            {
                if (i < nPlugins)
                {
                    return vPluginInfo[i][0];
                }
                return "";
            }
        inline string getPluginName(unsigned int i = 0) const
            {
                if (i < nPlugins)
                {
                    if (vPluginInfo[i][4][0] == '(' && vPluginInfo[i][4][vPluginInfo[i][4].length()-1] == ')')
                        return vPluginInfo[i][4].substr(1,vPluginInfo[i][4].length()-2);
                    return vPluginInfo[i][4];
                }
                return "";
            }
        inline string getPluginVersion(unsigned int i = 0) const
            {
                if (i < nPlugins)
                {
                    if (vPluginInfo[i][5][0] == '(' && vPluginInfo[i][5][vPluginInfo[i][5].length()-1] == ')')
                        return vPluginInfo[i][5].substr(1,vPluginInfo[i][5].length()-2);
                    return vPluginInfo[i][5];
                }
                return "";
            }
        inline string getPluginAuthor(unsigned int i = 0) const
            {
                if (i < nPlugins)
                {
                    if (vPluginInfo[i][6][0] == '(' && vPluginInfo[i][6][vPluginInfo[i][6].length()-1] == ')')
                        return vPluginInfo[i][6].substr(1,vPluginInfo[i][6].length()-2);
                    return vPluginInfo[i][6];
                }
                return "";
            }
        inline string getPluginDesc(unsigned int i = 0) const
            {
                if (i < nPlugins)
                {
                    if (vPluginInfo[i][7][0] == '(' && vPluginInfo[i][7][vPluginInfo[i][7].length()-1] == ')')
                        return vPluginInfo[i][7].substr(1,vPluginInfo[i][7].length()-2);
                    return vPluginInfo[i][7];
                }
                return "";
            }
};

#endif

