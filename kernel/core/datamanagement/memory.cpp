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


#include "memory.hpp"
#include "../../kernel.hpp"
#include "../io/file.hpp"
#include <memory>

using namespace std;

/*
 * Realisierung der Memory-Klasse
 */

void prepareRegion(RetoqueRegion& _region, unsigned int nSize, double _dMedian = NAN)
{
    _region.vDataArray.resize(nSize);
    _region.vValidationArray.resize(nSize);
    for (unsigned int i = 0; i < nSize; i++)
    {
        _region.vDataArray[i].resize(nSize, NAN);
        _region.vValidationArray[i].resize(nSize, false);
    }
    _region.dMedian = _dMedian;
    return;
}


size_t qSortDouble(double* dArray, size_t nlength);

// --> Standard-Konstruktor <--
Memory::Memory()
{
    nLines = 128;
    nCols = 8;
    nWrittenHeadlines = 0;
    dMemTable = nullptr;
    sHeadLine = nullptr;
    nAppendedZeroes = nullptr;
    bValidData = false;
    bSaveMutex = false;
    bIsSaved = true;
    nLastSaved = time(0);
}

// --> Allgemeiner Konstruktor <--
Memory::Memory(long long int _nLines, long long int _nCols) : Memory()
{
    nLines = _nLines;
    nCols = _nCols;
    Allocate(_nLines, _nCols);
}

// --> Destruktor <--
Memory::~Memory()
{
    // --> Gib alle Speicher frei, sofern sie belegt sind! (Pointer != 0) <--
    if (dMemTable)
    {
        for (long long int i = 0; i < nLines; i++)
            delete[] dMemTable[i];

        delete[] dMemTable;
        dMemTable = nullptr;
    }

    if (sHeadLine)
    {
        delete[] sHeadLine;
        sHeadLine = nullptr;
    }

    if (nAppendedZeroes)
    {
        delete[] nAppendedZeroes;
        nAppendedZeroes = nullptr;
    }
}

// --> Generiere eine neue Matrix auf Basis der gesetzten Werte. Pruefe zuvor, ob nicht schon eine vorhanden ist <--
bool Memory::Allocate(long long int _nNLines, long long int _nNCols, bool shrink)
{
    if (_nNCols * _nNLines > 1e8)
    {
        //throw TOO_LARGE_CACHE;
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, "", SyntaxError::invalid_position);
    }
    else if (!dMemTable && !nAppendedZeroes && !sHeadLine)
    {
        sHeadLine = new string[_nNCols];
        nAppendedZeroes = new long long int[_nNCols];
        for (long long int j = 0; j < _nNCols; j++)
        {
            sHeadLine[j] = "Spalte_" + toString(j + 1);
            nAppendedZeroes[j] = _nNLines;
        }

        dMemTable = new double*[_nNLines];
        for (long long int i = 0; i < _nNLines; i++)
        {
            dMemTable[i] = new double[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                dMemTable[i][j] = NAN;
            }
        }

        nLines = _nNLines;
        nCols = _nNCols;
    }
    else if (nLines && nCols && dMemTable && nAppendedZeroes)
    {
        // Do nothing if the cache is already equal or larger in size
        if (nLines >= _nNLines && nCols >= _nNCols && !shrink)
            return true;

        string* sNewHeadLine = new string[_nNCols];
        long long int* nNewAppendedZeroes = new long long int[_nNCols];
        for (long long int j = 0; j < _nNCols; j++)
        {
            if (j < nCols)
            {
                sNewHeadLine[j] = sHeadLine[j];
                nNewAppendedZeroes[j] = nAppendedZeroes[j] + (_nNLines - nLines);
            }
            else
            {
                sNewHeadLine[j] = "Spalte_" + toString(j + 1);
                nNewAppendedZeroes[j] = _nNLines;
            }
        }

        double** dNewCache = new double*[_nNLines];
        for (long long int i = 0; i < _nNLines; i++)
        {
            dNewCache[i] = new double[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                if (i < nLines && j < nCols)
                    dNewCache[i][j] = dMemTable[i][j];
                else
                    dNewCache[i][j] = NAN;
            }
        }

        for (long long int i = 0; i < nLines; i++)
        {
            delete[] dMemTable[i];
        }
        delete[] dMemTable;
        delete[] nAppendedZeroes;
        delete[] sHeadLine;

        nCols = _nNCols;
        nLines = _nNLines;

        dMemTable = dNewCache;
        nAppendedZeroes = nNewAppendedZeroes;
        sHeadLine = sNewHeadLine;
    }
    else
    {
        NumeReKernel::print("FEHLER: Kann nicht in den Memory schreiben!");
        return false;
    }
    return true;
}


// --> Setzt nCols <--
bool Memory::resizeMemory(long long int _nLines, long long int _nCols)
{
    long long int _nNCols = nCols;
    long long int _nNLines = nLines;

    while (_nLines > _nNLines)
        _nNLines *= 2;
    while (_nCols > _nNCols)
        _nNCols *= 2;
    if (!Allocate(_nNLines, _nNCols))
        return false;
    return true;
}

// --> gibt nCols zurueck <--
long long int Memory::getCols(bool _bFull) const
{
    if (!_bFull && dMemTable && (bValidData || nWrittenHeadlines))
    {
        if (nAppendedZeroes && bValidData)
        {
            long long int nReturn = nCols;
            /* --> Von oben runterzaehlen, damit nur die leeren Spalten rechts von den Daten
             *     ignoriert werden! <--
             */
            for (long long int i = nCols - 1; i >= 0; i--)
            {
                if (nAppendedZeroes[i] == nLines)
                    nReturn--;
                // --> Findest du eine Spalte die nicht leer ist, dann breche die Schleife ab! <--
                if (nAppendedZeroes[i] != nLines)
                    break;
            }
            return std::max(nReturn, nWrittenHeadlines);
        }
        else
            return nWrittenHeadlines;
    }
    else if (!dMemTable || !bValidData)
        return 0;
    else
        return nCols;
}

// --> gibt nLines zurueck <--
long long int Memory::getLines(bool _bFull) const
{
    if (!_bFull && dMemTable && bValidData)
    {
        if (nAppendedZeroes)
        {
            long long int nReturn = 0;
            /* --> Suche die Spalte, in der am wenigsten Nullen angehaengt sind, und gib deren
             *     Laenge zurueck <--
             */
            for (long long int i = 0; i < nCols; i++)
            {
                if (nLines - nAppendedZeroes[i] > nReturn)
                    nReturn = nLines - nAppendedZeroes[i];
            }
            return nReturn;
        }
        else
            return 0;
    }
    else if (!dMemTable || !bValidData)
        return 0;
    else
        return nLines;
}

// --> gibt das Element der _nLine-ten Zeile und der _nCol-ten Spalte zurueck <--
double Memory::readMem(long long int _nLine, long long int _nCol) const
{
    if (_nLine < nLines && _nCol < nCols && dMemTable && _nLine >= 0 && _nCol >= 0)
        return dMemTable[_nLine][_nCol];
    else
        return NAN;
}

vector<double> Memory::readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    vector<double> vReturn;

    if ((_vLine.size() > 1 && _vCol.size() > 1) || !dMemTable)
        vReturn.push_back(NAN);
    else
    {
        long long int nCurLines = getLines(false);
        long long int nCurCols = getCols(false);
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            for (unsigned int j = 0; j < _vCol.size(); j++)
            {
                if (_vLine[i] < 0
                        || _vLine[i] >= nCurLines
                        || _vCol[j] < 0
                        || _vCol[j] >= nCurCols
                        || _vLine[i] >= nLines - nAppendedZeroes[_vCol[j]])
                    vReturn.push_back(NAN);
                else
                    vReturn.push_back(dMemTable[_vLine[i]][_vCol[j]]);
            }
        }
    }
    return vReturn;
}

void Memory::copyElementsInto(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    vTarget->clear();
    if ((_vLine.size() > 1 && _vCol.size() > 1) || !dMemTable)
        vTarget->resize(1, NAN);
    else
    {
        vTarget->resize(_vLine.size()*_vCol.size(), NAN);
        long long int nCurLines = getLines(false);
        long long int nCurCols = getCols(false);
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            for (unsigned int j = 0; j < _vCol.size(); j++)
            {
                //cerr << _vLine[i] << endl;
                if (_vLine[i] >= nCurLines
                        || _vCol[j] >= nCurCols
                        || _vCol[j] < 0
                        || _vLine[i] < 0
                        || _vLine[i] >= nLines - nAppendedZeroes[_vCol[j]])
                    (*vTarget)[j + i * _vCol.size()] = NAN;
                else
                    (*vTarget)[j + i * _vCol.size()] = dMemTable[_vLine[i]][_vCol[j]];
            }
        }
    }
}

// --> gibt zurueck, ob das Element der _nLine-ten Zeile und _nCol-ten Spalte ueberhaupt gueltig ist <--
bool Memory::isValidElement(long long int _nLine, long long int _nCol) const
{
    if (_nLine < nLines && _nLine >= 0 && _nCol < nCols && _nCol >= 0 && dMemTable)
        return !isnan(dMemTable[_nLine][_nCol]);
    else
        return false;
}

// --> gibt den Wert von bValidData zurueck <--
bool Memory::isValid() const
{
    if (!dMemTable)
        return false;

    if (getCols(false))
        return true;
    return false;
}


bool Memory::shrink()
{
    if (!bValidData)
    {
        return true;
    }
    const long long int nCurLines = getLines(false);
    const long long int nCurCols = getCols(false);
    long long int nShrinkedLines = 1;
    long long int nShrinkedCols = 1;

    while (nShrinkedLines < nCurLines)
        nShrinkedLines *= 2;

    while (nShrinkedCols < nCurCols)
        nShrinkedCols *= 2;

    if (nShrinkedCols * nShrinkedLines < 100 * nLines * nCols)
    {
        if (!Allocate(nShrinkedLines, nShrinkedCols, true))
            return false;
    }
    return true;
}

