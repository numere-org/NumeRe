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
	}
	if (sHeadLine)
	{
		delete[] sHeadLine;
	}
	if (nAppendedZeroes)
	{
		delete[] nAppendedZeroes;
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

vector<double> Memory::readMem(const vector<long long int>& _vLine, const vector<long long int>& _vCol) const
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

void Memory::copyElementsInto(vector<double>* vTarget, const vector<long long int>& _vLine, const vector<long long int>& _vCol) const
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

vector<string> Memory::getHeadLineElement(vector<long long int> _vCol) const
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


// --> Schreibt einen Wert an beliebiger Stelle in den Memory <--
bool Memory::writeData(long long int _nLine, long long int _nCol, double _dData)
{
	if (dMemTable && (_nLine < nLines) && (_nCol < nCols))
	{
		if (isinf(_dData) || isnan(_dData))
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
	else if (!dMemTable && (isinf(_dData) || isnan(_dData)))
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

		if (isinf(_dData) || isnan(_dData))
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
		if (!isinf(_dData) && !isnan(_dData) && !bValidData)
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

	if (_idx.vI.size())
	{
		while (_idx.nI[1] == -2 && _idx.vI.size() < _nNum)
		{
			_idx.vI.push_back(_idx.vI.back() + 1);
		}
		while (_idx.nJ[1] == -2 && _idx.vJ.size() < _nNum)
		{
			_idx.vJ.push_back(_idx.vJ.back() + 1);
		}

		if (_idx.nI[1] != -1)
			nDirection = COLS;
		else if (_idx.nJ[1] != -1)
			nDirection = LINES;
		else if (_idx.vI.size() == _nNum && _idx.vJ.size() != _nNum)
			nDirection = COLS;
		else if (_idx.vJ.size() == _nNum && _idx.vI.size() != _nNum)
			nDirection = LINES;
		else if (_idx.vI.size() > _idx.vJ.size())
			nDirection = COLS;
		else
			nDirection = LINES;

		for (size_t i = 0; i < _idx.vI.size(); i++)
		{
			for (size_t j = 0; j < _idx.vJ.size(); j++)
			{
				if (nDirection == COLS)
				{
					if (_nNum > i && !isnan(_idx.vI[i]) && !isnan(_idx.vJ[j]))
						writeData(_idx.vI[i], _idx.vJ[j], _dData[i]);
				}
				else
				{
					if (_nNum > j && !isnan(_idx.vI[i]) && !isnan(_idx.vJ[j]))
						writeData(_idx.vI[i], _idx.vJ[j], _dData[j]);
				}
			}
		}
	}
	else
	{
		if (_idx.nI[1] == -2)
			_idx.nI[1] = _idx.nI[0] + _nNum - 1;
		if (_idx.nJ[1] == -2)
			_idx.nJ[1] = _idx.nJ[0] + _nNum - 1;

		if (_idx.nI[1] - _idx.nI[0] == _nNum - 1 && _idx.nJ[1] - _idx.nJ[0] != _nNum - 1)
			nDirection = COLS;
		else if (_idx.nI[1] - _idx.nI[0] != _nNum - 1 && _idx.nJ[1] - _idx.nJ[0] == _nNum - 1)
			nDirection = LINES;
		else if (_idx.nI[1] - _idx.nI[0] > _idx.nJ[1] - _idx.nJ[0])
			nDirection = COLS;
		else
			nDirection = LINES;

		for (int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
		{
			for (int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
			{
				if (nDirection == COLS)
				{
					if (_nNum > i - _idx.nI[0])
						writeData(i, j, _dData[i - _idx.nI[0]]);
				}
				else
				{
					if (_nNum > j - _idx.nJ[0])
						writeData(i, j, _dData[j - _idx.nJ[0]]);
				}
			}
		}
	}

	return true;
}

bool Memory::writeSingletonData(Indices& _idx, double* _dData)
{
	if (_idx.vI.size())
	{
		while (_idx.nI[1] == -2 && _idx.vI.back() < ::max(_idx.nI[0], getLines(false) - 1))
		{
			_idx.vI.push_back(_idx.vI.back() + 1);
		}
		while (_idx.nJ[1] == -2 && _idx.vJ.back() < ::max(_idx.nJ[0], getCols(false) - 1))
		{
			_idx.vJ.push_back(_idx.vJ.back() + 1);
		}
		for (size_t i = 0; i < _idx.vI.size(); i++)
		{
			for (size_t j = 0; j < _idx.vJ.size(); j++)
			{
				if (!isnan(_idx.vI[i]) && !isnan(_idx.vJ[j]))
					writeData(_idx.vI[i], _idx.vJ[j], _dData[0]);
			}
		}
	}
	else
	{
		if (_idx.nI[1] == -2)
			_idx.nI[1] = ::max(_idx.nI[0], getLines(false) - 1);
		if (_idx.nJ[1] == -2)
			_idx.nJ[1] = ::max(_idx.nJ[0], getCols(false) - 1);


		for (int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
		{
			for (int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
			{
				writeData(i, j, _dData[0]);
			}
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


bool Memory::qSortWrapper(int* nIndex, int nElements, int nKey, long long int nLeft, long long int nRight, int nSign)
{
	if (!nIndex || !nElements || nLeft < 0 || nRight > nElements || nRight < nLeft)
	{
		return false;
	}
	while (nRight >= nLeft && isnan(dMemTable[nIndex[nRight]][nKey]))
	{
		nRight--;
	}
	if (nRight < 0)
		return false;
	// swap all NaNs to the right
	int nPos = nRight;
	while (nPos >= nLeft)
	{
		if (isnan(dMemTable[nIndex[nPos]][nKey]))
		{
			int nTemp = nIndex[nPos];
			nIndex[nPos] = nIndex[nRight];
			nIndex[nRight] = nTemp;
			nRight--;
		}
		nPos--;
	}
	return qSort(nIndex, nElements, nKey, nLeft, nRight, nSign);
}

bool Memory::qSort(int* nIndex, int nElements, int nKey, long long int nLeft, long long int nRight, int nSign)
{
	//cerr << nLeft << "/" << nRight << endl;
	if (!nIndex || !nElements || nLeft < 0 || nRight > nElements || nRight < nLeft)
	{
		return false;
	}
	if (nRight == nLeft)
		return true;
	if (nRight - nLeft <= 1 && (nSign * dMemTable[nIndex[nLeft]][nKey] <= nSign * dMemTable[nIndex[nRight]][nKey] || isnan(dMemTable[nIndex[nRight]][nKey])))
		return true;
	else if (nRight - nLeft <= 1 && (nSign * dMemTable[nIndex[nRight]][nKey] <= nSign * dMemTable[nIndex[nLeft]][nKey] || isnan(dMemTable[nIndex[nLeft]][nKey])))
	{
		int nTemp = nIndex[nLeft];
		nIndex[nLeft] = nIndex[nRight];
		nIndex[nRight] = nTemp;
		return true;
	}

	double nPivot = nSign * dMemTable[nIndex[nRight]][nKey];
	int i = nLeft;
	int j = nRight - 1;
	do
	{
		while ((nSign * dMemTable[nIndex[i]][nKey] <= nPivot && !isnan(dMemTable[nIndex[i]][nKey])) && i < nRight)
			i++;
		while ((nSign * dMemTable[nIndex[j]][nKey] >= nPivot || isnan(dMemTable[nIndex[j]][nKey])) && j > nLeft)
			j--;
		if (i < j)
		{
			int nTemp = nIndex[i];
			nIndex[i] = nIndex[j];
			nIndex[j] = nTemp;
		}
	}
	while (i < j);

	if (nSign * dMemTable[nIndex[i]][nKey] > nPivot || isnan(dMemTable[nIndex[i]][nKey]))
	{
		int nTemp = nIndex[i];
		nIndex[i] = nIndex[nRight];
		nIndex[nRight] = nTemp;
	}
	while (isnan(dMemTable[nIndex[nRight - 1]][nKey]) && !isnan(dMemTable[nIndex[nRight]][nKey]))
	{
		int nTemp = nIndex[nRight - 1];
		nIndex[nRight - 1] = nIndex[nRight];
		nIndex[nRight] = nTemp;
		nRight--;
	}
	//cerr << nLeft << "/" << i << endl;
	if (i > nLeft)
	{
		if (!qSort(nIndex, nElements, nKey, nLeft, i - 1, nSign))
			return false;
	}
	if (i < nRight)
	{
		if (!qSort(nIndex, nElements, nKey, i + 1, nRight, nSign))
			return false;
	}
	return true;
}

vector<int> Memory::sortElements(long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
	if (!dMemTable)
		return vector<int>();
	bool bError = false;
	bool bReturnIndex = false;
	int nSign = 1;

	vector<int> vIndex;

	if (matchParams(sSortingExpression, "desc"))
		nSign = -1;

	if (!Memory::getCols(false))
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
			if (!qSortWrapper(&vIndex[0], i2 - i1 + 1, i, 0, i2 - i1, nSign))
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
		if (matchParams(sSortingExpression, "cols", '='))
		{
			sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "cols", '=') + 4);
		}
		else
		{
			sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "c", '=') + 1);
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
				if (!qSortWrapper(&vIndex[0], i2 - i1 + 1, j + j1, 0, i2 - i1, nSign))
				{
					delete keys;
					throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
				}
				// Subkey list
				if (keys->subkeys && keys->subkeys->subkeys)
				{
					if (!sortSubList(vIndex, keys, i1, i2, j1, nSign))
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

bool Memory::sortSubList(vector<int>& vIndex, ColumnKeys* KeyList, long long int i1, long long int i2, long long int j1, int nSign)
{
	ColumnKeys* subKeyList = KeyList->subkeys;
	int nTopColumn = KeyList->nKey[0];
	size_t nStart = i1;

	if (subKeyList && subKeyList->subkeys && j1 + nTopColumn < getCols(false))
	{
		for (size_t k = i1 + 1; k <= i2 && k < vIndex.size(); k++)
		{
			if (dMemTable[vIndex[k]][j1 + nTopColumn] != dMemTable[vIndex[nStart]][j1 + nTopColumn])
			{
				if (k > nStart + 1)
				{
					if (!qSortWrapper(&vIndex[0], vIndex.size(), j1 + subKeyList->nKey[0], nStart, k - 1, nSign))
					{
						return false;
					}
					if (!sortSubList(vIndex, subKeyList, nStart, k - 1, j1, nSign))
						return false;
				}
				nStart = k;
			}
		}
	}
	return true;
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
// cols=1[2:3]4[5:9]10:
ColumnKeys* Memory::evaluateKeyList(string& sKeyList, long long int nMax)
{
	ColumnKeys* keys = new ColumnKeys();
	if (sKeyList.find(':') == string::npos && sKeyList.find('[') == string::npos)
	{
		keys->nKey[0] = StrToInt(sKeyList) - 1;
		sKeyList.clear();
	}
	else
	{
		unsigned int nLastIndex = 0;
		for (unsigned int n = 0; n < sKeyList.length(); n++)
		{
			if (sKeyList[n] == ':')
			{
				if (n != nLastIndex)
					keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n - nLastIndex)) - 1;

				if (n + 1 == sKeyList.length())
					keys->nKey[1] = nMax;

				for (size_t i = n + 1; i < sKeyList.length(); i++)
				{
					if (sKeyList[i] == '[' || sKeyList[i] == ':' || sKeyList[i] == ',')
					{
						keys->nKey[1] = StrToInt(sKeyList.substr(n + 1, i - n - 1));
						sKeyList.erase(0, i + 1);
						break;
					}
					else if (i + 1 == sKeyList.length())
					{
						if (i == n + 1)
							keys->nKey[1] = nMax;
						else
							keys->nKey[1] = StrToInt(sKeyList.substr(n + 1));
						sKeyList.clear();
						break;
					}
				}

				break;
			}
			else if (sKeyList[n] == '[' && sKeyList.find(']', n) != string::npos)
			{
				keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n - nLastIndex)) - 1;
				string sColArray;

				size_t i = getMatchingParenthesis(sKeyList.substr(n));
				if (i != string::npos)
				{
					sColArray = sKeyList.substr(n + 1, i - 1);
					sKeyList.erase(0, i + n + 1);
				}

				keys->subkeys = evaluateKeyList(sColArray, nMax);

				break;
			}
			else if (sKeyList[n] == '[')
			{
				delete keys;
				return nullptr;
			}
		}
	}
	return keys;
}


Table Memory::extractTable(const string& _sTable)
{
	Table _table;

	_table.setName(_sTable);
	_table.setSize(this->getLines(false), this->getCols(false));

	for (long long int i = 0; i < this->getLines(false); i++)
	{
		for (long long int j = 0; j < this->getCols(false); j++)
		{
			if (!i)
				_table.setHead(j, sHeadLine[j]);
			_table.setValue(i, j, dMemTable[i][j]);
		}
	}

	return _table;
}


bool Memory::save(string _sFileName)
{
	ofstream file_out;
	if (file_out.is_open())
		file_out.close();
	file_out.open(_sFileName.c_str(), ios_base::binary | ios_base::trunc | ios_base::out);

	if (file_out.is_open() && file_out.good() && bValidData)
	{
		char** cHeadLine = new char* [nCols];
		long int nMajor = AutoVersion::MAJOR;
		long int nMinor = AutoVersion::MINOR;
		long int nBuild = AutoVersion::BUILD;
		long long int lines = getLines(false);
		long long int cols = getCols(false);
		long long int appendedzeroes[cols];
		double dDataValues[cols];
		bool bValidValues[cols];
		for (long long int i = 0; i < cols; i++)
		{
			appendedzeroes[i] = nAppendedZeroes[i] - (nLines - lines);
		}

		for (long long int i = 0; i < cols; i++)
		{
			cHeadLine[i] = new char[sHeadLine[i].length() + 1];
			for (unsigned int j = 0; j < sHeadLine[i].length(); j++)
			{
				cHeadLine[i][j] = sHeadLine[i][j];
			}
			cHeadLine[i][sHeadLine[i].length()] = '\0';
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
			size_t nlength = sHeadLine[i].length() + 1;
			//cerr << nlength << endl;
			file_out.write((char*)&nlength, sizeof(size_t));
			file_out.write(cHeadLine[i], sizeof(char)*sHeadLine[i].length() + 1);
		}
		file_out.write((char*)appendedzeroes, sizeof(long long int)*cols);

		for (long long int i = 0; i < lines; i++)
		{
			for (long long int j = 0; j < cols; j++)
				dDataValues[j] = dMemTable[i][j];
			file_out.write((char*)dDataValues, sizeof(double)*cols);
		}
		for (long long int i = 0; i < lines; i++)
		{
			for (long long int j = 0; j < cols; j++)
				bValidValues[j] = !isnan(dMemTable[i][j]);
			file_out.write((char*)bValidValues, sizeof(bool)*cols);
		}
		file_out.close();
		for (long long int i = 0; i < cols; i++)
		{
			delete[] cHeadLine[i];
		}
		delete[] cHeadLine;

		return true;
	}
	else
	{
		file_out.close();
		return false;
	}

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

void Memory::deleteBulk(long long int i1, long long int i2, long long int j1, long long int j2)
{
	//cerr << i1 << " " << i2 << " " << j1 << " " << j2 << endl;
	if (!Memory::getCols(false))
		return;
	if (i2 == -1)
		i2 = i1;
	if (j2 == -1)
		j2 = j1;
	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			dMemTable[i][j] = NAN;
		}
	}
	if (bIsSaved)
	{
		bIsSaved = false;
		nLastSaved = time(0);
	}
	for (long long int j = nCols - 1; j >= 0; j--)
	{
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
				if (!i1 && j1 <= j && j <= j2)
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

void Memory::deleteBulk(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
	bool bHasFirstLine = false;
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


double Memory::std(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dMean = 0.0;
	double dStd = 0.0;
	long long int nInvalid = 0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
			{
				nInvalid++;
				continue;
			}
			dMean += dMemTable[i][j];
		}
	}
	dMean /= (double)((i2 - i1 + 1) * (j2 - j1 + 1) - nInvalid);

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			dStd += (dMean - dMemTable[i][j]) * (dMean - dMemTable[i][j]);
		}
	}
	dStd /= (double)((i2 - i1 + 1) * (j2 - j1 + 1) - 1 - nInvalid);
	dStd = sqrt(dStd);
	return dStd;
}

double Memory::std(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::avg(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dMean = 0.0;
	long long int nInvalid = 0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
			{
				nInvalid++;
				continue;
			}
			dMean += dMemTable[i][j];
		}
	}
	dMean /= (double)((i2 - i1 + 1) * (j2 - j1 + 1) - nInvalid);
	return dMean;
}

double Memory::avg(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::max(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dMax = 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			if (i == i1 && j == j1)
				dMax = dMemTable[i][j];
			else if (dMemTable[i][j] > dMax)
				dMax = dMemTable[i][j];
			else
				continue;
		}
	}
	return dMax;
}

double Memory::max(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::min(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dMin = 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			if (i == i1 && j == j1)
				dMin = dMemTable[i][j];
			else if (dMemTable[i][j] < dMin)
				dMin = dMemTable[i][j];
			else
				continue;
		}
	}
	return dMin;
}

