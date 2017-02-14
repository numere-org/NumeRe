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
#include "../kernel/core/language.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/tokenzr.h>

extern Language _guilang;

BEGIN_EVENT_TABLE(TableViewer, wxGrid)
    EVT_KEY_DOWN                (TableViewer::OnKeyDown)
    EVT_CHAR                    (TableViewer::OnChar)
    EVT_GRID_CELL_CHANGING      (TableViewer::OnCellChange)
    EVT_GRID_CELL_RIGHT_CLICK   (TableViewer::OnCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK  (TableViewer::OnLabelRightClick)
    EVT_MENU_RANGE              (ID_MENU_INSERT_ROW, ID_MENU_PASTE, TableViewer::OnMenu)
    //EVT_ENTER_WINDOW    (TableViewer::OnEnter)
END_EVENT_TABLE()

TableViewer::TableViewer(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
            : wxGrid(parent, id, pos, size, style, name), nHeight(600), nWidth(800), nFirstNumRow(1), readOnly(true)
{
    GetGridWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridRowLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridColLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridCornerLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);

    FrameColor = wxColor(230,230,230);
    HeadlineColor = *wxLIGHT_GREY;

    m_popUpMenu.Append(ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS"));
}



void TableViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
    else if (event.GetKeyCode() == WXK_UP)
    {
        this->MoveCursorUp(false);
    }
    else if (event.GetKeyCode() == WXK_DOWN)
    {
        /*if (!readOnly && this->GetCursorRow() +1 == this->GetRows())
        {
            this->AppendRows();
            updateFrame();
        }*/
        this->MoveCursorDown(false);
    }
    else if (event.GetKeyCode() == WXK_RETURN)
    {
        /*if (!readOnly && this->GetCursorRow() +1 == this->GetRows())
        {
            this->AppendRows();
            updateFrame();
        }*/
        this->MoveCursorDown(false);
    }
    else if (event.GetKeyCode() == WXK_LEFT)
    {
        this->MoveCursorLeft(false);
    }
    else if (event.GetKeyCode() == WXK_RIGHT)
    {
        /*if (!readOnly && this->GetCursorColumn() +1 == this->GetCols())
        {
            this->AppendCols();
            updateFrame();
        }*/
        this->MoveCursorRight(false);
    }
    else if (event.GetKeyCode() == WXK_TAB)
    {
        /*if (!readOnly && this->GetCursorColumn() +1 == this->GetCols())
        {
            this->AppendCols();
            updateFrame();
        }*/
        this->MoveCursorRight(false);
    }
    else if (event.GetKeyCode() == WXK_DELETE)
    {
        if (!readOnly)
        {
            deleteSelection();
        }
    }
    else if (event.ControlDown() && event.ShiftDown())
    {
        if (event.GetKeyCode() == 'C')
        {
            copyContents();
        }
        else if (event.GetKeyCode() == 'V')
        {
            pasteContents();
        }
        //event.Skip();
    }
    else
    {
        event.Skip();
    }
}

void TableViewer::OnChar(wxKeyEvent& event)
{
    if (this->GetCursorColumn()+1 == this->GetCols())
    {
        this->AppendCols();
        updateFrame();
    }
    if (this->GetCursorRow()+1 == this->GetRows())
    {
        this->AppendRows();
        updateFrame();
    }
    event.Skip();
}

void TableViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

void TableViewer::OnCellChange(wxGridEvent& event)
{
    wxString newContent = event.GetString();
    if (!isNumerical(newContent.ToStdString()))
    {
        if (event.GetRow() == (int)nFirstNumRow)
        {
            if ((size_t)findEmptyHeadline(event.GetCol()) != nFirstNumRow)
            {
                this->SetCellValue(findEmptyHeadline(event.GetCol()), event.GetCol(), newContent);
                event.Veto();
                return;
            }
            else
            {
                this->InsertRows(nFirstNumRow);
                nFirstNumRow++;
                updateFrame();
            }
        }
        else if (event.GetRow() > (int)nFirstNumRow)
        {
            if ((size_t)findEmptyHeadline(event.GetCol()) != nFirstNumRow)
            {
                this->SetCellValue(findEmptyHeadline(event.GetCol()), event.GetCol(), newContent);
                event.Veto();
                return;
            }
            else
            {
                this->SetCellBackgroundColour(event.GetRow(), event.GetCol(), *wxRED);
            }
        }
    }
    else if ((size_t)event.GetRow() >= nFirstNumRow)
        this->SetCellBackgroundColour(event.GetRow(), event.GetCol(), *wxWHITE);
    event.Skip();
}

int TableViewer::findEmptyHeadline(int nCol)
{
    for (size_t i = 0; i < nFirstNumRow; i++)
    {
        if (!this->GetCellValue(i, nCol).length())
            return i;
    }
    return nFirstNumRow;
}

int TableViewer::findLastElement(int nCol)
{
    for (int i = this->GetRows()-1; i >= 0; i--)
    {
        if (this->GetCellValue(i,nCol).length())
            return i;
    }
    return 0;
}

void TableViewer::updateFrame()
{
    wxFont font = this->GetCellFont(0,0);
    font.SetWeight(wxFONTWEIGHT_NORMAL);
    for (int i = 0; i < this->GetRows(); i++)
    {
        this->SetRowLabelValue(i, GetRowLabelValue(i));
        for (int j = 0; j < this->GetCols(); j++)
        {
            if (!i)
                this->SetColLabelValue(j, GetColLabelValue(j));

            if (i+1 == this->GetRows() || j+1 == this->GetCols())
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                this->SetCellFont(i, j, font);
                this->SetCellBackgroundColour(i, j, FrameColor);
            }
            else if (i < (int)nFirstNumRow)
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                this->SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                this->SetCellBackgroundColour(i, j, HeadlineColor);
            }
            else
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                this->SetCellFont(i, j, font);
                this->SetCellBackgroundColour(i, j, *wxWHITE);
            }
        }
    }
}

