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

#include "../io/filesystem.hpp"
#include "../ui/error.hpp"

using namespace std;


/////////////////////////////////////////////////
/// \brief This class implements a single
/// declared package. It can be constructed
/// directly from an install information string.
/// It also supports importing from and exporting
/// to the plugin definition file.
/////////////////////////////////////////////////
class Package
{
    private:
        std::string getOptionValue(const std::string& sInstallInfoString, const std::string& sOption, const std::string& sDefault);

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
        std::string sCommand;
        std::string sMainProcedure;
        std::string sArgumentList;
        std::string sType;
        std::string sLicense;
        std::string sName;
        std::string sVersion;
        std::string sAuthor;
        std::string sDescription;
        std::string sMenuEntry;
        std::string sDocumentationIndexID;

        Package();
        Package(const std::string& sInstallInfoString);

        std::string exportDefinition() const;
        void importDefinition(std::string sDefinitionString);

        bool isPlugin() const;
        bool operator==(const Package& _plugin) const;
        bool operator!=(const Package& _plugin) const;
        void update(const Package& _plugin);
        void incrementVersion();
        std::string getName() const;
        std::string getAuthor() const;
        std::string getDescription() const;
        std::string getLicense() const;
        std::string getMenuEntry() const;
        std::string getCommandSignature() const;
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
class PackageManager : public FileSystem
{
    private:
        fstream fPlugins;
        vector<Package> vPackageInfo;
        string sPluginDefinitionFile;

        string sPluginProcName;
        string sPluginVarList;
        void assign(const PackageManager& _manager);
        void updatePluginFile();

    public:
        PackageManager();
        PackageManager(const PackageManager& _manager);
        ~PackageManager();
        PackageManager& operator= (const PackageManager& _manager);

        bool loadPlugins();
        bool evalPluginCmd(string& sCmd);
        bool declareNewPackage(const string& sInstallInfoString);
        bool isPluginCmd(const string& sCmd) const;
        string deletePackage(const string& sPackage);
        std::map<std::string, std::string> getMenuMap() const;

        /////////////////////////////////////////////////
        /// \brief Returns the number of installed
        /// plugins.
        ///
        /// \return unsigned int
        ///
        /////////////////////////////////////////////////
        inline unsigned int getPackageCount() const
            {return vPackageInfo.size();}

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

            for (unsigned int i = 0; i < vPackageInfo.size(); i++)
                sReturn += vPackageInfo[i].getName() + ";";

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
            if (i < vPackageInfo.size())
                return vPackageInfo[i].sCommand;

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the plugin command signature
        /// of the ith plugin.
        ///
        /// \param i unsigned int i
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginCommandSignature(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getCommandSignature();

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
        inline string getPackageName(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getName();

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
        inline string getPackageVersion(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].sVersion;

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the author of the ith plugin.
        ///
        /// \param i unsigned int
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPackageAuthor(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getAuthor();

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
        inline string getPackageDescription(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getDescription();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the license information of the
        /// ith plugin.
        ///
        /// \param unsigned int i = 0
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPackageLicense(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getLicense();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the menu entry of the ith
        /// plugin.
        ///
        /// \param unsigned int i = 0
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPluginMenuEntry(unsigned int i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getMenuEntry();

            return "";
        }
};

#endif

