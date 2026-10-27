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

    int getElemsInColumn(size_t col) const;
    int getFilledElemsInColumn(size_t col) const;
    bool isValidColumn(size_t col) const;

    TableColumnArray& getCols();

    TableSheet& operator=(const TableSheet& other);

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


struct TableData
{
    TableSheetArray m_sheets;

    void create(const std::vector<size_t>& cols_per_sheet);
    void addSheet(const std::string& sSheetName, size_t cols);

    TableData& operator=(const TableData& other);
    TableData& operator=(TableData&& other);

    void resize(size_t sSheets);
    void reserve(size_t sSheets);

    size_t rows() const;
    size_t cols() const;
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

    void write(const mu::Value& _value,
               const TableIndices& idx);
    void write(const mu::Array& _values,
               const TableIndices& idx);

    void erase(const TableIndices& idx);

    void clear();
};

#endif // TABLEDATA_HPP


