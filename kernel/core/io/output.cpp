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
}

// --> Destruktor <--
Output::~Output()
{
    if (bFile == true && bFileOpen == true)
        end();					// Schliesse Datei, falls eine geoeffnet ist
}

// --> Methoden <--
void Output::setStatus(bool bStatus)
{
    if (bFile == true && bFileOpen == true)
        end();					// Falls zuvor bereits ein Datenfile erzeugt wurde, schliesse dieses zuerst

    bFile = bStatus;			// Weise den neuen Wert an den Bool bFile zu.
}

// --> setzt Output auf die Defaultwerte zurueck <--
void Output::reset()
{
    if(bFile && bFileOpen)
        end();

    nTotalNumRows = 0;
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
}

string Output::replaceTeXControls(const string& _sText)
{
    string sReturn = _sText;

    for (size_t i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == (char)0xC4 || sReturn[i] == (char)142) // Ä
            sReturn.replace(i,1,"\\\"A");

        if (sReturn[i] == (char)0xE4 || sReturn[i] == (char)132) // ä
            sReturn.replace(i,1,"\\\"a");

        if (sReturn[i] == (char)0xD6 || sReturn[i] == (char)153) // Ö
            sReturn.replace(i,1,"\\\"O");

        if (sReturn[i] == (char)0xF6 || sReturn[i] == (char)148) // ö
            sReturn.replace(i,1,"\\\"o");

        if (sReturn[i] == (char)0xDC || sReturn[i] == (char)154) // Ü
            sReturn.replace(i,1,"\\\"U");

        if (sReturn[i] == (char)0xFC || sReturn[i] == (char)129) // ü
            sReturn.replace(i,1,"\\\"u");

        if (sReturn[i] == (char)0xDF || sReturn[i] == (char)225) // ß
            sReturn.replace(i,1,"\\ss ");

        if (sReturn[i] == (char)0xB0 || sReturn[i] == (char)248) // °
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

    if (sReturn == "inf")
        sReturn = "\\infty";
    else if (sReturn == "-inf")
        sReturn = "-\\infty";

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
        end();

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
        generateFileName();		// Generiere einen Dateinamen aus data_YYYY-MM-DD-hhmmss.dat
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
}


void Output::setPluginName(string _sPluginName)
{
    sPluginName = _sPluginName;	// Setze sPluginName = _sPluginName
}


void Output::setCommentLine(string _sCommentLine)
{
    sCommentLine = _sCommentLine;	// setze sCommentLine = _sCommentLine;
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

    print(sCommentSign);
    print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE1"));
    print(sCommentSign + " " + _lang.get("COMMON_APPNAME"));
    print(sCommentSign + "=============================================");
    print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE2", getVersion(), printBuildDate()));
    print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE3", getBuildYear()));
    print(sCommentSign + "");
    print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_LINE4", getDate(false)));
    print(sCommentSign + "");

    if (bPrintTeX)
        print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_TEX"));
    else
        print(sCommentSign + " " + _lang.get("OUTPUT_PRINTLEGAL_STD"));

    print(sCommentSign);
}


void Output::end()
{
    if (bFile == true && bFileOpen == true)
    {
        file_out.close();		// Schliesse die Datei
        bFileOpen = false;		// Setze den Boolean auf FALSE
    }
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
            start();

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
            file_out << "#" << sOutput.substr(1) << "\n";
        else
            file_out << sOutput << "\n";
    }
    else if (bSumBar)
        NumeReKernel::printPreFmt("|   " + sOutput.substr(4) + "\n");
    else
        NumeReKernel::printPreFmt(sOutput + "\n");
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
}


string Output::getDate(bool bForFile)	// Der Boolean entscheidet, ob ein Dateinamen-Datum oder ein "Kommentar-Datum" gewuenscht ist
{
    return toString(sys_time_now(), bForFile ? GET_AS_TIMESTAMP : GET_WITH_TEXT);
}


