/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <string>
#include <vector>

using namespace std;

// Structure for the indices
struct Indices
{
    long long int nI[2];
    long long int nJ[2];
    vector<long long int> vI;
    vector<long long int> vJ;
    string sCompiledAccessEquation;

    // Default constructor sets the casual indices to -1
    Indices()
        {
            for (size_t i = 0; i < 2; i++)
            {
                this->nI[i] = -1;
                this->nJ[i] = -1;
            }
        }
};

// Structure for the findCommand function
struct Match
{
    string sString;
    unsigned int nPos;
};

// Structure for the horizontal and vertical lines in plots
struct Line
{
    string sDesc;
    string sStyle;
    double dPos;

    Line() : sDesc(""), sStyle("k;2"), dPos(0.0) {}
};

// Structure for the axes in plots
struct Axis
{
    string sLabel;
    string sStyle;
    double dMin;
    double dMax;
};

// Structure as wrapper for the return value of procedures (which may be numerical or string values or a mixture of both)
struct Returnvalue
{
    vector<double> vNumVal;
    vector<string> vStringVal;
};

// Structure for the retouch functionality
struct RetoqueRegion
{
    vector<vector<double> > vDataArray;
    vector<vector<bool> > vValidationArray;
    double dMedian;
};

// Structure for the four standard variables
struct Integration_Vars
{
    string sName[4] = {"x", "y", "z", "t"};
    double vValue[4][4];
};

// Structure for the sorting functionality: used for the recursive definition of
// the index columns for sorting
struct ColumnKeys
{
    int nKey[2];
    // Contains a recursive pointer
    ColumnKeys* subkeys;

    // Default constructor
    ColumnKeys() : nKey{-1,-1}, subkeys(nullptr) {}
    // Destructor recursively deletes the stored pointers
    ~ColumnKeys()
        {
            if (subkeys)
                delete subkeys;
        }
};

#endif
