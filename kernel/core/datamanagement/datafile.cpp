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
Datafile::Datafile() : MemoryManager()
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

	tableColumnsCount = 0.0;
	tableLinesCount = 0.0;
}

// --> Allgemeiner Konstruktor <--
Datafile::Datafile(long long int _nLines, long long int _nCols) : MemoryManager()
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
	sHeadLine = new string[nCols];

	for (long long int i = 0; i < nLines; i++)
	{
		dDatafile[i] = new double[nCols];

		for (long long int j = 0; j < nCols; j++)
		{
			sHeadLine[j] = "";
			dDatafile[i][j] = 0.0;
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
		}

		nAppendedZeroes = new long long int[nCols];

		for (long long int i = 0; i < nCols; i++)
		{
			nAppendedZeroes[i] = nLines;
		}

		dDatafile = new double*[nLines];

		for (long long int i = 0; i < nLines; i++)
		{
			dDatafile[i] = new double[nCols];

			for (long long int j = 0; j < nCols; j++)
			{
				dDatafile[i][j] = NAN;
			}
		}
	}
}

// This function creates the table column headers,
// if the headers are empty
void Datafile::createTableHeaders()
{
    for (long long int j = 0; j < nCols; j++)
    {
        if (!sHeadLine[j].length())
            sHeadLine[j] = _lang.get("COMMON_COL") + "_" + toString(j+1);
    }
}

// This member function determines the number of appended
// zeroes in the current data set, i.e. the number of
// invalid values, which are appended at the end of the
// columns
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

// This function opens the desired file and stores its
// contents in the internal storage
NumeRe::FileHeaderInfo Datafile::openFile(string _sFile, Settings& _option, bool bAutoSave, bool bIgnore, int _nHeadline)
{
    NumeRe::FileHeaderInfo info;

    if (!bValidData)
    {
        // Ensure that the file name is valid
        sDataFile = FileSystem::ValidFileName(_sFile);

        // If the file seems not to exist and the user did
        // not provide the extension, try to detect it using
        // wildcard
        if (!fileExists(sDataFile) && (_sFile.find('.') == string::npos || _sFile.find('.') < _sFile.rfind('/')))
            sDataFile = FileSystem::ValidFileName(_sFile+".*");

        // Get an instance of the desired file type
        NumeRe::GenericFile<double>* file = NumeRe::getFileByType(sDataFile);

        // Ensure that the instance is valid
        if (!file)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, _sFile, SyntaxError::invalid_position, _sFile);

        // Try to read the contents of the file. This may
        // either result in a read error or the read method
        // is not defined for this function
        try
        {
            // Igor binary waves might contain three-dimensional
            // waves. We select the roll-out mode in this case
            if (file->getExtension() == "ibw" && _nHeadline == -1)
                static_cast<NumeRe::IgorBinaryWave*>(file)->useXZSlicing();

            // Read the file
            if (!file->read())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sDataFile, SyntaxError::invalid_position, sDataFile);
        }
        catch (...)
        {
            delete file;
            throw;
        }

        // Set the dimensions obtained from the file
        nLines = file->getRows();
        nCols = file->getCols();

        // Get the header informaion structure
        info = file->getFileHeaderInformation();

        // Create the internal memory
        Allocate();

        // If the dimensions were valid and the internal
        // memory was created, copy the data to this
        // memory
        if (dDatafile && sHeadLine)
        {
            // Copy them and delete the file instance
            // afterwards
            file->getData(dDatafile);
            file->getColumnHeadings(sHeadLine);
            delete file;

            // Declare the data as valid and try to
            // shrink it to save memory space
            bValidData = true;
            condenseDataSet();
            countAppendedZeroes();

            createTableHeaders();
        }
        else
        {
            delete file;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, _sFile, SyntaxError::invalid_position, _sFile);
        }

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
			return openFile(_sFile, _option, bAutoSave, bIgnore);	// Jetzt die Methode erneut aufrufen
		}
		else
		{
			NumeReKernel::print(toSystemCodePage(_lang.get("COMMON_CANCEL")));
		}

	}

	return info;
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
                    }
                }
                else if (i < nSkip && nSkip)
                {
                    if (sLine.length())
                    {
                        if (!i || sHeadLine[j] == _lang.get("COMMON_COL") + "_" + toString(j+1))
                            sHeadLine[j] = sLine;
                        else
                            sHeadLine[j] += "\\n" + sLine;
                    }
                }
                else
                {
                    dDatafile[i-nSkip][j] = NAN;
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
            break;

        if (i+1 == nCols)
            return;
    }

    long long int nColTemp = nCols;
    long long int nLinesTemp = nLines;
    long long int nEmptyCols = 0;
    long long int nSkip = 0;


    for (long long int i = 0; i < nColTemp; i++)
    {
        if (nAppendedZeroes[i] == nLinesTemp)
            nEmptyCols++;
    }

    if (nEmptyCols != nColTemp)
    {
        double** dDataTemp = new double*[nLines];

        for (long long int i = 0; i < nLines; i++)
            dDataTemp[i] = new double[nCols];

        string* sHeadTemp = new string[nCols];
        string sDataTemp = sDataFile;
        long long int* nAppendedTemp = new long long int[nCols];


        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                dDataTemp[i][j] = dDatafile[i][j];

                if (!i)
                {
                    nAppendedTemp[j] = nAppendedZeroes[j];
                    sHeadTemp[j] = sHeadLine[j];
                }
            }
        }

        removeData(false);

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

                if (!i)
                {
                    sHeadLine[j-nSkip] = sHeadTemp[j];
                    nAppendedZeroes[j-nSkip] = nAppendedTemp[j];
                }
            }
        }

        sDataFile = sDataTemp;

        delete[] sHeadTemp;
        delete[] nAppendedTemp;

        for (long long int i = 0; i < nLines; i++)
            delete[] dDataTemp[i];

        delete[] dDataTemp;
    }

    bValidData = true;
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
		return getTableCols(sCache, _bFull);
    else if (!dDatafile)
        return 0;
	else
		return nCols;
}

