/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#ifndef TABLEVIEWER_HPP
#define TABLEVIEWER_HPP

#include <wx/wx.h>
#include <wx/grid.h>

#include <vector>
#include <string>

#include "gridcellcoordscontainer.hpp"
#include "../../kernel/core/datamanagement/container.hpp"
#include "../../kernel/core/datamanagement/table.hpp"

class TablePanel;

/////////////////////////////////////////////////
/// \brief This class is an adaption of the
/// wxGrid class to present the tabular data in
/// NumeRe's memory and enabling copy-pasting of
/// tabular data to an from NumeRe.
/////////////////////////////////////////////////
class TableViewer : public wxGrid
{
    private:
        size_t nHeight;
        size_t nWidth;
        size_t nFirstNumRow;
        bool readOnly;
        bool isGridNumeReTable;
        wxGridCellCoords lastCursorPosition;
        wxGridCellCoordsArray selectedCells;
        std::vector<int> m_currentColTypes;
        std::string m_displayName;
        std::string m_intName;

        // External window elements
        TablePanel* m_parentPanel;
        wxStatusBar* m_statusBar;

        void layoutGrid();

        void OnKeyDown(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);
        void OnEnter(wxMouseEvent& event);
        void OnCellChange(wxGridEvent& event);
        void OnCellSelect(wxGridEvent& event);
        void OnCellRangeSelect(wxGridRangeSelectEvent& event);
        void OnLabelDoubleClick(wxGridEvent& event);
        void updateFrame();
        void deleteSelection();
        int findLastElement(int nCol);
        bool isNumerical(const std::string& sCell);
        bool isEmptyCol(int col);
        wxString replaceCtrlChars(const wxString& sStr);
        void copyContents();
        void pasteContents(bool useCursor = false);
        void applyConditionalCellColourScheme();
        void UpdateColumnAlignment(int col);
        std::vector<wxString> getLinesFromPaste(const wxString& data);
        void replaceDecimalSign(wxString& text);
        void replaceTabSign(wxString& text);
        void createZeroElementTable();
        wxGridCellCoords CreateEmptyGridSpace(int rows, int headrows, int cols, bool useCursor = false);

        mu::value_type CellToCmplx(int row, int col);

        double calculateMin(const wxGridCellCoordsContainer& coords);
        double calculateMax(const wxGridCellCoordsContainer& coords);
        mu::value_type calculateSum(const wxGridCellCoordsContainer& coords);
        mu::value_type calculateAvg(const wxGridCellCoordsContainer& coords);

        void updateStatusBar(const wxGridCellCoordsContainer& coords, wxGridCellCoords* cursor = nullptr);
        void createMenuBar();

        wxMenu m_popUpMenu;
        wxGridCellCoords m_lastRightClick;


    public:
        TableViewer(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, TablePanel* parentPanel, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxGridNameStr);

        void SetData(NumeRe::Container<std::string>& _stringTable, const std::string& sName, const std::string& sIntName);
        void SetData(NumeRe::Table& _table, const std::string& sName, const std::string& sIntName);
        NumeRe::Table GetData();
        NumeRe::Table GetDataCopy();

        void SetTableReadOnly(bool isReadOnly = true);
        void SetDefaultSize(size_t rows = 1, size_t cols = 1) __attribute__ ((deprecated));
        void OnLabelRightClick(wxGridEvent& event);
        void OnCellRightClick(wxGridEvent& event);
        void OnMenu(wxCommandEvent& event);
        void insertElement(int id);
        void removeElement(int id);
        void saveTable(bool saveAs = false);
        void reloadTable();
        void changeColType();
        void finalize();
        void groupHeaders(int startCol, int endCol, int row);

        wxString getSelectedValues();
        int GetInternalRows(int gridrow) const;


        size_t GetHeight() {return nHeight;}
        size_t GetWidth() {return nWidth;}

        enum TableViewerIDs
        {
            ID_MENU_SAVE = 15000,
            ID_MENU_SAVE_AS,
            ID_MENU_INSERT_ROW,
            ID_MENU_INSERT_COL,
            ID_MENU_INSERT_CELL,
            ID_MENU_REMOVE_ROW,
            ID_MENU_REMOVE_COL,
            ID_MENU_REMOVE_CELL,
            ID_MENU_COPY,
            ID_MENU_PASTE,
            ID_MENU_PASTE_HERE,
            ID_MENU_RELOAD,
            ID_MENU_CHANGE_COL_TYPE,
            ID_MENU_CVS
        };

        DECLARE_EVENT_TABLE();
};

#endif // TABLEVIEWER_HPP

