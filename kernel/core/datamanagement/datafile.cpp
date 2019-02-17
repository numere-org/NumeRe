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


#include "datafile.hpp"
#include "../../kernel.hpp"

bool fileExists(const string&);
string wcstombs(const wstring&);
using namespace std;
using namespace boost;
using namespace YExcel;

size_t qSortDouble(double* dArray, size_t nlength);


/*
 * Realisierung der Datafile-Klasse
 */

// --> Standard-Konstruktor <--
Datafile::Datafile() : Cache()
{
	nLines = 0;
	nCols = 0;
	dDatafile = 0;
	//bValidEntry = 0;
	sHeadLine = 0;
	nAppendedZeroes = 0;
	bValidData = false;
	bUseCache = false;
	bPauseOpening = false;
	bLoadEmptyCols = false;
	bLoadEmptyColsInNextFile = false;
	sDataFile = "";
	sOutputFile = "";
	sPrefix = "data";
	sSavePath = "<savepath>";
}

// --> Allgemeiner Konstruktor <--
Datafile::Datafile(long long int _nLines, long long int _nCols) : Cache()
{
	nLines = _nLines;
	nCols = _nCols;
	bValidData = false;
	bUseCache = false;
	bPauseOpening = false;
	bLoadEmptyCols = false;
	bLoadEmptyColsInNextFile = false;
	sDataFile = "";
	sOutputFile = "";
	sPrefix = "data";
	sSavePath = "<savepath>";
	nAppendedZeroes = 0;
	// --> Bereite gleich den Speicher auf Basis der beiden ints vor <--
	dDatafile = new double*[nLines];
	//bValidEntry = new bool*[nLines];
	sHeadLine = new string[nCols];

	for (long long int i = 0; i < nLines; i++)
	{
		dDatafile[i] = new double[nCols];
		//bValidEntry[i] = new bool[nCols];
		for (long long int j = 0; j < nCols; j++)
		{
			sHeadLine[j] = "";
			dDatafile[i][j] = 0.0;
			//bValidEntry[i][j] = false;
		}
	}
}

// --> Destruktor <--
Datafile::~Datafile()
{
	// --> Gib alle Speicher frei, sofern sie belegt sind! (Pointer != 0) <--
	if(dDatafile)
	{
		for(long long int i = 0; i < nLines; i++)
		{
			delete[] dDatafile[i];
		}
		delete[] dDatafile;
	}
	/*if(bValidEntry)
	{
		for (long long int i = 0; i < nLines; i++)
		{
			delete[] bValidEntry[i];
		}
		delete[] bValidEntry;
	}*/
	if(sHeadLine)
		delete[] sHeadLine;
	if(nAppendedZeroes)
		delete[] nAppendedZeroes;
	if(file_in.is_open())
		file_in.close();
}

// --> Generiere eine neue Matrix auf Basis der gesetzten Werte. Pruefe zuvor, ob nicht schon eine vorhanden ist <--
void Datafile::Allocate()
{
	if (nLines && nCols && !dDatafile && !nAppendedZeroes)// && !bValidEntry)
	{
        if (!sHeadLine)
        {
            sHeadLine = new string[nCols];
			for (long long int j = 0; j < nCols; j++)
			{
                sHeadLine[j] = "Spalte_"+toString(j+1);
			}
		}
		nAppendedZeroes = new long long int[nCols];
		for (long long int i = 0; i < nCols; i++)
		{
			nAppendedZeroes[i] = nLines;
		}
		//bValidEntry = new bool*[nLines];
		dDatafile = new double*[nLines];
		for (long long int i = 0; i < nLines; i++)
		{
			dDatafile[i] = new double[nCols];
			//bValidEntry[i] = new bool[nCols];
			for (long long int j = 0; j < nCols; j++)
			{
				dDatafile[i][j] = NAN;
				//bValidEntry[i][j] = false;
			}
		}
	}
	return;
}

void Datafile::countAppendedZeroes(long long int nCol)
{
    if (nCol == -1)
    {
        for (long long int i = 0; i < nCols; i++)
        {
            nAppendedZeroes[i] = 0;
            for (long long int j = nLines-1; j >= 0; j--)
            {
                if (isnan(dDatafile[j][i]))
                    nAppendedZeroes[i]++;
                else
                    break;
            }
        }
    }
    else if (nCol >= 0 && nCol < nCols)
    {
        nAppendedZeroes[nCol] = 0;
        for (long long int j = nLines-1; j >= 0; j--)
        {
            if (isnan(dDatafile[j][nCol]))
                nAppendedZeroes[nCol]++;
            else
                break;
        }
    }
    return;
}

// --> Oeffne eine LABX-Datei (CASSY) <--
void Datafile::openLabx(Settings& _option)
{
    if (toLowerCase(sDataFile.substr(sDataFile.rfind('.'))) != ".labx")
        return;

    string sLabx = "";
    string sLabx_substr = "";
    string* sCols = 0;
    string** sDataMatrix = 0;
    long long int nLine = 0;

    file_in.open(sDataFile.c_str());
    if (file_in.fail())
    {
        //sErrorToken = sDataFile;
        file_in.close();
        //throw DATAFILE_NOT_EXIST;
        throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
    }

    while (!file_in.eof())
    {
        getline(file_in, sLabx_substr);
        if (file_in.fail())
        {
            //sErrorToken = sDataFile;
            file_in.close();
            //throw CANNOT_READ_FILE;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
        }
        StripSpaces(sLabx_substr);
        sLabx += sLabx_substr;
    }
    if (!sLabx.length() || sLabx.find("<allchannels count=") == string::npos)
    {
        //sErrorToken = sDataFile;
        file_in.close();
        //throw DATAFILE_NOT_EXIST;
        throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
    }
    file_in.close();

    sLabx_substr = sLabx.substr(sLabx.find("<allchannels count="));
    sLabx_substr = sLabx_substr.substr(sLabx_substr.find("=\"")+2, sLabx_substr.find("\">")-sLabx_substr.find("=\"")-2);
    nCols = StrToInt(sLabx_substr);
    if (!nCols)
    {
        //sErrorToken = sDataFile;
        //throw CANNOT_READ_FILE;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    sCols = new string[nCols];
    sHeadLine = new string[nCols];

    sLabx_substr = sLabx.substr(sLabx.find("<allchannels"), sLabx.find("</allchannels>")-sLabx.find("<allchannels"));
    sLabx_substr = sLabx_substr.substr(sLabx_substr.find("<channels"));
    for (unsigned int i = 0; i < nCols; i++)
    {
        sCols[i] = sLabx_substr.substr(sLabx_substr.find("<values"), sLabx_substr.find("</channel>")-sLabx_substr.find("<values"));
        if (sLabx_substr.find("<unit />") != string::npos && sLabx_substr.find("<unit />") < sLabx_substr.find("<unit>"))
        {
            setHeadLineElement(i, "data", utf8parser(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10)));
        }
        else
        {
            setHeadLineElement(i, "data", utf8parser(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10))
                + "_[" + utf8parser(sLabx_substr.substr(sLabx_substr.find("<unit>")+6, sLabx_substr.find("</unit>")-sLabx_substr.find("<unit>")-6)) + "]");
        }
        sLabx_substr = sLabx_substr.substr(sLabx_substr.find("</channels>")+11);
        if (StrToInt(sCols[i].substr(sCols[i].find("count=\"")+7, sCols[i].find("\">")-sCols[i].find("count=\"")-7)) > nLine)
            nLine = StrToInt(sCols[i].substr(sCols[i].find("count=\"")+7, sCols[i].find("\">")-sCols[i].find("count=\"")-7));
        //cerr << sHeadLine[i] << endl;
    }
    if (!nLine)
    {
        //sErrorToken = sDataFile;
        delete[] sCols;
        delete[] sHeadLine;
        //throw CANNOT_READ_FILE;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    nLines = nLine;

    sDataMatrix = new string*[nLines];
    for (long long int i = 0; i < nLines; i++)
    {
        sDataMatrix[i] = new string[nCols];
    }

    unsigned int nElements = 0;
    for (long long int i = 0; i < nCols; i++)
    {
        if (sCols[i].find("<values count=\"0\" />") == string::npos)
        {
            nElements = StrToInt(sCols[i].substr(sCols[i].find('"')+1, sCols[i].find('"', sCols[i].find('"')+1)-1-sCols[i].find('"')));
            sCols[i] = sCols[i].substr(sCols[i].find('>')+1);
            //cerr << "first value" << endl;
            for (long long int j = 0; j < nLines; j++)
            {
                if (j >= nElements)
                {
                    sDataMatrix[j][i] = "<value />";
                }
                else
                {
                    sDataMatrix[j][i] = sCols[i].substr(sCols[i].find("<value"), sCols[i].find('<', sCols[i].find('/'))-sCols[i].find("<value"));
                    sCols[i] = sCols[i].substr(sCols[i].find('>', sCols[i].find('/'))+1);
                }
            }
            //cerr << "last value" << endl;
        }
        else
        {
            for (long long int j = 0; j < nLines; j++)
            {
                sDataMatrix[j][i] = "<value />";
            }
            //cerr << "col filled" << endl;
        }
    }

    //cerr << "Splitted" << endl;
    if (!dDatafile)
        Allocate();

    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            if (sDataMatrix[i][j] == "<value />")
            {
                dDatafile[i][j] = NAN;
                //bValidEntry[i][j] = false;
            }
            else
            {
                dDatafile[i][j] = StrToDb(sDataMatrix[i][j].substr(7, sDataMatrix[i][j].find('<', 7)-7));
                //bValidEntry[i][j] = true;
                if (!bValidData)
                    bValidData = true;
            }
        }
    }
    //cerr << "Parsed" << endl;
    countAppendedZeroes();

    for (long long int i = 0; i < nLines; i++)
    {
        delete[] sDataMatrix[i];
    }
    delete[] sDataMatrix;
    delete[] sCols;
    sDataMatrix = 0;
    sCols = 0;

    return;
}

// --> Oeffne eine CSV-Datei <--
void Datafile::openCSV(Settings& _option)
{
	if (!bValidData)			// Es sind hoffentlich noch keine Daten gespeichert ...
	{
		if(_option.getbDebug())
			cerr << "|-> DEBUG: bValidData = false" << endl;

		char cSep = 0;

		// --> Benoetigte temporaere Variablen initialisieren <--
		string s = "";
		long long int nLine = 0;
		long long int nCol = 0;
		long long int nComment = 0;
		string* sLine;
		string** sDataMatrix;

		// --> Oeffne die angegebene Datei <--
		file_in.open(sDataFile.c_str());

		// --> Hoppla! Offenbar ist da etwas nicht ganz richtig gelaufen! <--
		if (file_in.fail())
		{
            //sErrorToken = sDataFile;
            //throw DATAFILE_NOT_EXIST;
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
		}

		while (!file_in.eof())		// Zaehlen wir mal die Zeilen der Datei
		{
			if(file_in.fail())		// Sicherheit vor: Wer weiﬂ, ob da nicht ein Fehler inmitten des Lesevorgangs auftritt
			{
                //sErrorToken = sDataFile;
                //throw DATAFILE_NOT_EXIST;
                throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
			}
			// --> Schnapp' dir pro forma eine Zeile <--
			getline(file_in, s);

            stripTrailingSpaces(s);
			// --> Erhoehe den Counter <--
			if (s.length())
                nLine++;
		}

		if (!nLine)
		{
            //sErrorToken = sDataFile;
            //throw CANNOT_READ_FILE;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
		}

		file_in.clear();			// Den eofbit und den failbit aufheben
		file_in.seekg(0);			// Zurueck zum Dateianfang springen

		if(_option.getbDebug())
			cerr << "|-> DEBUG: nLine = " << nLine << endl;

		sLine = new string[nLine];	// Jetzt koennen wir auch ein Array fuer die Zeilen machen

		for (long long int i = 0; i < nLine; i++)
		{
			// --> Schreibe jetzt Zeile fuer Zeile in das string-Array <--
			getline(file_in, s);

			// --> Ersetze ggf. Tabulatoren durch Leerzeichen <--
			//replaceTabSign(s);
			stripTrailingSpaces(s);
			if (!s.length())
			{
                i--;
                continue;
			}
			else
                sLine[i] = s;

		}

		// --> Erst mal geschafft: Datei wieder schliessen <--
		file_in.close();

        if (_option.getbDebug())
            cerr << "|-> DEBUG: sLine[0] = " << sLine[0] << endl;

		if (sLine[0].find('.') != string::npos && sLine[0].find(',') != string::npos && sLine[0].find('\t') != string::npos)
		{
            cSep = ',';
        }
		else if (sLine[0].find(';') != string::npos && (sLine[0].find(',') != string::npos || sLine[0].find('.') != string::npos) && sLine[0].find('\t') != string::npos)
		{
            cSep = ';';
		}
		else if (sLine[0].find('\t') != string::npos)
        {
            cSep = '\t';
        }
        else if (sLine[0].find(';') != string::npos)
        {
            cSep = ';';
        }
        else if (sLine[0].find(',') != string::npos)
        {
            cSep = ',';
        }
        else if (sLine[0].find(' ') != string::npos && nLine > 1 && sLine[1].find(' ') != string::npos)
        {
            cSep = ' ';
        }
        else
        {
            if (sLine[0].find(',') != string::npos)
                cSep = ',';
            else if (sLine[0].find(';') != string::npos)
                cSep = ';';
            else if (sLine[0].find('\t') != string::npos)
                cSep = '\t';
            else if (sLine[0].find(' ') != string::npos)
                cSep = ' ';
            for (unsigned int i = 0; i < nLine; i++)
            {
                nCol = 1;
                for (unsigned int j = 0; j < sLine[i].length(); j++)
                {
                    if (sLine[i][j] == cSep)
                    {
                        nCol++;
                    }
                }
                if (!nCols)
                    nCols = nCol;
                else if (nCol != nCols)
                {
                    if (cSep == ',')
                        cSep = ';';
                    else if (cSep == ';')
                        cSep = ',';
                    else if (cSep == '\t')
                        cSep = ' ';
                    else if (cSep == ' ')
                        cSep = '\t';
                    nCols = 0;
                }
                else
                {
                    //nCols++;
                    break;
                }
                if (i+1 == nLine)
                {
                    nCols = 0;
                }
            }
            if (!nCols)
            {
                s = "";
                NumeReKernel::print(LineBreak(_lang.get("DATA_OPENCSV_COLUMNSEPARATOR_NOTFOUND"), _option));
                NumeReKernel::printPreFmt("|---<1>" + sLine[0].substr(0,65));
                if (sLine[0].length() > 65)
                    NumeReKernel::printPreFmt("[...]");
                NumeReKernel::printPreFmt("\n");
                if (nLine > 1)
                {
                    NumeReKernel::printPreFmt("|---<2>" + sLine[1].substr(0,65));
                    if (sLine[1].length() > 65)
                        NumeReKernel::printPreFmt("[...]");
                    NumeReKernel::printPreFmt("\n");
                }
                if (nLine > 2)
                {
                    NumeReKernel::printPreFmt("|---<3>" + sLine[2].substr(0,65));
                    if (sLine[2].length() > 65)
                        NumeReKernel::printPreFmt("[...]");
                    NumeReKernel::printPreFmt("\n");
                }
                do
                {
                    NumeReKernel::printPreFmt("|\n|<- ");
                    NumeReKernel::getline(s);
                }
                while (!s.length());
                cSep = s[0];
                if (cSep == ' ')
                    NumeReKernel::print(LineBreak(_lang.get("DATA_OPENCSV_SEPARATOR_WHITESPACE"), _option));
                else
                    NumeReKernel::print(LineBreak(_lang.get("DATA_OPENCSV_SEPARATOR", s.substr(0,1)), _option));
            }
        }

        if (_option.getbDebug())
            cerr << "|-> DEBUG: cSep = \"" << cSep << "\"" << endl;

        if (!nCols)
        {
            for (unsigned int i = 0; i < nLine; i++)
            {
                nCol = 1;
                for (unsigned int j = 0; j < sLine[i].length(); j++)
                {
                    if (sLine[i][j] == cSep)
                    {
                        nCol++;
                    }
                }
                if (!nCols)
                    nCols = nCol;
                else if (nCol != nCols)
                {
                    if (cSep == ',')
                        cSep = ';';
                    else if (cSep == ';')
                        cSep = ',';
                    else if (cSep == '\t')
                        cSep = ' ';
                    else if (cSep == ' ')
                        cSep = '\t';
                    nCols = 0;
                }
                else
                {
                    //nCols++;
                    break;
                }
                if (i+1 == nLine)
                {
                    delete[] sLine;
                    sLine = 0;
                    bValidData = false;
                    //sErrorToken = sDataFile;
                    //throw CANNOT_READ_FILE;
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
                }
            }
        }
        if (_option.getbDebug())
            cerr << "|-> DEBUG: nCols = " << nCols << endl;
        if (!sHeadLine)
            sHeadLine = new string[nCols];
        string sValidSymbols = "0123456789.,;-+eE ";
        sValidSymbols += cSep;
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sValidSymbols = \"" << sValidSymbols << "\"" << endl;
        for (unsigned int j = 0; j < sLine[0].length(); j++)
        {
            if (sValidSymbols.find(sLine[0][j]) == string::npos)
            {
                string __sLine = sLine[0];
                for (unsigned int n = 0; n < nCols-1; n++)
                {
                    sHeadLine[n] = __sLine.substr(0,__sLine.find(cSep));
                    StripSpaces(sHeadLine[n]);
                    if (!sHeadLine[n].length())
                        sHeadLine[n] = "Spalte_" + toString((int)n+1);
                    __sLine = __sLine.substr(__sLine.find(cSep)+1);
                }
                sHeadLine[nCols-1] = __sLine;
                for (unsigned int n = 0; n < nCols; n++)
                {
                    for (unsigned int k = 0; k < sHeadLine[n].length(); k++)
                    {
                        if (sValidSymbols.find(sHeadLine[n][k]) == string::npos)
                            break;
                        if (k == sHeadLine[n].length()-1)
                        {
                            nComment--;
                            break;
                        }
                    }
                    if (nComment < 0)
                    {
                        for (unsigned int k = 0; k < nCols; k++)
                        {
                            sHeadLine[k] = "Spalte_" + toString((int)k+1);
                        }
                        nComment = -1;
                        break;
                    }
                }
                if (!nComment)
                    sLine[0] = "";
                nComment++;
            }
        }

        if (_option.getbDebug())
            cerr << "|-> DEBUG: nComment = " << nComment << endl;
        if (nComment) // Problem: Offenbar scheinen die Eintraege nur aus Text zu bestehen --> Anderer Loesungsweg
        {
            nLines = nLine - 1;
        }
        else
            nLines = nLine;	// Die maximale Zahl der Zeilen ergibt die noetige Zahl der Zeilen

		sDataMatrix = new string*[nLines];	// Erzeugen wir nun eine Matrix, in der wir die Tokens einzeln speichern koennen
		for (long long int i = 0; i < nLines; i++)
		{
			sDataMatrix[i] = new string[nCols];
		}

		// --> Hier werden die Strings in Tokens zerlegt <--
		//nComment = 0;
		int n = 0;
		for (long long int i = 0; i < nLine; i++)
		{
            if (!sLine[i].length())
                continue;
            for (long long int j = 0; j < nCols-1; j++)
            {
                sDataMatrix[n][j] = sLine[i].substr(0,sLine[i].find(cSep));
                if (!sDataMatrix[n][j].length())
                    sDataMatrix[n][j] = "---";
                //replaceDecimalSign(sDataMatrix[n][j]);
                sLine[i] = sLine[i].substr(sLine[i].find(cSep)+1);
            }
            sDataMatrix[n][nCols-1] = sLine[i];
            if (!sDataMatrix[n][nCols-1].length() || sDataMatrix[n][nCols-1] == ";")
                sDataMatrix[n][nCols-1] = "---";
            //replaceDecimalSign(sDataMatrix[n][nCols-1]);
            n++;
		}

		if (!nComment)
		{
			if (nCols == 1)		// Hat nur eine Spalte: Folglich verwenden wir logischerweise den Dateinamen
			{
                if (sDataFile.find('/') == string::npos)
                    setHeadLineElement(0, "data", sDataFile.substr(0,sDataFile.rfind('.')));
                else
                    setHeadLineElement(0, "data", sDataFile.substr(sDataFile.rfind('/')+1, sDataFile.rfind('.')-1-sDataFile.rfind('/')));
			}
			else
			{
				for (long long int i = 0; i < nCols; i++)
				{
					sHeadLine[i] = "Spalte_" + toString(i+1);
				}
			}
		}

		/*
		 * --> Jetzt wissen wir, wie groﬂ dDatafile sein muss und koennen den Speicher allozieren <--
		 */
		if (nLines && nCols)
		{
			if (!dDatafile)
			{
				Allocate();
			}
			// --> Hier werden die strings in doubles konvertiert <--
			for (long long int i = 0; i < nLines; i++)
			{
				if(_option.getbDebug())
				{
					cerr << "|-> DEBUG: dDatafile[" << i << "] = ";
				}

				for (long long int j = 0; j < nCols; j++)
				{
					if (sDataMatrix[i][j] == "---"
                        || sDataMatrix[i][j] == "NaN"
                        || sDataMatrix[i][j] == "NAN"
                        || sDataMatrix[i][j] == "nan"
                        || sDataMatrix[i][j] == "inf"
                        || sDataMatrix[i][j] == "-inf")	// Liefert der Token den seltsamen '---'-String oder einen NaN?
					{
						// --> Aha! Da ist der gesuchte String. Dann ist das Wohl eine Leerzeile. Wir werden '0.0' in
						//	   den Datensatz schreiben und den Datenpunkt als zu ignorierende Nullzeile interpretieren <--
						dDatafile[i][j] = NAN;
						//bValidEntry[i][j] = false;
					}
					else
					{
                        for (unsigned int n = 0; n < sDataMatrix[i][j].length(); n++)
                        {
                            if (sValidSymbols.find(sDataMatrix[i][j][n]) == string::npos)
                            {
                                sHeadLine[j] += "\\n" + sDataMatrix[i][j];
                                dDatafile[i][j] = NAN;
                                //bValidEntry[i][j] = false;
                                break;
                            }
                            if (n+1 == sDataMatrix[i][j].length())
                            {
                                replaceDecimalSign(sDataMatrix[i][j]);
                                stringstream sstr;
                                sstr.precision(20);
                                sstr << sDataMatrix[i][j];
                                sstr >> dDatafile[i][j];
                                //bValidEntry[i][j] = true;
                            }
                        }
					}
					if(_option.getbDebug())
						cerr << dDatafile[i][j] << " ";
				}

				if(_option.getbDebug())
					cerr << endl;
			}
		}
		// --> WICHTIG: Jetzt sind ja Daten vorhanden. Also sollte der Boolean auch ein TRUE liefern <--
		bValidData = true;
        countAppendedZeroes();

		// --> Temporaeren Speicher wieder freigeben <--
		delete[] sLine;
		for (long long int i = 0; i < nLines; i++)
		{
			delete [] sDataMatrix[i];
		}
		delete[] sDataMatrix;
        /*if (!bAutoSave && !(bIgnore || _nHeadline))
            cin.ignore(1);*/
	}
	return;
}