// --> gibt nLines zurueck <--
long long int Datafile::getLines(const string& sCache, bool _bFull) const
{
	if (sCache != "data")
		return getTableLines(sCache, _bFull);
    else if (!dDatafile)
        return 0;
	else
		return nLines;
}

// --> gibt das Element der _nLine-ten Zeile und der _nCol-ten Spalte zurueck <--
double Datafile::getElement(long long int _nLine, long long int _nCol, const string& sCache) const
{
	if (sCache != "data")	// Daten aus dem Cache uebernehmen?
		return readFromTable(_nLine, _nCol, sCache);
    else if (_nLine >= nLines || _nCol >= nCols || _nLine < 0 || _nCol < 0)
        return NAN;
	else if (dDatafile)		// Sonst werden die Daten des Datafiles verwendet
		return dDatafile[_nLine][_nCol];
    else
        return NAN;
}

vector<double> Datafile::getElement(const VectorIndex& _vLine, const VectorIndex& _vCol, const string& sCache) const
{
    //cerr << _vLine.size() << " " << _vCol.size() << " " << sCache << endl;

    if (sCache != "data")
        return readFromTable(_vLine, _vCol, sCache);
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

void Datafile::copyElementsInto(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol, const string& sCache) const
{
    if (vTarget == nullptr)
        return;
    if (sCache != "data")
    {
        copyTableElementsToVector(vTarget, _vLine, _vCol, sCache);
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
		return getTableHeadlineElement(_i, sCache);
	}
	else if (_i >= nCols)
        return _lang.get("COMMON_COL") + " " + toString((int)_i+1) + " (leer)";
	else
		return sHeadLine[_i];
}

vector<string> Datafile::getHeadLineElement(const VectorIndex& _vCol, const string& sCache) const
{
    if (sCache != "data")
        return MemoryManager::getTableHeadlineElement(_vCol, sCache);
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
            _sHead[i] = 'Ä';
        else if (_sHead[i] == (char)132)
            _sHead[i] = 'ä';
        else if (_sHead[i] == (char)153)
            _sHead[i] = 'Ö';
        else if (_sHead[i] == (char)148)
            _sHead[i] = 'ö';
        else if (_sHead[i] == (char)154)
            _sHead[i] = 'Ü';
        else if (_sHead[i] == (char)129)
            _sHead[i] = 'ü';
        else if (_sHead[i] == (char)225)
            _sHead[i] = 'ß';
        else if (_sHead[i] == ' ')
            _sHead[i] = '_';
        else if (_sHead[i] == (char)248)
            _sHead[i] = '°';
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
		if (!setTableHeadlineElement(_i, sCache, _sHead))
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
		return MemoryManager::getAppendedZeroes(_i, sCache);
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
        return MemoryManager::getHeadlineCount(sCache);

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
	return MemoryManager::isValid();
}

void Datafile::clearCache()
{
	if (isValidCache())
		removeDataInMemory();
	return;
}

bool Datafile::setCacheSize(long long int _nLines, long long int _nCols, const string& _sCache)
{
	if (!resizeTables(_nLines, _nCols, _sCache))
        return false;
	return true;
}

void Datafile::openAutosave(string _sFile, Settings& _option)
{
    openFile(_sFile, _option, true);
    resizeTables(nLines, nCols, "cache");
    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            if (isValidEntry(i, j, "data"))
                writeToTable(i, j, "cache", dDatafile[i][j]);
        }
    }
    for (long long int i = 0; i < nCols; i++)
    {
        setTableHeadlineElement(i, 0, sHeadLine[i]);
    }
    removeData(true);
    return;
}

