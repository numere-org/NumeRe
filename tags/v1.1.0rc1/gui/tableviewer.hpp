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

using namespace std;

string toString(int);

class TableViewer : public wxGrid
{
    private:
        size_t nHeight;
        size_t nWidth;
        size_t nFirstNumRow;
        bool readOnly;
        wxColor HeadlineColor;
        wxColor FrameColor;

        void OnKeyDown(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);
        void OnEnter(wxMouseEvent& event);
        void OnCellChange(wxGridEvent& event);
        void updateFrame();
        void deleteSelection();
        int findEmptyHeadline(int nCol);
        int findLastElement(int nCol);
        wxString GetRowLabelValue(int row);
        wxString GetColLabelValue(int col);
        bool isNumerical(const string& sCell);
        bool isEmptyCol(int col);
        wxString replaceCtrlChars(const wxString& sStr);
        void copyContents();
        void pasteContents();
        vector<wxString> getLinesFromPaste(const wxString& data);
        void replaceDecimalSign(wxString& text);
        void replaceTabSign(wxString& text);
        wxGridCellCoords CreateEmptyGridSpace(int rows, int headrows, int cols);

        wxString copyCell(int row, int col);

        wxMenu m_popUpMenu;
        wxGridCellCoords m_lastRightClick;


    public:
        TableViewer(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxGridNameStr);

        void SetData(const vector<vector<string> >& vData);
        vector<vector<string> > GetData();

        void SetTableReadOnly(bool isReadOnly = true);
        void SetDefaultSize(size_t rows = 1, size_t cols = 1);
        void OnLabelRightClick(wxGridEvent& event);
        void OnCellRightClick(wxGridEvent& event);
        void OnMenu(wxCommandEvent& event);
        void insertElement(int id);
        void removeElement(int id);


        size_t GetHeight() {return nHeight;}
        size_t GetWidth() {return nWidth;}

        enum TableViewerIDs
        {
            ID_MENU_INSERT_ROW = 15000,
            ID_MENU_INSERT_COL,
            ID_MENU_INSERT_CELL,
            ID_MENU_REMOVE_ROW,
            ID_MENU_REMOVE_COL,
            ID_MENU_REMOVE_CELL,
            ID_MENU_COPY,
            ID_MENU_PASTE
        };

        DECLARE_EVENT_TABLE();
};

#endif // TABLEVIEWER_HPP