// --> Oeffne eine JCAMP-DX-Datei <--
void Datafile::openJDX(Settings& _option)
{
	if (!bValidData)			// Es sind hoffentlich noch keine Daten gespeichert ...
	{
		if(_option.getbDebug())
			cerr << "|-> DEBUG: bValidData = false" << endl;

		//char cSep = 0;

		// --> Benoetigte temporaere Variablen initialisieren <--
		string s = "";
		//string sNumericChars = "0123456789.eE+-";
		long long int nLine = 0;
		//long long int nCol = 0;
		vector<long long int> vComment;
		vector<long long int> vCols;
		vector<double> vLine;
		vector<string> vDataSet;
		vector<vector<string> > vDataMatrix;
		string* sLine;

		// --> Oeffne die angegebene Datei <--
		file_in.open(sDataFile.c_str());

		// --> Hoppla! Offenbar ist da etwas nicht ganz richtig gelaufen! <--
		if (file_in.fail())
		{
           //sErrorToken = sDataFile;
            //throw DATAFILE_NOT_EXIST;
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
		}

		while (!file_in.eof())		// Zaehlen wir mal die Zeilen der Datei
		{
			if (file_in.fail())		// Sicherheit vor: Wer weiﬂ, ob da nicht ein Fehler inmitten des Lesevorgangs auftritt
			{
                //sErrorToken = sDataFile;
                //throw DATAFILE_NOT_EXIST;
                throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
			}
			// --> Schnapp' dir pro forma eine Zeile <--
			getline(file_in, s);


            stripTrailingSpaces(s);
            parseJDXDataLabel(s);
            if (s.find("##JCAMPDX=") != string::npos)
            {
                if (StrToDb(s.substr(s.find("##JCAMPDX=")+11)) > 5.1)
                {
                    NumeReKernel::print(LineBreak("HINWEIS: Die Versionsnummer dieses JCAMP-DX-Spektrums ist grˆﬂer als 5.1. Es besteht die Mˆglichkeit, dass NumeRe diese Datei nicht korrekt lesen kann. Es wird trotzdem versucht, die Daten zu interpretieren.", _option));
                }
            }
			// --> Erhoehe den Counter <--
			if (s.length())
                nLine++;
		}

		if (!nLine)
		{
            //sErrorToken = sDataFile;
            //sDataFile = "";
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
		}

		file_in.clear();			// Den eofbit und den failbit aufheben
		file_in.seekg(0);			// Zurueck zum Dateianfang springen

		if(_option.getbDebug())
			cerr << "|-> DEBUG: nLine = " << nLine << endl;

		sLine = new string[nLine];	// Jetzt koennen wir auch ein Array fuer die Zeilen machen

		for (long long int i = 0; i < nLine; i++)
		{
			// --> Schreibe jetzt Zeile fuer Zeile in das string-Array <--
			getline(file_in, s);

			// --> Ersetze ggf. Tabulatoren durch Leerzeichen <--
			//replaceTabSign(s);
			StripSpaces(s);
			parseJDXDataLabel(s);
			if (!s.length())
			{
                i--;
                continue;
			}
			else
                sLine[i] = s;
		}

		// --> Erst mal geschafft: Datei wieder schliessen <--
		file_in.close();

        if (_option.getbDebug())
            cerr << "|-> DEBUG: sLine[0] = " << sLine[0] << endl;

        for (long long int i = 0; i < nLine; i++)
        {
            vDataSet.push_back(sLine[i]);
            if (sLine[i].substr(0,6) == "##END=")
            {
                vDataMatrix.push_back(vDataSet);
                vDataSet.clear();
            }
        }

        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            for (unsigned int j = 0; j < vDataMatrix[i].size(); j++)
            {
                if (vDataMatrix[i][j].substr(0,2) == "##")
                {
                    if (vComment.size() == i)
                        vComment.push_back(1);
                    else
                        vComment[i]++;
                }
            }
        }

        nLines = vDataMatrix[0].size() - vComment[0];
        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            if (vDataMatrix[i].size() - vComment[i] > nLines)
                nLines = vDataMatrix[i].size() - vComment[i];
        }

        if (_option.getbDebug())
            cerr << "|-> DEBUG: nLines = " << nLines << endl;

        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            vLine = parseJDXLine(vDataMatrix[i][vComment[i]-1]);
            vCols.push_back(vLine.size());
            /*for (unsigned j = 0; j < vDataMatrix[i][vComment[i]-1].length(); j++)
            {
                if (sNumericChars.find(vDataMatrix[i][vComment[i]-1][j]) == string::npos)
                {
                    vCols[i]++;
                    while (sNumericChars.find(vDataMatrix[i][vComment[i]-1][j+1]) == string::npos)
                        j++;
                }
            }*/
        }
        for (unsigned int i = 0; i < vCols.size(); i++)
        {
            nCols += vCols[i];
        }

        if (_option.getbDebug())
            cerr << "|-> DEBUG: nCols = " << nCols << endl;

        Allocate();

        for (long long int i = 0; i < vDataMatrix.size(); i++)
        {
            double dXFactor = 1.0;
            double dYFactor = 1.0;

            string sXUnit = "";
            string sYUnit = "";

            string sDataType = "";
            string sXYScheme = "";

            for (long long int j = 0; j < vComment[i]-1; j++)
            {
                if (vDataMatrix[i][j].find("$$") != string::npos)
                    vDataMatrix[i][j].erase(vDataMatrix[i][j].find("$$"));
                if (vDataMatrix[i][j].substr(0,10) == "##XFACTOR=")
                    dXFactor = StrToDb(vDataMatrix[i][j].substr(10));
                if (vDataMatrix[i][j].substr(0,10) == "##YFACTOR=")
                    dYFactor = StrToDb(vDataMatrix[i][j].substr(10));

                if (vDataMatrix[i][j].substr(0,9) == "##XUNITS=")
                {
                    sXUnit = vDataMatrix[i][j].substr(9);
                    StripSpaces(sXUnit);

                    if (toUpperCase(sXUnit) == "1/CM")
                        sXUnit = "Wellenzahl k [cm^-1]";
                    else if (toUpperCase(sXUnit) == "MICROMETERS")
                        sXUnit = "Wellenl‰nge lambda [mu m]";
                    else if (toUpperCase(sXUnit) == "NANOMETERS")
                        sXUnit = "Wellenl‰nge lambda [nm]";
                    else if (toUpperCase(sXUnit) == "SECONDS")
                        sXUnit = "Zeit t [s]";
                    else if (toUpperCase(sXUnit) == "1/S" || toUpperCase(sXUnit) == "1/SECONDS")
                        sXUnit = "Frequenz f [Hz]";
                    else
                        sXUnit = "[" + sXUnit + "]";

                }
                if (vDataMatrix[i][j].substr(0,9) == "##YUNITS=")
                {
                    sYUnit = vDataMatrix[i][j].substr(9);
                    StripSpaces(sYUnit);

                    if (toUpperCase(sYUnit) == "TRANSMITTANCE")
                        sYUnit = "Transmission";
                    else if (toUpperCase(sYUnit) == "REFLECTANCE")
                        sYUnit = "Reflexion";
                    else if (toUpperCase(sYUnit) == "ABSORBANCE")
                        sYUnit = "Absorbtion";
                    else if (toUpperCase(sYUnit) == "KUBELKA-MUNK")
                        sYUnit = "Kubelka-Munk";
                    else if (toUpperCase(sYUnit) == "ARBITRARY UNITS" || sYUnit.substr(0,9) == "Intensity")
                        sYUnit = "Intensit‰t";
                }
                if (vDataMatrix[i][j].substr(0,11) == "##DATATYPE=")
                {
                    sDataType = vDataMatrix[i][j].substr(11);
                    StripSpaces(sDataType);
                }
                if (vDataMatrix[i][j].substr(0,11) == "##XYPOINTS=")
                {
                    sXYScheme = vDataMatrix[i][j].substr(11);
                    StripSpaces(sXYScheme);
                }
            }



            for (long long int j = vComment[i]-1; j < vDataMatrix[i].size()-1; j++)
            {
                if (_option.getbDebug() && j == vComment[i]-1)
                    cerr << "|-> DEBUG: vDataMatrix[i][j] = " << vDataMatrix[i][j] << endl;
                //int nCol = 0;
                if (vDataMatrix[i][j].substr(0,2) == "##")
                    continue;
                if (vDataMatrix[i][j].substr(0,6) == "##END=")
                    break;
                if (j-vComment[i]+1 == nLines)
                    break;
                vLine = parseJDXLine(vDataMatrix[i][j]);
                for (unsigned int k = 0; k < vLine.size(); k++)
                {
                    if (k == vCols[i])
                        break;
                    if (j == vComment[i]-1)
                    {
                        if (i)
                        {
                            sHeadLine[vCols[i-1]+k] = (k % 2 ? sYUnit : sXUnit);
                        }
                        else
                        {
                            sHeadLine[k] = (k % 2 ? sYUnit : sXUnit);
                        }
                    }
                    if (i)
                    {
                        if (k+1 == vDataMatrix[i][j].length())
                            dDatafile[j-vComment[i]+1][vCols[i-1]+k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        else
                            dDatafile[j-vComment[i]+1][vCols[i-1]+k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        /*if (!isnan(dDatafile[j-vComment[i]+1][vCols[i-1]+k]) && !isinf(dDatafile[j-vComment[i]+1][vCols[i-1]+k]))
                            bValidEntry[j-vComment[i]+1][vCols[i-1]+k] = true;*/
                    }
                    else
                    {
                        if (k+1 == vDataMatrix[i][j].length())
                            dDatafile[j-vComment[i]+1][k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        else
                            dDatafile[j-vComment[i]+1][k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        /*if (!isnan(dDatafile[j-vComment[i]+1][k]) && !isinf(dDatafile[j-vComment[i]+1][k]))
                            bValidEntry[j-vComment[i]+1][k] = true;*/
                    }
                }
            }
        }
        bValidData = true;
        countAppendedZeroes();


		// --> Temporaeren Speicher wieder freigeben <--
		delete[] sLine;
	}
	return;
}

// --> Oeffne eine NDAT-Datei (NumeRe-Binaerformat) <--
void Datafile::openNDAT(Settings& _option)
{
    if (sDataFile.substr(sDataFile.rfind('.')) != ".ndat")
        return;
    bool* bValidEntry = 0;
    char** cHeadLine = 0;
    long int nMajor = 0;
    long int nMinor = 0;
    long int nBuild = 0;
    size_t nLength = 0;
    if (!file_in.is_open())
        file_in.close();
    file_in.open(sDataFile.c_str(), ios_base::in | ios_base::binary);

    if (!bValidData && file_in.good())
    {
        time_t tTime = 0;
        file_in.read((char*)&nMajor, sizeof(long int));
        file_in.read((char*)&nMinor, sizeof(long int));
        file_in.read((char*)&nBuild, sizeof(long int));
        file_in.read((char*)&tTime, sizeof(time_t));
        file_in.read((char*)&nLines, sizeof(long long int));
        file_in.read((char*)&nCols, sizeof(long long int));
        //cerr << nLines << " " << nCols << endl;
        Allocate();
        bValidEntry = new bool[nCols];
        cHeadLine = new char*[nCols];
        for (long long int i = 0; i < nCols; i++)
        {
            nLength = 0;
            file_in.read((char*)&nLength, sizeof(size_t));
            //cerr << nLength << endl;
            cHeadLine[i] = new char[nLength];
            file_in.read(cHeadLine[i], sizeof(char)*nLength);
            //cerr << sHeadLine[i] << " " << cHeadLine[i] << " " << sHeadLine[i].length() << endl;
            sHeadLine[i].resize(nLength-1);
            for (unsigned int j = 0; j < nLength-1; j++)
            {
                sHeadLine[i][j] = cHeadLine[i][j];
                //cerr << sHeadLine[i][j] << " " << cHeadLine[i][j] << endl;
            }
        }
        //cerr << endl << nMajor << " " << nMinor << " " << nBuild << " " << nLines << " " << nCols << endl;
        file_in.read((char*)nAppendedZeroes, sizeof(long long int)*nCols);
        for (long long int i = 0; i < nLines; i++)
        {
            file_in.read((char*)dDatafile[i], sizeof(double)*nCols);
        }
        //cerr << endl << nMajor << " " << nMinor << " " << nBuild << " " << nLines << " " << nCols << endl;
        for (long long int i = 0; i < nLines; i++)
        {
            file_in.read((char*)bValidEntry, sizeof(bool)*nCols);
            for (long long int j = 0; j < nCols; j++)
            {
                if (!bValidEntry[j])
                    dDatafile[i][j] = NAN;
            }
        }
        //cerr << endl << nMajor << " " << nMinor << " " << nBuild << " " << nLines << " " << nCols << endl;
        file_in.close();
        bValidData = true;
    }
    else if (!file_in.good())
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
    }
    if (cHeadLine)
    {
        for (long long int i = 0; i < nCols; i++)
        {
            delete[] cHeadLine[i];
        }
        delete[] cHeadLine;
    }
    if (bValidEntry)
        delete[] bValidEntry;
    if (nMajor == 1 && nMinor == 0 && nBuild == 3)
        countAppendedZeroes();
    return;

}

// --> Oeffne eine IgorBinaryWave <---
void Datafile::openIBW(Settings& _option, bool bXZSlice, bool bYZSlice)
{
    CP_FILE_REF _cp_file_ref;
    int nType = 0;
    long nPnts = 0;
    void* vWaveDataPtr = 0;
    int nError = 0;
    long long int nFirstCol = 0;
    long int nDim[MAXDIMS];
    double dScalingFactorA[MAXDIMS];
    double dScalingFactorB[MAXDIMS];
    bool bReadComplexData = false;
    int nSliceCounter = 0;
    float* fData = 0;
    double* dData = 0;
    int8_t* n8_tData = 0;
    int16_t* n16_tData = 0;
    int32_t* n32_tData = 0;
    char* cName = 0;

    if ((nError = CPOpenFile(sDataFile.c_str(), 0, &_cp_file_ref)))
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }
    nError = ReadWave(_cp_file_ref, &nType, &nPnts, nDim, dScalingFactorA, dScalingFactorB, &vWaveDataPtr, &cName);
    if (nError)
    {
        CPCloseFile(_cp_file_ref);
        if (vWaveDataPtr != NULL)
            free(vWaveDataPtr);
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    if (nType & NT_CMPLX)
        bReadComplexData = true;

    if (nType & NT_FP32)
        fData = (float*)vWaveDataPtr;
    else if (nType & NT_FP64)
        dData = (double*)vWaveDataPtr;
    else if (nType & NT_I8)
        n8_tData = (int8_t*)vWaveDataPtr;
    else if (nType & NT_I16)
        n16_tData = (int16_t*)vWaveDataPtr;
    else if (nType & NT_I32)
        n32_tData = (int32_t*)vWaveDataPtr;
    else
    {
        CPCloseFile(_cp_file_ref);
        if (vWaveDataPtr != NULL)
            free(vWaveDataPtr);
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    nLines = nDim[0];
    nCols = nDim[1];
    if (nDim[2])
        nCols *= nDim[2];
    if (!nLines)
    {
        CPCloseFile(_cp_file_ref);
        if (vWaveDataPtr != NULL)
            free(vWaveDataPtr);
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::FILE_IS_EMPTY, "", SyntaxError::invalid_position, sDataFile);
    }
    if (!nCols || nCols == 1)
    {
        nCols = 2;
        if (bReadComplexData)
            nCols++;
        nFirstCol = 1;
    }
    else if (nDim[1] && (!nDim[2] || nDim[2] == 1))
    {
        if (bReadComplexData)
            nCols *= 2;
        nCols += 2;
        nFirstCol = 2;
        if (nDim[1] > nDim[0])
            nLines = nDim[1];
    }
    else if (nDim[1] && nDim[2] && (!nDim[3] || nDim[3] == 1))
    {
        if (bReadComplexData)
            nCols *= 2;
        nCols += 3;
        nFirstCol = 3;
        nLines = nDim[2] > nLines ? nDim[2] : nLines;
        nLines = nDim[1] > nLines ? nDim[1] : nLines;
    }

    //cerr << nDim[0] << "  " << nDim[1] << "  " << nDim[2] << "  " << std::max(nDim[0], nDim[1]) << endl;
    //cerr << nCols << "  " << nLines << "  " << nFirstCol << endl;
    Allocate();

    for (long long int j = 0; j < nFirstCol; j++)
    {
        sHeadLine[j] = cName + string("_[")+(char)('x'+j)+string("]");
        for (long long int i = 0; i < nDim[j]; i++)
        {
            dDatafile[i][j] = dScalingFactorA[j]*(double)i+dScalingFactorB[j];
            //bValidEntry[i][j] = true;
        }
    }

    //cerr << "scaling written" << endl;
    for (long long int j = 0; j < nCols-nFirstCol; j++)
    {
        if (bXZSlice && nDim[2] > 1 && j)
        {
            nSliceCounter += nDim[1];
            if (!(j % nDim[2]))
                nSliceCounter = j/nDim[2];
        }
        else
            nSliceCounter = j;
        //cerr << nSliceCounter << endl;
        if (nCols == 2 && !j)
        {
            sHeadLine[1] = cName + string("_[y]");
        }
        else if (nCols == 3 && !j && bReadComplexData)
        {
            sHeadLine[1] = string("Re:_") + cName + string("_[y]");
            sHeadLine[2] = string("Im:_") + cName + string("_[y]");
        }
        else if (!bReadComplexData)
            sHeadLine[j+nFirstCol] = cName + string("_["+toString(j+1)+"]");
        else
        {
            sHeadLine[j+nFirstCol] = string("Re:_") + cName + string("_["+toString(j+1)+"]");
            sHeadLine[j+nFirstCol+1] = string("Im:_") + cName + string("_["+toString(j+1)+"]");
        }
        for (long long int i = 0; i < (nDim[0]+bReadComplexData*nDim[0]); i++)
        {
            if (dData && !isnan(dData[i+j*(nDim[0]+bReadComplexData*nDim[0])]) && !isinf(dData[i+j*(nDim[0]+bReadComplexData*nDim[0])]))
            {
                dDatafile[i][nSliceCounter+nFirstCol] = dData[i+j*(nDim[0]+bReadComplexData*nDim[0])];
                //bValidEntry[i][nSliceCounter+nFirstCol] = true;
                if (bReadComplexData)
                {
                    dDatafile[i][nSliceCounter+1+nFirstCol] = dData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                    //bValidEntry[i][nSliceCounter+1+nFirstCol] = true;
                    i++;
                }
            }
            else if (fData && !isnan(fData[i+j*(nDim[0]+bReadComplexData*nDim[0])] && !isinf(fData[i+j*(nDim[0]+bReadComplexData*nDim[0])])))
            {
                dDatafile[i][nSliceCounter+nFirstCol] = fData[i+j*(nDim[0]+bReadComplexData*nDim[0])];
                //bValidEntry[i][nSliceCounter+nFirstCol] = true;
                if (bReadComplexData)
                {
                    dDatafile[i][nSliceCounter+1+nFirstCol] = fData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                    //bValidEntry[i][nSliceCounter+1+nFirstCol] = true;
                    i++;
                }
            }
            else if (n8_tData)
            {
                dDatafile[i][nSliceCounter+nFirstCol] = (double)n8_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];
                //bValidEntry[i][nSliceCounter+nFirstCol] = true;
                if (bReadComplexData)
                {
                    dDatafile[i][nSliceCounter+1+nFirstCol] = (double)n8_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                    //bValidEntry[i][nSliceCounter+1+nFirstCol] = true;
                    i++;
                }
            }
            else if (n16_tData)
            {
                dDatafile[i][nSliceCounter+nFirstCol] = (double)n16_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];
                //bValidEntry[i][nSliceCounter+nFirstCol] = true;
                if (bReadComplexData)
                {
                    dDatafile[i][nSliceCounter+1+nFirstCol] = (double)n16_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                    //bValidEntry[i][nSliceCounter+1+nFirstCol] = true;
                    i++;
                }
            }
            else if (n32_tData)
            {
                dDatafile[i][nSliceCounter+nFirstCol] = (double)n32_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];
                //bValidEntry[i][nSliceCounter+nFirstCol] = true;
                if (bReadComplexData)
                {
                    dDatafile[i][nSliceCounter+1+nFirstCol] = (double)n32_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                    //bValidEntry[i][nSliceCounter+1+nFirstCol] = true;
                    i++;
                }
            }
            else
                continue;
        }
        if (bReadComplexData)
            j++;
    }

    //cerr << "data copied" << endl;
    bValidData = true;
    countAppendedZeroes();

    CPCloseFile(_cp_file_ref);
    if (vWaveDataPtr != NULL)
        free(vWaveDataPtr);
    return;
}

