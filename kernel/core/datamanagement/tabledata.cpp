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

#include "tabledata.hpp"
#include "tablecolumnimpl.hpp"

#define DEFAULT_COL_TYPE ValueColumn
#define ROWS 1
#define COLS 2


bool TableIndices::returnsMatrix() const
{
    bool r_vec = m_rows.size() > 1;
    bool c_vec = m_cols.size() > 1;
    bool s_vec = m_sheets.size() > 1;

    return r_vec + s_vec + c_vec >= 2;
}


bool TableIndices::writeSingleSheet(const mu::Array& _values) const
{
    bool r_vec = m_rows.size() > 1;
    bool c_vec = m_cols.size() > 1;
    bool s_vec = m_sheets.size() > 1;

    return !s_vec || r_vec + c_vec >= (int)_values.getDims();
}





TableSheet::TableSheet(const std::string& sName, size_t cols) : m_sheetName(sName)
{
    m_calculatedRows = 0;
    m_columns.resize(cols);
}


/////////////////////////////////////////////////
/// \brief Override for the virtual Sorter class
/// member function. Returns 0, if both elements
/// are equal, -1 if element i is smaller than
/// element j and 1 otherwise.
///
/// \param i int
/// \param j int
/// \param col int
/// \return int
///
/////////////////////////////////////////////////
int TableSheet::compare(int i, int j, int col)
{
    if (isValidColumn(col))
        return m_columns[col]->compare(i, j, bSortCaseInsensitive);

    return 0;
}