vector<int> Datafile::sortElements(const string& sLine) // data -sort[[=desc]] cols=1[2:3]4[5:9]10:
{
    if (!dDatafile && sLine.find("data") != string::npos)
        return vector<int>();
    else if (sLine.find("cache") != string::npos || sLine.find("data") == string::npos)
        return MemoryManager::sortElements(sLine);


    string sCache;
    string sSortingExpression = "-set";

    if (findCommand(sLine).sString != "sort")
    {
        sCache = findCommand(sLine).sString;
    }
    if (findParameter(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, findParameter(sLine, "sort", '=')+4) == "desc")
            sSortingExpression += " desc";
    }
    else if (findParameter(sLine, "data", '='))
    {
        if (getArgAtPos(sLine, findParameter(sLine, "data", '=')+4) == "desc")
            sSortingExpression += " desc";
    }

    if (findParameter(sLine, "cols", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, findParameter(sLine, "cols", '=')+4);
    else if (findParameter(sLine, "c", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, findParameter(sLine, "c", '=')+1);
    if (findParameter(sLine, "index"))
        sSortingExpression += " index";

    return sortElements(sCache, 0, nLines - 1, 0, nCols - 1, sSortingExpression);
}

vector<int> Datafile::sortElements(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    if (!dDatafile && sCache == "data")
        return vector<int>();
    else if (sCache != "data")
        return MemoryManager::sortElements(sCache, i1, i2, j1, j2, sSortingExpression);

    bool bError = false;
    bool bReturnIndex = false;
    int nSign = 1;
    vector<int> vIndex;

    if (findParameter(sSortingExpression, "desc"))
        nSign = -1;

    if (!getCols("data", false))
        return vIndex;
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;



    for (int i = i1; i <= i2; i++)
        vIndex.push_back(i);

    if (findParameter(sSortingExpression, "index"))
        bReturnIndex = true;

    if (!findParameter(sSortingExpression, "cols", '=') && !findParameter(sSortingExpression, "c", '='))
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
        if (findParameter(sSortingExpression, "cols", '='))
        {
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "cols", '=')+4);
        }
        else
        {
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "c", '=')+1);
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

