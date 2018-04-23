/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include <wx/image.h>
#include "dataops.hpp"
#include "../../kernel.hpp"
#include "../ui/language.hpp"
#include "../utils/tools.hpp"
#include "../utils/BasicExcel.hpp"
#include "../ui/error.hpp"
#include "../structures.hpp"


bool BI_parseStringArgs(const string& sCmd, string& sArgument, Parser& _parser, Datafile& _data, Settings& _option);
string parser_evalTargetExpression(string& sCmd, const string& sDefaultTarget, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option);

extern Language _lang;

void export_excel(Datafile& _data, Settings& _option, const string& sCache, const string& sFileName)
{
    using namespace YExcel;

    BasicExcel _excel;
    BasicExcelWorksheet* _sheet;
    BasicExcelCell* _cell;

    string sHeadLine;

    _excel.New(1);
    _excel.RenameWorksheet(0u, sCache.c_str());

    _sheet = _excel.GetWorksheet(0u);

    for (long long int j = 0; j < _data.getCols(sCache); j++)
    {
        _cell = _sheet->Cell(0u,j);
        sHeadLine = _data.getHeadLineElement(j, sCache);
        while (sHeadLine.find("\\n") != string::npos)
            sHeadLine.replace(sHeadLine.find("\\n"), 2, 1, (char)10);
        _cell->SetString(sHeadLine.c_str());
    }
    for (long long int i = 0; i < _data.getLines(sCache); i++)
    {
        for (long long int j = 0; j < _data.getCols(sCache); j++)
        {
            _cell = _sheet->Cell(1+i, j);
            if (_data.isValidEntry(i,j,sCache))
                _cell->SetDouble(_data.getElement(i,j,sCache));
            else
                _cell->EraseContents();
        }
    }
    _excel.SaveAs(sFileName.c_str());
    if (_option.getSystemPrintStatus())
        NumeReKernel::print(LineBreak(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE", toString((_data.getLines(sCache)+1)*_data.getCols(sCache)), sFileName), _option));
    return;
}


/* 2. Man moechte u.U. auch Daten einlesen, auf denen man agieren moechte.
 * Dies erlaubt diese Funktion in Verbindung mit dem Datafile-Objekt
 */
void load_data(Datafile& _data, Settings& _option, Parser& _parser, string sFileName)
{
    if (!sFileName.length())
    {
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ENTER_NAME", _data.getPath()), _option));
        //NumeReKernel::print(LineBreak("|-> Bitte den Dateinamen des Datenfiles eingeben! Wenn kein Pfad angegeben wird, wird standardmäßig im Ordner \"" + _data.getPath() + "\" gesucht.$(0 zum Abbrechen)", _option) );
        do
        {
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(sFileName);		// gesamte Zeile einlesen: Koennte ja auch eine Leerstelle enthalten sein
            StripSpaces(sFileName);
        }
        while (!sFileName.length());

        if (sFileName == "0")
        {
            NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            //NumeReKernel::print("|-> ABBRUCH!" );
            return;
        }

    }
	if (!_data.isValid())	// Es sind noch keine Daten vorhanden?
	{
		//if(_option.getbDebug())
		//	NumeReKernel::print("|-> DEBUG: sFileName = " + sFileName );
        _data.openFile(sFileName, _option, false, false); 			// gesammelte Daten an die Klasse uebergeben, die den Rest erledigt
	}
	else	// Sind sie doch? Dann muessen wir uns was ueberlegen...
	{
		string c = "";
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_APPEND", _data.getDataFileName("data")), _option));
		//NumeReKernel::print(LineBreak("|-> FEHLER: Speichergruppe bereits mit den Daten des Files \"" + _data.getDataFileName("data") + "\" besetzt. Sollen die neuen Daten stattdessen an die vorhandene Tabelle angehängt werden? (j/n)$(0 zum Abbrechen)", _option) );
		NumeReKernel::printPreFmt("|\n|<- ");
		NumeReKernel::getline(c);

		if (c == "0")
		{
			NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			//NumeReKernel::print("|-> ABBRUCH!" );
			return;
		}
		else if (c == _lang.YES())		// Anhaengen?
		{
			append_data("data -app=\"" + sFileName + "\" i", _data, _option, _parser);
		}
		else				// Nein? Dann vielleicht ueberschreiben?
		{
			c = "";
			NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_OVERRIDE"), _option));
			//NumeReKernel::print(LineBreak("|-> Daten werden nicht angehängt. Sollen die Daten überschrieben werden? (j/n)", _option) );
			NumeReKernel::printPreFmt("|\n|<- ");
			NumeReKernel::getline(c);

			if (c == _lang.YES())					// Also ueberschreiben
			{
				_data.removeData();			// Speicher freigeben...
				_data.openFile(sFileName, _option, false, false);
				if (_data.isValid())
                    NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
                    //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
			}
			else							// Kannst du dich vielleicht mal entscheiden?
			{
				NumeReKernel::print(_lang.get("COMMON_CANCEL"));
				//NumeReKernel::print("|-> ABBRUCH!" );
			}
		}
	}
	return;
}

// 3. Zur Kontrolle (oder aus anderen Gruenden) moechte man die eingelesenen Daten vielleicht auch betrachten. Wird hier erledigt
void show_data(Datafile& _data, Output& _out, Settings& _option, const string& _sCache, size_t nPrecision, bool bData, bool bCache, bool bSave, bool bDefaultName)
{
    string sCache = _sCache;
    string sFileName = "";
	if (_data.isValid() || _data.isValidCache())		// Sind ueberhaupt Daten vorhanden?
	{
        if (!(bData || bCache))
        {
            string c = "";
            if (_data.isValidCache())
            {
                NumeReKernel::printPreFmt("|-> Es sind Daten im Cache.\n");
                NumeReKernel::printPreFmt("|   Sollen sie statt der Daten des Datenfiles\n");
                if (_out.isFile())
                    NumeReKernel::printPreFmt("|   gespeichert werden? (j/n)\n");
                else
                    NumeReKernel::printPreFmt("|   angezeigt werden? (j/n)\n");
                NumeReKernel::printPreFmt("|   (0 zum Abbrechen)\n");
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(c);

                if (c == "0")
                {
                    NumeReKernel::print("ABBRUCH!" );
                    return;
                }
                else if (c == "j")
                    _data.setCacheStatus(true);
            }
		}
		else if (bCache && _data.isValidCache())
            _data.setCacheStatus(true);
        else if (bData && _data.isValid())
            _data.setCacheStatus(false);
        else
        {
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, "", SyntaxError::invalid_position);
        }

		long long int nLine = 0;// = _data.getLines(sCache)+nHeadlineCount;		// Wir muessen Zeilen fuer die Kopfzeile hinzufuegen
		long long int nCol = 0;// = _data.getCols(sCache);
		int nHeadlineCount = 0;
		/*if (!nCol || nLine == 1)
            throw NO_CACHED_DATA;*/

		if (_option.getbDebug())
			NumeReKernel::print("DEBUG: nLine = " + toString(nLine) + ", nCol = " + toString(nCol) );
		if (bSave && bData)
		{
            if (bDefaultName)
            {
                sFileName = _data.getDataFileName(sCache);
                if (sFileName.find_last_of("/") != string::npos)
                    sFileName = sFileName.substr(sFileName.find_last_of("/")+1);
                if (sFileName.find_last_of("\\") != string::npos)
                    sFileName = sFileName.substr(sFileName.find_last_of("\\")+1);
                sFileName = _out.getPath() + "/copy_of_" + sFileName;
                if (sFileName.substr(sFileName.length()-5,5) == ".labx")
                    sFileName = sFileName.substr(0,sFileName.length()-5) + ".dat";
                if (_option.getbDebug())
                    NumeReKernel::print("DEBUG: sFileName = " + sFileName );
                _out.setFileName(sFileName);
            }
            _out.setStatus(true);
		}
		else if (bSave && bCache)
		{
            if (bDefaultName)
            {
                _out.setPrefix(sCache);
                _out.generateFileName();
            }
            _out.setStatus(true);
		}
		if (bSave && _out.getFileName().substr(_out.getFileName().rfind('.')) == ".xls")
		{
            export_excel(_data, _option, sCache, _out.getFileName());
            _out.reset();
            return;
		}
		string** sOut = make_stringmatrix(_data, _out, _option, sCache, nLine, nCol, nHeadlineCount, nPrecision, bSave);// = new string*[nLine];		// die eigentliche Ausgabematrix. Wird spaeter gefuellt an Output::format(string**,int,int,Output&) uebergeben
        if (sCache.front() == '*')
            sCache.erase(0,1); // Vorangestellten Unterstrich wieder entfernen
		if (_data.getCacheStatus() && !bSave)
		{
			_out.setPrefix("cache");
			if(_out.isFile())
				_out.generateFileName();
		}
		_out.setPluginName("Datenanzeige der Daten aus " + _data.getDataFileName(sCache)); // Anzeige-Plugin-Parameter: Nur Kosmetik
		if (_option.getUseExternalViewer() && !bSave)
            NumeReKernel::showTable(sOut, nCol, nLine, sCache);
        else
        {
            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
                NumeReKernel::print("NUMERE: " + toUpperCase(sCache) + "()");
                make_hline();
            }
            _out.format(sOut, nCol, nLine, _option, (bData || bCache), nHeadlineCount);		// Eigentliche Ausgabe
            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
		}
		_out.reset();						// Ggf. bFile in der Klasse = FALSE setzen
		if ((bCache || _data.getCacheStatus()) && bSave)
            _data.setSaveStatus(true);
		_data.setCacheStatus(false);


		for (long long int i = 0; i < nLine; i++)
		{
			delete[] sOut[i];		// WICHTIG: Speicher immer freigeben!
		}
		delete[] sOut;
	}
	else		// Offenbar sind gar keine Daten geladen. Was soll ich also anzeigen?
	{
		if (bCache)
            //throw NO_CACHED_DATA;
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, "", SyntaxError::invalid_position);
        else
            //throw NO_DATA_AVAILABLE;
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, "", SyntaxError::invalid_position);
	}
	return;
}