void Datafile::openODS(Settings& _option)
{
    //cerr << toLowerCase(sDataFile.substr(sDataFile.rfind('.'))) << "'" << endl;
    if (toLowerCase(sDataFile.substr(sDataFile.rfind('.'))) != ".ods")
    {
        //cerr << "Extension" << endl;
        return;
    }
    string sODS = "";
    string sODS_substr = "";
    vector<string> vTables;
    vector<string> vMatrix;
    //string* sCols = 0;
    //string** sDataMatrix = 0;
    long long int nCommentLines = 0;
    long long int nMaxCols = 0;


    Zipfile _zip;
    if (!_zip.open(sDataFile))
    {
        _zip.close();
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
    }
    sODS = _zip.getZipItem("content.xml");
    _zip.close();

    if (!sODS.length())
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }
    sODS.erase(0,sODS.find("<office:spreadsheet>"));
    if (!sODS.length())
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    //cerr << sODS.substr(0,500) << endl;

    sODS.erase(0,sODS.find("<table:table "));
    while (sODS.size() && sODS.find("<table:table ") != string::npos && sODS.find("</table:table>") != string::npos)
    {
        vTables.push_back(sODS.substr(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table ")));
        sODS.erase(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table "));
    }
    if (!vTables.size())
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }


    for (unsigned int i = 0; i < vTables.size(); i++)
    {
        unsigned int nPos = 0;
        unsigned int nCount = 0;
        long long int _nCols = 0;
        string sLine = "";
        while (vTables[i].find("<table:table-row ", nPos) != string::npos && vTables[i].find("</table:table-row>", nPos) != string::npos)
        {
            //cerr << "0" << endl;
            nPos = vTables[i].find("<table:table-row ", nPos);
            sLine = vTables[i].substr(nPos, vTables[i].find("</table:table-row>", nPos)+18-nPos);
            //cerr << "1" << endl;
            sLine = Datafile::expandODSLine(sLine);
            //cerr << "2" << endl;
            if (sLine.length())
            {
                if (!i)
                    vMatrix.push_back(sLine);
                else
                {
                    if (vMatrix.size() <= nCount)
                    {
                        vMatrix.push_back("<>");
                        for (long long int n = 1; n < nMaxCols; n++)
                            vMatrix[nCount] += "<>";
                        vMatrix[nCount] += sLine;
                    }
                    else
                        vMatrix[nCount] += sLine;
                    nCount++;
                }
            }
            nPos++;
        }
        //cerr << "3" << endl;
        for (unsigned int j = 0; j < vMatrix.size(); j++)
        {
            _nCols = 0;
            for (unsigned int n = 0; n < vMatrix[j].length(); n++)
            {
                if (vMatrix[j][n] == '<')
                    _nCols++;
            }
            //cerr << _nCols << endl;
            if (_nCols > nMaxCols)
                nMaxCols = _nCols;
        }
        for (unsigned int j = 0; j < vMatrix.size(); j++)
        {
            _nCols = 0;
            for (unsigned int n = 0; n < vMatrix[j].length(); n++)
            {
                if (vMatrix[j][n] == '<')
                    _nCols++;
            }
            if (_nCols < nMaxCols)
            {
                for (long long int k = _nCols; k < nMaxCols; k++)
                    vMatrix[j] += "<>";
            }
        }
    }
    if (_option.getbDebug())
    {
        for (unsigned int i = 0; i < vMatrix.size(); i++)
            cerr << vMatrix[i] << endl;
        cerr << "lines=" << vMatrix.size() << ", cols = " << nMaxCols << endl;
    }
    // Replacements
    for (unsigned int i = 0; i < vMatrix.size(); i++)
    {
        while (vMatrix[i].find(' ') != string::npos)
            vMatrix[i].replace(vMatrix[i].find(' '),1,"_");
    }

    if (!vMatrix.size() || !nMaxCols)
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    for (unsigned int i = 0; i < vMatrix.size(); i++)
    {
        bool bBreak = false;
        for (unsigned int j = 0; j < vMatrix[i].length(); j++)
        {
            if (vMatrix[i][j] == '<' && vMatrix[i][j+1] != '>')
            {
                if (isNumeric(vMatrix[i].substr(j+1, vMatrix[i].find('>',j)-j-1)))
                {
                    bBreak = true;
                    break;
                }
            }
            if (j == vMatrix[i].length()-1)
                nCommentLines++;
        }
        if (bBreak)
            break;
    }

    // Allokation
    nLines = vMatrix.size() - nCommentLines;
    nCols = nMaxCols;
    Allocate();

    // Interpretation
    unsigned int nPos = 0;
    for (long long int i = 0; i < nCommentLines; i++)
    {
        nPos = 0;
        for (long long int j = 0; j < nCols; j++)
        {
            nPos = vMatrix[i].find('<', nPos);
            string sEntry = vMatrix[i].substr(nPos,vMatrix[i].find('>', nPos)+1-nPos);
            nPos++;
            if (sEntry == "<>")
                continue;
            sEntry.erase(0,1);
            sEntry.pop_back();
            if (sHeadLine[j] == "Spalte_"+toString(j+1))
            {
                sHeadLine[j] = utf8parser(sEntry);
            }
            else if (sHeadLine[j] == utf8parser(sEntry))
                continue;
            else
                sHeadLine[j] += "\\n" + utf8parser(sEntry);
        }
    }
    for (long long int i = 0; i < nLines; i++)
    {
        nPos = 0;
        for (long long int j = 0; j < nCols; j++)
        {
            nPos = vMatrix[i+nCommentLines].find('<', nPos);
            string sEntry = vMatrix[i+nCommentLines].substr(nPos,vMatrix[i+nCommentLines].find('>', nPos)+1-nPos);
            nPos++;
            if (sEntry == "<>")
                continue;
            sEntry.erase(0,1);
            sEntry.pop_back();
            if (isNumeric(sEntry))
            {
                if (sEntry.find_first_not_of('-') == string::npos)
                    continue;
                dDatafile[i][j] = StrToDb(sEntry);
                //bValidEntry[i][j] = true;
            }
            else if (sHeadLine[j] == "Spalte_"+toString(j+1))
            {
                sHeadLine[j] = utf8parser(sEntry);
            }
            else if (sHeadLine[j] == utf8parser(sEntry))
                continue;
            else
                sHeadLine[j] += "\\n" + utf8parser(sEntry);
        }
    }
    bValidData = true;
    countAppendedZeroes();

    return;
}

void Datafile::openXLS(Settings& _option)
{
    if (toLowerCase(sDataFile.substr(sDataFile.rfind('.'))) != ".xls")
    {
        //cerr << "Extension" << endl;
        return;
    }
    BasicExcel _excel;
    BasicExcelWorksheet* _sheet;
    BasicExcelCell* _cell;
    unsigned int nSheets = 0;
    long long int nExcelLines = 0;
    long long int nExcelCols = 0;
    long long int nOffset = 0;
    long long int nCommentLines = 0;
    vector<long long int> vCommentLines;
    bool bBreakSignal = false;
    string sEntry;

    if (!_excel.Load(sDataFile.c_str()))
    {
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }//cerr << "Loaded" << endl;

    // get the total number
    nSheets = _excel.GetTotalWorkSheets();
    if (!nSheets)
        return;

    //cerr << "sheets = " << nSheets << endl;
    // get the total size
    for (unsigned int n = 0; n < nSheets; n++)
    {
        _sheet = _excel.GetWorksheet(n);
        bBreakSignal = false;
        nCommentLines = 0;
        for (unsigned int i = 0; i < _sheet->GetTotalRows(); i++)
        {
            for (unsigned int j = 0; j < _sheet->GetTotalCols(); j++)
            {
                if (_sheet->Cell(i,j)->Type() != BasicExcelCell::STRING
                    && _sheet->Cell(i,j)->Type() != BasicExcelCell::WSTRING
                    && _sheet->Cell(i,j)->Type() != BasicExcelCell::UNDEFINED)
                {
                    bBreakSignal = true;
                    break;
                }
            }
            if (bBreakSignal)
                break;
            nCommentLines++;
        }
        vCommentLines.push_back(nCommentLines);
        if (nExcelLines < _sheet->GetTotalRows()-nCommentLines)
            nExcelLines = _sheet->GetTotalRows()-nCommentLines;
        nExcelCols += _sheet->GetTotalCols();
    }

    // create data
    nLines = nExcelLines;
    nCols = nExcelCols;
    Allocate();

    // copy data/strings
    for (unsigned int n = 0; n < nSheets; n++)
    {
        _sheet = _excel.GetWorksheet(n);
        nExcelCols = _sheet->GetTotalCols();
        nExcelLines = _sheet->GetTotalRows();

        for (long long int i = 0; i < vCommentLines[n]; i++)
        {
            if (i >= nExcelLines)
                break;
            for (long long int j = 0; j < nExcelCols; j++)
            {
                if (j+nOffset >= nCols)
                    break;
                _cell = _sheet->Cell(i,j);
                if (_cell->Type() == BasicExcelCell::STRING)
                    sEntry = _cell->GetString();
                else if (_cell->Type() == BasicExcelCell::WSTRING)
                    sEntry = wcstombs(_cell->GetWString());
                else
                    continue;
                while (sEntry.find('\n') != string::npos)
                    sEntry.replace(sEntry.find('\n'), 1, "\\n");
                while (sEntry.find((char)10) != string::npos)
                    sEntry.replace(sEntry.find((char)10), 1, "\\n");

                if (sHeadLine[j+nOffset] == "Spalte_"+toString(j+1+nOffset))
                {
                    sHeadLine[j+nOffset] = utf8parser(sEntry);
                }
                else if (sHeadLine[j+nOffset] == utf8parser(sEntry))
                    continue;
                else
                    sHeadLine[j+nOffset] += "\\n" + utf8parser(sEntry);
            }
        }
        nOffset += nExcelCols;
    }

    nOffset = 0;

    for (unsigned int n = 0; n < nSheets; n++)
    {
        _sheet = _excel.GetWorksheet(n);
        nExcelCols = _sheet->GetTotalCols();
        nExcelLines = _sheet->GetTotalRows();

        for (long long int i = vCommentLines[n]; i < nExcelLines; i++)
        {
            if (i-vCommentLines[n] >= nLines)
                break;
            for (long long int j = 0; j < nExcelCols; j++)
            {
                if (j >= nCols)
                    break;
                _cell = _sheet->Cell(i,j);
                sEntry.clear();
                switch (_cell->Type())
                {
                    case BasicExcelCell::UNDEFINED:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = NAN;
                        break;
                    case BasicExcelCell::INT:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = (double)_cell->GetInteger();
                        break;
                    case BasicExcelCell::DOUBLE:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = _cell->GetDouble();
                        break;
                    case BasicExcelCell::STRING:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = NAN;
                        sEntry = _cell->GetString();
                        break;
                    case BasicExcelCell::WSTRING:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = NAN;
                        sEntry = wcstombs(_cell->GetWString());
                        break;
                    default:
                        dDatafile[i-vCommentLines[n]][j+nOffset] = NAN;
                }
                if (sEntry.length())
                {
                    if (sHeadLine[j+nOffset] == "Spalte_"+toString(j+1+nOffset))
                    {
                        sHeadLine[j+nOffset] = utf8parser(sEntry);
                    }
                    else if (sHeadLine[j+nOffset] == utf8parser(sEntry))
                        continue;
                    else
                        sHeadLine[j+nOffset] += "\\n" + utf8parser(sEntry);
                }
            }
        }

        nOffset += nExcelCols;
    }
    bValidData = true;
    countAppendedZeroes();

    return;
}


void Datafile::openXLSX(Settings& _option)
{
    if (toLowerCase(sDataFile.substr(sDataFile.rfind('.'))) != ".xlsx")
    {
        return;
    }

    using namespace tinyxml2;

    unsigned int nSheets = 0;
    long long int nExcelLines = 0;
    long long int nExcelCols = 0;
    long long int nOffset = 0;
    int nLine = 0, nCol = 0;
    int nLinemin = 0, nLinemax = 0;
    int nColmin = 0, nColmax = 0;
    bool bBreakSignal = false;

    vector<long long int> vCommentLines;
    string sEntry;
    string sSheetContent;
    string sStringsContent;
    string sCellLocation;

    XMLDocument _workbook;
    XMLDocument _sheet;
    XMLDocument _strings;
    XMLNode* _node;
    XMLElement* _element;
    XMLElement* _stringelement;

    Zipfile _zip;
    if (!_zip.open(sDataFile))
    {
        _zip.close();
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
    }
    sEntry = _zip.getZipItem("xl/workbook.xml");
    //cerr << sEntry << endl;
    if (!sEntry.length())
    {
        _zip.close();
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    // Get the number of sheets
    _workbook.Parse(sEntry.c_str());
    //cerr << "parsed" << endl;
    if (_workbook.ErrorID())
    {
        _zip.close();
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }
    _node = _workbook.FirstChildElement()->FirstChildElement("sheets")->FirstChild();
    //cerr << "node" << endl;
    if (_node)
        nSheets++;
    while ((_node = _node->NextSibling()))
        nSheets++;
    //cerr << nSheets << endl;

    if (!nSheets)
    {
        _zip.close();
        //sErrorToken = sDataFile;
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
    }

    // Walk through the sheets and extract the dimension info
    for (unsigned int i = 0; i < nSheets; i++)
    {
        sSheetContent = _zip.getZipItem("xl/worksheets/sheet"+toString(i+1)+".xml");
        //cerr << sSheetContent.length() << endl;
        if (!sSheetContent.length())
        {
            _zip.close();
            //sErrorToken = sDataFile;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
        }
        _sheet.Parse(sSheetContent.c_str());
        if (_sheet.ErrorID())
        {
            _zip.close();
            //sErrorToken = sDataFile;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
        }
        _element = _sheet.FirstChildElement()->FirstChildElement("dimension");
        //cerr << "element" << endl;
        sCellLocation = _element->Attribute("ref");
        //cerr << sCellLocation << endl;
        int nLinemin = 0, nLinemax = 0;
        int nColmin = 0, nColmax = 0;

        // Take care of comment lines => todo
        evalExcelIndices(sCellLocation.substr(0,sCellLocation.find(':')), nLinemin, nColmin);
        evalExcelIndices(sCellLocation.substr(sCellLocation.find(':')+1), nLinemax, nColmax);

        vCommentLines.push_back(0);
        _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();
        if (!_node)
            continue;
        //cerr << "node" << endl;
        do
        {
            _element = _node->ToElement()->FirstChildElement("c");
            //cerr << "element" << endl;
            do
            {
                if (_element->Attribute("t"))
                {
                    //cerr << _element->Attribute("t") << endl;
                    if (_element->Attribute("t") != string("s"))
                    {
                        bBreakSignal = true;
                        break;
                    }
                }
                else
                {
                    bBreakSignal = true;
                    break;
                }
            }
            while ((_element = _element->NextSiblingElement()));
            if (!bBreakSignal)
                vCommentLines[i]++;
            else
                break;
        }
        while ((_node = _node->NextSibling()));
        bBreakSignal = false;

        if (nExcelLines < nLinemax-nLinemin+1-vCommentLines[i])
            nExcelLines = nLinemax-nLinemin+1-vCommentLines[i];
        nExcelCols += nColmax-nColmin+1;
    }

    nLines = nExcelLines;
    nCols = nExcelCols;
    //cerr << vCommentLines.size() << endl;
    //cerr << nLines << endl << nCols << endl;

    //Allocate the memory
    Allocate();

    // Walk through the sheets and extract the contents to memory
    sStringsContent = _zip.getZipItem("xl/sharedStrings.xml");
    //cerr << sStringsContent << endl;
    _strings.Parse(sStringsContent.c_str());
    for (unsigned int i = 0; i < nSheets; i++)
    {
        //cerr << vCommentLines[i] << endl;
        sSheetContent = _zip.getZipItem("xl/worksheets/sheet"+toString(i+1)+".xml");
        _sheet.Parse(sSheetContent.c_str());
        _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();
        //cerr << _node << endl;
        _element = _sheet.FirstChildElement()->FirstChildElement("dimension");
        if (!_node)
            continue;

        sCellLocation = _element->Attribute("ref");
        evalExcelIndices(sCellLocation.substr(0,sCellLocation.find(':')), nLinemin, nColmin);
        evalExcelIndices(sCellLocation.substr(sCellLocation.find(':')+1), nLinemax, nColmax);
        do
        {
            _element = _node->ToElement()->FirstChildElement("c");
            if (!_element)
                continue;
            do
            {
                sCellLocation = _element->Attribute("r");
                //cerr << sCellLocation;
                evalExcelIndices(sCellLocation, nLine, nCol);
                nCol -= nColmin;
                nLine -= nLinemin;
                if (nCol+nOffset >= nCols || nLine-vCommentLines[i] >= nLines)
                    continue;
                if (_element->Attribute("t"))
                {
                    if (_element->Attribute("t") == string("s"))
                    {
                        //Handle text
                        int nPos = 0;
                        _element->FirstChildElement("v")->QueryIntText(&nPos);
                        _stringelement = _strings.FirstChildElement()->FirstChildElement("si");
                        for (int k = 1; k <= nPos; k++)
                        {
                            _stringelement = _stringelement->NextSiblingElement();
                        }
                        if (_stringelement->FirstChildElement()->FirstChild())
                            sEntry = _stringelement->FirstChildElement()->FirstChild()->ToText()->Value();
                        else
                            sEntry.clear();
                        //cerr << sEntry << endl;
                        if (sEntry.length())
                        {
                            if (sHeadLine[nCol+nOffset] == "Spalte_"+toString(nCol+1+nOffset))
                            {
                                sHeadLine[nCol+nOffset] = utf8parser(sEntry);
                            }
                            else if (sHeadLine[nCol+nOffset] == utf8parser(sEntry))
                                continue;
                            else
                                sHeadLine[nCol+nOffset] += "\\n" + utf8parser(sEntry);
                        }
                        continue;
                    }
                }
                //cerr << "   Loc: ";
                if (_element->FirstChildElement("v"))
                    _element->FirstChildElement("v")->QueryDoubleText(&dDatafile[nLine-vCommentLines[i]][nCol+nOffset]);
                //cerr << nLine-vCommentLines[i] << ", " << nCol+nOffset << endl;
            }
            while ((_element = _element->NextSiblingElement()));
        }
        while ((_node = _node->NextSibling()));

        nOffset += nColmax-nColmin+1;
    }
    _zip.close();
    bValidData = true;
    countAppendedZeroes();

    return;
}

// --> Oeffne eine Datei, lies sie ein und interpretiere die Daten als Double <--
void Datafile::openFile(string _sFile, Settings& _option, bool bAutoSave, bool bIgnore, int _nHeadline)
{
	if (!bValidData)			// Es sind hoffentlich noch keine Daten gespeichert ...
	{
		if(_option.getbDebug())
			cerr << "|-> DEBUG: bValidData = false, _sFile = " << _sFile << endl;

		boost::char_separator<char> sep(" ");				// Setze " " als Zeichentrenner fuer den Tokenizer -> Wird spaeter verwendet

        sDataFile = FileSystem::ValidFileName(_sFile);
        if (!fileExists(sDataFile) && (_sFile.find('.') == string::npos || _sFile.find('.') < _sFile.rfind('/')))
            sDataFile = FileSystem::ValidFileName(_sFile+".*");

		if(_option.getbDebug())
			cerr << "|-> DEBUG: sDataFile = " << sDataFile << endl;

        string sExt = "";
        if (sDataFile.find('.') != string::npos)
            sExt = toLowerCase(sDataFile.substr(sDataFile.rfind('.')));
        else
            sExt = ".NOEXT";

        if (sExt == ".labx")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".labx");
            Datafile::openLabx(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".csv")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".csv");
            Datafile::openCSV(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".ndat")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".ndat");
            Datafile::openNDAT(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".jdx"
            || sExt == ".dx"
            || sExt == ".jcm")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".jdx");
            Datafile::openJDX(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".ibw")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".ibw");
            if (_nHeadline == -1)
                Datafile::openIBW(_option, true, false);
            else if (_nHeadline == -2)
                Datafile::openIBW(_option, false, true);
            else
                Datafile::openIBW(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".ods")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".ods");
            Datafile::openODS(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".xls")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".xls");
            Datafile::openXLS(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt == ".xlsx")
        {
            sDataFile = FileSystem::ValidFileName(_sFile, ".xlsx");
            Datafile::openXLSX(_option);
            Datafile::condenseDataSet();
            return;
        }
        else if (sExt != ".dat"
            && sExt != ".txt"
            && sExt != ".NOEXT")
        {
            string sErrorToken = sDataFile;
            sDataFile = "";
            throw SyntaxError(SyntaxError::INVALID_FILETYPE, "", SyntaxError::invalid_position, sErrorToken);
        }

        if (_nHeadline < 0)
            _nHeadline = 0;

		// --> Benoetigte temporaere Variablen initialisieren <--
		string s = "";
		long long int nLine = 0;
		long long int nCol = 0;
		long long int nComment = 0;
		string* sLine;
		string** sDataMatrix;

		// --> Oeffne die angegebene Datei <--
		file_in.open(sDataFile.c_str());

		// --> Hoppla! Offenbar ist da etwas nicht ganz richtig gelaufen! <--
		if (file_in.fail())
		{
            //sErrorToken = sDataFile;
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
		}

		while (!file_in.eof())		// Zaehlen wir mal die Zeilen der Datei
		{
			if (file_in.fail())		// Sicherheit vor: Wer weiﬂ, ob da nicht ein Fehler inmitten des Lesevorgangs auftritt
			{
                string sErrorToken = sDataFile;
                sDataFile = "";
                throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sDataFile);
			}
			// --> Schnapp' dir pro forma eine Zeile <--
			getline(file_in, s);

            stripTrailingSpaces(s);
			// --> Erhoehe den Counter <--
			if (s.length())
                nLine++;
		}

		if (!nLine)
		{
            //sErrorToken = sDataFile;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDataFile);
        }
		file_in.clear();			// Den eofbit und den failbit aufheben
		file_in.seekg(0);			// Zurueck zum Dateianfang springen

		if (_option.getbDebug())
			cerr << "|-> DEBUG: nLine = " << nLine << endl;

		sLine = new string[nLine];	// Jetzt koennen wir auch ein Array fuer die Zeilen machen

		for (long long int i = 0; i < nLine; i++)
		{
			// --> Schreibe jetzt Zeile fuer Zeile in das string-Array <--
			getline(file_in, s);

			// --> Ersetze ggf. Tabulatoren durch Leerzeichen <--
			if (!isNumeric(s))
                replaceTabSign(s, true);
            else
            {
                replaceTabSign(s);
                stripTrailingSpaces(s);
			}
			if (!s.length())
			{
                i--;
                continue;
			}
			else
                sLine[i] = s;
		}

		// --> Erst mal geschafft: Datei wieder schliessen <--
		file_in.close();

		// --> Kommentare muessen auch noch escaped werden... <--
		for (long long int i = 0; i < nLine; i++)
		{
			if (sLine[i][0] == '#' || !isNumeric(sLine[i]))	// ist das erste Zeichen ein #?
			{
				nComment++;			// Kommentarzeilen zaehlen
				if(_option.getbDebug())
					cerr << "|-> DEBUG: Kommentar >> continue!" << endl;
				continue; 			// Dann ist das wohl ein Kommentar -> Ueberspringen
			}
			else if (!nCol)			// Suche so lange, bis du eine Zeile findest, die nicht kommentiert wurde
			{						// Sobald die Spaltenzahl einmal bestimmt wurde, braucht das hier nicht mehr durchgefuehrt werden
				// --> Verwende einen Tokenizer, um den String in Teile zu zerlegen und diese zu zaehlen <--
				tokenizer<char_separator<char> > tok(sLine[i],sep);
                for(tokenizer<char_separator<char> >::iterator beg=tok.begin(); beg !=tok.end(); ++beg)
				{
					nCol++;			// Zaehle die Elemente, die der Tokenizer liefert
					if(_option.getbDebug())
						cerr << "|-> DEBUG: Token >> "<< *beg << endl;
				}

				if(_option.getbDebug())
					cerr << "|-> DEBUG: nCol = " << nCol << endl;
			}
		}

		nLines = nLine - nComment;	// Die maximale Zahl der Zeilen - die Zahl der Kommentare ergibt die noetige Zahl der Zeilen
		nCols = nCol;

		sDataMatrix = new string*[nLines];	// Erzeugen wir nun eine Matrix, in der wir die Tokens einzeln speichern koennen
		for (long long int i = 0; i < nLines; i++)
		{
			sDataMatrix[i] = new string[nCols];
		}

		// --> Hier werden die Strings in Tokens zerlegt <--
		nComment = 0;
		for (long long int i = 0; i < nLine; i++)
		{
			if (sLine[i][0] == '#' || !isNumeric(sLine[i]))
			{
				nComment++;
				continue;
			}
			else
			{
				// --> Tokenizer wieder... Was hier genau passiert... Aehem. Besser nicht fragen ... <--
				tokenizer<char_separator<char> > tok(sLine[i],sep);
				int j = 0;

				for (tokenizer<char_separator<char> >::iterator beg=tok.begin(); beg != tok.end(); ++beg)
				{
                    if (j >= nCols)
                    {
                        for (long long int n = 0; n < nLines; n++)
                        {
                            delete[] sDataMatrix[n];
                        }
                        delete[] sDataMatrix;
                        delete[] sLine;
                        string sErrorToken = sDataFile;
                        nCols = 0;
                        nLines = 0;
                        sDataFile = "";
                        throw SyntaxError(SyntaxError::COL_COUNTS_DOESNT_MATCH, "", SyntaxError::invalid_position, sErrorToken);
                    }
					sDataMatrix[i-nComment][j] = *beg; // Wenn das klappt, dann stehen jetzt in den einzelnen Elementen die zerlegten tokens.
                    // --> Sollte die Daten mit Kommata als Dezimaltrennzeichen vorliegen, werden die hier ersetzt <--
					replaceDecimalSign(sDataMatrix[i-nComment][j]);
					j++;
				}
			}
		}

		// --> Tabellen benoetigen eine saubere Kopfzeile ... <--
		if (nLines && nCols) // Machen wir schon einmal ein Array fuer die Kopfzeile
		{
            Allocate();
		}
		else
		{
            for (long long int n = 0; n < nLines; n++)
            {
                delete[] sDataMatrix[n];
            }
            delete[] sDataMatrix;
            delete[] sLine;
            string sErrorToken = sDataFile;
            nCols = 0;
            nLines = 0;
            sDataFile = "";
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sErrorToken);
		}

        // --> Wurde die Datei schon von NumeRe erzeugt? Dann sind die Kopfzeilen einfach zu finden <--
        if (nComment && !_nHeadline)
        {
            string sCommentSign = "#";
            if (nLine > 14)
                sCommentSign.append(sLine[13].length()-1, '=');
            if (nComment >= 15 && sLine[2].substr(0,21) == "# NumeRe: Framework f")
            {
                for (long long int k = 13; k < nLine; k++)
                {
                    if (sLine[k] == sCommentSign)
                    {
                        _nHeadline = 14;
                        break;
                    }
                }
                //_nHeadline = 14;
            }
            else if (nComment == 1)
            {
                for (long long int i = 0; i < nLine; i++)
                {
                    if (sLine[i][0] == '#')
                    {
                        if (sLine[i].find(' ') != string::npos)
                        {
                            if (sLine[i][1] != ' ')
                            {
                                for (unsigned int n = 0; n < sLine[i].length(); n++)
                                {
                                    if (sLine[i][n] != '#')
                                    {
                                        if (sLine[i][n] != ' ')
                                        {
                                            sLine[i] = sLine[i].substr(0,n) + " " + sLine[i].substr(n);
                                        }
                                        break;
                                    }
                                }
                            }
                            int n = 0;
                            tokenizer<char_separator<char> > tok(sLine[i], sep);
                            for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                            {
                                if (string(*iter).find_first_not_of('_') != string::npos)
                                    n++;
                            }
                            if (n-1 == nCols)
                                _nHeadline = 1;
                        }
                        break;
                    }
                    else if (!isNumeric(sLine[i]))
                    {
                        int n = 0;
                        tokenizer<char_separator<char> > tok(sLine[i], sep);
                        for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                        {
                            if (string(*iter).find_first_not_of('_') != string::npos)
                                n++;
                        }
                        if (n == nCols)
                            _nHeadline = 1;
                    }
                }
            }
            else if (sLine[0][0] == '#' || !isNumeric(sLine[0]))
            {
                for (long long int i = 0; i < nLine; i++)
                {
                    if (sLine[i][0] != '#' && isNumeric(sLine[i]))
                    {
                        if (sLine[i-1][0] == '#')
                        {
                            if ((nCols > 1 && sLine[i-1].find(' ') != string::npos) || (nCols == 1 && sLine[i-1].length() > 1))
                            {
                                if (sLine[i-1][1] != ' ')
                                {
                                    for (unsigned int n = 0; n < sLine[i-1].length(); n++)
                                    {
                                        if (sLine[i-1][n] != '#')
                                        {
                                            if (sLine[i-1][n] != ' ')
                                            {
                                                sLine[i-1] = sLine[i-1].substr(0,n) + " " + sLine[i-1].substr(n);
                                            }
                                            break;
                                        }
                                    }
                                }

                                int n = 0;
                                tokenizer<char_separator<char> > tok(sLine[i-1], sep);
                                for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                                {
                                    if (string(*iter).find_first_not_of('_') != string::npos)
                                        n++;
                                }
                                if (n-1 == nCols)
                                {
                                    _nHeadline = i;
                                    break;
                                }
                            }
                            if (i > 1 && sLine[i-2][0] == '#' && ((sLine[i-2].find(' ') != string::npos && nCols > 1) || (nCols == 1 && sLine[i-2].length() > 1)))
                            {
                                if (sLine[i-2][1] != ' ')
                                {
                                    for (unsigned int n = 0; n < sLine[i-2].length(); n++)
                                    {
                                        if (sLine[i-2][n] != '#')
                                        {
                                            if (sLine[i-2][n] != ' ')
                                            {
                                                sLine[i-2] = sLine[i-2].substr(0,n) + " " + sLine[i-2].substr(n);
                                            }
                                            break;
                                        }
                                    }
                                }

                                int n = 0;
                                tokenizer<char_separator<char> > tok(sLine[i-2], sep);
                                for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                                {
                                    if (string(*iter).find_first_not_of('_') != string::npos)
                                        n++;
                                }
                                if (n-1 == nCols)
                                    _nHeadline = i-1;
                            }
                        }
                        else if (!isNumeric(sLine[i-1]))
                        {
                            if ((sLine[i-1].find(' ') != string::npos && nCols > 1) || (nCols == 1 && sLine[i-1].length() > 1))
                            {
                                if (sLine[i-1][1] != ' ')
                                {
                                    for (unsigned int n = 0; n < sLine[i-1].length(); n++)
                                    {
                                        if (sLine[i-1][n] != '#')
                                        {
                                            if (sLine[i-1][n] != ' ')
                                            {
                                                sLine[i-1] = sLine[i-1].substr(0,n) + " " + sLine[i-1].substr(n);
                                            }
                                            break;
                                        }
                                    }
                                }

                                int n = 0;
                                tokenizer<char_separator<char> > tok(sLine[i-1], sep);
                                for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                                {
                                    if (string(*iter).find_first_not_of('_') != string::npos)
                                        n++;
                                }
                                if (n == nCols)
                                {
                                    _nHeadline = i;
                                    break;
                                }
                            }
                            if (i > 1 && sLine[i-2][0] == '#' && ((sLine[i-2].find(' ') != string::npos && nCols > 1) || (nCols == 1 && sLine[i-2].length() > 1)))
                            {
                                if (sLine[i-2][1] != ' ')
                                {
                                    for (unsigned int n = 0; n < sLine[i-2].length(); n++)
                                    {
                                        if (sLine[i-2][n] != '#')
                                        {
                                            if (sLine[i-2][n] != ' ')
                                            {
                                                sLine[i-2] = sLine[i-2].substr(0,n) + " " + sLine[i-2].substr(n);
                                            }
                                            break;
                                        }
                                    }
                                }

                                int n = 0;
                                tokenizer<char_separator<char> > tok(sLine[i-2], sep);
                                for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                                {
                                    if (string(*iter).find_first_not_of('_') != string::npos)
                                        n++;
                                }
                                if (n == nCols)
                                    _nHeadline = i-1;
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (_option.getbDebug())
            cerr << "|-> DEBUG: _nHeadline = " << _nHeadline << endl;
		if (nComment && !bIgnore && !_nHeadline) 	// Offenbar gibt es Kommentare, wenn nComment != 0 --> Vielleicht gibt es dann auch Tabellenkoepfe?
		{

            string sInput = "";
            if (!bAutoSave)
            {
                NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_TABLEHEADINGS"), _option));
                NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sInput);
            }
			if (sInput == _lang.YES() || bAutoSave)	// Es gibt wohl welche. Nun, dann wird's etwas kompliziert ...
			{
                int nHeadLine = 0;
                if (!bAutoSave)
                {
                    NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_LISTING_COMMENTS")+" ...", _option));
                    NumeReKernel::printPreFmt("|\n");
                    nComment = 0;

                    for (long long int i = 0; i < nLine; i++)
                    {
                        if (sLine[i][0] == '#' || !isNumeric(sLine[i]))
                        {
                            NumeReKernel::printPreFmt("|-");

                            if (nComment < 9)
                                NumeReKernel::printPreFmt("-");

                            NumeReKernel::printPreFmt("<" + toString(nComment+1) + "> " + sLine[i] + "\n");
                            nComment++;
                        }
                    }

                    NumeReKernel::printPreFmt("|\n");
                    NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_ENTERNUMBER"), _option));
                    NumeReKernel::printPreFmt("|\n|<- ");
                    NumeReKernel::getline(sInput);
                    nHeadLine = StrToInt(sInput);
                }
                else
                    nHeadLine = 14;

				if (nHeadLine)
				{
					while (nHeadLine > nComment || nHeadLine < 0)	// Oha! Eine Zahl eingeben, die groesser als die Zahl an Kommentarzeilen ist, geht aber nicht!
					{
						NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_LINEDOESNTEXIST", toString(nComment)), _option));
						NumeReKernel::printPreFmt("|\n|<- ");
						cin >> nHeadLine;
					}

					long long int n = 0; 		// zweite Zaehlvariable

					for (long long int i = 0; i < nLine; i++)
					{
						if (sLine[i][0] == '#' || !isNumeric(sLine[i]))
						{
							n++;	// Erst n erhoehen, da man zuvor auch mit 1 angefangen hat, zu zaehlen

							if (n == nHeadLine) // Da ist sie ja, die Zeile...
							{
								tokenizer<char_separator<char> > tok(sLine[i], sep);
								long long int j = -1;

								for(tokenizer<char_separator<char> >::iterator beg=tok.begin(); beg != tok.end(); ++beg)
								{
									if (j == nCols)
									{
                                        NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_REPLACING_HEADS"), _option));
                                        for (n = 0; n < nCols; n++)
                                        {
                                            sHeadLine[n] = "Spalte_" + toString(n+1);
                                        }
										break;
									}

									if (j == -1)	// Wir muessen den ersten Token ueberspringen: Er beinhaltet das '#'!
									{
										j++;
										continue;
									}

									sHeadLine[j] = *beg;
									j++;
								}
								break; //Jetzt haben wir sie ja gefunden. Raus aus der Schleife!
							}
						}
					}
				}
				else
				{
					NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_AUTOMATIC_HEADS"), _option));

					for (long long int i = 0; i < nCols; i++)
					{
						sHeadLine[i] = "Spalte_" + toString(i+1);
					}
				}
			}
			else				// Gibt es keine? Dann moechte man womoeglich selbst welche eingeben?
			{
				NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_ENTER_HEADINGS"), _option));
				NumeReKernel::printPreFmt("|\n|<- ");
                NumeReKernel::getline(sInput);

				if (sInput == _lang.YES()) // Ja? Dann viel Spaﬂ...
				{
					NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_ENTERYOURHEADS"), _option));
					NumeReKernel::printPreFmt("|\n");

					string _sHead;
					//cin.ignore(1);
					for (long long int i = 0; i < nCols; i++)
					{
						NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_HEADFORCOLUMN", toString(i+1)), _option));
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(_sHead);
						setHeadLineElement(i, "data", _sHead);

					}
				}
				else			// Nein? Dann generieren wir einfach welche...
				{
					NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_AUTOMATIC_HEADS"), _option));

					for (long long int i = 0; i < nCols; i++)
					{
						sHeadLine[i] = "Spalte_" + toString(i+1);
					}
				}
			}
		}
		else if (_nHeadline)
		{
            if (_nHeadline > nComment || _nHeadline < 0)
            {
                string sInput = "";
                while (_nHeadline > nComment || _nHeadline < 0)	// Oha! Eine Zahl eingeben, die groesser als die Zahl an Kommentarzeilen ist, geht aber nicht!
                {
                    NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_LINEDOESNTEXIST", toString(nComment)), _option));
                    NumeReKernel::printPreFmt("|\n|<- ");
                    NumeReKernel::getline(sInput);
                    _nHeadline = StrToInt(sInput);
                }
            }
            long long int n = 0; 		// zweite Zaehlvariable

            for (long long int i = 0; i < nLine; i++)
            {
                if (sLine[i][0] == '#' || !isNumeric(sLine[i]))
                {
                    n++;	// Erst n erhoehen, da man zuvor auch mit 1 angefangen hat, zu zaehlen

                    if (n == _nHeadline) // Da ist sie ja, die Zeile...
                    {
                        for (long long int k = i+1; k < nLine; k++)
                        {
                            // TAB-Replace ignorieren
                            if (sLine[k].find(" _ ") != string::npos || (sLine[k].find_first_not_of(" #") != string::npos && sLine[k][sLine[k].find_first_not_of(" #")] == '_') || sLine[k].back() == '_')
                            {
                                break;
                            }

                            if (sLine[k][0] != '#' && isNumeric(sLine[k]))
                                break;
                            if (sLine[k].substr(0,4) == "#===" || sLine[k].substr(0,5) == "# ===")
                                break;
                            if (sLine[k].length() == sLine[i].length())
                            {
                                for (unsigned int l = 0; l < sLine[k].length(); l++)
                                {
                                    if (sLine[i][l] != ' ' && sLine[k][l] == ' ')
                                        sLine[k][l] = '_';
                                }
                            }
                            else if (sLine[k].length() < sLine[i].length() && sLine[i][sLine[k].length()-1] != ' ' && sLine[k].back() != ' ')
                            {
                                sLine[k].append(sLine[i].length()-sLine[k].length(),' ');
                                for (unsigned int l = 0; l < sLine[k].length(); l++)
                                {
                                    if (sLine[i][l] != ' ' && sLine[k][l] == ' ')
                                        sLine[k][l] = '_';
                                }
                            }
                            else if (sLine[k].length() > sLine[i].length() && sLine[k][sLine[i].length()-1] != ' ' && sLine[i].back() != ' ')
                            {
                                for (unsigned int l = 0; l < sLine[i].length(); l++)
                                {
                                    if (sLine[i][l] != ' ' && sLine[k][l] == ' ')
                                        sLine[k][l] = '_';
                                }
                            }
                            else
                                break;
                        }
                        bool bBreakSignal = false;
                        vector<string> vHeadline;
                        //vHeadline.reserve((unsigned int)(2*nCols));
                        vHeadline.resize((unsigned int)(2*nCols), "");

                        for (long long int k = i; k < nLine; k++)
                        {
                            //cerr << sLine[k] << endl;
                            if (sLine[k][0] != '#' && isNumeric(sLine[k]))
                                break;
                            if (sLine[k].substr(0,4) == "#===" || sLine[k].substr(0,5) == "# ===")
                                break;

                            tokenizer<char_separator<char> > tok(sLine[k], sep);
                            long long int j = -1;
                            if (sLine[i][0] != '#')
                                j = 0;

                            for (tokenizer<char_separator<char> >::iterator beg=tok.begin(); beg != tok.end(); ++beg)
                            {
                                /*if (j == nCols)
                                {
                                    cerr << LineBreak("|-> "+_lang.get("DATA_OPENFILE_REPLACING_HEADS"), _option) << endl;
                                    for (n = 0; n < nCols; n++)
                                    {
                                        sHeadLine[n] = "Spalte_" + toString(n+1);
                                    }
                                    bBreakSignal = true;
                                    break;
                                }*/

                                if (j == -1)	// Wir muessen den ersten Token ueberspringen: Er beinhaltet das '#'!
                                {
                                    j++;
                                    continue;
                                }
                                if (k == i)
                                    vHeadline.push_back("");
                                if (k != i && (unsigned)j >= vHeadline.size())
                                {
                                    bBreakSignal = true;
                                    break;
                                }


                                string sHead = *beg;
                                if (sHead.find_first_not_of('_') == string::npos)
                                {
                                    j++;
                                    continue;
                                }
                                while (sHead.front() == '_')
                                    sHead.erase(0,1);
                                while (sHead.back() == '_')
                                    sHead.pop_back();

                                if (!vHeadline[j].length())
                                    vHeadline[j] = sHead;
                                else
                                {
                                    vHeadline[j] += "\\n";
                                    vHeadline[j] += sHead;
                                }

                                /*if (sHeadLine[j] == "Spalte_"+toString(j+1))
                                    sHeadLine[j] = sHead;
                                else
                                {
                                    sHeadLine[j] += "\\n";
                                    sHeadLine[j] += sHead;
                                }*/
                                j++;
                            }

                            if (bBreakSignal)
                                break;
                            /*if (j < nCols-i)
                            {
                                for (; j < nCols; j++)
                                {
                                    sHeadLine[j] = "Spalte_" + toString(j+1);
                                }
                            }*/
                        }
                        for (auto iter = vHeadline.begin(); iter != vHeadline.end(); ++iter)
                        {
                            if (!(iter->length()))
                            {
                                iter = vHeadline.erase(iter);
                                iter--;
                            }
                        }

                        if (vHeadline.size() <= nCols)
                        {
                            for (n = 0; n < nCols; n++)
                            {
                                if (vHeadline.size() > n)
                                    sHeadLine[n] = vHeadline[n];
                                else
                                    sHeadLine[n] = "Spalte_" + toString(n+1);
                            }
                        }
                        else
                        {
                            NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_REPLACING_HEADS"), _option));
                            for (n = 0; n < nCols; n++)
                            {
                                sHeadLine[n] = "Spalte_" + toString(n+1);
                            }
                        }

                        break; //Jetzt haben wir sie ja gefunden. Raus aus der Schleife!
                    }
                }
            }
		}
		else // Okay... keine Kommentarzeilen. Trotzdem automatische Koepfe!
		{
			if (nCols == 1)		// Hat nur eine Spalte: Folglich verwenden wir logischerweise den Dateinamen
			{
                if (sDataFile.find('/') == string::npos)
                    setHeadLineElement(0, "data", sDataFile.substr(0,sDataFile.rfind('.')));
                else
                    setHeadLineElement(0, "data", sDataFile.substr(sDataFile.rfind('/')+1, sDataFile.rfind('.')-1-sDataFile.rfind('/')));
			}
			/*else
			{
				for (long long int i = 0; i < nCols; i++)
				{
					sHeadLine[i] = "Spalte_" + toString(i+1);
				}
			}*/
		}

		/*
		 * --> Jetzt wissen wir, wie groﬂ dDatafile sein muss und koennen den Speicher allozieren <--
		 */
		if (nLines && nCols)
		{
			if (!dDatafile)
			{
				Allocate();
			}
			// --> Hier werden die strings in doubles konvertiert <--
			for (long long int i = 0; i < nLines; i++)
			{
				if (_option.getbDebug())
				{
					cerr << "|-> DEBUG: dDatafile[" << i << "] = ";
				}

				for (long long int j = 0; j < nCols; j++)
				{
					if (sDataMatrix[i][j] == "---"
                        || sDataMatrix[i][j] == "NaN"
                        || sDataMatrix[i][j] == "NAN"
                        || sDataMatrix[i][j] == "nan"
                        || sDataMatrix[i][j] == "inf"
                        || sDataMatrix[i][j] == "-inf")	// Liefert der Token den seltsamen '---'-String oder einen NaN?
					{
						// --> Aha! Da ist der gesuchte String. Dann ist das Wohl eine Leerzeile. Wir werden '0.0' in
						//	   den Datensatz schreiben und den Datenpunkt als zu ignorierende Nullzeile interpretieren <--
						dDatafile[i][j] = NAN;
						//bValidEntry[i][j] = false;
					}
					else
					{
						// --> Nein, dann wandle in double um <--
						stringstream sstr;			// Wir brauchen mal wieder einen stringstream
						sstr.precision(20);     	// Praezision setzen
						sstr << sDataMatrix[i][j];	// machen wir aus den tokens mal Zahlen
						sstr >> dDatafile[i][j];	// schreiben wir den Stringstream in die Daten-Matrix
						//bValidEntry[i][j] = true;	// Dieser Eintrag existiert nun offensichtlich!
					}
					if(_option.getbDebug())
						cerr << dDatafile[i][j] << " ";
				}

				if(_option.getbDebug())
					cerr << endl;
			}
		}
		// --> WICHTIG: Jetzt sind ja Daten vorhanden. Also sollte der Boolean auch ein TRUE liefern <--
		bValidData = true;
        countAppendedZeroes();
        condenseDataSet();
		// --> Temporaeren Speicher wieder freigeben <--
		delete[] sLine;
		for (long long int i = 0; i < nLines; i++)
		{
			delete [] sDataMatrix[i];
		}
		delete[] sDataMatrix;
        /*if (!bAutoSave && !(bIgnore || _nHeadline))
            cin.ignore(1);*/
	}
	else		// Oh! Es waren doch schon Daten vorhanden... Was jetzt? Anhaengen? / Ueberschreiben?
	{
		string cReplace = "";
		NumeReKernel::print(LineBreak(_lang.get("DATA_OPENFILE_OVERWRITE_DATA", sDataFile), _option));
		NumeReKernel::printPreFmt("|\n|<- ");
		NumeReKernel::getline(cReplace);

		if (cReplace == _lang.YES())
		{
			removeData();				// Zuerst mal die Daten loeschen
			openFile(_sFile, _option, bAutoSave, bIgnore);	// Jetzt die Methode erneut aufrufen
		}
		else
		{
			NumeReKernel::print(toSystemCodePage(_lang.get("COMMON_CANCEL")));
		}

	}

	return;
}

vector<string> Datafile::getPastedDataFromCmdLine(const Settings& _option, bool& bKeepEmptyTokens)
{
    vector<string> vPaste;
    string sLine;
    make_hline();
    NumeReKernel::print(LineBreak("NUMERE: "+toUpperCase(_lang.get("DATA_PASTE_HEADLINE")), _option));
    make_hline();
    NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("DATA_PASTE_DESCRIPTION"), _option) + "\n|\n");
    while (true)
    {
        NumeReKernel::printPreFmt("|PASTE> ");
        NumeReKernel::getline(sLine);
        if (sLine == "endpaste")
            break;
        if (!isNumeric(sLine) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ' ')
                    sLine[i] = '_';
            }
            if (!bKeepEmptyTokens)
                bKeepEmptyTokens = true;
        }
        replaceTabSign(sLine);
        if (sLine.find_first_not_of(' ') == string::npos)
            continue;
        if (isNumeric(sLine) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
            replaceDecimalSign(sLine);
        else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                    sLine[i] = '.';
                if (sLine[i] == ';')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        vPaste.push_back(sLine);
    }
    make_hline();
    return vPaste;
}

