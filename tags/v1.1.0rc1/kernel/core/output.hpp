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


#include <string>
#include <ctime>

#include "error.hpp"
#include "filesystem.hpp"
#include "settings.hpp"
#include "version.h"
#include "tools.hpp"

#ifndef OUTPUT_HPP
#define OUTPUT_HPP
using namespace std;

/*
 * Headerdatei 'output.hpp' fuer die Class 'Output'
 * --> Definiert zugleich das Interface <--
 */
extern const string sVersion; 					// String, der die Versionsnummmer beinhaelt;

class Output : public FileSystem				// Diese Klasse ist ein CHILD von FileSystem
{
	private:
		string sFileName; 						// Dateiname fuer das erzeugte Datenfile. Default: Aufruf von generateFileName()
		string sPluginName;						// Speicherstelle, um den Namen des ausführenden Plugins auszulesen
		string sCommentLine;					// Speicherstelle, um eventuelle Kommentarzeilen zu setzen
		string sPluginPrefix;					// Speicherstelle, um den Prefix fuer den Default-Dateinamen zu setzen
		bool bFile; 							// true, wenn in eine Datei geschrieben wird, false sonst
		bool bFileOpen;							// true, wenn eine Datei von der Klasse geoeffnet wurde
		bool bCompact;                          // Kompakte-Ausgabe-Boolean
		bool bSumBar;
		bool bPrintTeX;                         // Ausgabe in eine TeX-Datei
		bool bPrintCSV;
		ofstream file_out;						// Ein Objekt eines Dateistreams
		string getDate(bool bForFile);			// Eine Methode, die das aktuelle Datum als String zurueckgibt.
		string replaceTeXControls(const string& _sText);
		//string ValidFileName(string _sFileName);// Eine Methode, die den Dateinamen auf Gueltigkeit prueft
	public:
		//--> Konstruktoren und Destruktor <--
		Output(); 								// setzt bFile & bFileOpen = false und sFileName auf default
		Output(bool bStatus, string sFile); 	// Erzeugt ggf. Datenfile ueber Aufruf von start().
												//		|-> Falls bStatus = false wird sFileName auf _default_ gesetzt und sFile ignoriert
		~Output(); 								// Schliesst Datenfile, wenn bFile == true und bFileOpen == true, sonst return 0;

		//--> Methoden <--
		void start(); 							// Oeffnet Datenfile, wenn bFile == true und bFileOpen == false, sonst return 0;
		void end(); 							// Schliesst Datenfile, wenn bFile == true und bFileOpen == true, sonst return 0;
		void setStatus(bool bStatus); 			// setzt den Boolean bFile. Wenn true -> false gesetzt wird, ruft dies die Methode end() auf.
		bool isFile() const; 					// gibt den Wert von bFile zurueck
		void setFileName(string sFile); 		// setzt den Dateinamen in sFileName
		string getFileName() const;				// gibt den Dateinamen aus sFileName zurueck
		void generateFileName();				// generiert einen Dateinamen auf Basis der aktuellen Zeit.
		void setPluginName(string _sPluginName);// Namen des Plugins in die Variable sPluginName schreiben
		void setCommentLine(string _sCommentLine); // Setzt die Kommentarzeile
		string getPluginName() const;			// Gibt den Namen des Plugins zurueck
		string getCommentLine() const;			// Gibt den Inhalt der Variable sCommentLine zurueck
		void reset();							// Setzt den Output auf die Default-Werte zurueck
		void setPrefix(string _sPrefix);		// setzt sPluginPrefix auf _sPrefix
		string getPrefix() const;				// gibt sPluginPrefix zurueck
		void print_legal();						// Gibt ein paar Copyright-Infos in das Ziel
		void setCompact(bool _bCompact);        // setzt den Wert des Kompakte-Ausgabe-Booleans
		inline bool isCompact() const
            {return bCompact;}


			/*
			 * Eine Art universelle Formatierungsfunktion: Bekommt als Uebergabe im Wesentlichen die bereits in einen string
			 * umgewandelten, aber unformatierten Tabelleneintraege mittels eines 2D-string-Arrays.
			 *
			 * Die Funktion sucht das laengste Element aus allen Tabelleneintraegen, addiert zwei Zeichen zu dieser Laenge
			 * und gleicht alle Eintraege durch Voranstellen der noetigen Anzahl an Leerstellen daran an.
			 *
			 * Zuletzt werden diese formatierten Eintraege zeilenweise an die Output-Klasse uebergeben, die sie entsprechend
			 * der Voreinstellung weiterverarbeitet.
			 */
		void format(string** _sMatrix, long long int _nCol, long long int _nLine, Settings& _option, bool bDontAsk = false, int nHeadLineCount = 1);
		void print(string sOutput); 			// Zentrale Methode
												// 		|-> Schreibt den String in das Ziel.
												// 		|-> Prueft ggf., ob bFileOpen == true und ruft ggf. start() auf.
												// 		|-> Falls nicht in das Datenfile geschrieben werden kann, wird eine Fehlermeldung
												//			in der Konsole angezeigt. Automatisch wird auch auf die Ausgabe ueber die
												//			Konsole gewechselt. Der String, der den Fehler ausgeloest hat, wird ebenfalls
												//			auf der Konsole ausgegeben.
};

#endif
