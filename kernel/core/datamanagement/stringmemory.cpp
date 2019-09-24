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


// STRINGINTERNALMEMORY IMPLEMENTATION

// Implementation for the "Sorter" object
int StringInternalMemory::compare(int i, int j, int col)
{
    if (sStrings.size() <= col)
        return 0;

    // Ensure that the coordinates reference a
    // valid string object
    if (i <= j && sStrings[col].size() <= i)
        return 0;
    else if (i < j && sStrings[col].size() <= j)
        return -1;
    else if (i > j && sStrings[col].size() <= j)
        return 0;
    else if (i > j && sStrings[col].size() <= i)
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
    if (sStrings.size() <= col)
        return false;
    if (sStrings[col].size() <= line)
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
    if (matchParams(sSortingExpression, "desc"))
        nSign = -1;

    if (matchParams(sSortingExpression, "ignorecase"))
        bSortCaseInsensitive = true;

    if (matchParams(sSortingExpression, "index"))
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
    if (!matchParams(sSortingExpression, "cols", '=') && !matchParams(sSortingExpression, "c", '='))
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
        if (matchParams(sSortingExpression, "cols", '='))
        {
            sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "cols", '=')+4);
        }
        else
        {
            sCols = getArgAtPos(sSortingExpression, matchParams(sSortingExpression, "c", '=')+1);
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

                    for (int j = subKeyList->nKey[0]; j < subKeyList->nKey[1]; j++)
                    {
                        reorderColumn(vIndex, i1, i2, j+j1);
                    }

                    subKeyList = subKeyList->subkeys;
                }

                // Re-create the plain index
                for (int j = i1; j <= i2; j++)
                    vIndex[j] = j;
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
bool StringMemory::writeString(const string& _sString, unsigned int _nthString, unsigned int nCol)
{
    // If this is the first string to be written
    if (_stringIntMem.sStrings.empty())
    {
        // Only do something, if the source string is not empty
        if (_sString.length())
        {
            // Prepare the storage if needed
            for (unsigned int i = 0; i <= nCol; i++)
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
        for (unsigned int i = _stringIntMem.sStrings.size(); i <= nCol; i++)
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
string StringMemory::readString(unsigned int _nthString, unsigned int nCol)
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
string StringMemory::maxString(unsigned int i1, unsigned int i2, unsigned int nCol)
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
    for (unsigned int i = i1+1; i < i2; i++)
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
    if (_vCol.front() >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > _stringIntMem.sStrings[_vCol.front()].size())
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
string StringMemory::minString(unsigned int i1, unsigned int i2, unsigned int nCol)
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
    for (unsigned int i = i1+1; i < i2; i++)
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
    if (_vCol.front() >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > _stringIntMem.sStrings[_vCol.front()].size())
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
string StringMemory::sumString(unsigned int i1, unsigned int i2, unsigned int nCol)
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
    for (unsigned int i = i1; i < i2; i++)
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
    if (_vCol.front() >= _stringIntMem.sStrings.size())
        return "";

    // Fill the second line index automatically
    if (_vLine.isOpenEnd() || _vLine.last() > _stringIntMem.sStrings[_vCol.front()].size())
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

// This private member function examines the first and
// the last character of the passed string and determines,
// whether it is a delimiter or not
bool StringMemory::checkStringvarDelimiter(const string& sToken) const
{
    static const string sDELIMITER = "+-*/ ()={}^&|!<>,\\%#[]?:\";";

    // The opening parenthesis at the end indicates a function.
    // This is obviously not a string variable
    if (sToken.back() == '(')
        return false;

    // If the first and the last character are part of the
    // delimiter list (or the last character is a dot), then
    // we indicate the current token as delimited.
    return sDELIMITER.find(sToken.front()) != string::npos && (sDELIMITER.find(sToken.back()) != string::npos || sToken.back() == '.');
}

// This public member function determines, whether the passed
// string line contains string variables as part of the expression
bool StringMemory::containsStringVars(const string& _sLine) const
{
    // Do nothing, if no string variables were declared
    if (!_stringIntMem.sStringVars.size())
        return false;

    // Add whitespaces for safety
    string sLine = " " + _sLine + " ";

    // Search for the first match of all declared string variables
    for (auto iter = _stringIntMem.sStringVars.begin(); iter != _stringIntMem.sStringVars.end(); ++iter)
    {
        // Compare the located match to the delimiters and return
        // true, if the match is delimited on both sides
        if (sLine.find(iter->first) != string::npos
            && sLine[sLine.find(iter->first)+(iter->first).length()] != '('
            && checkStringvarDelimiter(sLine.substr(sLine.find(iter->first)-1, (iter->first).length()+2))
            )
            return true;
    }

    // No match found
    return false;
}

// This public member function resolves all string variable
// occurences and replaces them with their value or the standard
// string function signature, if the string variable is connected
// to a method.
void StringMemory::getStringValues(string& sLine, unsigned int nPos)
{
    // Do nothing, if no string variables were declared
    if (!_stringIntMem.sStringVars.size())
        return;

    unsigned int __nPos = nPos;
    sLine += " ";

    // Try to find every string variable into the passed string and
    // replace it correspondingly
    for (auto iter = _stringIntMem.sStringVars.begin(); iter != _stringIntMem.sStringVars.end(); ++iter)
    {
        __nPos = nPos;

        // Examine all occurences of the current variable in the
        // string
        while (sLine.find(iter->first, __nPos) != string::npos)
        {
            __nPos = sLine.find(iter->first, __nPos)+1;

            // Appended opening parenthesis indicates a function
            if (sLine[__nPos+(iter->first).length()-1] == '(')
                continue;

            // Check, whether the found occurence is correctly
            // delimited and replace it. If the match is at the
            // beginning of the command line, it serves a special
            // treatment
            if (__nPos == 1)
            {
                // Check with delimiter
                if (checkStringvarDelimiter(" " + sLine.substr(0, (iter->first).length()+1)) && !isInQuotes(sLine, 0, true))
                {
                    // Replace it with standard function signature or its value
                    if (sLine[(iter->first).length()] == '.')
                    {
                        replaceStringMethod(sLine, 0, (iter->first).length(), "\"" + iter->second + "\"");
                    }
                    else
                    {
                        sLine.replace(0, (iter->first).length(), "\"" + iter->second + "\"");
                    }
                }

                continue;
            }

            // Check with delimiter
            if (checkStringvarDelimiter(sLine.substr(__nPos-2, (iter->first).length()+2)) && !isInQuotes(sLine, __nPos-1, true))
            {
                // Replace it with standard function signature or its value
                if (sLine[__nPos+(iter->first).length()-1] == '.')
                {
                    replaceStringMethod(sLine, __nPos-1, (iter->first).length(), "\"" + iter->second + "\"");
                }
                else
                    sLine.replace(__nPos-1, (iter->first).length(), "\"" + iter->second + "\"");
            }
        }
    }

    return;
}

// This public member function creates or updates a string variable
// and fills it with the passed value
void StringMemory::setStringValue(const string& sVar, const string& sValue)
{
    static const string sVALIDCHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890~";

    // Variable names cannot start with a number
    if (sVar[0] >= '0' && sVar[0] <= '9')
        throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER, "", SyntaxError::invalid_position, sVar);

    // Compare every character to the list of valid characters,
    // to ensure that the name is absolutly valid
    for (unsigned int i = 0; i < sVar.length(); i++)
    {
        if (sVALIDCHARACTERS.find(sVar[i]) == string::npos)
        {
            throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_CONTAIN, "", SyntaxError::invalid_position, sVar.substr(i,1));
        }
    }

    // Create or update the variable name with the passed value.
    // Omit the surrounding quotation marks
    if (sValue[0] == '"' && sValue[sValue.length()-1] == '"')
        _stringIntMem.sStringVars[sVar] = sValue.substr(1,sValue.length()-2);
    else
        _stringIntMem.sStringVars[sVar] = sValue;

    // If the current string variables value contains further
    // quotation marks, mask them correctly
    if (_stringIntMem.sStringVars[sVar].find('"') != string::npos)
    {
        unsigned int nPos = 0;

        while (_stringIntMem.sStringVars[sVar].find('"', nPos) != string::npos)
        {
            nPos = _stringIntMem.sStringVars[sVar].find('"', nPos);

            // Is the found quotation mark masked by a backslash?
            if (_stringIntMem.sStringVars[sVar][nPos-1] == '\\')
            {
                nPos++;
                continue;
            }
            else
            {
                _stringIntMem.sStringVars[sVar].insert(nPos,1,'\\');
                nPos += 2;
                continue;
            }
        }
    }

    return;
}

// This public member function removes the selected string variable
// from memory
void StringMemory::removeStringVar(const string& sVar)
{
    // Do nothing, if no string variables were declared
    if (!_stringIntMem.sStringVars.size())
        return;

    // Delete the string variable, if it exists
    auto iter = _stringIntMem.sStringVars.find(sVar);
    if (iter != _stringIntMem.sStringVars.end())
        _stringIntMem.sStringVars.erase(iter);
}