string** make_stringmatrix(Datafile& _data, Output& _out, Settings& _option, const string& sCache, long long int& nLines, long long int& nCols, int& nHeadlineCount, size_t nPrecision, bool bSave)
{
    nHeadlineCount = 1;
    if (_option.getUseExternalViewer())
        _out.setCompact(false);
    if (!_out.isCompact())
    {
        for (long long int j = 0; j < _data.getCols(sCache); j++)
        {
            if (_data.getHeadLineElement(j, sCache).find("\\n") == string::npos)
                continue;
            int nLinebreak = 0;
            for (unsigned int n = 0; n < _data.getHeadLineElement(j, sCache).length()-2; n++)
            {
                if (_data.getHeadLineElement(j, sCache).substr(n,2) == "\\n")
                    nLinebreak++;
            }
            if (nLinebreak+1 > nHeadlineCount)
                nHeadlineCount = nLinebreak+1;
        }
    }
    nLines = _data.getLines(sCache)+nHeadlineCount;		// Wir muessen Zeilen fuer die Kopfzeile hinzufuegen
    nCols = _data.getCols(sCache);
    if (!nCols || nLines == 1)
        //throw NO_CACHED_DATA;
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "", SyntaxError::invalid_position);

    if (_option.getbDebug())
        NumeReKernel::print("DEBUG: nLine = " + toString(nLines) + ", nCol = " + toString(nCols) );

    string** sOut = new string*[nLines];		// die eigentliche Ausgabematrix. Wird spaeter gefuellt an Output::format(string**,int,int,Output&) uebergeben
    for (long long int i = 0; i < nLines; i++)
    {
        sOut[i] = new string[nCols];			// Vollstaendig Allozieren!
    }

    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            if (!i)						// Erste Zeile? -> Kopfzeilen uebertragen
            {
                if (_out.isCompact())
                    sOut[i][j] = _data.getTopHeadLineElement(j, sCache);
                else
                    sOut[i][j] = _data.getHeadLineElement(j, sCache);
                if (_out.isCompact() && (int)sOut[i][j].length() > 11 && !bSave)
                {
                    //sOut[i][j].replace(4, sOut[i][j].length()-9, "...");
                    sOut[i][j].replace(8, string::npos, "...");
                }
                else if (nHeadlineCount > 1 && sOut[i][j].find("\\n") != string::npos)
                {
                    string sHead = sOut[i][j];
                    int nCount = 0;
                    for (unsigned int n = 0; n < sHead.length(); n++)
                    {
                        if (sHead.substr(n,2) == "\\n")
                        {
                            sOut[i+nCount][j] = sHead.substr(0,n);
                            sHead.erase(0,n+2);
                            n = 0;
                            nCount++;
                        }
                    }
                    sOut[i+nCount][j] = sHead;
                }
                if (j == nCols-1)
                    i = nHeadlineCount-1;
                continue;
            }
            if (!_data.isValidEntry(i-nHeadlineCount,j, sCache))
            {
                sOut[i][j] = "---";			// Nullzeile? -> Da steht ja in Wirklichkeit auch kein Wert drin...
                continue;
            }
            if (_out.isCompact() && !bSave)
                sOut[i][j] = toString(_data.getElement(i-nHeadlineCount,j, sCache), 4);		// Daten aus _data in die Ausgabematrix uebertragen
            else
                sOut[i][j] = toString(_data.getElement(i-nHeadlineCount,j, sCache), nPrecision);		// Daten aus _data in die Ausgabematrix uebertragen
        }
    }
    return sOut;
}