/////////////////////////////////////////////////
/// \brief Override for the virtual Sorter class
/// member function. Returns true, if the
/// selected element is a valid value.
///
/// \param line int
/// \param col int
/// \return bool
///
/////////////////////////////////////////////////
bool TableSheet::isValue(int line, int col)
{
    if (isValidColumn(col))
        return m_columns[col]->isValid(line);

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function simply reorders
/// the contents of the selected column using the
/// passed index vector.
///
/// \param vIndex const VectorIndex&
/// \param original const VectorIndex&
/// \param col int
/// \return void
///
/////////////////////////////////////////////////
void TableSheet::reorderColumn(const VectorIndex& vIndex, const VectorIndex& original, int col)
{
    if (isValidColumn(col))
    {
        TblColPtr copied(m_columns[col]->copy(vIndex));
        m_columns[col]->insert(original, copied.get());
        m_columns[col]->shrink();
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the interface
/// function for the Sorter class. It will pre-
/// evaluate the passed parameters and redirect
/// the control to the corresponding sorting
/// function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param sSortingExpression const std::string&
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> TableSheet::sortElements(const VectorIndex& _vRow, const VectorIndex& _vCol, const std::string& sSortingExpression)
{
    if (!rows())
        return std::vector<int>();

    bool bError = false;
    bool bReturnIndex = false;
    bool bInternalIndex = false;
    bSortCaseInsensitive = findParameter(sSortingExpression, "ignorecase");
    int nSign = 1;

    _vRow.setOpenEndIndex(rows()-1);
    _vCol.setOpenEndIndex(cols()-1);

    std::vector<int> vIndex;

    // Determine the sorting direction
    if (findParameter(sSortingExpression, "desc"))
        nSign = -1;

    // Prepare the sorting index
    vIndex = _vRow.getVector();

    // Evaluate, whether an index shall be returned
    // (instead of actual reordering the columns)
    if (findParameter(sSortingExpression, "index"))
        bReturnIndex = true;

    // Evaluate, whether an index shall be returned
    // (instead of actual reordering the columns)
    if (findParameter(sSortingExpression, "internal"))
        bInternalIndex = true;

    // Is a column group selected or do we actually
    // sort everything?
    if (!findParameter(sSortingExpression, "cols", '=') && !findParameter(sSortingExpression, "c", '='))
    {
        // Make a copy of the global index for the private threads
        std::vector<int> vPrivateIndex = vIndex;

        // Sort everything independently (we use vIndex from
        // the outside, we therefore must declare it as firstprivate)
        #pragma omp parallel for firstprivate(vPrivateIndex)
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            // Change for OpenMP
            if (j && bReturnIndex)
                continue;

            // Sort the current column
            if (!qSort(&vPrivateIndex[0], vPrivateIndex.size(), _vCol[j], 0, vPrivateIndex.size()-1, nSign))
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);

            // Abort after the first column, if
            // an index shall be returned
            // Continue is a change for OpenMP
            if (bReturnIndex)
            {
                vIndex = vPrivateIndex;
                continue;
            }

            // Actually reorder the column
            reorderColumn(vPrivateIndex, _vRow, _vCol[j]);

            // Reset the sorting index
            vPrivateIndex = _vRow.getVector();
        }
    }
    else
    {
        // Sort groups of columns (including
        // hierarchical sorting)
        std::string sCols = "";

        // Find the column group definition
        if (findParameter(sSortingExpression, "cols", '='))
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "cols", '=') + 4);
        else
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "c", '=') + 1);

        // As long as the column group definition
        // has a length
        if (sCols.length())
        {
            // Get a new column keys instance
            ColumnKeys* keys = evaluateKeyList(sCols, _vCol.size());

            // Ensure that we obtained an actual
            // instance
            if (!keys)
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);

            // Go through the group definition
            for (size_t j = 0; j < keys->cols.size(); j++)
            {
                if (keys->cols[j] >= (int)_vCol.size() || keys->cols[j] < 0)
                {
                    delete keys;
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sSortingExpression, SyntaxError::invalid_position);
                }

                // Sort the current key list level
                // independently
                if (!qSort(&vIndex[0], vIndex.size(), _vCol[keys->cols[j]], 0, vIndex.size()-1, nSign))
                {
                    delete keys;
                    throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                }

                // Subkey list: sort the subordinate group
                // depending on the higher-level key group
                if (keys->subkeys && keys->subkeys->subkeys)
                {
                    if (!sortSubList(&vIndex[0], vIndex.size(), keys, 0, vIndex.size()-1, _vCol, nSign, cols()))
                    {
                        delete keys;
                        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                    }
                }

                // Break, if the index shall be returned
                if (bReturnIndex)
                    break;

                // Actually reorder the current column
                reorderColumn(vIndex, _vRow, _vCol[keys->cols[j]]);

                // Obtain the subkey list
                ColumnKeys* subKeyList = keys->subkeys;

                // As long as a subkey list is available
                while (subKeyList)
                {
                    // Reorder the subordinate key list
                    for (size_t _j = 0; _j < subKeyList->cols.size(); _j++)
                    {
                        if (subKeyList->cols[_j] >= (int)_vCol.size() || subKeyList->cols[_j] < 0)
                            throw SyntaxError(SyntaxError::INVALID_INDEX, sSortingExpression, SyntaxError::invalid_position);

                        reorderColumn(vIndex, _vRow, _vCol[subKeyList->cols[_j]]);
                    }

                    // Find the next subordinate list
                    subKeyList = subKeyList->subkeys;
                }

                // Hierarchical sorting does not allow havin multiple columns in
                // their parent hierarchy
                if (keys->subkeys && keys->subkeys->subkeys)
                    break;

                // Reset the sorting index for the next column
                vIndex = _vRow.getVector();
            }

            // Free the occupied memory
            delete keys;
        }
    }

    // Number of lines might have changed
    m_calculatedRows = 0;

    // Increment each index value, if the index
    // vector shall be returned
    if (bReturnIndex && !bInternalIndex)
    {
        for (size_t i = 0; i < vIndex.size(); i++)
            vIndex[i]++;
    }

    if (bError || !bReturnIndex)
        return std::vector<int>();

    return vIndex;
}


size_t TableSheet::rows() const
{
    if (m_columns.size())
    {
        if (m_calculatedRows)
            return m_calculatedRows;

        size_t nReturn = 0;

        for (const TblColPtr& col : m_columns)
        {
            if (col && col->size() > nReturn)
                nReturn = col->size();
        }

        // Cache the number of lines until invalidation
        // for faster access
        m_calculatedRows = nReturn;

        return nReturn;
    }

    return 0;
}


size_t TableSheet::cols() const
{
    for (int j = m_columns.size() - 1; j >= 0; j--)
    {
        if (m_columns[j] && m_columns[j]->size())
            return j+1;
    }

    return 0;
}


size_t TableSheet::reserved_cols() const
{
    return m_columns.size();
}


int TableSheet::getElemsInColumn(size_t col) const
{
    if (isValidColumn(col))
        return m_columns[col]->size();

    return 0;
}


int TableSheet::getFilledElemsInColumn(size_t col) const
{
    if (isValidColumn(col))
        return m_columns[col]->getNumFilledElements();

    return 0;
}


