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


#include "settings.hpp"
#include "../kernel.hpp"

#include <fstream>

int StrToInt(const std::string&);
int findParameter(const std::string& sCmd, const std::string& sParam, const char cFollowing);
void StripSpaces(std::string&);

/*
 * Realisierung der Klasse Settings
 */


/////////////////////////////////////////////////
/// \brief Settings class default constructor.
/// Creates and fills the internal setting value
/// map with their default values.
/////////////////////////////////////////////////
Settings::Settings() : Documentation()
{
    m_settings[SETTING_B_DEVELOPERMODE] = SettingsValue(false, SettingsValue::HIDDEN);
    m_settings[SETTING_B_DEBUGGER] = SettingsValue(false, SettingsValue::NONE);
    m_settings[SETTING_B_DRAFTMODE] = SettingsValue(false);
    m_settings[SETTING_B_COMPACT] = SettingsValue(false);
    m_settings[SETTING_B_GREETING] = SettingsValue(true);
    m_settings[SETTING_B_DEFCONTROL] = SettingsValue(false);
    m_settings[SETTING_B_TABLEREFS] = SettingsValue(true, SettingsValue::UIREFRESH | SettingsValue::SAVE);
    m_settings[SETTING_B_SYSTEMPRINTS] = SettingsValue(true, SettingsValue::HIDDEN);
    m_settings[SETTING_B_EXTENDEDFILEINFO] = SettingsValue(false, SettingsValue::UIREFRESH | SettingsValue::SAVE);
    m_settings[SETTING_B_LOGFILE] = SettingsValue(true);
    m_settings[SETTING_B_LOADEMPTYCOLS] = SettingsValue(false);
    m_settings[SETTING_B_SHOWHINTS] = SettingsValue(true);
    m_settings[SETTING_B_USEESCINSCRIPTS] = SettingsValue(true);
    m_settings[SETTING_B_USECUSTOMLANG] = SettingsValue(true);
    m_settings[SETTING_B_EXTERNALDOCWINDOW] = SettingsValue(true);
    m_settings[SETTING_B_ENABLEEXECUTE] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_MASKDEFAULT] = SettingsValue(false);
    m_settings[SETTING_B_DECODEARGUMENTS] = SettingsValue(false);
    m_settings[SETTING_V_PRECISION] = SettingsValue(7u, 1u, 14u);
    m_settings[SETTING_V_AUTOSAVE] = SettingsValue(30u, 1u, -1u);
    m_settings[SETTING_V_WINDOW_X] = SettingsValue(140u, 0u, -1u, SettingsValue::HIDDEN);
    m_settings[SETTING_V_WINDOW_Y] = SettingsValue(34u, 0u, -1u, SettingsValue::HIDDEN);
    m_settings[SETTING_V_BUFFERSIZE] = SettingsValue(300u, 300u, 1000u, SettingsValue::IMMUTABLE | SettingsValue::SAVE);
    m_settings[SETTING_S_EXEPATH] = SettingsValue("./", SettingsValue::PATH | SettingsValue::IMMUTABLE);
    m_settings[SETTING_S_SAVEPATH] = SettingsValue("<>/save", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_S_LOADPATH] = SettingsValue("<>/data", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_S_PLOTPATH] = SettingsValue("<>/plots", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_S_SCRIPTPATH] = SettingsValue("<>/scripts", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_S_PROCPATH] = SettingsValue("<>/procedures", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_S_WORKPATH] = SettingsValue("<>", SettingsValue::PATH);
    m_settings[SETTING_S_PLOTFONT] = SettingsValue("pagella");

    m_settings[SETTING_S_LATEXROOT] = SettingsValue("C:/Program Files", SettingsValue::SAVE | SettingsValue::PATH | SettingsValue::UIREFRESH);
    m_settings[SETTING_B_PRINTINCOLOR] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_V_CARETBLINKTIME] = SettingsValue(500u, 100u, 2000u, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_V_FOCUSEDLINE] = SettingsValue(10u, 1u, 30u, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_LINELENGTH] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_LINESINSTACK] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_MODULESINSTACK] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_GLOBALVARS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_PROCEDUREARGS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_FLASHTASKBAR] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_TOOLBARTEXT] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_PATHSONTABS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_PRINTLINENUMBERS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_SAVESESSION] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_SAVEBOOKMARKS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_FORMATBEFORESAVING] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_USEREVISIONS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_FOLDLOADEDFILE] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_HIGHLIGHTLOCALS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_USETABS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_HOMEENDCANCELS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_AUTOSAVEEXECUTION] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_BRACEAUTOCOMP] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_QUOTEAUTOCOMP] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_BLOCKAUTOCOMP] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_SMARTSENSE] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_S_EDITORFONT] = SettingsValue("consolas 10 windows-1252", SettingsValue::SAVE | SettingsValue::IMMUTABLE);
    m_settings[SETTING_B_AN_START] = SettingsValue(true, SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_USENOTES] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_USEWARNINGS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_USEERRORS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_MAGICNUMBERS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_UNDERSCOREARGS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_THISFILE] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_COMMENTDENS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_LOC] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_COMPLEXITY] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_ALWAYSMETRICS] = SettingsValue(false, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_RESULTSUP] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_RESULTASS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_TYPING] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_VARLENGTH] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_UNUSEDVARS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_GLOBALVARS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_CONSTANTS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_INLINEIF] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_PROCLENGTH] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_PROGRESS] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_B_AN_FALLTHROUGH] = SettingsValue(true, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_START] = SettingsValue("0:0:0-0:0:0-0000", SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_STANDARD] = SettingsValue(DEFAULT_ST_STANDARD, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_CONSOLESTD] = SettingsValue(DEFAULT_ST_CONSOLESTD, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_COMMAND] = SettingsValue(DEFAULT_ST_COMMAND, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_PROCCOMMAND] = SettingsValue(DEFAULT_ST_PROCCOMMAND, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_COMMENT] = SettingsValue(DEFAULT_ST_COMMENT, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_DOCCOMMENT] = SettingsValue(DEFAULT_ST_DOCCOMMENT, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_DOCKEYWORD] = SettingsValue(DEFAULT_ST_DOCKEYWORD, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_OPTION] = SettingsValue(DEFAULT_ST_OPTION, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_FUNCTION] = SettingsValue(DEFAULT_ST_FUNCTION, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_CUSTOMFUNC] = SettingsValue(DEFAULT_ST_CUSTOMFUNC, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_CLUSTER] = SettingsValue(DEFAULT_ST_CLUSTER, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_CONSTANT] = SettingsValue(DEFAULT_ST_CONSTANT, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_SPECIALVAL] = SettingsValue(DEFAULT_ST_SPECIALVAL, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_STRING] = SettingsValue(DEFAULT_ST_STRING, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_STRINGPARSER] = SettingsValue(DEFAULT_ST_STRINGPARSER, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_INCLUDES] = SettingsValue(DEFAULT_ST_INCLUDES, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_OPERATOR] = SettingsValue(DEFAULT_ST_OPERATOR, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_PROCEDURE] = SettingsValue(DEFAULT_ST_PROCEDURE, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_NUMBER] = SettingsValue(DEFAULT_ST_NUMBER, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_METHODS] = SettingsValue(DEFAULT_ST_METHODS, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_INSTALL] = SettingsValue(DEFAULT_ST_INSTALL, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_DEFVARS] = SettingsValue(DEFAULT_ST_DEFVARS, SettingsValue::SAVE | SettingsValue::HIDDEN);
    m_settings[SETTING_S_ST_ACTIVELINE] = SettingsValue(DEFAULT_ST_ACTIVELINE, SettingsValue::SAVE | SettingsValue::HIDDEN);


	sSettings_ini = "numere.ini";
}


