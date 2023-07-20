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
#include "../../kernel.hpp"
#include "../utils/tools.hpp"

//////////////////////
// CLASS PLUGIN
//////////////////////


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
Package::Package() : sCommand(""), sMainProcedure(""), sArgumentList(""), sType("TYPE_UNSPECIFIED"), sLicense(""), sName("Plugin"), sVersion("<AUTO>"), sAuthor("User"), sDescription("Description"), sMenuEntry(""), sDocumentationIndexID(""), sKeyWords("NONE"), sChangesLog("NONE")
{ }


/////////////////////////////////////////////////
/// \brief Specialized constructor. Will fill the
/// internal attributes using the passed install
/// information string.
///
/// \param sInstallInfoString const std::string&
///
/////////////////////////////////////////////////
Package::Package(const std::string& sInstallInfoString) : Package()
{
    // Get the options values from the string
    sCommand = getOptionValue(sInstallInfoString, "plugincommand", "");
    sMainProcedure = getOptionValue(sInstallInfoString, "pluginmain", "");
    sArgumentList = getOptionValue(sInstallInfoString, "pluginmain", "");
    sMenuEntry = getOptionValue(sInstallInfoString, "pluginmenuentry", "");
    sType = getOptionValue(sInstallInfoString, "type", "TYPE_UNSPECIFIED");
    sLicense = getOptionValue(sInstallInfoString, "license", "???");
    sName = getOptionValue(sInstallInfoString, "name", "Plugin");
    sVersion = getOptionValue(sInstallInfoString, "version", "0.0.1");
    sAuthor = getOptionValue(sInstallInfoString, "author", "User");
    sKeyWords = getOptionValue(sInstallInfoString, "keywords", "NONE");
    sChangesLog = getOptionValue(sInstallInfoString, "changelog", "NONE");
    sDescription = getOptionValue(sInstallInfoString, "desc", "Description");

    if (!sDescription.length())
        sDescription = getOptionValue(sInstallInfoString, "plugindesc", "Description");

    // If the main procedure was defined, separate it
    // here into name and argument list
    if (sMainProcedure.length())
    {
        sMainProcedure.erase(sMainProcedure.find('('));

        if (sMainProcedure.front() == '$')
            sMainProcedure.erase(0, 1);

        sArgumentList = sArgumentList.substr(sArgumentList.find('('));
    }
}


/////////////////////////////////////////////////
/// \brief This private member function extracts
/// the option value of the passed option and
/// replaces it by its default value, if it does
/// not exist.
///
/// \param sInstallInfoString const std::string&
/// \param sOption const std::string&
/// \param sDefault const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getOptionValue(const std::string& sInstallInfoString, const std::string& sOption, const std::string& sDefault)
{
    // Option is available? If no,
    // return the default value
    if (findParameter(sInstallInfoString, sOption, '='))
    {
        // Get the value
        std::string sOptionValue = getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, sOption, '=')+sOption.length());
        StripSpaces(sOptionValue);

        // Does it have a length? If no,
        // return the default value
        if (sOptionValue.length())
        {
            // If the default value has a non-zero length,
            // surround the extracted value using parentheses,
            // if it contains whitespaces or commas.
            if (sDefault.length() && (sOptionValue.find(' ') != std::string::npos || sOptionValue.find(',') != std::string::npos))
                sOptionValue = "(" + sOptionValue + ")";

            return sOptionValue;
        }
    }

    return sDefault;
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// definition export string to be written to the
/// plugin definition file.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::exportDefinition() const
{
    return sCommand + "," + sMainProcedure + "," + sArgumentList + "," + sType + "," + sName + "," + sVersion + "," + sAuthor + "," + sDescription + "," + sDocumentationIndexID + "," + sLicense + "," + sMenuEntry + "," + sKeyWords + "," + sChangesLog + ",";
}