bool TableSheet::isValidColumn(size_t col) const
{
    return m_columns.size() > col && m_columns[col];
}


TableColumnArray& TableSheet::getCols()
{
    return m_columns;
}


TableSheet& TableSheet::operator=(const TableSheet& other)
{
    m_columns.resize(other.m_columns.size());

    #pragma omp parallel for
    for (size_t i = 0; i < other.m_columns.size(); i++)
    {
        if (!other.m_columns[i])
            continue;

        switch (other.m_columns[i]->m_type)
        {
            case TableColumn::TYPE_DATETIME:
                m_columns[i].reset(new DateTimeColumn);
                break;
            case TableColumn::TYPE_DURATION:
                m_columns[i].reset(new DurationColumn);
                break;
            case TableColumn::TYPE_STRING:
                m_columns[i].reset(new StringColumn);
                break;
            case TableColumn::TYPE_LOGICAL:
                m_columns[i].reset(new LogicalColumn);
                break;
            case TableColumn::TYPE_CATEGORICAL:
                m_columns[i].reset(new CategoricalColumn);
                break;

            // These labels are only for getting warnings
            // if new column types are added
            case TableColumn::TYPE_NONE:
            case TableColumn::VALUELIKE:
            case TableColumn::STRINGLIKE:
            case TableColumn::TYPE_MIXED:
                break;
            default:
            {
                if (TableColumn::isValueType(other.m_columns[i]->m_type))
                    m_columns[i].reset(createValueTypeColumn(other.m_columns[i]->m_type));
            }
        }

        m_columns[i]->assign(other.m_columns[i].get());
    }

    m_calculatedRows = other.m_calculatedRows;
    m_sheetName = other.m_sheetName;

    return *this;
}


void TableSheet::resize(const size_t nCols)
{
    m_columns.resize(nCols);
}


void TableSheet::reserve(const size_t nCols)
{
    m_columns.resize(std::max(nCols, m_columns.size()));
}


void TableSheet::shrink(const VectorIndex& _vCol)
{
    // Shrink each column
    for (size_t i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0 || !isValidColumn(_vCol[i]))
            continue;

        TblColPtr& col = m_columns[_vCol[i]];

        if (col)
            col->shrink();

        if (col && !col->size())
            col.reset(nullptr);
    }

    m_calculatedRows = 0;

    // Remove obsolete columns
    for (int i = m_columns.size()-1; i >= 0; i--)
    {
        if (m_columns[i])
        {
            m_columns.resize(i+1);
            return;
        }
    }

    // if this place is reached, delete everything
    m_columns.clear();
}


void TableSheet::writeHeading(const std::string& sHeading, int col)
{
    if (col < 0)
        return;

    reserve(col+1);

    if (!m_columns[col])
        m_columns[col].reset(new DEFAULT_COL_TYPE);

    m_columns[col]->m_sHeadLine = sHeading;
}


void TableSheet::writeHeadings(const mu::Array& _headings, const VectorIndex& _cols)
{
    _cols.setOpenEndIndex(_cols.front()+_headings.size()-1);
    reserve(_cols.max()+1);

    for (size_t i = 0; i < _cols.size(); i++)
    {
        int c = _cols[i];

        if (c < 0)
            continue;

        if (!m_columns[c])
            m_columns[c].reset(new DEFAULT_COL_TYPE);

        m_columns[c]->m_sHeadLine = _headings.get(i).getStr();
    }
}


void TableSheet::write(const mu::Value& _value, int row, int col, TableColumn::ColumnType type)
{
    if ((!m_columns.size() && !_value.isValid()) || col < 0)
        return;

    if ((int)m_columns.size() <= col)
        reserve(col+1);

    if (row == VectorIndex::STRING)
    {
        writeHeading(_value.getStr(), col);
        return;
    }
    else if (row < 0)
        return;

    promote_if_needed(m_columns[col], col, type != TableColumn::TYPE_NONE ? type : to_column_type(_value));
    m_columns[col]->set(row, _value);

    if (m_calculatedRows && (!_value.isValid() || row >= (int)m_calculatedRows))
        m_calculatedRows = 0;
}