// --> gibt den Wert von bIsSaved zurueck <--
bool Memory::getSaveStatus() const
{
    return bIsSaved;
}

// --> gibt das _i-te Element der gespeicherten Kopfzeile zurueck <--
string Memory::getHeadLineElement(long long int _i) const
{
    if (_i >= getCols(false))
        return "Spalte " + toString((int)_i + 1) + " (leer)";
    else
        return sHeadLine[_i];
}

vector<string> Memory::getHeadLineElement(const VectorIndex& _vCol) const
{
    vector<string> vHeadLines;
    long long int nCurCols = getCols(false);

    for (unsigned int i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0)
            continue;
        if (_vCol[i] >= nCurCols || _vCol[i] < 0)
            vHeadLines.push_back("Spalte " + toString((int)_vCol[i] + 1) + " (leer)");
        else
            vHeadLines.push_back(sHeadLine[_vCol[i]]);
    }

    return vHeadLines;
}

// --> schreibt _sHead in das _i-te Element der Kopfzeile <--
bool Memory::setHeadLineElement(long long int _i, string _sHead)
{
    if (_i < nCols && dMemTable)
    {
        sHeadLine[_i] = _sHead;
        if (_i >= nWrittenHeadlines && _sHead != "Spalte_" + toString(_i + 1) && _sHead != "Spalte " + toString(_i + 1) + " (leer)")
            nWrittenHeadlines = _i+1;
    }
    else
    {
        if (!resizeMemory(nLines, _i + 1))
            return false;
        sHeadLine[_i] = _sHead;
        if (_sHead != "Spalte_" + toString(_i + 1) && _sHead != "Spalte " + toString(_i + 1) + " (leer)")
            nWrittenHeadlines = _i+1;
    }

    if (bIsSaved)
    {
        nLastSaved = time(0);
        bIsSaved = false;
    }
    return true;
}

// --> gibt die Zahl der in der _i-ten Spalte angehaengten Nullzeilen zurueck <--
long long int Memory::getAppendedZeroes(long long int _i) const
{
    if (nAppendedZeroes && _i < nCols)
        return nAppendedZeroes[_i];
    else
        return nLines;
}

// This function counts the number of all headline lines
int Memory::getHeadlineCount() const
{
    int nHeadlineCount = 1;
    // Get the dimensions of the complete headline (i.e. including possible linebreaks)
    for (long long int j = 0; j < getCols(); j++)
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


// --> Schreibt einen Wert an beliebiger Stelle in den Memory <--
bool Memory::writeData(long long int _nLine, long long int _nCol, double _dData)
{
    if (dMemTable && (_nLine < nLines) && (_nCol < nCols))
    {
        if (isnan(_dData))
        {
            dMemTable[_nLine][_nCol] = NAN;

            // re-count the number of appended zeros for the current column
            if (nLines - nAppendedZeroes[_nCol] == _nLine + 1)
            {
                nAppendedZeroes[_nCol] = 0;

                for (long long int j = nLines - 1; j >= 0; j--)
                {
                    if (isnan(dMemTable[j][_nCol]))
                        nAppendedZeroes[_nCol]++;
                    else
                        break;
                }
            }
        }
        else
        {
            dMemTable[_nLine][_nCol] = _dData;

            if (nLines - nAppendedZeroes[_nCol] <= _nLine)
            {
                nAppendedZeroes[_nCol] = nLines - _nLine - 1;
            }

            bValidData = true;
        }
    }
    else if (!dMemTable && isnan(_dData))
        return true;
    else
    {
        /* --> Ist der Memory zu klein? Verdoppele die fehlenden Dimensionen so lange,
         *     bis die fehlende Dimension groesser als das zu schreibende Matrixelement
         *     ist. <--
         */
        long long int _nNLines = nLines;
        long long int _nNCols = nCols;

        while (_nLine + 1 >= _nNLines)
        {
            _nNLines = 2 * _nNLines;
        }

        while (_nCol + 1 >= _nNCols)
        {
            _nNCols = 2 * _nNCols;
        }

        if (!Allocate(_nNLines, _nNCols))
            return false;

        if (isnan(_dData))
        {
            dMemTable[_nLine][_nCol] = NAN;
        }
        else
        {
            dMemTable[_nLine][_nCol] = _dData;

            if (nLines - nAppendedZeroes[_nCol] <= _nLine)
                nAppendedZeroes[_nCol] = nLines - _nLine - 1;
        }

        nLines = _nNLines;
        nCols = _nNCols;

        if (!isnan(_dData) && !bValidData)
            bValidData = true;
    }

    // --> Setze den Zeitstempel auf "jetzt", wenn der Memory eben noch gespeichert war <--
    if (bIsSaved)
    {
        nLastSaved = time(0);
        bIsSaved = false;
    }
    return true;
}

// --> Schreibt einen Wert an beliebiger Stelle in den Memory <--
bool Memory::writeData(Indices& _idx, double* _dData, unsigned int _nNum)
{
    int nDirection = LINES;
    if (_nNum == 1)
        return writeSingletonData(_idx, _dData);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _idx.row.front() + _nNum - 1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + _nNum - 1);

    if (_idx.row.size() > 1)
        nDirection = COLS;
    else if (_idx.col.size() > 1)
        nDirection = LINES;

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (nDirection == COLS)
            {
                if (_nNum > i && !isnan(_idx.row[i]) && !isnan(_idx.col[j]))
                    writeData(_idx.row[i], _idx.col[j], _dData[i]);
            }
            else
            {
                if (_nNum > j && !isnan(_idx.row[i]) && !isnan(_idx.col[j]))
                    writeData(_idx.row[i], _idx.col[j], _dData[j]);
            }
        }
    }


    return true;
}

bool Memory::writeSingletonData(Indices& _idx, double* _dData)
{
    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, ::max(_idx.row.front(), getLines(false)) - 1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, ::max(_idx.col.front(), getCols(false)) - 1);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (!isnan(_idx.row[i]) && !isnan(_idx.col[j]))
                writeData(_idx.row[i], _idx.col[j], _dData[0]);
        }
    }

    return true;
}


void Memory::setSaveStatus(bool _bIsSaved)
{
    bIsSaved = _bIsSaved;
    if (bIsSaved)
        nLastSaved = time(0);
    return;
}

long long int Memory::getLastSaved() const
{
    return nLastSaved;
}

vector<int> Memory::sortElements(long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    if (!dMemTable)
        return vector<int>();
    bool bError = false;
    bool bReturnIndex = false;
    int nSign = 1;

    vector<int> vIndex;

    if (findParameter(sSortingExpression, "desc"))
        nSign = -1;

    if (!Memory::getCols(false))
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
            if (!qSort(&vIndex[0], i2 - i1 + 1, i, 0, i2 - i1, nSign))
            {
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
            }
            if (bReturnIndex)
                break;
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
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "cols", '=') + 4);
        }
        else
        {
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "c", '=') + 1);
        }

        while (sCols.length())
        {
            ColumnKeys* keys = evaluateKeyList(sCols, j2 - j1 + 1);
            if (!keys)
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE,  sSortingExpression, SyntaxError::invalid_position);

            if (keys->nKey[1] == -1)
                keys->nKey[1] = keys->nKey[0] + 1;

            for (int j = keys->nKey[0]; j < keys->nKey[1]; j++)
            {
                if (!qSort(&vIndex[0], i2 - i1 + 1, j + j1, 0, i2 - i1, nSign))
                {
                    delete keys;
                    throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                }
                // Subkey list
                if (keys->subkeys && keys->subkeys->subkeys)
                {
                    if (!sortSubList(&vIndex[0], i2 - i1 + 1, keys, i1, i2, j1, nSign, getCols(false)))
                    {
                        delete keys;
                        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                    }
                }
                if (bReturnIndex)
                    break;
                reorderColumn(vIndex, i1, i2, j + j1);
                ColumnKeys* subKeyList = keys->subkeys;

                while (subKeyList)
                {
                    if (subKeyList->nKey[1] == -1)
                        subKeyList->nKey[1] = subKeyList->nKey[0] + 1;
                    for (int j = subKeyList->nKey[0]; j < subKeyList->nKey[1]; j++)
                    {
                        reorderColumn(vIndex, i1, i2, j + j1);
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

    for (int i = 0; i < getCols(false); i++)
    {
        for (int j = nLines - 1; j >= 0; j--)
        {
            if (!isnan(dMemTable[j][i]))
            {
                nAppendedZeroes[i] = nLines - j - 1;
                break;
            }
        }
    }

    if (bReturnIndex)
    {
        for (int i = 0; i <= i2 - i1; i++)
            vIndex[i]++;
    }

    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }

    if (bError || !bReturnIndex)
        return vector<int>();
    return vIndex;
}

void Memory::reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1)
{
    double* dSortVector = new double[i2 - i1 + 1];
    for (int i = 0; i <= i2 - i1; i++)
    {
        dSortVector[i] = dMemTable[vIndex[i]][j1];
    }
    for (int i = 0; i <= i2 - i1; i++)
    {
        dMemTable[i + i1][j1] = dSortVector[i];
    }
    delete[] dSortVector;
}

// Implementation for the "Sorter" object
int Memory::compare(int i, int j, int col)
{
    if (dMemTable[i][col] == dMemTable[j][col])
        return 0;
    else if (dMemTable[i][col] < dMemTable[j][col])
        return -1;

    return 1;
}

// Implementation for the "Sorter" object
bool Memory::isValue(int line, int col)
{
    return !isnan(dMemTable[line][col]);
}

// Create a copy-efficient table object
// from the data contents
NumeRe::Table Memory::extractTable(const string& _sTable)
{
    return NumeRe::Table(dMemTable, sHeadLine, getLines(false), getCols(false), _sTable);
}

