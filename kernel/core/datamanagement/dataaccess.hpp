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
        const std::string& getDataObject() const;
        std::string getIndexString();
        Indices& getIndices();
        const Indices& getIndices() const;
        bool isCluster() const;
        bool isMatrix() const;
        std::vector<size_t> getDataGridDimensions() const;
};


/////////////////////////////////////////////////
/// \brief This class provides a view to either a
/// referenced table instance or to an Array of
/// values. It is intended to abstrahize away the
/// slight differences between accesses to a
/// table or an Array.
/////////////////////////////////////////////////
class DataView
{
    private:
        mu::Array m_array;
        DataAccessParser m_access;
        std::string m_dataName;
        size_t m_axisCount;

    public:
        DataView(const mu::Array& arr, const std::string& dataName) : m_array(arr), m_dataName(dataName), m_axisCount(0) { }
        DataView(const DataAccessParser& acc) : m_access(acc), m_axisCount(0)
        {
            m_dataName = m_access.getDataObject() + "(";
        }
        DataView(const DataAccessParser&& acc) : m_access(acc), m_axisCount(0)
        {
            m_dataName = m_access.getDataObject() + "(";
        }
        DataView(const DataView&) = default;
        DataView(DataView&&) = default;

        void sortTable();

        /////////////////////////////////////////////////
        /// \brief Evaluates the indices, if they have
        /// open ends depending on whether a matrix or
        /// just multiple columns are accepted. Only
        /// necessary, if a table is referenced.
        ///
        /// \param asMatrix bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        void evalIndices(bool asMatrix = false)
        {
            if (isTable())
                m_access.evalIndices(asMatrix);
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of dimensions
        /// available in the referenced data source.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t getDims() const
        {
            if (isTable())
                return m_access.getIndices().col.size() > 1 ? 2ull : 1ull;

            return m_array.getDims();
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of rows referenced
        /// in the data source.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rows() const
        {
            if (isTable())
                return m_access.getIndices().row.size();

            return m_array.rows();
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of columns
        /// referenced in the data source.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t cols() const
        {
            if (isTable())
                return m_access.getIndices().col.size() - m_axisCount;

            return m_array.cols();
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if a table has been
        /// referenced.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isTable() const
        {
            return m_access.getDataObject().length();
        }

        /////////////////////////////////////////////////
        /// \brief Get a human-readable version of the
        /// referenced data source.
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        const std::string& getDataName() const
        {
            return m_dataName;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the table access indices. Only
        /// meaningful for table accesses.
        ///
        /// \return const Indices&
        ///
        /////////////////////////////////////////////////
        const Indices& getTableIndices() const
        {
            return m_access.getIndices();
        }

        /////////////////////////////////////////////////
        /// \brief Returns the internal array. Only
        /// meaningful, if an internal array is present.
        ///
        /// \return const mu::Array&
        ///
        /////////////////////////////////////////////////
        const mu::Array& getArray() const
        {
            return m_array;
        }

        bool isValueLike() const;
        bool isValid() const;

        mu::Array getAxis(size_t axis) const;
        bool reserveAxes(size_t axisCount, bool asDataGrid = true);

        size_t getAxisCount() const
        {
            return m_axisCount;
        }

        mu::Value get(size_t i, size_t j) const;
        mu::Array get(const VectorIndex& rows, const VectorIndex& cols) const;

        mu::Value sum() const;
        mu::Value min() const;
        mu::Value max() const;
        mu::Value med() const;
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