void Datafile::reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1)
{
    double* dSortVector = new double[i2-i1+1];
    for (int i = 0; i <= i2-i1; i++)
    {
        dSortVector[i] = dDatafile[vIndex[i]][j1];
    }

    for (int i = 0; i <= i2-i1; i++)
    {
        dDatafile[i+i1][j1] = dSortVector[i];
    }
    delete[] dSortVector;
}

// --> Lese den Inhalt eines Tabellenpastes <--
void Datafile::pasteLoad(const Settings& _option)
{
    if (!bValidData && !dDatafile)
    {
        vector<string> vPaste;
        string sLine = "";
        string sClipboard = getClipboardText();
        boost::char_separator<char> sep(" ");
        long long int nSkip = 0;
        long long int nColsTemp = 0;
        bool bKeepEmptyTokens = false;
        bool bReadClipboard = (bool)(sClipboard.length());

        //cerr << sClipboard << endl;

        if (!sClipboard.length())
        {
            vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
        }
        else
        {
            while (true)
            {
                if (!sClipboard.length() || sClipboard == "\n")
                    break;
                sLine = sClipboard.substr(0, sClipboard.find('\n'));
                if (sLine.back() == (char)13) // CR entfernen
                    sLine.pop_back();
                //cerr << sLine << " " << (int)sLine.back() << endl;
                if (sClipboard.find('\n') != string::npos)
                    sClipboard.erase(0, sClipboard.find('\n')+1);
                else
                    sClipboard.clear();
                //StripSpaces(sLine);
                if (!isNumeric(sLine) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
                {
                    for (unsigned int i = 0; i < sLine.length(); i++)
                    {
                        if (sLine[i] == ' ')
                            sLine[i] = '_';
                    }
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                }
                replaceTabSign(sLine);
                if (sLine.find_first_not_of(' ') == string::npos)
                    continue;
                if (isNumeric(sLine) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
                    replaceDecimalSign(sLine);
                else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
                {
                    for (unsigned int i = 0; i < sLine.length(); i++)
                    {
                        if (sLine[i] == ',')
                            sLine[i] = '.';
                        if (sLine[i] == ';')
                        {
                            if (!bKeepEmptyTokens)
                                bKeepEmptyTokens = true;
                            sLine[i] = ' ';
                        }
                    }
                }
                else
                {
                    for (unsigned int i = 0; i < sLine.length(); i++)
                    {
                        if (sLine[i] == ',')
                        {
                            if (!bKeepEmptyTokens)
                                bKeepEmptyTokens = true;
                            sLine[i] = ' ';
                        }
                    }
                }
                vPaste.push_back(sLine);
            }
            if (!vPaste.size())
            {
                bReadClipboard = false;
                bKeepEmptyTokens = false;
                vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
            }
        }
        //cerr << vPaste.size() << endl;
        if (!vPaste.size())
            return;

        nLines = vPaste.size();
        //cerr << nLines << endl;
        for (unsigned int i = 0; i < vPaste.size(); i++)
        {
            if (!isNumeric(vPaste[i]))
            {
                nLines--;
                nSkip++;
                if (nLines > i+1 && vPaste[i+1].find(' ') == string::npos && vPaste[i].find(' ') != string::npos)
                {
                    for (unsigned int j = 0; j < vPaste[i].size(); j++)
                    {
                        if (vPaste[i][j] == ' ')
                            vPaste[i][j] = '_';
                    }
                }
            }
            else
                break;
        }
        //cerr << nLines << endl;
        if (!nLines && !bReadClipboard)
        {
            NumeReKernel::print(LineBreak(_lang.get("DATA_COULD_NOT_IDENTIFY_PASTED_CONTENT"), _option));
            return;
        }
        else if (bReadClipboard && !nLines)
        {
            bKeepEmptyTokens = false;
            vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
            if (!vPaste.size())
                return;
            nLines = vPaste.size();
            nSkip = 0;
            //cerr << nLines << endl;
            for (unsigned int i = 0; i < vPaste.size(); i++)
            {
                if (!isNumeric(vPaste[i]))
                {
                    nLines--;
                    nSkip++;
                    if (nLines > i+1 && vPaste[i+1].find(' ') == string::npos && vPaste[i].find(' ') != string::npos)
                    {
                        for (unsigned int j = 0; j < vPaste[i].size(); j++)
                        {
                            if (vPaste[i][j] == ' ')
                                vPaste[i][j] = '_';
                        }
                    }
                }
                else
                    break;
            }
            if (!nLines)
            {
                NumeReKernel::print(LineBreak(_lang.get("DATA_COULD_NOT_IDENTIFY_PASTED_CONTENT"), _option));
                return;
            }
        }

        if (bKeepEmptyTokens)
        {
            sep = char_separator<char>(" ","", boost::keep_empty_tokens);
        }
        for (unsigned int i = 0; i < vPaste.size(); i++)
        {
            nColsTemp = 0;
            stripTrailingSpaces(vPaste[i]);
            //cerr << vPaste[i] << endl;
            tokenizer<char_separator<char> > tok(vPaste[i],sep);
            for (auto iter = tok.begin(); iter != tok.end(); ++iter)
                nColsTemp++;
            if (nColsTemp > nCols)
                nCols = nColsTemp;
        }

        sDataFile = "Pasted Data";

        Allocate();
        for (unsigned int i = 0; i < vPaste.size(); i++)
        {
            tokenizer<char_separator<char> > tok(vPaste[i], sep);
            long long int j = 0;
            for (auto iter = tok.begin(); iter != tok.end(); ++iter)
            {
                sLine = *iter;
                if (sLine[sLine.length()-1] == '%')
                    sLine.erase(sLine.length()-1);
                if (isNumeric(sLine) && sLine != "NAN" && sLine != "NaN" && sLine != "nan")
                {
                    if (i < nSkip && nSkip)
                    {
                        if (sLine.length())
                        {
                            if (!i)
                                sHeadLine[j] = sLine;
                            else
                                sHeadLine[j] += "\\n" + sLine;
                        }
                    }
                    else
                    {
                        dDatafile[i-nSkip][j] = StrToDb(sLine);
                        //bValidEntry[i-nSkip][j] = true;
                    }
                }
                else if (i < nSkip && nSkip)
                {
                    if (sLine.length())
                    {
                        if (!i || sHeadLine[j] == "Spalte_" + toString(j+1))
                            sHeadLine[j] = sLine;
                        else
                            sHeadLine[j] += "\\n" + sLine;
                    }
                }
                else
                {
                    dDatafile[i-nSkip][j] = NAN;
                    //bValidEntry[i-nSkip][j] = false;
                }
                j++;
                if (j == nCols)
                    break;
            }
        }
    }
    else
    {
        Datafile _tempData;
        _tempData.pasteLoad(_option);
        melt(_tempData);
    }
    countAppendedZeroes();
    Datafile::condenseDataSet();
    bValidData = true;
    return;
}

void Datafile::condenseDataSet()
{
    if (!bValidData)
        return;
    if (bLoadEmptyCols || bLoadEmptyColsInNextFile)
    {
        bLoadEmptyColsInNextFile = false;
        return;
    }
    for (long long int i = 0; i < nCols; i++)
    {
        if (nAppendedZeroes[i] == nLines)
        {
            break;
        }
        if (i+1 == nCols)
            return;
    }

    double dDataTemp[nLines][nCols];
    //bool bValidTemp[nLines][nCols];
    string sHeadTemp[nCols];
    string sDataTemp = sDataFile;
    long long int nAppendedTemp[nCols];
    long long int nColTemp = nCols;
    long long int nLinesTemp = nLines;
    long long int nEmptyCols = 0;
    long long int nSkip = 0;

    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            dDataTemp[i][j] = dDatafile[i][j];
            //bValidTemp[i][j] = bValidEntry[i][j];
            if (!i)
            {
                nAppendedTemp[j] = nAppendedZeroes[j];
                sHeadTemp[j] = sHeadLine[j];
            }
        }
    }

    removeData(false);

    for (long long int i = 0; i < nColTemp; i++)
    {
        if (nAppendedTemp[i] == nLinesTemp)
            nEmptyCols++;
    }

    if (nEmptyCols == nColTemp)
        return;

    nLines = nLinesTemp;
    nCols = nColTemp - nEmptyCols;

    Allocate();

    for (long long int i = 0; i < nLines; i++)
    {
        nSkip = 0;
        for (long long int j = 0; j < nColTemp; j++)
        {
            if (nAppendedTemp[j] == nLines)
            {
                nSkip++;
                continue;
            }
            dDatafile[i][j-nSkip] = dDataTemp[i][j];
            //bValidEntry[i][j-nSkip] = bValidTemp[i][j];
            if (!i)
            {
                sHeadLine[j-nSkip] = sHeadTemp[j];
                nAppendedZeroes[j-nSkip] = nAppendedTemp[j];
            }
        }
    }
    sDataFile = sDataTemp;
    for (unsigned int i = 0; i < nCols; i++)
    {
        if (sHeadLine[i].substr(0,7) == "Spalte_" && sHeadLine[i].find_first_not_of("0123456789\0", 7) == string::npos && sHeadLine[i] != "Spalte_"+toString((int)i+1))
            sHeadLine[i] = "Spalte_"+toString((int)i+1);
    }
    bValidData = true;
    return;
}

