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

/*
 * Realisierung der Klasse Settings
 */

// --> Standard-Konstruktor <--
Settings::Settings() : Documentation()
{
	bDebug = false;
	bUseDebugger = false;
	bTestTabelle = false;
	bOnce = false;
	bFastStart = false;
	bUseDraftMode = false;
	bCompact = false;
	bGreeting = true;
	bDefineAutoLoad = false;
	bUseSystemPrints = true;
	bShowExtendedFileInfo = false;
	bUseLogfile = true;
	bLoadEmptyCols = false;
	bShowHints = true;
	bUseESCinScripts = true;
	bUseCustomLanguageFile = true;
	bUseExternalDocViewer = true;
	bUseExecuteCommand = false; // execute command is disabled by the default
	bUseMaskAsDefault = false; // mask is disabled as default
	bTryToDecodeProcedureArguments = false;
	nPrecision = 7;			// Standardmaessig setzen wir die Praezision auf 7
	nAutoSaveInterval = 30; // 30 sec
	sPath = "./";
	sSavePath = "<>/save";
	sLoadPath = "<>/data";
	sPlotOutputPath = "<>/plots";
	sScriptpath = "<>/scripts";
	sProcsPath = "<>/procedures";
	sWorkPath = "<>";
	sFramework = "calc";
	sSettings_ini = "numere.ini";
	sCmdCache = "";
	sViewer = "";
	sEditor = "notepad.exe";
	sDefaultfont = "pagella";
	nBuffer_x = 141;
	nBuffer_y = 300;
	nWindow_x = 140;
	nWindow_y = 34;
	nColorTheme = 0;
}

Settings::Settings(const Settings& _settings) : Settings()
{
    copySettings(_settings);
}
// --> Destruktor: Falls die INI-Datei noch geoeffnet ist, wird sie hier auf jeden Fall geschlossen <--
Settings::~Settings()
{
	if(Settings_ini.is_open())
	{
		Settings_ini.close();
	}
}

// --> Dateipfad zum Image-Viewer einstellen <--
void Settings::setViewerPath(const string& _sViewerPath)
{
    sViewer = _sViewerPath;

    // --> Entferne umschliessende Leerzeichen <--
    StripSpaces(sViewer);

    // --> Wenn nicht ".exe" gefunden wird, wird es hier ergaenzt <--
    if (sViewer.find(".exe") == string::npos)
        sViewer += ".exe";

    /* --> Sollte IN dem Dateipfad ein oder mehrere Leerzeichen sein, sollten wir den Pfad
     *     auf jeden Fall mit Anfuehrungszeichen umschliessen, falls selbige noch nicht
     *     vorhanden sind <--
     */
    if (sViewer.find(' ') != string::npos)
    {
        if (sViewer[0] != '"')
            sViewer = "\"" + sViewer;
        if (sViewer[sViewer.length()-1] != '"')
            sViewer += "\"";
    }
    return;
}


// --> Dateipfad zum Editor einstellen <--
void Settings::setEditorPath(const string& _sEditorPath)
{
    sEditor = _sEditorPath;

    // --> Entferne umschliessende Leerzeichen <--
    StripSpaces(sEditor);

    // --> Wenn nicht ".exe" gefunden wird, wird es hier ergaenzt <--
    if (sEditor.find(".exe") == string::npos)
        sEditor += ".exe";

    /* --> Sollte IN dem Dateipfad ein oder mehrere Leerzeichen sein, sollten wir den Pfad
     *     auf jeden Fall mit Anfuehrungszeichen umschliessen, falls selbige noch nicht
     *     vorhanden sind <--
     */
    if (sEditor.find(' ') != string::npos)
    {
        if (sEditor[0] != '"')
            sEditor = "\"" + sEditor;
        if (sEditor[sEditor.length()-1] != '"')
            sEditor += "\"";
    }
    return;
}