// 4. Sehr spannend: Einzelne Datenreihen zu einer einzelnen Tabelle verknuepfen
void append_data(const string& __sCmd, Datafile& _data, Settings& _option, Parser& _parser)
{
    string sCmd = __sCmd;
    Datafile _cache;
    _cache.setPath(_data.getPath(), false, _data.getProgramPath());
    _cache.setTokens(_option.getTokenPaths());
    int nArgument = 0;
    string sArgument = "";
    if (_data.containsStringVars(sCmd))
        _data.getStringValues(sCmd);
    addArgumentQuotes(sCmd, "app");
    if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
    {
        if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
        {
            if (sArgument.find('/') == string::npos)
                sArgument = "<loadpath>/"+sArgument;
            vector<string> vFilelist = getFileList(sArgument, _option);
            if (!vFilelist.size())
            {
                throw SyntaxError(SyntaxError::FILE_NOT_EXIST, __sCmd, SyntaxError::invalid_position, sArgument);
            }
            string sPath = "<loadpath>/";
            if (sArgument.find('/') != string::npos)
                sPath = sArgument.substr(0,sArgument.rfind('/')+1);
            /*Datafile _cache;
            _cache.setTokens(_option.getTokenPaths());
            _cache.setPath(_data.getPath(), false, _data.getProgramPath());*/
            for (unsigned int i = 0; i < vFilelist.size(); i++)
            {
                if (!_data.isValid())
                {
                    _data.openFile(sPath+vFilelist[0], _option, false, true);
                    continue;
                }
                _cache.removeData(false);
                _cache.openFile(sPath+vFilelist[i], _option, false, true);
                _data.melt(_cache);
            }
            if (_data.isValid() && _option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_APPENDDATA_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
                //NumeReKernel::print(LineBreak("|-> Alle Daten der " +toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich mit den Daten im Speicher zusammengeführt: der Datensatz besteht nun aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
            return;
        }

        if (_data.isValid())	// Sind ueberhaupt Daten in _data?
        {
            if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
            {
                if (matchParams(sCmd, "head", '='))
                    nArgument = matchParams(sCmd, "head", '=')+4;
                else
                    nArgument = matchParams(sCmd, "h", '=')+1;
                nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                _cache.openFile(sArgument, _option, false, true, nArgument);
            }
            else
                _cache.openFile(sArgument, _option, false, true);

            _data.melt(_cache);
            if (_cache.isValid() && _option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_APPENDDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _cache.getDataFileName("data") + "\" wurden erfolgreich mit den Daten im Speicher zusammengeführt: der Datensatz besteht nun aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
        }
        else
        {
            if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
            {
                if (matchParams(sCmd, "head", '='))
                    nArgument = matchParams(sCmd, "head", '=')+4;
                else
                    nArgument = matchParams(sCmd, "h", '=')+1;
                nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
                _data.openFile(sArgument, _option, false, true, nArgument);
            }
            else
                _data.openFile(sArgument, _option, false, true);
            if (_data.isValid() && _option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
                //NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
        }
    }
	return;
}

// 5. Vielleicht hat man sich irgendwie vertan und moechte die Daten wieder entfernen -> Das klappt hiermit
void remove_data (Datafile& _data, Settings& _option, bool bIgnore)
{
	if (_data.isValid())
	{
        if (!bIgnore)
        {
            string c = "";
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_CONFIRM"), _option));
            //NumeReKernel::print(LineBreak("|-> Die gespeicherten Daten werden aus dem Speicher entfernt!$Sicher? (j/n)", _option) );
            NumeReKernel::printPreFmt("|\n|<- ");
            // Bist du sicher?
            NumeReKernel::getline(c);

            if (c == _lang.YES())
            {
                _data.removeData();		// Wenn ja: Aufruf der Methode Datafile::removeData(), die den Rest erledigt
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_SUCCESS"), _option));
                //NumeReKernel::print(LineBreak("|-> Der Speicher wurde erfolgreich freigegeben.", _option) );
            }
            else					// Wieder mal anders ueberlegt, hm?
            {
                NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            }
        }
        else if (_option.getSystemPrintStatus())
        {
            _data.removeData();
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_SUCCESS"), _option));
            //NumeReKernel::print(LineBreak("|-> Der Speicher wurde erfolgreich freigegeben.", _option) );
        }
        else
        {
            _data.removeData();
        }
	}
	else if (_option.getSystemPrintStatus())
	{
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_NO_DATA"), _option));
		//NumeReKernel::print(LineBreak("|-> Es existieren keine Daten, die gelöscht werden können.", _option) );
	}
	return;
}

// 8. Den Cache leeren
void clear_cache(Datafile& _data, Settings& _option, bool bIgnore)
{
	if (_data.isValidCache())
	{
        if (!bIgnore)
        {
            string c = "";
            if (!_data.getSaveStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM_NOTSAFED"), _option));
                //NumeReKernel::print(LineBreak("|-> Alle Caches und die automatische Speicherung werden gelöscht, obwohl sie NICHT gespeichert wurden!$Sicher? (j/n)", _option) );
            else
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM"), _option));
                //NumeReKernel::print(LineBreak("|-> Alle Caches und die automatische Speicherung werden gelöscht!$Sicher? (j/n)", _option) );

            NumeReKernel::printPreFmt("|\n|<- ");
            //NumeReKernel::print("|<- ");			// Bist du sicher?
            NumeReKernel::getline(c);

            if(c == _lang.YES())
            {
                string sAutoSave = _option.getSavePath() + "/cache.tmp";
                string sCache_file = _option.getExePath() + "/numere.cache";
                _data.clearCache();	// Wenn ja: Aufruf der Methode Datafile::clearCache(), die den Rest erledigt
                remove(sAutoSave.c_str());
                remove(sCache_file.c_str());
            }
            else					// Wieder mal anders ueberlegt, hm?
            {
                NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            }
            //cin.ignore(1);
		}
		else
		{
            string sAutoSave = _option.getSavePath() + "/cache.tmp";
            string sCache_file = _option.getExePath() + "/numere.cache";
            _data.clearCache();
            remove(sAutoSave.c_str());
            remove(sCache_file.c_str());
		}
		if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_SUCCESS"), _option));
            //NumeReKernel::print("|-> Alle Caches wurden entfernt und der Speicher wurde erfolgreich freigegeben." );
	}
	else if (_option.getSystemPrintStatus())
	{
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_EMPTY"), _option));
		//NumeReKernel::print("|-> Der Cache ist bereits leer." );
	}
	return;
}

// 20. Loescht ein der mehrere Eintraege im Cache
bool deleteCacheEntry(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _iDeleteIndex;
    bool bSuccess = false;

    /*if (sCmd.find("data(") != string::npos)
        return false;*/

    while (sCmd.find("()") != string::npos)
        sCmd.replace(sCmd.find("()"),2,"(:,:)");

    sCmd.erase(0,findCommand(sCmd).nPos+findCommand(sCmd).sString.length());

    while (getNextArgument(sCmd, false).length())
    {
        string sCache = getNextArgument(sCmd, true);
        if (sCache.substr(0,5) == "data(")
            continue;
        StripSpaces(sCache);
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sCache.substr(0,sCache.find('(')) == iter->first) //(sCache.find(iter->first+"(") != string::npos)
            {
                _iDeleteIndex = parser_getIndices(sCache.substr(sCache.find('(')), _parser, _data, _option);
                if ((_iDeleteIndex.nI[0] == -1 && !_iDeleteIndex.vI.size()) || (_iDeleteIndex.nJ[0] == -1 && !_iDeleteIndex.vJ.size()))
                    return false;

                _data.setCacheStatus(true);
                if (_iDeleteIndex.nI[1] == -2)
                    _iDeleteIndex.nI[1] = _data.getLines(iter->first, false);
                else if (_iDeleteIndex.nI[1] != -1)
                    _iDeleteIndex.nI[1] += 1;
                if (_iDeleteIndex.nJ[1] == -2)
                    _iDeleteIndex.nJ[1] = _data.getCols(iter->first);
                else if (_iDeleteIndex.nJ[1] != -1)
                    _iDeleteIndex.nJ[1] += 1;

                if (!_iDeleteIndex.vI.size() && !_iDeleteIndex.vJ.size())
                    _data.deleteBulk(iter->first, _iDeleteIndex.nI[0], _iDeleteIndex.nI[1], _iDeleteIndex.nJ[0], _iDeleteIndex.nJ[1]);
                else
                {
                    _data.deleteBulk(iter->first, _iDeleteIndex.vI, _iDeleteIndex.vJ);
                }

                _data.setCacheStatus(false);
                bSuccess = true;
                break;
            }
        }
    }
    return bSuccess;
}

// 21. Kopiert ganze Teile eines Datenobjekts in den Cache (oder im Cache umher)
bool CopyData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    string sToCopy = "";
    string sTarget = "";
    bool bTranspose = false;

    Indices _iCopyIndex;
    Indices _iTargetIndex;
    for (int i = 0; i < 2; i++)
    {
        _iTargetIndex.nI[i] = -1;
        _iTargetIndex.nJ[i] = -1;
    }

    if (matchParams(sCmd, "transpose"))
        bTranspose = true;

    if (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '='))
    {
        if (matchParams(sCmd, "target", '='))
        {
            sTarget = getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6);
            sCmd.erase(matchParams(sCmd, "target", '=')-2);
        }
        else
        {
            sTarget = getArgAtPos(sCmd, matchParams(sCmd, "t", '=')+1);
            sCmd.erase(matchParams(sCmd, "t", '=')-2);
        }
        if (sTarget.substr(0,5) == "data(")
            return false;

        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sTarget.find(iter->first+"(") != string::npos)
            {
                _iTargetIndex = parser_getIndices(sTarget.substr(sTarget.find(iter->first+"(")), _parser, _data, _option);
                sTarget = iter->first;
                break;
            }
        }
        if ((_iTargetIndex.nI[0] == -1 && !_iTargetIndex.vI.size()) || (_iTargetIndex.nJ[0] == -1 && !_iTargetIndex.vJ.size()))
            return false;
        if (_iTargetIndex.nI[1] == -1)
            _iTargetIndex.nI[1] = _iTargetIndex.nI[0];
        if (_iTargetIndex.nJ[1] == -1)
            _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0];
    }
    sToCopy = sCmd.substr(sCmd.find(' '));
    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
    {
        if (sToCopy.find(" " + iter->first+"(") != string::npos || sToCopy.find(" data(") != string::npos)
        {
            _iCopyIndex = parser_getIndices(sToCopy, _parser, _data, _option);
            if (sToCopy.find(" " + iter->first+"(") != string::npos)
            {
                _data.setCacheStatus(true);
                sToCopy = iter->first;
                if (!sTarget.length())
                    sTarget = iter->first;
            }
            else
                sToCopy = "data";
            if (!sTarget.length())
                sTarget = "cache";
            if ((_iCopyIndex.nI[0] == -1 && !_iCopyIndex.vI.size()) || (_iCopyIndex.nJ[0] == -1 && !_iCopyIndex.vJ.size()))
                return false;

            if (_iCopyIndex.nI[1] == -1)
                _iCopyIndex.nI[1] = _iCopyIndex.nI[0];
            if (_iCopyIndex.nJ[1] == -1)
                _iCopyIndex.nJ[1] = _iCopyIndex.nJ[0];
            if (_iCopyIndex.nI[1] == -2)
                _iCopyIndex.nI[1] = _data.getLines(sToCopy, false)-1;
            if (_iCopyIndex.nJ[1] == -2)
                _iCopyIndex.nJ[1] = _data.getCols(sToCopy)-1;
            if (_iTargetIndex.nI[0] == -1 && !_iTargetIndex.vI.size())
            {
                if (!bTranspose)
                {
                    _iTargetIndex.nJ[0] = _data.getCacheCols(sTarget, false);
                    if (!_iCopyIndex.vJ.size())
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + (_iCopyIndex.nJ[1]-_iCopyIndex.nJ[0])+1;
                    else
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + _iCopyIndex.vJ.size();
                    _iTargetIndex.nI[0] = 0;
                    if (!_iCopyIndex.vI.size())
                        _iTargetIndex.nI[1] = _iCopyIndex.nI[1] - _iCopyIndex.nI[0];
                    else
                        _iTargetIndex.nI[1] = _iCopyIndex.vI.size();
                }
                else
                {
                    _iTargetIndex.nI[0] = 0;
                    if (!_iCopyIndex.vJ.size())
                        _iTargetIndex.nI[1] = (_iCopyIndex.nJ[1]-_iCopyIndex.nJ[0]);
                    else
                        _iTargetIndex.nI[1] = _iCopyIndex.vJ.size();
                    _iTargetIndex.nJ[0] = _data.getCacheCols(sTarget, false);
                    if (!_iCopyIndex.vI.size())
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + (_iCopyIndex.nI[1] - _iCopyIndex.nI[0])+1;
                    else
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + _iCopyIndex.vI.size();
                }
            }
            else if (_iTargetIndex.vI.size())
            {
                if (!bTranspose)
                {
                    if (_iTargetIndex.nI[1] == -2)
                    {
                        _iTargetIndex.vI.clear();
                        if (_iCopyIndex.vI.size())
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i < _iTargetIndex.nI[0]+_iCopyIndex.vI.size(); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                        else
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i <= _iTargetIndex.nI[0]+(_iCopyIndex.nI[1]-_iCopyIndex.nI[0]); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                    }
                    if (_iTargetIndex.nJ[1] == -2)
                    {
                        _iTargetIndex.vJ.clear();
                        if (_iCopyIndex.vJ.size())
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j < _iTargetIndex.nJ[0]+_iCopyIndex.vJ.size(); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                        else
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j <= _iTargetIndex.nJ[0]+(_iCopyIndex.nJ[1]-_iCopyIndex.nJ[0]); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                    }
                }
                else
                {
                    if (_iTargetIndex.nI[1] == -2)
                    {
                        _iTargetIndex.vI.clear();
                        if (_iCopyIndex.vJ.size())
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i < _iTargetIndex.nI[0]+_iCopyIndex.vJ.size(); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                        else
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i <= _iTargetIndex.nI[0]+(_iCopyIndex.nJ[1]-_iCopyIndex.nJ[0]); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                    }
                    if (_iTargetIndex.nJ[1] == -2)
                    {
                        _iTargetIndex.vJ.clear();
                        if (_iCopyIndex.vI.size())
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j < _iTargetIndex.nJ[0]+_iCopyIndex.vI.size(); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                        else
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j <= _iTargetIndex.nJ[0]+(_iCopyIndex.nI[1]-_iCopyIndex.nI[0]); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                    }
                }
            }

            if (!bTranspose && !_iTargetIndex.vI.size())
            {
                if (_iTargetIndex.nI[1] == -2)
                {
                    if (!_iCopyIndex.vI.size())
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iCopyIndex.nI[1] - _iCopyIndex.nI[0];
                    else
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iCopyIndex.vI.size();
                }
                if (_iTargetIndex.nJ[1] == -2)
                {
                    if (!_iCopyIndex.vJ.size())
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iCopyIndex.nJ[1] - _iCopyIndex.nJ[0];
                    else
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iCopyIndex.vJ.size();
                }
            }
            else if (!_iTargetIndex.vI.size())
            {
                if (_iTargetIndex.nJ[1] == -2)
                {
                    if (!_iCopyIndex.vI.size())
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iCopyIndex.nI[1] - _iCopyIndex.nI[0];
                    else
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iCopyIndex.vI.size();
                }
                if (_iTargetIndex.nI[1] == -2)
                {
                    if (!_iCopyIndex.vJ.size())
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iCopyIndex.nJ[1] - _iCopyIndex.nJ[0];
                    else
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iCopyIndex.vJ.size();
                }
            }

            parser_CheckIndices(_iCopyIndex.nI[0], _iCopyIndex.nI[1]);
            parser_CheckIndices(_iCopyIndex.nJ[0], _iCopyIndex.nJ[1]);
            parser_CheckIndices(_iTargetIndex.nI[0], _iTargetIndex.nI[1]);
            parser_CheckIndices(_iTargetIndex.nJ[0], _iTargetIndex.nJ[1]);

            Datafile _cache;
            _cache.setCacheStatus(true);
            if (!_iCopyIndex.vI.size() && !_iCopyIndex.vJ.size())
            {
                for (long long int i = _iCopyIndex.nI[0]; i <= _iCopyIndex.nI[1]; i++)
                {
                    for (long long int j = _iCopyIndex.nJ[0]; j <= _iCopyIndex.nJ[1]; j++)
                    {
                        if (!i)
                            _cache.setHeadLineElement(j-_iCopyIndex.nJ[0], "cache", _data.getHeadLineElement(j, sToCopy));
                        if (_data.isValidEntry(i,j, sToCopy))
                            _cache.writeToCache(i-_iCopyIndex.nI[0], j-_iCopyIndex.nJ[0], "cache", _data.getElement(i,j, sToCopy));
                    }
                }
            }
            else
            {
                for (unsigned int i = 0; i < _iCopyIndex.vI.size(); i++)
                {
                    for (unsigned int j = 0; j < _iCopyIndex.vJ.size(); j++)
                    {
                        //NumeReKernel::print(_iCopyIndex.vI[i] + "  " + _iCopyIndex.vJ[j] + "  " + _data.getElement(_iCopyIndex.vI[i], _iCopyIndex.vJ[j], sToCopy) );
                        if (!i)
                            _cache.setHeadLineElement(j,"cache",_data.getHeadLineElement(_iCopyIndex.vJ[j], sToCopy));
                        if (_data.isValidEntry(_iCopyIndex.vI[i], _iCopyIndex.vJ[j], sToCopy))
                            _cache.writeToCache(i,j,"cache", _data.getElement(_iCopyIndex.vI[i], _iCopyIndex.vJ[j], sToCopy));
                    }
                }
            }


            if (!_iTargetIndex.vI.size())
            {
                for (long long int i = 0; i < _cache.getCacheLines("cache", false); i++)
                {
                    if (!bTranspose)
                    {
                        if (i > _iTargetIndex.nI[1]-_iTargetIndex.nI[0])
                            break;
                    }
                    else
                    {
                        if (i > _iTargetIndex.nJ[1]-_iTargetIndex.nJ[0])
                            break;
                    }
                    for (long long int j = 0; j < _cache.getCacheCols("cache", false); j++)
                    {
                        if (!bTranspose)
                        {
                            if (!i && !j && (!_iTargetIndex.nI[0] || _iTargetIndex.nJ[0] >= _data.getCols(sTarget)))
                            {
                                for (long long int n = 0; n < _cache.getCacheCols("cache", false); n++)
                                    _data.setHeadLineElement(n+_iTargetIndex.nJ[0], sTarget, _cache.getHeadLineElement(n,"cache"));
                            }
                            if (j > _iTargetIndex.nJ[1]-_iTargetIndex.nJ[0])
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget, _cache.getElement(i,j,"cache"));
                            else if (_data.isValidEntry(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget))
                                _data.deleteEntry(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget);
                        }
                        else
                        {
                            if (j > _iTargetIndex.nI[1]-_iTargetIndex.nI[0])
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget, _cache.getElement(i,j, "cache"));
                            else if (_data.isValidEntry(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget))
                                _data.deleteEntry(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget);
                        }
                    }
                }
            }
            else
            {
                for (long long int i = 0; i < _cache.getCacheLines("cache", false); i++)
                {
                    if (!bTranspose)
                    {
                        if (i >= _iTargetIndex.vI.size())
                            break;
                    }
                    else
                    {
                        if (i >= _iTargetIndex.vJ.size())
                            break;
                    }
                    for (long long int j = 0; j < _cache.getCacheCols("cache", false); j++)
                    {
                        if (!bTranspose)
                        {
                            if (!_iTargetIndex.vI[i] && _data.getHeadLineElement(_iTargetIndex.vJ[j],sTarget).substr(0,5) == "Spalte")
                            {
                                _data.setHeadLineElement(_iTargetIndex.vJ[j], sTarget, _cache.getHeadLineElement(j, "cache"));
                            }
                            if (j > _iTargetIndex.vJ.size())
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget, _cache.getElement(i,j,"cache"));
                            else if (_data.isValidEntry(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget))
                                _data.deleteEntry(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget);
                        }
                        else
                        {
                            if (j > _iTargetIndex.vI.size())
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget, _cache.getElement(i,j, "cache"));
                            else if (_data.isValidEntry(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget))
                                _data.deleteEntry(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget);
                        }
                    }
                }
            }
            _data.setCacheStatus(false);

            return true;
        }
    }
    return false;
}