// This member function updates the special dimension variables
// (table end constants) for the selected table name or the string
// table. It has to be called directly before the corresponding indices
// are evaluated.
bool Datafile::updateDimensionVariables(const string& sTableName)
{
    // Determine the type of table
    if (sTableName != "string")
    {
        // Update the dimensions for the selected
        // numerical table
        tableLinesCount = getLines(sTableName, false);
        tableColumnsCount = getCols(sTableName, false);
    }
    else
    {
        // Update the dimensions for the selected
        // string table
        tableLinesCount = getStringElements();
        tableColumnsCount = getStringCols();
    }

    return true;
}

// This member function is a wrapper for the tables and
// the clusters detector functions
bool Datafile::containsTablesOrClusters(const string& sCmdLine)
{
    return containsTables(" " + sCmdLine + " ") || containsClusters(" " + sCmdLine + " ");
}

// Create a copy-efficient table object from the
// contents of the contained table (or the
// corresponding tables from the cache)
NumeRe::Table Datafile::extractTable(const string& _sTable)
{
    if (_sTable != "data")
        return MemoryManager::extractTable(_sTable);

    return NumeRe::Table(dDatafile, sHeadLine, nLines, nCols, "data");
}

// This member function is use th save the contents
// of this class into a file. The file type is selected
// by the extension of the passed file name
bool Datafile::saveFile(const string& sCache, string _sFileName, unsigned short nPrecision)
{
    if (!_sFileName.length())
        generateFileName();
    else
    {
        string sTemp = sPath;
        setPath(sSavePath, false, sWhere);

        sOutputFile = FileSystem::ValidFileName(_sFileName, ".ndat");
        setPath(sTemp, false, sWhere);
    }

    // Redirect the control to the memory manager, if not
    // the data in this class was adressed
    if (sCache != "data")
        return MemoryManager::saveLayer(sOutputFile, sCache, nPrecision);

    if (getCacheStatus())
        return MemoryManager::saveLayer(sOutputFile, "cache", nPrecision);

    // Get an instance of the desired file type
    NumeRe::GenericFile<double>* file = NumeRe::getFileByType(sOutputFile);

    // Ensure that the file instance is valid
    if (!file)
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sOutputFile, SyntaxError::invalid_position, sOutputFile);

    // Set generic informations in the file instance
    file->setDimensions(nLines, nCols);
    file->setColumnHeadings(sHeadLine, nCols);
    file->setData(dDatafile, nLines, nCols);
    file->setTableName("data");
    file->setTextfilePrecision(nPrecision);

    // If the file type is a NumeRe data file,
    // we may set the comment associated with this
    // class
    if (file->getExtension() == "ndat")
        static_cast<NumeRe::NumeReDataFile*>(file)->setComment("");

    // Try to write the data to the file. This might
    // either result in writing errors or the write
    // function is not defined for this file type
    try
    {
        if (!file->write())
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sOutputFile, SyntaxError::invalid_position, sOutputFile);
    }
    catch (...)
    {
        delete file;
        throw;
    }

    delete file;

    return true;
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
        return MemoryManager::std(sCache, i1,i2,j1,j2);
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

