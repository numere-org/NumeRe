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
#include "../../kernel.hpp"


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
/// \param _vCol const VectorIndex&
/// \param nSign int
/// \param nColumns long longint
/// \return bool
///
/////////////////////////////////////////////////
bool Sorter::sortSubList(int* nIndex, int nElements, ColumnKeys* KeyList, long long int i1, long long int i2, const VectorIndex& _vCol, int nSign, long long int nColumns)
{
    // Get the subkey list
	ColumnKeys* subKeyList = KeyList->subkeys;
	int nStart = i1;

	if (subKeyList->cols.front() >= _vCol.size() && subKeyList->cols.front() < 0)
        return false;

	// If the subkey list is valid and contains further subkeys and the current
	// column number is not larger than the maximal column number, search through
	// the current column and look for blocks of equal numbers, which may be
	// sorted using further columns
	if (subKeyList && subKeyList->subkeys && _vCol[subKeyList->cols.front()] < nColumns)
	{
		for (int k = i1 + 1; k <= i2 && k < nElements; k++)
		{
		    if (subKeyList->cols.front() >= _vCol.size() || subKeyList->cols.front() < 0)
                return false;

		    // Is this the first element in the parent column, which is not equal to the current
		    // start element?
			if (compare(nIndex[k], nIndex[nStart], _vCol[KeyList->cols.front()]) != 0)
			{
			    // Do only something, if this is a block of at least two
			    // equal numbers
				if (k > nStart + 1)
				{
				    // Sort the current block of equal numbers
					if (!qSort(nIndex, nElements, _vCol[subKeyList->cols.front()], nStart, k - 1, nSign))
						return false;

					// Redo this for this block recursively using the next columns
					// in the ColumnKeys list
					if (!sortSubList(nIndex, nElements, subKeyList, nStart, k - 1, _vCol, nSign, nColumns))
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
/// \param sKeyList std::string&
/// \param nColumnCount long longint
/// \return ColumnKeys*
///
/////////////////////////////////////////////////
ColumnKeys* Sorter::evaluateKeyList(std::string sKeyList, long long int nColumnCount)
{
    // Create a new ColumnKeys object. The calling function is responsible
    // for cleanup
	ColumnKeys* keys = new ColumnKeys();
	mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

	// Determine, whether a hierarchical sorting is required
	if (sKeyList.find(':') == std::string::npos && sKeyList.find('[') == std::string::npos)
	{
	    // In this case not. Simply set the current number as first key
	    _parser.SetExpr(sKeyList);
		keys->cols = VectorIndex(_parser.Eval());
		return keys;
	}

	size_t subListStart;

    // In this case we want a hierarchical sorting
    // Go through the complete keylist string and decode it into
    // a recursive ColumnKeys object
    if ((subListStart = sKeyList.find('[')) != std::string::npos)
    {
        if (sKeyList.find(']', subListStart) != std::string::npos)
        {
            // Found a hierarchical subset. Extract the hierarchical
            // definition first
            size_t i = getMatchingParenthesis(StringView(sKeyList, subListStart));

            if (i != std::string::npos)
            {
                // Decode the hierarchical subset recursively and assign
                // the returned pointer to a subfield of the ColumnKeys object
                keys->subkeys = evaluateKeyList(sKeyList.substr(subListStart + 1, i - 1), nColumnCount);
            }
        }
        else
        {
            // Something is wrong here. Clean up memory and return a
            // null pointer
            delete keys;
            return nullptr;
        }
    }

    // Decode all remaining top-level indices
    EndlessVector<std::string> strIndex = getAllIndices(sKeyList.substr(0, sKeyList.find('[')));
    std::vector<int> indices;

    for (size_t i = 0; i < strIndex.size(); i++)
    {
        if (!strIndex[i].length() && !i)
            indices.push_back(0);
        else if (!strIndex[i].length())
            indices.push_back(nColumnCount-1);
        else
        {
            _parser.SetExpr(strIndex[i]);
            mu::Array res = _parser.Eval();

            for (size_t i = 0; i < res.size(); i++)
            {
                indices.push_back(res.get(i).getNum().asI64()-1);
            }
        }
    }

    keys->cols = VectorIndex(indices);
    return keys;
}