void Output::format(std::vector<std::vector<string>>& _sMatrix, size_t nHeadLineCount)
{
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (!nHeadLineCount)
        nHeadLineCount = 1;

    size_t nRows = _sMatrix.size();
    size_t nCols = _sMatrix.front().size();

    if (bPrintCSV)
    {
        std::string sPrint;

        for (size_t i = 0; i < nRows; i++)
        {
            for (size_t j = 0; j < nCols; j++)
            {
                if (_sMatrix[i][j] != "---")
                    sPrint += _sMatrix[i][j];

                sPrint += ",";
            }

            print(sPrint);
            sPrint.clear();
        }

        std::string sConsoleOut = "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                          toString(nCols*(nTotalNumRows ? nTotalNumRows : nRows-nHeadLineCount)),
                                                          sFileName));

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

        end();
        return;
    }

    std::vector<size_t> nLongest(nCols, 0);
    const size_t COLUMNSEPARATOR = 3;

    if (!bCompact || nRows < 12)
    {
        // --> Laufe durch jedes Element der Tabelle <--
        for (size_t j = 0; j < nCols; j++)
        {
            for (size_t i = 0; i < nRows; i++)
            {
                if (bPrintTeX)
                    _sMatrix[i][j] = replaceTeXControls(_sMatrix[i][j]);

                nLongest[j] = std::max(nLongest[j], _sMatrix[i][j].length());
            }

            nLongest[j] += COLUMNSEPARATOR;
        }
    }
    else
    {
        for (size_t j = 0; j < nCols; j++)
        {
            for (size_t i = 0; i < nRows; i++)
            {
                if (i < 5 || i >= nRows - 5)
                {
                    nLongest[j] = std::max(nLongest[j], _sMatrix[i][j].length());
                }
            }

            nLongest[j] += COLUMNSEPARATOR;
        }
    }

    if (bCompact)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (nLongest[j] < 7)
                nLongest[j] = 7;
        }
    }

    size_t nLen = 0;

    for (size_t j = 0; j < nCols; j++)
        nLen += nLongest[j];

    if (bPrintTeX)
    {
        std::string sLabel = sFileName;

        if (sLabel.find('/') != string::npos)
            sLabel.erase(0,sLabel.rfind('/')+1);

        if (sLabel.find(".tex") != string::npos)
            sLabel.erase(sLabel.rfind(".tex"));

        replaceAll(sLabel, " ", "_");

        if (nRows < 30)
        {
            print("\\begin{table}[htb]");
            print("\\centering");
            std::string sPrint = "\\begin{tabular}{";

            for (size_t j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            print(sPrint);
            print("\\toprule");
        }
        else
        {
            std::string sPrint = "\\begin{longtable}{";

            for (size_t j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            print(sPrint);
            print("\\caption{" + _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
            print("\\label{tab:" + sLabel + "}\\\\");
            print("\\toprule");
            sPrint.clear();

            for (size_t i = 0; i < nHeadLineCount; i++)
            {
                for (size_t j = 0; j < nCols; j++)
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
            print("\\multicolumn{" + toString(nCols) + "}{c}{--- \\emph{"+_lang.get("OUTPUT_FORMAT_TEXLONG_FOOT")+"} ---}\\\\");
            print("\\bottomrule");
            print("\\endfoot");
            print("\\bottomrule");
            print("\\endlastfoot");
        }

        // Do the matrix printing here
        for (size_t i = 0; i < nRows; i++)
        {
            if (nRows >= 30 && i < nHeadLineCount)
                continue;

            std::string sPrint;

            for (size_t j = 0; j < nCols; j++)
            {
                if (j)
                    sPrint += " &";

                if (i < nHeadLineCount)
                    sPrint.append(COLUMNSEPARATOR, ' ');
                else
                    sPrint.append(nLongest[j] - _sMatrix[i][j].length(), ' ');

                if (i >= nHeadLineCount && _sMatrix[i][j] != "---" && !bSumBar)
                    sPrint += "$";
                else if (!bSumBar)
                    sPrint += "  ";

                if (bSumBar)
                {
                    if (_sMatrix[i][j].find(':') == string::npos)
                        sPrint += '$' + _sMatrix[i][j];
                    else
                        sPrint += _sMatrix[i][j].substr(0,_sMatrix[i][j].find(':')+2) + "$"
                            + _sMatrix[i][j].substr(_sMatrix[i][j].find(':')+2);
                }

                if (i < nHeadLineCount)
                    sPrint.append(nLongest[j] - _sMatrix[i][j].length() - COLUMNSEPARATOR, ' ');

                if (i >= nHeadLineCount && _sMatrix[i][j] != "---")
                    sPrint += "$";
            }

            print(sPrint + "\\\\");

            if (i == nHeadLineCount-1 && nRows < 30)
                print("\\midrule");
        }

        if (sCommentLine.length())
        {
            print("%");
            print("% " + sCommentLine);
        }

        if (nRows < 30)
        {
            print("\\bottomrule");
            print("\\end{tabular}");
            print("\\caption{"+ _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
            print("\\label{tab:" + sLabel + "}");
            print("\\end{table}");
        }
        else
            print("\\end{longtable}");

        std::string sConsoleOut = "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                          toString(nCols*(nTotalNumRows ? nTotalNumRows : nRows-nHeadLineCount)),
                                                          sFileName));

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

        end();
        return;
    }

    size_t nLineStartCol = 0;
    size_t nLineEndCol = nCols;
    size_t nLineLength = 0;
    int nNotRepeatFirstCol = 1;

    if (!bFile && _option.getWindow()-4 < nLen)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (_option.getWindow()-4 < nLineLength+nLongest[j])
            {
                nLineEndCol = j;
                break;
            }

            nLineLength += nLongest[j];
        }
    }
    else
        nLineLength = nLen;

    do
    {
        if (nLineStartCol == nLineEndCol)
            nLineEndCol = nCols;

        for (size_t i = 0; i < nRows; i++)
        {
            std::string sPrint;

            if (!bCompact || nRows < 12 || (bCompact && nRows >= 12 && (i < 5 || i >= nRows - 5)))
            {
                for (size_t j = nLineStartCol*nNotRepeatFirstCol; j < nLineEndCol; j++)
                {
                    if (i < nHeadLineCount && j == nLineStartCol*nNotRepeatFirstCol) 	// Erstes Element, erste Zeile?
                    {
                        if (bFile)
                            sPrint = "#";		// Tabellenheader, Auskommentierung fuer GNUPlot
                        else if (!bFile)
                            sPrint = "|   ";
                    }
                    else	    // In allen anderen Faellen: ergaenze die fehlenden Leerstellen vor dem String
                    {
                        if (!bFile && j == nLineStartCol*nNotRepeatFirstCol)
                            sPrint = "|";

                        if (i < nHeadLineCount)
                            sPrint.append(COLUMNSEPARATOR, ' ');
                        else
                            sPrint.append(nLongest[j] - _sMatrix[i][j].length(), ' ');
                    }

                    sPrint += _sMatrix[i][j];	// Verknuepfe alle Elemente einer Zeile zu einem einzigen String

                    if (i < nHeadLineCount)
                        sPrint.append(nLongest[j] - _sMatrix[i][j].length() - COLUMNSEPARATOR, ' ');

                    if (!nLineStartCol && nLineEndCol != nCols && nNotRepeatFirstCol && i >= nHeadLineCount)
                    {
                        if (_sMatrix[i][0].find(':') != string::npos)
                            nNotRepeatFirstCol = 0;
                    }

                    if (!nNotRepeatFirstCol && nLineStartCol && !j)
                        j = nLineStartCol-1;
                }

                print(sPrint); 				// Ende der Zeile: Ausgabe in das Ziel
            }
            else if (bCompact && nRows >= 10 && i == 5)
            {
                if (!bFile)
                    sPrint = "|  ";

                for (size_t j = nLineStartCol*nNotRepeatFirstCol; j < nLineEndCol; j++)
                {
                    for (size_t k = 0; k < nLongest[j] - 5; k++)
                    {
                        if (j == nLineStartCol*nNotRepeatFirstCol && k == nLongest[j]-6)
                            break;

                        sPrint += " ";
                    }

                    sPrint += "[...]";

                    if (!nNotRepeatFirstCol && nLineStartCol && !j)
                        j = nLineStartCol-1;
                }

                print(sPrint);
            }

            if (i == nHeadLineCount-1)	// War das die erste Zeile? Mach' darunter eine schoene, auskommentierte Doppellinie
            {
                if (bFile)
                    sPrint = "#";
                else
                    sPrint = "|   ";

                if (!bFile)
                    sPrint.assign(nLineLength+1, '-');
                else
                {
                    sPrint.assign(nLen-1, '=');
                    sPrint.insert(0, "#");
                }

                print(sPrint);
            }
        }

        if (nLineEndCol != nCols)
        {
            if (nLineLength > 2)
                print(std::string(nLineLength+1, '-'));

            nLineStartCol = nLineEndCol;
            nLineLength = 0;

            if (!nNotRepeatFirstCol)
                nLineLength = nLongest[0];

            for (size_t j = nLineEndCol; j < nCols; j++)
            {
                if (_option.getWindow()-4 < nLineLength + nLongest[j])
                {
                    nLineEndCol = j;
                    break;
                }

                nLineLength += nLongest[j];
            }
        }
    }
    while (nLineEndCol != nCols);

    if (sCommentLine.length())
    {
        if (bSumBar)
            bSumBar = false;

        if (bFile)
        {
            print("#");
            print("# " + sCommentLine);
        }
        else
        {
            print("|");
            NumeReKernel::print(sCommentLine);
        }
    }

    std::string sConsoleOut = "";

    if (!bFile)
    {
        sConsoleOut += "|   -- " + toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY",
                                                              toString(nCols),
                                                              toString(nTotalNumRows ? nTotalNumRows : nRows-nHeadLineCount),
                                                              toString(nCols*(nTotalNumRows ? nTotalNumRows : nRows-nHeadLineCount)))) + " --";
    }
    else
        sConsoleOut += "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                         toString(nCols*(nTotalNumRows ? nTotalNumRows : nRows-nHeadLineCount)),
                                                         sFileName));

    if (_option.systemPrints())
        NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

    if (bFile)
        end();
}