bool moveData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    // move cache(i1:i2, j1:j2) -target=cache(i,j)
    string sToMove = "";
    string sTarget = "";
    bool bTranspose = false;

    Indices _iMoveIndex;
    Indices _iTargetIndex;
    for (int i = 0; i < 2; i++)
    {
        _iTargetIndex.nI[i] = -1;
        _iTargetIndex.nJ[i] = -1;
    }

    if (matchParams(sCmd, "transpose"))
        bTranspose = true;

    if (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '='))
    {
        if (matchParams(sCmd, "target", '='))
        {
            sTarget = getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6);
            sCmd.erase(matchParams(sCmd, "target", '=')-2);
        }
        else
        {
            sTarget = getArgAtPos(sCmd, matchParams(sCmd, "t", '=')+1);
            sCmd.erase(matchParams(sCmd, "t", '=')-2);
        }
        if (sTarget.substr(0,5) == "data(")
            return false;

        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sTarget.find(iter->first+"(") != string::npos)
            {
                _iTargetIndex = parser_getIndices(sTarget.substr(sTarget.find(iter->first+"(")), _parser, _data, _option);
                sTarget = iter->first;

                if ((_iTargetIndex.nI[0] == -1 && !_iTargetIndex.vI.size()) || (_iTargetIndex.nJ[0] == -1 && !_iTargetIndex.vJ.size()))
                    return false;
                if (_iTargetIndex.nI[1] == -1)
                    _iTargetIndex.nI[1] = _iTargetIndex.nI[0];
                if (_iTargetIndex.nJ[1] == -1)
                    _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0];
            }
        }
    }
    else
        return false;

    sToMove = sCmd.substr(sCmd.find(' '));
    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
    {
        if (sToMove.find(" " + iter->first+"(") != string::npos)
        {
            _iMoveIndex = parser_getIndices(sToMove, _parser, _data, _option);

            sToMove = iter->first;
            _data.setCacheStatus(true);
            if ((_iMoveIndex.nI[0] == -1 && !_iMoveIndex.vI.size()) || (_iMoveIndex.nJ[0] == -1 && !_iMoveIndex.vJ.size()))
                return false;

            if (_iMoveIndex.nI[1] == -1)
                _iMoveIndex.nI[1] = _iMoveIndex.nI[0];
            if (_iMoveIndex.nJ[1] == -1)
                _iMoveIndex.nJ[1] = _iMoveIndex.nJ[0];
            if (_iMoveIndex.nI[1] == -2)
                _iMoveIndex.nI[1] = _data.getLines(sToMove, false)-1;
            if (_iMoveIndex.nJ[1] == -2)
                _iMoveIndex.nJ[1] = _data.getCols(sToMove)-1;
            if (_iTargetIndex.nI[0] == -1 && !_iTargetIndex.vI.size())
            {
                if (!bTranspose)
                {
                    _iTargetIndex.nJ[0] = _data.getCacheCols(sTarget, false);
                    if (!_iMoveIndex.vJ.size())
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + (_iMoveIndex.nJ[1]-_iMoveIndex.nJ[0])+1;
                    else
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + _iMoveIndex.vJ.size();
                    _iTargetIndex.nI[0] = 0;
                    if (!_iMoveIndex.vI.size())
                        _iTargetIndex.nI[1] = _iMoveIndex.nI[1] - _iMoveIndex.nI[0];
                    else
                        _iTargetIndex.nI[1] = _iMoveIndex.vI.size();
                }
                else
                {
                    _iTargetIndex.nI[0] = 0;
                    if (!_iMoveIndex.vJ.size())
                        _iTargetIndex.nI[1] = (_iMoveIndex.nJ[1]-_iMoveIndex.nJ[0]);
                    else
                        _iTargetIndex.nI[1] = _iMoveIndex.vJ.size();
                    _iTargetIndex.nJ[0] = _data.getCacheCols(sTarget, false);
                    if (!_iMoveIndex.vI.size())
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + (_iMoveIndex.nI[1] - _iMoveIndex.nI[0])+1;
                    else
                        _iTargetIndex.nJ[1] = _data.getCacheCols(sTarget, false) + _iMoveIndex.vI.size();
                }
            }
            else if (_iTargetIndex.vI.size())
            {
                if (!bTranspose)
                {
                    if (_iTargetIndex.nI[1] == -2)
                    {
                        _iTargetIndex.vI.clear();
                        if (_iMoveIndex.vI.size())
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i < _iTargetIndex.nI[0]+_iMoveIndex.vI.size(); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                        else
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i <= _iTargetIndex.nI[0]+(_iMoveIndex.nI[1]-_iMoveIndex.nI[0]); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                    }
                    if (_iTargetIndex.nJ[1] == -2)
                    {
                        _iTargetIndex.vJ.clear();
                        if (_iMoveIndex.vJ.size())
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j < _iTargetIndex.nJ[0]+_iMoveIndex.vJ.size(); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                        else
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j <= _iTargetIndex.nJ[0]+(_iMoveIndex.nJ[1]-_iMoveIndex.nJ[0]); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                    }
                }
                else
                {
                    if (_iTargetIndex.nI[1] == -2)
                    {
                        _iTargetIndex.vI.clear();
                        if (_iMoveIndex.vJ.size())
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i < _iTargetIndex.nI[0]+_iMoveIndex.vJ.size(); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                        else
                        {
                            for (long long int i = _iTargetIndex.nI[0]; i <= _iTargetIndex.nI[0]+(_iMoveIndex.nJ[1]-_iMoveIndex.nJ[0]); i++)
                                _iTargetIndex.vI.push_back(i);
                        }
                    }
                    if (_iTargetIndex.nJ[1] == -2)
                    {
                        _iTargetIndex.vJ.clear();
                        if (_iMoveIndex.vI.size())
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j < _iTargetIndex.nJ[0]+_iMoveIndex.vI.size(); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                        else
                        {
                            for (long long int j = _iTargetIndex.nJ[0]; j <= _iTargetIndex.nJ[0]+(_iMoveIndex.nI[1]-_iMoveIndex.nI[0]); j++)
                                _iTargetIndex.vJ.push_back(j);
                        }
                    }
                }
            }

            if (!bTranspose && !_iTargetIndex.vI.size())
            {
                if (_iTargetIndex.nI[1] == -2)
                {
                    if (!_iMoveIndex.vI.size())
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iMoveIndex.nI[1] - _iMoveIndex.nI[0];
                    else
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iMoveIndex.vI.size();
                }
                if (_iTargetIndex.nJ[1] == -2)
                {
                    if (!_iMoveIndex.vJ.size())
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iMoveIndex.nJ[1] - _iMoveIndex.nJ[0];
                    else
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iMoveIndex.vJ.size();
                }
            }
            else if (!_iTargetIndex.vI.size())
            {
                if (_iTargetIndex.nJ[1] == -2)
                {
                    if (!_iMoveIndex.vI.size())
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iMoveIndex.nI[1] - _iMoveIndex.nI[0];
                    else
                        _iTargetIndex.nJ[1] = _iTargetIndex.nJ[0] + _iMoveIndex.vI.size();
                }
                if (_iTargetIndex.nI[1] == -2)
                {
                    if (!_iMoveIndex.vJ.size())
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iMoveIndex.nJ[1] - _iMoveIndex.nJ[0];
                    else
                        _iTargetIndex.nI[1] = _iTargetIndex.nI[0] + _iMoveIndex.vJ.size();
                }
            }

            parser_CheckIndices(_iMoveIndex.nI[0], _iMoveIndex.nI[1]);
            parser_CheckIndices(_iMoveIndex.nJ[0], _iMoveIndex.nJ[1]);
            parser_CheckIndices(_iTargetIndex.nI[0], _iTargetIndex.nI[1]);
            parser_CheckIndices(_iTargetIndex.nJ[0], _iTargetIndex.nJ[1]);

            Datafile _cache;
            _cache.setCacheStatus(true);
            if (!_iMoveIndex.vI.size() && !_iMoveIndex.vJ.size())
            {
                for (long long int i = _iMoveIndex.nI[0]; i <= _iMoveIndex.nI[1]; i++)
                {
                    for (long long int j = _iMoveIndex.nJ[0]; j <= _iMoveIndex.nJ[1]; j++)
                    {
                        if (!i)
                            _cache.setHeadLineElement(j-_iMoveIndex.nJ[0], "cache", _data.getHeadLineElement(j, sToMove));
                        if (_data.isValidEntry(i,j, sToMove))
                        {
                            _cache.writeToCache(i-_iMoveIndex.nI[0], j-_iMoveIndex.nJ[0], "cache", _data.getElement(i,j, sToMove));
                            _data.deleteEntry(i,j,sToMove);
                        }
                    }
                }
            }
            else
            {
                for (unsigned int i = 0; i < _iMoveIndex.vI.size(); i++)
                {
                    for (unsigned int j = 0; j < _iMoveIndex.vJ.size(); j++)
                    {
                        if (!i)
                            _cache.setHeadLineElement(j,"cache",_data.getHeadLineElement(_iMoveIndex.vJ[j], sToMove));
                        if (_data.isValidEntry(_iMoveIndex.vI[i], _iMoveIndex.vJ[j], sToMove))
                        {
                            _cache.writeToCache(i,j,"cache", _data.getElement(_iMoveIndex.vI[i], _iMoveIndex.vJ[j], sToMove));
                            _data.deleteEntry(_iMoveIndex.vI[i],_iMoveIndex.vJ[j], sToMove);
                        }
                    }
                }
            }


            if (!_iTargetIndex.vI.size())
            {
                for (long long int i = 0; i < _cache.getCacheLines("cache", false); i++)
                {
                    if (!bTranspose)
                    {
                        if (i > _iTargetIndex.nI[1]-_iTargetIndex.nI[0])
                            break;
                    }
                    else
                    {
                        if (i > _iTargetIndex.nJ[1]-_iTargetIndex.nJ[0])
                            break;
                    }
                    for (long long int j = 0; j < _cache.getCacheCols("cache", false); j++)
                    {
                        if (!bTranspose)
                        {
                            if (!i && !j && (!_iTargetIndex.nI[0] || _iTargetIndex.nJ[0] >= _data.getCols(sTarget)))
                            {
                                for (long long int n = 0; n < _cache.getCacheCols("cache", false); n++)
                                    _data.setHeadLineElement(n+_iTargetIndex.nJ[0], sTarget, _cache.getHeadLineElement(n,"cache"));
                            }
                            if (j > _iTargetIndex.nJ[1]-_iTargetIndex.nJ[0])
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget, _cache.getElement(i,j,"cache"));
                            else if (_data.isValidEntry(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget))
                                _data.deleteEntry(i+_iTargetIndex.nI[0], j+_iTargetIndex.nJ[0], sTarget);
                        }
                        else
                        {
                            if (j > _iTargetIndex.nI[1]-_iTargetIndex.nI[0])
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget, _cache.getElement(i,j, "cache"));
                            else if (_data.isValidEntry(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget))
                                _data.deleteEntry(j+_iTargetIndex.nI[0], i+_iTargetIndex.nJ[0], sTarget);
                        }
                    }
                }
            }
            else
            {
                for (long long int i = 0; i < _cache.getCacheLines("cache", false); i++)
                {
                    if (!bTranspose)
                    {
                        if (i >= _iTargetIndex.vI.size())
                            break;
                    }
                    else
                    {
                        if (i >= _iTargetIndex.vJ.size())
                            break;
                    }
                    for (long long int j = 0; j < _cache.getCacheCols("cache", false); j++)
                    {
                        if (!bTranspose)
                        {
                            if (!_iTargetIndex.vI[i] && _data.getHeadLineElement(_iTargetIndex.vJ[j],sTarget).substr(0,5) == "Spalte")
                            {
                                _data.setHeadLineElement(_iTargetIndex.vJ[j], sTarget, _cache.getHeadLineElement(j, "cache"));
                            }
                            if (j > _iTargetIndex.vJ.size())
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget, _cache.getElement(i,j,"cache"));
                            else if (_data.isValidEntry(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget))
                                _data.deleteEntry(_iTargetIndex.vI[i], _iTargetIndex.vJ[j], sTarget);
                        }
                        else
                        {
                            if (j > _iTargetIndex.vI.size())
                                break;
                            if (_cache.isValidEntry(i,j, "cache"))
                                _data.writeToCache(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget, _cache.getElement(i,j, "cache"));
                            else if (_data.isValidEntry(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget))
                                _data.deleteEntry(_iTargetIndex.vJ[j], _iTargetIndex.vI[i], sTarget);
                        }
                    }
                }
            }

            _data.setCacheStatus(false);
            return true;
        }
    }
    return false;
}