// Import data from a copy-efficient table object
void Memory::importTable(NumeRe::Table _table)
{
    deleteBulk(VectorIndex(0, -2), VectorIndex(0, -2));
    resizeMemory(_table.getLines(), _table.getCols());

    for (size_t i = 0; i < _table.getLines(); i++)
    {
        for (size_t j = 0; j < _table.getCols(); j++)
        {
            // Use writeData() to automatically set all
            // other parameters
            writeData(i, j, _table.getValue(i, j));
        }
    }

    // Set the table heads, if they have
    // a non-zero length
    for (size_t j = 0; j < _table.getCols(); j++)
    {
        if (_table.getHead(j).length())
            sHeadLine[j] = _table.getHead(j);
    }
}

// This member function is used for saving the
// contents of this memory page into a file. The
// type of the file is selected by the name of the
// file
bool Memory::save(string _sFileName, const string& sTableName, unsigned short nPrecision)
{
    // Get an instance of the desired file type
    NumeRe::GenericFile<double>* file = NumeRe::getFileByType(_sFileName);

    // Ensure that a file was created
    if (!file)
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, _sFileName, SyntaxError::invalid_position, _sFileName);

    long long int lines = getLines(false);
    long long int cols = getCols(false);

    // Set the dimensions and the generic information
    // in the file
    file->setDimensions(lines, cols);
    file->setColumnHeadings(sHeadLine, cols);
    file->setData(dMemTable, lines, cols);
    file->setTableName(sTableName);
    file->setTextfilePrecision(nPrecision);

    // If the file type is a NumeRe data file, then
    // we can also set the comment associated with
    // this memory page
    if (file->getExtension() == "ndat")
        static_cast<NumeRe::NumeReDataFile*>(file)->setComment("");

    // Try to write the data to the file. This might
    // either result in writing errors or the write
    // function is not defined for this file type
    try
    {
        if (!file->write())
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, _sFileName, SyntaxError::invalid_position, _sFileName);
    }
    catch (...)
    {
        delete file;
        throw;
    }

    // Delete the created file instance
    delete file;

    return true;
}


void Memory::deleteEntry(long long int _nLine, long long int _nCol)
{
    if (dMemTable)
    {
        if (!isnan(dMemTable[_nLine][_nCol]))
        {
            dMemTable[_nLine][_nCol] = NAN;
            if (bIsSaved)
            {
                nLastSaved = time(0);
                bIsSaved = false;
            }
            for (long long int i = nLines - 1; i >= 0; i--)
            {
                if (!isnan(dMemTable[i][_nCol]))
                {
                    nAppendedZeroes[_nCol] = nLines - i - 1;
                    break;
                }
                if (!i && isnan(dMemTable[i][_nCol]))
                {
                    nAppendedZeroes[_nCol] = nLines;
                    if (!_nLine)
                    {
                        sHeadLine[_nCol] = "Spalte_" + toString((int)_nCol + 1);
                        if (nWrittenHeadlines > _nCol)
                            nWrittenHeadlines = _nCol;
                    }
                }
            }
            if (!getLines(false) && !getCols(false))
                bValidData = false;
        }
    }
    return;
}

void Memory::deleteBulk(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    bool bHasFirstLine = false;

    if (!Memory::getCols(false))
        return;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        if (!_vLine[i])
            bHasFirstLine = true;
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] >= nCols || _vCol[j] < 0 || _vLine[i] >= nLines || _vLine[i] < 0)
                continue;

            dMemTable[_vLine[i]][_vCol[j]] = NAN;
        }
    }
    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }

    for (long long int j = nCols - 1; j >= 0; j--)
    {
        int currentcol = -1;
        for (size_t i = 0; i < _vCol.size(); i++)
        {
            if (_vCol[i] == j)
            {
                currentcol = (int)i;
            }
        }
        for (long long int i = nLines - 1; i >= 0; i--)
        {
            if (!isnan(dMemTable[i][j]))
            {
                nAppendedZeroes[j] = nLines - i - 1;
                break;
            }
            if (!i && isnan(dMemTable[i][j]))
            {
                nAppendedZeroes[j] = nLines;
                if (currentcol >= 0 && bHasFirstLine)
                {
                    sHeadLine[j] = "Spalte_" + toString((int)j + 1);
                    if (nWrittenHeadlines > j)
                        nWrittenHeadlines = j;
                }
            }
        }
    }

    if (!getLines(false) && !getCols(false))
        bValidData = false;
    return;
}


bool Memory::evaluateIndices(long long int& i1, long long int& i2, long long int& j1, long long int& j2)
{
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
        i2 = getLines(false) - 1;
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCols(false) - 1;

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

    if (i1 >= getLines(false) || j1 >= getCols(false))
        return false;
    if (i2 >= getLines(false))
        i2 = getLines(false) - 1;
    if (j2 >= getCols(false))
        j2 = getCols(false) - 1;
    return true;
}

// This member function counts the number of
// appended zeroes, i.e. the number of invalid
// values, which are appended at the end of the
// columns
void Memory::countAppendedZeroes()
{
    for (long long int i = 0; i < nCols; i++)
    {
        nAppendedZeroes[i] = 0;

        for (long long int j = nLines-1; j >= 0; j--)
        {
            if (isnan(dMemTable[j][i]))
                nAppendedZeroes[i]++;
            else
                break;
        }
    }
}



double Memory::std(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dAvg = avg(_vLine, _vCol);
    double dStd = 0.0;
    unsigned int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                nInvalid++;
            else if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                nInvalid++;
            else
                dStd += (dAvg - dMemTable[_vLine[i]][_vCol[j]]) * (dAvg - dMemTable[_vLine[i]][_vCol[j]]);
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size() - 1)
        return NAN;
    return sqrt(dStd / ((_vLine.size() * _vCol.size()) - 1 - nInvalid));
}

double Memory::avg(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dAvg = 0.0;
    unsigned int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                nInvalid++;
            else if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                nInvalid++;
            else
                dAvg += dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    return dAvg / (_vLine.size() * _vCol.size() - nInvalid);
}

double Memory::max(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dMax = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;
            if (isnan(dMax))
                dMax = dMemTable[_vLine[i]][_vCol[j]];
            if (dMax < dMemTable[_vLine[i]][_vCol[j]])
                dMax = dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    return dMax;
}

double Memory::min(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dMin = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;
            if (isnan(dMin))
                dMin = dMemTable[_vLine[i]][_vCol[j]];
            if (dMin > dMemTable[_vLine[i]][_vCol[j]])
                dMin = dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    return dMin;
}

double Memory::prd(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dPrd = 1.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;
            dPrd *= dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    return dPrd;
}

double Memory::sum(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dSum = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;
            dSum += dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    return dSum;
}

double Memory::num(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return 0;
    int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                nInvalid++;
            else if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                nInvalid++;
        }
    }
    return (_vLine.size() * _vCol.size()) - nInvalid;
}

double Memory::and_func(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return 0.0;


    double dRetVal = NAN;
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dRetVal))
                dRetVal = 1.0;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]) || dMemTable[_vLine[i]][_vCol[j]] == 0)
                return 0.0;
        }
    }

    if (isnan(dRetVal))
        return 0.0;
    return 1.0;
}

double Memory::or_func(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]) || dMemTable[_vLine[i]][_vCol[j]] != 0)
                return 1.0;
        }
    }
    return 0.0;
}

double Memory::xor_func(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return 0.0;

    bool isTrue = false;
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]) || dMemTable[_vLine[i]][_vCol[j]] != 0)
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

double Memory::cnt(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return 0;
    int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0
                    || _vLine[i] >= getLines(true)
                    || _vCol[j] < 0
                    || _vCol[j] >= getCols(true))
                nInvalid++;
        }
    }
    return (_vLine.size() * _vCol.size()) - nInvalid;
}

double Memory::norm(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!bValidData)
        return NAN;
    double dNorm = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;
            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;
            dNorm += dMemTable[_vLine[i]][_vCol[j]] * dMemTable[_vLine[i]][_vCol[j]];
        }
    }
    return sqrt(dNorm);
}

double Memory::cmp(const VectorIndex& _vLine, const VectorIndex& _vCol, double dRef, int _nType)
{
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
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                continue;

            if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;

            if (dMemTable[_vLine[i]][_vCol[j]] == dRef)
            {
                if (nType & RETURN_VALUE)
                    return dMemTable[_vLine[i]][_vCol[j]];

                if (_vLine[0] == _vLine[_vLine.size() - 1])
                    return _vCol[j] + 1;

                return _vLine[i] + 1;
            }
            else if (nType & RETURN_GE && dMemTable[_vLine[i]][_vCol[j]] > dRef)
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dMemTable[_vLine[i]][_vCol[j]];

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        return _vCol[j] + 1;

                    return _vLine[i] + 1;
                }

                if (nKeep == -1 || dMemTable[_vLine[i]][_vCol[j]] < dKeep)
                {
                    dKeep = dMemTable[_vLine[i]][_vCol[j]];
                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
            else if (nType & RETURN_LE && dMemTable[_vLine[i]][_vCol[j]] < dRef)
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return dMemTable[_vLine[i]][_vCol[j]];

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        return _vCol[j] + 1;

                    return _vLine[i] + 1;
                }

                if (nKeep == -1 || dMemTable[_vLine[i]][_vCol[j]] > dKeep)
                {
                    dKeep = dMemTable[_vLine[i]][_vCol[j]];
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

double Memory::med(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
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
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                nInvalid++;
            else if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                nInvalid++;
        }
    }

    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;

    dData = new double[(_vLine.size()*_vCol.size()) - nInvalid];

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false) || isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;

            dData[nCount] = dMemTable[_vLine[i]][_vCol[j]];
            nCount++;

            if (nCount == (_vLine.size()*_vCol.size()) - nInvalid)
                break;
        }

        if (nCount == (_vLine.size()*_vCol.size()) - nInvalid)
            break;
    }

    nCount = qSortDouble(dData, nCount);

    if (!nCount)
    {
        delete[] dData;
        return NAN;
    }

    dMed = gsl_stats_median_from_sorted_data(dData, 1, nCount);

    delete[] dData;

    return dMed;
}

