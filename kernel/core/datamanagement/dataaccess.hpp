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
#include "memorymanager.hpp"
#include "table.hpp"
#include "../settings.hpp"
#include "../structures.hpp"

bool parser_CheckMultArgFunc(const std::string&, const std::string&);

std::string getDataElements(std::string& sLine, mu::Parser& _parser, MemoryManager& _data, const Settings& _option, bool bReplaceNANs = true);
void replaceDataEntities(std::string&, const std::string&, MemoryManager&, mu::Parser&, const Settings&, bool);
bool getData(const std::string& sTableName, Indices& _idx, const MemoryManager& _data, MemoryManager& _cache, int nDesiredCols = 2, bool bSort = true);
NumeRe::Table parser_extractData(const std::string& sDataExpression, mu::Parser& _parser, MemoryManager& _data, const Settings& _option);
bool isNotEmptyExpression(const std::string&);
bool isClusterCandidate(std::string& sLine, std::string& sCluster, bool doCut = true);
double getDataFromObject(const std::string& sObject, long long int i, long long int j, bool isCluster);
Indices getIndicesForPlotAndFit(const std::string& sExpression, std::string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster);

Indices getIndices(StringView sCmd, mu::Parser& _parser, MemoryManager& _data, const Settings& _option) __attribute__ ((deprecated));
void getIndices(StringView sCmd, Indices& _idx, mu::Parser& _parser, MemoryManager& _data, const Settings& _option);

inline bool isValidIndexSet(const Indices& _idx)
{
    return _idx.row.isValid() && _idx.col.isValid();
}


/////////////////////////////////////////////////
/// \brief This class is defined to abstrahize
/// the determination of the correct data object
/// and the calculation of the corresponding
/// indices.
/////////////////////////////////////////////////
class DataAccessParser
{
    private:
        std::string sDataObject;
        Indices idx;
        bool bIsCluster;

    public:
        DataAccessParser(StringView sCommand);
        void evalIndices();
        std::string& getDataObject();
        Indices& getIndices();
        bool isCluster() const;
};


#endif // DATAACCESS_HPP