bool sortData(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    vector<int> vSortIndex;
    string sCache = sCmd.substr(sCmd.find(' ')+1);
    sCache.erase(getMatchingParenthesis(sCache)+1);
    Indices _idx = parser_getIndices(sCache, _parser, _data, _option);

    if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, "", _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);

    sCache.erase(sCache.find('('));

    if (_idx.nI[1] == -2)
        _idx.nI[1] = _data.getLines(sCache, false);
    if (_idx.nJ[1] == -2)
        _idx.nJ[1] = _data.getCols(sCache, false);

    vSortIndex = _data.sortElements(sCache, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1], sCmd.substr(5+sCache.length()));

    if (vSortIndex.size())
    {
        vector<double> vDoubleSortIndex;
        for (size_t i = 0; i < vSortIndex.size(); i++)
            vDoubleSortIndex.push_back(vSortIndex[i]);
        sCmd = "_~sortIndex[]";
        _parser.SetVectorVar(sCmd, vDoubleSortIndex);
    }
    else
        sCmd.clear();
    return true;
}


bool writeToFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
    fstream fFile;
    string sFileName = "";
    string sExpression = "";
    string sParams = "";
    string sArgument = "";
    bool bAppend = false;
    bool bTrunc = true;
    bool bNoQuotes = false;

    if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
    {
        if (sCmd.find("-set") != string::npos)
        {
            sParams = sCmd.substr(sCmd.find("-set"));
            sCmd.erase(sCmd.find("-set"));
        }
        else
        {
            sParams = sCmd.substr(sCmd.find("--"));
            sCmd.erase(sCmd.find("--"));
        }

        if (matchParams(sParams, "file", '='))
        {
            if (_data.containsStringVars(sParams))
                _data.getStringValues(sParams);
            addArgumentQuotes(sParams, "file");
            BI_parseStringArgs(sParams, sFileName, _parser, _data, _option);
            StripSpaces(sFileName);
            if (!sFileName.length())
                return false;
            sFileName = _data.ValidFileName(sFileName, ".txt");
            if (sFileName.substr(sFileName.rfind('.')) == ".nprc" || sFileName.substr(sFileName.rfind('.')) == ".nscr" || sFileName.substr(sFileName.rfind('.')) == ".ndat")
            {
                string sErrorToken;
                if (sFileName.substr(sFileName.rfind('.')) == ".nprc")
                    sErrorToken = "NumeRe-Prozedur";
                else if (sFileName.substr(sFileName.rfind('.')) == ".nscr")
                    sErrorToken = "NumeRe-Script";
                else if (sFileName.substr(sFileName.rfind('.')) == ".ndat")
                    sErrorToken = "NumeRe-Datenfile";
                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sErrorToken);
            }
        }
        if (matchParams(sParams, "noquotes") || matchParams(sParams, "nq"))
            bNoQuotes = true;
        if (matchParams(sParams, "mode", '='))
        {
            if (getArgAtPos(sParams, matchParams(sParams, "mode", '=')+4) == "append"
                || getArgAtPos(sParams, matchParams(sParams, "mode", '=')+4) == "app")
                bAppend = true;
            else if (getArgAtPos(sParams, matchParams(sParams, "mode", '=')+4) == "trunc")
                bTrunc = true;
            else if (getArgAtPos(sParams, matchParams(sParams, "mode", '=')+4) == "override"
                || getArgAtPos(sParams, matchParams(sParams, "mode", '=')+4) == "overwrite")
            {
                bAppend = false;
                bTrunc = false;
            }
            else
                return false;
        }
    }
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);
    sExpression = sCmd.substr(findCommand(sCmd).nPos + findCommand(sCmd).sString.length());
    if (containsStrings(sExpression) || _data.containsStringVars(sExpression))
    {
        sExpression += " -komq";
        string sDummy = "";
        parser_StringParser(sExpression, sDummy, _data, _parser, _option, true);
    }
    else
        throw SyntaxError(SyntaxError::NO_STRING_FOR_WRITING, sCmd, SyntaxError::invalid_position);
    if (bAppend)
        fFile.open(sFileName.c_str(), ios_base::app | ios_base::out | ios_base::ate);
    else if (bTrunc)
        fFile.open(sFileName.c_str(), ios_base::trunc | ios_base::out);
    else
    {
        if (!BI_FileExists(sFileName))
            ofstream fTemp(sFileName.c_str());
        fFile.open(sFileName.c_str());
    }
    if (fFile.fail())
    {
        //sErrorToken = sFileName;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sCmd, SyntaxError::invalid_position, sFileName);
    }

    if (!sExpression.length() || sExpression == "\"\"")
        throw SyntaxError(SyntaxError::NO_STRING_FOR_WRITING, sCmd, SyntaxError::invalid_position);

    while (sExpression.length())
    {
        sArgument = getNextArgument(sExpression, true);
        StripSpaces(sArgument);
        if (bNoQuotes && sArgument[0] == '"' && sArgument[sArgument.length()-1] == '"')
            sArgument = sArgument.substr(1,sArgument.length()-2);
        if (!sArgument.length() || sArgument == "\"\"")
            continue;
        while (sArgument.find("\\\"") != string::npos)
        {
            sArgument.erase(sArgument.find("\\\""), 1);
        }
        if (sArgument.length() >= 2 && sArgument.substr(sArgument.length()-2) == "\\ ")
            sArgument.pop_back();

        fFile << sArgument << endl;
        if (sExpression == ",")
            break;
    }

    if (fFile.is_open())
        fFile.close();
    return true;
}

