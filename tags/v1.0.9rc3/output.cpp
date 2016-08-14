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


#include "output.hpp"

/*
 * Realisierung der Klasse 'Output'
 */

// --> Standardkonstruktor <--
Output::Output() : FileSystem()
{
	Output::reset();			// Aus Gruenden der Konsistenz ist es sinnvoll die Resetmethode aufzurufen, damit die Defaultwerte
								//		nur an einer Stelle im Code geaenderte werden muessen
}

// --> Allgemeiner Konstruktor <--
Output::Output(bool bStatus, string sFile) : FileSystem()
{
	Output::reset();			// Hier rufen wir ebenfalls die reset()-Methode und ueberschreiben dann lieber die nicht-defaults
	bFile = bStatus;
	if (bStatus) 				// Wenn ein Datenfile verwendet werden soll, erzeuge dieses sogleich
	{
		setFileName(sFile);		// sFile wird nur in diesem Fall uebernommen, damit hier so Schlauberger, die Output _out(false,0); aufrufen
								// 		keinen Muell machen koennen
		start();				// Datenfile anlegen
	}
	sPath = "save";
	return;
}

// --> Destruktor <--
Output::~Output()
{
	if (bFile == true && bFileOpen == true)
	{
		end();					// Schliesse Datei, falls eine geoeffnet ist
	}
	return;
}

// --> Methoden <--
void Output::setStatus(bool bStatus)
{
	if (bFile == true && bFileOpen == true)
		end();					// Falls zuvor bereits ein Datenfile erzeugt wurde, schliesse dieses zuerst
	bFile = bStatus;			// Weise den neuen Wert an den Bool bFile zu.
	return;
}

// --> setzt Output auf die Defaultwerte zurueck <--
void Output::reset()
{
	if(bFile && bFileOpen)
		end();
	bFile = false;
	bFileOpen = false;
	bCompact = false;
	bSumBar = false;
	bPrintTeX = false;
	bPrintCSV = false;
	sPluginName = "";
	sFileName = "";
	sCommentLine = "";
	sPluginPrefix = "data";
	generateFileName();
	return;
}

string Output::replaceTeXControls(const string& _sText)
{
    string sReturn = _sText;

    for (unsigned int i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == 'Ä' || sReturn[i] == (char)142)
            sReturn.replace(i,1,"\\\"A");
        if (sReturn[i] == 'ä' || sReturn[i] == (char)132)
            sReturn.replace(i,1,"\\\"a");
        if (sReturn[i] == 'Ö' || sReturn[i] == (char)153)
            sReturn.replace(i,1,"\\\"O");
        if (sReturn[i] == 'ö' || sReturn[i] == (char)148)
            sReturn.replace(i,1,"\\\"o");
        if (sReturn[i] == 'Ü' || sReturn[i] == (char)154)
            sReturn.replace(i,1,"\\\"U");
        if (sReturn[i] == 'ü' || sReturn[i] == (char)129)
            sReturn.replace(i,1,"\\\"u");
        if (sReturn[i] == 'ß' || sReturn[i] == (char)225)
            sReturn.replace(i,1,"\\ss");
        if (sReturn[i] == '°' || sReturn[i] == (char)248)
            sReturn.replace(i,1,"$^\\circ$");
        if (sReturn[i] == (char)196 || sReturn[i] == (char)249)
            sReturn.replace(i,1,"\\pm");
        if (sReturn[i] == (char)171 || sReturn[i] == (char)174)
            sReturn.replace(i,1,"\"<");
        if (sReturn[i] == (char)187 || sReturn[i] == (char)175)
            sReturn.replace(i,1,"\">");
    }
    return sReturn;
}

// --> Setzt den Wert des Kompakte-Ausgabe-Booleans <--
void Output::setCompact(bool _bCompact)
{
    bCompact = _bCompact;
    return;
}

bool Output::isFile() const
{
	return bFile;				// Ausgabe an Datei: TRUE/FALSE
}


