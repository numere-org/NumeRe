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


#ifndef SETTINGS_HPP
#define SETTINGS_HPP
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <windows.h>

#include "error.hpp"
#include "doc_helper.hpp"
#include "debugger.hpp"

using namespace std;
/*
 * Headerdatei zur Settings-Klasse
 */

int StrToInt(const string&);
int matchParams(const string& sCmd, const string& sParam, const char cFollowing);
void StripSpaces(string&);

class Settings : public Documentation
{
	private:
		bool bDebug;			    // Determiniert, ob der Debug-Modus aktiv ist
		bool bTestTabelle;		    // Determiniert, ob automatisch die Test-Tabelle generiert wird... -> Debugging
		bool bOnce;				    // Determiniert, ob das Programm nur einmal durchlaeuft
		bool bFastStart;            // Determiniert, ob der Parser zu Beginn getestet wird
		bool bCompact;              // Kompakte-Ausgabe-Boolean
		bool bGreeting;             // Begruessungs-Bool
		bool bDefineAutoLoad;       // Determiniert, ob die functions.def automatisch geladen und gespeichert wird
		bool bUseDraftMode;         // Determiniert, ob der "Draft-Modus" beim Plotten verwendet werden soll
		bool bUseSystemPrints;
		bool bShowExtendedFileInfo;
		bool bUseLogfile;
		bool bLoadEmptyCols;
		bool bShowHints;
		bool bUseESCinScripts;
		bool bUseDebugger;
		bool bUseCustomLanguageFile;
		int nPrecision;			    // Setzt die Genauigkeit der Ausgabe
		int nAutoSaveInterval;      // Das Intervall fuer die automatische Speicherung
		string sPath;               // Programm-Hauptpfad
		string sSavePath;		    // Pfad zur Ausgabe
		string sPlotOutputPath;     // Pfad zur Plot-Ausgabe
		string sLoadPath;		    // Pfad zum Einlesen
		string sScriptpath;         // Pfad zu den Scripten
		string sProcsPath;          // Pfad zu den Prozeduren
		string sWorkPath;           // Pfad zu den Prozeduren
		fstream Settings_ini;	    // file-stream fuer das Settings-ini-file
		string sSettings_ini;       // Dateiname des INI-Files
		string sFramework;          // string, der das Standard-Framework beim Start bestimmt
		string sViewer;             // string, der den Pfad zum Plotviewer speichert
		string sEditor;             // string, der den (Pfad zum) Editor speichert
		string sCmdCache;
		string sDefaultfont;        // string, der die Standard-Schriftart speichert
		unsigned int nBuffer_x;
		unsigned int nBuffer_y;
		unsigned int nWindow_x;
        unsigned int nWindow_y;
        unsigned int nColorTheme;


		bool set(const string&);    // PRIVATE-SET-Methode: Erkennt die Parameter im INI-File und setzt die Werte entsprechend
		inline string replaceExePath(const string& _sPath)
            {
                string sReturn = _sPath;
                if (sReturn.find(sPath) != string::npos)
                    sReturn.replace(sReturn.find(sPath), sPath.length(), "<>");
                return sReturn;
            }

	public:
		Settings();				    // Standard-Konstruktor
		~Settings();                // Destruktor (schliesst ggf. die INI-Datei)

        Debugger _debug;

        // --> Speicher- und Lade-Methoden <--
		void save(string _sWhere, bool bMkBackUp = false);
		void load(string _sWhere);