// --> Methode zum Speichern der getaetigten Einstellungen <--
void Settings::save(string _sWhere, bool bMkBackUp)
{
    string sExecutablePath = _sWhere + "\\" + sSettings_ini;
    if (bMkBackUp)
        sExecutablePath += ".back";

    // --> INI-Datei oeffnen, wobei "ios_base::out" bedeutet, dass wir in die Datei schreiben moechten <--
	Settings_ini.open(sExecutablePath.c_str(), ios_base::out | ios_base::trunc);

	/* --> Sollte aus irgendeinem Grund nicht in die INI-Datei geschrieben werden koennen, brechen wir ab und
	 *     geben eine entsprechende Fehlermeldung aus <--
	 */
	if (Settings_ini.fail() && !bMkBackUp)
	{
		NumeReKernel::print("ERROR: Could not save your configuration.");
		Settings_ini.close();
		return;
	}
	else if (Settings_ini.fail() && bMkBackUp)
	{
        NumeReKernel::print("ERROR: Could not save a backup of your configuration.");
        Settings_ini.close();
        return;
	}

	// --> Ggf vorherige Error-Flags, die beim Lesen aufgetreten sind, entfernen <--
	//Settings_ini.clear();

	// --> Zum Anfang springen <--
	Settings_ini.seekg(0);

	// --> Info-Text in die INI schreiben <--
	Settings_ini << "# NUMERE-CONFIG-09-SERIES" << endl;
	Settings_ini << "# =======================" << endl;
	Settings_ini << "# !!!Dieses File (speziell die allererste Zeile) nicht bearbeiten!!!" << endl;
	Settings_ini << "# In dieser Datei wird die Konfiguration fuer NumeRe gespeichert. Diese" << endl;
	Settings_ini << "# Konfiguration ist zu NumeRe-Versionen unterhalb v 0.9.3 NICHT kompatibel!" << endl;
	Settings_ini << "# Sollte es einmal zu Problemen beim Laden der Konfiguration kommen, oder es" << endl;
	Settings_ini << "# soll eine Version geringer v 0.9.3 geladen werden, dann muss diese Datei" << endl;
	Settings_ini << "# vollstaendig geloescht werden, um NumeRe's DEFAULT-KONFIGURATION zu laden." << endl;

	/* --> Sollte aus irgendeinem Grund nicht in die INI-Datei geschrieben werden koennen, brechen wir ab und
	 *     geben eine entsprechende Fehlermeldung aus <--
	 */
	if (Settings_ini.fail() && !bMkBackUp)
	{
		NumeReKernel::print("ERROR: Could not save your configuration.");
		Settings_ini.close();
		return;
	}
	else if (Settings_ini.fail() && bMkBackUp)
	{
        NumeReKernel::print("ERROR: Could not save a backup of your configuration.");
        Settings_ini.close();
        return;
	}

	// --> Eigentliche Einstellungen schreiben <--
	Settings_ini << "#" << endl << "# Standard-Dateipfade (relative Pfade gehen stets vom NumeRe-Stammordner aus):" << endl;
	Settings_ini << "-SAVEPATH=" << replaceExePath(sSavePath) << endl;
	Settings_ini << "-LOADPATH=" << replaceExePath(sLoadPath) << endl;
	Settings_ini << "-PLOTPATH=" << replaceExePath(sPlotOutputPath) << endl;
	Settings_ini << "-SCRIPTPATH=" << replaceExePath(sScriptpath) << endl;
	Settings_ini << "-PROCPATH=" << replaceExePath(sProcsPath) << endl;
	if (sViewer.length())
        Settings_ini << "-PLOTVIEWERPATH=" << replaceExePath(sViewer) << endl;
    Settings_ini << "-EDITORPATH=" << replaceExePath(sEditor) << endl;
	Settings_ini << "#" << endl << "# Programmkonfiguration:" << endl;
	Settings_ini << "-DEFAULTFRAMEWORK=" << sFramework << endl;
	Settings_ini << "-PRECISION=" << nPrecision << endl;
	Settings_ini << "-AUTOSAVEINTERVAL=" << nAutoSaveInterval << endl;
	Settings_ini << "-FASTSTART=" << bFastStart << endl;
	Settings_ini << "-GREETING=" << bGreeting << endl;
	Settings_ini << "-HINTS=" << bShowHints << endl;
	Settings_ini << "-ESCINSCRIPTS=" << bUseESCinScripts << endl;
	Settings_ini << "-PLOTFONT=" << sDefaultfont << endl;
	Settings_ini << "-USECOMPACTTABLES=" << bCompact << endl;
	Settings_ini << "-USECUSTOMLANGFILE=" << bUseCustomLanguageFile << endl;
	Settings_ini << "-USEEXTERNALVIEWER=" << bUseExternalDocViewer << endl;
	Settings_ini << "-USEEXECUTECOMMAND=" << bUseExecuteCommand << endl;
	Settings_ini << "-USEMASKASDEFAULT=" << bUseMaskAsDefault << endl;
	Settings_ini << "-TRYTODECODEPROCEDUREARGUMENTS=" << bTryToDecodeProcedureArguments << endl;
	Settings_ini << "-DEFCONTROL=" << bDefineAutoLoad << endl;
	Settings_ini << "-USEDRAFTMODE=" << bUseDraftMode << endl;
	Settings_ini << "-EXTENDEDFILEINFO=" << bShowExtendedFileInfo << endl;
	Settings_ini << "-USELOGFILE=" << bUseLogfile << endl;
	Settings_ini << "-LOADEMPTYCOLS=" << bLoadEmptyCols << endl;
	Settings_ini << "-BUFFERSIZE=" << nBuffer_x << "," << nBuffer_y << endl;
	Settings_ini << "-WINDOWSIZE=" << nWindow_x << "," << nWindow_y << endl;
	Settings_ini << "-COLORTHEME=" << nColorTheme << endl;
	Settings_ini << "#" << endl << "# Ende der Konfiguration" << endl;

	// --> Datei auf jeden Fall wieder schliessen, Erfolgsmeldung ausgeben und zurueck zur aufrufenden Stelle <--
	Settings_ini.close();
	if (!bMkBackUp)
        NumeReKernel::print(toSystemCodePage(_lang.get("SETTINGS_SAVE_SUCCESS"))+"\n");
	return;
}