bool readFromFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
    string sFileName = "";
    string sInput = "";
    string sExt = ".txt";
    string sCommentEscapeSequence = "";
    string sParams = "";
    if (sCmd.rfind('-') != string::npos && !isInQuotes(sCmd, sCmd.rfind('-')))
    {
        sParams = sCmd.substr(sCmd.rfind('-'));
        sCmd.erase(sCmd.rfind('-'));
    }
    ifstream fFile;
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());

    //NumeReKernel::print(sCmd + endl + sParams );
    if (matchParams(sParams, "comments", '='))
    {
        sCommentEscapeSequence = getArgAtPos(sParams, matchParams(sParams, "comments", '=')+8);
        if (sCommentEscapeSequence != " ")
            StripSpaces(sCommentEscapeSequence);
        while (sCommentEscapeSequence.find("\\t") != string::npos)
            sCommentEscapeSequence.replace(sCommentEscapeSequence.find("\\t"), 2, "\t");
    }
    if (matchParams(sParams, "file", '='))
    {
        if (_data.containsStringVars(sParams))
            _data.getStringValues(sParams);
        addArgumentQuotes(sParams, "file");
        //NumeReKernel::print(sParams );
        BI_parseStringArgs(sParams, sFileName, _parser, _data, _option);
        StripSpaces(sFileName);
        if (!sFileName.length())
            return false;
        if (sFileName.find('.') != string::npos)
        {
            unsigned int nPos = sFileName.find_last_of('/');
            if (nPos == string::npos)
                nPos = 0;
            if (sFileName.find('\\', nPos) != string::npos)
                nPos = sFileName.find_last_of('\\');
            if (sFileName.find('.', nPos) != string::npos)
                sExt = sFileName.substr(sFileName.rfind('.'));

            if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
            {
                //sErrorToken = sExt;
                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
            }
            _fSys.declareFileType(sExt);
        }
        sFileName = _fSys.ValidFileName(sFileName, sExt);
    }
    else
    {
        if (_data.containsStringVars(sCmd))
            _data.getStringValues(sCmd);
        sFileName = sCmd.substr(sCmd.find_first_not_of(' ', 4));
        StripSpaces(sFileName);
        if (!sFileName.length())
            return false;
        if (containsStrings(sFileName))
        {
            sFileName += " -nq";
            parser_StringParser(sFileName, sCmd, _data, _parser, _option, true);
        }
        if (sFileName.find('.') != string::npos)
        {
            unsigned int nPos = sFileName.find_last_of('/');
            if (nPos == string::npos)
                nPos = 0;
            if (sFileName.find('\\', nPos) != string::npos)
                nPos = sFileName.find_last_of('\\');
            if (sFileName.find('.', nPos) != string::npos)
                sExt = sFileName.substr(sFileName.rfind('.'));

            if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
            {
                //sErrorToken = sExt;
                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
            }
            _fSys.declareFileType(sExt);
        }
        sFileName = _fSys.ValidFileName(sFileName, sExt);
    }
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    sCmd.clear();

    fFile.open(sFileName.c_str());
    if (fFile.fail())
    {
        //sErrorToken = sFileName;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFileName);
    }

    while (!fFile.eof())
    {
        getline(fFile, sInput);
        //StripSpaces(sInput);
        if (!sInput.length() || sInput == "\"\"" || sInput == "\"")
            continue;
        if (sCommentEscapeSequence.length() && sInput.find(sCommentEscapeSequence) != string::npos)
        {
            sInput.erase(sInput.find(sCommentEscapeSequence));
            if (!sInput.length() || sInput == "\"\"" || sInput == "\"")
                continue;
        }
        if (sInput.front() != '"')
            sInput = '"' + sInput;
        if (sInput.back() == '\\')
            sInput += ' ';
        if (sInput.back() != '"')
            sInput += '"';
        for (unsigned int i = 1; i < sInput.length()-1; i++)
        {
            if (sInput[i] == '\\')
                sInput.insert(i+1, 1, ' ');
            if (sInput[i] == '"' && sInput[i-1] != '\\')
                sInput.insert(i, 1, '\\');
        }
        sCmd += sInput + ",";
    }
    if (sCmd.length())
        sCmd.pop_back();
    else
        sCmd = "\"\"";
    //NumeReKernel::print(sCmd);
    //sCmd = sCmd;
    fFile.close();

    return true;
}