/////////////////////////////////////////////////
/// \brief This member function will import the
/// package definition from the passed definition
/// string.
///
/// \param sDefinitionString std::string
/// \return void
///
/// \remark A default constructed Package object
/// is assumed by this member function.
///
/////////////////////////////////////////////////
void Package::importDefinition(std::string sDefinitionString)
{
    sCommand = getNextArgument(sDefinitionString, true);
    sMainProcedure = getNextArgument(sDefinitionString, true);
    sArgumentList = getNextArgument(sDefinitionString, true);
    sType = getNextArgument(sDefinitionString, true);
    sName = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sVersion = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sAuthor = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sDescription = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sDocumentationIndexID = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sLicense = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sMenuEntry = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sKeyWords = getNextArgument(sDefinitionString, true);

    if (!sDefinitionString.length())
        return;

    sChangesLog = getNextArgument(sDefinitionString, true);
}


/////////////////////////////////////////////////
/// \brief Returns, whether the current package
/// provides plugin functionalities.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Package::isPlugin() const
{
    return sType.substr(0, 11) == "TYPE_PLUGIN";
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the equality comparison operator.
///
/// \param _package const Package&
/// \return bool
///
/////////////////////////////////////////////////
bool Package::operator==(const Package& _package) const
{
    return _package.sCommand == sCommand && _package.sName == sName && _package.sAuthor == sAuthor;
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the inequality comparison operator.
///
/// \param _package const Package&
/// \return bool
///
/////////////////////////////////////////////////
bool Package::operator!=(const Package& _package) const
{
    return !operator==(_package);
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// update a package definition with a newer
/// definition. It will automatically increment
/// the packge version, if necessary.
///
/// \param _package const Package&
/// \return void
///
/////////////////////////////////////////////////
void Package::update(const Package& _package)
{
    sMainProcedure = _package.sMainProcedure;
    sArgumentList = _package.sArgumentList;
    sType = _package.sType;
    sName = _package.sName;
    sAuthor = _package.sAuthor;
    sDescription = _package.sDescription;
    sLicense = _package.sLicense;

    // Do we need to increment the current
    // package version?
    if (_package.sVersion == "<AUTO>")
        incrementVersion();
    else
        sVersion = _package.sVersion;
}


/////////////////////////////////////////////////
/// \brief This member function will increment
/// the package version number by a build count.
///
/// \return void
///
/////////////////////////////////////////////////
void Package::incrementVersion()
{
    sVersion = ::incrementVersion(sVersion);
}


/////////////////////////////////////////////////
/// \brief Returns the package name.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getName() const
{
    return stripParentheses(sName);
}


/////////////////////////////////////////////////
/// \brief Returns the package author.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getAuthor() const
{
    return stripParentheses(sAuthor);
}


/////////////////////////////////////////////////
/// \brief Returns the package description.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getDescription() const
{
    return stripParentheses(sDescription);
}


/////////////////////////////////////////////////
/// \brief Returns the package keywords.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getKeyWords() const
{
    return stripParentheses(sKeyWords);
}


/////////////////////////////////////////////////
/// \brief Returns the package license.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getLicense() const
{
    return stripParentheses(sLicense);
}


/////////////////////////////////////////////////
/// \brief Returns the menu entry of this plugin.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getMenuEntry() const
{
    return stripParentheses(sMenuEntry);
}


/////////////////////////////////////////////////
/// \brief Creates a command signature for a
/// plugin depending on the selected command line
/// exraction tags of the main procedure.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getCommandSignature() const
{
    if (!sCommand.length())
        return "";

    std::string sSignature = sCommand + " ";

    if (sArgumentList.find("<EXPRESSION>") != std::string::npos && sArgumentList.find("<PARAMSTRING>") != std::string::npos)
        sSignature += "EX -set PAR ";
    else if (sArgumentList.find("<EXPRESSION>") != std::string::npos)
        sSignature += "EX ";
    else if (sArgumentList.find("<PARAMSTRING>") != std::string::npos)
        sSignature += "-PAR ";
    else if (sArgumentList.find("<CMDSTRING>") != std::string::npos)
        sSignature += "(...) ";

    if (sType.find("TYPE_PLUGIN_WITH_RETURN_VALUE") != std::string::npos)
        sSignature += "-> ARG";

    return sSignature;
}


/////////////////////////////////////////////////
/// \brief Returns the package changeslog.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Package::getChangesLog() const
{
    return stripParentheses(sChangesLog);
}





//////////////////////
// CLASS PLUGINMANAGER
//////////////////////



/////////////////////////////////////////////////
/// \brief PluginManager default constructor.
/////////////////////////////////////////////////
PackageManager::PackageManager() : FileSystem()
{
    sPluginDefinitionFile = "<>/numere.plugins";
    sPluginProcName = "";
    sPluginVarList = "";
}


/////////////////////////////////////////////////
/// \brief PluginManager copy constructor.
///
/// \param _manager const PluginManager&
///
/////////////////////////////////////////////////
PackageManager::PackageManager(const PackageManager& _manager) : PackageManager()
{
    assign(_manager);
}


/////////////////////////////////////////////////
/// \brief This private member function handles
/// the actual copy process.
///
/// \param _manager const PluginManager&
/// \return void
///
/////////////////////////////////////////////////
void PackageManager::assign(const PackageManager& _manager)
{
    vPackageInfo = _manager.vPackageInfo;
}


/////////////////////////////////////////////////
/// \brief PluginManager destructor. Will close
/// the internal file stream if it is still open.
/////////////////////////////////////////////////
PackageManager::~PackageManager()
{
    if (fPlugins.is_open())
        fPlugins.close();
}


/////////////////////////////////////////////////
/// \brief This is the overload for the
/// assignment operator.
///
/// \param _manager const PluginManager&
/// \return PluginManager&
///
/////////////////////////////////////////////////
PackageManager& PackageManager::operator=(const PackageManager& _manager)
{
    assign(_manager);
    return *this;
}


/////////////////////////////////////////////////
/// \brief This member function will update the
/// plugin definition file with the internal
/// plugin definitions.
///
/// \return void
///
/////////////////////////////////////////////////
void PackageManager::updatePluginFile()
{
    sPluginDefinitionFile = FileSystem::ValidFileName(sPluginDefinitionFile, ".plugins");

    fPlugins.open(sPluginDefinitionFile.c_str(), std::ios_base::trunc | std::ios_base::out);

    // Ensure that the file is read and writeable
    if (fPlugins.fail())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sPluginDefinitionFile);

    // Write the contents to file
    for (size_t i = 0; i < vPackageInfo.size(); i++)
    {
        fPlugins << vPackageInfo[i].exportDefinition() << std::endl;
    }

    fPlugins.close();
    return;
}


/////////////////////////////////////////////////
/// \brief This member function will read the
/// plugin definitions from the definitions file
/// and create the internal representations.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageManager::loadPlugins()
{
    std::string sLine = "";

    sPluginDefinitionFile = FileSystem::ValidFileName(sPluginDefinitionFile, ".plugins");

    fPlugins.open(sPluginDefinitionFile.c_str());

    // Ensure that the file is readable
    if (fPlugins.fail())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sPluginDefinitionFile);

    // Read the file's contents to memory
    while (!fPlugins.eof())
    {
        std::getline(fPlugins, sLine);
        StripSpaces(sLine);

        if (sLine.length())
        {
            // Create a new Plugin instance and import
            // the definition
            vPackageInfo.push_back(Package());
            vPackageInfo.back().importDefinition(sLine);
        }
    }

    fPlugins.close();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function converts the call
/// to a plugin in the passed command line into
/// a call to the corresponding plugin main
/// procedure.
///
/// \param sCmd std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageManager::evalPluginCmd(std::string& sCmd)
{
    std::string sExpr = "";
    std::string sParams = "";
    std::string sCommand = "";
    std::string sCommandLine = "";
    Package _plugin;

    if (!vPackageInfo.size())
        return false;

    // Find the plugin definition
    for (size_t i = 0; i < vPackageInfo.size(); i++)
    {
        if (findCommand(sCmd, vPackageInfo[i].sCommand).sString == vPackageInfo[i].sCommand)
        {
            _plugin = vPackageInfo[i];
            break;
        }
    }

    if (!_plugin.sCommand.length())
        return false;

    // Find the plugin call
    Match _match = findCommand(sCmd, _plugin.sCommand);
    sCommandLine = extractCommandString(sCmd, _match);

    // Fill the internal variables with the values from the
    // definition
    sPluginProcName = _plugin.sMainProcedure;
    sPluginVarList = _plugin.sArgumentList;

    if (sPluginVarList[0] == '(' && sPluginVarList[sPluginVarList.length()-1] == ')')
        sPluginVarList = sPluginVarList.substr(1, sPluginVarList.length()-2);

    // Fill the different command line tags
    // If the argument list expects an expression, expression
    // and parameters are determined different than without
    // an expression
    if (sPluginVarList.find("<EXPRESSION>") != std::string::npos)
    {
        if (sCommandLine.find("-set") != std::string::npos || sCommandLine.find("--") != std::string::npos)
        {
            if (sCommandLine.find("-set") != std::string::npos)
                sParams = sCommandLine.substr(sCommandLine.find("-set"));
            else
                sParams = sCommandLine.substr(sCommandLine.find("--"));
        }

        if (sParams.length())
        {
            sExpr = sCommandLine.substr(sCommandLine.find(_plugin.sCommand)+_plugin.sCommand.length());
            sExpr.erase(sExpr.find(sParams));
            StripSpaces(sParams);
        }
        else
            sExpr = sCommandLine.substr(sCommandLine.find(_plugin.sCommand)+_plugin.sCommand.length());

        StripSpaces(sExpr);
        sParams = "\"" + sParams + "\"";
    }
    else if (sPluginVarList.find("<PARAMSTRING>") != std::string::npos)
    {
        if (sCommandLine.find('-') != std::string::npos)
            sParams = sCommandLine.substr(sCommandLine.find('-', sCommandLine.find(_plugin.sCommand)));

        StripSpaces(sParams);
        sParams = "\"" + sParams + "\"";
    }

    sCommand = "\"" + sCommandLine + "\"";

    for (size_t i = 1; i < sCommandLine.length()-1; i++)
    {
        if (sCommandLine[i] == '"' && sCommandLine[i-1] != '\\')
            sCommandLine.insert(i,1,'\\');
    }

    if (sParams.length())
    {
        for (size_t i = 1; i < sParams.length()-1; i++)
        {
            if (sParams[i] == '"' && sParams[i-1] != '\\')
                sParams.insert(i,1,'\\');
        }
    }

    // Replace the procedure argument list with the
    // corresponding command line tags
    while (sPluginVarList.find("<CMDSTRING>") != std::string::npos)
        sPluginVarList.replace(sPluginVarList.find("<CMDSTRING>"), 11, sCommand);

    while (sPluginVarList.find("<EXPRESSION>") != std::string::npos)
        sPluginVarList.replace(sPluginVarList.find("<EXPRESSION>"), 12, sExpr);

    while (sPluginVarList.find("<PARAMSTRING>") != std::string::npos)
        sPluginVarList.replace(sPluginVarList.find("<PARAMSTRING>"), 13, sParams);

    // If the plugin should have a return value,
    // add a corresponding tag to the command line
    // at the location of the call to the plugin.
    if (_plugin.sType.find("TYPE_PLUGIN_WITH_RETURN_VALUE") != std::string::npos)
        sCmd.replace(_match.nPos, sCommandLine.length(), "<<RETURNVAL>>");
    else
        sCmd.clear();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function declares a new
/// plugin from the passed install information
/// string.
///
/// \param sInstallInfoString const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageManager::declareNewPackage(const std::string& sInstallInfoString)
{
    static std::string sProtectedCommands = ";quit;help;find;uninstall;install;include;credits;about;continue;break;var;cst;tab;global;throw;namespace;return;abort;explicit;str;if;else;elseif;endif;while;endwhile;for;endfor;switch;case;default;endswitch;";
    bool bAllowOverride = false;

    // Create the new plugin
    Package _package(sInstallInfoString);

    // Determine, whether a forced override is
    // allowed
    if (findParameter(sInstallInfoString, "flags", '='))
    {
        if (getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, "flags", '=')+5).find("ENABLE_FORCE_OVERRIDE") != std::string::npos)
            bAllowOverride = true;
    }

    // Ensure that the necessary information has been provided
    // if the new package is a plugin
    if (_package.isPlugin())
    {
        if (!_package.sCommand.length())
            throw SyntaxError(SyntaxError::PLUGIN_HAS_NO_CMD, "", SyntaxError::invalid_position);

        if (sProtectedCommands.find(";" + _package.sCommand + ";") != std::string::npos)
            throw SyntaxError(SyntaxError::PLUGIN_MAY_NOT_OVERRIDE, "", SyntaxError::invalid_position, _package.sCommand);

        if (!_package.sMainProcedure.length())
            throw SyntaxError(SyntaxError::PLUGIN_HAS_NO_MAIN, "", SyntaxError::invalid_position);
    }

    // Append the plugin or override an existing one
    if (vPackageInfo.size())
    {
        for (size_t i = 0; i < vPackageInfo.size(); i++)
        {
            // Identical plugin command found?
            if (_package.isPlugin() && vPackageInfo[i].sCommand == _package.sCommand)
            {
                // Plugin is identical or forced override is enabled
                if (vPackageInfo[i] == _package || bAllowOverride)
                {
                    // Plugin names have to be unique: ensure that there's no
                    // duplicate
                    for (size_t j = i+1; j < vPackageInfo.size(); j++)
                    {
                        if (vPackageInfo[j].sName == _package.sName && vPackageInfo[j] != _package)
                            throw SyntaxError(SyntaxError::PLUGINNAME_ALREADY_EXISTS, "", SyntaxError::invalid_position, _package.getName());
                    }

                    // Update the existing plugin
                    vPackageInfo[i].update(_package);
                }
                else
                    throw SyntaxError(SyntaxError::PLUGINCMD_ALREADY_EXISTS, "", SyntaxError::invalid_position, _package.sCommand);

                break;
            }
            else if (vPackageInfo[i].sName == _package.sName)
            {
                // Plugin names have to be unique
                if (vPackageInfo[i] != _package)
                    throw SyntaxError(SyntaxError::PLUGINNAME_ALREADY_EXISTS, "", SyntaxError::invalid_position, _package.getName());

                vPackageInfo[i] = _package;
                break;
            }

            // Nothing found? Simply append the new plugin
            if (i == vPackageInfo.size()-1)
            {
                vPackageInfo.push_back(_package);

                if (vPackageInfo.back().sVersion == "<AUTO>")
                    vPackageInfo.back().sVersion = "0.0.1";

                break;
            }
        }
    }
    else
    {
        // No plugin installed? Simply append the
        // new plugin
        vPackageInfo.push_back(_package);

        if (vPackageInfo.back().sVersion == "<AUTO>")
            vPackageInfo.back().sVersion = "0.0.1";
    }

    updatePluginFile();

    NumeReKernel::getInstance()->refreshFunctionTree();
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function adds the passed
/// documentation index ID to the plugin
/// definition.
///
/// \param _sPluginName const std::string&
/// \param _sHelpId std::string
/// \return void
///
/////////////////////////////////////////////////
void PackageManager::addHelpIndex(const std::string& _sPluginName, std::string _sHelpId)
{
    StripSpaces(_sHelpId);
    std::string sPluginName = _sPluginName;
    StripSpaces(sPluginName);

    if (!vPackageInfo.size() || !_sHelpId.length())
        return;

    if (sPluginName.length())
    {
        // Search for the plugin with the selected name
        for (size_t i = 0; i < vPackageInfo.size(); i++)
        {
            // Identical name found? Append the documentation
            // index ID
            if (vPackageInfo[i].getName() == sPluginName)
            {
                if (vPackageInfo[i].sDocumentationIndexID.length())
                {
                    if (vPackageInfo[i].sDocumentationIndexID != _sHelpId
                        && vPackageInfo[i].sDocumentationIndexID.find(";"+_sHelpId) == std::string::npos
                        && vPackageInfo[i].sDocumentationIndexID.find(_sHelpId+";") == std::string::npos
                        && vPackageInfo[i].sDocumentationIndexID.find(";"+_sHelpId+";") == std::string::npos)
                    vPackageInfo[i].sDocumentationIndexID += ";" + _sHelpId;
                }
                else
                    vPackageInfo[i].sDocumentationIndexID = _sHelpId;

                updatePluginFile();
                break;
            }
        }
    }
    else
    {
        // No plugin name selected? Simply append it to the last
        // available plugin definition
        if (vPackageInfo.back().sDocumentationIndexID.length())
            vPackageInfo.back().sDocumentationIndexID += ";" + _sHelpId;
        else
            vPackageInfo.back().sDocumentationIndexID = _sHelpId;

        updatePluginFile();
    }

    return;
}


/////////////////////////////////////////////////
/// \brief This member function determines,
/// whether the passed command line contains a
/// plugin command.
///
/// \param sCmd const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageManager::isPluginCmd(const std::string& sCmd) const
{
    if (findCommand(sCmd, "explicit").sString == "explicit")
        return false;

    if (vPackageInfo.size())
    {
        for (size_t i = 0; i < vPackageInfo.size(); i++)
        {
            if (vPackageInfo[i].isPlugin() && findCommand(sCmd, vPackageInfo[i].sCommand).sString == vPackageInfo[i].sCommand)
                return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function deletes the
/// plugin with the passed name from the internal
/// set of definitions and returns the stored
/// documentation index IDs.
///
/// \param sPackage const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageManager::deletePackage(const std::string& sPackage)
{
    std::string sHLPIDs = "<<NO_HLP_ENTRY>>";

    for (size_t i = 0; i < vPackageInfo.size(); i++)
    {
        // Plugin found?
        if (vPackageInfo[i].getName() == sPackage)
        {
            // Store the documentation index ID
            if (vPackageInfo[i].sDocumentationIndexID.length())
                sHLPIDs = vPackageInfo[i].sDocumentationIndexID;

            // Remove the plugin
            vPackageInfo.erase(i + vPackageInfo.begin());

            updatePluginFile();

            NumeReKernel::getInstance()->refreshFunctionTree();
            return sHLPIDs;
        }
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the menu map connecting menu
/// entry names with their corresponding
/// procedure, which shall be called.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
std::map<std::string, std::string> PackageManager::getMenuMap() const
{
    std::map<std::string, std::string> mMenuMap;

    for (size_t i = 0; i < vPackageInfo.size(); i++)
    {
        if (vPackageInfo[i].sType == "TYPE_GUI_PLUGIN" && vPackageInfo[i].getMenuEntry().length())
            mMenuMap[vPackageInfo[i].getMenuEntry()] = vPackageInfo[i].sMainProcedure;
    }

    return mMenuMap;
}


/////////////////////////////////////////////////
/// \brief This member function simply returns
/// the plugin definition file path.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageManager::getPluginInfoPath()
{
    return FileSystem::ValidFileName(sPluginDefinitionFile, ".plugins");
}

