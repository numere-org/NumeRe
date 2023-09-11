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
#include "../../kernel.hpp"

using namespace std;

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

    for (size_t i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == (char)0xC4 || sReturn[i] == (char)142) // �
            sReturn.replace(i,1,"\\\"A");

        if (sReturn[i] == (char)0xE4 || sReturn[i] == (char)132) // �
            sReturn.replace(i,1,"\\\"a");

        if (sReturn[i] == (char)0xD6 || sReturn[i] == (char)153) // �
            sReturn.replace(i,1,"\\\"O");

        if (sReturn[i] == (char)0xF6 || sReturn[i] == (char)148) // �
            sReturn.replace(i,1,"\\\"o");

        if (sReturn[i] == (char)0xDC || sReturn[i] == (char)154) // �
            sReturn.replace(i,1,"\\\"U");

        if (sReturn[i] == (char)0xFC || sReturn[i] == (char)129) // �
            sReturn.replace(i,1,"\\\"u");

        if (sReturn[i] == (char)0xDF || sReturn[i] == (char)225) // �
            sReturn.replace(i,1,"\\ss ");

        if (sReturn[i] == (char)0xB0 || sReturn[i] == (char)248) // �
            sReturn.replace(i,1,"$^\\circ$");

        if (sReturn[i] == (char)196 || sReturn[i] == (char)249)
            sReturn.replace(i,1,"\\pm ");

        if (sReturn[i] == (char)171 || sReturn[i] == (char)174)
            sReturn.replace(i,1,"\"<");

        if (sReturn[i] == (char)187 || sReturn[i] == (char)175)
            sReturn.replace(i,1,"\">");

        if ((!i && sReturn[i] == '_') || (i && sReturn[i] == '_' && sReturn[i-1] != '\\'))
            sReturn.insert(i,1,'\\');
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
    print(sCommentSign + " " + _lang.get("COMMON_APPNAME"));
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
    if (sOutput.find("---------") != string::npos || sOutput.find("<<SUMBAR>>") != string::npos)
    {
        if (sOutput.find("<<SUMBAR>>") != string::npos)
        {
            size_t nLength = sOutput.length();

            if (!bFile)
                sOutput.assign(nLength, '-');
            else
                sOutput.assign(nLength, '-');
        }
        else if (!bFile)
            sOutput.assign(sOutput.length(), '-');

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
            NumeReKernel::printPreFmt("*************************************************\n");
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE1")) + "\n");
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE2")) + "\n");
            NumeReKernel::printPreFmt("|   \"" + sFileName + "\"\n");
            NumeReKernel::printPreFmt("|   " + toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE3")) + "\n");
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("OUTPUT_PRINT_INACCESSIBLE4")) + "\n");
            NumeReKernel::printPreFmt("*************************************************\n");
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
        NumeReKernel::printPreFmt("|   " + sOutput.substr(4) + "\n");
    }
    else
        NumeReKernel::printPreFmt(sOutput + "\n");

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


void Output::format(string** _sMatrix, long long int _nCol, long long int _nLine, const Settings& _option, bool bDontAsk, int nHeadLineCount)
{
    if (!nHeadLineCount)
        nHeadLineCount = 1;

    size_t nLongest[_nCol];		// Int fuer die laengste Zeichenkette: unsigned da string::length() einen unsigned zurueck gibt
    size_t nLen = 0;			// Int fuer die aktuelle Laenge: dito
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

                    if (_sMatrix[i][j] == "inf")
                    {
                        _sMatrix[i][j] = "\\infty";
                    }

                    if (_sMatrix[i][j] == "-inf")
                    {
                        _sMatrix[i][j] = "-\\infty";
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

            for (size_t i = 0; i < sPrint.length(); i++)
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
            sPrint = "";
        }
    }
    else
    {
        long long int nCol_0 = 0;
        long long int nCol_1 = _nCol;
        size_t nLine = 0;
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

                            for (size_t n = 0; n < nLongest[j] - _sMatrix[i][j].length() - 1; n++)
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

                            for (size_t n = 0; n < nLongest[j] - _sMatrix[i][j].length(); n++)
                            {
                                sPrint += " ";
                            }
                        }

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

                    if (bPrintTeX && i < nHeadLineCount)
                    {
                        for (size_t k = 0; k < sPrint.length(); k++)
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
                        for (size_t k = 0; k < nLongest[j] - 5; k++)
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
            NumeReKernel::print(LineBreak(sCommentLine, _option));
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

    string sConsoleOut = "";

    if (!bFile)
    {
        sConsoleOut += "|   -- " + toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY", toString(_nCol), toString(_nLine-nHeadLineCount), toString(_nCol*(_nLine-nHeadLineCount)))) + " --";
    }
    else
        sConsoleOut += "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE", toString(_nCol*(_nLine-nHeadLineCount)), sFileName));

    if (_option.systemPrints())
        NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

    if (bFile)
    {
        end();
        return;
    }
    else
    {
        if (!bDontAsk)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("OUTPUT_FORMAT_ASK_FILEOUT")));
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(cRerun);
        }


        if (cRerun == _lang.YES())
        {
            setCompact(false);
            generateFileName();			// Im Voraus schon einmal den Dateinamen generieren
            NumeReKernel::print(LineBreak(_lang.get("OUTPUT_FORMAT_ASK_FILENAME"), _option));
            NumeReKernel::printPreFmt("|   (-> " + sFileName + ")\n");
            NumeReKernel::print(toSystemCodePage(_lang.get("COMMON_FILENAME")) + ":");
            NumeReKernel::printPreFmt("|\n|<- " + sPath + "/");

            NumeReKernel::getline(sPrint);

            bFile = true;

            if (sPrint != "0")	// WICHTIG: Diese Bedingung liefert FALSE, wenn sPrint == "0";
            {
                setFileName(sPrint);	// WICHTIG: Durch das Aufrufen dieser Funktion wird auch geprueft, ob der Dateiname moeglich ist
            }
            else
            {
               NumeReKernel::print(toSystemCodePage(_lang.get("OUTPUT_FORMAT_CONFIRMDEFAULT")));
            }

            format(_sMatrix, _nCol, _nLine, _option);	// Nochmal dieselbe Funktion mit neuen Parametern aufrufen
        }
        else if (!bDontAsk)
        {
            NumeReKernel::print(LineBreak(_lang.get("OUTPUT_FORMAT_NOFILECREATED"), _option));
        }
    }

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


