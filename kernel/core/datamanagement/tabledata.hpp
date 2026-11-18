/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#ifndef TABLEDATA_HPP
#define TABLEDATA_HPP

#include <string>
#include <vector>

#include "sorter.hpp"
#include "tablecolumn.hpp"

/////////////////////////////////////////////////
/// \brief This structure is a simple container
/// to bundle the three VectorIndices as
/// references together and make managing them
/// doable.
/////////////////////////////////////////////////
struct TableIndices
{
    const VectorIndex& m_rows;
    const VectorIndex& m_cols;
    const VectorIndex& m_sheets;

    TableIndices(const VectorIndex& _rows,
                 const VectorIndex& _cols = VectorIndex(0),
                 const VectorIndex& _sheets = VectorIndex(0))
        : m_rows(_rows), m_cols(_cols), m_sheets(_sheets)
    {
        //
    }

    bool returnsMatrix() const;
    bool writeSingleSheet(const mu::Array& _values) const;
};


/////////////////////////////////////////////////
/// \brief This structure represents a single
/// sheet in the complete table data structure.
/// Sheets are multiple columns of data bundled
/// together into a two-dimensional data array.
/// Via the Sorter mixin, they do support
/// efficient sorting of the data on their
/// columns, especially hierarchical sorting.
/////////////////////////////////////////////////
struct TableSheet : public Sorter
{
    mutable size_t m_calculatedRows = 0;
    std::string m_sheetName;
    bool bSortCaseInsensitive;
    TableColumnArray m_columns;

    void reorderColumn(const VectorIndex& vIndex, const VectorIndex& original, int col = 0);
    virtual int compare(int i, int j, int col) override;
    virtual bool isValue(int line, int col) override;
    std::vector<int> sortElements(const VectorIndex& _vRow,
                                  const VectorIndex& _vCol,
                                  const std::string& sSortingExpression = "");

    TableSheet(const std::string& sName, size_t cols = 0);

    size_t rows() const;
    size_t cols() const;

    size_t reserved_cols() const;
    int getElemsInColumn(size_t col) const;
    int getFilledElemsInColumn(size_t col) const;
    bool isValidColumn(size_t col) const;

    TableColumnArray& getCols();

    TableSheet& operator=(const TableSheet& other);

    /////////////////////////////////////////////////
    /// \brief Get a reference to the i-th column.
    ///
    /// \param i size_t
    /// \return TblColPtr&
    ///
    /////////////////////////////////////////////////
    TblColPtr& operator[](size_t i)
    {
        return m_columns.at(i);
    }

    /////////////////////////////////////////////////
    /// \brief Get a const reference to the i-th
    /// column.
    ///
    /// \param i size_t
    /// \return const TblColPtr&
    ///
    /////////////////////////////////////////////////
    const TblColPtr& operator[](size_t i) const
    {
        return m_columns.at(i);
    }

    void resize(const size_t nCols);
    void reserve(const size_t nCols);
    void shrink(const VectorIndex& _vCol);

    void writeHeading(const std::string& sHeading, int col);
    void writeHeadings(const mu::Array& _headings, const VectorIndex& _cols);
    void write(const mu::Value& _value, int row, int col, TableColumn::ColumnType type = TableColumn::TYPE_NONE);
    void write(const mu::Value& _value, const VectorIndex& _rows, const VectorIndex& _cols);
    void write(const mu::Array& _values, const VectorIndex& _rows, const VectorIndex& _cols);

    bool empty() const;
    void clear();
};


typedef std::unique_ptr<TableSheet> TblSheetPtr;
typedef std::vector<TblSheetPtr> TableSheetArray;


/////////////////////////////////////////////////
/// \brief This structure bundles all sheets of a
/// table together. It provides the major
/// interfaces for reading and writing in a
/// three-dimensional manner and is therefore
/// similar to the functionality of a three-
/// dimensional array.
/////////////////////////////////////////////////
struct TableData
{
    TableSheetArray m_sheets;

    void create(const std::vector<size_t>& cols_per_sheet);
    void addSheet(const std::string& sSheetName, size_t cols);

    TableData& operator=(const TableData& other);
    TableData& operator=(TableData&& other);

    void resize(size_t nSheets);
    void reserve(size_t nSheets, size_t _nCols = 0);

    size_t rows() const;
    size_t cols() const;
    size_t reserved_cols() const;
    size_t sheets() const;
    bool empty() const;

    size_t getSize() const;

    TableColumn::ColumnType getType(const VectorIndex& _vCols,
                                    const VectorIndex& _vSheets = VectorIndex(0)) const;

    mu::Value read(size_t row, size_t col, size_t sheet) const;
    mu::Array read(const TableIndices& idx) const;
    void read(const TableIndices& idx, mu::Variable* target) const;

    TableData* extract(const TableIndices& idx) const;
    TableSheet& getSheet(size_t sheet);
    const TableSheet& getSheet(size_t sheet) const;

    /////////////////////////////////////////////////
    /// \brief Get a reference to the i-th sheet.
    ///
    /// \param sheet size_t
    /// \return TableSheet&
    ///
    /////////////////////////////////////////////////
    TableSheet& operator[](size_t sheet)
    {
        return getSheet(sheet);
    }

    /////////////////////////////////////////////////
    /// \brief Get a const reference to the i-th
    /// sheet.
    ///
    /// \param sheet size_t
    /// \return const TableSheet&
    ///
    /////////////////////////////////////////////////
    const TableSheet& operator[](size_t sheet) const
    {
        return getSheet(sheet);
    }

    void write(const mu::Value& _value,
               const TableIndices& idx);
    void write(const mu::Array& _values,
               const TableIndices& idx);

    void erase(const TableIndices& idx);

    void clear();
};

#endif // TABLEDATA_HPP