		/* --> Methoden, um ein Element zu erreichen, und seinen Status zu aendern. Nichts
		 *     besonderes. Selbsterklaerend <--
		 * --> Aufgrund geringer Komplexitaet als inline deklariert <--
		 * --> Zuerst alle SET-Methoden: <--
		 */
		inline void setbTestTabelle(bool _bTestTabelle)
            {
                bTestTabelle = _bTestTabelle;
                return;
            }
		inline void setbDebug(bool _bDebug)
            {
                bDebug = _bDebug;
                return;
            }
		inline void setbOnce(bool _bOnce)
            {
                bOnce = _bOnce;
                return;
            }
		inline void setbFastStart(bool _bFastStart)
            {
                bFastStart = _bFastStart;
                return;
            }
        inline void setbUseDraftMode(bool _bDraftMode)
            {
                bUseDraftMode = _bDraftMode;
                return;
            }
		inline void setbGreeting(bool _bGreeting)
            {
                bGreeting = _bGreeting;
                return;
            }
        inline void setbDefineAutoLoad(bool _bDefineAutoLoad)
            {
                bDefineAutoLoad = _bDefineAutoLoad;
                return;
            }
		inline void setprecision(int _nPrec)
            {
                if (_nPrec > 0 && _nPrec < 15)
                    nPrecision = _nPrec;
                return;
            }
        inline void setExePath(const string& _sPath)
            {
                if (_sPath.length())
                    sPath = _sPath;
                return;
            }
        inline void setWorkPath(const string& _sWorkPath)
            {
                if (_sWorkPath.length())
                    sWorkPath = _sWorkPath;
                return;
            }
		inline void setSavePath(const string& _sSavePath)
            {
                if (_sSavePath.length())
                    sSavePath = _sSavePath;
                return;
            }
		inline void setPlotOutputPath(const string& _sPlOutPath)
            {
                if (_sPlOutPath.length())
                    sPlotOutputPath = _sPlOutPath;
                return;
            }
		inline void setLoadPath(const string& _sLoadPath)
            {
                if (_sLoadPath.length())
                    sLoadPath = _sLoadPath;
                return;
            }
		inline void setScriptPath(const string& _sScriptPath)
            {
                if (_sScriptPath.length())
                    sScriptpath = _sScriptPath;
                return;
            }
        inline void setProcPath(const string& _sProcPath)
            {
                if (_sProcPath.length())
                    sProcsPath = _sProcPath;
                return;
            }
		inline void setFramework(const string& _sFramework)
            {
                if (_sFramework == "calc" || _sFramework == "menue")
                    sFramework = _sFramework;
                return;
            }
		inline void setbCompact(bool _bCompact)
            {
                bCompact = _bCompact;
                return;
            }
		inline void setbExtendedFileInfo(bool _bExtendedFileInfo)
            {
                bShowExtendedFileInfo = _bExtendedFileInfo;
                return;
            }
        inline void setbUseLogFile(bool _bUseLogfile)
            {
                bUseLogfile = _bUseLogfile;
                return;
            }
        inline void setbLoadEmptyCols(bool _bLoadEmptyCols)
            {
                bLoadEmptyCols = _bLoadEmptyCols;
                return;
            }
		inline void setAutoSaveInterval(int _nInterval)
            {
                if(_nInterval > 0)
                    nAutoSaveInterval = _nInterval;
                return;
            }
        inline void setWindowBufferSize(unsigned int _nBuffer_x = 81, unsigned int _nBuffer_y = 300)
            {
                if (_nBuffer_x >= 81)
                    nBuffer_x = _nBuffer_x;
                if (_nBuffer_y >= 300)
                    nBuffer_y = _nBuffer_y;
                return;
            }
        inline void setWindowSize(unsigned int _nWindow_x = 81, unsigned int _nWindow_y = 35)
            {
                if (_nWindow_x >= 81)
                    nWindow_x = _nWindow_x-1;
                if (_nWindow_y >= 35)
                    nWindow_y = _nWindow_y-1;
                return;
            }
        inline void setColorTheme(unsigned int _nColorTheme)
            {
                nColorTheme = _nColorTheme;
                return;
            }
        inline void setSystemPrintStatus(bool _bSystemPrints = true)
            {
                bUseSystemPrints = _bSystemPrints;
                return;
            }
        inline void setbShowHints(bool _bShowHints)
            {
                bShowHints = _bShowHints;
                return;
            }
        inline void setbUseESCinScripts(bool _bUseESC)
            {
                bUseESCinScripts = _bUseESC;
                return;
            }
        inline void setDefaultPlotFont(string& _sPlotFont)
            {
                if (_sPlotFont == "palatino")
                    _sPlotFont = "pagella";
                if (_sPlotFont == "times")
                    _sPlotFont = "termes";
                if (_sPlotFont == "bookman")
                    _sPlotFont = "bonum";
                if (_sPlotFont == "avantgarde")
                    _sPlotFont = "adventor";
                if (_sPlotFont == "chancery")
                    _sPlotFont = "chorus";
                if (_sPlotFont == "courier")
                    _sPlotFont = "cursor";
                if (_sPlotFont == "helvetica")
                    _sPlotFont = "heros";
                if (_sPlotFont == "pagella"
                        || _sPlotFont == "adventor"
                        || _sPlotFont == "bonum"
                        || _sPlotFont == "chorus"
                        || _sPlotFont == "cursor"
                        || _sPlotFont == "heros"
                        || _sPlotFont == "heroscn"
                        || _sPlotFont == "schola"
                        || _sPlotFont == "termes"
                    )
                {
                    sDefaultfont = _sPlotFont;
                }
                return;
            }
        inline void setDebbuger(bool _debugger)
            {
                bUseDebugger = _debugger;
                return;
            }
        inline void setUserLangFiles(bool _langfiles)
            {
                bUseCustomLanguageFile = _langfiles;
                return;
            }
        // --> Keine inline-Methode, da hoehere Komplexitaet <--
		void setViewerPath(const string& _sViewerPath);
		void setEditorPath(const string& _sEditorPath);