void TableViewer::deleteSelection()
{
    if (!(GetSelectedCells().size()
        || GetSelectedCols().size()
        || GetSelectedRows().size()
        || GetSelectionBlockTopLeft().size()
        || GetSelectionBlockBottomRight().size()))
        this->SetCellValue(this->GetCursorRow(), this->GetCursorColumn(), "");
    if (GetSelectedCells().size())
    {
        wxGridCellCoordsArray cellarray = GetSelectedCells();
        for (size_t i = 0; i < cellarray.size(); i++)
        {
            this->SetCellValue(cellarray[i], "");
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
                this->SetCellValue(i,j, "");
            }
        }
    }
    else if (GetSelectedCols().size())
    {
        wxArrayInt colarray = GetSelectedCols();
        for (int i = 0; i < GetRows(); i++)
        {
            for (size_t j = 0; j < colarray.size(); j++)
            {
                this->SetCellValue(i, colarray[j], "");
            }
        }
    }
    else
    {
        wxArrayInt rowarray = GetSelectedRows();
        for (size_t i = 0; i < rowarray.size(); i++)
        {
            for (int j = 0; j < GetCols(); j++)
            {
                this->SetCellValue(rowarray[i], j, "");
            }
        }
    }
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
    string sNums = "0123456789,.eE+- ";
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
    wxString sSelection;
    if (!(GetSelectedCells().size() || GetSelectedCols().size() || GetSelectedRows().size() || GetSelectionBlockTopLeft().size() || GetSelectionBlockBottomRight().size()))
        sSelection = copyCell(this->GetCursorRow(), this->GetCursorColumn());
    if (GetSelectedCells().size())
    {
        wxGridCellCoordsArray cellarray = GetSelectedCells();
        for (size_t i = 0; i < cellarray.size(); i++)
        {
            sSelection += copyCell(cellarray[i].GetRow(), cellarray[i].GetCol());
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
                sSelection += copyCell(i,j);
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
                sSelection += copyCell(i, colarray[j]);
                if (j < colarray.size()-1)
                    sSelection += "\t";
            }
            if (i < GetRows()-1);
                sSelection += "\n";
        }
    }
    else if (GetSelectedRows().size())
    {
        wxArrayInt rowarray = GetSelectedRows();
        for (size_t i = 0; i < rowarray.size(); i++)
        {
            for (int j = 0; j < GetCols(); j++)
            {
                sSelection += copyCell(rowarray[i], j);
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

void TableViewer::pasteContents()
{
    vector<wxString> vTableData;
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            vTableData = getLinesFromPaste(data.GetText());
        }
        wxTheClipboard->Close();
    }
    else
        return;

    if (!vTableData.size())
        return;


    int nLines = vTableData.size();
    int nCols = 0;
    int nSkip = 0;
    //cerr << nLines << endl;
    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        if (!isNumerical(vTableData[i].ToStdString()))
        {
            nLines--;
            nSkip++;
            if (nLines > (int)i+1 && vTableData[i+1].find(' ') == string::npos && vTableData[i].find(' ') != string::npos)
            {
                for (unsigned int j = 0; j < vTableData[i].size(); j++)
                {
                    if (vTableData[i][j] == ' ')
                        vTableData[i][j] = '_';
                }
            }
        }
        else
            break;
    }
    if (nLines <= 0)
        return;



    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        //stripTrailingSpaces(vTableData[i]);

        wxStringTokenizer tok(vTableData[i], " ");

        if (nCols < (int)tok.CountTokens())
            nCols = (int)tok.CountTokens();
    }

    wxGridCellCoords topleft = CreateEmptyGridSpace(nLines, nSkip, nCols);

    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        wxStringTokenizer tok(vTableData[i], " ");
        wxString sLine;

        long long int j = 0;
        while (tok.HasMoreTokens())
        {
            sLine = tok.GetNextToken();
            if (sLine[sLine.length()-1] == '%')
                sLine.erase(sLine.length()-1);
            if (isNumerical(sLine.ToStdString()) && sLine != "NAN" && sLine != "NaN" && sLine != "nan")
            {
                if (i < (size_t)nSkip && nSkip)
                {
                    if (sLine.length())
                    {
                        /*if (!i)
                            sHeadLine[j] = sLine;
                        else
                            sHeadLine[j] += "\\n" + sLine;*/
                        this->SetCellValue(i, topleft.GetCol()+j, sLine);
                    }
                }
                else
                {
                    this->SetCellValue(topleft.GetRow()+i-nSkip, topleft.GetCol()+j, sLine);
                    //dDatafile[i-nSkip][j] = StrToDb(sLine);
                    //bValidEntry[i-nSkip][j] = true;
                }
            }
            else if (i < (size_t)nSkip && nSkip)
            {
                if (sLine.length())
                {
                    this->SetCellValue(i, topleft.GetCol()+j, sLine);

                    /*if (!i || sHeadLine[j] == "Spalte_" + toString(j+1))
                        sHeadLine[j] = sLine;
                    else
                        sHeadLine[j] += "\\n" + sLine;*/
                }
            }
            else
            {
                this->SetCellValue(topleft.GetRow()+i-nSkip, topleft.GetCol()+j, sLine);
                //dDatafile[i-nSkip][j] = NAN;
                //bValidEntry[i-nSkip][j] = false;
            }
            j++;
            if (j == nCols)
                break;
        }
    }
}

