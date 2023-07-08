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

#include "stringmemory.hpp"
#include "../utils/tools.hpp"

using namespace std;

// STRINGINTERNALMEMORY IMPLEMENTATION

// Implementation for the "Sorter" object
int StringInternalMemory::compare(int i, int j, int col)
{
    if (sStrings.size() <= (size_t)col)
        return 0;

    // Ensure that the coordinates reference a
    // valid string object
    if (i <= j && sStrings[col].size() <= (size_t)i)
        return 0;
    else if (i < j && sStrings[col].size() <= (size_t)j)
        return -1;
    else if (i > j && sStrings[col].size() <= (size_t)j)
        return 0;
    else if (i > j && sStrings[col].size() <= (size_t)i)
        return 1;

    // Comparsion currently done depending on the
    // value of the internal private member variable
    if (bSortCaseInsensitive)
    {
        if (toLowerCase(sStrings[col][i]) < toLowerCase(sStrings[col][j]))
            return -1;
        else if (toLowerCase(sStrings[col][i]) == toLowerCase(sStrings[col][j]))
            return 0;
    }
    else
    {
        if (sStrings[col][i] < sStrings[col][j])
            return -1;
        else if (sStrings[col][i] == sStrings[col][j])
            return 0;
    }
    return 1;
}

// Implementation for the "Sorter" object
bool StringInternalMemory::isValue(int line, int col)
{
    if (sStrings.size() <= (size_t)col)
        return false;
    if (sStrings[col].size() <= (size_t)line)
        return false;

    return sStrings[col][line].length();
}

// Entry point into the sorting algorithm. Prepares the indices
// and the ColumnKeys object, if necessary
vector<int> StringInternalMemory::sortElements(long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    if (!sStrings.size())
        return vector<int>();

    bool bError = false;
    bool bReturnIndex = false;
    bSortCaseInsensitive = false;
    int nSign = 1;
    vector<int> vIndex;

    // Look for command line parameters
    if (findParameter(sSortingExpression, "desc"))
        nSign = -1;

    if (findParameter(sSortingExpression, "ignorecase"))
        bSortCaseInsensitive = true;

    if (findParameter(sSortingExpression, "index"))
        bReturnIndex = true;

    // Prepare the indices
    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;

    if (j2 >= sStrings.size())
        j2 = sStrings.size() - 1;

    // Create the sorting index
    for (int i = i1; i <= i2; i++)
        vIndex.push_back(i);

    // Enter the quicksort algorithm
    if (!findParameter(sSortingExpression, "cols", '=') && !findParameter(sSortingExpression, "c", '='))
    {
        // Sort everything
        for (int i = j1; i <= j2; i++)
        {
            if (!qSort(&vIndex[0], i2-i1+1, i, 0, i2-i1, nSign))
            {
                throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, "sort string" + sSortingExpression, SyntaxError::invalid_position);
            }

            // If the sorting index is requested,
            // then only sort the first column and return
            if (bReturnIndex)
            {
                break;
            }

            reorderColumn(vIndex, i1, i2, i);

            // Re-create the plain index
            for (int j = i1; j <= i2; j++)
                vIndex[j] = j;
        }
    }
    else
    {
        // Sort a column selection or sort hierarchically
        string sCols = "";

        // Extract the column definitions
        if (findParameter(sSortingExpression, "cols", '='))
        {
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "cols", '=')+4);
        }
        else
        {
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "c", '=')+1);
        }

        // Decode the column definitions and apply the
        // sorting algorithm it to the columns
        while (sCols.length())
        {
            // Create a ColumnKeys object
            ColumnKeys* keys = evaluateKeyList(sCols, j2-j1+1);

            if (!keys)
                throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, "string() " + sSortingExpression, SyntaxError::invalid_position);

            // The keys work as regular column indices, prepare them
            if (keys->nKey[1] == -1)
                keys->nKey[1] = keys->nKey[0]+1;

            // Apply the sorting algorithm to every column in the range
            // of the two keys
            for (int j = keys->nKey[0]; j < keys->nKey[1]; j++)
            {
                if (!qSort(&vIndex[0], i2-i1+1, j+j1, 0, i2-i1, nSign))
                {
                    throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, "string() " + sSortingExpression, SyntaxError::invalid_position);
                }

                // Subkey list: sort hierarchically
                if (keys->subkeys && keys->subkeys->subkeys)
                {
                   if (!sortSubList(&vIndex[0], i2-i1+1, keys, i1, i2, j1, nSign, sStrings.size()))
                    {
                        delete keys;
                        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, "string() " + sSortingExpression, SyntaxError::invalid_position);
                    }
                }

                // If the index is requested, stop the process here
                if (bReturnIndex)
                    break;

                // Reorder the columns
                reorderColumn(vIndex, i1, i2, j+j1);

                ColumnKeys* subKeyList = keys->subkeys;

                // Apply the sorted index to every column in the subkey list
                while (subKeyList)
                {
                    if (subKeyList->nKey[1] == -1)
                        subKeyList->nKey[1] = subKeyList->nKey[0]+1;

                    for (int _j = subKeyList->nKey[0]; _j < subKeyList->nKey[1]; _j++)
                    {
                        reorderColumn(vIndex, i1, i2, _j+j1);
                    }

                    subKeyList = subKeyList->subkeys;
                }

                // Re-create the plain index
                for (int _j = i1; _j <= i2; _j++)
                    vIndex[_j] = _j;
            }

            // Free the memory of the ColumnKeys object
            delete keys;

            if (bReturnIndex)
                break;
        }
    }

    // If the index was requested, increment every index by one
    if (bReturnIndex)
    {
        for (int i = 0; i <= i2-i1; i++)
            vIndex[i]++;
    }

    if (bError || !bReturnIndex)
        return vector<int>();

    return vIndex;
}