// --> PRIVATE-Methode, die die Parameter des INI-Files identifiziert und die Einstellungen uebernimmt <--
bool Settings::set(const string& _sOption)
{
    /* --> Im Wesentlichen immer dasselbe: wenn der Parameter "-PARAM=" in der Zeile auftaucht,
     *     verwende alles danach (bis zum Ende der Zeile) als Wert der entsprechenden Einstellung <--
     */
    if (findParameter(_sOption, "savepath", '='))
    {
        sSavePath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sSavePath);
        if (sSavePath.length())
        {
            while (sSavePath.find('\\') != string::npos)
                sSavePath[sSavePath.find('\\')] = '/';
            return true;
        }
        else
            sSavePath = "<>/save";
    }
    else if (findParameter(_sOption, "loadpath", '='))
    {
        sLoadPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sLoadPath);
        if (sLoadPath.length())
        {
            while (sLoadPath.find('\\') != string::npos)
                sLoadPath[sLoadPath.find('\\')] = '/';
            return true;
        }
        else
            sLoadPath = "<>/data";
    }
    else if (findParameter(_sOption, "plotpath", '='))
    {
        sPlotOutputPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sPlotOutputPath);
        if (sPlotOutputPath.length())
        {
            while (sPlotOutputPath.find('\\') != string::npos)
                sPlotOutputPath[sPlotOutputPath.find('\\')] = '/';
            return true;
        }
        else
            sPlotOutputPath = "<>/plots";
    }
    else if (findParameter(_sOption, "scriptpath", '='))
    {
        sScriptpath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sScriptpath);
        if (sScriptpath.length())
        {
            while (sScriptpath.find('\\') != string::npos)
                sScriptpath[sScriptpath.find('\\')] = '/';
            return true;
        }
        else
            sScriptpath = "<>/scripts";
    }
    else if (findParameter(_sOption, "procpath", '='))
    {
        sProcsPath = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sProcsPath);
        if (sProcsPath.length())
        {
            while (sProcsPath.find('\\') != string::npos)
                sProcsPath[sProcsPath.find('\\')] = '/';
            return true;
        }
        else
            sProcsPath = "<>/procedures";
    }
    else if (findParameter(_sOption, "plotviewerpath", '='))
    {
        sViewer = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sViewer);
        if (sViewer.length())
        {
            while (sViewer.find('\\') != string::npos)
                sViewer[sViewer.find('\\')] = '/';
            return true;
        }
    }
    else if (findParameter(_sOption, "editorpath", '='))
    {
        sEditor = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sEditor);
        if (sEditor.length())
        {
            while (sEditor.find('\\') != string::npos)
                sEditor[sEditor.find('\\')] = '/';
            return true;
        }
        else
        {
            sEditor = "notepad.exe";
        }
    }
    else if (findParameter(_sOption, "precision", '='))
    {
        nPrecision = StrToInt(_sOption.substr(_sOption.find('=')+1));
        if (nPrecision > 0 && nPrecision < 15)
            return true;
        else
            nPrecision = 7;
    }
    else if (findParameter(_sOption, "faststart", '='))
    {
        bFastStart = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "greeting", '='))
    {
        bGreeting = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "hints", '='))
    {
        bShowHints = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "escinscripts", '='))
    {
        bUseESCinScripts = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usedraftmode", '='))
    {
        bUseDraftMode = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "defaultframework", '='))
    {
        sFramework = _sOption.substr(_sOption.find('=')+1);
        StripSpaces(sFramework);
        if (sFramework == "calc" || sFramework == "menue")
            return true;
        else
            sFramework = "calc";
    }
    else if (findParameter(_sOption, "plotfont", '='))
    {
        sDefaultfont = _sOption.substr(_sOption.find('=')+1);
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
            return true;
        }
        else
            sDefaultfont = "pagella";
    }
    else if (findParameter(_sOption, "usecompacttables", '='))
    {
        bCompact = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usecustomlangfile", '='))
    {
        bUseCustomLanguageFile = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "useexternaldocviewer", '=') || findParameter(_sOption, "useexternalviewer", '='))
    {
        bUseExternalDocViewer = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "useexecutecommand", '='))
    {
        bUseExecuteCommand = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "usemaskasdefault", '='))
    {
        bUseMaskAsDefault = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "trytodecodeprocedurearguments", '='))
    {
        bTryToDecodeProcedureArguments = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "extendedfileinfo", '='))
    {
        bShowExtendedFileInfo = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "uselogfile", '='))
    {
        bUseLogfile = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "loademptycols", '='))
    {
        bLoadEmptyCols = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "autosaveinterval", '='))
    {
        nAutoSaveInterval = StrToInt(_sOption.substr(_sOption.find('=')+1));
        if (nAutoSaveInterval)
            return true;
        else
            nAutoSaveInterval = 30;
    }
    else if (findParameter(_sOption, "buffersize", '='))
    {
        nBuffer_x = (unsigned)StrToInt(_sOption.substr(_sOption.find('=')+1,_sOption.find(',')));
        nBuffer_y = (unsigned)StrToInt(_sOption.substr(_sOption.find(',')+1));
        if (nBuffer_x >= 81 && nBuffer_y >= 300)
            return true;
        else
        {
            nBuffer_x = 81;
            nBuffer_y = 300;
        }
    }
    else if (findParameter(_sOption, "windowsize", '='))
    {
        nWindow_x = (unsigned)StrToInt(_sOption.substr(_sOption.find('=')+1,_sOption.find(',')));
        nWindow_y = (unsigned)StrToInt(_sOption.substr(_sOption.find(',')+1));
        if(nWindow_x >= 80 && nWindow_y >= 34)
            return true;
        else
        {
            nWindow_x = 80;
            nWindow_y = 34;
        }
    }
    else if (findParameter(_sOption, "defcontrol", '='))
    {
        bDefineAutoLoad = (bool)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }
    else if (findParameter(_sOption, "colortheme", '='))
    {
        nColorTheme = (unsigned)StrToInt(_sOption.substr(_sOption.find('=')+1));
        return true;
    }

    /* --> Taucht ein Fehler auf (ein Wert kann nicht korrekt identifziziert werden),
     *     oder gibt's den Parameter in dieser Liste nicht, dann
     *     gib' FALSE zurueck <--
     */
    return false;
}

