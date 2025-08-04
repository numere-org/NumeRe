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
        DataAccessParser();
        DataAccessParser(StringView sCommand, bool isAssignment, bool isLocal = false);
        DataAccessParser(const DataAccessParser& _accessParser);
        DataAccessParser(DataAccessParser&& moved) = default;
        DataAccessParser& operator=(const DataAccessParser& copied) = default;
        void evalIndices(bool asMatrix = true);
        std::string& getDataObject();
        std::string getIndexString();
        Indices& getIndices();
        bool isCluster() const;
        bool isMatrix() const;
        std::vector<size_t> getDataGridDimensions() const;
};


enum DataOptions
{
    NO_OPTION = 0x0,
    REPLACE_NAN = 0x1,
    INSERT_STRINGS = 0x2
};


bool parser_CheckMultArgFunc(StringView, StringView);

std::string getDataElements(std::string& sLine, mu::Parser& _parser, MemoryManager& _data, int options = REPLACE_NAN);
void replaceDataEntities(std::string&, const std::string&, MemoryManager&, mu::Parser&, int options = NO_OPTION);
Memory* extractRange(const std::string& sCmd, DataAccessParser& _accessParser, int nDesiredCols = -1, bool bSort = false);
bool isNotEmptyExpression(StringView);
mu::Value getDataFromObject(const std::string& sObject, long long int i, long long int j, bool isCluster);
mu::Array getDataFromObject(const std::string& sObject, const VectorIndex& vRows, long long int j, bool isCluster);
DataAccessParser getAccessParserForPlotAndFit(StringView sExpression);
Indices getIndicesForPlotAndFit(const std::string& sExpression, std::string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster);

Indices getIndices(StringView sCmd, mu::Parser& _parser, MemoryManager& _data, bool isAssignment);
void getIndices(StringView sCmd, Indices& _idx, mu::Parser& _parser, MemoryManager& _data, bool isAssignment);

inline bool isValidIndexSet(const Indices& _idx)
{
    return _idx.row.isValid() && _idx.col.isValid();
}

std::vector<size_t> getDataGridDimensions(const Indices& _idx, const std::string& sTableName);
size_t findAssignmentOperator(StringView sCmd);


#endif // DATAACCESS_HPP