double Memory::pct(const VectorIndex& _vLine, const VectorIndex& _vCol, double dPct)
{
    if (!bValidData)
        return NAN;
    unsigned int nInvalid = 0;
    unsigned int nCount = 0;
    double* dData = 0;

    if (dPct >= 1 || dPct <= 0)
        return NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false))
                nInvalid++;
            else if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                nInvalid++;
        }
    }

    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;

    dData = new double[(_vLine.size()*_vCol.size()) - nInvalid];

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getLines(false) || _vCol[j] < 0 || _vCol[j] >= getCols(false) || isnan(dMemTable[_vLine[i]][_vCol[j]]))
                continue;

            dData[nCount] = dMemTable[_vLine[i]][_vCol[j]];
            nCount++;

            if (nCount == (_vLine.size()*_vCol.size()) - nInvalid)
                break;
        }

        if (nCount == (_vLine.size()*_vCol.size()) - nInvalid)
            break;
    }

    nCount = qSortDouble(dData, nCount);

    if (!nCount)
    {
        delete[] dData;
        return NAN;
    }

    dPct = gsl_stats_quantile_from_sorted_data(dData, 1, nCount, dPct);

    delete[] dData;

    return dPct;
}



bool Memory::retoque(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    if (!bValidData)
        return false;

    if (!_vLine.isValid() || !_vCol.isValid())
        return false;

    // Evaluate the indices
    if (_vLine.isOpenEnd())
    {
        bUseAppendedZeroes = true;
    }

    _vLine.setRange(0, nLines-1);
    _vCol.setRange(0, nCols-1);

    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (nMax < nLines - nAppendedZeroes[_vCol[j]] - 1)
                nMax = nLines - nAppendedZeroes[_vCol[j]] - 1;
        }

        _vLine.setRange(0, nMax);
    }

    if (Direction == GRID)
    {
        if (bUseAppendedZeroes)
        {
            if (!retoque(_vLine, VectorIndex(_vCol[0]), COLS) || !retoque(_vLine, VectorIndex(_vCol[1]), COLS))
                return false;
        }
        else
        {
            if (!retoque(_vLine, _vCol.subidx(0, 2), COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    if (Direction == ALL || Direction == GRID)
    {
        _vLine.linearize();
        _vCol.linearize();

        for (long long int i = _vLine.front(); i <= _vLine.last(); i++)
        {
            for (long long int j = _vCol.front(); j <= _vCol.last(); j++)
            {
                if (isnan(dMemTable[i][j]))
                {
                    if (i > _vLine.front() && i < _vLine.last() && j > _vCol.front() && j < _vCol.last() && isValidDisc(i - 1, j - 1, 2))
                    {
                        retoqueRegion(i - 1, i + 1, j - 1, j + 1, 1, ALL);

                        if (bIsSaved)
                        {
                            bIsSaved = false;
                            nLastSaved = time(0);
                        }
                    }
                }
            }
        }

        for (long long int i = _vLine.front(); i <= _vLine.last(); i++)
        {
            for (long long int j = _vCol.front(); j <= _vCol.last(); j++)
            {
                if (isnan(dMemTable[i][j]))
                {
                    if (i > _vLine.front() && i < _vLine.last() && j > _vCol.front() && j < _vCol.last() && isValidDisc(i - 1, j - 1, 2))
                    {
                        retoqueRegion(i - 1, i + 1, j - 1, j + 1, 1, ALL);
                    }
                    else if (i == _vLine.front() || i == _vLine.last() || j == _vCol.front() || j == _vCol.last())
                    {
                        unsigned int nOrder = 1;
                        long long int __i = i;
                        long long int __j = j;

                        if (i == _vLine.front())
                        {
                            if (j == _vCol.front())
                            {
                                while (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last()
                                        && num(VectorIndex(i + nOrder + 1, i + nOrder + 2), VectorIndex(j, j + nOrder + 1)) != cnt(VectorIndex(i + nOrder + 1, i + nOrder + 2), VectorIndex(j, j + nOrder + 1))
                                        && num(VectorIndex(i, i + nOrder + 2), VectorIndex(j + nOrder + 1, j + nOrder + 2)) != cnt(VectorIndex(i, i + nOrder + 2), VectorIndex(j + nOrder + 1, j + nOrder + 2)))
                                {
                                    nOrder++;
                                }

                                if (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                        {
                                            if (_i == __i + nOrder || _j == __j + nOrder)
                                            {
                                                _region.vDataArray[(_i == __i + nOrder ? 0 : _i - __i + 1)][(_j == __j + nOrder ? 0 : _j - __j + 1)] = dMemTable[_i][_j];
                                                _region.vValidationArray[(_i == __i + nOrder ? 0 : _i - __i + 1)][(_j == __j + nOrder ? 0 : _j - __j + 1)] = true;
                                            }

                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j + 1] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i + 1][_j - __j + 1] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j + 1] = NAN;
                                                _region.vValidationArray[_i - __i + 1][_j - __j + 1] = false;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i + 1][_j - __j + 1])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j + 1];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else if (j == _vCol.last())
                            {
                                __j--;

                                while (__i + nOrder + 1 <= _vLine.last() && __j >= _vCol.front() && __j + nOrder + 1 < _vCol.last()
                                        && num(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 1)) != cnt(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 1))
                                        && num(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j, __j + 1)) != cnt(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j, __j + 1)))
                                {
                                    nOrder++;

                                    if (__j > _vCol.front())
                                        __j--;
                                }

                                if (__i + nOrder + 1 <= _vLine.last() && __j >= _vCol.front() && __j + nOrder + 1 < _vCol.last())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i + 1][_j - __j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j] = NAN;
                                                _region.vValidationArray[_i - __i + 1][_j - __j] = false;
                                            }

                                            if (_i == __i + nOrder || _j == __j)
                                            {
                                                _region.vDataArray[(_i == __i + nOrder ? 0 : _i - __i + 1)][(_j == __j ? nOrder + 1 : _j - __j)] = dMemTable[_i][_j];
                                                _region.vValidationArray[(_i == __i + nOrder ? 0 : _i - __i + 1)][(_j == __j ? nOrder + 1 : _j - __j)] = true;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i + 1][_j - __j])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else
                            {
                                while (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last() && __j >= _vCol.front()
                                        && num(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 2)) != cnt(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 2))
                                        && num(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)))
                                {
                                    if (__j > _vCol.front())
                                        nOrder += 2;
                                    else
                                        nOrder++;

                                    if (__j > _vCol.front())
                                        __j--;
                                }

                                if (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last() && __j >= _vCol.front())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i + 1][_j - __j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i + 1][_j - __j] = NAN;
                                                _region.vValidationArray[_i - __i + 1][_j - __j] = false;
                                            }

                                            if (_i == __i + nOrder)
                                            {
                                                _region.vDataArray[0][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[0][_j - __j] = true;
                                                continue;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i + 1][_j - __j])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                        }
                        else if (i == _vLine.last())
                        {
                            __i--;

                            if (j == _vCol.last())
                            {
                                __j--;

                                while (__i >= _vLine.front() && __j >= _vCol.front()
                                        && num(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 1)) != cnt(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 1))
                                        && num(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + 1)) != cnt(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + 1)))
                                {
                                    if (__j > _vCol.front())
                                        __j--;

                                    if (__i > _vLine.front())
                                        __i--;

                                    nOrder++;
                                }

                                if (__i >= _vLine.front() && __j >= _vCol.front())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i][_j - __j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i][_j - __j] = NAN;
                                                _region.vValidationArray[_i - __i][_j - __j] = false;
                                            }

                                            if (_i == __i || _j == __j)
                                            {
                                                _region.vDataArray[(_i == __i ? nOrder + 1 : _i - __i)][(_j == __j ? nOrder + 1 : _j - __j)] = dMemTable[_i][_j];
                                                _region.vValidationArray[(_i == __i ? nOrder + 1 : _i - __i)][(_j == __j ? nOrder + 1 : _j - __j)] = true;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i][_j - __j])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else if (j == _vCol.front())
                            {
                                while (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last()
                                        && num(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 2)) != cnt(VectorIndex(__i + nOrder + 1, __i + nOrder + 2), VectorIndex(__j, __j + nOrder + 2))
                                        && num(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)))
                                {
                                    if (__i > _vLine.front())
                                        __i--;

                                    nOrder++;
                                }

                                if (__i + nOrder + 1 <= _vLine.last() && __j + nOrder + 1 <= _vCol.last())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                        {
                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i][_j - __j + 1] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i][_j - __j + 1] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i][_j - __j + 1] = NAN;
                                                _region.vValidationArray[_i - __i][_j - __j + 1] = false;
                                            }

                                            if (_i == __i || _j == __j + nOrder)
                                            {
                                                _region.vDataArray[(_i == __i ? nOrder + 1 : _i - __i)][(_j == __j + nOrder ? 0 : _j - __j + 1)] = dMemTable[_i][_j];
                                                _region.vValidationArray[(_i == __i ? nOrder + 1 : _i - __i)][(_j == __j + nOrder ? 0 : _j - __j + 1)] = true;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i][_j - __j + 1])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j + 1];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else
                            {
                                while (__i >= _vLine.front() && __j + nOrder + 1 <= _vCol.last()
                                        && num(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 2))
                                        && num(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)))
                                {
                                    nOrder++;

                                    if (__j > _vCol.front())
                                        __j--;

                                    if (__i > _vLine.front())
                                        __i--;
                                }

                                if (__i >= _vLine.front() && __j + nOrder + 1 <= _vCol.last())
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder)));

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (!isnan(dMemTable[_i][_j]))
                                            {
                                                _region.vDataArray[_i - __i][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[_i - __i][_j - __j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i - __i][_j - __j] = NAN;
                                                _region.vValidationArray[_i - __i][_j - __j] = false;
                                            }

                                            if (_i == __i)
                                            {
                                                _region.vDataArray[nOrder + 1][_j - __j] = dMemTable[_i][_j];
                                                _region.vValidationArray[nOrder + 1][_j - __j] = true;
                                            }
                                        }
                                    }

                                    retoqueRegion(_region);

                                    for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                        {
                                            if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i][_j - __j])
                                                dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                        }
                        else if (j == _vCol.front())
                        {
                            while (__i + nOrder + 1 <= _vLine.last() && __i >= _vLine.front() && __j + nOrder + 1 <= _vCol.last()
                                    && num(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 2))
                                    && num(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)) != cnt(VectorIndex(__i, __i + nOrder + 2), VectorIndex(__j + nOrder + 1, __j + nOrder + 2)))
                            {
                                if (__i > _vLine.front())
                                    nOrder += 2;
                                else
                                    nOrder++;

                                if (__i > _vLine.front())
                                    __i--;
                            }

                            if (__i + nOrder + 1 <= _vLine.last() && __i >= _vLine.front() && __j + nOrder + 1 <= _vCol.last())
                            {
                                RetoqueRegion _region;
                                prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                    {
                                        if (!isnan(dMemTable[_i][_j]))
                                        {
                                            _region.vDataArray[_i - __i][_j - __j + 1] = dMemTable[_i][_j];
                                            _region.vValidationArray[_i - __i][_j - __j + 1] = true;
                                        }
                                        else
                                        {
                                            _region.vDataArray[_i - __i][_j - __j + 1] = NAN;
                                            _region.vValidationArray[_i - __i][_j - __j + 1] = false;
                                        }

                                        if (_j == __j + nOrder)
                                        {
                                            _region.vDataArray[_i - __i][0] = dMemTable[_i][_j];
                                            _region.vValidationArray[_i - __i][0] = true;
                                        }
                                    }
                                }

                                retoqueRegion(_region);

                                for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j + nOrder; _j++)
                                    {
                                        if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i][_j - __j + 1])
                                            dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j + 1];
                                    }
                                }
                            }
                            else
                                continue;
                        }
                        else
                        {
                            __j--;

                            while (__i + nOrder + 1 <= _vLine.last() && __i >= _vLine.front() && __j >= _vCol.front()
                                    && num(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 1)) != cnt(VectorIndex(__i, __i + 1), VectorIndex(__j, __j + nOrder + 1))
                                    && num(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + 1)) != cnt(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + 1)))
                            {
                                nOrder++;

                                if (__j > _vCol.front())
                                    __j--;

                                if (__i > _vLine.front())
                                    __i--;
                            }

                            if (__i + nOrder + 1 <= _vLine.last() && __i >= _vLine.front() && __j - nOrder - 1 >= _vCol.front())
                            {
                                RetoqueRegion _region;
                                prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                                for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                    {
                                        if (!isnan(dMemTable[_i][_j]))
                                        {
                                            _region.vDataArray[_i - __i][_j - __j] = dMemTable[_i][_j];
                                            _region.vValidationArray[_i - __i][_j - __j] = true;
                                        }
                                        else
                                        {
                                            _region.vDataArray[_i - __i][_j - __j] = NAN;
                                            _region.vValidationArray[_i - __i][_j - __j] = false;
                                        }

                                        if (_j == __j)
                                        {
                                            _region.vDataArray[_i - __i][nOrder + 1] = dMemTable[_i][_j];
                                            _region.vValidationArray[_i - __i][nOrder + 1] = true;
                                        }
                                    }
                                }

                                retoqueRegion(_region);

                                for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                                    {
                                        if (isnan(dMemTable[_i][_j]) && _region.vValidationArray[_i - __i][_j - __j])
                                            dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
                                    }
                                }
                            }
                            else
                                continue;
                        }
                    }
                    else
                    {
                        unsigned int nOrder = 1;
                        long long int __i = i - 1;
                        long long int __j = j - 1;

                        while (!isValidDisc(__i, __j, nOrder + 1))
                        {
                            for (long long int _i = __i; _i <= __i + nOrder + 1; _i++)
                            {
                                if (isnan(dMemTable[_i][__j]))
                                {
                                    __j--;
                                    break;
                                }

                                if (isnan(dMemTable[_i][__j + nOrder + 1]))
                                {
                                    nOrder++;
                                    break;
                                }
                            }

                            if (__i < _vLine.front() || __i + nOrder + 1 > _vLine.last() || __j < _vCol.front() || __j + nOrder + 1 > _vCol.last())
                                break;

                            for (long long int _j = __j; _j <= __j + nOrder + 1; _j++)
                            {
                                if (isnan(dMemTable[__i][_j]))
                                {
                                    __i--;
                                    break;
                                }

                                if (isnan(dMemTable[__i + nOrder + 1][_j]))
                                {
                                    nOrder++;
                                    break;
                                }
                            }

                            if (__i < _vLine.front() || __i + nOrder + 1 > _vLine.last() || __j < _vCol.front() || __j + nOrder + 1 > _vCol.last())
                                break;
                        }

                        if (__i < _vLine.front() || __i + nOrder + 1 > _vLine.last() || __j < _vCol.front() || __j + nOrder + 1 > _vCol.last())
                            continue;

                        RetoqueRegion _region;
                        prepareRegion(_region, nOrder + 2, med(VectorIndex(__i, __i + nOrder + 1), VectorIndex(__j, __j + nOrder + 1)));

                        for (long long int k = __i; k <= __i + nOrder + 1; k++)
                        {
                            for (long long int l = __j; l <= __j + nOrder + 1; l++)
                            {
                                if (!isnan(dMemTable[k][l]))
                                {
                                    _region.vDataArray[k - __i][l - __j] = dMemTable[k][l];
                                    _region.vValidationArray[k - __i][l - __j] = true;
                                }
                                else
                                {
                                    _region.vDataArray[k - __i][l - __j] = NAN;
                                    _region.vValidationArray[k - __i][l - __j] = false;
                                }
                            }
                        }

                        retoqueRegion(_region);

                        for (long long int k = __i; k <= __i + nOrder + 1; k++)
                        {
                            for (long long int l = __j; l <= __j + nOrder + 1; l++)
                            {
                                if (isnan(dMemTable[k][l]) && _region.vValidationArray[k - __i][l - __j])
                                    dMemTable[k][l] = _region.vDataArray[k - __i][l - __j];
                            }
                        }

                    }

                    if (bIsSaved)
                    {
                        bIsSaved = false;
                        nLastSaved = time(0);
                    }
                }
            }
        }
    }
    else if (Direction == LINES)
    {
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                {
                    for (size_t _j = j; _j < _vCol.size(); _j++)
                    {
                        if (!isnan(dMemTable[_vLine[i]][_vCol[_j]]))
                        {
                            if (j)
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    dMemTable[_vLine[i]][_vCol[__j]] = (dMemTable[_vLine[i]][_vCol[_j]] - dMemTable[_vLine[i]][_vCol[j-1]]) / (double)(_j - j) * (double)(__j - j + 1) + dMemTable[_vLine[i]][_vCol[j-1]];
                                }

                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }

                                break;
                            }
                            else if (!j && _j+1 < _vCol.size())
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    dMemTable[_vLine[i]][_vCol[__j]] = dMemTable[_vLine[i]][_vCol[_j]];
                                }

                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }

                                break;
                            }
                        }

                        if (j && _j+1 == _vCol.size() && isnan(dMemTable[_vLine[i]][_vCol[_j]]))
                        {
                            for (size_t __j = j; __j < _vCol.size(); __j++)
                            {
                                dMemTable[_vLine[i]][_vCol[__j]] = dMemTable[_vLine[i]][_vCol[j - 1]];
                            }

                            if (bIsSaved)
                            {
                                bIsSaved = false;
                                nLastSaved = time(0);
                            }

                            break;
                        }
                    }
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
                {
                    for (size_t _i = i; _i < _vLine.size(); _i++)
                    {
                        if (!isnan(dMemTable[_vLine[_i]][_vCol[j]]))
                        {
                            if (i)
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    dMemTable[_vLine[__i]][_vCol[j]] = (dMemTable[_vLine[_i]][_vCol[j]] - dMemTable[_vLine[i-1]][_vCol[j]]) / (double)(_i - i) * (double)(__i - i + 1) + dMemTable[_vLine[i-1]][_vCol[j]];
                                }

                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }

                                break;
                            }
                            else if (!i && _i+1 < _vLine.size())
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    dMemTable[_vLine[__i]][_vCol[j]] = dMemTable[_vLine[_i]][_vCol[j]];
                                }

                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }

                                break;
                            }
                        }

                        if (i  && _i+1 == _vLine.size() && isnan(dMemTable[_vLine[_i]][_vCol[j]]))
                        {
                            for (size_t __i = i; __i < _vLine.size(); __i++)
                            {
                                dMemTable[_vLine[__i]][_vCol[j]] = dMemTable[_vLine[i-1]][_vCol[j]];
                            }

                            if (bIsSaved)
                            {
                                bIsSaved = false;
                                nLastSaved = time(0);
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool Memory::retoqueRegion(long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nOrder, AppDir Direction)
{
    bool bUseAppendedZeroes = false;
    if (!bValidData)
        return false;
    if (nOrder < 1)
        return false;
    if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
        return false;
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
    {
        i2 = getLines(true) - 1;
        bUseAppendedZeroes = true;
    }
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCols(false) - 1;

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
    if (i1 >= getLines(false) || j1 >= getCols(false))
        return NAN;
    if (i2 >= getLines(false))
        i2 = getLines(false) - 1;
    if (j2 >= getCols(false))
        j2 = getCols(false) - 1;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;
        for (long long int j = j1; j <= j2; j++)
        {
            if (nMax < nLines - nAppendedZeroes[j] - 1)
                nMax = nLines - nAppendedZeroes[j] - 1;
        }
        if (i2 > nMax)
            i2 = nMax;
    }

    if ((Direction == ALL || Direction == GRID) && nOrder > 1)
    {
        if (bUseAppendedZeroes)
        {
            Memory::smooth(VectorIndex(i1, -2), VectorIndex(j1), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), COLS);
            Memory::smooth(VectorIndex(i1, -2), VectorIndex(j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), COLS);
            Memory::smooth(VectorIndex(i1), VectorIndex(j1, j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), LINES);
            Memory::smooth(VectorIndex(i2), VectorIndex(j1, j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), LINES);
        }
        else
        {
            Memory::smooth(VectorIndex(i1, i2), VectorIndex(j1), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), COLS);
            Memory::smooth(VectorIndex(i1, i2), VectorIndex(j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), COLS);
            Memory::smooth(VectorIndex(i1), VectorIndex(j1, j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), LINES);
            Memory::smooth(VectorIndex(i2), VectorIndex(j1, j2), NumeRe::FilterSettings(NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR, nOrder), LINES);
        }
    }

    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << " " << nOrder << endl;
    if (Direction == LINES)
    {
        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1 + 1; j <= j2 - nOrder; j++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dMemTable[i][j - 1]) && !isnan(dMemTable[i][j + nOrder]) && !isnan(dMemTable[i][j + n]))
                        dMemTable[i][j + n] = 0.5 * dMemTable[i][j + n] + 0.5 * (dMemTable[i][j - 1] + (dMemTable[i][j + nOrder] - dMemTable[i][j - 1]) / (double)(nOrder + 1) * (double)(n + 1));
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            for (long long int i = i1 + 1; i <= i2 - nOrder; i++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dMemTable[i - 1][j]) && !isnan(dMemTable[i + nOrder][j]) && !isnan(dMemTable[i + n][j]))
                        dMemTable[i + n][j] = 0.5 * dMemTable[i + n][j] + 0.5 * (dMemTable[i - 1][j] + (dMemTable[i + nOrder][j] - dMemTable[i - 1][j]) / (double)(nOrder + 1) * (double)(n + 1));
                }
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && i2 - i1 > 1 && j2 - j1 > 1)
    {
        for (long long int j = j1; j <= j2 - nOrder - 1; j++)
        {
            for (long long int i = i1; i <= i2 - nOrder - 1; i++)
            {
                for (unsigned int nj = 1; nj <= nOrder; nj++)
                {
                    for (unsigned int ni = 1; ni <= nOrder; ni++) // nOrder-nj+1: Dreieckig glaetten => weniger Glaettungen je Punkt
                    {
                        if (nOrder == 1)
                        {
                            if (!isnan(dMemTable[i + ni][j + nOrder + 1])
                                    && !isnan(dMemTable[i + ni][j])
                                    && !isnan(dMemTable[i + nOrder + 1][j + nj])
                                    && !isnan(dMemTable[i][j + nj]))
                            {
                                dMemTable[i + ni][j + nj] = 0.5 * med(VectorIndex(i1, i2 + 1), VectorIndex(j1, j2 + 1)) + 0.25 * (
                                        dMemTable[i][j + nj] + (dMemTable[i + nOrder + 1][j + nj] - dMemTable[i][j + nj]) / (double)(nOrder + 1) * (double)ni
                                        + dMemTable[i + ni][j] + (dMemTable[i + ni][j + nOrder + 1] - dMemTable[i + ni][j]) / (double)(nOrder + 1) * (double)nj);
                            }
                        }
                        else
                        {
                            if (isValidDisc(i, j, nOrder + 1))
                            {
                                double dAverage = dMemTable[i][j + nj]
                                                  + (dMemTable[i + nOrder + 1][j + nj] - dMemTable[i][j + nj]) / (double)(nOrder + 1) * (double)ni
                                                  + dMemTable[i + ni][j]
                                                  + (dMemTable[i + ni][j + nOrder + 1] - dMemTable[i + ni][j]) / (double)(nOrder + 1) * (double)nj;
                                dAverage *= 2;
                                if (ni >= nj)
                                {
                                    dAverage += dMemTable[i][j + (ni - nj)]
                                                + (dMemTable[i + nOrder + 1 - (ni - nj)][j + nOrder + 1] - dMemTable[i][j + (ni - nj)]) / (double)(nOrder - (ni - nj) + 1) * (double)ni;
                                }
                                else
                                {
                                    dAverage += dMemTable[i][j + (nj - ni)]
                                                + (dMemTable[i + nOrder + 1][j + nOrder + 1 - (nj - ni)] - dMemTable[i + (nj - ni)][j]) / (double)(nOrder - (nj - ni) + 1) * (double)ni;
                                }
                                if (ni + nj <= nOrder + 1)
                                {
                                    dAverage += dMemTable[i + ni + nj][j]
                                                + (dMemTable[i][j + ni + nj] - dMemTable[i + ni + nj][j]) / (double)(ni + nj) * (double)(nj);
                                }
                                else
                                {
                                    dAverage += dMemTable[i + nOrder + 1][j + (ni + nj - nOrder - 1)]
                                                + (dMemTable[i + (ni + nj - nOrder - 1)][j + nOrder + 1] - dMemTable[i + nOrder + 1][j + (ni + nj - nOrder - 1)]) / (double)(2 * nOrder + 2 - (ni + nj)) * (double)(nj - (ni + nj - nOrder - 1));
                                }
                                dAverage /= 6.0;
                                if (!isnan(dMemTable[i + ni][j + nj]))
                                {
                                    if (nOrder % 2)
                                    {
                                        dMemTable[i + ni][j + nj] =
                                                        0.5 * (1.0 - 0.5 * hypot(ni - (nOrder + 1) / 2, nj - (nOrder + 1) / 2) / hypot(1 - (nOrder + 1) / 2, 1 - (nOrder + 1) / 2))
                                                        * dMemTable[i + ni][j + nj]
                                                        + 0.5 * (1.0 + 0.5 * hypot(ni - (nOrder + 1) / 2, nj - (nOrder + 1) / 2) / hypot(1 - (nOrder + 1) / 2, 1 - (nOrder + 1) / 2)) * dAverage;
                                    }
                                    else
                                    {
                                        dMemTable[i + ni][j + nj] =
                                                        0.5 * (1.0 - 0.5 * hypot(ni - (nOrder) / 2, nj - (nOrder) / 2) / hypot(nOrder / 2, nOrder / 2))
                                                        * dMemTable[i + ni][j + nj]
                                                        + 0.5 * (1.0 + 0.5 * hypot(ni - (nOrder) / 2, nj - (nOrder) / 2) / hypot(nOrder / 2, nOrder / 2)) * dAverage;
                                    }
                                }
                                else
                                {
                                    dMemTable[i + ni][j + nj] = dAverage;

                                }
                            }
                        }
                    }
                }
                i += nOrder / 2;
            }
            j += nOrder / 2;
        }
    }
    return true;
}

