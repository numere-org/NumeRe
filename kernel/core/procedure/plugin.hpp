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
        /// \param sString const std::string&
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string stripParentheses(const std::string& sString) const
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
        std::string sKeyWords;
        std::string sChangesLog;

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
        std::string getKeyWords() const;
        std::string getLicense() const;
        std::string getMenuEntry() const;
        std::string getCommandSignature() const;
        std::string getChangesLog() const;
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
        std::fstream fPlugins;
        std::vector<Package> vPackageInfo;
        std::string sPluginDefinitionFile;

        std::string sPluginProcName;
        std::string sPluginVarList;
        void assign(const PackageManager& _manager);
        void updatePluginFile();

    public:
        PackageManager();
        PackageManager(const PackageManager& _manager);
        ~PackageManager();
        PackageManager& operator= (const PackageManager& _manager);

        bool loadPlugins();
        bool evalPluginCmd(std::string& sCmd);
        bool declareNewPackage(const std::string& sInstallInfoString);
        bool isPluginCmd(const std::string& sCmd) const;
        std::string deletePackage(const std::string& sPackage);
        std::map<std::string, std::string> getMenuMap() const;

        /////////////////////////////////////////////////
        /// \brief Returns the number of installed
        /// plugins.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        inline size_t getPackageCount() const
            {return vPackageInfo.size();}

        std::string getPluginInfoPath();
        void addHelpIndex(const std::string& _sPluginName, std::string _sHelpId);

        /////////////////////////////////////////////////
        /// \brief Returns a const reference to the
        /// currently installed packages.
        ///
        /// \return const std::vector<Package>&
        ///
        /////////////////////////////////////////////////
        const std::vector<Package>& getPackages() const
        {
            return vPackageInfo;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the names of the installed
        /// plugins.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginNames() const
        {
            std::string sReturn = ";";

            for (size_t i = 0; i < vPackageInfo.size(); i++)
                sReturn += vPackageInfo[i].getName() + ";";

            return sReturn;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the current plugin's procedure
        /// name.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginProcName() const
            {return sPluginProcName;}

        /////////////////////////////////////////////////
        /// \brief Returns the current plugin's procedure
        /// argument list.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginVarList() const
            {return sPluginVarList;}

        /////////////////////////////////////////////////
        /// \brief Returns the plugin command of the ith
        /// plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginCommand(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].sCommand;

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the plugin command signature
        /// of the ith plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginCommandSignature(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getCommandSignature();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the plugin name of the ith
        /// plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPackageName(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getName();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the version number string of
        /// the ith plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPackageVersion(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].sVersion;

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the author of the ith plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPackageAuthor(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getAuthor();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the description of the ith
        /// plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPackageDescription(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getDescription();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the license information of the
        /// ith plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPackageLicense(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getLicense();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the menu entry of the ith
        /// plugin.
        ///
        /// \param i size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPluginMenuEntry(size_t i = 0) const
        {
            if (i < vPackageInfo.size())
                return vPackageInfo[i].getMenuEntry();

            return "";
        }
};

#endif