void TableSheet::write(const mu::Value& _value, const VectorIndex& _rows, const VectorIndex& _cols)
{
    bool rewriteColumn = false;

    if (_rows.front() == 0 && _rows.isOpenEnd())
        rewriteColumn = true;

    _rows.setOpenEndIndex(std::max(_rows.front(), (int)rows()) - 1);
    _cols.setOpenEndIndex(std::max(_cols.front(), (int)cols()) - 1);
    TableColumn::ColumnType t = to_column_type(_value);

    for (size_t j = 0; j < _cols.size(); j++)
    {
        int c = _cols[j];

        for (size_t i = 0; i < _rows.size(); i++)
        {
            int r = _rows[i];

            if (!i && rewriteColumn && (int)m_columns.size() > c)
                convert_for_overwrite(m_columns[c], c, t);

            write(_value, r, c, t);
        }
    }
}


void TableSheet::write(const mu::Array& _values, const VectorIndex& _rows, const VectorIndex& _cols)
{
    TableColumn::ColumnType t = to_column_type(_values);
    bool rewriteColumn = false;

    if (_rows.front() == 0 && _rows.isOpenEnd())
        rewriteColumn = true;

    if (_rows.isString())
    {
        writeHeadings(_values, _cols);
        return;
    }

    int nDirection = ROWS;

    if (_values.size() == 1)
    {
        if (_values.get(0).isArray())
            write(_values.get(0).getArray(), _rows, _cols);
        else
            write(_values.front(), _rows, _cols);

        return;
    }

    if (_values.isMatrix() && !_values.isVector())
    {
        if (cols() != _values.cols()
            && rows() != _values.rows()
            && _rows.isFullRange(rows()-1)
            && _cols.isFullRange(cols()-1))
            clear();

        _rows.setOpenEndIndex(_rows.front() + _values.rows() - 1);
        _cols.setOpenEndIndex(_cols.front() + _values.cols() - 1);

        // Prepare the target size
        reserve(_cols.max()+1);

        // Write the contents to the table
        for (size_t j = 0; j < _values.cols(); j++)
        {
            int c = _cols[j];

            if (c < 0)
                continue;

            for (size_t i = 0; i < _values.rows(); i++)
            {
                int r = _rows[i];

                if (r == VectorIndex::INVALID)
                    break;

                if (!i && rewriteColumn && (int)m_columns.size() > c)
                    convert_for_overwrite(m_columns[c], c, t);

                write(_values.get(i, j), r, c, t);
            }
        }
    }
    else // vectors are accepted here for backwards compatibility
    {
        // Prefer writing columns
        if (_rows.size() > 1)
            _cols.setOpenEndIndex(VectorIndex::INVALID);
        else
            _cols.setOpenEndIndex(_cols.front() + _values.size() - 1);

        _rows.setOpenEndIndex(_rows.front() + _values.size() - 1);

        if (_rows.size() > 1)
            nDirection = COLS;
        else if (_cols.size() > 1)
            nDirection = ROWS;

        for (size_t j = 0; j < _cols.size(); j++)
        {
            int c = _cols[j];

            if (c < 0)
                continue;

            for (size_t i = 0; i < _rows.size(); i++)
            {
                int r = _rows[i];

                if (r == VectorIndex::INVALID)
                    break;

                if (nDirection == COLS)
                {
                    if (!i && rewriteColumn && (int)m_columns.size() > c)
                        convert_for_overwrite(m_columns[c], c, t);

                    if (_values.size() > i)
                        write(_values.get(i), r, c, t);
                }
                else
                {
                    if (_values.size() > j)
                        write(_values.get(j), r, c);
                }
            }
        }
    }
}


bool TableSheet::empty() const
{
    return !m_columns.size() || !rows();
}


void TableSheet::clear()
{
    m_columns.clear();
    m_calculatedRows = 0;
}







void TableData::create(const std::vector<size_t>& cols_per_sheet)
{
    clear();

    for (size_t i = 0; i < cols_per_sheet.size(); i++)
    {
        addSheet("Sheet_" + toString(i+1), cols_per_sheet[i]);
    }
}


void TableData::addSheet(const std::string& sSheetName, size_t cols)
{
    m_sheets.emplace_back(new TableSheet(sSheetName, cols));
}


TableData& TableData::operator=(const TableData& other)
{
    resize(other.sheets());

    for (size_t sheet = 0; sheet < other.sheets(); sheet++)
    {
        getSheet(sheet) = *other.m_sheets[sheet];
    }

    return *this;
}


TableData& TableData::operator=(TableData&& other)
{
    m_sheets.resize(other.sheets());

    for (size_t sheet = 0; sheet < other.sheets(); sheet++)
    {
        m_sheets[sheet].reset(other.m_sheets[sheet].release());
    }

    return *this;
}


