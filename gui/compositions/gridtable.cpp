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

std::string toString(int nNumber);

// Default constructor
GridNumeReTable::GridNumeReTable()
{
    _table = NumeRe::Table();
}

// General constructor. Will create an empty internal table
GridNumeReTable::GridNumeReTable(int numRows, int numCols)
{
    _table = NumeRe::Table(numRows-2, numCols-1);
}

// Move constructor taking the contents of the passed table
GridNumeReTable::GridNumeReTable(NumeRe::Table&& _extTable)
{
    _table = std::move(_extTable);
}

// This member function will return the number of headlines
// available in the internal buffer
int GridNumeReTable::getNumHeadlines()
{
    return _table.getHeadCount();
}

NumeRe::Table GridNumeReTable::getTableCopy()
{
    NumeRe::Table tableCopy(_table);
    return tableCopy;
}


// This member function will return the number of lines
// including the headlines handled by this data provider
// class
int GridNumeReTable::GetNumberRows()
{
    return std::max(_table.getLines(), 1u) + getNumHeadlines() + 1;
}

// This member function will return the number of columns
// handled by this data provider class
int GridNumeReTable::GetNumberCols()
{
    return _table.getCols() + 1;
}

// This virtual member function will tell the grid, which
// data types may be returned by the current cell
bool GridNumeReTable::CanGetValueAs(int row, int col, const wxString& sTypeName)
{
    // Headlines
    if (row == 0 && sTypeName == wxGRID_VALUE_FLOAT)
        return false;
    else if (row < getNumHeadlines() && sTypeName == wxGRID_VALUE_STRING)
        return true;

    // Regular cells
    if (sTypeName == wxGRID_VALUE_FLOAT
        && _table.getColumnType(col) == TableColumn::TYPE_VALUE
        && _table.getValue(row-getNumHeadlines(), col).imag() == 0)
        return true;

    if (sTypeName == wxGRID_VALUE_STRING && _table.getColumnType(col) != TableColumn::TYPE_NONE)
        return true;

    return false;
}

// This virtual member function returns the selected
// cell value as a double
double GridNumeReTable::GetValueAsDouble(int row, int col)
{
    // Return NAN, if this is not a numeric cell
    if (row < getNumHeadlines())
        return NAN;

    if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return NAN;

    return _table.getValue(row - getNumHeadlines(), col).real();
}

// This virtual member function returns the value of the
// selected cell as string
wxString GridNumeReTable::GetValue(int row, int col)
{
    if (row < getNumHeadlines() && col < (int)_table.getCols())
        return _table.getCleanHeadPart(col, row);
    else if (row - getNumHeadlines() >= (int)_table.getLines() || col >= (int)_table.getCols())
        return "";
    else
        return _table.getValueAsString(row - getNumHeadlines(), col);
}

// This virtual member function sets the passed value
// in the internal buffer. It's decided automatically,
// whether it's stored as a headline and whether a new
// headline is required
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

// This virtual function redirects the control directly
// to the internal buffer and cleares it contents
void GridNumeReTable::Clear()
{
    _table.Clear();
}

// This virtual function redirects the control directly
// to the internal buffer and inserts rows
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

// This virtual function redirects the control directly
// to the internal buffer and appends rows
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

// This virtual function redirects the control directly
// to the internal buffer and deletes rows
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

// This virtual function redirects the control directly
// to the internal buffer and inserts columns
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

// This virtual function redirects the control directly
// to the internal buffer and appends columns
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

// This virtual function redirects the control directly
// to the internal buffer and deletes columns
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

// This virtual member function will return the
// label for the selected row as a string
wxString GridNumeReTable::GetRowLabelValue(int row)
{
    if (row < getNumHeadlines())
        return "#";
    else
        return toString(row - (getNumHeadlines()-1));
}

// This virtual member function will return the
// label for the selected column as a string
wxString GridNumeReTable::GetColLabelValue(int col)
{
    return toString(col+1);
}