string Datafile::expandODSLine(const string& sLine)
{
    string sExpandedLine = "";
    //cerr << sLine << endl;
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine.substr(i,17) == "<table:table-cell")
        {
            if (sLine[sLine.find('>',i)-1] != '/')
            {
                // --> Value-reader
                string sCellEntry = sLine.substr(i, sLine.find("</table:table-cell>", i)-i);
                if (sCellEntry.find("office:value-type=") != string::npos && getArgAtPos(sCellEntry, sCellEntry.find("office:value-type=")+18) == "float")
                {
                    sExpandedLine += "<" + getArgAtPos(sCellEntry, sCellEntry.find("office:value=")+13) + ">";
                }
                else if (sCellEntry.find("<text:p>") != string::npos)
                    sExpandedLine += "<" + sCellEntry.substr(sCellEntry.find("<text:p>")+8, sCellEntry.find("</text:p>")-sCellEntry.find("<text:p>")-8) + ">";
            }
            else
            {
                if (sLine.find("<table:table-cell", i+1) == string::npos && sLine.find("<table:covered-table-cell", i+1) == string::npos)
                    break;
                if (sLine.substr(i, sLine.find('>',i)+1-i).find("table:number-columns-repeated=") != string::npos)
                {
                    string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);
                    if (sTemp.front() == '"')
                        sTemp.erase(0,1);
                    if (sTemp.back() == '"')
                        sTemp.pop_back();
                    //cerr << "stemp=" << sTemp << endl;
                    //cerr << "val=" << StrToInt(sTemp) << endl;
                    for (int j = 0; j < StrToInt(sTemp); j++)
                        sExpandedLine += "<>";
                }
                else
                    sExpandedLine += "<>";
            }
        }
        else if (sLine.substr(i,25) == "<table:covered-table-cell")
        {
            string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);
            if (sTemp.front() == '"')
                sTemp.erase(0,1);
            if (sTemp.back() == '"')
                sTemp.pop_back();
            for (int j = 0; j < StrToInt(sTemp); j++)
                sExpandedLine += "<>";
        }
    }

    if (sExpandedLine.length())
    {
        //cerr << "length_i: " << sExpandedLine.length() << endl;
        while (sExpandedLine.substr(sExpandedLine.length()-2) == "<>")
        {
            sExpandedLine.erase(sExpandedLine.length()-2);
            if (!sExpandedLine.length() || sExpandedLine.length() < 2)
                break;
        }
        //cerr << "length_f: " << sExpandedLine.length() << endl;
        //cerr << sExpandedLine << endl;
    }
    return sExpandedLine;
}

