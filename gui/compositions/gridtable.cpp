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

std::string removeQuotationMarks(const std::string& sString);


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
GridNumeReTable::GridNumeReTable() : m_showQMarks(true)
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
GridNumeReTable::GridNumeReTable(int numRows, int numCols) : m_showQMarks(true)
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
GridNumeReTable::GridNumeReTable(NumeRe::Table&& _extTable, bool showQMarks) : m_showQMarks(showQMarks)
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
        && (_table.getValue(row-getNumHeadlines(), col).imag() == 0 || mu::isnan(_table.getValue(row-getNumHeadlines(), col))))
        return true;

    if (sTypeName == wxGRID_VALUE_BOOL
        && _table.getColumnType(col) == TableColumn::TYPE_LOGICAL
        && _table.getColumn(col)->isValid(row-getNumHeadlines()))
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

    return _table.getValue(row - getNumHeadlines(), col).real();
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

    return _table.getValue(row - getNumHeadlines(), col).real() != 0.0;
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
    value = _table.getValue(row - getNumHeadlines(), col);
    return static_cast<void*>(&value);
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
        return removeQuotationMarks(_table.getValueAsString(row - getNumHeadlines(), col));
    else
        return replaceControlCharacters(_table.getValueAsString(row - getNumHeadlines(), col));
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
        return replaceControlCharacters(_table.getValueAsInternalString(row - getNumHeadlines(), col));
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
        _table.setValueAsString(row - nHeadRows, col, value.ToStdString());

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
    _table.insertLines(pos, numRows);

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
    _table.deleteLines(pos, numRows);

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
        return toString(row - (getNumHeadlines()-1));
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
    return toString(col+1);
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
    const int nHeadLines = getNumHeadlines();
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            double val = _table.getValue(i - nHeadLines, j).real();

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
    const int nHeadLines = getNumHeadlines();
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            double val = _table.getValue(i - nHeadLines, j).real();

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
    const int nHeadLines = getNumHeadlines();
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            std::complex<double> val = _table.getValue(i - nHeadLines, j);

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
    const int nHeadLines = getNumHeadlines();
    const wxGridCellsExtent& cellExtent = coords.getExtent();

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            std::complex<double> val = _table.getValue(i - nHeadLines, j);

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
                    sSerialized += _table.getValueAsInternalString(i - nHeadLines, j);

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

