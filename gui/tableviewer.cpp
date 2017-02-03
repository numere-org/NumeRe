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

#include "tableviewer.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>

BEGIN_EVENT_TABLE(TableViewer, wxGrid)
    EVT_KEY_DOWN        (TableViewer::OnKeyDown)
    //EVT_ENTER_WINDOW    (TableViewer::OnEnter)
END_EVENT_TABLE()

TableViewer::TableViewer(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
            : wxGrid(parent, id, pos, size, style, name), nHeight(600), nWidth(800), nFirstNumRow(1)
{
    GetGridWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridRowLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridColLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridCornerLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
}



void TableViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
    else if (event.GetKeyCode() == WXK_UP)
        this->MoveCursorUp(false);
    else if (event.GetKeyCode() == WXK_DOWN)
        this->MoveCursorDown(false);
    else if (event.GetKeyCode() == WXK_RETURN)
        this->MoveCursorDown(false);
    else if (event.GetKeyCode() == WXK_LEFT)
        this->MoveCursorLeft(false);
    else if (event.GetKeyCode() == WXK_RIGHT)
        this->MoveCursorRight(false);
    else if (event.GetKeyCode() == WXK_TAB)
        this->MoveCursorRight(false);
    else if (event.ControlDown() && event.ShiftDown())
    {
        if (event.GetKeyCode() == 'C')
        {
            copyContents();
        }
        //event.Skip();
    }
    else
        event.Skip();
}

void TableViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

wxString TableViewer::GetRowLabelValue(int row)
{
    if (row < (int)nFirstNumRow)
        return "#";
    return toString(row+1-nFirstNumRow);
}

wxString TableViewer::GetColLabelValue(int col)
{
    return toString(col+1);
}

bool TableViewer::isNumerical(const string& sCell)
{
    string sNums = "0123456789.eE+- ";
    return sCell.find_first_not_of(sNums) == string::npos;
}

wxString TableViewer::replaceCtrlChars(const wxString& sStr)
{
    wxString sReturn = sStr;

    while (sReturn.find('_') != string::npos)
        sReturn[sReturn.find('_')] = ' ';
    return sReturn;
}


void TableViewer::copyContents()
{
    if (!(GetSelectedCells().size() || GetSelectedCols().size() || GetSelectedRows().size() || GetSelectionBlockTopLeft().size() || GetSelectionBlockBottomRight().size()))
        return;
    wxString sSelection;
    if (GetSelectedCells().size())
    {
        wxGridCellCoordsArray cellarray = GetSelectedCells();
        for (size_t i = 0; i < cellarray.size(); i++)
        {
            sSelection += this->GetCellValue(cellarray[i]);
            if (i < cellarray.size()-1)
                sSelection += "\t";
        }
    }
    else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size())
    {
        wxGridCellCoordsArray topleftarray = GetSelectionBlockTopLeft();
        wxGridCellCoordsArray bottomrightarray = GetSelectionBlockBottomRight();

        for (int i = topleftarray[0].GetRow(); i <= bottomrightarray[0].GetRow(); i++)
        {
            for (int j = topleftarray[0].GetCol(); j <= bottomrightarray[0].GetCol(); j++)
            {
                sSelection += this->GetCellValue(i,j);
                if (j < bottomrightarray[0].GetCol())
                    sSelection += "\t";
            }
            if (i < bottomrightarray[0].GetRow())
                sSelection += "\n";
        }
    }
    else if (GetSelectedCols().size())
    {
        wxArrayInt colarray = GetSelectedCols();
        for (int i = 0; i < GetRows(); i++)
        {
            for (size_t j = 0; j < colarray.size(); j++)
            {
                sSelection += GetCellValue(i, colarray[j]);
                if (j < colarray.size()-1)
                    sSelection += "\t";
            }
            if (i < GetRows()-1);
                sSelection += "\n";
        }
    }
    else
    {
        wxArrayInt rowarray = GetSelectedRows();
        for (size_t i = 0; i < rowarray.size(); i++)
        {
            for (int j = 0; j < GetCols(); j++)
            {
                sSelection += GetCellValue(rowarray[i], j);
                if (j < GetCols()-1)
                    sSelection += "\t";
            }
            if (i < rowarray.size()-1)
                sSelection += "\n";
        }
    }
    if (!sSelection.length())
        return;
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(sSelection));
        wxTheClipboard->Close();
    }
    return;
}

void TableViewer::SetData(const vector<vector<string> >& vData)
{
    this->CreateGrid(vData.size()+1, vData[0].size()+1);
    nFirstNumRow = -1;
    for (size_t i = 0; i < vData.size(); i++)
    {
        for (size_t j = 0; j < vData[0].size(); j++)
        {
            if (vData[i][j].length() && (isNumerical(vData[i][j]) || vData[i][j] == "---"))
            {
                nFirstNumRow = i;
                break;
            }
        }
        if (nFirstNumRow != string::npos)
            break;
    }

    for (size_t i = 0; i < vData.size()+1; i++)
    {
        this->SetRowLabelValue(i, GetRowLabelValue(i));
        for (size_t j = 0; j < vData[0].size()+1; j++)
        {
            if (i < nFirstNumRow && j < vData[0].size())
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                this->SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                this->SetCellBackgroundColour(i, j, *wxLIGHT_GREY);
            }
            else if (i == vData.size() || j == vData[0].size())
            {
                this->SetColLabelValue(j, GetColLabelValue(j));
                this->SetCellBackgroundColour(i, j, wxColor(230,230,230));
                this->SetReadOnly(i, j);
                continue;
            }
            else
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTER);
            }
            this->SetCellValue(i, j, replaceCtrlChars(vData[i][j]));
            this->SetReadOnly(i, j);
        }
    }

    int nColSize = GetColSize(0);
    this->AutoSize();
    this->SetColSize(vData[0].size(), nColSize);
    nHeight = GetRowHeight(0) * (vData.size()+4.5);
    nWidth = nColSize*(vData[0].size()+2.5);
}