void TableData::resize(size_t nSheets)
{
    if (nSheets > m_sheets.size())
        reserve(nSheets);
    else if (nSheets < m_sheets.size())
        m_sheets.resize(nSheets);
}


void TableData::reserve(size_t nSheets, size_t _nCols)
{
    while (nSheets > m_sheets.size())
        addSheet("Sheet_" + toString(m_sheets.size()+1), _nCols);
}


size_t TableData::rows() const
{
    size_t r = 0;

    for (const TblSheetPtr& sheet : m_sheets)
    {
        r = std::max(r, sheet->rows());
    }

    return r;
}


size_t TableData::cols() const
{
    size_t c = 0;

    for (const TblSheetPtr& sheet : m_sheets)
    {
        c = std::max(c, sheet->cols());
    }

    return c;
}


size_t TableData::reserved_cols() const
{
    size_t c = 0;

    for (const TblSheetPtr& sheet : m_sheets)
    {
        c = std::max(c, sheet->reserved_cols());
    }

    return c;
}


size_t TableData::sheets() const
{
    return m_sheets.size();
}


bool TableData::empty() const
{
    return m_sheets.size() == 0 || rows() == 0;
}


size_t TableData::getSize() const
{
    size_t bytes = 0;

    for (const TblSheetPtr& sheet : m_sheets)
    {
        for (const TblColPtr& col : sheet->m_columns)
        {
            if (col)
                bytes += col->getBytes();
        }

        bytes += sheet->m_sheetName.length();
    }

    return bytes;
}


TableColumn::ColumnType TableData::getType(const VectorIndex& _vCols, const VectorIndex& _vSheets) const
{
    TableColumn::ColumnType type = TableColumn::TYPE_NONE;

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        int s = _vSheets[k];

        if (s < 0 || s >= (int)sheets())
            continue;

        for (size_t j = 0; j < _vCols.size(); j++)
        {
            int c = _vCols[j];

            if (c >= 0 && m_sheets[s]->isValidColumn(c))
            {
                if (type == TableColumn::TYPE_NONE)
                    type = m_sheets[s]->m_columns[c]->m_type;
                else if (type != m_sheets[s]->m_columns[c]->m_type
                         && (type > TableColumn::STRINGLIKE || m_sheets[s]->m_columns[c]->m_type > TableColumn::STRINGLIKE))
                    return TableColumn::TYPE_MIXED;
            }
        }
    }

    return type;
}


mu::Value TableData::read(size_t row, size_t col, size_t sheet) const
{
    mu::Value retVal;

    if (!sheets() || sheets() <= sheet)
        return retVal;

    TableSheet& currSheet = *m_sheets[sheet];

    if (currSheet.isValidColumn(col))
        return currSheet.m_columns[col]->get(row);

    return retVal;
}


mu::Array TableData::read(const TableIndices& idx) const
{
    mu::Array vReturn;

    if (empty())
        return vReturn;

    size_t rowCount = idx.m_rows.size();
    size_t colCount = idx.m_cols.size();
    size_t sheetCount = idx.m_sheets.size();

    vReturn.resize(rowCount * colCount * sheetCount, mu::Value());

    #pragma omp parallel for if (rowCount * colCount * sheetCount > 10000)
    for (size_t k = 0; k < sheetCount; k++)
    {
        int s = idx.m_sheets[k];

        if (s < 0 || s >= (int)sheets())
            continue;

        if (rowCount == 1ull
            && colCount == 1ull
            && idx.m_rows.isString()
            && idx.m_cols.isString())
        {
            vReturn[k] = m_sheets[s]->m_sheetName;
            continue;
        }

        for (size_t j = 0; j < colCount; j++)
        {
            int c = idx.m_cols[j];

            if (c < 0 || !m_sheets[s]->isValidColumn(c))
                continue;

            for (size_t i = 0; i < rowCount; i++)
            {
                int r = idx.m_rows[i];

                if (r == VectorIndex::STRING)
                    vReturn[i + j * rowCount + k * rowCount * colCount] = m_sheets[s]->m_columns[c]->m_sHeadLine;

                if (r < 0)
                    continue;

                vReturn[i + j * rowCount + k * rowCount * colCount] = m_sheets[s]->m_columns[c]->get(r);
            }
        }
    }

    // Set the corresponding dimension sizes
    if (idx.returnsMatrix())
    {
        if (sheetCount > 1)
            vReturn.setDimSizes({rowCount, colCount, sheetCount});
        else
            vReturn.setDimSizes({rowCount, colCount});
    }

    return vReturn;
}