bool readImage(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
    string sFileName = "";
    string sInput = "";
    string sExt = ".bmp";
    string sParams = "";
    string sTargetCache = "image";
    Indices _idx;

    sTargetCache = parser_evalTargetExpression(sCmd, "image", _idx, _parser, _data, _option);
    if (sCmd.rfind('-') != string::npos && !isInQuotes(sCmd, sCmd.rfind('-')))
    {
        sParams = sCmd.substr(sCmd.rfind('-'));
        sCmd.erase(sCmd.rfind('-'));
    }
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());

    if (matchParams(sParams, "file", '='))
    {
        if (_data.containsStringVars(sParams))
            _data.getStringValues(sParams);
        addArgumentQuotes(sParams, "file");
        BI_parseStringArgs(sParams, sFileName, _parser, _data, _option);
        StripSpaces(sFileName);
        if (!sFileName.length())
            return false;
        if (sFileName.find('.') != string::npos)
        {
            unsigned int nPos = sFileName.find_last_of('/');
            if (nPos == string::npos)
                nPos = 0;
            if (sFileName.find('\\', nPos) != string::npos)
                nPos = sFileName.find_last_of('\\');
            if (sFileName.find('.', nPos) != string::npos)
                sExt = sFileName.substr(sFileName.rfind('.'));

            if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
            {
                //sErrorToken = sExt;
                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
            }
            _fSys.declareFileType(sExt);
        }
        sFileName = _fSys.ValidFileName(sFileName, sExt);
    }
    else
    {
        if (_data.containsStringVars(sCmd))
            _data.getStringValues(sCmd);
        sFileName = sCmd.substr(sCmd.find_first_not_of(' ', 6));
        StripSpaces(sFileName);
        if (!sFileName.length())
            return false;
        if (containsStrings(sFileName))
        {
            sFileName += " -nq";
            parser_StringParser(sFileName, sCmd, _data, _parser, _option, true);
        }
        if (sFileName.find('.') != string::npos)
        {
            unsigned int nPos = sFileName.find_last_of('/');
            if (nPos == string::npos)
                nPos = 0;
            if (sFileName.find('\\', nPos) != string::npos)
                nPos = sFileName.find_last_of('\\');
            if (sFileName.find('.', nPos) != string::npos)
                sExt = sFileName.substr(sFileName.rfind('.'));

            if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
            {
                //sErrorToken = sExt;
                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
            }
            _fSys.declareFileType(sExt);
        }
        sFileName = _fSys.ValidFileName(sFileName, sExt);
    }
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    wxInitAllImageHandlers();

    wxImage image;

    if (!image.LoadFile(sFileName, wxBITMAP_TYPE_ANY))
    {
        //sErrorToken = sFileName;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFileName);
    }

    int nWidth = image.GetWidth();
    int nHeight = image.GetHeight();
    unsigned char* imageData = image.GetData();

    /*if (!_data.isCacheElement("image"))
        _data.addCache("image", _option);*/
    //int nFirstColumn = _data.getCols(sTargetCache, false);

    if (_idx.nI[1] == -2)
        _idx.nI[1] = _idx.nI[0] + nWidth;
    if (_idx.nJ[1] == -2)
        _idx.nJ[1] = _idx.nJ[0] + 2 + nHeight;

    for (int i = 0; i < nWidth; i++)
    {
        if (_idx.nI[0]+i > _idx.nI[1])
            break;
        _data.writeToCache(_idx.nI[0]+i, _idx.nJ[0], sTargetCache, i+1);
    }
    for (int i = 0; i < nHeight; i++)
    {
        if (_idx.nI[0]+i > _idx.nI[1])
            break;
        _data.writeToCache(_idx.nI[0]+i, _idx.nJ[0]+1, sTargetCache, i+1);
    }

    int iData;
    for (int j = 0; j < nHeight; j++)
    {
        iData = 0;
        if (_idx.nJ[0]+2+j > _idx.nJ[1])
            break;
        for (int i = 0; i < nWidth; i++)
        {
            if (_idx.nI[0]+i > _idx.nI[1])
                break;
            _data.writeToCache(_idx.nI[0]+i, _idx.nJ[0]+2+(nHeight-j-1), sTargetCache, imageData[j*3*nWidth + iData]/3.0 + imageData[j*3*nWidth + iData+1]/3.0 + imageData[j*3*nWidth + iData+2]/3.0);
            iData += 3;
        }
    }


    return true;
}

