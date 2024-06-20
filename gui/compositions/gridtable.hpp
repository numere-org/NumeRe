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

#include <wx/grid.h>

#include "../../kernel/core/datamanagement/table.hpp"
#include "gridcellcoordscontainer.hpp"

#ifndef GRIDTABLE_HPP
#define GRIDTABLE_HPP


/////////////////////////////////////////////////
/// \brief This class is a specialisation for the
/// standard wxGridTableBase supporting complex
/// numbers as well as the internal data model.
/////////////////////////////////////////////////
class GridNumeReTable : public wxGridTableBase
{
    private:
        NumeRe::Table _table;
        int getNumHeadlines() const;
        mu::value_type value;
        bool m_showQMarks;

    public:
        GridNumeReTable();
        GridNumeReTable(int numRows, int numCols);
        GridNumeReTable(NumeRe::Table&& _extTable, bool showQMarks);
        virtual ~GridNumeReTable() {}

        NumeRe::Table getTable()
        {
            return _table;
        }

        NumeRe::Table getTableCopy();
        NumeRe::Table& getTableRef();

        virtual int GetNumberRows();
        virtual int GetNumberCols();

        virtual bool CanGetValueAs(int row, int col, const wxString& sTypeName);
        virtual double GetValueAsDouble(int row, int col);
        virtual bool GetValueAsBool(int row, int col);
        virtual void* GetValueAsCustom(int row, int col, const wxString& sTypeName);

        virtual wxString GetValue(int row, int col);
        virtual wxString GetEditableValue(int row, int col);
        virtual void SetValue(int row, int col, const wxString& value);

        virtual void Clear();
        virtual bool InsertRows(size_t pos = 0, size_t numRows = 1);
        virtual bool AppendRows(size_t numRows = 1);
        virtual bool DeleteRows(size_t pos = 0, size_t numRows = 1);
        virtual bool InsertCols(size_t pos = 0, size_t numRows = 1);
        virtual bool AppendCols(size_t numRows = 1);
        virtual bool DeleteCols(size_t pos = 0, size_t numRows = 1);

        virtual wxString GetRowLabelValue(int row);
        virtual wxString GetColLabelValue(int col);

        double min(const wxGridCellCoordsContainer& coords) const;
        double max(const wxGridCellCoordsContainer& coords) const;
        mu::value_type avg(const wxGridCellCoordsContainer& coords) const;
        mu::value_type sum(const wxGridCellCoordsContainer& coords) const;
        std::string serialize(const wxGridCellCoordsContainer& coords) const;

        std::vector<int> getColumnTypes() const;
        std::vector<std::string> getCategories(int col) const;
};



#endif // GRIDTABLE_HPP