void TableData::read(const TableIndices& idx, mu::Variable* target) const
{
    if (empty())
    {
        target->std::vector<mu::Value>::assign(1ull, mu::Value());
        return;
    }

    size_t rowCount = idx.m_rows.size();
    size_t colCount = idx.m_cols.size();
    size_t sheetCount = idx.m_sheets.size();
    target->std::vector<mu::Value>::assign(rowCount * colCount * sheetCount, mu::Value());

    #pragma omp parallel for if (rowCount * colCount * sheetCount > 10000)
    for (size_t k = 0; k < sheetCount; k++)
    {
        int s = idx.m_sheets[k];

        if (s < 0 || s >= (int)sheets())
            continue;

        if (rowCount == 1ull
            && colCount == 1ull
            && idx.m_rows.isString()
            && idx.m_cols.isString())
        {
            (*target)[k] = m_sheets[k]->m_sheetName;
            continue;
        }

        for (size_t j = 0; j < colCount; j++)
        {
            int c = idx.m_cols[j];

            if (c < 0 || !m_sheets[s]->isValidColumn(c))
                continue;

            for (size_t i = 0; i < rowCount; i++)
            {
                int r = idx.m_rows[i];

                if (r == VectorIndex::STRING)
                    (*target)[i + j * rowCount + k * rowCount * colCount] = m_sheets[s]->m_columns[c]->m_sHeadLine;

                if (r < 0)
                    continue;

                (*target)[i + j * rowCount + k * rowCount * colCount] = m_sheets[s]->m_columns[c]->get(r);
            }
        }
    }

    // Set the corresponding dimension sizes
    if (idx.returnsMatrix())
        target->setDimSizes({rowCount, colCount, sheetCount});
}


TableData* TableData::extract(const TableIndices& idx) const
{
    TableData* _extract = new TableData;

    size_t rowCount = idx.m_rows.size();
    size_t colCount = idx.m_cols.size();
    size_t sheetCount = idx.m_sheets.size();

    #pragma omp parallel for if (rowCount * colCount * sheetCount > 10000)
    for (size_t k = 0; k < sheetCount; k++)
    {
        int s = idx.m_sheets[k];

        if (s < 0 || s >= (int)sheets())
            continue;

        size_t targetSheet = _extract->sheets();
        _extract->addSheet(m_sheets[s]->m_sheetName, colCount);

        for (size_t j = 0; j < colCount; j++)
        {
            int c = idx.m_cols[j];

            if (c >= 0 && m_sheets[s]->isValidColumn(c))
            {
                _extract->m_sheets[targetSheet]->m_columns[j].reset(m_sheets[s]->m_columns[c]->copy(idx.m_rows));
                _extract->m_sheets[targetSheet]->m_columns[j]->shrink();
            }
        }
    }

    return _extract;
}


TableSheet& TableData::getSheet(size_t sheet)
{
    if (sheet < m_sheets.size())
        return *m_sheets[sheet];

    throw std::out_of_range("Sheet " + toString(sheet) + " does not exists.");
}


const TableSheet& TableData::getSheet(size_t sheet) const
{
    if (sheet < m_sheets.size())
        return *m_sheets[sheet];

    throw std::out_of_range("Sheet " + toString(sheet) + " does not exists.");
}


void TableData::write(const mu::Value& _value, const TableIndices& idx)
{
    for (size_t k = 0; k < idx.m_sheets.size(); k++)
    {
        int s = idx.m_sheets[k];

        if (s < 0)
            continue;

        reserve(s+1);

        if (idx.m_rows.isString() && idx.m_cols.isString())
            m_sheets[s]->m_sheetName = _value.getStr();
        else
            m_sheets[s]->write(_value, idx.m_rows, idx.m_cols);
    }
}