void Output::setFileName(string sFile)
{
	if (bFileOpen)				// Ist bereits eine Datei offen? Besser wir schliessen sie vorher.
	{
		end();
	}
	if (sFile != "0")		// Ist sFile != 0, pruefe und korrigiere sFile und weise dann an
	{							//		sFileName zu, sonst verwende den Default
        if (sFile.rfind('.') != string::npos
            && (sFile.substr(sFile.rfind('.')) == ".ndat"
                    || sFile.substr(sFile.rfind('.')) == ".nprc"
                    || sFile.substr(sFile.rfind('.')) == ".nscr"))
            sFile = sFile.substr(0,sFile.rfind('.'));
		sFileName = FileSystem::ValidFileName(sFile);	// Rufe die Methode der PARENT-Klasse auf, um den Namen zu pruefen
		if (sFileName.substr(sFileName.rfind('.')) == ".tex")
            bPrintTeX = true;
        else if (sFileName.substr(sFileName.rfind('.')) == ".csv")
            bPrintCSV = true;
	}
	else
	{
		generateFileName();		// Generiere einen Dateinamen aus data_YYYY-MM-DD-hhmmss.dat
	}
	return;
}


string Output::getFileName() const
{
	return sFileName;			// Gib den aktuellen Dateinamen zurueck
}


void Output::start()
{
	if (bFile == true && bFileOpen == false)
	{
		file_out.open(sFileName.c_str(), ios_base::out | ios_base::trunc);	// Oeffne die Datei
		bFileOpen = true;		// Setze den Boolean auf TRUE
		print_legal();			// Schreibe das Copyright
	}

	return;						// Mach nichts, wenn gar nicht in eine Datei geschrieben werden soll,
								// 		oder die Datei bereits geoeffnet ist
}


void Output::setPluginName(string _sPluginName)
{
	sPluginName = _sPluginName;	// Setze sPluginName = _sPluginName
	return;
}


void Output::setCommentLine(string _sCommentLine)
{
	sCommentLine = _sCommentLine;	// setze sCommentLine = _sCommentLine;
	return;
}


string Output::getPluginName() const
{
	return sPluginName;			// gib sPlugin zurueck
}


string Output::getCommentLine() const
{
	return sCommentLine;		// gib sCommentLine zurueck
}


void Output::print_legal()		// Pro forma Kommentare in die Datei
{
    if (bPrintCSV)
        return;
    string sCommentSign = "#";
    if (bPrintTeX)
        sCommentSign = "%";
    string sBuild = AutoVersion::YEAR;
    sBuild += "-";
    sBuild += AutoVersion::MONTH;
    sBuild += "-";
    sBuild += AutoVersion::DATE;
	print(sCommentSign);
	print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE1"));
	print(sCommentSign + " NumeRe: Framework für Numerische Rechnungen");
	print(sCommentSign + "=============================================");
	print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE2", sVersion, sBuild));
	print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE3", sBuild.substr(0,4)));
	print(sCommentSign + "");
	print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE4", getDate(false)));
	print(sCommentSign + "");
	if (bPrintTeX)
        print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_TEX"));
    else
        print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_STD"));
	print(sCommentSign);
	return;
}

void Output::end()
{
	if (bFile == true && bFileOpen == true)
	{
		file_out.close();		// Schliesse die Datei
		bFileOpen = false;		// Setze den Boolean auf FALSE
	}
	return;						// Mach nichts, wenn gar nicht in eine Datei geschrieben wurde,
								//		oder die Datei bereits geschlossen ist
}