vector<double> Datafile::parseJDXLine(const string& sLine)
{
    vector<double> vLine;
    string sValue = "";
    const string sNumericChars = "0123456789.+-";

    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if ((sLine[i] >= '0' && sLine[i] <= '9') || sLine[i] == '.')
            sValue += sLine[i];
        else if (sValue.length()
            && (sLine[i] == 'e' || sLine[i] == 'E')
            && sNumericChars.find(sValue[0]) != string::npos)
            sValue += sLine[i];
        else if (sValue.length()
            && (sLine[i] == '+' || sLine[i] == '-')
            && sNumericChars.find(sValue[0]) != string::npos
            && (sValue[sValue.length()-1] == 'e' || sValue[sValue.length()-1] == 'E'))
            sValue += sLine[i];
        else
        {
            if (sValue.length())
            {
                if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
                {
                    sValue[0] = toString(sValue[0]-'J'+1)[0];
                    vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
                }
                else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
                {
                    sValue[0] = toString(sValue[0]-'j'+1)[0];
                    vLine.push_back(vLine[vLine.size()-1]-StrToDb(sValue));
                }
                else if (sValue[0] == '%')
                {
                    sValue[0] = '0';
                    vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
                }
                else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
                {
                    if (sValue[0] == 's')
                    {
                        for (int j = 0; j < 9; j++)
                            vLine.push_back(vLine[vLine.size()-1]);
                    }
                    else
                    {
                        for (int j = 0; j <= sValue[0]-'S'; j++)
                            vLine.push_back(vLine[vLine.size()-1]);
                    }
                }
                else
                    vLine.push_back(StrToDb(sValue));
                sValue.clear();
            }
            if (sLine[i] >= 'A' && sLine[i] <= 'I')
                sValue += toString(sLine[i]-'A'+1)[0];
            else if (sLine[i] >= 'a' && sLine[i] <= 'i')
                sValue += toString(sLine[i]-'a'+1)[0];
            else if ((vLine.size()
                && ((sLine[i] >= 'J' && sLine[i] <= 'R')
                    || (sLine[i] >= 'j' && sLine[i] <= 'r')
                    || sLine[i] == '%'
                    || (sLine[i] >= 'S' && sLine[i] <= 'Z')
                    || sLine[i] == 's'))
                    || sLine[i] == '+'
                    || sLine[i] == '-')
                sValue += sLine[i];
        }
    }
    if (sValue.length())
    {
        if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
        {
            sValue[0] = toString(sValue[0]-'J'+1)[0];
            vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
        }
        else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
        {
            sValue[0] = toString(sValue[0]-'j'+1)[0];
            vLine.push_back(vLine[vLine.size()-1]-StrToDb(sValue));
        }
        else if (sValue[0] == '%')
        {
            sValue[0] = '0';
            vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
        }
        else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
        {
            if (sValue[0] == 's')
            {
                for (int j = 0; j < 9; j++)
                    vLine.push_back(vLine[vLine.size()-1]);
            }
            else
            {
                for (int j = 0; j <= sValue[0]-'S'; j++)
                    vLine.push_back(vLine[vLine.size()-1]);
            }
        }
        else
            vLine.push_back(StrToDb(sValue));
        sValue.clear();
    }

    return vLine;
}

void Datafile::evalExcelIndices(const string& _sIndices, int& nLine, int& nCol)
{
    //A1 -> IV65536
    string sIndices = toUpperCase(_sIndices);
    for (unsigned int i = 0; i < sIndices.length(); i++)
    {
        if (!isalpha(sIndices[i]))
        {
            nLine = StrToInt(sIndices.substr(i))-1;
            if (i == 2)
            {
                nCol = (sIndices[0]-'A'+1)*26+sIndices[1]-'A';
            }
            else if (i == 1)
            {
                nCol = sIndices[0]-'A';
            }
            break;
        }
    }
    return;
}

bool Datafile::isNumeric(const string& _sString)
{
    if (!_sString.length())
        return false;
    for (unsigned int i = 0; i < _sString.length(); i++)
    {
        if ((_sString[i] >= '0' && _sString[i] <= '9')
            || _sString[i] == 'e'
            || _sString[i] == 'E'
            || _sString[i] == '-'
            || _sString[i] == '+'
            || _sString[i] == '.'
            || _sString[i] == ','
            || _sString[i] == '\t'
            || _sString[i] == '%'
            || _sString[i] == ' ')
            continue;
        else if (_sString.substr(i, 3) == "nan"
            || _sString.substr(i, 3) == "NaN"
            || _sString.substr(i, 3) == "NAN"
            || _sString.substr(i, 3) == "inf"
            || _sString.substr(i, 3) == "INF"
            )
        {
            i += 2;
            continue;
        }
        else
        {
            //cerr << _sString[i] << endl;
            return false;
        }
    }
    return true;
}

// Implementation for the "Sorter" object
int Datafile::compare(int i, int j, int col)
{
    if (dDatafile[i][col] == dDatafile[j][col])
        return 0;
    else if (dDatafile[i][col] < dDatafile[j][col])
        return -1;

    return 1;
}

// Implementation for the "Sorter" object
bool Datafile::isValue(int line, int col)
{
    return !isnan(dDatafile[line][col]);
}

// --> Setzt nCols <--
void Datafile::setCols(long long int _nCols)
{
	nCols = _nCols;
	return;
}

// --> Setzt nLines <--
void Datafile::setLines(long long int _nLines)
{
	nLines = _nLines;
	return;
}

// --> gibt nCols zurueck <--
long long int Datafile::getCols(const string& sCache, bool _bFull) const
{
	if (sCache != "data")
		return getCacheCols(sCache, _bFull);
    else if (!dDatafile)
        return 0;
	else
		return nCols;
}

// --> gibt nLines zurueck <--
long long int Datafile::getLines(const string& sCache, bool _bFull) const
{
	if (sCache != "data")
		return getCacheLines(sCache, _bFull);
    else if (!dDatafile)
        return 0;
	else
		return nLines;
}

// --> gibt das Element der _nLine-ten Zeile und der _nCol-ten Spalte zurueck <--
double Datafile::getElement(long long int _nLine, long long int _nCol, const string& sCache) const
{
	if (sCache != "data")	// Daten aus dem Cache uebernehmen?
		return readFromCache(_nLine, _nCol, sCache);
    else if (_nLine >= nLines || _nCol >= nCols || _nLine < 0 || _nCol < 0)
        return NAN;
	else if (dDatafile)		// Sonst werden die Daten des Datafiles verwendet
		return dDatafile[_nLine][_nCol];
    else
        return NAN;
}

vector<double> Datafile::getElement(const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& sCache) const
{
    //cerr << _vLine.size() << " " << _vCol.size() << " " << sCache << endl;

    if (sCache != "data")
        return readFromCache(_vLine, _vCol, sCache);
    vector<double> vReturn;

    if ((_vLine.size() > 1 && _vCol.size() > 1) || !dDatafile)
        vReturn.push_back(NAN);
    else
    {
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            for (unsigned int j = 0; j < _vCol.size(); j++)
            {
                //cerr << _vLine[i] << endl;
                if (_vLine[i] >= nLines
                    || _vCol[j] >= nCols
                    || _vCol[j] < 0
                    || _vLine[i] < 0)
                    vReturn.push_back(NAN);
                else
                    vReturn.push_back(dDatafile[_vLine[i]][_vCol[j]]);
            }
        }
    }
    return vReturn;
}