double Memory::min(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::prd(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dPrd = 1.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			dPrd *= dMemTable[i][j];
		}
	}
	return dPrd;

}

double Memory::prd(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
	if (!bValidData)
		return NAN;
	double dPrd = 0.0;

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


double Memory::sum(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dSum = 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			dSum += dMemTable[i][j];
		}
	}
	return dSum;
}

double Memory::sum(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::num(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return 0;
	int nInvalid = 0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				nInvalid++;
		}
	}
	return (double)((i2 - i1 + 1) * (j2 - j1 + 1) - nInvalid);
}

double Memory::num(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::and_func(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	double dRetVal = NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dRetVal))
				dRetVal = 1.0;
			if (isnan(dMemTable[i][j]) || dMemTable[i][j] == 0)
				return 0.0;
		}
	}
	if (isnan(dRetVal))
		return 0.0;
	return 1.0;
}

double Memory::and_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::or_func(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (!isnan(dMemTable[i][j]) && dMemTable[i][j] != 0.0)
				return 1.0;
		}
	}
	return 0.0;
}

double Memory::or_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::xor_func(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	bool isTrue = false;
	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (!isnan(dMemTable[i][j]) && dMemTable[i][j] != 0.0)
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

double Memory::xor_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::cnt(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return 0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	if (j2 >= nCols)
		j2 = nCols - 1;
	if (i2 >= nLines - getAppendedZeroes(j1))
		i2 = nLines - 1 - getAppendedZeroes(j1);

	return (double)((i2 - i1 + 1) * (j2 - j1 + 1));
}

double Memory::cnt(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::norm(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	double dNorm = 0.0;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			dNorm += dMemTable[i][j] * dMemTable[i][j];
		}
	}
	return sqrt(dNorm);
}

