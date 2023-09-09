/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "sorter.hpp"
#include "../utils/tools.hpp"

using namespace std;


/////////////////////////////////////////////////
/// \brief This public member function is the
/// interface to the quicksort algorithm, which
/// itself is implemented as a private member
/// function of this class.
///
/// \param nIndex int*
/// \param nElements int
/// \param nColumn int
/// \param nLeft long longint
/// \param nRight long longint
/// \param nSign int
/// \return bool
///
/////////////////////////////////////////////////
bool Sorter::qSort(int* nIndex, int nElements, int nColumn, long long int nLeft, long long int nRight, int nSign)
{
    // Ensure that all necessary paramters are available and valid
	if (!nIndex || !nElements || nLeft < 0 || nRight > nElements || nRight < nLeft)
	{
		return false;
	}

	// Ignore all invalid data points, which are at the
	// lower "right" end of the data set
	while (nRight >= nLeft && !isValue(nIndex[nRight], nColumn))
	{
		nRight--;
	}

	// Ensure that the right border is larger or equal to zero
	if (nRight < 0)
		return false;

	int nPos = nRight;

	// swap all invalid values to the right end of the array
	while (nPos >= nLeft)
	{
		if (!isValue(nIndex[nPos], nColumn))
		{
			int nTemp = nIndex[nPos];
			nIndex[nPos] = nIndex[nRight];
			nIndex[nRight] = nTemp;
			nRight--;
		}
		nPos--;
	}

	// Redirect the control to the quicksort implementation
	return qSortImplementation(nIndex, nElements, nColumn, nLeft, nRight, nSign);
}