/////////////////////////////////////////////////
/// \brief Settings class copy constructor.
/// Delegates the initialization to the default
/// constructor.
///
/// \param _settings const Settings&
///
/////////////////////////////////////////////////
Settings::Settings(const Settings& _settings) : Settings()
{
    copySettings(_settings);
}


/////////////////////////////////////////////////
/// \brief Saves the setting values to the
/// corresponding config file. Does only save the
/// setting values, which are marked that they
/// shall be saved.
///
/// \param _sWhere const std::string&
/// \param bMkBackUp bool
/// \return void
///
/////////////////////////////////////////////////
void Settings::save(const std::string& _sWhere, bool bMkBackUp)
{
    std::fstream Settings_ini;
    std::string sExecutablePath = _sWhere + "\\" + sSettings_ini;

    if (bMkBackUp)
        sExecutablePath += ".back";

    // Open config file
	Settings_ini.open(sExecutablePath.c_str(), std::ios_base::out | std::ios_base::trunc);

	// Ensure that the file is writeable
	if (Settings_ini.fail())
	{
		NumeReKernel::print("ERROR: Could not save your configuration.");
		Settings_ini.close();
		return;
	}

	// Write the header for the current configuration
	// file version
	Settings_ini << "# NUMERE-CONFIG-1x-SERIES\n";
	Settings_ini << "# =======================\n";
	Settings_ini << "# !!! DO NOT MODIFY THIS FILE UNLESS YOU KNOW THE CONSEQUENCES!!!\n#\n";
	Settings_ini << "# This file contains the configuration of NumeRe. This configuration is incompatible\n";
	Settings_ini << "# to versions prior to v 1.1.2.\n";
	Settings_ini << "# If you experience issues resulting form erroneous configurations, you may delete\n";
	Settings_ini << "# this and all other INI files in this directory. Delete also the numere.ini.back\n";
	Settings_ini << "# file for a complete reset. All configuration values will of course get lost during\n";
	Settings_ini << "# this process.\n#\n";
	Settings_ini << "# (relative paths are anchored in NumeRe's root directory):" << std::endl;

	// Write the setting values to the file. The values
	// are structured by their names and therefore will be
	// grouped together automatically
	for (auto iter = m_settings.begin(); iter != m_settings.end(); ++iter)
    {
        // Ensure that the setting shall be saved
        if (iter->second.shallSave())
        {
            // Write setting value identifier string
            Settings_ini << iter->first + "=";

            // Write the value
            switch (iter->second.getType())
            {
                case SettingsValue::BOOL:
                    Settings_ini << (iter->second.active() ? "true" : "false") << std::endl;
                    break;
                case SettingsValue::UINT:
                    Settings_ini << iter->second.value() << std::endl;
                    break;
                case SettingsValue::STRING:
                    Settings_ini << replaceExePath(iter->second.stringval()) << std::endl;
                    break;
                case SettingsValue::TYPELESS:
                    break;
            }
        }
    }

    // Write a footer line
	Settings_ini << "#\n# End of configuration file" << std::endl;

	// Close the file stream
	Settings_ini.close();

	if (!bMkBackUp)
        NumeReKernel::print(toSystemCodePage(_lang.get("SETTINGS_SAVE_SUCCESS"))+"\n");
}