double Memory::norm(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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


double Memory::cmp(long long int i1, long long int i2, long long int j1, long long int j2, double dRef, int nType)
{
	if (!bValidData)
		return NAN;
	double dKeep = 0.0;
	int nKeep = -1;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (isnan(dMemTable[i][j]))
				continue;
			if (dMemTable[i][j] == dRef)
			{
				if (abs(nType) <= 1)
				{
					if (i1 == i2)
						return j + 1;
					else
						return i + 1;
				}
				else
					return dMemTable[i][j];
			}
			else if ((nType == 1 || nType == 2) && dMemTable[i][j] > dRef)
			{
				if (nKeep == -1 || dMemTable[i][j] < dKeep)
				{
					dKeep = dMemTable[i][j];
					if (i1 == i2)
						nKeep = j;
					else
						nKeep = i;
				}
				else
					continue;
			}
			else if ((nType == -1 || nType == -2) && dMemTable[i][j] < dRef)
			{
				if (nKeep == -1 || dMemTable[i][j] > dKeep)
				{
					dKeep = dMemTable[i][j];
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
		return nKeep + 1;
}

double Memory::cmp(const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef, int nType)
{
	if (!bValidData)
		return NAN;
	double dKeep = 0.0;
	int nKeep = -1;

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
				if (abs(nType) <= 1)
				{
					if (_vLine[0] == _vLine[_vLine.size() - 1])
						return _vCol[j] + 1;
					else
						return _vLine[i] + 1;
				}
				else
					return dMemTable[_vLine[i]][_vCol[j]];
			}
			else if ((nType == 1 || nType == 2) && dMemTable[_vLine[i]][_vCol[j]] > dRef)
			{
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
			else if ((nType == -1 || nType == -2) && dMemTable[_vLine[i]][_vCol[j]] < dRef)
			{
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
	else if (abs(nType) == 2)
		return dKeep;
	else
		return nKeep + 1;
}


double Memory::med(long long int i1, long long int i2, long long int j1, long long int j2)
{
	if (!bValidData)
		return NAN;
	Memory _cache;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (i1 != i2 && j1 != j2)
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData((j - j1) + (i - i1) * (j2 - j1 + 1), 0, dMemTable[i][j]);
			}
			else if (i1 != i2)
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData(i - i1, j - j1, dMemTable[i][j]);
			}
			else
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData(j - j1, i - i1, dMemTable[i][j]);
			}
		}
	}

	_cache.sortElements(0, _cache.getLines(false) - 1, 0, _cache.getCols(false) - 1, "");

	if (_cache.getLines(false) % 2)
	{
		return _cache.readMem(_cache.getLines(false) / 2, 0);
	}
	else
	{
		return (_cache.readMem(_cache.getLines(false) / 2, 0) + _cache.readMem(_cache.getLines(false) / 2 - 1, 0)) / 2.0;
	}
}