bool Memory::retoqueRegion(RetoqueRegion& _region)
{
    int nOrder = _region.vDataArray.size() - 2;
    for (unsigned int i = 0; i < _region.vDataArray.size(); i++)
    {
        for (unsigned int j = 0; j < _region.vDataArray.size(); j++)
        {
            if (!_region.vValidationArray[i][j])
            {
                double dAverage = _region.vDataArray[0][j]
                                  + (_region.vDataArray[nOrder + 1][j] - _region.vDataArray[0][j]) / (double)(nOrder + 1) * (double)i
                                  + _region.vDataArray[i][0]
                                  + (_region.vDataArray[i][nOrder + 1] - _region.vDataArray[i][0]) / (double)(nOrder + 1) * (double)j;
                dAverage *= 2.0;
                if (i >= j)
                {
                    dAverage += _region.vDataArray[0][(i - j)]
                                + (_region.vDataArray[nOrder + 1 - (i - j)][nOrder + 1] - _region.vDataArray[0][(i - j)]) / (double)(nOrder - (i - j) + 1) * (double)i;
                }
                else
                {
                    dAverage += _region.vDataArray[0][(j - i)]
                                + (_region.vDataArray[nOrder + 1][nOrder + 1 - (j - i)] - _region.vDataArray[(j - i)][0]) / (double)(nOrder - (j - i) + 1) * (double)i;
                }
                if (i + j <= (unsigned)nOrder + 1)
                {
                    dAverage += _region.vDataArray[i + j][0]
                                + (_region.vDataArray[0][i + j] - _region.vDataArray[i + j][0]) / (double)(i + j) * (double)j;
                }
                else
                {
                    dAverage += _region.vDataArray[nOrder + 1][(i + j - nOrder - 1)]
                                + (_region.vDataArray[(i + j - nOrder - 1)][nOrder + 1] - _region.vDataArray[nOrder + 1][(i + j - nOrder - 1)]) / (double)(2 * nOrder + 2 - (i + j)) * (double)(j - (i + j - nOrder - 1));
                }
                dAverage /= 6.0;

                if (isnan(_region.dMedian))
                    _region.vDataArray[i][j] = dAverage;
                else
                {
                    _region.vDataArray[i][j] =
                                    0.5 * (1.0 - 0.5 * hypot(i - (nOrder) / 2.0, j - (nOrder) / 2.0) / (M_SQRT2 * (nOrder / 2.0)))
                                    * _region.dMedian
                                    + 0.5 * (1.0 + 0.5 * hypot(i - (nOrder) / 2.0, j - (nOrder) / 2.0) / (M_SQRT2 * (nOrder / 2.0))) * dAverage;
                }
                if (!isnan(dAverage) && !isinf(dAverage) && !isnan(_region.vDataArray[i][j]) && !isinf(_region.vDataArray[i][j]))
                    _region.vValidationArray[i][j] = true;
                else
                    _region.vValidationArray[i][j] = false;
            }
        }
    }
    return true;
}