void Output::print(string sOutput)
{
    if (!bFile)
    {
        for (unsigned int i = 0; i < sOutput.length(); i++)
        {
            if (sOutput[i] == 'Ä')
                sOutput[i] = (char)142;
            else if (sOutput[i] == 'ä')
                sOutput[i] = (char)132;
            else if (sOutput[i] == 'Ö')
                sOutput[i] = (char)153;
            else if (sOutput[i] == 'ö')
                sOutput[i] = (char)148;
            else if (sOutput[i] == 'Ü')
                sOutput[i] = (char)154;
            else if (sOutput[i] == 'ü')
                sOutput[i] = (char)129;
            else if (sOutput[i] == 'ß')
                sOutput[i] = (char)225;
            else if (sOutput[i] == '°')
                sOutput[i] = (char)248;
            else if (sOutput[i] == (char)249)
                sOutput[i] = (char)196;
            else if (sOutput[i] == (char)171)
                sOutput[i] = (char)174;
            else if (sOutput[i] == (char)187)
                sOutput[i] = (char)175;
            else
                continue;
        }
	}
	if (sOutput.find("---------") != string::npos || sOutput.find("<<SUMBAR>>") != string::npos)
    {
        if (sOutput.find("<<SUMBAR>>") != string::npos)
        {
            unsigned int nLength = sOutput.length();
            if (!bFile)
                sOutput.assign(nLength, (char)196);
            else
                sOutput.assign(nLength, '-');
        }
        else if (!bFile)
            sOutput.assign(sOutput.length(), (char)196);
        if (bPrintTeX)
            sOutput = "\\midrule";
        bSumBar = true;
	}
	if (bFile)
	{
		if (!bFileOpen)
		{
			start();			// Wenn in eine Datei geschrieben werden soll, aber diese noch nicht geoeffnet wurde,
								// 		rufe die Methode start() auf.
		}
		if (file_out.fail())	// Fehlermeldung, falls nicht in die Datei geschrieben werden kann
		{
			cerr << "*************************************************" << endl;
			cerr << "|-> " << toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE1")) << endl;
			cerr << "|-> " << toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE2")) << endl;
			cerr << "|   \"" << sFileName << "\"" << endl;
			cerr << "|   " << toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE3")) << endl;
			cerr << "|-> " << toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE4")) << endl;
			cerr << "*************************************************" << endl;
			end();				// Schliesse ggf. die Datei
			bFile = false;		// Setze den Boolean auf FALSE
			print(sOutput);		// Wiederhole den Aufruf dieser Methode und schreibe von nun an auf die Konsole
		}
		else if (bSumBar && !bPrintTeX && !bPrintCSV)					// Ausgabe in die Datei
			file_out << "#" << sOutput.substr(1) << endl;
        else
            file_out << sOutput << endl;
	}
	else if (bSumBar)						// Ausgabe auf die Konsole. Kann in Linux mit dem Umlenker '>' trotzdem in eine Datei ausgegeben werden.
	{
		cout << "|   " << sOutput.substr(4) << endl;
	}
	else
        cout << sOutput << endl;
	return;
}


void Output::generateFileName()	// Generiert einen Dateinamen auf Basis des aktuellen Datums
{								//		Der Dateinamen hat das Format: data_YYYY-MM-DD_hhmmss.dat
	string sTime;
	if (sPath.find('"') != string::npos)
        sTime = sPath.substr(1,sPath.length()-2);
    else
        sTime = sPath;

    while (sTime.find('\\') != string::npos)
        sTime[sTime.find('\\')] = '/';
    sTime += "/" + sPluginPrefix + "_";		// Prefix laden
	sTime += getDate(true);		// Datum aus der Klasse laden
	sTime += ".dat";
	sFileName = sTime;			// Dateinamen an sFileName zuweisen
	return;
}