double Datafile::std(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::std(sCache, _vLine, _vCol);
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
        return MemoryManager::avg(sCache, i1,i2,j1,j2);
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

double Datafile::avg(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::avg(sCache, _vLine, _vCol);
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
        return MemoryManager::max(sCache, i1,i2,j1,j2);
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

double Datafile::max(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::max(sCache, _vLine, _vCol);
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
        return MemoryManager::min(sCache, i1,i2,j1,j2);
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

double Datafile::min(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::min(sCache, _vLine, _vCol);
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
        return MemoryManager::prd(sCache, i1,i2,j1,j2);
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

double Datafile::prd(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::prd(sCache, _vLine, _vCol);
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
        return MemoryManager::sum(sCache, i1,i2,j1,j2);
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

double Datafile::sum(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::sum(sCache, _vLine, _vCol);
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

double Datafile::num(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::num(sCache, _vLine, _vCol);
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
        return MemoryManager::num(sCache, i1,i2,j1,j2);
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
        return MemoryManager::and_func(sCache, i1,i2,j1,j2);
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

double Datafile::and_func(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::and_func(sCache, _vLine, _vCol);
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
        return MemoryManager::or_func(sCache, i1,i2,j1,j2);
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

double Datafile::or_func(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::or_func(sCache, _vLine, _vCol);
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
        return MemoryManager::xor_func(sCache, i1,i2,j1,j2);
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

double Datafile::xor_func(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::xor_func(sCache, _vLine, _vCol);
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
        return MemoryManager::cnt(sCache, i1,i2,j1,j2);
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

double Datafile::cnt(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::cnt(sCache, _vLine, _vCol);
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
        return MemoryManager::norm(sCache, i1,i2,j1,j2);
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

double Datafile::norm(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::norm(sCache, _vLine, _vCol);
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

double Datafile::cmp(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, double dRef, int _nType)
{
    //cerr << sCache << i1 << " " << i2 << " " << j1 << " " << j2 << " " << dRef << " " << nType << endl;
    if (sCache != "data")
        return MemoryManager::cmp(sCache, i1, i2, j1, j2, dRef, _nType);

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

    enum
    {
        RETURN_VALUE = 1,
        RETURN_LE = 2,
        RETURN_GE = 4,
        RETURN_FIRST = 8
    };

    int nType = 0;

    double dKeep = dRef;
    int nKeep = -1;

    if (_nType > 0)
        nType = RETURN_GE;
    else if (_nType < 0)
        nType = RETURN_LE;

    switch (intCast(fabs(_nType)))
    {
        case 2:
            nType |= RETURN_VALUE;
            break;
        case 3:
            nType |= RETURN_FIRST;
            break;
        case 4:
            nType |= RETURN_FIRST | RETURN_VALUE;
            break;
    }

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dDatafile[i][j]))
				continue;

			if (dDatafile[i][j] == dRef)
			{
				if (nType & RETURN_VALUE)
					return dDatafile[i][j];

                if (i1 == i2)
                    return j + 1;

                return i + 1;
			}
			else if (nType & RETURN_GE && dDatafile[i][j] > dRef)
			{
			    if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dDatafile[i][j];

                    if (i1 == i2)
						return j + 1;

					return i + 1;
                }

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
			else if (nType & RETURN_LE && dDatafile[i][j] < dRef)
			{
				if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dDatafile[i][j];

                    if (i1 == i2)
						return j + 1;

					return i + 1;
                }

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
	else if (nType & RETURN_VALUE)
		return dKeep;
	else
		return nKeep + 1;
}

double Datafile::cmp(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dRef, int _nType)
{
    if (sCache != "data")
        return MemoryManager::cmp(sCache, _vLine, _vCol, dRef, _nType);

	if (!bValidData)
		return NAN;

    enum
    {
        RETURN_VALUE = 1,
        RETURN_LE = 2,
        RETURN_GE = 4,
        RETURN_FIRST = 8
    };

    int nType = 0;

    double dKeep = dRef;
    int nKeep = -1;

    if (_nType > 0)
        nType = RETURN_GE;
    else if (_nType < 0)
        nType = RETURN_LE;

    switch (intCast(fabs(_nType)))
    {
        case 2:
            nType |= RETURN_VALUE;
            break;
        case 3:
            nType |= RETURN_FIRST;
            break;
        case 4:
            nType |= RETURN_FIRST | RETURN_VALUE;
            break;
    }

	for (long long int i = 0; i < _vLine.size(); i++)
	{
		for (long long int j = 0; j < _vCol.size(); j++)
		{
			if (_vLine[i] < 0 || _vLine[i] >= nLines || _vCol[j] < 0 || _vCol[j] >= nCols)
				continue;

			if (isnan(dDatafile[_vLine[i]][_vCol[j]]))
				continue;

			if (dDatafile[_vLine[i]][_vCol[j]] == dRef)
			{
				if (nType & RETURN_VALUE)
					return dDatafile[_vLine[i]][_vCol[j]];

                if (_vLine[0] == _vLine[_vLine.size() - 1])
                    return _vCol[j] + 1;

                return _vLine[i] + 1;
			}
			else if (nType & RETURN_GE && dDatafile[_vLine[i]][_vCol[j]] > dRef)
			{
			    if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dDatafile[_vLine[i]][_vCol[j]];

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
						return _vCol[j] + 1;

					return _vLine[i] + 1;
                }

				if (nKeep == -1 || dDatafile[_vLine[i]][_vCol[j]] < dKeep)
				{
					dKeep = dDatafile[_vLine[i]][_vCol[j]];
					if (_vLine[0] == _vLine[_vLine.size() - 1])
						nKeep = _vCol[j];
					else
						nKeep = _vLine[i];
				}
				else
					continue;
			}
			else if (nType & RETURN_LE && dDatafile[_vLine[i]][_vCol[j]] < dRef)
			{
				if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dDatafile[_vLine[i]][_vCol[j]];

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
						return _vCol[j] + 1;

					return _vLine[i] + 1;
                }

                if (nKeep == -1 || dDatafile[_vLine[i]][_vCol[j]] > dKeep)
				{
					dKeep = dDatafile[_vLine[i]][_vCol[j]];
					if (_vLine[0] == _vLine[_vLine.size() - 1])
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
	else if (nType & RETURN_VALUE)
		return dKeep;
	else
		return nKeep + 1;
}

double Datafile::med(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (sCache != "data")
        return MemoryManager::med(sCache, i1, i2, j1, j2);
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
                    _cache.writeToTable((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dDatafile[i][j]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToTable(i-i1,j-j1,"cache",dDatafile[i][j]);
            }
            else
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToTable(j-j1,i-i1,"cache",dDatafile[i][j]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    if (_cache.getTableLines("cache", false) % 2)
    {
        return _cache.getElement(_cache.getTableLines("cache", false) / 2, 0, "cache");
    }
    else
    {
        return (_cache.getElement(_cache.getTableLines("cache", false) / 2, 0, "cache") + _cache.getElement(_cache.getTableLines("cache", false) / 2 - 1, 0, "cache")) / 2.0;
    }
}

double Datafile::med(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (sCache != "data")
        return MemoryManager::med(sCache, _vLine, _vCol);
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
        return MemoryManager::pct(sCache, i1, i2, j1, j2, dPct);
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
                    _cache.writeToTable((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dDatafile[i][j]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToTable(i-i1,j-j1,"cache",dDatafile[i][j]);
            }
            else
            {
                if (!isnan(dDatafile[i][j]))
                    _cache.writeToTable(j-j1,i-i1,"cache",dDatafile[i][j]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    return (1-((_cache.getTableLines(0,false)-1)*dPct-floor((_cache.getTableLines(0,false)-1)*dPct)))*_cache.getElement(floor((_cache.getTableLines(0,false)-1)*dPct),0,"cache")
        + ((_cache.getTableLines(0,false)-1)*dPct-floor((_cache.getTableLines(0,false)-1)*dPct))*_cache.getElement(floor((_cache.getTableLines(0,false)-1)*dPct)+1,0,"cache");
}

double Datafile::pct(const string& sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dPct)
{
    if (sCache != "data")
        return MemoryManager::pct(sCache, _vLine, _vCol, dPct);
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





