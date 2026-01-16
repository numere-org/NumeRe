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
#include <boost/nowide/fstream.hpp>

#include "../ui/error.hpp"
#include "filesystem.hpp"
#include "../settings.hpp"
#include "../../versioninformation.hpp"
#include "../utils/tools.hpp"
#include "../datamanagement/table.hpp"

#ifndef OUTPUT_HPP
#define OUTPUT_HPP

/*
 * Headerdatei 'output.hpp' fuer die Class 'Output'
 * --> Definiert zugleich das Interface <--
 */

class Output : public FileSystem				// Diese Klasse ist ein CHILD von FileSystem
{
	private:
		std::string sFileName; 						// Dateiname fuer das erzeugte Datenfile. Default: Aufruf von generateFileName()
		std::string sPluginName;						// Speicherstelle, um den Namen des ausführenden Plugins auszulesen
		std::string sCommentLine;					// Speicherstelle, um eventuelle Kommentarzeilen zu setzen
		std::string sPluginPrefix;					// Speicherstelle, um den Prefix fuer den Default-Dateinamen zu setzen
		bool bFile; 							// true, wenn in eine Datei geschrieben wird, false sonst
		bool bFileOpen;							// true, wenn eine Datei von der Klasse geoeffnet wurde
		bool bCompact;                          // Kompakte-Ausgabe-Boolean
		bool bSumBar;
		bool bPrintTeX;                         // Ausgabe in eine TeX-Datei
		bool bPrintCSV;
		int64_t nTotalNumRows;
		boost::nowide::ofstream file_out;						// Ein Objekt eines Dateistreams
		std::string getDate(bool bForFile);			// Eine Methode, die das aktuelle Datum als String zurueckgibt.
		std::string replaceTeXControls(const std::string& _sText);

	public:
		//--> Konstruktoren und Destruktor <--
		Output(); 								// setzt bFile & bFileOpen = false und sFileName auf default
		Output(bool bStatus, std::string sFile); 	// Erzeugt ggf. Datenfile ueber Aufruf von start().
												//		|-> Falls bStatus = false wird sFileName auf _default_ gesetzt und sFile ignoriert
		~Output(); 								// Schliesst Datenfile, wenn bFile == true und bFileOpen == true, sonst return 0;

		//--> Methoden <--
		void start(); 							// Oeffnet Datenfile, wenn bFile == true und bFileOpen == false, sonst return 0;
		void end(); 							// Schliesst Datenfile, wenn bFile == true und bFileOpen == true, sonst return 0;
		void setStatus(bool bStatus); 			// setzt den Boolean bFile. Wenn true -> false gesetzt wird, ruft dies die Methode end() auf.
		bool isFile() const; 					// gibt den Wert von bFile zurueck
		void setFileName(std::string sFile); 		// setzt den Dateinamen in sFileName
		std::string getFileName() const;				// gibt den Dateinamen aus sFileName zurueck
		void generateFileName();				// generiert einen Dateinamen auf Basis der aktuellen Zeit.
		void setPluginName(std::string _sPluginName);// Namen des Plugins in die Variable sPluginName schreiben
		void setCommentLine(std::string _sCommentLine); // Setzt die Kommentarzeile
		std::string getPluginName() const;			// Gibt den Namen des Plugins zurueck
		std::string getCommentLine() const;			// Gibt den Inhalt der Variable sCommentLine zurueck
		void reset();							// Setzt den Output auf die Default-Werte zurueck
		void setPrefix(std::string _sPrefix);		// setzt sPluginPrefix auf _sPrefix
		std::string getPrefix() const;				// gibt sPluginPrefix zurueck
		void print_legal();						// Gibt ein paar Copyright-Infos in das Ziel
		void setCompact(bool _bCompact);        // setzt den Wert des Kompakte-Ausgabe-Booleans
		inline bool isCompact() const
        {
            return bCompact;
        }
        void setTotalNumRows(int64_t rows)
        {
            nTotalNumRows = rows;
        }


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
		void format(std::vector<std::vector<std::string>>& _sMatrix, size_t nHeadLineCount = 1);
		void format(NumeRe::Table _table, size_t digits = 0, size_t chars = 0);
		void print(std::string sOutput); 			// Zentrale Methode
												// 		|-> Schreibt den String in das Ziel.
												// 		|-> Prueft ggf., ob bFileOpen == true und ruft ggf. start() auf.
												// 		|-> Falls nicht in das Datenfile geschrieben werden kann, wird eine Fehlermeldung
												//			in der Konsole angezeigt. Automatisch wird auch auf die Ausgabe ueber die
												//			Konsole gewechselt. Der String, der den Fehler ausgeloest hat, wird ebenfalls
												//			auf der Konsole ausgegeben.
};

#endif