/////////////////////////////////////////////////
/// \brief Imports a setting value string from
/// a v1.x configuration file.
///
/// \param sSettings const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Settings::import_v1x(const std::string& sSettings)
{
    // Separate identifier and corresponding value
    std::string id = sSettings.substr(0, sSettings.find('='));
    std::string value = sSettings.substr(sSettings.find('=')+1);

    // Search for the identifier
    auto iter = m_settings.find(id);

    // If the identifier exists, import
    // the value
    if (iter != m_settings.end())
    {
        switch (iter->second.getType())
        {
            case SettingsValue::BOOL:
                iter->second.active() = value == "true" ? true : false;
                break;
            case SettingsValue::UINT:
                iter->second.value() = (size_t)StrToInt(value);
                break;
            case SettingsValue::STRING:
                iter->second.stringval() = value;
                break;
            case SettingsValue::TYPELESS:
                break;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Imports a setting value from a v0.9x
/// configuration file.
///
/// \param _sOption const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Settings::set(const std::string& _sOption)
{
    /* --> Im Wesentlichen immer dasselbe: wenn der Parameter "-PARAM=" in der Zeile auftaucht,
     *     verwende alles danach (bis zum Ende der Zeile) als Wert der entsprechenden Einstellung <--
     */
    if (findParameter(_sOption, "savepath", '='))
    {
        std::string sSavePath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sSavePath);

        if (sSavePath.length())
        {
            while (sSavePath.find('\\') != std::string::npos)
                sSavePath[sSavePath.find('\\')] = '/';

            m_settings[SETTING_S_SAVEPATH].stringval() = sSavePath;
            return true;
        }
    }
    else if (findParameter(_sOption, "loadpath", '='))
    {
        std::string sLoadPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sLoadPath);

        if (sLoadPath.length())
        {
            while (sLoadPath.find('\\') != std::string::npos)
                sLoadPath[sLoadPath.find('\\')] = '/';

            m_settings[SETTING_S_LOADPATH].stringval() = sLoadPath;
            return true;
        }
    }
    else if (findParameter(_sOption, "plotpath", '='))
    {
        std::string sPlotOutputPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sPlotOutputPath);

        if (sPlotOutputPath.length())
        {
            while (sPlotOutputPath.find('\\') != std::string::npos)
                sPlotOutputPath[sPlotOutputPath.find('\\')] = '/';

            m_settings[SETTING_S_PLOTPATH].stringval() = sPlotOutputPath;
            return true;
        }
    }
    else if (findParameter(_sOption, "scriptpath", '='))
    {
        std::string sScriptpath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sScriptpath);

        if (sScriptpath.length())
        {
            while (sScriptpath.find('\\') != std::string::npos)
                sScriptpath[sScriptpath.find('\\')] = '/';

            m_settings[SETTING_S_SCRIPTPATH].stringval() = sScriptpath;
            return true;
        }
    }
    else if (findParameter(_sOption, "procpath", '='))
    {
        std::string sProcsPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sProcsPath);

        if (sProcsPath.length())
        {
            while (sProcsPath.find('\\') != std::string::npos)
                sProcsPath[sProcsPath.find('\\')] = '/';

            m_settings[SETTING_S_PROCPATH].stringval() = sProcsPath;
            return true;
        }
    }
    else if (findParameter(_sOption, "precision", '='))
    {
        size_t nPrecision = StrToInt(_sOption.substr(_sOption.find('=')+1));

        if (nPrecision > 0 && nPrecision < 15)
        {
            m_settings[SETTING_V_PRECISION].value() = nPrecision;
            return true;
        }
    }
    else if (findParameter(_sOption, "greeting", '='))
    {
        m_settings[SETTING_B_GREETING].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "hints", '='))
    {
        m_settings[SETTING_B_SHOWHINTS].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "escinscripts", '='))
    {
        m_settings[SETTING_B_USEESCINSCRIPTS].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usedraftmode", '='))
    {
        m_settings[SETTING_B_DRAFTMODE].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "plotfont", '='))
    {
        std::string sDefaultfont = _sOption.substr(_sOption.find('=')+1);

        if (sDefaultfont == "palatino")
            sDefaultfont = "pagella";

        if (sDefaultfont == "times")
            sDefaultfont = "termes";

        if (sDefaultfont == "bookman")
            sDefaultfont = "bonum";

        if (sDefaultfont == "avantgarde")
            sDefaultfont = "adventor";

        if (sDefaultfont == "chancery")
            sDefaultfont = "chorus";

        if (sDefaultfont == "courier")
            sDefaultfont = "cursor";

        if (sDefaultfont == "helvetica")
            sDefaultfont = "heros";

        if (sDefaultfont == "pagella"
                || sDefaultfont == "adventor"
                || sDefaultfont == "bonum"
                || sDefaultfont == "chorus"
                || sDefaultfont == "cursor"
                || sDefaultfont == "heros"
                || sDefaultfont == "heroscn"
                || sDefaultfont == "schola"
                || sDefaultfont == "termes"
            )
        {
            m_settings[SETTING_S_PLOTFONT].stringval() = sDefaultfont;
            return true;
        }
    }
    else if (findParameter(_sOption, "usecompacttables", '='))
    {
        m_settings[SETTING_B_COMPACT].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usecustomlangfile", '='))
    {
        m_settings[SETTING_B_USECUSTOMLANG].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "useexternaldocviewer", '=') || findParameter(_sOption, "useexternalviewer", '='))
    {
        m_settings[SETTING_B_EXTERNALDOCWINDOW].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "useexecutecommand", '='))
    {
        m_settings[SETTING_B_ENABLEEXECUTE].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usemaskasdefault", '='))
    {
        m_settings[SETTING_B_MASKDEFAULT].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "trytodecodeprocedurearguments", '='))
    {
        m_settings[SETTING_B_DECODEARGUMENTS].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "extendedfileinfo", '='))
    {
        m_settings[SETTING_B_EXTENDEDFILEINFO].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "uselogfile", '='))
    {
        m_settings[SETTING_B_LOGFILE].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "loademptycols", '='))
    {
        m_settings[SETTING_B_LOADEMPTYCOLS].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "autosaveinterval", '='))
    {
        size_t nAutoSaveInterval = StrToInt(_sOption.substr(_sOption.find('=')+1));

        if (nAutoSaveInterval)
        {
            m_settings[SETTING_V_AUTOSAVE].value() = nAutoSaveInterval;
            return true;
        }
    }
    else if (findParameter(_sOption, "buffersize", '='))
    {
        size_t nBuffer_y = (size_t)StrToInt(_sOption.substr(_sOption.find(',')+1));

        if (nBuffer_y >= 300)
        {
            m_settings[SETTING_V_BUFFERSIZE].value() = nBuffer_y;
            return true;
        }
    }
    else if (findParameter(_sOption, "windowsize", '='))
    {
        size_t nWindow_x = (size_t)StrToInt(_sOption.substr(_sOption.find('=')+1,_sOption.find(',')));
        size_t nWindow_y = (size_t)StrToInt(_sOption.substr(_sOption.find(',')+1));

        if (nWindow_x >= 80 && nWindow_y >= 34)
        {
            m_settings[SETTING_V_WINDOW_X].value() = nWindow_x;
            m_settings[SETTING_V_WINDOW_Y].value() = nWindow_y;
            return true;
        }
    }
    else if (findParameter(_sOption, "defcontrol", '='))
    {
        m_settings[SETTING_B_DEFCONTROL].active() = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }

    /* --> Taucht ein Fehler auf (ein Wert kann nicht korrekt identifziziert werden),
     *     oder gibt's den Parameter in dieser Liste nicht, dann
     *     gib' FALSE zurueck <--
     */
    return false;
}


