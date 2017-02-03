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

        void OnKeyDown(wxKeyEvent& event);
        void OnEnter(wxMouseEvent& event);
        wxString GetRowLabelValue(int row);
        wxString GetColLabelValue(int col);
        bool isNumerical(const string& sCell);
        wxString replaceCtrlChars(const wxString& sStr);
        void copyContents();


    public:
        TableViewer(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxGridNameStr);

        void SetData(const vector<vector<string> >& vData);


        size_t GetHeight() {return nHeight;}
        size_t GetWidth() {return nWidth;}

        DECLARE_EVENT_TABLE();
};

#endif // TABLEVIEWER_HPP