void Datafile::copyElementsInto(vector<double>* vTarget, const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& sCache) const
{
    if (vTarget == nullptr)
        return;
    if (sCache != "data")
    {
        copyCachedElementsInto(vTarget, _vLine, _vCol, sCache);
        return;
    }
    vTarget->clear();
    if ((_vLine.size() > 1 && _vCol.size() > 1) || !dDatafile)
        vTarget->resize(1, NAN);
    else
    {
        vTarget->resize(_vLine.size()*_vCol.size(), NAN);
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            for (unsigned int j = 0; j < _vCol.size(); j++)
            {
                //cerr << _vLine[i] << endl;
                if (_vLine[i] >= nLines
                    || _vCol[j] >= nCols
                    || _vCol[j] < 0
                    || _vLine[i] < 0)
                    (*vTarget)[j+i*_vCol.size()] = NAN;
                else
                    (*vTarget)[j+i*_vCol.size()] = dDatafile[_vLine[i]][_vCol[j]];
            }
        }
    }

}

// --> loescht den Inhalt des Datenfile-Objekts, ohne selbiges zu zerstoeren <--
void Datafile::removeData(bool bAutoSave)
{
	if(bValidData)	// Sind ueberhaupt Daten vorhanden?
	{
		// --> Speicher, wo denn noetig freigeben <--
		for (long long int i = 0; i < nLines; i++)
		{
			delete[] dDatafile[i];
			//delete[] bValidEntry[i];
		}
		delete[] dDatafile;
		//delete[] bValidEntry;
		if (nAppendedZeroes)
		{
			delete[] nAppendedZeroes;
			nAppendedZeroes = 0;
		}
		if (sHeadLine)
		{
			delete[] sHeadLine;
			sHeadLine = 0;
		}

		// --> Variablen zuruecksetzen <--
		nLines = 0;
		nCols = 0;
		dDatafile = 0;
		bValidData = false;
		//bValidEntry = 0;
		sDataFile = "";

	}
	return;
}

// --> gibt den Wert von bValidData zurueck <--
bool Datafile::isValid() const
{
	if(bUseCache)
		return isValidCache();
	else
		return bValidData;
}

// --> gibt den Wert von sDataFile zurueck <--
string Datafile::getDataFileName(const string& sCache) const
{
	if (sCache != "data")
		return sCache;
	else
		return sDataFile;
}

// --> gibt den Wert von sDataFile (ggf. gekuerzt) zurueck <--
string Datafile::getDataFileNameShort() const
{
    string sFileName = getDataFileName("data");
    unsigned int nPos = -1;
    unsigned int nPos_2 = 0;
    while (sFileName.find('\\') != string::npos)
    {
        sFileName[sFileName.find('\\')] = '/';
    }
    while (sFileName.rfind('/', nPos) != string::npos)
    {
        nPos = sFileName.rfind('/', nPos);
        if (nPos != 0 && nPos-1 != ':')
        {
            nPos_2 = sFileName.rfind('/', nPos-1);
            if (nPos_2 != string::npos)
            {
                sFileName = sFileName.substr(0,nPos_2+1) + ".." + sFileName.substr(nPos);
                nPos = nPos_2;
            }
            else
                break;
        }
        else
            break;
    }
    return sFileName;
}

// --> gibt das _i-te Element der gespeicherten Kopfzeile zurueck <--
string Datafile::getHeadLineElement(long long int _i, const string& sCache) const
{
	if (sCache != "data")
	{
		return getCacheHeadLineElement(_i, sCache);
	}
	else if (_i >= nCols)
        return "Spalte " + toString((int)_i+1) + " (leer)";
	else
		return sHeadLine[_i];
}

vector<string> Datafile::getHeadLineElement(vector<long long int> _vCol, const string& sCache) const
{
    if (sCache != "data")
        return Cache::getCacheHeadLineElement(_vCol, sCache);
    vector<string> vHeadlines;
    for (unsigned int i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0 || _vCol[i] >= nLines)
            continue;
        vHeadlines.push_back(sHeadLine[_vCol[i]]);
    }
    return vHeadlines;
}

// --> schreibt _sHead in das _i-te Element der Kopfzeile <--
bool Datafile::setHeadLineElement(long long int _i, const string& sCache, string _sHead)
{
	for (unsigned int i = 0; i < _sHead.length(); i++)
	{
        if (_sHead[i] == (char)142)
            _sHead[i] = 'ƒ';
        else if (_sHead[i] == (char)132)
            _sHead[i] = '‰';
        else if (_sHead[i] == (char)153)
            _sHead[i] = '÷';
        else if (_sHead[i] == (char)148)
            _sHead[i] = 'ˆ';
        else if (_sHead[i] == (char)154)
            _sHead[i] = '‹';
        else if (_sHead[i] == (char)129)
            _sHead[i] = '¸';
        else if (_sHead[i] == (char)225)
            _sHead[i] = 'ﬂ';
        else if (_sHead[i] == ' ')
            _sHead[i] = '_';
        else if (_sHead[i] == (char)248)
            _sHead[i] = '∞';
        else if (_sHead[i] == (char)194)
        {
            _sHead = _sHead.substr(0,i)+_sHead.substr(i+1);
            i--;
        }
        else
            continue;
	}

	// --> Jetzt koennen wir den String verwenden <--
	if (sCache != "data")
	{
		if (!setCacheHeadLineElement(_i, sCache, _sHead))
            return false;
	}
	else if (_i < nCols)
		sHeadLine[_i] = _sHead;
    else
        return false;
	return true;
}

// --> gibt die Zahl der in der _i-ten Spalte angehaengten Nullzeilen zurueck <--
long long int Datafile::getAppendedZeroes(long long int _i, const string& sCache) const
{
	if (sCache != "data")
	{
		return Cache::getAppendedZeroes(_i, sCache);
	}
	else
	{
		if (nAppendedZeroes && _i < nCols)
			return nAppendedZeroes[_i];
		else
			return nLines;
	}
}

// This function counts the headline lines of the whole table
int Datafile::getHeadlineCount(const string& sCache) const
{
    if (sCache != "data")
        return Cache::getHeadlineCount(sCache);

    int nHeadlineCount = 1;
    // Get the dimensions of the complete headline (i.e. including possible linebreaks)
    for (long long int j = 0; j < getCols(sCache); j++)
    {
        // No linebreak? Continue
        if (sHeadLine[j].find("\\n") == string::npos)
            continue;

        int nLinebreak = 0;

        // Count all linebreaks
        for (unsigned int n = 0; n < sHeadLine[j].length() - 2; n++)
        {
            if (sHeadLine[j].substr(n, 2) == "\\n")
                nLinebreak++;
        }

        // Save the maximal number
        if (nLinebreak + 1 > nHeadlineCount)
            nHeadlineCount = nLinebreak + 1;
    }

    return nHeadlineCount;
}

// --> gibt zurueck, ob das Element der _nLine-ten Zeile und der _nCol-ten Spalte ueberhaupt gueltig ist <--
bool Datafile::isValidEntry(long long int _nLine, long long int _nCol, const string& sCache) const
{
	if (sCache != "data")
	{
		return isValidElement(_nLine,_nCol,sCache);
	}
	else if (_nLine < nLines && _nLine >= 0 && _nCol < nCols && _nCol >= 0 && dDatafile)
		return !isnan(dDatafile[_nLine][_nCol]);
	else
		return false;
}

// --> Spannend: Macht aus den Daten zweier Datafile-Objekte eine Tabelle, die im operierenden Objekt gespeichert wird <--
void Datafile::melt(Datafile& _cache)
{
	if(_cache.bValidData) // Ist denn im zweiten Objekt ueberhaupt eine Information vorhanden?
	{
		// --> Temporaere Variablen initialisieren <--
		long long int nOldCols = nCols;
		long long int nOldLines = nLines;
		sDataFile = "Merged Data"; 				// "merged Data" bietet sich einfach an ...
		long long int nNewCols = nCols + _cache.nCols;	// Die neue Zahl der Spalten ist einfach die Summe beider Spalten
		long long int nNewLines;							// Die neue Zahl der Zeilen wird durch das Datafile-Objekt mit den meisten Zeilen bestimmt
		if (nLines >= _cache.nLines)
			nNewLines = nLines;
		else
			nNewLines = _cache.nLines;

		// --> Speicher fuer Kopie anlegen <--
		double** dDataFileOld = new double*[nOldLines];
		//bool** bValidEntryOld = new bool*[nOldLines];
		for (long long int i = 0; i < nOldLines; i++)
		{
			//bValidEntryOld[i] = new bool[nOldCols];
			dDataFileOld[i] = new double[nOldCols];
		}
		long long int* nAppendedZeroesOld = new long long int[nOldCols];
		string* sHeadLineOld = new string[nOldCols];

		// --> Kopieren der bisherigen Daten <--
		for (long long int i = 0; i < nOldLines; i++)
		{
			for (long long int j = 0; j < nOldCols; j++)
			{
				if(i == 0)
				{
					sHeadLineOld[j] = sHeadLine[j];
					if(nAppendedZeroes)	// Waren schon irgendwo Nullzeilen angehaengt? Ebenfalls uebernehmen ...
						nAppendedZeroesOld[j] = nAppendedZeroes[j];
					else
						nAppendedZeroesOld[j] = 0;
				}
				dDataFileOld[i][j] = dDatafile[i][j];
				//bValidEntryOld[i][j] = bValidEntry[i][j];
			}
		}

		// --> Alten Speicher freigeben, sonst kann der nicht neu beschrieben werden! <--
		delete[] sHeadLine;
		sHeadLine = 0;
		if(nAppendedZeroes)
		{
			delete[] nAppendedZeroes;
			nAppendedZeroes = 0;
		}
		for (long long int i = 0; i < nOldLines; i++)
		{
			delete[] dDatafile[i];
			//delete[] bValidEntry[i];
		}
		delete[] dDatafile;
		//delete[] bValidEntry;
		dDatafile = 0;
		//bValidEntry = 0;

		// --> Neuen Speicher mit neuer Groesse allozieren <--
		dDatafile = new double*[nNewLines];
		//bValidEntry = new bool*[nNewLines];
		for (long long int i = 0; i < nNewLines; i++)
		{
			//bValidEntry[i] = new bool[nNewCols];
			dDatafile[i] = new double[nNewCols];
		}
		sHeadLine = new string[nNewCols];
		nAppendedZeroes = new long long int[nNewCols];

		// --> Alte Eintraege wieder einfuegen <--
		for (long long int i = 0; i < nNewLines; i++)
		{
			for (long long int j = 0; j < nOldCols; j++)
			{
				if (i == 0)
				{
					sHeadLine[j] = sHeadLineOld[j];
					nAppendedZeroes[j] = nAppendedZeroesOld[j];
					if (nOldLines < nNewLines)		// Falls Nullzeilen noetig sind: speichern!
						nAppendedZeroes[j] += (nNewLines - nOldLines);
				}
				if (i >= nOldLines)
				{
					dDatafile[i][j] = NAN;			// Nullzeilen, falls noetig
					//bValidEntry[i][j] = false;
					continue;
				}
				else
				{
					dDatafile[i][j] = dDataFileOld[i][j];
					//bValidEntry[i][j] = bValidEntryOld[i][j];
				}

			}
		}

		// --> Neue Eintraege aus dem Objekt _cache hinzufuegen <--
		for (long long int i = 0; i < nNewLines; i++)
		{
			for (long long int j = 0; j < _cache.nCols; j++)
			{
				if (i == 0)
				{
					sHeadLine[j+nOldCols] = _cache.sHeadLine[j];
					if (_cache.nAppendedZeroes)
						nAppendedZeroes[j+nOldCols] = _cache.nAppendedZeroes[j];
					else
						nAppendedZeroes[j+nOldCols] = 0;
					if (_cache.nLines < nNewLines)		// Ebenfalls die Nullzeilen merken, falls denn welche noetig sind
						nAppendedZeroes[j+nOldCols] += (nNewLines - _cache.nLines);
				}
				if (i >= _cache.nLines)
				{
					dDatafile[i][j+nOldCols] = NAN;		// Nullzeilen, falls noetig
					//bValidEntry[i][j+nOldCols] = false;
					continue;
				}
				else
				{
					//bValidEntry[i][j+nOldCols] = _cache.bValidEntry[i][j];
					dDatafile[i][j+nOldCols] = _cache.dDatafile[i][j];
				}
			}
		}

		// --> Zeilen- und Spaltenzahlen in die eigentlichen Variablen uebernehmen <--
		nLines = nNewLines;
		nCols = nNewCols;

		// --> temporaeren Speicher wieder freigeben <--
		delete[] sHeadLineOld;
		delete[] nAppendedZeroesOld;
		for (long long int i = 0; i < nOldLines; i++)
		{
			delete[] dDataFileOld[i];
			//delete[] bValidEntryOld[i];
		}
		delete[] dDataFileOld;
		//delete[] bValidEntryOld;

	}
	return;
}

void Datafile::setCacheStatus(bool _bCache)
{
	bUseCache = _bCache;
	return;
}

bool Datafile::getCacheStatus() const
{
	return bUseCache;
}

bool Datafile::isValidCache() const
{
	return Cache::isValid();
}

void Datafile::clearCache()
{
	if (isValidCache())
		removeCachedData();
	return;
}

bool Datafile::setCacheSize(long long int _nLines, long long int _nCols, const string& _sCache)
{
	if (!resizeCache(_nLines, _nCols, _sCache))
        return false;
	return true;
}

void Datafile::openAutosave(string _sFile, Settings& _option)
{
    openFile(_sFile, _option, true);
    resizeCache(nLines, nCols, "cache");
    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            if (isValidEntry(i, j, "data"))
                writeToCache(i, j, "cache", dDatafile[i][j]);
        }
    }
    for (long long int i = 0; i < nCols; i++)
    {
        setCacheHeadLineElement(i, 0, sHeadLine[i]);
    }
    removeData(true);
    return;
}

vector<int> Datafile::sortElements(const string& sLine) // data -sort[[=desc]] cols=1[2:3]4[5:9]10:
{
    if (!dDatafile && sLine.find("data") != string::npos)
        return vector<int>();
    else if (sLine.find("cache") != string::npos || sLine.find("data") == string::npos)
        return Cache::sortElements(sLine);


    string sCache;
    string sSortingExpression = "-set";

    if (findCommand(sLine).sString != "sort")
    {
        sCache = findCommand(sLine).sString;
    }
    if (matchParams(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, matchParams(sLine, "sort", '=')+4) == "desc")
            sSortingExpression += " desc";
    }
    else if (matchParams(sLine, "data", '='))
    {
        if (getArgAtPos(sLine, matchParams(sLine, "data", '=')+4) == "desc")
            sSortingExpression += " desc";
    }

    if (matchParams(sLine, "cols", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, matchParams(sLine, "cols", '=')+4);
    else if (matchParams(sLine, "c", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, matchParams(sLine, "c", '=')+1);
    if (matchParams(sLine, "index"))
        sSortingExpression += " index";

    return sortElements(sCache, 0, nLines - 1, 0, nCols - 1, sSortingExpression);
}

vector<int> Datafile::sortElements(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    if (!dDatafile && sCache == "data")
        return vector<int>();
    else if (sCache != "data")
        return Cache::sortElements(sCache, i1, i2, j1, j2, sSortingExpression);

    bool bError = false;
    bool bReturnIndex = false;
    int nSign = 1;
    vector<int> vIndex;

    if (matchParams(sSortingExpression, "desc"))
        nSign = -1;

    if (!getCols("data", false))
        return vIndex;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;



    for (int i = i1; i <= i2; i++)
        vIndex.push_back(i);

    if (matchParams(sSortingExpression, "index"))
        bReturnIndex = true;

    if (!matchParams(sSortingExpression, "cols", '=') && !matchParams(sSortingExpression, "c", '='))
    {
        for (int i = j1; i <= j2; i++)
        {
            if (!qSort(&vIndex[0], i2-i1+1, i, 0, i2-i1, nSign))
            {
                throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, sCache + " " + sSortingExpression, SyntaxError::invalid_position);
            }
            if (bReturnIndex)
            {
                break;
            }
            reorderColumn(vIndex, i1, i2, i);
            for (int j = i1; j <= i2; j++)
                vIndex[j] = j;
        }
    }
    else
    {
        string sCols = "";
        if (matchParams(sSortingExpression, "cols", '='))
        {
            sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "cols", '=')+4);
        }
        else
        {
            sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "c", '=')+1);
        }

        while (sCols.length())
        {
            ColumnKeys* keys = evaluateKeyList(sCols, j2-j1+1);
            if (!keys)
                throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, sCache + " " + sSortingExpression, SyntaxError::invalid_position);

            if (keys->nKey[1] == -1)
                keys->nKey[1] = keys->nKey[0]+1;

            for (int j = keys->nKey[0]; j < keys->nKey[1]; j++)
            {
                if (!qSort(&vIndex[0], i2-i1+1, j+j1, 0, i2-i1, nSign))
                {
                    throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, sCache + " " + sSortingExpression, SyntaxError::invalid_position);
                }
                // Subkey list
                if (keys->subkeys && keys->subkeys->subkeys)
                {
                   if (!sortSubList(&vIndex[0], i2-i1+1, keys, i1, i2, j1, nSign, nCols))
                    {
                        delete keys;
                        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sCache + " " + sSortingExpression, SyntaxError::invalid_position);
                    }
                }
                if (bReturnIndex)
                    break;
                reorderColumn(vIndex, i1, i2, j+j1);

                ColumnKeys* subKeyList = keys->subkeys;
                while (subKeyList)
                {
                    if (subKeyList->nKey[1] == -1)
                        subKeyList->nKey[1] = subKeyList->nKey[0]+1;
                    for (int j = subKeyList->nKey[0]; j < subKeyList->nKey[1]; j++)
                    {
                        reorderColumn(vIndex, i1, i2, j+j1);
                    }
                    subKeyList = subKeyList->subkeys;
                }

                for (int j = i1; j <= i2; j++)
                    vIndex[j] = j;
            }

            delete keys;
            if (bReturnIndex)
                break;
        }
    }

    if (bReturnIndex)
    {
        for (int i = 0; i <= i2-i1; i++)
            vIndex[i]++;
    }

    countAppendedZeroes();


    if (bError || !bReturnIndex)
        return vector<int>();
    return vIndex;
}