/////////////////////////////////////////////////
/// \brief This private member function is the
/// actual implementation of the quicksort
/// algorithm.
///
/// \param nIndex int*
/// \param nElements int
/// \param nColumn int
/// \param nLeft long longint
/// \param nRight long longint
/// \param nSign int
/// \return bool
///
/////////////////////////////////////////////////
bool Sorter::qSortImplementation(int* nIndex, int nElements, int nColumn, long long int nLeft, long long int nRight, int nSign)
{
    // Ensure that all parameters are available and valid
	if (!nIndex || !nElements || nLeft < 0 || nRight > nElements || nRight < nLeft)
	{
		return false;
	}

	// If the left and right border are equal, return.
	// This section is already sorted
	if (nRight == nLeft)
		return true;

    // Catch the cases, where the distance between the left
    // and the right border equals to one
	if (nRight - nLeft <= 1 && (nSign * compare(nIndex[nLeft], nIndex[nRight], nColumn) <= 0))
		return true;
	else if (nRight - nLeft <= 1 && (nSign * compare(nIndex[nLeft], nIndex[nRight], nColumn) >= 0))
	{
		int nTemp = nIndex[nLeft];
		nIndex[nLeft] = nIndex[nRight];
		nIndex[nRight] = nTemp;
		return true;
	}

	// Move the middle element to the right
    int nTemp = nIndex[nRight];
    nIndex[nRight] = nIndex[(nRight+nLeft)/2];
    nIndex[(nRight+nLeft)/2] = nTemp;

	// Define pivot and running indices
	int nPivot = nRight;
	int i = nLeft;
	int j = nRight - 1;

	// Main part of the quicksort algorithm. Move all values,
	// which are smaller than the pivot value to the left and
	// all values, which are larger to the right
	do
	{
	    // Jump over all values, which are smaller
		while (i < nRight && (nSign * compare(nIndex[i], nIndex[nPivot], nColumn) <= 0))
			i++;

        // Jump over all values, which are larger
		while (j > nLeft && (nSign * compare(nIndex[j], nIndex[nPivot], nColumn) >= 0))
			j--;

        // Did we find two candidates, which are on the wrong side?
        // Exchange them.
		if (i < j)
		{
			int nTemp = nIndex[i];
			nIndex[i] = nIndex[j];
			nIndex[j] = nTemp;
		}
	}
	while (i < j);

	// Move the pivot element to its correct position
	if (nSign * compare(nIndex[i], nIndex[nPivot], nColumn) > 0)
	{
		int nTemp = nIndex[i];
		nIndex[i] = nIndex[nRight];
		nIndex[nRight] = nTemp;
	}

	// Call this algorithm recursively for the left and
	// the right sections of the whole interval
	if (i > nLeft)
	{
		if (!qSortImplementation(nIndex, nElements, nColumn, nLeft, i - 1, nSign))
			return false;
	}

	if (i < nRight)
	{
		if (!qSortImplementation(nIndex, nElements, nColumn, i + 1, nRight, nSign))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This public member function handles
/// the hierarchical sorting process of many
/// columns together. The ColumnKeys object
/// contains the necessary information needed for
/// the hierarchical sorting.
///
/// \param nIndex int*
/// \param nElements int
/// \param KeyList ColumnKeys*
/// \param i1 long longint
/// \param i2 long longint
/// \param j1 long longint
/// \param nSign int
/// \param nColumns long longint
/// \return bool
///
/////////////////////////////////////////////////
bool Sorter::sortSubList(int* nIndex, int nElements, ColumnKeys* KeyList, long long int i1, long long int i2, long long int j1, int nSign, long long int nColumns)
{
    // Get the subkey list
	ColumnKeys* subKeyList = KeyList->subkeys;
	int nTopColumn = KeyList->nKey[0];
	int nStart = i1;

	// If the subkey list is valid and contains further subkeys and the current
	// column number is not larger than the maximal column number, search through
	// the current column and look for blocks of equal numbers, which may be
	// sorted using further columns
	if (subKeyList && subKeyList->subkeys && j1 + nTopColumn < nColumns)
	{
		for (int k = i1 + 1; k <= i2 && k < nElements; k++)
		{
		    // Is this the first element, which is not equal to the current
		    // start element?
			if (compare(nIndex[k], nIndex[nStart], j1 + nTopColumn) != 0)
			{
			    // Do only something, if this is a block of at least two
			    // equal numbers
				if (k > nStart + 1)
				{
				    // Sort the current block of equal numbers
					if (!qSort(nIndex, nElements, j1 + subKeyList->nKey[0], nStart, k - 1, nSign))
					{
						return false;
					}

					// Redo this for this block recursively using the next columns
					// in the ColumnKeys list
					if (!sortSubList(nIndex, nElements, subKeyList, nStart, k - 1, j1, nSign, nColumns))
						return false;
				}

				// Set the current number as the next start number
				nStart = k;
			}
		}
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This public member function creates a
/// ColumnKeys object from a string containing
/// the hierarchical sorting order for the
/// current sorting process.
///
/// \param sKeyList string&
/// \param nColumnCount long longint
/// \return ColumnKeys*
///
/////////////////////////////////////////////////
ColumnKeys* Sorter::evaluateKeyList(string& sKeyList, long long int nColumnCount)
{
    // Create a new ColumnKeys object. The calling function is responsible
    // for cleanup
	ColumnKeys* keys = new ColumnKeys();

	// Determine, whether a hierarchical sorting is required
	if (sKeyList.find(':') == string::npos && sKeyList.find('[') == string::npos)
	{
	    // In this case not. Simply set the current number as first key
		keys->nKey[0] = StrToInt(sKeyList) - 1;
		sKeyList.clear();
	}
	else
	{
	    // In this case we want a hierarchical sorting
		size_t nLastIndex = 0;

		// Go through the complete keylist string and decode it into
		// a recursive ColumnKeys object
		for (size_t n = 0; n < sKeyList.length(); n++)
		{
			if (sKeyList[n] == ':')
			{
			    // Found a column separator
				if (n != nLastIndex)
					keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n - nLastIndex)) - 1;

                // Last element? Use the maximal number of columns as last column
				if (n + 1 == sKeyList.length())
					keys->nKey[1] = nColumnCount;

                // Find the end of the next column key
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
							keys->nKey[1] = nColumnCount;
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
			    // Found a hierarchical subset. Extract the hierarchical
			    // definition first
				keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n - nLastIndex)) - 1;
				string sColArray;

				size_t i = getMatchingParenthesis(StringView(sKeyList, n));

				if (i != string::npos)
				{
					sColArray = sKeyList.substr(n + 1, i - 1);
					sKeyList.erase(0, i + n + 1);
				}

				// Decode the hierarchical subset recursively and assign
				// the returned pointer to a subfield of the ColumnKeys object
				keys->subkeys = evaluateKeyList(sColArray, nColumnCount);

				break;
			}
			else if (sKeyList[n] == '[')
			{
			    // Something is wrong here. Clean up memory and return a
			    // null pointer
				delete keys;
				return nullptr;
			}
		}
	}

	return keys;
}