// --> Methode zum Laden der Einstellungen aus dem INI-File "numere.ini" <--
void Settings::load(string _sWhere)
{
	string s;       // Temporaerer string fuer die verwendete Funktion "getline()"

    string sExecutablePath = _sWhere + "\\" + sSettings_ini;
	// --> Oeffne die INI-Datei mit dem IOS-Flag "in" <--
	Settings_ini.open(sExecutablePath.c_str(),ios_base::in);

	// --> Falls etwas schief laeuft, oder die Datei nicht existiert, gib eine Fehlermeldung aus und kehre zurueck <--
	if (Settings_ini.fail())
	{
        Settings_ini.close();
        Settings_ini.clear();
        Settings_ini.open((sExecutablePath+".back").c_str(),ios_base::in);
        if (Settings_ini.fail())
        {
            NumeReKernel::printPreFmt(" -> NOTE: Could not find the configuration file \"" + sSettings_ini + "\".\n");
            NumeReKernel::printPreFmt("    Loading default settings.\n");
            Sleep(500);
            Settings_ini.close();
            if (sPath == "./" || sPath == "<>")
                sPath = _sWhere;
            if (sLoadPath.substr(0,3) == "<>/")
                sLoadPath.replace(0,2,_sWhere);
            if (sSavePath.substr(0,3) == "<>/")
                sSavePath.replace(0,2,_sWhere);
            if (sPlotOutputPath.substr(0,3) == "<>/")
                sPlotOutputPath.replace(0,2,_sWhere);
            if (sScriptpath.substr(0,3) == "<>/")
                sScriptpath.replace(0,2,_sWhere);
            if (sProcsPath.substr(0,3) == "<>/")
                sProcsPath.replace(0,2,_sWhere);

            return;
        }
	}

    // --> Lies' die erste Zeile ein <--
    getline(Settings_ini,s);

    // --> Falls was schief laeuft: Zurueck <--
    if(Settings_ini.eof() || Settings_ini.fail())
    {
        return;
    }

    // --> Lautet die erste Zeile "# NUMERE-CONFIG-09-SERIES"? <--
    if (s != "# NUMERE-CONFIG-09-SERIES")
    {
        /* --> Nein? Dann ist das hier ein INI-File aus einer NumeRe-Version vor 0.9.3 <--
         * --> Hier stehen die Werte einfach so in den Zeilen und die Beschreibung darueber <--
         */
        getline(Settings_ini,s);
        getline(Settings_ini,s);
        if (bDebug)
            cerr << "|-> DEBUG: s = " << s << endl;
        nPrecision = StrToInt(s);
        getline(Settings_ini,s);
        getline(Settings_ini,sLoadPath);
        getline(Settings_ini,s);
        getline(Settings_ini,sSavePath);
        if (!Settings_ini.eof())
        {
            getline(Settings_ini, s);
            getline(Settings_ini, s);
            if(s.length())
                bFastStart = (bool)StrToInt(s);
        }
        else
        {
            bFastStart = false;
        }
        if (!Settings_ini.eof())
        {
            getline(Settings_ini, s);
            getline(Settings_ini, s);
            if (s == "calc" || s == "menue")
                sFramework = s;
            else
                sFramework = "calc";
        }
        else
        {
            sFramework = "calc";
        }
        if (!Settings_ini.eof())
        {
            getline(Settings_ini, s);
            getline(Settings_ini, s);
            if (s.length())
                bCompact = (bool)StrToInt(s);
        }
        else
        {
            bCompact = false;
        }
        if (!Settings_ini.eof())
        {
            getline(Settings_ini, s);
            getline(Settings_ini, s);
            if (s.length())
                nAutoSaveInterval = StrToInt(s);
        }
        else
        {
            nAutoSaveInterval = 30;
        }
        if (!Settings_ini.eof())
        {
            getline(Settings_ini, s);
            getline(Settings_ini, s);
            if (s.length())
                sPlotOutputPath = s;
        }
    }
    else
    {
        // --> Ja? Dann ist das hier das neue Format der INI-Datei. Zurueck zum ersten Zeichen <--
        Settings_ini.seekg(0);

        // --> Lese jetzt nacheinander jede Zeile ein <--
        do
        {
            getline(Settings_ini, s);

            // --> Ist das erste Zeichen ein "#"? Dann ist das ein Kommentar. Ignorieren! <--
            if (s[0] == '#')
                continue;

            // --> Besitzt die Zeile gar keine Zeichen? Ebenfalls ignorieren <--
            if (!s.length())
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
        while (!Settings_ini.eof() && !Settings_ini.fail());    // So lange nicht der EOF-Flag oder der FAIL-FLAG erscheint
    }

    if (sPath == "./" || sPath == "<>")
    {
        sPath = _sWhere;
        sWorkPath = _sWhere;
    }
    if (sWorkPath == "<>")
        sWorkPath = sPath;
    if (sLoadPath.substr(0,3) == "<>/")
        sLoadPath.replace(0,2,_sWhere);
    if (sSavePath.substr(0,3) == "<>/")
        sSavePath.replace(0,2,_sWhere);
    if (sPlotOutputPath.substr(0,3) == "<>/")
        sPlotOutputPath.replace(0,2,_sWhere);
    if (sScriptpath.substr(0,3) == "<>/")
        sScriptpath.replace(0,2,_sWhere);
    if (sProcsPath.substr(0,3) == "<>/")
        sProcsPath.replace(0,2,_sWhere);
    if (sEditor.find("<>/") != string::npos)
        sEditor.replace(sEditor.find("<>/"), 2, sPath);
    if (sViewer.find("<>/") != string::npos)
        sViewer.replace(sViewer.find("<>/"), 2, sPath);

    // --> Datei schliessen, Erfolgsmeldung und zurueck zur aufrufenden Stelle <--
    Settings_ini.close();
    Settings::save(_sWhere, true);
    NumeReKernel::printPreFmt(" -> Configuration loaded successful.");
	return;
}

Settings Settings::sendSettings()
{
    Settings _option(*this);
    return _option;
}

void Settings::copySettings(const Settings& _settings)
{
    bCompact = _settings.bCompact;
    bDefineAutoLoad = _settings.bDefineAutoLoad;
    bGreeting = _settings.bGreeting;
    bLoadEmptyCols = _settings.bLoadEmptyCols;
    bShowExtendedFileInfo = _settings.bShowExtendedFileInfo;
    bShowHints = _settings.bShowHints;
    bUseCustomLanguageFile = _settings.bUseCustomLanguageFile;
    bUseESCinScripts = _settings.bUseESCinScripts;
    bUseLogfile = _settings.bUseLogfile;
    sPath = _settings.sPath;
    sLoadPath = _settings.sLoadPath;
    sSavePath = _settings.sSavePath;
    sScriptpath = _settings.sScriptpath;
    sProcsPath = _settings.sProcsPath;
    sPlotOutputPath = _settings.sPlotOutputPath;
    sDefaultfont = _settings.sDefaultfont;
    nPrecision = _settings.nPrecision;
    nAutoSaveInterval = _settings.nAutoSaveInterval;
    nBuffer_x = _settings.nBuffer_x;
    nBuffer_y = _settings.nBuffer_y;
    bUseDebugger = _settings.bUseDebugger;
    bUseExternalDocViewer = _settings.bUseExternalDocViewer;
    bUseExecuteCommand = _settings.bUseExecuteCommand;
    bUseMaskAsDefault = _settings.bUseMaskAsDefault;
    bTryToDecodeProcedureArguments = _settings.bTryToDecodeProcedureArguments;
    setTokens(_settings.getTokenPaths());
}