wxGridCellCoords TableViewer::CreateEmptyGridSpace(int rows, int headrows, int cols)
{
    wxGridCellCoords topLeft(headrows,0);

    for (int i = this->GetCols()-1; i >= 0; i--)
    {
        if (this->GetCellValue(0,i).length())
        {
            if (this->GetCols()-i-1 < cols+1)
                this->AppendCols(cols-(this->GetCols()-(i+1))+1);
            topLeft.SetCol(i+1);
            break;
        }
    }
    for (int i = 0; i < this->GetRows(); i++)
    {
        if (this->GetRowLabelValue(i) != "#")
        {
            if (i >= headrows)
            {
                topLeft.SetRow(i);
                if (this->GetRows()-i < rows+1)
                {
                    this->AppendRows(rows-(this->GetRows()-i)+1);
                }
            }
            else
            {
                nFirstNumRow = headrows;
                this->InsertRows(i, headrows-(i-1));
                for (int j = i-1; j < GetRows(); j++)
                    this->SetRowLabelValue(j, this->GetRowLabelValue(j));
                if (this->GetRows()-headrows < rows+1)
                {
                    this->AppendRows(rows-(this->GetRows()-headrows)+1);
                }
            }
            break;
        }
    }

    updateFrame();

    return topLeft;
}

wxString TableViewer::copyCell(int row, int col)
{
    if (row >= (int)nFirstNumRow)
        return this->GetCellValue(row, col);
    wxString cell = this->GetCellValue(row, col);

    while (cell.find(' ') != string::npos)
        cell[cell.find(' ')] = '_';
    return cell;
}

void TableViewer::replaceDecimalSign(wxString& text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        if (text[i] == ',')
            text[i] = '.';
    }
}

void TableViewer::replaceTabSign(wxString& text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        if (text[i] == '\t')
            text[i] = ' ';
    }
}