// Private member function to reorder a selected column based
// upon the passed index
void StringInternalMemory::reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1)
{
    // Create new memory on the heap
    string* sSortVector = new string[i2-i1+1];

    // Copy the elements in the new order
    // into the prepared memory
    for (int i = 0; i <= i2-i1; i++)
    {
        sSortVector[i] = sStrings[j1][vIndex[i]];
    }

    // Copy the contents directly from the
    // prepared in the new order
    for (int i = 0; i <= i2-i1; i++)
    {
        sStrings[j1][i+i1] = sSortVector[i];
    }

    // Free the prepared memory
    delete[] sSortVector;
}




// STRINGMEMORY IMPLEMENTATION

// This member function is used for writing strings into the "string()" object
bool StringMemory::writeString(const string& _sString, size_t _nthString, size_t nCol)
{
    // If this is the first string to be written
    if (_stringIntMem.sStrings.empty())
    {
        // Only do something, if the source string is not empty
        if (_sString.length())
        {
            // Prepare the storage if needed
            for (size_t i = 0; i <= nCol; i++)
            {
                _stringIntMem.sStrings.push_back(vector<string>());
            }
            if (_nthString == string::npos)
            {
                _stringIntMem.sStrings[nCol].push_back(_sString);
            }
            else
            {
                _stringIntMem.sStrings[nCol].resize(_nthString+1,"");
                _stringIntMem.sStrings[nCol][_nthString] = _sString;
            }
        }
        return true;
    }

    // Add a new column, if needed
    if (nCol >= _stringIntMem.sStrings.size())
    {
        for (size_t i = _stringIntMem.sStrings.size(); i <= nCol; i++)
            _stringIntMem.sStrings.push_back(vector<string>());
    }

    // If the string shall be appended at the end or the current
    // column is empty, add a new string element
    if (_nthString == string::npos || !_stringIntMem.sStrings[nCol].size())
    {
        if (_sString.length())
            _stringIntMem.sStrings[nCol].push_back(_sString);
        return true;
    }

    // If the string is not empty but shall be written to a larger location
    // than the storage size, resize the storage correspondingly
    while (_nthString >= _stringIntMem.sStrings[nCol].size() && _sString.length())
        _stringIntMem.sStrings[nCol].resize(_nthString+1, "");

    // All other cases
    if (!_sString.length() && _nthString+1 == _stringIntMem.sStrings[nCol].size())
    {
        // this is an empty string, and it allows to reduce the size of
        // the storage
        _stringIntMem.sStrings[nCol].pop_back();
        while (_stringIntMem.sStrings[nCol].size() && !_stringIntMem.sStrings[nCol].back().length())
            _stringIntMem.sStrings[nCol].pop_back();
    }
    else if (_nthString < _stringIntMem.sStrings[nCol].size())
        _stringIntMem.sStrings[nCol][_nthString] = _sString;
    return true;
}

// This member function is the interface to read strings from the "string()" object
string StringMemory::readString(size_t _nthString, size_t nCol)
{
    // Ensure that the selected column exists
    if (nCol >= _stringIntMem.sStrings.size())
        return "";

    // Select the string from the table
    if (_nthString == string::npos)
    {
        if (_stringIntMem.sStrings[nCol].size())
            return _stringIntMem.sStrings[nCol].back();

        return "";
    }
    else if (_nthString >= _stringIntMem.sStrings[nCol].size())
        return "";
    else
        return _stringIntMem.sStrings[nCol][_nthString];

    return "";
}

