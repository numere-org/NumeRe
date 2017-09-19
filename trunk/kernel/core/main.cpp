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


// --> GLOBALE LIBRARIES <--
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <windows.h>
#include <conio.h>
#include <mgl2/mgl.h>
#include <ctime>


// --> LOKALE HEADER <--
#include "error.hpp"
#include "settings.hpp"
#include "output.hpp"
#include "datafile.hpp"
#include "plugins.hpp"
#include "version.h"
#include "functionimplementation.hpp"
#include "tools.hpp"
#include "built-in.hpp"
#include "parser_functions.hpp"
#include "define.hpp"
#include "plotdata.hpp"
//#include "menues.hpp"
#include "script.hpp"
#include "loop.hpp"
#include "procedure.hpp"
#include "plugin.hpp"
#include "language.hpp"
// --> PARSER-HEADER <--
#include "ParserLib/muParser.h"

// --> Wir wollen verhindern, jedes Mal std::cout, std::cerr oder std:cin zu schreiben <--
// --> EDIT: Der namespace mu gehoert zu den Methoden des Commandzeilen-Parsers <--
using namespace std;
using namespace mu;

// Globale Versions-Veriablen:
// --> Stable: Major++; Neues Feature: Minor++; Bugfix: Release++; Rebuild: Build++ (automatisch durch den Compiler) <--
const string sVersion = toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + AutoVersion::STATUS + "\"";
/* --> STATUS: Versionsname des Programms; Aktuell "Ampere", danach "Angstroem". Ab 1.0 Namen mit "B",
 *     z.B.: Biot(1774), Boltzmann(1844), Becquerel(1852), Bragg(1862), Bohr(1885), Brillouin(1889),
 *     de Broglie(1892, Bose(1894), Bloch(1905), Bethe(1906)) <--
 * --> de Coulomb(1736), Carnot(1796), P.Curie(1859), M.Curie(1867), A.Compton(1892), Cherenkov(1904), Casimir(1909), Chandrasekhar(1910), Chamberlain(1920), Cabibbo(1935) <--
 */

const string PI_TT = "0.1.4";
mglGraph _fontData;
//Language _lang;
//extern Plugin _plugin;
//extern volatile sig_atomic_t exitsignal;


// Globale Variable fuer die Zeilenlaenge
//int nLINE_LENGTH = 80;
//time_t tTimeZero = time(0);

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
bool IsWow64()
{
    BOOL bIsWow64 = false;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            return false;
        }
    }
    return (bool)bIsWow64;
}


/*
 * -> HIER UND IM FOLGENDEN: Objekte werden stets als Referenz& uebergeben, damit zum einen unnoetiges und aufwaendiges
 *    Kopieren vermieden wird, zum anderen aber auch auf den Objekten und nicht auf ihrer Kopie agiert werden kann
 */

/****
    * Hauptprogramm mit den zentralen Optionen.
    * -> Ist in der Lage, Kommandozeilen-Optionen auszulesen:
    *			"d" fuer Debugging, "f" fuer Ausgabe in Datei, "p" fuer volle Praezision, "o" fuer einmaligen Programmdurchlauf
    * -> Generiert je ein Objekt jeder Klasse: Output _out, Datafile _data und Settings _option
    *
    * Version 0.9.0 (v 0.9.0)
    */
