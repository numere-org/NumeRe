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


#ifndef DATAACCESS_HPP
#define DATAACCESS_HPP

#include <string>
#include "../ParserLib/muParser.h"
#include "datafile.hpp"
#include "table.hpp"
#include "../settings.hpp"
#include "../structures.hpp"

using namespace std;
using namespace mu;

bool parser_CheckMultArgFunc(const string&, const string&);
void parser_CheckIndices(long long int&, long long int&);
void parser_CheckIndices(int&, int&);

string getDataElements(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option, bool bReplaceNANs = true);
void replaceDataEntities(string&, const string&, Datafile&, Parser&, const Settings&, bool);
bool getData(const string& sTableName, Indices& _idx, const Datafile& _data, Datafile& _cache, int nDesiredCols = 2, bool bSort = true);
NumeRe::Table parser_extractData(const string& sDataExpression, Parser& _parser, Datafile& _data, const Settings& _option);
bool isNotEmptyExpression(const string&);
bool isClusterCandidate(string& sLine, string& sCluster, bool doCut = true);
double getDataFromObject(const string& sObject, long long int i, long long int j, bool isCluster);
Indices getIndicesForPlotAndFit(const string& sExpression, string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster);

Indices getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);

inline bool isValidIndexSet(const Indices& _idx)
{
    return _idx.row.isValid() && _idx.col.isValid();
}

// This class is defined to abstract the determination
// of the correct data object and the calculation of
// the corresponding indices
class DataAccessParser
{
    private:
        string sDataObject;
        Indices idx;
        bool bIsCluster;

    public:
        DataAccessParser(const string& sCommand);
        string& getDataObject();
        Indices& getIndices();
        bool isCluster();
};


#endif // DATAACCESS_HPP