double Memory::med(const vector<long long int>& _vLine, const vector<long long int>& _vCol)
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
			if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
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


double Memory::pct(long long int i1, long long int i2, long long int j1, long long int j2, double dPct)
{
	if (!bValidData)
		return NAN;
	if (dPct >= 1 || dPct <= 0)
		return NAN;
	Memory _cache;

	if (!evaluateIndices(i1, i2, j1, j2))
		return NAN;

	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			if (i1 != i2 && j1 != j2)
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData((j - j1) + (i - i1) * (j2 - j1 + 1), 0, dMemTable[i][j]);
			}
			else if (i1 != i2)
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData(i - i1, j - j1, dMemTable[i][j]);
			}
			else
			{
				if (!isnan(dMemTable[i][j]))
					_cache.writeData(j - j1, i - i1, dMemTable[i][j]);
			}
		}
	}

	_cache.sortElements(0, _cache.getLines(false) - 1, 0, _cache.getCols(false) - 1, "");

	return (1 - ((_cache.getLines(false) - 1) * dPct - floor((_cache.getLines(false) - 1) * dPct))) * _cache.readMem(floor((_cache.getLines(false) - 1) * dPct), 0)
		   + ((_cache.getLines(false) - 1) * dPct - floor((_cache.getLines(false) - 1) * dPct)) * _cache.readMem(floor((_cache.getLines(false) - 1) * dPct) + 1, 0);
}

double Memory::pct(const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct)
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
			if (isnan(dMemTable[_vLine[i]][_vCol[j]]))
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