int temp_main(int argc, char* argv[])
{
 	cerr << endl;
	printLogo();
    cerr << endl;
	//cerr << toSystemCodePage("Starte NumeRe: Framework für Numerische Rechnungen (v ") << sVersion << ")" << endl;
	cerr << toSystemCodePage(" Starting NumeRe: Framework für Numerische Rechnungen. Please be patient ...") << endl;
	//cerr << endl;

    int nReturnValue = -1;      // Zum Pruefen der Rueckgabewerte der Frameworks
	string sFile = ""; 			// String fuer den Dateinamen.
	string sScriptName = "";
	string sTime = getTimeStamp(false);
	string sLogFile = "numere.log";
	ofstream oLogFile;

    cerr << " -> Starting NumeRe-Core ... ";
	Settings _option;			// Starte eine Instanz der Settings-Klasse
	Output _out;				// Starte eine Instanz der Output-Klasse
	Datafile _data;				// Starte eine Instanz eines Datenfile-Objekts
	Parser _parser;             // Starte eine Instanz der muParser-Klasse
	Define _functions;          // Starte eine Instanz der Define-Klasse
    PlotData _pData;            // Starte eine Instanz der Plotdata-Klasse
    Script _script;             // Starte eine Instanz der Script-Klasse
    Procedure _procedure;       // Starte eine Instanz der Procedure-Klasse
    _data.setPredefinedFuncs(_functions.getPredefinedFuncs());
    Sleep(50);
    cerr << "Done.";

    nextLoadMessage(50);
    cerr << " -> Reading system's information ... ";
    char __cPath[1024];
    OSVERSIONINFO _osversioninfo;
    _osversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&_osversioninfo);
    GetModuleFileName(NULL, __cPath, 1024);
    string sPath = __cPath;
    sPath = sPath.substr(0,sPath.rfind("\\numere.exe"));
    while (sPath.find('\\') != string::npos)
        sPath[sPath.find('\\')] = '/';
    Sleep(50);
    cerr << "Done.";

    nextLoadMessage(50);
    _option.setExePath(sPath);
 	_option.load(sPath);				// Lade Informationen aus einem ini-File

    if (_option.getbUseLogFile())
    {
        reduceLogFilesize((sPath+"/"+sLogFile).c_str());
        oLogFile.open((sPath+"/"+sLogFile).c_str(), ios_base::out | ios_base::app | ios_base::ate);
        if (oLogFile.fail())
            oLogFile.close();
    }
    if (oLogFile.is_open())
    {
        oLogFile << "--- NUMERE-SESSION-PROTOCOL: " << sTime << " ---" << endl;
        oLogFile << "--- NumeRe v " << sVersion
                 << " | Build " << AutoVersion::YEAR << "-" << AutoVersion::MONTH << "-" << AutoVersion::DATE
                 << " | OS: Windows v " << _osversioninfo.dwMajorVersion << "." << _osversioninfo.dwMinorVersion << "." << _osversioninfo.dwBuildNumber << " " << _osversioninfo.szCSDVersion << (IsWow64() ? " (64 Bit) ---" : " ---") << endl;
    }

 	nextLoadMessage(50);
 	cerr << " -> Setting global parameters ... ";
 	_data.setTokens(_option.getTokenPaths());
 	_out.setTokens(_option.getTokenPaths());
 	_pData.setTokens(_option.getTokenPaths());
 	_script.setTokens(_option.getTokenPaths());
 	_functions.setTokens(_option.getTokenPaths());
 	_procedure.setTokens(_option.getTokenPaths());
 	_option.setTokens(_option.getTokenPaths());
 	_lang.setTokens(_option.getTokenPaths());
	ResizeConsole(_option);
    nLINE_LENGTH = _option.getWindow();
    Sleep(50);
    cerr << "Done.";

 	nextLoadMessage(50);
 	cerr << toSystemCodePage(" -> Verifying NumeRe file system ... ");
	_out.setPath(_option.getSavePath(), true, sPath);
	_data.setPath(_option.getLoadPath(), true, sPath);
	_data.setSavePath(_option.getSavePath());
	_data.setbLoadEmptyCols(_option.getbLoadEmptyCols());
	_pData.setPath(_option.getPlotOutputPath(), true, sPath);
	_script.setPath(_option.getScriptPath(), true, sPath);
	_procedure.setPath(_option.getProcsPath(), true, sPath);
	_option.setPath(_option.getExePath() + "/docs/plugins", true, sPath);
	_option.setPath(_option.getExePath() + "/docs", true, sPath);
	_option.setPath(_option.getExePath() + "/user/lang", true, sPath);
	_option.setPath(_option.getExePath() + "/user/docs", true, sPath);
	_functions.setPath(_option.getExePath(), false, sPath);
	Sleep(50);
    cerr << "Done.";
    if (oLogFile.is_open())
        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: File system was verified." << endl;

    nextLoadMessage(50);
    cerr << toSystemCodePage(" -> Loading documentation index ... ");
    _option.loadDocIndex(false);
    Sleep(50);
    cerr << "Done.";
    if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Documentation index was loaded." << endl;

    if (BI_FileExists(_option.getExePath() + "/update.hlpidx"))
    {
        nextLoadMessage(50);
        cerr << toSystemCodePage(" -> Updating documentation index ... ");
        _option.updateDocIndex();
        Sleep(50);
        cerr << "Done.";
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Documentation index was updated." << endl;
    }
    if (_option.getUseCustomLanguageFiles())
    {
        nextLoadMessage(50);
        cerr << toSystemCodePage(" -> Loading user documentation index ... ");
        _option.loadDocIndex(_option.getUseCustomLanguageFiles());
        Sleep(50);
        cerr << "Done.";
        if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: User Documentation index was loaded." << endl;
    }
    nextLoadMessage(50);
    cerr << toSystemCodePage(" -> Loading language files ... ");
    _lang.loadStrings(_option.getUseCustomLanguageFiles());
    Sleep(50);
    cerr << "Done.";
    if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Language files were loaded." << endl;



	// --> Kommandozeilen-Optionen einlesen und in die Instanz der Settings-Klasse uebertragen <--
    if (argc > 1)
    {
        nextLoadMessage(50);
        cerr << " -> " + _lang.get("MAIN_LOADING_CMDLINE") + " ... ";
        string sTemp = "";
        if (oLogFile.is_open())
            oLogFile << toString(time(0) - tTimeZero, true) << "> SYSTEM: Command line: \"numere";
        for (int i = 1; i < argc; i++)
        {
            sTemp = argv[i];
            if (oLogFile.is_open())
                oLogFile << " " << sTemp;
            sTemp = replacePathSeparator(sTemp);
            //cerr << sTemp << endl;
            if (sTemp == "-d")
                _option.setbDebug(true);
            else if (sTemp == "-p")
                _option.setprecision(14);
            else if (sTemp.length() > 5 && sTemp.substr(sTemp.length()-5) == ".nscr")
            {
                sScriptName = sTemp;
            }
            else if (sTemp.length() > 5 && sTemp.substr(sTemp.length()-5) == ".ndat")
            {
                _data.openFromCmdLine(_option, sTemp, false);
            }
            else if (sTemp.length() > 5 && sTemp.substr(sTemp.length()-5) == ".nprc")
            {
                sTemp = "$'" + sTemp.substr(0,sTemp.rfind('.')) + "'()";
                _option.cacheCmd(sTemp);
            }
            else if (sTemp.length() > 4
                && (toLowerCase(sTemp.substr(sTemp.length()-4)) == ".dat"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".csv"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".ods"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".ibw"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".txt"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".jdx"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".xls"
                    || toLowerCase(sTemp.substr(sTemp.length()-4)) == ".jcm"))
            {
                sTemp = "data -load=\""+sTemp+"\"";
                _option.cacheCmd(sTemp);
            }
            else if (sTemp.length() > 3 && toLowerCase(sTemp.substr(sTemp.length()-3)) == ".dx")
            {
                sTemp = "data -load=\""+sTemp+"\"";
                _option.cacheCmd(sTemp);
            }
            else if (sTemp.length() > 5 && toLowerCase(sTemp.substr(sTemp.length()-5)) == ".labx")
            {
                sTemp = "data -load=\"" + sTemp+"\"";
                _option.cacheCmd(sTemp);
            }
        }
        cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
        if (oLogFile.is_open())
        {
            oLogFile << "\"" << endl << toString(time(0)-tTimeZero, true) << "> SYSTEM: Command line parameters were processed." << endl;
        }
    }

    if (_option.getbDebug())
        cerr << "PATH: " << __cPath << endl;

    // --> Hier wollen wir den Titel der Console aendern. Ist eine Windows-Funktion <--
    SetConsTitle(_data, _option);

    string sAutosave = _option.getSavePath() + "/cache.tmp";
    string sCacheFile = _option.getExePath() + "/numere.cache";


	if (!_option.getbFastStart())
	{
        nextLoadMessage(50);
        cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_PARSER_SELFTEST") + " ... ");
        Sleep(600);
        parser_SelfTest(_parser);   // Fuehre den Parser-Selbst-Test aus
        Sleep(650);				    // Warte 500 msec
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Parser self test was done." << endl;
    }
    nextLoadMessage(50);
    cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_IOSTREAM") + " ... ");
    Sleep(50);
    cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
    if (BI_FileExists(_procedure.getPluginInfoPath()))
    {
        nextLoadMessage(50);
        cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_PLUGINS")+" ... ", _option);
        _procedure.loadPlugins();
        _plugin = _procedure;
        _data.setPluginCommands(_procedure.getPluginNames());
        cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Plugin information was loaded." << endl;
    }
    if (_option.getbDefineAutoLoad() && BI_FileExists(_option.getExePath() + "\\functions.def"))
    {
        nextLoadMessage(50);
        cerr << " -> ";
        _functions.load(_option, true);
        if (!_option.getbFastStart())
            Sleep(350);
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Function definitions were loaded." << endl;
    }

    nextLoadMessage(50);
    cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_FONT", toUpperCase(_option.getDefaultPlotFont().substr(0,1))+_option.getDefaultPlotFont().substr(1)) + " ... ");
    //cerr << toSystemCodePage(" -> Lade Schriftsatz \""+toUpperCase(_option.getDefaultPlotFont().substr(0,1))+_option.getDefaultPlotFont().substr(1)+"\" für Graph ... ");
    _fontData.LoadFont(_option.getDefaultPlotFont().c_str(), (_option.getExePath()+ "\\fonts").c_str());
    cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";

    nextLoadMessage(50);
    cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE_SEARCH")+" ... ", _option);
    Sleep(50);
    if (BI_FileExists(sAutosave) || BI_FileExists(sCacheFile))
    {
        cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_FOUND"));
        if (BI_FileExists(sAutosave))
        {
            // --> Lade den letzten Cache, falls dieser existiert <--
            nextLoadMessage(50);
            cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE")+" ... ", _option);
            _data.openAutosave(sAutosave, _option);
            _data.setSaveStatus(true);
            remove(sAutosave.c_str());
            cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_TRANSLATING")+" ... ");
            if (_data.saveCache())
                cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
            else
            {
                cerr << endl << " -> " << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_ERROR_SAVING")) << endl;
                Sleep(50);
            }
        }
        else
        {
            nextLoadMessage(50);
            cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE")+" ... ", _option);
            if (_data.loadCache())
                cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
            else
            {
                cerr << endl << " -> " << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_ERROR_LOADING")) << endl;
                Sleep(50);
            }
        }
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Automatic backup was loaded." << endl;
    }
    else
        cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_NOT_FOUND"));

    nextLoadMessage(50);
    cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_USER_INTERFACE")+" ... ", _option);
    Sleep(50);
    cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
    nextLoadMessage(50);
    cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_COLORTHEME")+" ... ", _option);
    Sleep(200);

    // --> Farben der Console aendern <--
    if (!ColorTheme(_option))
    {
        cerr << endl << LineBreak(" -> "+_lang.get("MAIN_LOADING_COLORTHEME_ERROR"), _option) << endl;
    }
	// --> Mach' einen netten Header <--
	//BI_hline(80);
	BI_splash();
	//BI_hline(80);
	//cerr << "|-> Copyright " << (char)184 << " " << AutoVersion::YEAR << toSystemCodePage(", E. Hänel et al.  +  +  +  siehe \"about\" für rechtl. Info |") << endl;
	cerr << "|-> Copyright " << (char)184 << " " << AutoVersion::YEAR << toSystemCodePage(", E. Hänel et al.") << std::setfill(' ') << std::setw(_option.getWindow()-37) << toSystemCodePage(_lang.get("MAIN_ABOUT_NBR") + " |") << endl; //toSystemCodePage("Über: siehe \"about\" |") << endl; //MAIN_ABOUT
	cerr << "|   Version: " << sVersion << std::setfill(' ') << std::setw(_option.getWindow()-25-sVersion.length()) << "Build: " << AutoVersion::YEAR << "-" << AutoVersion::MONTH << "-" << AutoVersion::DATE << " |" << endl;
	make_hline();
	/*cerr << "|" << endl;
	cerr << "|-> Willkommen bei NumeRe v " << sVersion << "!" << endl;
	cerr << "|   " << std::setfill((char)196) << std::setw(sVersion.length()+25) << (char)196 << endl;*/
	cerr << "|" << endl;

	if (_option.getbGreeting() && BI_FileExists(_option.getExePath()+"\\numere.ini"))
	{
        cerr << toSystemCodePage(BI_Greeting(_option));
        cerr << "|" << endl;
    }

	if (sScriptName.length())
	{
        _script.setScriptFileName(sScriptName);
        _script.setAutoStart(true);
    }
	// --> Zeige Info, dass bDebug == true! <--
	if (_option.getbDebug())
	{
		cerr << LineBreak("|-> DEBUG-MODE AKTIVIERT: Zusätzliche Zwischenergebnisse werden zur Fehlersuche angezeigt!", _option) << endl;
		cerr << "|" << endl;
	}

	/*
	 * --> Ab hier beginnt das eigentliche 'Programm' <--
	 * --> Von der folgenden Schleife aus startet das eigentliche User Interface <--
	 * --> Als Legacy-Option gibt es hier noch den "Menue-Based"-Modus, der aber
	 *     eigentlich nicht mehr noetig ist. Aus diesem Grund startet als Default auch
	 *     der "NumeRe-Rechner" (muParser und MathGL) <--
	 */

    if (_option.getFramework() == "menue")
    {
        cerr << LineBreak("|-> HINWEIS: Der Menü-Modus wird nicht mehr unterstützt!", _option) << endl;
        cerr << "|" << endl;
        _option.setFramework("calc");
    }
    do
    {
        try
        {
            nReturnValue = parser_Calc(_data, _out, _parser, _option, _functions, _pData, _script, _procedure, oLogFile);
        }
        catch (...)
        {
            return 1;
        }
    }
    while (nReturnValue);

    // --> Sind ungesicherte Daten im Cache? Dann moechte der Nutzer diese vielleicht speichern <--
    if (!_data.getSaveStatus()) // MAIN_UNSAVED_CACHE
    {
        if (!exitsignal)
        {
            string c = "";
            cerr << LineBreak("|-> "+_lang.get("MAIN_UNSAVED_CACHE"), _option) << endl;
            //cerr << LineBreak("|-> Es sind ungesicherte Daten im Cache vorhanden! Sollen sie gespeichert werden? (j/n)", _option) << endl;
            cerr << "|" << endl;
            cerr << "|<- ";
            getline(cin, c);
            if (c == _lang.YES())
            {
                _data.saveCache(); // MAIN_CACHE_SAVED
                cerr << LineBreak("|-> "+_lang.get("MAIN_CACHE_SAVED"), _option) << endl;
                //cerr << LineBreak("|-> Cache wurde erfolgreich gespeichert.", _option) << endl;
                // --> Sleep, damit genug Zeit zum Lesen ist <--
                //Sleep(1500);
                Sleep(500);
            }
        }
        else
        {
            _data.saveCache(); // MAIN_CACHE_SAVED
            cerr << LineBreak("|-> "+_lang.get("MAIN_CACHE_SAVED"), _option) << endl;
            //cerr << LineBreak("|-> Cache wurde erfolgreich gespeichert.", _option) << endl;
            Sleep(500);
        }
    }
    // Speicher aufraeumen
    _data.clearCache();
    _data.removeData(false); // MAIN_BYE
	cerr << toSystemCodePage("|-> "+_lang.get("MAIN_BYE_NBR")) << endl;
	//cerr << toSystemCodePage("|-> Bis zum nächsten Mal!") << endl;

    // --> Konfiguration aus den Objekten zusammenfassen und anschliessend speichern <--
	_option.setSavePath(_out.getPath());
	_option.setLoadPath(_data.getPath());
	_option.setPlotOutputPath(_pData.getPath());
	_option.setScriptPath(_script.getPath());
	if (_option.getbDefineAutoLoad() && _functions.getDefinedFunctions())
	{
        _functions.save(_option);
        Sleep(100);
	}
	_option.save(sPath); // MAIN_QUIT
	cerr << LineBreak("|-> "+_lang.get("MAIN_QUIT", sVersion), _option) << endl;
	//cerr << LineBreak("|-> NumeRe v " + sVersion + " wurde erfolgreich beendet.", _option) << endl;
	cerr << "|" << endl;
	if (oLogFile.is_open())
	{
        oLogFile << "--- NUMERE WAS TERMINATED SUCCESSFULLY ---" << endl << endl << endl;
        oLogFile.close();
	}
	// --> Sleep, damit genug Zeit zum Lesen ist <--
	Sleep(400);
	return 0;
}

/*
 * Ende der Hauptfunktion
 * -> Die komplette, restliche Funktionalitaet ist in den folgenden Unterfunktionen, den Klassen Output, Settings und Datafile und den plugin_* -Funktionen
 *    untergebracht
 */