string Output::getDate(bool bForFile)	// Der Boolean entscheidet, ob ein Dateinamen-Datum oder ein "Kommentar-Datum" gewuenscht ist
{
	time_t now = time(0);		// Aktuelle Zeit initialisieren
	tm *ltm = localtime(&now);
	ostringstream Temp_str;
	Temp_str << 1900+ltm->tm_year << "-"; //YYYY-
	if(1+ltm->tm_mon < 10)		// 0, falls Monat kleiner als 10
		Temp_str << "0";
	Temp_str << 1+ltm->tm_mon << "-"; // MM-
	if(ltm->tm_mday < 10)		// 0, falls Tag kleiner als 10
		Temp_str << "0";
	Temp_str << ltm->tm_mday; 	// DD
	if(bForFile)
		Temp_str << "_";		// Unterstrich im Dateinamen
	else
		Temp_str << ", um ";	// Komma im regulaeren Datum
	if(ltm->tm_hour < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_hour; 	// hh
	if(!bForFile)
		Temp_str << ":";		// ':' im regulaeren Datum
	if(ltm->tm_min < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_min;	// mm
	if(!bForFile)
		Temp_str << ":";
	if(ltm->tm_sec < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_sec;	// ss
	return Temp_str.str();
}


void Output::format(string** _sMatrix, long long int _nCol, long long int _nLine, Settings& _option, bool bDontAsk, int nHeadLineCount)
{
    if (!nHeadLineCount)
        nHeadLineCount = 1;
	unsigned int nLongest[_nCol];		// Int fuer die laengste Zeichenkette: unsigned da string::length() einen unsigned zurueck gibt
	unsigned int nLen = 0;			// Int fuer die aktuelle Laenge: dito
	string sPrint = "";				// String fuer die endgueltige Ausgabe
	string sLabel = sFileName;
	if (sLabel.find('/') != string::npos)
        sLabel.erase(0,sLabel.rfind('/')+1);
    if (sLabel.find(".tex") != string::npos)
        sLabel.erase(sLabel.rfind(".tex"));
    while (sLabel.find(' ') != string::npos)
        sLabel[sLabel.find(' ')] = '_';
	string cRerun = "";				// String fuer den erneuten Aufruf
									// Wegen dem Rueckgabewert von string::length() sind alle Schleifenindices unsigned

	if(_option.getbDebug())
		cerr << "|-> DEBUG: starte _out.format..." << endl;


    if ((!bCompact || _nLine < 12) && !bPrintCSV)
    {
        // --> Laufe durch jedes Element der Tabelle <--
        for (long long int j = 0; j < _nCol; j++)
        {
            nLongest[j] = 0;
            for (long long int i = 0; i < _nLine; i++)
            {
                if (i >= nHeadLineCount && bPrintTeX)
                {
                    if (_sMatrix[i][j].find('e') != string::npos)
                    {
                        _sMatrix[i][j] = _sMatrix[i][j].substr(0,_sMatrix[i][j].find('e'))
                                + "\\cdot10^{"
                                + (_sMatrix[i][j][_sMatrix[i][j].find('e')+1] == '-' ? "-" : "")
                                + _sMatrix[i][j].substr(_sMatrix[i][j].find_first_not_of('0', _sMatrix[i][j].find('e')+2))
                                + "}";
                    }
                    if (_sMatrix[i][j].find('%') != string::npos)
                    {
                        _sMatrix[i][j] = _sMatrix[i][j].substr(0,_sMatrix[i][j].find('%'))
                                + "\\"
                                + _sMatrix[i][j].substr(_sMatrix[i][j].find('%'));
                    }
                    if (_sMatrix[i][j].find("+/-") != string::npos)
                    {
                        _sMatrix[i][j] = _sMatrix[i][j].substr(0,_sMatrix[i][j].find("+/-"))
                                + "\\pm"
                                + _sMatrix[i][j].substr(_sMatrix[i][j].find("+/-")+3);
                    }
                }
                if (bPrintTeX)
                    _sMatrix[i][j] = replaceTeXControls(_sMatrix[i][j]);
                nLen = _sMatrix[i][j].length(); // Erhalte die Laenge der aktuellen Zeichenkette
                if (nLen > nLongest[j])
                    nLongest[j] = nLen;	// Weise neue Laenge an nLongest zu, falls nLen > nLongest
            }
            nLongest[j] += 2;
        }
	}
	else if (!bPrintCSV)
	{
        for (long long int j = 0; j < _nCol; j++)
        {
            nLongest[j] = 0;
            for (long long int i = 0; i < _nLine; i++)
            {
                if (i < 5 || i >= _nLine - 5)
                {
                    nLen = _sMatrix[i][j].length();
                    if (nLen > nLongest[j])
                        nLongest[j] = nLen;
                }
            }
            nLongest[j] += 2;
        }
	}



	if (_option.getbDebug())
		cerr << "|-> DEBUG: nLongest = " << nLongest[0] << endl;
    if (bCompact)
    {
        for (long long int j = 0; j < _nCol; j++)
        {
            if (nLongest[j] < 7)
                nLongest[j] = 7;
        }
    }
	nLen = 0;
	for (long long int j = 0; j < _nCol; j++)
        nLen += nLongest[j];

    if (bFile && !bPrintTeX && !bPrintCSV)
    {
        print("# " + _lang.get("OUTPUT_FORMAT_COMMENTLINE", sPluginName));	// Informative Ausgabe
        print("#");
    }
    else if (bPrintTeX && !bPrintCSV)
    {
        print("% " + _lang.get("OUTPUT_FORMAT_COMMENTLINE", sPluginName));
        print("%");
    }

    if (bPrintTeX)
    {
        if (_nLine < 30)
        {
            print("\\begin{table}[htb]");
            print("\\centering");
            sPrint = "\\begin{tabular}{";
            for (long long int j = 0; j < _nCol; j++)
                sPrint += "c";
            sPrint += "}";
            print(sPrint);
            print("\\toprule");
        }
        else
        {
            sPrint = "\\begin{longtable}{";
            for (long long int j = 0; j < _nCol; j++)
                sPrint += "c";
            sPrint += "}";
            print(sPrint);
            print("\\caption{" + _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
            print("\\label{tab:" + sLabel + "}\\\\");
            print("\\toprule");
            sPrint = "";
            for (int i = 0; i < nHeadLineCount; i++)
            {
                for (long long int j = 0; j < _nCol; j++)
                    sPrint += _sMatrix[i][j] + " & ";
                sPrint = sPrint.substr(0,sPrint.length()-2) + "\\\\\n";
            }
            for (unsigned int i = 0; i < sPrint.length(); i++)
            {
                if (sPrint[i] == '_')
                    sPrint[i] = ' ';
            }
            print(sPrint);
            print("\\midrule");
            print("\\endfirsthead");
            print("\\caption{"+_lang.get("OUTPUT_FORMAT_TEXLONG_CAPTION")+"}\\\\");
            print("\\toprule");
            print(sPrint);
            print("\\midrule");
            print("\\endhead");
            print("\\midrule");
            print("\\multicolumn{" + toString(_nCol) + "}{c}{--- \\emph{"+_lang.get("OUTPUT_FORMAT_TEXLONG_FOOT")+"} ---}\\\\");
            print("\\bottomrule");
            print("\\endfoot");
            print("\\bottomrule");
            print("\\endlastfoot");
        }
        sPrint = "";
    }

    if (bPrintCSV)
    {
        for (long long int i = 0; i < _nLine; i++)
        {
            for (long long int j = 0; j < _nCol; j++)
            {
                if (_sMatrix[i][j] != "---")
                    sPrint += _sMatrix[i][j];
                sPrint += ",";
            }
            print(sPrint);
            //cerr << sPrint << endl;
            sPrint = "";
        }
    }
    else
    {
        long long int nCol_0 = 0;
        long long int nCol_1 = _nCol;
        unsigned int nLine = 0;
        int nNotRepeatFirstCol = 1;
        if (!bFile && _option.getWindow()-4 < nLen)
        {
            for (long long int j = 0; j < _nCol; j++)
            {
                if (_option.getWindow()-4 < nLine+nLongest[j])
                {
                    nCol_1 = j;
                    break;
                }
                nLine += nLongest[j];
            }
        }
        else
            nLine = nLen;
        do
        {
            if (nCol_0 == nCol_1)
                nCol_1 = _nCol;
            for (long long int i = 0; i < _nLine; i++)
            {
                if (bPrintTeX && _nLine >= 30 && i < nHeadLineCount)
                    continue;
                if (!bCompact || _nLine < 12 || (bCompact && _nLine >= 12 && (i < 5 || i >= _nLine - 5)))
                {
                    for (long long int j = nCol_0*nNotRepeatFirstCol; j < nCol_1; j++)
                    {
                        if (i < nHeadLineCount && j == nCol_0*nNotRepeatFirstCol) 	// Erstes Element, erste Zeile?
                        {

                            if (bFile && !bPrintTeX)
                                sPrint = "#";		// Tabellenheader, Auskommentierung fuer GNUPlot
                            else if (!bFile)
                                sPrint = "|  ";

                            for (unsigned int n = 0; n < nLongest[j] - _sMatrix[i][j].length() - 1; n++)
                            {
                                sPrint += " ";	// Auf jeden Fall die Leerstellen zum Ausrichten ergaenzen
                            }
                        }
                        else	    // In allen anderen Faellen: ergaenze die fehlenden Leerstellen vor dem String
                        {
                            if (!bFile && j == nCol_0*nNotRepeatFirstCol)
                                sPrint = "| ";
                            else if (bPrintTeX && j)
                                sPrint += " &";

                            for (unsigned int n = 0; n < nLongest[j] - _sMatrix[i][j].length(); n++)
                            {
                                /*if (!j && n == nLongest-_sMatrix[i][j].length()-1)
                                    break;*/
                                sPrint += " ";
                            }
                        }
                        /*else
                            sPrint += "  ";*/
                        if (bPrintTeX && i >= nHeadLineCount && _sMatrix[i][j] != "---" && !bSumBar)
                            sPrint += "$";
                        else if (bPrintTeX && !bSumBar)
                            sPrint += "  ";

                        if (bPrintTeX && bSumBar)
                        {
                            if (_sMatrix[i][j].find(':') == string::npos)
                                sPrint += '$' + _sMatrix[i][j];
                            else
                                sPrint += _sMatrix[i][j].substr(0,_sMatrix[i][j].find(':')+2) + "$" + _sMatrix[i][j].substr(_sMatrix[i][j].find(':')+2);
                        }
                        else
                            sPrint += _sMatrix[i][j];	// Verknuepfe alle Elemente einer Zeile zu einem einzigen String
                        if (!nCol_0 && nCol_1 != _nCol && nNotRepeatFirstCol && i >= nHeadLineCount)
                        {
                            if (_sMatrix[i][0].find(':') != string::npos)
                                nNotRepeatFirstCol = 0;
                        }
                        if (bPrintTeX && i >= nHeadLineCount && _sMatrix[i][j] != "---")
                            sPrint += "$";
                        if (!nNotRepeatFirstCol && nCol_0 && !j)
                            j = nCol_0-1;
                    }
                    if ((!bFile || bPrintTeX) && i < nHeadLineCount)
                    {
                        for (unsigned int k = 0; k < sPrint.length(); k++)
                        {
                            if (sPrint[k] == '_')
                                sPrint[k] = ' ';
                        }
                    }
                    if (bPrintTeX)
                        sPrint += "\\\\";
                    print(sPrint); 				// Ende der Zeile: Ausgabe in das Ziel
                }
                else if (bCompact && _nLine >= 10 && i == 5)
                {
                    if (!bFile)
                        sPrint = "|  ";
                    for (long long int j = nCol_0*nNotRepeatFirstCol; j < nCol_1; j++)
                    {
                        for (unsigned int k = 0; k < nLongest[j] - 5; k++)
                        {
                            if (j == nCol_0*nNotRepeatFirstCol && k == nLongest[j]-6)
                                break;
                            sPrint += " ";
                        }
                        sPrint += "[...]";
                        if (!nNotRepeatFirstCol && nCol_0 && !j)
                            j = nCol_0-1;
                    }
                    print(sPrint);
                }

                if (i == nHeadLineCount-1)					// War das die erste Zeile? Mach' darunter eine schoene, auskommentierte Doppellinie
                {
                    if (bFile)
                    {
                        if (bPrintTeX && _nLine < 30)
                            sPrint = "\\midrule";
                        else if (!bPrintTeX)
                            sPrint = "#";
                    }
                    else
                        sPrint = "|   ";
                    if (!bPrintTeX)
                    {
                        if (!bFile)
                        {
                            /*sPrint.assign(nLen-2, (char)205);
                            sPrint.insert(0,"|   ");*/
                            sPrint.assign(nLine+2, '-');
                        }
                        else
                        {
                            sPrint.assign(nLen-1, '=');
                            sPrint.insert(0, "#");
                        }
                    }
                    print(sPrint);
                }

                sPrint = "";				// WICHTIG: sPrint auf einen leeren String setzen, sonst verknuepft man alle Zeilen iterativ miteinander... LOL
            }
            if (nCol_1 != _nCol)
            {
                if (nLine > 2)
                {
                    sPrint.assign(nLine+2, '-');
                    print(sPrint);
                    sPrint.clear();
                }
                nCol_0 = nCol_1;
                nLine = 0;
                if (!nNotRepeatFirstCol)
                {
                    nLine = nLongest[0];
                }
                for (long long int j = nCol_1; j < _nCol; j++)
                {
                    if (_option.getWindow()-4 < nLine + nLongest[j])
                    {
                        nCol_1 = j;
                        break;
                    }
                    nLine += nLongest[j];
                }
            }
        }
        while (nCol_1 != _nCol);
    }
    if (sCommentLine.length() && !bPrintCSV)
    {
        if (bSumBar)
            bSumBar = false;
        if (bFile)
        {
            if (bPrintTeX)
            {
                print("%");
                print("% " + sCommentLine);
            }
            else
            {
                print("#");
                print("# " + sCommentLine);		// Kommentarzeile fuer eventuell zusaetzliche Informationen
            }
        }
        else
        {
            print("|");
            cerr << LineBreak("|-> "+sCommentLine, _option) << endl;
        }
    }
    if (bPrintTeX && _nLine < 30)
    {
        print("\\bottomrule");
        print("\\end{tabular}");
        print("\\caption{"+ _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
        print("\\label{tab:" + sLabel + "}");
        print("\\end{table}");
    }
    else if (bPrintTeX)
    {
        print("\\end{longtable}");
    }

    if (nLen > 2)
        sPrint.assign(nLen+2, '-');
        //sPrint.assign(nLen-2, (char)205);

    string sConsoleOut = "";
    if (!bFile)
    {
        //print("|   " + sPrint);
        //print(sPrint);
        sConsoleOut += "|   -- " + toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY", toString(_nCol), toString(_nLine-nHeadLineCount), toString(_nCol*(_nLine-nHeadLineCount)))) + " --";
        //sConsoleOut += "|   -- " + toString(_nCol) + " Spalte(n) und " + toString(_nLine-1) + " Zeile(n) [" + toString(_nCol*(_nLine-1)) + " Elemente] --";
    }
    else
        sConsoleOut += "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE", toString(_nCol*(_nLine-nHeadLineCount)), sFileName));
        //sConsoleOut += "|-> Eine Tabelle mit " + toString(_nCol*(_nLine-1)) + " Elementen ";

    if (_option.getSystemPrintStatus())
        cerr << LineBreak(sConsoleOut, _option) << endl;
    if (bFile)
    {
        /*sConsoleOut += "wurde erfolgreich in die Datei \"" + sFileName + "\" geschrieben.";
        if (_option.getSystemPrintStatus())
            cerr << LineBreak(sConsoleOut, _option) << endl;*/

        end();
        return;
    }
    else
    {
        /*if (_option.getSystemPrintStatus())
            cerr << LineBreak(sConsoleOut, _option) << endl;*/
        if (!bDontAsk)
        {
            cerr << "|-> " << toSystemCodePage(_lang.get("OUTPUT_FORMAT_ASK_FILEOUT")) << endl;
            cerr << "|" << endl;
            cerr << "|<- ";
            getline(cin, cRerun);
        }


        if (cRerun == _lang.YES())
        {
            setCompact(false);
            generateFileName();			// Im Voraus schon einmal den Dateinamen generieren
            cerr << LineBreak("|-> " + _lang.get("OUTPUT_FORMAT_ASK_FILENAME"), _option) << endl;
            cerr << "|   (-> " << sFileName << ")" << endl;
            cerr << "|-> " << toSystemCodePage(_lang.get("COMMON_FILENAME")) << ":" << endl;
            cerr << "|" << endl;
            cerr << "|<- " << sPath << "/";
            getline(cin, sPrint);

            bFile = true;

            if (sPrint != "0")	// WICHTIG: Diese Bedingung liefert FALSE, wenn sPrint == "0";
            {
                setFileName(sPrint);	// WICHTIG: Durch das Aufrufen dieser Funktion wird auch geprueft, ob der Dateiname moeglich ist
            }
            else
            {
                cerr << "|-> " << toSystemCodePage(_lang.get("OUTPUT_FORMAT_CONFIRMDEFAULT")) << endl;
            }
            format(_sMatrix, _nCol, _nLine, _option);	// Nochmal dieselbe Funktion mit neuen Parametern aufrufen
        }
        else if (!bDontAsk)
        {
            cerr << LineBreak("|-> "+_lang.get("OUTPUT_FORMAT_NOFILECREATED"), _option) << endl;
        }
    }

	/*if (!bDontAsk)
        cin.ignore(1);*/
	return;
}

// --> setzt sPluginPrefix auf _sPrefix <--
void Output::setPrefix(string _sPrefix)
{
	sPluginPrefix = _sPrefix;
	return;
}

// --> gibt sPluginPrefix zureuck <--
string Output::getPrefix() const
{
	return sPluginPrefix;
}