bool Memory::retoque(long long int i1, long long int i2, long long int j1, long long int j2, AppDir Direction)
{
	bool bUseAppendedZeroes = false;
	if (!bValidData)
		return false;
	if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
		return false;

	// Evaluate the indices
    if (i2 == -2)
        bUseAppendedZeroes = true;
    if (!evaluateIndices(i1, i2, j1, j2))
        return false;

	if ((Direction == ALL || Direction == GRID) && i2 - i1 < 3)
		Direction = LINES;
	if ((Direction == ALL || Direction == GRID) && j2 - j1 < 3)
		Direction = COLS;

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


	if (Direction == GRID)
	{
		if (bUseAppendedZeroes)
		{
			if (!retoque(i1, -2, j1, -1, COLS) || !retoque(i1, -2, j1 + 1, -1, COLS))
				return false;
		}
		else
		{
			if (!retoque(i1, i2, j1, j1 + 1, COLS))
				return false;
		}
		j1 += 2;
	}

	if (Direction == ALL || Direction == GRID)
	{
		for (long long int i = i1; i <= i2; i++)
		{
			for (long long int j = j1; j <= j2; j++)
			{
				if (isnan(dMemTable[i][j]))
				{
					if (i > i1 && i < i2 && j > j1 && j < j2 && isValidDisc(i - 1, j - 1, 2))
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

		for (long long int i = i1; i <= i2; i++)
		{
			for (long long int j = j1; j <= j2; j++)
			{
				if (isnan(dMemTable[i][j]))
				{
					if (i > i1 && i < i2 && j > j1 && j < j2 && isValidDisc(i - 1, j - 1, 2))
					{
						retoqueRegion(i - 1, i + 1, j - 1, j + 1, 1, ALL);
					}
					else if (i == i1 || i == i2 || j == j1 || j == j2)
					{
						unsigned int nOrder = 1;
						long long int __i = i;
						long long int __j = j;
						if (i == i1)
						{
							if (j == j1)
							{
								while (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2
										&& num(i + nOrder + 1, i + nOrder + 2, j, j + nOrder + 1) != cnt(i + nOrder + 1, i + nOrder + 2, j, j + nOrder + 1)
										&& num(i, i + nOrder + 2, j + nOrder + 1, j + nOrder + 2) != cnt(i, i + nOrder + 2, j + nOrder + 1, j + nOrder + 2))
								{
									nOrder++;
								}
								if (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j + 1];
											}
										}
									}
								}
								else
									continue;
							}
							else if (j == j2)
							{
								__j--;
								while (__i + nOrder + 1 <= i2 && __j >= j1 && __j + nOrder + 1 < j2
										&& num(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 1) != cnt(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 1)
										&& num(__i, __i + nOrder + 2, __j, __j + 1) != cnt(__i, __i + nOrder + 2, __j, __j + 1))
								{
									nOrder++;
									if (__j > j1)
										__j--;
								}
								if (__i + nOrder + 1 <= i2 && __j >= j1 && __j + nOrder + 1 < j2)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j];
											}
										}
									}
								}
								else
									continue;
							}
							else
							{
								while (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2 && __j >= j1
										&& num(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 2) != cnt(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 2)
										&& num(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2) != cnt(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2))
								{
									if (__j > j1)
										nOrder += 2;
									else
										nOrder++;
									if (__j > j1)
										__j--;
								}
								if (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2 && __j >= j1)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));

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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i + 1][_j - __j];
											}
										}
									}
								}
								else
									continue;
							}
						}
						else if (i == i2)
						{
							__i--;
							if (j == j2)
							{
								__j--;
								while (__i >= i1 && __j >= j1
										&& num(__i, __i + 1, __j, __j + nOrder + 1) != cnt(__i, __i + 1, __j, __j + nOrder + 1)
										&& num(__i, __i + nOrder + 1, __j, __j + 1) != cnt(__i, __i + nOrder + 1, __j, __j + 1))
								{
									if (__j > j1)
										__j--;
									if (__i > i1)
										__i--;
									nOrder++;
								}
								if (__i >= i1 && __j >= j1)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));

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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
											}
										}
									}
								}
								else
									continue;
							}
							else if (j == j1)
							{
								while (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2
										&& num(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 2) != cnt(__i + nOrder + 1, __i + nOrder + 2, __j, __j + nOrder + 2)
										&& num(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2) != cnt(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2))
								{
									if (__i > i1)
										__i--;
									nOrder++;
								}
								if (__i + nOrder + 1 <= i2 && __j + nOrder + 1 <= j2)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j + 1];
											}
										}
									}
								}
								else
									continue;
							}
							else
							{
								while (__i >= i1 && __j + nOrder + 1 <= j2
										&& num(__i, __i + 1, __j, __j + nOrder + 2) != cnt(__i, __i + 1, __j, __j + nOrder + 2)
										&& num(__i, __i + nOrder + 1, __j + nOrder + 1, __j + nOrder + 2) != cnt(__i, __i + nOrder + 1, __j + nOrder + 1, __j + nOrder + 2))
								{
									nOrder++;
									if (__j > j1)
										__j--;
									if (__i > i1)
										__i--;
								}
								if (__i >= i1 && __j + nOrder + 1 <= j2)
								{
									RetoqueRegion _region;
									prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder));
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
											{
												dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
											}
										}
									}
								}
								else
									continue;
							}
						}
						else if (j == j1)
						{
							while (__i + nOrder + 1 <= i2 && __i >= i1 && __j + nOrder + 1 <= j2
									&& num(__i, __i + 1, __j, __j + nOrder + 2) != cnt(__i, __i + 1, __j, __j + nOrder + 2)
									&& num(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2) != cnt(__i, __i + nOrder + 2, __j + nOrder + 1, __j + nOrder + 2))
							{
								if (__i > i1)
									nOrder += 2;
								else
									nOrder++;
								if (__i > i1)
									__i--;
							}
							if (__i + nOrder + 1 <= i2 && __i >= i1 && __j + nOrder + 1 <= j2)
							{
								RetoqueRegion _region;
								prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
										{
											dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j + 1];
										}
									}
								}
							}
							else
								continue;
						}
						else
						{
							__j--;
							while (__i + nOrder + 1 <= i2 && __i >= i1 && __j >= j1
									&& num(__i, __i + 1, __j, __j + nOrder + 1) != cnt(__i, __i + 1, __j, __j + nOrder + 1)
									&& num(__i, __i + nOrder + 1, __j, __j + 1) != cnt(__i, __i + nOrder + 1, __j, __j + 1))
							{
								nOrder++;
								if (__j > j1)
									__j--;
								if (__i > i1)
									__i--;
							}
							if (__i + nOrder + 1 <= i2 && __i >= i1 && __j - nOrder - 1 >= j1)
							{
								RetoqueRegion _region;
								prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
										{
											dMemTable[_i][_j] = _region.vDataArray[_i - __i][_j - __j];
										}
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
							if (__i < i1 || __i + nOrder + 1 > i2 || __j < j1 || __j + nOrder + 1 > j2)
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
							if (__i < i1 || __i + nOrder + 1 > i2 || __j < j1 || __j + nOrder + 1 > j2)
								break;
						}
						if (__i < i1 || __i + nOrder + 1 > i2 || __j < j1 || __j + nOrder + 1 > j2)
							continue;
						RetoqueRegion _region;
						prepareRegion(_region, nOrder + 2, med(__i, __i + nOrder + 1, __j, __j + nOrder + 1));
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
								{
									dMemTable[k][l] = _region.vDataArray[k - __i][l - __j];
								}
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
		for (long long int i = i1; i <= i2; i++)
		{
			for (long long int j = j1; j <= j2; j++)
			{
				if (isnan(dMemTable[i][j]))
				{
					for (long long int _j = j; _j <= j2; _j++)
					{
						if (!isnan(dMemTable[i][_j]))
						{
							if (j != j1)
							{
								for (long long int __j = j; __j < _j; __j++)
								{
									dMemTable[i][__j] = (dMemTable[i][_j] - dMemTable[i][j - 1]) / (double)(_j - j) * (double)(__j - j + 1) + dMemTable[i][j - 1];
								}
								if (bIsSaved)
								{
									bIsSaved = false;
									nLastSaved = time(0);
								}
								break;
							}
							else if (j == j1 && _j != j2)
							{
								for (long long int __j = j; __j < _j; __j++)
								{
									dMemTable[i][__j] = dMemTable[i][_j];
								}
								if (bIsSaved)
								{
									bIsSaved = false;
									nLastSaved = time(0);
								}
								break;
							}
						}
						if (j != j1 && _j == j2 && isnan(dMemTable[i][_j]))
						{
							for (long long int __j = j; __j <= j2; __j++)
							{
								dMemTable[i][__j] = dMemTable[i][j - 1];
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
		for (long long int j = j1; j <= j2; j++)
		{
			for (long long int i = i1; i <= i2; i++)
			{
				if (isnan(dMemTable[i][j]))
				{
					for (long long int _i = i; _i <= i2; _i++)
					{
						if (!isnan(dMemTable[_i][j]))
						{
							if (i != i1)
							{
								for (long long int __i = i; __i < _i; __i++)
								{
									dMemTable[__i][j] = (dMemTable[_i][j] - dMemTable[i - 1][j]) / (double)(_i - i) * (double)(__i - i + 1) + dMemTable[i - 1][j];
								}
								if (bIsSaved)
								{
									bIsSaved = false;
									nLastSaved = time(0);
								}
								break;
							}
							else if (i == i1 && _i != i2)
							{
								for (long long int __i = i; __i < _i; __i++)
								{
									dMemTable[__i][j] = dMemTable[_i][j];
								}
								if (bIsSaved)
								{
									bIsSaved = false;
									nLastSaved = time(0);
								}
								break;
							}
						}
						if (i != i1 && _i == i2 && isnan(dMemTable[_i][j]))
						{
							for (long long int __i = i; __i <= i2; __i++)
							{
								dMemTable[__i][j] = dMemTable[i - 1][j];
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
			Memory::smooth(i1, -2, j1, j1, nOrder, COLS);
			Memory::smooth(i1, -2, j2, j2, nOrder, COLS);
			Memory::smooth(i1, i1, j1, j2, nOrder, LINES);
			Memory::smooth(i2, i2, j1, j2, nOrder, LINES);
		}
		else
		{
			Memory::smooth(i1, i2, j1, j1, nOrder, COLS);
			Memory::smooth(i1, i2, j2, j2, nOrder, COLS);
			Memory::smooth(i1, i1, j1, j2, nOrder, LINES);
			Memory::smooth(i2, i2, j1, j2, nOrder, LINES);
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
								dMemTable[i + ni][j + nj] = 0.5 * med(i1, i2 + 1, j1, j2 + 1) + 0.25 * (
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


// This member function smoothes the data described by the passed coordinates using the order nOrder
bool Memory::smooth(long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nOrder, AppDir Direction)
{
	bool bUseAppendedZeroes = false;

	// Avoid the border cases
	if (!bValidData)
		throw SyntaxError(SyntaxError::NO_CACHED_DATA, "smooth", SyntaxError::invalid_position);
	if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
		throw SyntaxError(SyntaxError::INVALID_INDEX, "smooth", SyntaxError::invalid_position, i1, i2, j1, j2);

	// Evaluate the indices
    if (i2 == -2)
        bUseAppendedZeroes = true;
    if (!evaluateIndices(i1, i2, j1, j2))
        throw SyntaxError(SyntaxError::INVALID_INDEX, "smooth", SyntaxError::invalid_position, i1, i2, j1, j2);

    // Change the predefined application directions, if it's needed
	if ((Direction == ALL || Direction == GRID) && i2 - i1 < 3)
		Direction = LINES;
	if ((Direction == ALL || Direction == GRID) && j2 - j1 < 3)
		Direction = COLS;

    // Check the order
	if (nOrder < 1 || (nOrder >= nLines && Direction == COLS) || (nOrder >= nCols && Direction == LINES) || ((nOrder >= nLines || nOrder >= nCols) && (Direction == ALL || Direction == GRID)))
		throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, "smooth", SyntaxError::invalid_position);

    // Get the appended zeros
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

    // If the application direction is equal to GRID, then the first two columns
    // should be evaluted separately, because they contain the axis values
	if (Direction == GRID)
	{
	    // Will never return false
		if (bUseAppendedZeroes)
		{
			if (!smooth(i1, -2, j1, -1, nOrder, COLS) || !smooth(i1, -2, j1 + 1, -1, nOrder, COLS))
				return false;
		}
		else
		{
			if (!smooth(i1, i2, j1, j1 + 1, nOrder, COLS))
				return false;
		}
		j1 += 2;
	}

	// The first job is to simply remove invalid values and then smooth the
	// framing points of the data section
	if (Direction == ALL || Direction == GRID)
	{
		Memory::retoque(i1, i2 + 1, j1, j2 + 1, ALL);
		Memory::smooth(i1, i2, j1, j1, nOrder, COLS);
		Memory::smooth(i1, i2, j2, j2, nOrder, COLS);
		Memory::smooth(i1, i1, j1, j2, nOrder, LINES);
		Memory::smooth(i2, i2, j1, j2, nOrder, LINES);
	}

	// Apply the actual smoothing of the data
	if (Direction == LINES)
	{
	    // Smooth the lines
		for (long long int i = i1; i <= i2; i++)
		{
			for (long long int j = j1 + 1; j <= j2 - nOrder; j++)
			{
				for (unsigned int n = 0; n < nOrder; n++)
				{
				    // Smooth only, if the boundaries and the current point are real values.
				    // Smooth by halving the distance to the average line between the boundaries
					if (!isnan(dMemTable[i][j - 1]) && !isnan(dMemTable[i][j + nOrder]) && !isnan(dMemTable[i][j + n]))
						dMemTable[i][j + n] = 0.5 * dMemTable[i][j + n] + 0.5 * (dMemTable[i][j - 1] + (dMemTable[i][j + nOrder] - dMemTable[i][j - 1]) / (double)(nOrder + 1) * (double)(n + 1));
				}
			}
		}
	}
	else if (Direction == COLS)
	{
	    // Smooth the columns
		for (long long int j = j1; j <= j2; j++)
		{
			for (long long int i = i1 + 1; i <= i2 - nOrder; i++)
			{
				for (unsigned int n = 0; n < nOrder; n++)
				{
				    // Smooth only, if the boundaries and the current point are real values.
				    // Smooth by halving the distance to the average line between the boundaries
					if (!isnan(dMemTable[i - 1][j]) && !isnan(dMemTable[i + nOrder][j]) && !isnan(dMemTable[i + n][j]))
						dMemTable[i + n][j] = 0.5 * dMemTable[i + n][j] + 0.5 * (dMemTable[i - 1][j] + (dMemTable[i + nOrder][j] - dMemTable[i - 1][j]) / (double)(nOrder + 1) * (double)(n + 1));
				}
			}
		}
	}
	else if ((Direction == ALL || Direction == GRID) && i2 - i1 > 1 && j2 - j1 > 1)
	{
	    // Smooth the data in two dimensions, if that is reasonable
	    // Go through every point
		for (long long int j = j1; j <= j2 - nOrder - 1; j++)
		{
			for (long long int i = i1; i <= i2 - nOrder - 1; i++)
			{
			    // Only smooth the data points more left and further down in the data grid
				for (unsigned int nj = 1; nj <= nOrder; nj++)
				{
					for (unsigned int ni = 1; ni <= nOrder; ni++)
					{
						if (nOrder == 1)
						{
                            // Simple case: nOrder == c1
							if (!isnan(dMemTable[i + ni][j + nOrder + 1])
									&& !isnan(dMemTable[i + ni][j])
									&& !isnan(dMemTable[i + nOrder + 1][j + nj])
									&& !isnan(dMemTable[i][j + nj]))
							{
							    // Smooth only, if the boundaries and the current point are real values.
                                // Smooth by halving the distance to the average line between the boundaries
								if (!isnan(dMemTable[i + ni][j + nj]))
									dMemTable[i + ni][j + nj] = 0.5 * dMemTable[i + ni][j + nj] + 0.25 * (
											dMemTable[i][j + nj] + (dMemTable[i + nOrder + 1][j + nj] - dMemTable[i][j + nj]) / (double)(nOrder + 1) * (double)ni
											+ dMemTable[i + ni][j] + (dMemTable[i + ni][j + nOrder + 1] - dMemTable[i + ni][j]) / (double)(nOrder + 1) * (double)nj);
								else
								{
									dMemTable[i + ni][j + nj] = 0.5 * (
											dMemTable[i][j + nj] + (dMemTable[i + nOrder + 1][j + nj] - dMemTable[i][j + nj]) / (double)(nOrder + 1) * (double)ni
											+ dMemTable[i + ni][j] + (dMemTable[i + ni][j + nOrder + 1] - dMemTable[i + ni][j]) / (double)(nOrder + 1) * (double)nj);
								}
							}
						}
						else
						{
						    // More complicated case
						    // evaluate first, whether the current data section is surrounded by valid values
							if (isValidDisc(i, j, nOrder + 1))
							{
							    // cross hair: summarize the linearily interpolated values along the rows and cols at the desired position
							    // Summarize implies that the value is not averaged yet
								double dAverage = dMemTable[i][j + nj]
												  + (dMemTable[i + nOrder + 1][j + nj] - dMemTable[i][j + nj]) / (double)(nOrder + 1) * (double)ni
												  + dMemTable[i + ni][j]
												  + (dMemTable[i + ni][j + nOrder + 1] - dMemTable[i + ni][j]) / (double)(nOrder + 1) * (double)nj;

								// Additional weighting because are the nearest neighbours
								dAverage *= 2.0;

								// Calculate along columns
								// Find the diagonal neighbours and interpolate the value
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

								// calculate along rows
								// Find the diagonal neighbours and interpolate the value
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

								// Restore the desired average
								dAverage /= 6.0;

								// Apply the actual smoothing
								if (!isnan(dMemTable[i + ni][j + nj]))
								{
									dMemTable[i + ni][j + nj] =
													0.5 * (1.0 - 0.5 * hypot(ni - (nOrder) / 2.0, nj - (nOrder) / 2.0) / (M_SQRT2 * ((nOrder) / 2.0)))
													* dMemTable[i + ni][j + nj]
													+ 0.5 * (1.0 + 0.5 * hypot(ni - (nOrder) / 2.0, nj - (nOrder) / 2.0) / (M_SQRT2 * (nOrder / 2.0))) * dAverage;

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
	if (bIsSaved)
	{
		bIsSaved = false;
		nLastSaved = time(0);
	}
	return true;
}


// This member function resamples the data described by the passed coordinates using the new samples nSamples
bool Memory::resample(long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nSamples, AppDir Direction)
{

	bool bUseAppendedZeroes = false;

	const long long int __nLines = nLines;
	const long long int __nCols = nCols;

	// Avoid border cases
	if (!bValidData)
		throw SyntaxError(SyntaxError::NO_CACHED_DATA, "resample", SyntaxError::invalid_position);
	if (!nSamples)
		throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
	if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
		throw SyntaxError(SyntaxError::INVALID_INDEX, "resample", SyntaxError::invalid_position, i1, i2, j1, j2);

    // Evaluate the indices
    if (i2 == -2)
        bUseAppendedZeroes = true;
    if (!evaluateIndices(i1, i2, j1, j2))
        throw SyntaxError(SyntaxError::INVALID_INDEX, "resample", SyntaxError::invalid_position, i1, i2, j1, j2);

    // Change the predefined application directions, if it's needed
	if ((Direction == ALL || Direction == GRID) && i2 - i1 < 3)
		Direction = LINES;
	if ((Direction == ALL || Direction == GRID) && j2 - j1 < 3)
		Direction = COLS;

    // Get the appended zeros
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

	// If the application direction is equal to GRID, then the indices should
	// match a sufficiently enough large data array
	if (Direction == GRID)
	{
		if (j2 - j1 - 2 != i2 - i1 && !bUseAppendedZeroes)
			throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
		else if (j2 - j1 - 2 != (nLines - nAppendedZeroes[j1 + 1] - 1) - i1 && bUseAppendedZeroes)
			throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
	}

	// Create the dynamic memory
	double** dResampleBuffer = new double*[__nLines];
	for (long long int i = 0; i < __nLines; i++)
		dResampleBuffer[i] = new double[__nCols];

    // Prepare a pointer to the resampler object
	Resampler* _resampler = nullptr;

	// Create the actual resample object based upon the application direction
	if (Direction == ALL || Direction == GRID) // 2D
	{
		if (Direction == GRID)
		{
		    try
		    {
                // Apply the resampling to the first two columns first:
                // These contain the axis values
                if (bUseAppendedZeroes)
                {
                    resample(i1, -2, j1, -1, nSamples, COLS);
                    resample(i1, -2, j1 + 1, -1, nSamples, COLS);
                }
                else
                {
                    // Achsenwerte getrennt resamplen
                    resample(i1, i2, j1, j1 + 1, nSamples, COLS);
                }
            }
            catch (...)
            {
                for (long long int i = 0; i < __nLines; i++)
                    delete[] dResampleBuffer[i];
                delete[] dResampleBuffer;
                throw;
            }
                j1 += 2;
		}
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

		// Create the resample object and prepare the needed memory
		_resampler = new Resampler(j2 - j1 + 1, i2 - i1 + 1, nSamples, nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
		if (nSamples > i2 - i1 + 1 || nSamples > j2 - j1 + 1)
			resizeMemory(nLines + nSamples - (i2 - i1 + 1), nCols + nSamples - (j2 - j1 + 1));
	}
	else if (Direction == COLS) // cols
	{
		// Create the resample object and prepare the needed memory
		_resampler = new Resampler(j2 - j1 + 1, i2 - i1 + 1, j2 - j1 + 1, nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
		if (nSamples > i2 - i1 + 1)
			resizeMemory(nLines + nSamples - (i2 - i1 + 1), nCols - 1);
	}
	else if (Direction == LINES)// lines
	{
		// Create the resample object and prepare the needed memory
		_resampler = new Resampler(j2 - j1 + 1, i2 - i1 + 1, nSamples, i2 - i1 + 1, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
		if (nSamples > j2 - j1 + 1)
			resizeMemory(nLines - 1, nCols + nSamples - (j2 - j1 + 1));
	}

	// Ensure that the resampler was created
	if (!_resampler)
	{
		for (long long int i = 0; i < __nLines; i++)
			delete[] dResampleBuffer[i];
		delete[] dResampleBuffer;

		throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
	}

	const double* dOutputSamples = 0;
	double* dInputSamples = new double[j2 - j1 + 1];
	long long int _ret_line = 0;
	long long int _final_cols = 0;

	// Determine the number of final columns. These will stay constant only in
	// the column application direction
	if (Direction == ALL || Direction == GRID || Direction == LINES)
		_final_cols = nSamples;
	else
		_final_cols = j2 - j1 + 1;

    // Copy the whole memory
	for (long long int i = 0; i < __nLines; i++)
	{
		for (long long int j = 0; j < __nCols; j++)
		{
			dResampleBuffer[i][j] = dMemTable[i][j];
		}
	}

	// Resample the data table
	// Apply the resampling linewise
	for (long long int i = i1; i <= i2; i++)
	{
		for (long long int j = j1; j <= j2; j++)
		{
			dInputSamples[j - j1] = dMemTable[i][j];
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
							dResampleBuffer[i1 + _ret_line][j1 + _fin] = NAN;
							continue;
						}
						dResampleBuffer[i1 + _ret_line][j1 + _fin] = dOutputSamples[_fin];
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
				dResampleBuffer[i1 + _ret_line][j1 + _fin] = NAN;
				continue;
			}
			dResampleBuffer[i1 + _ret_line][j1 + _fin] = dOutputSamples[_fin];
		}
		_ret_line++;
	}
	_ret_line++;

	// Delete the resampler: it is not used any more
	delete _resampler;

	// Block unter dem resampleten kopieren
	if (i2 - i1 + 1 < nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
	{
		for (long long int i = i2 + 1; i < __nLines; i++)
		{
			for (long long int j = j1; j <= j2; j++)
			{
				if (_ret_line + i - (i2 + 1) + i1 >= nLines)
				{
					for (long long int i = 0; i < __nLines; i++)
						delete[] dResampleBuffer[i];
					delete[] dResampleBuffer;

					throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
				}
				if (isnan(dMemTable[i][j]))
				{
					dResampleBuffer[_ret_line + i - (i2 + 1) + i1][j] = NAN;
				}
				else
				{
					dResampleBuffer[_ret_line + i - (i2 + 1) + i1][j] = dMemTable[i][j];
				}
			}
		}
	}
	else if (i2 - i1 + 1 > nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
	{
		for (long long int i = i1 + nSamples - 1; i <= i2; i++)
		{
			for (long long int j = j1; j <= j2; j++)
			{
				if (i >= nLines)
				{
					for (long long int i = 0; i < __nLines; i++)
						delete[] dResampleBuffer[i];
					delete[] dResampleBuffer;

					throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
				}
				dResampleBuffer[i][j] = NAN;
			}
		}
	}

	// Block rechts kopieren
	if (j2 - j1 + 1 < nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
	{
		for (long long int i = 0; i < __nLines; i++)
		{
			for (long long int j = j2 + 1; j < __nCols; j++)
			{
				if (_final_cols + j - (j2 + 1) + j1 >= nCols)
				{
					for (long long int i = 0; i < __nLines; i++)
						delete[] dResampleBuffer[i];
					delete[] dResampleBuffer;

					throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
				}
				if (isnan(dMemTable[i][j]))
				{
					dResampleBuffer[i][_final_cols + j - (j2 + 1) + j1] = NAN;
				}
				else
				{
					dResampleBuffer[i][_final_cols + j - (j2 + 1) + j1] = dMemTable[i][j];
				}
			}
		}
	}
	else if (j2 - j1 + 1 > nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
	{
		for (long long int i = i1; i < i2; i++)
		{
			for (long long int j = j1 + nSamples - 1; j <= j2; j++)
			{
				if (j >= nCols)
				{
					for (long long int i = 0; i < __nLines; i++)
						delete[] dResampleBuffer[i];
					delete[] dResampleBuffer;

					throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
				}
				dResampleBuffer[i][j] = NAN;
			}
		}
	}

	// After all data is restored successfully
	// copy the data points from the buffer back to their original state
	for (long long int i = 0; i < nLines; i++)
	{
		for (long long int j = 0; j < nCols; j++)
		{
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
	//cerr << "Erfolg!" << endl;
	return true;
}