/////////////////////////////////////////////////
/// \brief This member function is a helper,
/// which will replace the executable path part
/// in the passed file path with the <> path
/// token.
///
/// \param _sPath const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Settings::replaceExePath(const std::string& _sPath)
{
    const std::string& sPath = m_settings[SETTING_S_EXEPATH].stringval();
    std::string sReturn = _sPath;

    if (sReturn.find(sPath) != std::string::npos)
        sReturn.replace(sReturn.find(sPath), sPath.length(), "<>");

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This member function is a helper,
/// which will replace the <> path token in all
/// default file paths with the corresponding
/// executable path.
///
/// \param _sExePath const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Settings::prepareFilePaths(const std::string& _sExePath)
{
    if (m_settings[SETTING_S_EXEPATH].stringval() == "./" || m_settings[SETTING_S_EXEPATH].stringval() == "<>")
        m_settings[SETTING_S_EXEPATH].stringval() = _sExePath;

    m_settings[SETTING_S_WORKPATH].stringval() = m_settings[SETTING_S_EXEPATH].stringval();

    if (m_settings[SETTING_S_LOADPATH].stringval().substr(0, 3) == "<>/")
        m_settings[SETTING_S_LOADPATH].stringval().replace(0, 2, _sExePath);

    if (m_settings[SETTING_S_SAVEPATH].stringval().substr(0, 3) == "<>/")
        m_settings[SETTING_S_SAVEPATH].stringval().replace(0, 2, _sExePath);

    if (m_settings[SETTING_S_PLOTPATH].stringval().substr(0, 3) == "<>/")
        m_settings[SETTING_S_PLOTPATH].stringval().replace(0, 2, _sExePath);

    if (m_settings[SETTING_S_SCRIPTPATH].stringval().substr(0, 3) == "<>/")
        m_settings[SETTING_S_SCRIPTPATH].stringval().replace(0, 2, _sExePath);

    if (m_settings[SETTING_S_PROCPATH].stringval().substr(0, 3) == "<>/")
        m_settings[SETTING_S_PROCPATH].stringval().replace(0, 2, _sExePath);
}


/////////////////////////////////////////////////
/// \brief Opens the configuration file,
/// identifies its version and imports the
/// setting values.
///
/// \param _sWhere const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Settings::load(const std::string& _sWhere)
{
    std::ifstream Settings_ini;
	std::string s;

    std::string sExecutablePath = _sWhere + "\\" + sSettings_ini;

	// Open the configuration file
	Settings_ini.open(sExecutablePath.c_str());

	// If the file is not readable, try the *.ini.back
	// file as fallback
	if (Settings_ini.fail())
	{
        Settings_ini.close();
        Settings_ini.clear();
        Settings_ini.open((sExecutablePath+".back").c_str(), std::ios_base::in);

        if (Settings_ini.fail())
        {
            NumeReKernel::printPreFmt(" -> NOTE: Could not find the configuration file \"" + sSettings_ini + "\".\n");
            NumeReKernel::printPreFmt("    Loading default settings.\n");
            Sleep(500);
            Settings_ini.close();

            prepareFilePaths(_sWhere);

            return;
        }
	}

    // Read the first line
    std::getline(Settings_ini,s);

    // Ensure that the file contains some
    // strings
    if (Settings_ini.eof() || Settings_ini.fail())
        return;

    // Identify the version of the configuration
    // file and use the corresponding importing
    // member function
    if (s == "# NUMERE-CONFIG-1x-SERIES")
    {
        // Read every line of the file and
        // import the values
        while (!Settings_ini.eof())
        {
            std::getline(Settings_ini, s);

            // Ignore empty lines or comments
            if (!s.length() || s[0] == '#')
                continue;

            import_v1x(s);
        }
    }
    else if (s == "# NUMERE-CONFIG-09-SERIES")
    {
        // Read every line of the file and
        // import the values
        while (!Settings_ini.eof())
        {
            std::getline(Settings_ini, s);

            // Ignore empty lines or comments
            if (!s.length() || s[0] == '#')
                continue;

            // --> Gib die Zeile an die Methode "set()" weiter <--
            if (!Settings::set(s))
            {
                // --> FALSE wird nur dann zurueckgegeben, wenn ein Fehler auftaucht, oder ein Parameter unbekannt ist <--
                NumeReKernel::printPreFmt("\n -> NOTE: The setting \"" + s + "\" is not known and will be ignored.\n");
                Sleep(500);
                continue;
            }
        }
    }
    else
    {
        // This is a legacy format before v0.9.x
        std::getline(Settings_ini, s);
        std::getline(Settings_ini, s);
        m_settings[SETTING_V_PRECISION].value() = StrToInt(s);
        std::getline(Settings_ini, s);
        std::getline(Settings_ini, m_settings[SETTING_S_LOADPATH].stringval());
        std::getline(Settings_ini, s);
        std::getline(Settings_ini, m_settings[SETTING_S_SAVEPATH].stringval());

        if (!Settings_ini.eof())
        {
            for (size_t i = 0; i < 6; i++)
                std::getline(Settings_ini, s);

            if (s.length())
                m_settings[SETTING_B_COMPACT].active() = (bool)StrToInt(s);
        }

        if (!Settings_ini.eof())
        {
            std::getline(Settings_ini, s);
            std::getline(Settings_ini, s);

            if (s.length())
                m_settings[SETTING_V_AUTOSAVE].value() = StrToInt(s);
        }

        if (!Settings_ini.eof())
        {
            std::getline(Settings_ini, s);
            std::getline(Settings_ini, s);

            if (s.length())
                m_settings[SETTING_S_PLOTPATH].stringval() = s;
        }
    }

    prepareFilePaths(_sWhere);

    Settings::save(_sWhere, true);
    //NumeReKernel::printPreFmt(" -> Configuration loaded successful.");
}


/////////////////////////////////////////////////
/// \brief This member function is an alias for
/// the assignment operator overload.
///
/// \param _settings const Settings&
/// \return void
///
/////////////////////////////////////////////////
void Settings::copySettings(const Settings& _settings)
{
    m_settings = _settings.m_settings;
    setTokens(_settings.getTokenPaths());
}