vector<wxString> TableViewer::getLinesFromPaste(const wxString& data)
{
    vector<wxString> vPaste;
    bool bKeepEmptyTokens = false;
    wxString sClipboard = data;
    wxString sLine = "";
    while (true)
    {
        if (!sClipboard.length() || sClipboard == "\n")
            break;
        sLine = sClipboard.substr(0, sClipboard.find('\n'));
        if (sLine.length() && sLine[sLine.length()-1] == (char)13) // CR entfernen
            sLine.erase(sLine.length()-1);
        //cerr << sLine << " " << (int)sLine.back() << endl;
        if (sClipboard.find('\n') != string::npos)
            sClipboard.erase(0, sClipboard.find('\n')+1);
        else
            sClipboard.clear();
        //StripSpaces(sLine);
        if (!isNumerical(sLine.ToStdString()) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ' ')
                    sLine[i] = '_';
            }
            if (!bKeepEmptyTokens)
                bKeepEmptyTokens = true;
        }
        replaceTabSign(sLine);
        if (sLine.find_first_not_of(' ') == string::npos)
            continue;
        if (isNumerical(sLine.ToStdString()) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
            replaceDecimalSign(sLine);
        else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                    sLine[i] = '.';
                if (sLine[i] == ';')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        vPaste.push_back(sLine);
    }
    return vPaste;
}

void TableViewer::SetData(const vector<vector<string> >& vData)
{
    this->CreateGrid(vData.size()+1, vData[0].size()+1);
    nFirstNumRow = -1;
    for (size_t i = 0; i < vData.size(); i++)
    {
        for (size_t j = 0; j < vData[0].size(); j++)
        {
            if ((vData[i][j].length() && (isNumerical(vData[i][j]) || vData[i][j] == "---")) || (!vData[i][j].length() && i+1 == vData.size()))
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
                this->SetReadOnly(i, j, readOnly);
                continue;
            }
            else
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTER);
            }
            this->SetCellValue(i, j, replaceCtrlChars(vData[i][j]));
            this->SetReadOnly(i, j, readOnly);
        }
    }

    int nColSize = GetColSize(0);
    if (readOnly)
        this->AutoSize();
    this->SetColSize(vData[0].size(), nColSize);
    nHeight = GetRowHeight(0) * (vData.size()+4.5);
    nWidth = nColSize*(vData[0].size()+2.5);
    if (!readOnly)
    {
        if (nHeight < 400)
            nHeight = 400;
        if (nWidth < 600)
            nWidth = 600;
    }
}

void TableViewer::SetTableReadOnly(bool isReadOnly)
{
    readOnly = isReadOnly;

    if (!readOnly)
    {
        //this->EnableDragCell();
        //this->EnableDragColMove();
        m_popUpMenu.Append(ID_MENU_PASTE, _guilang.get("GUI_PASTE_TABLE_CONTENTS"));
        m_popUpMenu.AppendSeparator();
        m_popUpMenu.Append(ID_MENU_INSERT_ROW, _guilang.get("GUI_INSERT_TABLE_ROW"));
        m_popUpMenu.Append(ID_MENU_INSERT_COL, _guilang.get("GUI_INSERT_TABLE_COL"));
        m_popUpMenu.Append(ID_MENU_INSERT_CELL, _guilang.get("GUI_INSERT_TABLE_CELL"));
        m_popUpMenu.AppendSeparator();
        m_popUpMenu.Append(ID_MENU_REMOVE_ROW, _guilang.get("GUI_REMOVE_TABLE_ROW"));
        m_popUpMenu.Append(ID_MENU_REMOVE_COL, _guilang.get("GUI_REMOVE_TABLE_COL"));
        m_popUpMenu.Append(ID_MENU_REMOVE_CELL, _guilang.get("GUI_REMOVE_TABLE_CELL"));
    }
}

void TableViewer::SetDefaultSize(size_t rows, size_t cols)
{
    this->CreateGrid(rows+1,cols+1);
    for (size_t i = 0; i < rows+1; i++)
    {
        this->SetRowLabelValue(i, GetRowLabelValue(i));
        for (size_t j = 0; j < cols+1; j++)
        {
            if (i < 1)
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                this->SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                this->SetCellBackgroundColour(i, j, *wxLIGHT_GREY);
            }
            else if (i == rows || j == cols)
            {
                this->SetColLabelValue(j, GetColLabelValue(j));
                this->SetCellBackgroundColour(i, j, wxColor(230,230,230));
                this->SetReadOnly(i, j, readOnly);
                continue;
            }
            else
            {
                this->SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTER);
            }
            this->SetReadOnly(i, j, readOnly);
        }
    }
}

