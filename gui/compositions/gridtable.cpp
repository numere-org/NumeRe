/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "gridtable.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../kernel/core/datamanagement/tablecolumnimpl.hpp"

/////////////////////////////////////////////////
/// \brief Private helper member to calculate the
/// correct row position depending on the sorting
/// state.
///
/// \param row int
/// \return int
///
/////////////////////////////////////////////////
int GridNumeReTable::getRow(int row) const
{
    if ((size_t)(row-getNumHeadlines()) < m_sortIndex.size())
        return m_sortIndex[(row-getNumHeadlines())];

    return (row-getNumHeadlines());
}


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
GridNumeReTable::GridNumeReTable() : m_showQMarks(true), m_sortIndex(0, VectorIndex::OPEN_END)
{
    _table = NumeRe::Table();
}


/////////////////////////////////////////////////
/// \brief General constructor. Will create an
/// empty internal table.
///
/// \param numRows int
/// \param numCols int
///
/////////////////////////////////////////////////
GridNumeReTable::GridNumeReTable(int numRows, int numCols) : m_showQMarks(true), m_sortIndex(0, VectorIndex::OPEN_END)
{
    _table = NumeRe::Table(numRows-2, numCols-1);
}


/////////////////////////////////////////////////
/// \brief Move constructor taking the contents
/// of the passed table.
///
/// \param _extTable NumeRe::Table&&
/// \param showQMarks bool
///
/////////////////////////////////////////////////
GridNumeReTable::GridNumeReTable(NumeRe::Table&& _extTable, bool showQMarks) : m_showQMarks(showQMarks), m_sortIndex(0, VectorIndex::OPEN_END)
{
    _table = std::move(_extTable);
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of headlines available in the internal
/// buffer.
///
/// \return int
///
/////////////////////////////////////////////////
int GridNumeReTable::getNumHeadlines() const
{
    return _table.getHeadCount();
}


/////////////////////////////////////////////////
/// \brief This member function returns a copy of
/// the internal table.
///
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table GridNumeReTable::getTableCopy()
{
    NumeRe::Table tableCopy(_table);
    return tableCopy;
}


/////////////////////////////////////////////////
/// \brief This member function returns a
/// reference to the internal table.
///
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table& GridNumeReTable::getTableRef()
{
    return _table;
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of lines including the headlines
/// handled by this data provider class.
///
/// \return int
///
/////////////////////////////////////////////////
int GridNumeReTable::GetNumberRows()
{
    return std::max(_table.getLines(), (size_t)1u) + getNumHeadlines() + 1;
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of columns handled by this data
/// provider class.
///
/// \return int
///
/////////////////////////////////////////////////
int GridNumeReTable::GetNumberCols()
{
    return _table.getCols() + 1;
}


/////////////////////////////////////////////////
/// \brief This virtual member function will tell
/// the grid, which data types may be returned by
/// the current cell.
///
/// \param row int
/// \param col int
/// \param sTypeName const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::CanGetValueAs(int row, int col, const wxString& sTypeName)
{
    // Headlines
    if (row == 0 && (sTypeName == wxGRID_VALUE_FLOAT || sTypeName == "complex"))
        return false;
    else if (row < getNumHeadlines() && (sTypeName == wxGRID_VALUE_STRING || sTypeName == "plain"))
        return true;

    // Regular cells
    if (sTypeName == "complex" && TableColumn::isValueType(_table.getColumnType(col)))// == TableColumn::TYPE_VALUE)
        return true;

    if (sTypeName == "datetime" && _table.getColumnType(col) == TableColumn::TYPE_DATETIME)
        return true;

    if (sTypeName == wxGRID_VALUE_FLOAT
        && TableColumn::isValueType(_table.getColumnType(col))
        && (_table.getValue(getRow(row), col).imag() == 0 || mu::isnan(_table.getValue(getRow(row), col))))
        return true;

    if (sTypeName == wxGRID_VALUE_BOOL
        && _table.getColumnType(col) == TableColumn::TYPE_LOGICAL
        && _table.getColumn(col)->isValid(getRow(row)))
        return true;

    if (sTypeName == "plain" && _table.getColumnType(col) == TableColumn::TYPE_STRING)
        return true;

    if (sTypeName == wxGRID_VALUE_STRING && _table.getColumnType(col) != TableColumn::TYPE_NONE)
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This virtual member function returns
/// the selected cell value as a double.
///
/// \param row int
/// \param col int
/// \return double
///
/////////////////////////////////////////////////
double GridNumeReTable::GetValueAsDouble(int row, int col)
{
    // Return NAN, if this is not a numeric cell
    if (row < getNumHeadlines())
        return NAN;

    if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return NAN;

    return _table.getValue(getRow(row), col).real();
}


/////////////////////////////////////////////////
/// \brief This virtual member function returns
/// the selected cell value as a bool.
///
/// \param row int
/// \param col int
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::GetValueAsBool(int row, int col)
{
    // Return NAN, if this is not a numeric cell
    if (row < getNumHeadlines())
        return false;

    if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return false;

    return _table.getValue(getRow(row), col).real() != 0.0;
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// internal data as a void pointer referencing
/// the internal std::complex<double> attribute.
///
/// \param row int
/// \param col int
/// \param sTypeName const wxString&
/// \return void*
///
/////////////////////////////////////////////////
void* GridNumeReTable::GetValueAsCustom(int row, int col, const wxString& sTypeName)
{
    if (sTypeName == "mu::Value")
    {
        value = _table.get(getRow(row), col);
        return static_cast<void*>(&value);
    }

    cmplx = _table.getValue(getRow(row), col);
    return static_cast<void*>(&cmplx);
}


/////////////////////////////////////////////////
/// \brief This virtual member function returns
/// the value of the selected cell as string.
///
/// \param row int
/// \param col int
/// \return wxString
///
/////////////////////////////////////////////////
wxString GridNumeReTable::GetValue(int row, int col)
{
    if (row < getNumHeadlines() && col < (int)_table.getCols())
        return _table.getCleanHeadPart(col, row);
    else if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return "";
    else if (!m_showQMarks)
    {
        std::string sValue = toInternalString(_table.getValueAsString(getRow(row), col));
        replaceAll(sValue, "\t", "    ");
        return sValue;
    }
    else
        return replaceControlCharacters(_table.getValueAsString(getRow(row), col));
}


/////////////////////////////////////////////////
/// \brief This virtual member function returns
/// the value of the selected cell as "editable"
/// string, i.e. without additional unit.
///
/// \param row int
/// \param col int
/// \return wxString
///
/////////////////////////////////////////////////
wxString GridNumeReTable::GetEditableValue(int row, int col)
{
    if (row < getNumHeadlines() && col < (int)_table.getCols())
        return _table.getCleanHeadPart(col, row);
    else if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return "";
    else
        return replaceControlCharacters(_table.getValueAsInternalString(getRow(row), col));
}


/////////////////////////////////////////////////
/// \brief This virtual member function sets the
/// passed value in the internal buffer. It's
/// decided automatically, whether it's stored as
/// a headline and whether a new headline is
/// required.
///
/// \param row int
/// \param col int
/// \param value const wxString&
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::SetValue(int row, int col, const wxString& value)
{
    int nHeadRows = getNumHeadlines();

    // Set the value
    if (row < nHeadRows)
        _table.setHeadPart(col, row, value.ToStdString());
    else
        _table.setValueAsString(getRow(row), col, value.ToStdString());

    // If the number of headlines changed, notify the
    // grid to draw the newly added cells
    if (nHeadRows != getNumHeadlines() && GetView())
    {
        if (nHeadRows < getNumHeadlines())
        {
            wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, nHeadRows, getNumHeadlines() - nHeadRows);
            GetView()->ProcessTableMessage(msg);
        }
        else
        {
            wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, getNumHeadlines(), nHeadRows - getNumHeadlines());
            GetView()->ProcessTableMessage(msg);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// cleares it contents.
///
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::Clear()
{
    _table.Clear();
    m_sortIndex = VectorIndex(0, VectorIndex::OPEN_END);
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// inserts rows.
///
/// \param pos size_t
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::InsertRows(size_t pos, size_t numRows)
{
    if ((isSorted() && numRows > 1)
        || pos < getNumHeadlines())
        return false;

    _table.insertLines(getRow(pos), numRows);

    // Update the index, if necessary
    if (isSorted())
    {
        // This is the new index value
        int partition = getRow(pos);

        // All same or higher index values must be incremented
        for (size_t i = 0; i < m_sortIndex.size(); i++)
        {
            if (m_sortIndex[i] >= partition)
                m_sortIndex.setIndex(i, m_sortIndex[i]+1);
        }

        // Insert the new index value at the correct position. First
        // and last position are quite simple
        if (pos-getNumHeadlines() == 0)
            m_sortIndex.prepend(partition);
        else if (pos-getNumHeadlines()+1 == m_sortIndex.size())
            m_sortIndex.append(partition);
        else
        {
            VectorIndex inserted = m_sortIndex.subidx(0, pos-getNumHeadlines());
            inserted.append(partition);
            inserted.append(m_sortIndex.subidx(pos-getNumHeadlines()));
            m_sortIndex = inserted;
        }
    }

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, pos, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// appends rows.
///
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::AppendRows(size_t numRows)
{
    _table.appendLines(numRows);

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// deletes rows.
///
/// \param pos size_t
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::DeleteRows(size_t pos, size_t numRows)
{
    if ((isSorted() && numRows > 1)
        || pos < getNumHeadlines())
        return false;

    _table.deleteLines(getRow(pos), numRows);

    // Update the index, if necessary
    if (isSorted())
    {
        // This is the removed index value
        int partition = getRow(pos);

        // All same or higher index values must be decremented
        for (size_t i = 0; i < m_sortIndex.size(); i++)
        {
            if (m_sortIndex[i] >= partition)
                m_sortIndex.setIndex(i, m_sortIndex[i]-1);
        }

        // Remove the index value from the correct position. First
        // and last position are quite simple
        if (pos-getNumHeadlines() == 0)
            m_sortIndex = m_sortIndex.subidx(1);
        else if (pos-getNumHeadlines()+1 == m_sortIndex.size())
            m_sortIndex = m_sortIndex.subidx(0, m_sortIndex.size()-1);
        else
        {
            VectorIndex removed = m_sortIndex.subidx(0, pos-getNumHeadlines());
            removed.append(m_sortIndex.subidx(pos-getNumHeadlines()+1));
            m_sortIndex = removed;
        }
    }

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, pos, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// inserts columns.
///
/// \param pos size_t
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::InsertCols(size_t pos, size_t numRows)
{
    _table.insertCols(pos, numRows);

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_INSERTED, pos, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// appends columns.
///
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::AppendCols(size_t numRows)
{
    _table.appendCols(numRows);

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual function redirects the
/// control directly to the internal buffer and
/// deletes columns.
///
/// \param pos size_t
/// \param numRows size_t
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::DeleteCols(size_t pos, size_t numRows)
{
    _table.deleteCols(pos, numRows);

    // Notify the grid that the number of elements have
    // changed and that the grid has to be redrawn
    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED, pos, numRows);
        GetView()->ProcessTableMessage(msg);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This virtual member function will
/// return the label for the selected row as a
/// string.
///
/// \param row int
/// \return wxString
///
/////////////////////////////////////////////////
wxString GridNumeReTable::GetRowLabelValue(int row)
{
    if (row < getNumHeadlines())
        return "#";
    else
        return toString(getRow(row)+1);
}


/////////////////////////////////////////////////
/// \brief This virtual member function will
/// return the label for the selected column as a
/// string.
///
/// \param col int
/// \return wxString
///
/////////////////////////////////////////////////
wxString GridNumeReTable::GetColLabelValue(int col)
{
    auto iter = m_customColLabels.find(col);

    if (iter != m_customColLabels.end())
        return iter->second;

    return toString(col+1);
}


/////////////////////////////////////////////////
/// \brief Sets a custom column label.
///
/// \param col int
/// \param label const wxString&
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::SetColLabelValue(int col, const wxString& label)
{
    if (col >= 0 && col < _table.getCols())
        m_customColLabels[col] = label;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// minimal value in the range (r1,c1)->(r2,c2).
///
/// \param coords const wxGridCellCoordsContainer&
/// \return double
///
/////////////////////////////////////////////////
double GridNumeReTable::min(const wxGridCellCoordsContainer& coords) const
{
    double dMin = NAN;
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            double val = _table.getValue(getRow(i), j).real();

            if (isnan(dMin) || val < dMin)
                dMin = val;
        }
    }

    return dMin;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// maximal value in the range (r1,c1)->(r2,c2).
///
/// \param coords const wxGridCellCoordsContainer&
/// \return double
///
/////////////////////////////////////////////////
double GridNumeReTable::max(const wxGridCellCoordsContainer& coords) const
{
    double dMax = NAN;
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            double val = _table.getValue(getRow(i), j).real();

            if (isnan(dMax) || val > dMax)
                dMax = val;
        }
    }

    return dMax;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// average value of the range (r1,c1)->(r2,c2).
///
/// \param coords const wxGridCellCoordsContainer&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> GridNumeReTable::avg(const wxGridCellCoordsContainer& coords) const
{
    std::complex<double> dSum = 0;
    size_t nCount = 0;
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            std::complex<double> val = _table.getValue(getRow(i), j);

            if (!mu::isnan(val))
            {
                nCount++;
                dSum += val;
            }
        }
    }

    if (nCount)
        return dSum / (double)nCount;

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// sum of the range (r1,c1)->(r2,c2).
///
/// \param coords const wxGridCellCoordsContainer&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> GridNumeReTable::sum(const wxGridCellCoordsContainer& coords) const
{
    std::complex<double> dSum = 0;
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            std::complex<double> val = _table.getValue(getRow(i), j);

            if (!mu::isnan(val))
                dSum += val;
        }
    }

    return dSum;
}


/////////////////////////////////////////////////
/// \brief This member function serializes the
/// table within the range (r1,c1)->(r2,c2).
///
/// \param coords const wxGridCellCoordsContainer&
/// \return std::string
///
/////////////////////////////////////////////////
std::string GridNumeReTable::serialize(const wxGridCellCoordsContainer& coords) const
{
    std::string sSerialized;
    const int nHeadLines = getNumHeadlines();
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    if (cellExtent.m_topleft.GetRow() < nHeadLines)
    {
        for (int i = cellExtent.m_topleft.GetRow(); i <= std::min(nHeadLines-1, cellExtent.m_bottomright.GetRow()); i++)
        {
            for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
            {
                if (coords.contains(i, j))
                    sSerialized += _table.getCleanHeadPart(j, i);

                sSerialized += "\t";
            }

            sSerialized.back() = '\n';
        }
    }

    if (cellExtent.m_bottomright.GetRow() >= nHeadLines)
    {
        for (int i = std::max(nHeadLines, cellExtent.m_topleft.GetRow()); i <= cellExtent.m_bottomright.GetRow(); i++)
        {
            for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
            {
                if (coords.contains(i, j))
                    sSerialized += _table.getValueAsInternalString(getRow(i), j);

                sSerialized += "\t";
            }

            sSerialized.back() = '\n';
        }
    }

    return sSerialized;
}


/////////////////////////////////////////////////
/// \brief Returns the types of the handled table.
///
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> GridNumeReTable::getColumnTypes() const
{
    std::vector<int> vTypes;

    for (size_t i = 0; i < _table.getCols(); i++)
        vTypes.push_back(_table.getColumnType(i));

    return vTypes;
}


/////////////////////////////////////////////////
/// \brief Returns the categories in the selected
/// column, if this is a categorical column.
///
/// \param col int
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> GridNumeReTable::getCategories(int col) const
{
    if (_table.getColumnType(col) == TableColumn::TYPE_CATEGORICAL)
        return static_cast<CategoricalColumn*>(_table.getColumn(col))->getCategories();

    return std::vector<std::string>();
}


/////////////////////////////////////////////////
/// \brief Enable or disable the quoation marks
/// around strings.
///
/// \param enable bool
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::enableQuotationMarks(bool enable)
{
    m_showQMarks = enable;
}


/////////////////////////////////////////////////
/// \brief Set the selected column as sorting
/// column.
///
/// \param col int
/// \param ascending bool
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::sortCol(int col, bool ascending)
{
    if (col >= _table.getCols())
        return;

    m_sortIndex = VectorIndex(_table.getColumn(col)->get(VectorIndex(0, VectorIndex::OPEN_END)).call("order"));

    // Just reverse it
    if (!ascending)
        m_sortIndex = m_sortIndex.get(VectorIndex(m_sortIndex.size()-1, 0));
}


/////////////////////////////////////////////////
/// \brief Return, if the table somehow sorted.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GridNumeReTable::isSorted() const
{
    return !m_sortIndex.isOpenEnd();
}


/////////////////////////////////////////////////
/// \brief Remove the sorting column.
///
/// \return void
///
/////////////////////////////////////////////////
void GridNumeReTable::removeSort()
{
    m_sortIndex = VectorIndex(0, VectorIndex::OPEN_END);
}