// This member function returns the maximal string in the "string()" object
// in the selected column
string StringMemory::maxString(size_t i1, size_t i2, size_t nCol)
{
    // Ensure that the selected column exists
    if (nCol >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (i2 == string::npos || i2 > _stringIntMem.sStrings[nCol].size())
        i2 = _stringIntMem.sStrings[nCol].size();

    // Return an empty string, if the second column does not exist
    if (!i2 || _stringIntMem.sStrings[nCol].empty())
        return "";

    string sMax = _stringIntMem.sStrings[nCol][i1];

    // Search for the maximal string
    for (size_t i = i1+1; i < i2; i++)
    {
        if (sMax < _stringIntMem.sStrings[nCol][i])
            sMax = _stringIntMem.sStrings[nCol][i];
    }

    return sMax;
}

// This member function returns the maximal string in the "string()" object
// in the selected column
string StringMemory::maxString(VectorIndex _vLine, VectorIndex _vCol)
{
    // Ensure that the selected column exists
    if (_vCol.front() >= (int)_stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > (int)_stringIntMem.sStrings[_vCol.front()].size())
        _vLine.setRange(0, _stringIntMem.sStrings[_vCol.front()].size()-1);

    // Return an empty string, if the second column does not exist
    if (!_vLine.last() || _stringIntMem.sStrings[_vCol.front()].empty())
        return "";

    string sMax = _stringIntMem.sStrings[_vCol.front()][_vLine[0]];

    // Search for the maximal string
    for (size_t i = 1; i < _vLine.size(); i++)
    {
        if (sMax < _stringIntMem.sStrings[_vCol.front()][_vLine[i]])
            sMax = _stringIntMem.sStrings[_vCol.front()][_vLine[i]];
    }

    return sMax;
}

// This member function returns the minimal string in the "string()" object
// in the selected column
string StringMemory::minString(size_t i1, size_t i2, size_t nCol)
{
    // Ensure that the selected column exists
    if (nCol >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (i2 == string::npos || i2 > _stringIntMem.sStrings[nCol].size())
        i2 = _stringIntMem.sStrings[nCol].size();

    // Return an empty string, if the second column does not exist
    if (!i2 || _stringIntMem.sStrings[nCol].empty())
        return "";

    string sMin = _stringIntMem.sStrings[nCol][i1];

    // Search for the minimal string
    for (size_t i = i1+1; i < i2; i++)
    {
        if (sMin > _stringIntMem.sStrings[nCol][i])
            sMin = _stringIntMem.sStrings[nCol][i];
    }

    return sMin;
}

// This member function returns the minimal string in the "string()" object
// in the selected column
string StringMemory::minString(VectorIndex _vLine, VectorIndex _vCol)
{
    // Ensure that the selected column exists
    if (_vCol.front() >= (int)_stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > (int)_stringIntMem.sStrings[_vCol.front()].size())
        _vLine.setRange(0, _stringIntMem.sStrings[_vCol.front()].size()-1);

    // Return an empty string, if the second column does not exist
    if (!_vLine.last() || _stringIntMem.sStrings[_vCol.front()].empty())
        return "";

    string sMin = _stringIntMem.sStrings[_vCol.front()][_vLine[0]];

    // Search for the minimal string
    for (size_t i = 1; i < _vLine.size(); i++)
    {
        if (sMin > _stringIntMem.sStrings[_vCol.front()][_vLine[i]])
            sMin = _stringIntMem.sStrings[_vCol.front()][_vLine[i]];
    }

    return sMin;
}

// This member function concatenates the strings in the "string()" object
// in the selected range and returns it
string StringMemory::sumString(size_t i1, size_t i2, size_t nCol)
{
    // Ensure that the selected column exists
    if (nCol >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (i2 == string::npos || i2 > _stringIntMem.sStrings[nCol].size())
        i2 = _stringIntMem.sStrings[nCol].size();

    // Return an empty string, if the second column does not exist
    if (!i2 || _stringIntMem.sStrings[nCol].empty())
        return "";

    string sSum = "";

    // Concatenate the strings
    for (size_t i = i1; i < i2; i++)
    {
        sSum += _stringIntMem.sStrings[nCol][i];
    }

    return sSum;
}

// This member function concatenates the strings in the "string()" object
// in the selected range and returns it
string StringMemory::sumString(VectorIndex _vLine, VectorIndex _vCol)
{
    // Ensure that the selected column exists
    if (_vCol.front() >= (int)_stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > (int)_stringIntMem.sStrings[_vCol.front()].size())
        _vLine.setRange(0, _stringIntMem.sStrings[_vCol.front()].size()-1);

    // Return an empty string, if the second column does not exist
    if (!_vLine.last() || _stringIntMem.sStrings[_vCol.front()].empty())
        return "";

    string sSum = "";

    // Concatenate the strings
    for (size_t i = 0; i < _vLine.size(); i++)
    {
        sSum += _stringIntMem.sStrings[_vCol.front()][_vLine[i]];
    }

    return sSum;
}