        inline void cacheCmd(const string& _sCmd)
            {
                if (_sCmd.length())
                    sCmdCache = _sCmd;
                return;
            }

        // --> Alle GET-Methoden. const-Methoden, da sie das Objekt nicht aendern <--
		inline bool getbDebug() const
            {return bDebug;}
		inline bool getbTestTabelle() const
            {return bTestTabelle;}
		inline bool getbOnce() const
            {return bOnce;}
		inline bool getbFastStart() const
            {return bFastStart;}
        inline bool getbUseDraftMode() const
            {return bUseDraftMode;}
		inline bool getbCompact() const
            {return bCompact;}
		inline bool getbGreeting() const
            {return bGreeting;}
        inline bool getbShowHints() const
            {return bShowHints;}
        inline bool getbUseESCinScripts() const
            {return bUseESCinScripts;}
        inline bool getbDefineAutoLoad() const
            {return bDefineAutoLoad;}
        inline bool getbShowExtendedFileInfo() const
            {return bShowExtendedFileInfo;}
        inline bool getbUseLogFile() const
            {return bUseLogfile;}
        inline bool getbLoadEmptyCols() const
            {return bLoadEmptyCols;}
		inline int getPrecision() const
            {return nPrecision;}
        inline string getExePath() const
            {return sPath;}
        inline string getWorkPath() const
            {return sWorkPath;}
		inline string getSavePath() const
            {return sSavePath;}
		inline string getLoadPath() const
            {return sLoadPath;}
		inline string getPlotOutputPath() const
            {return sPlotOutputPath;}
		inline string getScriptPath() const
            {return sScriptpath;}
        inline string getProcsPath() const
            {return sProcsPath;}
		inline string getFramework() const
            {return sFramework;}
		inline string getViewerPath() const
            {return sViewer;}
        inline string getEditorPath() const
            {return sEditor;}
		inline int getAutoSaveInterval() const
            {return nAutoSaveInterval;}
        inline unsigned int getBuffer(int nBuffer = 0) const
            {return nBuffer ? nBuffer_y : nBuffer_x;}
        inline unsigned int getWindow(int nWindow = 0) const
            {return nWindow ? nWindow_y : nWindow_x;}
        inline unsigned int getColorTheme() const
            {return nColorTheme;}
        inline string getDefaultPlotFont() const
            {return sDefaultfont;}
        inline string getTokenPaths() const
            {return "<>="+sPath
                    +";<wp>="+sWorkPath
                    +";<savepath>="+sSavePath
                    +";<loadpath>="+sLoadPath
                    +";<plotpath>="+sPlotOutputPath
                    +";<scriptpath>="+sScriptpath
                    +";<procpath>="+sProcsPath+";";}


        inline string readCmdCache(bool bClear = false)
            {
                string sTemp = sCmdCache;
                if (bClear)
                    sCmdCache = "";
                return sTemp;
            }
        inline bool getSystemPrintStatus() const
            {return bUseSystemPrints;}
        inline bool getUseDebugger() const
            {return bUseDebugger;}
        inline bool getUseCustomLanguageFiles() const
            {return bUseCustomLanguageFile;}

};
#endif