// This member function evaluates the values at the frame of the square described
// by the diagonal (_nLine, _nCol) -> (_nLine+nSize, _nCol+nSize) and ensures that none
// of the values is NaN
bool Memory::isValidDisc(long long int _nLine, long long int _nCol, unsigned int nSize)
{
    // validate the input
    if (_nLine >= Memory::getLines(false) - nSize
            || _nCol >= Memory::getCols(false) - nSize
            || !bValidData)
        return false;

    // Validate along the columns
    for (long long int i = _nLine; i <= _nLine + nSize; i++)
    {
        if (isnan(dMemTable[i][_nCol]) || isnan(dMemTable[i][_nCol + nSize]))
            return false;
    }

    // validate along the rows
    for (long long int j = _nCol; j <= _nCol + nSize; j++)
    {
        if (isnan(dMemTable[_nLine][j]) || isnan(dMemTable[_nLine + nSize][j]))
            return false;
    }

    return true;
}

// This member function evaluates the values at the frame of the square described
// by the diagonal (_nLine, _nCol) -> (_nLine+nSize, _nCol+nSize) and ensures that none
// of the values is NaN
bool Memory::isValidDisc(VectorIndex _vLine, VectorIndex _vCol)
{
    // validate the input
    if (_vLine.max() >= Memory::getLines(false)
            || _vCol.max() >= Memory::getCols(false)
            || !bValidData)
        return false;

    // Validate along the columns
    for (size_t i = 0; i < _vLine.size(); i++)
    {
        if (isnan(readMem(_vLine[i], _vCol.front())) || isnan(readMem(_vLine[i], _vCol.last())))
            return false;
    }

    // validate along the rows
    for (size_t j = 0; j < _vCol.size(); j++)
    {
        if (isnan(readMem(_vLine.front(), _vCol[j])) || isnan(readMem(_vLine.last(), _vCol[j])))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function realizes
/// the application of a smoothing window to 1D
/// data sets.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \param smoothLines bool
/// \return void
///
/////////////////////////////////////////////////
void Memory::smoothingWindow1D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines)
{
    auto sizes = _filter->getWindowSize();

    // Update the boundaries for the weighted linear filter
    if (_filter->getType() == NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR)
        static_cast<NumeRe::WeightedLinearFilter*>(_filter)->setBoundaries(readMem(_vLine.subidx(i-1*(!smoothLines),0), _vCol.subidx(j-1*smoothLines, 0)), readMem(_vLine.subidx(i+(sizes.first+1)*(!smoothLines),0), _vCol.subidx(j+(sizes.first+1)*smoothLines,0)));

    double sum = 0.0;

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        if (!_filter->isConvolution())
            writeData(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines], _filter->apply(n, 0, readMem(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines])));
        else
            sum += _filter->apply(n, 0, readMem(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines]));
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
        writeData(_vLine[i + sizes.first/2*(!smoothLines)], _vCol[j + sizes.first/2*smoothLines], sum);
}