static std::string formatUnit(const std::string& sValue, const std::string& sUnit)
{
    if (sUnit.length())
        return sValue + " " + sUnit;

    return sValue;
}


void Output::format(NumeRe::Table _table, size_t digits, size_t chars)
{
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    size_t nHeadLineCount = _table.getHeadCount();
    size_t nRows = _table.getLines();
    size_t nCols = _table.getCols();
    std::vector<TableColumn::ColumnType> vTypes;

    for (size_t i = 0; i < nCols; i++)
    {
        vTypes.push_back(_table.getColumnType(i));
    }

    if (bPrintCSV)
    {
        std::string sPrint;

        for (size_t i = 0; i < nHeadLineCount; i++)
        {
            for (size_t j = 0; j < nCols; j++)
            {
                sPrint += _table.getCleanHeadPart(j, i) + ",";
            }

            print(sPrint);
            sPrint.clear();
        }

        for (size_t i = 0; i < nRows; i++)
        {
            for (size_t j = 0; j < nCols; j++)
            {
                if (_table.get(i, j).isValid())
                    sPrint += _table.get(i, j).printVal();

                sPrint += ",";
            }

            print(sPrint);
            sPrint.clear();
        }

        std::string sConsoleOut = "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                          toString(nCols*nRows),
                                                          sFileName));

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

        end();
        return;
    }

    if (bPrintTeX)
    {
        std::string sLabel = sFileName;

        if (sLabel.find('/') != string::npos)
            sLabel.erase(0,sLabel.rfind('/')+1);

        if (sLabel.find(".tex") != string::npos)
            sLabel.erase(sLabel.rfind(".tex"));

        replaceAll(sLabel, " ", "_");

        if (nRows < 30)
        {
            print("\\begin{table}[htb]");
            print("\\centering");
            std::string sPrint = "\\begin{tabular}{";

            for (size_t j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            print(sPrint);
            print("\\toprule");

            for (size_t i = 0; i < nHeadLineCount; i++)
            {
                for (size_t j = 0; j < nCols; j++)
                    sPrint += _table.getCleanHeadPart(j, i) + " & ";

                sPrint = sPrint.substr(0,sPrint.length()-2) + "\\\\\n";
            }

            print("\\midrule");
        }
        else
        {
            std::string sPrint = "\\begin{longtable}{";

            for (size_t j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            print(sPrint);
            print("\\caption{" + _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
            print("\\label{tab:" + sLabel + "}\\\\");
            print("\\toprule");
            sPrint.clear();

            for (size_t i = 0; i < nHeadLineCount; i++)
            {
                for (size_t j = 0; j < nCols; j++)
                    sPrint += _table.getCleanHeadPart(j, i) + " & ";

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
            print("\\multicolumn{" + toString(nCols) + "}{c}{--- \\emph{"+_lang.get("OUTPUT_FORMAT_TEXLONG_FOOT")+"} ---}\\\\");
            print("\\bottomrule");
            print("\\endfoot");
            print("\\bottomrule");
            print("\\endlastfoot");
        }

        // Do the matrix printing here
        for (size_t i = 0; i < nRows; i++)
        {
            std::string sPrint;

            for (size_t j = 0; j < nCols; j++)
            {
                if (j)
                    sPrint += " &";

                if (_table.get(i, j).isValid() && TableColumn::isValueType(vTypes[j]))
                    sPrint += "$";
                else
                    sPrint += "  ";

                sPrint += replaceTeXControls(_table.get(i, j).printVal());

                if (_table.get(i, j).isValid() && TableColumn::isValueType(vTypes[j]))
                    sPrint += "$";
            }

            print(sPrint + "\\\\");
        }

        if (sCommentLine.length())
        {
            print("%");
            print("% " + sCommentLine);
        }

        if (nRows < 30)
        {
            print("\\bottomrule");
            print("\\end{tabular}");
            print("\\caption{"+ _lang.get("OUTPUT_FORMAT_TEX_HEAD", sPluginName)+"}");
            print("\\label{tab:" + sLabel + "}");
            print("\\end{table}");
        }
        else
            print("\\end{longtable}");

        std::string sConsoleOut = "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                          toString(nCols*nRows),
                                                          sFileName));

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

        end();
        return;
    }

    std::vector<size_t> nLongest(nCols, 0);
    const size_t COLUMNSEPARATOR = 3;

    for (size_t i = 0; i < nHeadLineCount; i++)
    {
        for (size_t j = 0; j < nCols; j++)
            nLongest[j] = std::max(nLongest[j], _table.getCleanHeadPart(j, i).length());
    }

    if (!bCompact || nRows < 12)
    {
        // --> Laufe durch jedes Element der Tabelle <--
        for (size_t j = 0; j < nCols; j++)
        {
            if (_table.getColumn(j))
            {
                for (size_t i = 0; i < nRows; i++)
                {
                    nLongest[j] = std::max(nLongest[j],
                                           formatUnit(_table.get(i, j).print(digits, chars),
                                                      _table.getColumn(j)->m_sUnit).length());
                }
            }

            nLongest[j] += COLUMNSEPARATOR;
        }
    }
    else
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (_table.getColumn(j))
            {
                for (size_t i = 0; i < nRows; i++)
                {
                    if (i < 5 || i >= nRows - 5)
                    {
                        nLongest[j] = std::max(nLongest[j],
                                               formatUnit(_table.get(i, j).print(digits, chars),
                                                          _table.getColumn(j)->m_sUnit).length());
                    }
                }
            }

            nLongest[j] += COLUMNSEPARATOR;
        }
    }

    if (bCompact)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            // Ensure that the hidden parts have enough room for being print
            if (nLongest[j] < COLUMNSEPARATOR+5)
                nLongest[j] = COLUMNSEPARATOR+5;
        }
    }

    size_t nLen = 0;

    for (size_t j = 0; j < nCols; j++)
        nLen += nLongest[j];

    size_t nLineStartCol = 0;
    size_t nLineEndCol = nCols;
    size_t nLineLength = 0;
    int nNotRepeatFirstCol = 1;

    if (!bFile && _option.getWindow()-4 < nLen)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (_option.getWindow()-4 < nLineLength+nLongest[j])
            {
                nLineEndCol = j;
                break;
            }

            nLineLength += nLongest[j];
        }
    }
    else
        nLineLength = nLen;

    do
    {
        if (nLineStartCol == nLineEndCol)
            nLineEndCol = nCols;

        for (size_t i = 0; i < nHeadLineCount; i++)
        {
            std::string sPrint;

            for (size_t j = nLineStartCol*nNotRepeatFirstCol; j < nLineEndCol; j++)
            {
                std::string sValue = _table.getCleanHeadPart(j, i);

                if (j == nLineStartCol*nNotRepeatFirstCol)
                {
                    if (bFile)
                        sPrint = "#";
                    else if (!bFile)
                        sPrint = "|   ";
                }
                else
                {
                    if (!bFile && j == nLineStartCol*nNotRepeatFirstCol)
                        sPrint = "|";

                    sPrint.append(COLUMNSEPARATOR, ' ');
                }

                sPrint += sValue;
                sPrint.append(nLongest[j] - sValue.length() - COLUMNSEPARATOR, ' ');

                /*if (!nLineStartCol && nLineEndCol != nCols && nNotRepeatFirstCol && i >= nHeadLineCount)
                {
                    if (_sMatrix[i][0].find(':') != string::npos)
                        nNotRepeatFirstCol = 0;
                }*/

                if (!nNotRepeatFirstCol && nLineStartCol && !j)
                    j = nLineStartCol-1;
            }

            print(sPrint); 				// Ende der Zeile: Ausgabe in das Ziel
        }

        if (bFile)
        {
            std::string sPrint = "#";
            sPrint.append(nLen-1, '=');
            print(sPrint);
        }
        else
        {
            std::string sPrint = "|   ";
            sPrint.append(nLineLength-3, '-');
            print(sPrint);
        }

        for (size_t i = 0; i < nRows; i++)
        {
            std::string sPrint;

            if (!bCompact || nRows < 12 || (bCompact && nRows >= 12 && (i < 5 || i >= nRows - 5)))
            {
                for (size_t j = nLineStartCol*nNotRepeatFirstCol; j < nLineEndCol; j++)
                {
                    std::string sValue = "---";

                    if (_table.getColumn(j))
                    {
                        sValue = _table.get(i,j).isValid() ? formatUnit(_table.get(i, j).print(digits, chars),
                                                                        _table.getColumn(j)->m_sUnit) : "---";
                    }

                    if (!bFile && j == nLineStartCol*nNotRepeatFirstCol)
                        sPrint = "|";

                    if (!TableColumn::isValueType(vTypes[j]))
                        sPrint.append(COLUMNSEPARATOR, ' ');
                    else
                        sPrint.append(nLongest[j] - sValue.length(), ' ');

                    sPrint += sValue;

                    if (!TableColumn::isValueType(vTypes[j]))
                        sPrint.append(nLongest[j] - sValue.length() - COLUMNSEPARATOR, ' ');

                    /*if (!nLineStartCol && nLineEndCol != nCols && nNotRepeatFirstCol && i >= nHeadLineCount)
                    {
                        if (_sMatrix[i][0].find(':') != string::npos)
                            nNotRepeatFirstCol = 0;
                    }*/

                    if (!nNotRepeatFirstCol && nLineStartCol && !j)
                        j = nLineStartCol-1;
                }

                print(sPrint); 				// Ende der Zeile: Ausgabe in das Ziel
            }
            else if (bCompact && nRows >= 10 && i == 5)
            {
                if (!bFile)
                    sPrint = "|";

                for (size_t j = nLineStartCol*nNotRepeatFirstCol; j < nLineEndCol; j++)
                {
                    if (!TableColumn::isValueType(vTypes[j]))
                        sPrint.append(COLUMNSEPARATOR, ' ');
                    else
                        sPrint.append(nLongest[j] - 5, ' ');

                    sPrint += "[...]";

                    if (!TableColumn::isValueType(vTypes[j]))
                        sPrint.append(nLongest[j] - 5 - COLUMNSEPARATOR, ' ');

                    if (!nNotRepeatFirstCol && nLineStartCol && !j)
                        j = nLineStartCol-1;
                }

                print(sPrint);
            }
        }

        if (nLineEndCol != nCols)
        {
            if (nLineLength > 2)
                print(std::string(nLineLength+1, '-'));

            nLineStartCol = nLineEndCol;
            nLineLength = 0;

            if (!nNotRepeatFirstCol)
                nLineLength = nLongest[0];

            for (size_t j = nLineEndCol; j < nCols; j++)
            {
                if (_option.getWindow()-4 < nLineLength + nLongest[j])
                {
                    nLineEndCol = j;
                    break;
                }

                nLineLength += nLongest[j];
            }
        }
    }
    while (nLineEndCol != nCols);

    if (sCommentLine.length())
    {
        if (bSumBar)
            bSumBar = false;

        if (bFile)
        {
            print("#");
            print("# " + sCommentLine);
        }
        else
        {
            print("|");
            NumeReKernel::print(sCommentLine);
        }
    }

    std::string sConsoleOut = "";

    if (!bFile)
    {
        sConsoleOut += "|   -- " + toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY",
                                                              toString(nCols),
                                                              toString(nRows),
                                                              toString(nCols*nRows))) + " --";
    }
    else
        sConsoleOut += "|-> "+toSystemCodePage(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE",
                                                         toString(nCols*nRows),
                                                         sFileName));

    if (_option.systemPrints())
        NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option) + "\n");

    if (bFile)
        end();
}


// --> setzt sPluginPrefix auf _sPrefix <--
void Output::setPrefix(string _sPrefix)
{
    sPluginPrefix = _sPrefix;
}

// --> gibt sPluginPrefix zureuck <--
string Output::getPrefix() const
{
    return sPluginPrefix;
}