void TableViewer::OnCellRightClick(wxGridEvent& event)
{
    m_lastRightClick = wxGridCellCoords(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        for (int i = ID_MENU_INSERT_ROW; i <= ID_MENU_REMOVE_CELL; i++)
            m_popUpMenu.Enable(i,true);
    }
    PopupMenu(&m_popUpMenu, event.GetPosition());
}

void TableViewer::OnLabelRightClick(wxGridEvent& event)
{
    m_lastRightClick = wxGridCellCoords(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        if (event.GetRow() == -1)
        {
            m_popUpMenu.Enable(ID_MENU_INSERT_ROW, false);
            m_popUpMenu.Enable(ID_MENU_REMOVE_ROW, false);
            m_popUpMenu.Enable(ID_MENU_INSERT_COL, true);
            m_popUpMenu.Enable(ID_MENU_REMOVE_COL, true);
        }
        else if (event.GetCol() == -1)
        {
            m_popUpMenu.Enable(ID_MENU_INSERT_ROW, true);
            m_popUpMenu.Enable(ID_MENU_REMOVE_ROW, true);
            m_popUpMenu.Enable(ID_MENU_INSERT_COL, false);
            m_popUpMenu.Enable(ID_MENU_REMOVE_COL, false);
        }

        m_popUpMenu.Enable(ID_MENU_INSERT_CELL, false);
        m_popUpMenu.Enable(ID_MENU_REMOVE_CELL, false);
    }
    PopupMenu(&m_popUpMenu, event.GetPosition());
}

void TableViewer::OnMenu(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        // fallthrough intended
        case ID_MENU_INSERT_ROW:
        case ID_MENU_INSERT_COL:
        case ID_MENU_INSERT_CELL:
            insertElement(event.GetId());
            break;
        case ID_MENU_REMOVE_ROW:
        case ID_MENU_REMOVE_COL:
        case ID_MENU_REMOVE_CELL:
            removeElement(event.GetId());
            break;
        case ID_MENU_COPY:
            copyContents();
            break;
        case ID_MENU_PASTE:
            pasteContents();
            break;
    }
}

void TableViewer::insertElement(int id)
{
    if (id == ID_MENU_INSERT_ROW)
        this->InsertRows(m_lastRightClick.GetRow());
    else if (id == ID_MENU_INSERT_COL)
        this->InsertCols(m_lastRightClick.GetCol());
    else
    {
        int nLastLine = findLastElement(m_lastRightClick.GetCol());
        if (nLastLine+2 == this->GetRows())
            this->AppendRows();
        for (int i = nLastLine; i >= m_lastRightClick.GetRow(); i--)
        {
            this->SetCellValue(i+1, m_lastRightClick.GetCol(), this->GetCellValue(i, m_lastRightClick.GetCol()));
        }
        this->SetCellValue(m_lastRightClick.GetRow(), m_lastRightClick.GetCol(), "");
    }
    updateFrame();
}

void TableViewer::removeElement(int id)
{
    if (id == ID_MENU_REMOVE_ROW)
        this->DeleteRows(m_lastRightClick.GetRow());
    else if (id == ID_MENU_REMOVE_COL)
        this->DeleteCols(m_lastRightClick.GetCol());
    else
    {
        int nLastLine = findLastElement(m_lastRightClick.GetCol());
        for (int i = m_lastRightClick.GetRow(); i < nLastLine; i++)
        {
            this->SetCellValue(i, m_lastRightClick.GetCol(), this->GetCellValue(i+1, m_lastRightClick.GetCol()));
        }
        this->SetCellValue(nLastLine, m_lastRightClick.GetCol(), "");
    }
    updateFrame();
}

vector<vector<string> > TableViewer::GetData()
{
    vector<vector<string> > vTableContents;
    for (int i = 0; i < this->GetRows()-1; i++)
    {
        vTableContents.push_back(vector<string>(this->GetCols()-1, ""));
        for (int j = 0; j < this->GetCols()-1; j++)
        {
            if (!j && this->GetRowLabelValue(i) == "#")
                vTableContents[i][j] = "#"+this->GetCellValue(i,j);
            else if (this->GetCellBackgroundColour(i,j) == *wxRED)
                vTableContents[i][j] == "---";
            else
                vTableContents[i][j] = this->GetCellValue(i,j);
        }
    }
    return vTableContents;
}