/////////////////////////////////////////////////
/// \brief This private member function realizes
/// the application of a smoothing window to 2D
/// data sets.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \return void
///
/////////////////////////////////////////////////
void Memory::smoothingWindow2D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter)
{
    auto sizes = _filter->getWindowSize();

    // Update the boundaries for the weighted linear filter
    if (_filter->getType() == NumeRe::FilterSettings::FILTER_WEIGHTED_LINEAR)
    {
        static_cast<NumeRe::WeightedLinearFilter*>(_filter)->setBoundaries(readMem(_vLine.subidx(i-1, sizes.first+2), _vCol.subidx(j-1, 0)),
                                                                           readMem(_vLine.subidx(i-1, sizes.first+2), _vCol.subidx(j+sizes.second+1, 0)),
                                                                           readMem(_vLine.subidx(i-1, 0), _vCol.subidx(j-1, sizes.second+2)),
                                                                           readMem(_vLine.subidx(i+sizes.first+1, 0), _vCol.subidx(j-1, sizes.second+2)));
    }

    double sum = 0.0;

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        for (size_t m = 0; m < sizes.second; m++)
        {
            if (!_filter->isConvolution())
                writeData(_vLine[i+n], _vCol[j+m], _filter->apply(n, m, readMem(_vLine[i+n], _vCol[j+m])));
            else
                sum += _filter->apply(n, m, readMem(_vLine[i+n], _vCol[j+m]));
        }
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
        writeData(_vLine[i + sizes.first/2], _vCol[j + sizes.second/2], sum);
}


