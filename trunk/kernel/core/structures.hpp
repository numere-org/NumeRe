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

struct Indices
{
    long long int nI[2];
    long long int nJ[2];
    vector<long long int> vI;
    vector<long long int> vJ;
    string sCompiledAccessEquation;
};

struct Match
{
    string sString;
    unsigned int nPos;
};

struct Line
{
    string sDesc;
    string sStyle;
    double dPos;
};

struct Axis
{
    string sLabel;
    string sStyle;
    double dMin;
    double dMax;
};

struct Returnvalue
{
    vector<double> vNumVal;
    vector<string> vStringVal;
};

struct RetoqueRegion
{
    vector<vector<double> > vDataArray;
    vector<vector<bool> > vValidationArray;
    double dMedian;
};



#endif
