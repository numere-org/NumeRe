/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef TABLECOLUMN_HPP
#define TABLECOLUMN_HPP

#include <string>
#include <vector>
#include <memory>
#include "../ParserLib/muParserDef.h"
#include "../structures.hpp"

/////////////////////////////////////////////////
/// \brief Abstract table column, which allows
/// using it to compose the data table in each
/// Memory instance.
/////////////////////////////////////////////////
struct TableColumn
{
    enum ColumnType
    {
        TYPE_NONE,
        TYPE_VALUE,
        TYPE_STRING
    };

    std::string m_sHeadLine;
    ColumnType m_type;

    TableColumn() : m_type(TYPE_NONE) {}
    virtual ~TableColumn() {}

    std::vector<std::string> getValueAsString(const VectorIndex& idx) const;
    std::vector<mu::value_type> getValue(const VectorIndex& idx) const;

    virtual std::string getValueAsString(int elem) const = 0;
    virtual mu::value_type getValue(int elem) const = 0;

    virtual void setValue(int elem, const std::string& sValue) = 0;
    virtual void setValue(int elem, const mu::value_type& vValue) = 0;
    virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& vValue) = 0;
    virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) = 0;
    virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) = 0;

    TableColumn* copy() const;
    virtual TableColumn* copy(const VectorIndex& idx) const = 0;
    virtual void assign(const TableColumn* column) = 0;
    virtual void insert(const VectorIndex& idx, const TableColumn* column) = 0;
    virtual void deleteElements(const VectorIndex& idx) = 0;
    virtual void shrink() = 0;

    virtual int compare(int i, int j) const = 0;
    virtual bool isValid(int elem) const = 0;

    virtual bool asBool(int elem) const = 0;

    virtual size_t size() const = 0;
    virtual size_t getBytes() const = 0;

    static std::string getDefaultColumnHead(size_t colNo);
};


/////////////////////////////////////////////////
/// \brief Typedef for simplifying the usage of
/// a smart pointer in combination with a
/// TableColumn instance.
/////////////////////////////////////////////////
typedef std::unique_ptr<TableColumn> TblColPtr;

/////////////////////////////////////////////////
/// \brief This typedef represents the actual
/// table, which is implemented using a
/// std::vector.
/////////////////////////////////////////////////
typedef std::vector<TblColPtr> TableColumnArray;



#endif // TABLECOLUMN_HPP



