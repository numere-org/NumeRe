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


/////////////////////////////////////////////////
/// \brief This class implements a single
/// declared plugin. It can be constructed
/// directly from an install information string.
/// It also supports importing from and exporting
/// to the plugin definition file.
/////////////////////////////////////////////////
class Plugin
{
    private:
        string getOptionValue(const string& sInstallInfoString, const string& sOption, const string& sDefault);

    public:
        string sCommand;
        string sMainProcedure;
        string sArgumentList;
        string sType;
        string sName;
        string sVersion;
        string sAuthor;
        string sDescription;
        string sDocumentationIndexID;

        Plugin();
        Plugin(const string& sInstallInfoString);

        string exportDefinition();
        void importDefinition(string sDefinitionString);

        bool operator==(const Plugin& _plugin);
        bool operator!=(const Plugin& _plugin);
        void update(const Plugin& _plugin);
        void incrementVersion();
};


/////////////////////////////////////////////////
/// \brief This class implements the procedure
/// plugin system. It will be a parent class of
/// the procedure class.
///
/// The plugin system is represented by dedicated
/// NumeRe procedures, which one has to declare
/// during plugin installation. This class will
/// redirect the control to this dedicated "main"
/// procedure once the plugin keyword has been
/// found.
/////////////////////////////////////////////////
class PluginManager : public FileSystem
{
    private:
        fstream fPlugins;
        vector<Plugin> vPluginInfo;
        string sPluginDefinitionFile;

        string sPluginProcName;
        string sPluginVarList;
        void assign(const PluginManager& _pluginManager);
        void updatePluginFile();

        /////////////////////////////////////////////////
        /// \brief This private member function removes
        /// the surrounding parentheses, if available.
        ///
        /// \param sString const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string stripParentheses(const string& sString) const
        {
            if (sString.front() == '(' && sString.back() == ')')
                return sString.substr(1, sString.length()-2);

            return sString;
        }

    public:
        PluginManager();
        PluginManager(const PluginManager& _pluginManager);
        ~PluginManager();
        PluginManager& operator= (const PluginManager& _pluginManager);

        bool loadPlugins();
        bool evalPluginCmd(string& sCmd);
        bool declareNewPlugin(const string& sInstallInfoString);
        bool isPluginCmd(const string& sCmd) const;
        string deletePlugin(const string& sPlugin);

        /////////////////////////////////////////////////
        /// \brief Returns the number of installed
        /// plugins.
        ///
        /// \return unsigned int
        ///
        /////////////////////////////////////////////////
        inline unsigned int getPluginCount() const
            {return vPluginInfo.size();}

        string getPluginInfoPath();
        void addHelpIndex(const string& _sPluginName, string _sHelpId);

        /////////////////////////////////////////////////
        /// \brief Returns the names of the installed
        /// plugins.
        ///
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginNames() const
        {
            string sReturn = ";";

            for (unsigned int i = 0; i < vPluginInfo.size(); i++)
                sReturn += vPluginInfo[i].sName + ";";

            return sReturn;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the current plugin's procedure
        /// name.
        ///
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginProcName() const
            {return sPluginProcName;}

        /////////////////////////////////////////////////
        /// \brief Returns the current plugin's procedure
        /// argument list.
        ///
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginVarList() const
            {return sPluginVarList;}

        /////////////////////////////////////////////////
        /// \brief Returns the plugin command of the ith
        /// plugin.
        ///
        /// \param i unsigned int i
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginCommand(unsigned int i = 0) const
        {
            if (i < vPluginInfo.size())
                return vPluginInfo[i].sCommand;

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the plugin name of the ith
        /// plugin.
        ///
        /// \param i unsigned int
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginName(unsigned int i = 0) const
        {
            if (i < vPluginInfo.size())
                return stripParentheses(vPluginInfo[i].sName);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the version number string of
        /// the ith plugin.
        ///
        /// \param i unsigned int
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginVersion(unsigned int i = 0) const
        {
            if (i < vPluginInfo.size())
                return stripParentheses(vPluginInfo[i].sVersion);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the author of the ith plugin.
        ///
        /// \param i unsigned int
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginAuthor(unsigned int i = 0) const
        {
            if (i < vPluginInfo.size())
                return stripParentheses(vPluginInfo[i].sAuthor);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the description of the ith
        /// plugin.
        ///
        /// \param unsigned int i = 0
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginDesc(unsigned int i = 0) const
        {
            if (i < vPluginInfo.size())
                return stripParentheses(vPluginInfo[i].sDescription);

            return "";
        }
};

#endif