/////////////////////////////////////////////////
/// \brief This member function smoothes the data
/// described by the passed VectorIndex indices
/// using the passed FilterSettings to construct
/// the corresponding filter.
///
/// \param _vLine VectorIndex
/// \param _vCol VectorIndex
/// \param _settings NumeRe::FilterSettings
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    // Avoid the border cases
    if (!bValidData)
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "smooth", SyntaxError::invalid_position);

    if (!_vLine.isValid() || !_vCol.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "smooth", SyntaxError::invalid_position);

    // Evaluate the indices
    if (_vLine.isOpenEnd())
        bUseAppendedZeroes = true;

    // Force the index ranges
    _vLine.setRange(0, nLines-1);
    _vCol.setRange(0, nCols-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Check the order
    if ((_settings.row >= nLines && Direction == COLS) || (_settings.col >= nCols && Direction == LINES) || ((_settings.row >= nLines || _settings.col >= nCols) && (Direction == ALL || Direction == GRID)))
        throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, "smooth", SyntaxError::invalid_position);

    // Get the appended zeros
    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (nMax < nLines - nAppendedZeroes[_vCol[j]] - 1)
                nMax = nLines - nAppendedZeroes[_vCol[j]] - 1;
        }

        _vLine.setRange(0, nMax);
    }

    // If the application direction is equal to GRID, then the first two columns
    // should be evaluted separately, because they contain the axis values
    if (Direction == GRID)
    {
        // Will never return false
        if (bUseAppendedZeroes)
        {
            if (!smooth(_vLine, VectorIndex(_vCol[0]), _settings, COLS) || !smooth(_vLine, VectorIndex(_vCol[1]), _settings, COLS))
                return false;
        }
        else
        {
            if (!smooth(_vLine, _vCol.subidx(0, 2), _settings, COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    // The first job is to simply remove invalid values and then smooth the
    // framing points of the data section
    if (Direction == ALL || Direction == GRID)
    {
        // Retouch everything
        Memory::retoque(_vLine, _vCol, ALL);

        Memory::smooth(_vLine, VectorIndex(_vCol.front()), _settings, COLS);
        Memory::smooth(_vLine, VectorIndex(_vCol.last()), _settings, COLS);
        Memory::smooth(VectorIndex(_vLine.front()), _vCol, _settings, LINES);
        Memory::smooth(VectorIndex(_vLine.last()), _vCol, _settings, LINES);

        if (_settings.row == 1u && _settings.col != 1u)
            _settings.row = _settings.col;
        else if (_settings.row != 1u && _settings.col == 1u)
            _settings.col = _settings.row;
    }
    else
    {
        _settings.row = std::max(_settings.row, _settings.col);
        _settings.col = 1u;
    }

    if (isnan(_settings.alpha))
        _settings.alpha = 1.0;

    // Apply the actual smoothing of the data
    if (Direction == LINES)
    {
        // Pad the beginning and the of the vector with multiple copies
        _vCol.prepend(vector<long long int>(_settings.row/2+1, _vCol.front()));
        _vCol.append(vector<long long int>(_settings.row/2+1, _vCol.last()));

        // Create a filter from the filter settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Smooth the lines
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            for (size_t j = 1; j < _vCol.size() - _filterPtr.get()->getWindowSize().first-1; j++)
            {
                smoothingWindow1D(_vLine, _vCol, i, j, _filterPtr.get(), true);
            }
        }
    }
    else if (Direction == COLS)
    {
        // Pad the beginning and end of the vector with multiple copies
        _vLine.prepend(vector<long long int>(_settings.row/2+1, _vLine.front()));
        _vLine.append(vector<long long int>(_settings.row/2+1, _vLine.last()));

        // Create a filter from the settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Smooth the columns
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 1; i < _vLine.size() - _filterPtr.get()->getWindowSize().first-1; i++)
            {
                smoothingWindow1D(_vLine, _vCol, i, j, _filterPtr.get(), false);
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && _vLine.size() > 2 && _vCol.size() > 2)
    {
        // Pad the beginning and end of both vectors with multiple copies
        _vLine.prepend(vector<long long int>(_settings.row/2+1, _vLine.front()));
        _vLine.append(vector<long long int>(_settings.row/2+1, _vLine.last()));
        _vCol.prepend(vector<long long int>(_settings.col/2+1, _vCol.front()));
        _vCol.append(vector<long long int>(_settings.col/2+1, _vCol.last()));

        // Create a filter from the settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Smooth the data in two dimensions, if that is reasonable
        // Go through every point
        for (size_t j = 1; j < _vCol.size() - _filterPtr.get()->getWindowSize().second-1; j++)
        {
            for (size_t i = 1; i < _vLine.size() - _filterPtr.get()->getWindowSize().first-1; i++)
            {
                smoothingWindow2D(_vLine, _vCol, i, j, _filterPtr.get());
            }
        }
    }

    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }

    return true;
}


// This member function resamples the data described by the passed coordinates using the new samples nSamples
bool Memory::resample(VectorIndex _vLine, VectorIndex _vCol, unsigned int nSamples, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    const long long int __nOrigLines = nLines;
    const long long int __nOrigCols = nCols;

    long long int __nLines = nLines;
    long long int __nCols = nCols;

    // Avoid border cases
    if (!bValidData)
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "resample", SyntaxError::invalid_position);

    if (!nSamples)
        throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);

    if (!_vLine.isValid() || !_vCol.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "resample", SyntaxError::invalid_position);

    // Evaluate the indices
    if (_vCol.isOpenEnd())
        bUseAppendedZeroes = true;

    _vLine.setRange(0, nLines-1);
    _vLine.linearize();
    _vCol.setRange(0, nCols-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Get the appended zeros
    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (nMax < nLines - nAppendedZeroes[_vCol[j]] - 1)
                nMax = nLines - nAppendedZeroes[_vCol[j]] - 1;
        }

        _vLine.setRange(0, nMax);
    }

    // If the application direction is equal to GRID, then the indices should
    // match a sufficiently enough large data array
    if (Direction == GRID)
    {
        if (_vCol.size() - 2 != _vLine.size() && !bUseAppendedZeroes)
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
        else if (_vCol.size() - 2 != (nLines - nAppendedZeroes[_vCol[1]] - 1) - _vLine.front() && bUseAppendedZeroes)
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
    }

    // Prepare a pointer to the resampler object
    Resampler* _resampler = nullptr;

    // Create the actual resample object based upon the application direction.
    // Additionally determine the size of the resampling buffer, which might
    // be larger than the current data set
    if (Direction == ALL || Direction == GRID) // 2D
    {
        if (Direction == GRID)
        {
            // Apply the resampling to the first two columns first:
            // These contain the axis values
            if (bUseAppendedZeroes)
            {
                resample(_vLine, VectorIndex(_vCol[0]), nSamples, COLS);
                resample(_vLine, VectorIndex(_vCol[1]), nSamples, COLS);
            }
            else
            {
                // Achsenwerte getrennt resamplen
                resample(_vLine, _vCol.subidx(0, 2), nSamples, COLS);
            }

            // Increment the first column
            _vCol = _vCol.subidx(2);
            _vCol.linearize();

            // Determine the size of the buffer
            if (nSamples > _vLine.size())
                __nLines += nSamples - _vLine.size();

            if (nSamples > _vCol.size())
                __nCols += nSamples - _vCol.size();
        }

        if (bUseAppendedZeroes)
        {
            long long int nMax = 0;

            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (nMax < nLines - nAppendedZeroes[_vCol[j]] - 1)
                    nMax = nLines - nAppendedZeroes[_vCol[j]] - 1;
            }

            _vLine.setRange(0, nMax);
        }

        // Create the resample object and prepare the needed memory
        _resampler = new Resampler(_vCol.size(), _vLine.size(), nSamples, nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");

        // Determine final size (only upscale)
        if (nSamples > _vLine.size() || nSamples > _vCol.size())
            resizeMemory(nLines + nSamples - _vLine.size(), nCols + nSamples - _vCol.size());
    }
    else if (Direction == COLS) // cols
    {
        _vCol.linearize();

        // Create the resample object and prepare the needed memory
        _resampler = new Resampler(_vCol.size(), _vLine.size(), _vCol.size(), nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");

        // Determine final size (only upscale)
        if (nSamples > _vLine.size())
            resizeMemory(nLines + nSamples - _vLine.size(), nCols - 1);

        // Determine the size of the buffer
        if (nSamples > _vLine.size())
            __nLines += nSamples - _vLine.size();
    }
    else if (Direction == LINES)// lines
    {
        // Create the resample object and prepare the needed memory
        _resampler = new Resampler(_vCol.size(), _vLine.size(), nSamples, _vLine.size(), Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");

        // Determine final size (only upscale)
        if (nSamples > _vCol.size())
            resizeMemory(nLines - 1, nCols + nSamples - _vCol.size());

        // Determine the size of the buffer
        if (nSamples > _vCol.size())
            __nCols += nSamples - _vCol.size();
    }

    // Ensure that the resampler was created
    if (!_resampler)
    {
        throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
    }

    // Create and initalize the dynamic memory: resampler buffer
    double** dResampleBuffer = new double*[__nLines];

    for (long long int i = 0; i < __nLines; i++)
    {
        dResampleBuffer[i] = new double[__nCols];

        for (long long int j = 0; j < __nCols; j++)
            dResampleBuffer[i][j] = NAN;
    }

    // resampler output buffer
    const double* dOutputSamples = 0;
    double* dInputSamples = new double[_vCol.size()];
    long long int _ret_line = 0;
    long long int _final_cols = 0;

    // Determine the number of final columns. These will stay constant only in
    // the column application direction
    if (Direction == ALL || Direction == GRID || Direction == LINES)
        _final_cols = nSamples;
    else
        _final_cols = _vCol.size();

    // Copy the whole memory
    for (long long int i = 0; i < __nOrigLines; i++)
    {
        for (long long int j = 0; j < __nOrigCols; j++)
        {
            dResampleBuffer[i][j] = dMemTable[i][j];
        }
    }

    // Resample the data table
    // Apply the resampling linewise
    for (size_t i = 0; i < _vLine.size(); i++)
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            dInputSamples[j] = dMemTable[_vLine[i]][_vCol[j]];
        }

        // If the resampler doesn't accept a further line
        // the buffer is probably full
        if (!_resampler->put_line(dInputSamples))
        {
            if (_resampler->status() != Resampler::STATUS_SCAN_BUFFER_FULL)
            {
                // Obviously not the case
                // Clear the memory and return
                delete _resampler;

                for (long long int i = 0; i < __nLines; i++)
                    delete[] dResampleBuffer[i];

                delete[] dResampleBuffer;
                delete[] dInputSamples;

                throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
            }
            else if (_resampler->status() == Resampler::STATUS_SCAN_BUFFER_FULL)
            {
                // Free the scan buffer of the resampler by extracting the already resampled lines
                while (true)
                {
                    dOutputSamples = _resampler->get_line();

                    // dOutputSamples will be a nullptr, if no more resampled
                    // lines are available
                    if (!dOutputSamples)
                        break;

                    for (long long int _fin = 0; _fin < _final_cols; _fin++)
                    {
                        if (isnan(dOutputSamples[_fin]))
                        {
                            dResampleBuffer[_vLine.front() + _ret_line][_vCol.front() + _fin] = NAN;
                            continue;
                        }

                        dResampleBuffer[_vLine.front() + _ret_line][_vCol.front() + _fin] = dOutputSamples[_fin];
                    }

                    _ret_line++;
                }

                // Try again to put the current line
                _resampler->put_line(dInputSamples);
            }
        }
    }

    // Clear the input sample memory
    delete[] dInputSamples;

    // Extract the remaining resampled lines from the resampler's memory
    while (true)
    {
        dOutputSamples = _resampler->get_line();

        // dOutputSamples will be a nullptr, if no more resampled
        // lines are available
        if (!dOutputSamples)
            break;

        for (long long int _fin = 0; _fin < _final_cols; _fin++)
        {
            if (isnan(dOutputSamples[_fin]))
            {
                dResampleBuffer[_vLine.front() + _ret_line][_vCol.front() + _fin] = NAN;
                continue;
            }

            dResampleBuffer[_vLine.front() + _ret_line][_vCol.front() + _fin] = dOutputSamples[_fin];
        }

        _ret_line++;
    }

    //_ret_line++;

    // Delete the resampler: it is not used any more
    delete _resampler;

    // Block unter dem resampleten kopieren
    if (_vLine.size() < nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
    {
        for (long long int i = _vLine.last() + 1; i < __nOrigLines; i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (_ret_line + i - (_vLine.last() + 1) + _vLine.front() >= nLines)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dResampleBuffer[i];

                    delete[] dResampleBuffer;

                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                }

                if (isnan(dMemTable[i][_vCol[j]]))
                {
                    dResampleBuffer[_ret_line + i - (_vLine.last() + 1) + _vLine.front()][_vCol[j]] = NAN;
                }
                else
                {
                    dResampleBuffer[_ret_line + i - (_vLine.last() + 1) + _vLine.front()][_vCol[j]] = dMemTable[i][_vCol[j]];
                }
            }
        }
    }
    else if (_vLine.size() > nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
    {
        for (size_t i = nSamples - 1; i < _vLine.size(); i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (_vLine[i] >= nLines)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dResampleBuffer[i];

                    delete[] dResampleBuffer;

                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                }

                dResampleBuffer[_vLine[i]][_vCol[j]] = NAN;
            }
        }
    }

    // Block rechts kopieren
    if (_vCol.size() < nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
    {
        for (long long int i = 0; i < __nOrigLines; i++)
        {
            for (long long int j = _vCol.last() + 1; j < __nOrigCols; j++)
            {
                if (_final_cols + j - (_vCol.last() + 1) + _vCol.front() >= nCols)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dResampleBuffer[i];

                    delete[] dResampleBuffer;

                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                }

                if (isnan(dMemTable[i][j]))
                {
                    dResampleBuffer[i][_final_cols + j - (_vCol.last() + 1) + _vCol.front()] = NAN;
                }
                else
                {
                    dResampleBuffer[i][_final_cols + j - (_vCol.last() + 1) + _vCol.front()] = dMemTable[i][j];
                }
            }
        }
    }
    else if (_vCol.size() > nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
    {
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            for (size_t j = nSamples - 1; j < _vCol.size(); j++)
            {
                if (_vCol[j] >= nCols)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dResampleBuffer[i];

                    delete[] dResampleBuffer;

                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                }

                dResampleBuffer[_vLine[i]][_vCol[j]] = NAN;
            }
        }
    }

    // After all data is restored successfully
    // copy the data points from the buffer back to their original state
    for (long long int i = 0; i < nLines; i++)
    {
        if (i >= __nLines)
            break;

        for (long long int j = 0; j < nCols; j++)
        {
            if (j >= __nCols)
                break;

            dMemTable[i][j] = dResampleBuffer[i][j];
        }
    }


    // appended zeroes zaehlen
    for (long long int j = 0; j < nCols; j++)
    {
        for (long long int i = nLines; i >= 0; i--)
        {
            if (i == nLines)
                nAppendedZeroes[j] = 0;
            else if (isnan(dMemTable[i][j]))
                nAppendedZeroes[j]++;
            else
                break;

        }
    }

    // Clear unused memory
    for (long long int i = 0; i < __nLines; i++)
        delete[] dResampleBuffer[i];

    delete[] dResampleBuffer;

    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }

    return true;
}