bool Datafile::saveFile(const string& sCache, string _sFileName)
{
    if (!_sFileName.length())
        generateFileName();
    else
    {
        string sTemp = sPath;
        setPath(sSavePath, false, sWhere);
        if (_sFileName.find('.') != string::npos && _sFileName.substr(_sFileName.rfind('.')) != ".ndat")
            _sFileName = _sFileName.substr(0,_sFileName.rfind('.'));
        sOutputFile = FileSystem::ValidFileName(_sFileName, ".ndat");
        setPath(sTemp, false, sWhere);
    }
    if (sCache != "data")
        return Cache::saveLayer(sOutputFile, sCache);
    if (getCacheStatus())
        return Cache::saveLayer(sOutputFile, "cache");
    if (file_out.is_open())
        file_out.close();
    file_out.open(sOutputFile.c_str(), ios_base::binary | ios_base::trunc | ios_base::out);

    if (file_out.is_open() && file_out.good() && bValidData)
    {
        char** cHeadLine = new char*[getCols("data")];
        long int nMajor = AutoVersion::MAJOR;
        long int nMinor = AutoVersion::MINOR;
        long int nBuild = AutoVersion::BUILD;
        long long int lines = 0;
        long long int cols = 0;

        cols = Datafile::nCols;
        lines = Datafile::nLines;
        for (long long int i = 0; i < cols; i++)
        {
            cHeadLine[i] = new char[getHeadLineElement(i, "data").length()+1];
            for (unsigned int j = 0; j < getHeadLineElement(i, "data").length(); j++)
            {
                cHeadLine[i][j] = getHeadLineElement(i, "data")[j];
            }
            cHeadLine[i][getHeadLineElement(i, "data").length()] = '\0';
        }

        time_t tTime = time(0);
        file_out.write((char*)&nMajor, sizeof(long));
        file_out.write((char*)&nMinor, sizeof(long));
        file_out.write((char*)&nBuild, sizeof(long));
        file_out.write((char*)&tTime, sizeof(time_t));
        file_out.write((char*)&lines, sizeof(long long int));
        file_out.write((char*)&cols, sizeof(long long int));
        //cerr << lines << " " << cols << endl;
        for (long long int i = 0; i < cols; i++)
        {
            size_t nlength = getHeadLineElement(i, "data").length()+1;
            //cerr << nlength << endl;
            file_out.write((char*)&nlength, sizeof(size_t));
            file_out.write(cHeadLine[i], sizeof(char)*getHeadLineElement(i, "data").length()+1);
        }
        long long int* appendedzeroes = 0;
        appendedzeroes = Datafile::nAppendedZeroes;
        file_out.write((char*)appendedzeroes, sizeof(long long int)*cols);

        double** data = 0;
        bool* validelement = new bool[cols];

        data = Datafile::dDatafile;
        //validelement = Datafile::bValidEntry;
        for (long long int i = 0; i < lines; i++)
        {
            file_out.write((char*)data[i], sizeof(double)*cols);
        }
        for (long long int i = 0; i < lines; i++)
        {
            for (long long int j = 0; j < cols; j++)
            {
                validelement[j] = !isnan(data[i][j]);
            }
            file_out.write((char*)validelement, sizeof(bool)*cols);
        }
        file_out.close();
        for (long long int i = 0; i < cols; i++)
        {
            delete[] cHeadLine[i];
        }
        delete[] cHeadLine;
        if (validelement)
            delete[] validelement;

        return true;
    }
    else
    {
        file_out.close();
        return false;
    }

}

void Datafile::generateFileName()
{
	string sTime;
	if (sSavePath.find('"') != string::npos)
        sTime = sSavePath.substr(1,sSavePath.length()-2);
    else
        sTime = sSavePath;
    while (sTime.find('\\') != string::npos)
        sTime[sTime.find('\\')] = '/';
    sTime += "/" + sPrefix + "_";		// Prefix laden
	sTime += getDate();		// Datum aus der Klasse laden
	sTime += ".ndat";
	sOutputFile = sTime;			// Dateinamen an sFileName zuweisen
	return;
}

string Datafile::getDate()	// Der Boolean entscheidet, ob ein Dateinamen-Datum oder ein "Kommentar-Datum" gewuenscht ist
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
		Temp_str << "_";		// Unterstrich im Dateinamen
	if(ltm->tm_hour < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_hour; 	// hh
	if(ltm->tm_min < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_min;	// mm
	if(ltm->tm_sec < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_sec;	// ss
	return Temp_str.str();
}

void Datafile::openFromCmdLine(Settings& _option, string _sFile, bool bOpen)
{
    if (_sFile.length())
    {
        sDataFile = FileSystem::ValidFileName(_sFile, ".ndat");
        if (!bOpen)
            bPauseOpening = true;
    }
    if (bOpen && sDataFile.length() && bPauseOpening)
    {
        Datafile::openFile(sDataFile, _option, false, true);
        bPauseOpening = false;
    }
    return;
}


double Datafile::std(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::std(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dMean = 0.0;
    double dStd = 0.0;
    long long int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;

    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << endl;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
            {
                nInvalid++;
                continue;
            }
            dMean += dDatafile[i][j];
        }
    }
    dMean /= (double)((i2-i1+1)*(j2-j1+1)-nInvalid);
    //cerr << dMean << endl;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            dStd += (dMean - dDatafile[i][j]) * (dMean - dDatafile[i][j]);
        }
    }
    dStd /= (double)((i2-i1+1)*(j2-j1+1)-1-nInvalid);
    dStd = sqrt(dStd);
    return dStd;
}

double Datafile::std(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::std(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dAvg = Datafile::avg(sCache, _vLine, _vCol);
    double dStd = 0.0;
    unsigned int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                nInvalid++;
            else if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                nInvalid++;
            else
                dStd += (dAvg - dDatafile[_vLine[i]][_vCol[j]])*(dAvg - dDatafile[_vLine[i]][_vCol[j]]);
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size()-1)
        return NAN;
    return sqrt(dStd / ((_vLine.size()*_vCol.size())-1-nInvalid));
}

double Datafile::avg(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::avg(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dMean = 0.0;
    long long int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
            {
                nInvalid++;
                continue;
            }
            dMean += dDatafile[i][j];
        }
    }
    dMean /= (double)((i2-i1+1)*(j2-j1+1) - nInvalid);
    return dMean;
}

double Datafile::avg(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::avg(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dAvg = 0.0;
    unsigned int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                nInvalid++;
            else if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                nInvalid++;
            else
                dAvg += dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    return dAvg/(_vLine.size()*_vCol.size()-nInvalid);
}

double Datafile::max(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::max(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dMax = 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            if (i == i1 && j == j1)
                dMax = dDatafile[i][j];
            else if (dDatafile[i][j] > dMax)
                dMax = dDatafile[i][j];
            else
                continue;
        }
    }
    return dMax;
}

double Datafile::max(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::max(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dMax = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            if (isnan(dMax))
                dMax = dDatafile[_vLine[i]][_vCol[j]];
            if (dMax < dDatafile[_vLine[i]][_vCol[j]])
                dMax = dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    return dMax;
}

double Datafile::min(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::min(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dMin = 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            if (i == i1 && j == j1)
                dMin = dDatafile[i][j];
            else if (dDatafile[i][j] < dMin)
                dMin = dDatafile[i][j];
            else
                continue;
        }
    }
    return dMin;
}

double Datafile::min(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::min(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dMin = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            if (isnan(dMin))
                dMin = dDatafile[_vLine[i]][_vCol[j]];
            if (dMin > dDatafile[_vLine[i]][_vCol[j]])
                dMin = dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    return dMin;
}

double Datafile::prd(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::prd(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dPrd = 1.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            dPrd *= dDatafile[i][j];
        }
    }
    return dPrd;

}

double Datafile::prd(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::prd(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dPrd = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            dPrd *= dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    return dPrd;
}

double Datafile::sum(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::sum(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dSum = 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            dSum += dDatafile[i][j];
        }
    }
    return dSum;
}

double Datafile::sum(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::sum(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dSum = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            dSum += dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    return dSum;
}

double Datafile::num(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::num(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    //double dNum = 0.0;
    int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                nInvalid++;
            else if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                nInvalid++;
        }
    }
    return (_vLine.size()*_vCol.size())-nInvalid;
}

double Datafile::num(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::num(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                nInvalid++;
        }
    }
    return (double)((i2-i1+1)*(j2-j1+1)-nInvalid);
}

double Datafile::and_func(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::and_func(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    double dRetVal = NAN;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dRetVal))
                dRetVal = 1.0;
            if (isnan(dDatafile[i][j]) || dDatafile[i][j] == 0)
                return 0.0;
        }
    }
    if (isnan(dRetVal))
        return 0.0;
    return 1.0;
}

double Datafile::and_func(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::and_func(sCache, _vLine, _vCol);
    if (!bValidData)
        return 0.0;

    double dRetVal = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dRetVal))
                dRetVal = 1.0;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]) || dDatafile[_vLine[i]][_vCol[j]] == 0)
                return 0.0;
        }
    }
    if (isnan(dRetVal))
        return 0.0;
    return 1.0;
}

double Datafile::or_func(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::or_func(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (!isnan(dDatafile[i][j]) && dDatafile[i][j] != 0.0)
                return 1.0;
        }
    }
    return 0.0;
}

double Datafile::or_func(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::or_func(sCache, _vLine, _vCol);
    if (!bValidData)
        return 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]) || dDatafile[_vLine[i]][_vCol[j]] != 0)
                return 1.0;
        }
    }
    return 0.0;
}

double Datafile::xor_func(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::xor_func(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    bool isTrue = false;
    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (!isnan(dDatafile[i][j]) && dDatafile[i][j] != 0.0)
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return 0.0;
            }
        }
    }
    if (isTrue)
        return 1.0;
    return 0.0;
}

double Datafile::xor_func(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::xor_func(sCache, _vLine, _vCol);
    if (!bValidData)
        return 0.0;

    bool isTrue = false;
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]) || dDatafile[_vLine[i]][_vCol[j]] != 0)
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return 0.0;
            }
        }
    }
    if (isTrue)
        return 1.0;
    return 0.0;
}


double Datafile::cnt(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::cnt(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    if (j2 >= nCols)
        j2 = nCols-1;
    if (i2 >= nLines-getAppendedZeroes(j1, "data"))
        i2 = nLines-1-getAppendedZeroes(j1, "data");

    return (double)((i2-i1+1)*(j2-j1+1));
}

double Datafile::cnt(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::cnt(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0
                || _vLine[i] >= getLines(sCache, false)
                || _vLine[i] >= getLines(sCache, false) - getAppendedZeroes(_vCol[j], sCache)
                || _vCol[j] < 0
                || _vCol[j] >= getCols(sCache))
                nInvalid++;
        }
    }
    return (_vLine.size()*_vCol.size())-nInvalid;
}


double Datafile::norm(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::norm(sCache, i1,i2,j1,j2);
    if (!bValidData)
        return NAN;
    double dNorm = 0.0;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            dNorm += dDatafile[i][j]*dDatafile[i][j];
        }
    }
    return sqrt(dNorm);
}

double Datafile::norm(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::norm(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dNorm = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            dNorm += dDatafile[_vLine[i]][_vCol[j]]*dDatafile[_vLine[i]][_vCol[j]];
        }
    }
    return sqrt(dNorm);
}

double Datafile::cmp(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, double dRef, int nType)
{
    //cerr << sCache << i1 << " " << i2 << " " << j1 << " " << j2 << " " << dRef << " " << nType << endl;
    if (sCache != "data")
        return Cache::cmp(sCache, i1, i2, j1, j2, dRef, nType);
    if (!bValidData)
        return NAN;
    double dKeep = 0.0;
    int nKeep = -1;

    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dDatafile[i][j]))
                continue;
            if (dDatafile[i][j] == dRef)
            {
                if (abs(nType) <= 1)
                {
                    if (i1 == i2)
                        return j+1;
                    else
                        return i+1;
                }
                else
                    return dDatafile[i][j];
            }
            else if ((nType == 1 || nType == 2) && dDatafile[i][j] > dRef)
            {
                if (nKeep == -1 || dDatafile[i][j] < dKeep)
                {
                    dKeep = dDatafile[i][j];
                    if (i1 == i2)
                        nKeep = j;
                    else
                        nKeep = i;
                }
                else
                    continue;
            }
            else if ((nType == -1 || nType == -2) && dDatafile[i][j] < dRef)
            {
                if (nKeep == -1 || dDatafile[i][j] > dKeep)
                {
                    dKeep = dDatafile[i][j];
                    if (i1 == i2)
                        nKeep = j;
                    else
                        nKeep = i;
                }
                else
                    continue;
            }
        }
    }
    if (nKeep == -1)
        return NAN;
    else if (abs(nType) == 2)
        return dKeep;
    else
        return nKeep+1;
}

double Datafile::cmp(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef, int nType)
{
    if (sCache != "data")
        return Cache::cmp(sCache, _vLine, _vCol, dRef, nType);
    if (!bValidData)
        return NAN;
    double dKeep = 0.0;
    int nKeep = -1;

    for (long long int i = 0; i < _vLine.size(); i++)
    {
        for (long long int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                continue;
            if (dDatafile[_vLine[i]][_vCol[j]] == dRef)
            {
                if (abs(nType) <= 1)
                {
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        return _vCol[j]+1;
                    else
                        return _vLine[i]+1;
                }
                else
                    return dDatafile[_vLine[i]][_vCol[j]];
            }
            else if ((nType == 1 || nType == 2) && dDatafile[_vLine[i]][_vCol[j]] > dRef)
            {
                if (nKeep == -1 || dDatafile[_vLine[i]][_vCol[j]] < dKeep)
                {
                    dKeep = dDatafile[_vLine[i]][_vCol[j]];
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
            else if ((nType == -1 || nType == -2) && dDatafile[_vLine[i]][_vCol[j]] < dRef)
            {
                if (nKeep == -1 || dDatafile[_vLine[i]][_vCol[j]] > dKeep)
                {
                    dKeep = dDatafile[_vLine[i]][_vCol[j]];
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
        }
    }
    if (nKeep == -1)
        return NAN;
    else if (abs(nType) == 2)
        return dKeep;
    else
        return nKeep+1;
}

double Datafile::med(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return Cache::med(sCache, i1, i2, j1, j2);
    if (!bValidData)
        return NAN;
    Datafile _cache;
    _cache.setCacheStatus(true);
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (i1 != i2 && j1 != j2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dDatafile[i][j]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache(i-i1,j-j1,"cache",dDatafile[i][j]);
            }
            else
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache(j-j1,i-i1,"cache",dDatafile[i][j]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    if (_cache.getCacheLines("cache", false) % 2)
    {
        return _cache.getElement(_cache.getCacheLines("cache", false) / 2, 0, "cache");
    }
    else
    {
        return (_cache.getElement(_cache.getCacheLines("cache", false) / 2, 0, "cache") + _cache.getElement(_cache.getCacheLines("cache", false) / 2 - 1, 0, "cache")) / 2.0;
    }
}

double Datafile::med(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (sCache != "data")
        return Cache::med(sCache, _vLine, _vCol);
    if (!bValidData)
        return NAN;
    double dMed = 0.0;
    unsigned int nInvalid = 0;
    unsigned int nCount = 0;
    double* dData = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                nInvalid++;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                nInvalid++;
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    dData = new double[(_vLine.size()*_vCol.size())-nInvalid];
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            dData[nCount] = dDatafile[_vLine[i]][_vCol[j]];
            nCount++;
            if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
                break;
        }
        if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
            break;
    }

    nCount = qSortDouble(dData, nCount);
    //gsl_sort(dData, 1, nCount);
    if (!nCount)
    {
        delete[] dData;
        return NAN;
    }
    dMed = gsl_stats_median_from_sorted_data(dData, 1, nCount);

    delete[] dData;

    return dMed;
}

double Datafile::pct(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, double dPct)
{
    if (sCache != "data")
        return Cache::pct(sCache, i1, i2, j1, j2, dPct);
    if (!bValidData)
        return NAN;
    if (dPct <= 0 || dPct >= 1)
        return NAN;
    Datafile _cache;
    _cache.setCacheStatus(true);
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;


    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getLines("data", false) || j1 >= getCols("data"))
        return NAN;
    if (i2 >= getLines("data", false))
        i2 = getLines("data", false)-1;
    if (j2 >= getCols("data"))
        j2 = getCols("data")-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (i1 != i2 && j1 != j2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dDatafile[i][j]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache(i-i1,j-j1,"cache",dDatafile[i][j]);
            }
            else
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToCache(j-j1,i-i1,"cache",dDatafile[i][j]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    return (1-((_cache.getCacheLines(0,false)-1)*dPct-floor((_cache.getCacheLines(0,false)-1)*dPct)))*_cache.getElement(floor((_cache.getCacheLines(0,false)-1)*dPct),0,"cache")
        + ((_cache.getCacheLines(0,false)-1)*dPct-floor((_cache.getCacheLines(0,false)-1)*dPct))*_cache.getElement(floor((_cache.getCacheLines(0,false)-1)*dPct)+1,0,"cache");
}

double Datafile::pct(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct)
{
    if (sCache != "data")
        return Cache::pct(sCache, _vLine, _vCol, dPct);
    if (!bValidData)
        return NAN;
    if (dPct <= 0 || dPct >= 1)
        return NAN;
    unsigned int nInvalid = 0;
    unsigned int nCount = 0;
    double* dData = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                nInvalid++;
            if (isnan(dDatafile[_vLine[i]][_vCol[j]]))// || !bValidEntry[_vLine[i]][_vCol[j]])
                nInvalid++;
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    dData = new double[(_vLine.size()*_vCol.size())-nInvalid];
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(sCache, false) || _vCol[j] < 0 || _vCol[j] >= getCols(sCache))
                continue;
            dData[nCount] = dDatafile[_vLine[i]][_vCol[j]];
            nCount++;
            if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
                break;
        }
        if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
            break;
    }

    nCount = qSortDouble(dData, nCount);
    //gsl_sort(dData, 1, nCount);
    if (!nCount)
    {
        delete[] dData;
        return NAN;
    }
    dPct = gsl_stats_quantile_from_sorted_data(dData, 1, nCount, dPct);

    delete[] dData;

    return dPct;
}