void TableData::write(const mu::Array& _values, const TableIndices& idx)
{
    if (_values.size() == 1)
    {
        if (_values.front().isArray())
            write(_values.front().getArray(), idx);
        else
            write(_values.front(), idx);

        return;
    }

    if (idx.writeSingleSheet(_values))
    {
        int s = idx.m_sheets.front();

        if (s < 0)
            return;

        reserve(s+1);

        if (idx.m_rows.isString() && idx.m_cols.isString())
            m_sheets[s]->m_sheetName = _values.get(0).getStr();
        else
            m_sheets[s]->write(_values, idx.m_rows, idx.m_cols);

        return;
    }

    TableColumn::ColumnType t = to_column_type(_values);

    bool rewriteColumn = false;

    if (idx.m_rows.front() == 0 && idx.m_rows.isOpenEnd())
        rewriteColumn = true;

    if (_values.getDims() == 3)
    {
        // If the dimensions do not match and the indices provide the full range,
        // clear the table first
        if (cols() != _values.cols()
            && rows() != _values.rows()
            && sheets() != _values.layers()
            && idx.m_rows.isFullRange(rows()-1)
            && idx.m_cols.isFullRange(cols()-1)
            && idx.m_sheets.isFullRange(sheets()-1))
            clear();

        idx.m_rows.setOpenEndIndex(idx.m_rows.front() + _values.rows() - 1);
        idx.m_cols.setOpenEndIndex(idx.m_cols.front() + _values.cols() - 1);
        idx.m_sheets.setOpenEndIndex(idx.m_sheets.front() + _values.layers() - 1);

        // Prepare the target size
        reserve(idx.m_sheets.max()+1);

        // Write the contents to the table
        for (size_t k = 0; k < _values.layers(); k++)
        {
            int s = idx.m_sheets[k];

            if (s < 0)
                continue;

            for (size_t j = 0; j < _values.cols(); j++)
            {
                int c = idx.m_cols[j];

                if (c < 0)
                    continue;

                for (size_t i = 0; i < _values.rows(); i++)
                {
                    int r = idx.m_rows[i];

                    if (r == VectorIndex::INVALID)
                        break;

                    if (!i && rewriteColumn && (int)m_sheets[s]->m_columns.size() > c)
                        convert_for_overwrite(m_sheets[s]->m_columns[c], c, t);

                    m_sheets[s]->write(_values.get({i, j, k}), r, c, t);
                }
            }
        }
    }
    else if (_values.isMatrix() && !_values.isVector())
    {
        idx.m_sheets.setOpenEndIndex(idx.m_sheets.front() + _values.cols() - 1);

        if (idx.m_rows.size() > 1)
            idx.m_rows.setOpenEndIndex(idx.m_rows.front() + _values.rows() - 1);
        else
            idx.m_cols.setOpenEndIndex(idx.m_cols.front() + _values.rows() - 1);

        // Prepare the target size
        reserve(idx.m_sheets.max()+1);

        // Write the contents to the table
        for (size_t j = 0; j < _values.cols(); j++)
        {
            int s = idx.m_sheets[j];

            if (s < 0)
                continue;

            for (size_t i = 0; i < _values.rows(); i++)
            {
                int r = idx.m_rows[i];
                int c = idx.m_cols[i];

                if (c < 0 || r == VectorIndex::INVALID)
                    continue;

                if (!i && rewriteColumn && (int)m_sheets[s]->m_columns.size() > c)
                    convert_for_overwrite(m_sheets[s]->m_columns[c], c, t);

                m_sheets[s]->write(_values.get(i, j), r, c, t);
            }
        }
    }
    else
    {
        // vectors are accepted here for backwards compatibility
        for (size_t k = 0; k < idx.m_sheets.size(); k++)
        {
            int s = idx.m_sheets[k];

            if (s < 0)
                continue;

            reserve(s+1);

            if (idx.m_rows.isString() && idx.m_cols.isString())
                m_sheets[s]->m_sheetName = _values.get(k).getStr();
            else
                m_sheets[s]->write(_values.get(k), idx.m_rows.front(), idx.m_cols.front());
        }
    }
}


void TableData::erase(const TableIndices& idx)
{
    // Delete the selected entries
    #pragma omp parallel for
    for (size_t k = 0; k < idx.m_sheets.size(); k++)
    {
        int s = idx.m_sheets[k];

        if (s < 0 || s >= (int)sheets())
            continue;

        for (size_t j = 0; j < idx.m_cols.size(); j++)
        {
            int c = idx.m_cols[j];

            if (c >= 0 && m_sheets[s]->isValidColumn(c))
                m_sheets[s]->m_columns[c]->deleteElements(idx.m_rows);
        }

        m_sheets[s]->m_calculatedRows = 0;
    }
}


void TableData::clear()
{
    m_sheets.clear();
}

