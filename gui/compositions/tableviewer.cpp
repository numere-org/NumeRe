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
#include "gridtable.hpp"
#include "../../kernel/core/ui/language.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/tokenzr.h>

double StrToDb(const string&);
string toString(double, int);

extern Language _guilang;

BEGIN_EVENT_TABLE(TableViewer, wxGrid)
    EVT_KEY_DOWN                (TableViewer::OnKeyDown)
    EVT_CHAR                    (TableViewer::OnChar)
    EVT_GRID_CELL_CHANGING      (TableViewer::OnCellChange)
    EVT_GRID_CELL_RIGHT_CLICK   (TableViewer::OnCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK  (TableViewer::OnLabelRightClick)
    EVT_GRID_LABEL_LEFT_DCLICK  (TableViewer::OnLabelDoubleClick)
    EVT_MENU_RANGE              (ID_MENU_INSERT_ROW, ID_MENU_PASTE_HERE, TableViewer::OnMenu)
    EVT_GRID_SELECT_CELL        (TableViewer::OnCellSelect)
    EVT_GRID_RANGE_SELECT       (TableViewer::OnCellRangeSelect)
END_EVENT_TABLE()

// Constructor
TableViewer::TableViewer(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
            : wxGrid(parent, id, pos, size, style, name), nHeight(600), nWidth(800), nFirstNumRow(1), readOnly(true)
{
    // Bind enter window events dynamically to the local event handler funtion
    GetGridWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridRowLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridColLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);
    GetGridCornerLabelWindow()->Bind(wxEVT_ENTER_WINDOW, &TableViewer::OnEnter, this);

    // Cells are always aligned right and centered vertically
    SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);

    // DEfine the standard functions
    FrameColor = wxColor(230, 230, 230);
    HeadlineColor = *wxLIGHT_GREY;
    HighlightColor = wxColor(192, 227, 248);
    HighlightHeadlineColor = wxColor(131, 200, 241);

    // Prepare the context menu
    m_popUpMenu.Append(ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS"));

    // prepare the status bar
    m_statusBar = statusbar;
    int widths[3] = {-1, -1, -3};
    m_statusBar->SetStatusWidths(3, widths);
}

// This private member function will layout the initial grid
// after the data table was set
void TableViewer::layoutGrid()
{
    BeginBatch();

    // Enable editing, if the table was not
    // set to read-only mode
    EnableEditing(!readOnly);

    // Search the boundaries and color the frame correspondingly
    for (int i = 0; i < GetNumberRows(); i++)
    {
        for (int j = 0; j < GetNumberCols(); j++)
        {
            if (i < (int)nFirstNumRow && j < GetNumberCols()-1)
            {
                // Headlines
                SetCellFont(i, j, GetCellFont(i, j).MakeBold());
                SetCellBackgroundColour(i, j, HeadlineColor);
            }
            else if (i == GetNumberRows()-1 || j == GetNumberCols()-1)
            {
                // Surrounding frame
                this->SetCellBackgroundColour(i, j, FrameColor);
            }
            else if (!nFirstNumRow && this->GetCellValue(i, j)[0] == '"')
            {
                this->SetCellAlignment(wxALIGN_LEFT, i, j);
            }
        }
    }

    // Define the minimal size of the window depending
    // on the number of columns. The maximal size is defined
    // by the surrounding ViewerFrame class
    int nColSize = GetColSize(0);

    nHeight = GetRowHeight(0) * (GetNumberRows()+3.5);
    nWidth = nColSize*(GetNumberCols()+1.5);

    if (!readOnly)
    {
        if (nHeight < 400)
            nHeight = 400;
    }

    if (nWidth < 600)
        nWidth = 600;

    EndBatch();
}

// This member function tracks the entered keys and
// processes a keybord navigation.
void TableViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the table
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
    else if (event.GetKeyCode() == WXK_UP)
    {
        this->MoveCursorUp(false);
        //highlightCursorPosition();
    }
    else if (event.GetKeyCode() == WXK_DOWN)
    {
        /*if (!readOnly && this->GetCursorRow() +1 == this->GetRows())
        {
            this->AppendRows();
            updateFrame();
        }*/
        this->MoveCursorDown(false);
        //highlightCursorPosition();
    }
    else if (event.GetKeyCode() == WXK_RETURN)
    {
        /*if (!readOnly && this->GetCursorRow() +1 == this->GetRows())
        {
            this->AppendRows();
            updateFrame();
        }*/
        this->MoveCursorDown(false);
        //highlightCursorPosition();
    }
    else if (event.GetKeyCode() == WXK_LEFT)
    {
        this->MoveCursorLeft(false);
        //highlightCursorPosition();
    }
    else if (event.GetKeyCode() == WXK_RIGHT)
    {
        /*if (!readOnly && this->GetCursorColumn() +1 == this->GetCols())
        {
            this->AppendCols();
            updateFrame();
        }*/
        this->MoveCursorRight(false);
        //highlightCursorPosition();
    }
    else if (event.GetKeyCode() == WXK_TAB)
    {
        /*if (!readOnly && this->GetCursorColumn() +1 == this->GetCols())
        {
            this->AppendCols();
            updateFrame();
        }*/
        this->MoveCursorRight(false);
        //highlightCursorPosition();
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

// This member function appends necessary columns or rows,
// if the user enterd a character in the last column or the
// last row
void TableViewer::OnChar(wxKeyEvent& event)
{
    // Is this the last column?
    if (this->GetCursorColumn()+1 == this->GetCols())
    {
        this->AppendCols();
        updateFrame();
    }

    // Is this the last row?
    if (this->GetCursorRow()+1 == this->GetRows())
    {
        this->AppendRows();
        updateFrame();
    }

    event.Skip();
}

// This member function is the event handler for entering
// this window. It will activate the focus and bring it to
// the front
void TableViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

// This member function processes the values entered in
// the table and updates the frame after entering (due to
// a possible created new headline row)
void TableViewer::OnCellChange(wxGridEvent& event)
{
    SetCellValue(event.GetRow(), event.GetCol(), event.GetString());

    updateFrame();
    highlightCursorPosition(GetGridCursorRow(), GetGridCursorCol());
    event.Veto();
}

// This member function will highlight the cursor position
// in the grid and update the status bar correspondingly
void TableViewer::OnCellSelect(wxGridEvent& event)
{
    highlightCursorPosition(event.GetRow(), event.GetCol());
    wxGridCellCoords coords(event.GetRow(), event.GetCol());
    updateStatusBar(coords, coords, &coords);
}

// This member function will calculate the simple statistics
// if the user selected a range of cells in the table
void TableViewer::OnCellRangeSelect(wxGridRangeSelectEvent& event)
{
    if (event.Selecting())
        updateStatusBar(event.GetTopLeftCoords(), event.GetBottomRightCoords());
}

// This member function will autosize the columns, if the
// user double clicked on their labels
void TableViewer::OnLabelDoubleClick(wxGridEvent& event)
{
    if (event.GetCol() >= 0)
        AutoSizeColumn(event.GetCol());
}

// This member function will search the last non-empty
// cell in the selected column
int TableViewer::findLastElement(int nCol)
{
    for (int i = this->GetRows()-1; i >= 0; i--)
    {
        if (GetCellValue(i, nCol).length() && GetCellValue(i, nCol) != "---")
            return i;
    }

    return 0;
}

// This member function is called every time the grid
// itself changes to draw the frame colours.
void TableViewer::updateFrame()
{
    wxFont font = this->GetCellFont(0,0);
    font.SetWeight(wxFONTWEIGHT_NORMAL);

    for (int i = 0; i < this->GetRows(); i++)
    {
        for (int j = 0; j < this->GetCols(); j++)
        {
            if (i+1 == this->GetRows() || j+1 == this->GetCols())
            {
                // Surrounding frame
                SetCellFont(i, j, font);
                SetCellBackgroundColour(i, j, FrameColor);
            }
            else if (GetRowLabelValue(i) == "#")
            {
                // Headline
                SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                SetCellBackgroundColour(i, j, HeadlineColor);
            }
            else
            {
                // Remaining central cells
                SetCellFont(i, j, font);
                SetCellBackgroundColour(i, j, *wxWHITE);
            }
        }
    }

    lastCursorPosition = wxGridCellCoords();

    // Highlight the position of the cursor
    highlightCursorPosition(this->GetCursorRow(), this->GetCursorColumn());
}

// This member function will delete the contents of
// the selected cells by setting them to NaN
void TableViewer::deleteSelection()
{
    // Simple case: only one cell
    if (!(GetSelectedCells().size()
        || GetSelectedCols().size()
        || GetSelectedRows().size()
        || GetSelectionBlockTopLeft().size()
        || GetSelectionBlockBottomRight().size()))
        this->SetCellValue(this->GetCursorRow(), this->GetCursorColumn(), "");

    // More difficult: multiple selections
    if (GetSelectedCells().size())
    {
        // not a block layout
        wxGridCellCoordsArray cellarray = GetSelectedCells();

        for (size_t i = 0; i < cellarray.size(); i++)
        {
            this->SetCellValue(cellarray[i], "");
        }
    }
    else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size())
    {
        // block layout
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
        // multiple selected columns
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
        // multiple selected rows
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

// This is a simple helper function to determine, whether
// the entered cell value is a numerical value
bool TableViewer::isNumerical(const string& sCell)
{
    static string sNums = "0123456789,.eE+- INFinf";
    return sCell.find_first_not_of(sNums) == string::npos;
}

// This member function will determine, whether the selected
// column is completely empty or not
bool TableViewer::isEmptyCol(int col)
{
    if (col >= this->GetCols() || col < 0)
        return false;

    for (int i = nFirstNumRow; i < this->GetRows()-1; i++)
    {
        if (GetCellValue(i, col).length() && GetCellValue(i, col) != "---")
            return false;
    }

    return true;
}

// This member function is a simple helper to replace
// underscores with whitespaces
wxString TableViewer::replaceCtrlChars(const wxString& sStr)
{
    wxString sReturn = sStr;

    while (sReturn.find('_') != string::npos)
        sReturn[sReturn.find('_')] = ' ';

    return sReturn;
}

// This member function will copy the contents of the
// selected cells to the clipboard.
// Maybe it should consider the language and convert
// the decimal dots correspondingly?
void TableViewer::copyContents()
{
    wxString sSelection;

    // Simple case: only one cell selected
    if (!(GetSelectedCells().size() || GetSelectedCols().size() || GetSelectedRows().size() || GetSelectionBlockTopLeft().size() || GetSelectionBlockBottomRight().size()))
        sSelection = copyCell(this->GetCursorRow(), this->GetCursorColumn());

    // More diffcult: more than one cell selected
    if (GetSelectedCells().size())
    {
        // Non-block selection
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
        // Block selection
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
        // Multiple selected columns
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
        // Multiple selected rows
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

    // Open the clipboard and store the selection
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(sSelection));
        wxTheClipboard->Close();
    }

    return;
}

// This member function handles the case that the
// user tries to paste text contents, which may be
// be converted into a table
void TableViewer::pasteContents(bool useCursor)
{
    vector<wxString> vTableData;

    // Get the data from the clipboard as a
    // vector table
    if (wxTheClipboard->Open())
    {
        // Ensure that the data in the clipboard
        // can be converted into simple text
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

    // Do nothing if the data is empty
    if (!vTableData.size())
        return;

    int nLines = vTableData.size();
    int nCols = 0;
    int nSkip = 0;

    // Distinguish between numerical and text data
    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        if (!isNumerical(vTableData[i].ToStdString()))
        {
            nLines--;
            nSkip++;

            // Replace text data with underscores
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

    // Return if the number of lines is zero
    if (nLines <= 0)
        return;

    // Get the number of columns in the data table
    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        wxStringTokenizer tok(vTableData[i], " ");

        if (nCols < (int)tok.CountTokens())
            nCols = (int)tok.CountTokens();
    }

    // Create a place in the grid to paste the data
    wxGridCellCoords topleft = CreateEmptyGridSpace(nLines, nSkip, nCols, useCursor);

    // Go to the whole data table
    for (unsigned int i = 0; i < vTableData.size(); i++)
    {
        // Tokenize the current line
        wxStringTokenizer tok(vTableData[i], " ");
        wxString sLine;

        long long int j = 0;

        // As long as there are more elements in the
        // tokenizer cache
        while (tok.HasMoreTokens())
        {
            sLine = tok.GetNextToken();

            // Remove trailing percentage signs
            if (sLine[sLine.length()-1] == '%')
                sLine.erase(sLine.length()-1);

            // Set the value to the correct cell
            if (isNumerical(sLine.ToStdString()) && sLine != "NAN" && sLine != "NaN" && sLine != "nan")
            {
                // Numerical value
                if (i < (size_t)nSkip && nSkip)
                {
                    if (sLine.length())
                    {
                        this->SetCellValue(i, topleft.GetCol()+j, sLine);
                    }
                }
                else
                {
                    this->SetCellValue(topleft.GetRow()+i-nSkip, topleft.GetCol()+j, sLine);
                }
            }
            else if (i < (size_t)nSkip && nSkip)
            {
                // string value in the top section
                if (sLine.length())
                {
                    this->SetCellValue(i, topleft.GetCol()+j, sLine);
                }
            }
            else
            {
                // string value
                this->SetCellValue(topleft.GetRow()+i-nSkip, topleft.GetCol()+j, sLine);
            }

            j++;

            // Stop, if the the counter reached the
            // number of prepared columns
            if (j == nCols)
                break;
        }
    }

    // Go to the topleft cell and select the entered block of data
    this->GoToCell(topleft);
    this->SelectBlock(topleft, wxGridCellCoords(topleft.GetRow()+nLines-1, topleft.GetCol()+nCols-1));
}

// This member function draws the coloured columns,
// which shall indicate the position of the cursor
void TableViewer::highlightCursorPosition(int nRow, int nCol)
{
    BeginBatch();

    // Restore the previously coloured lines to their
    // original colour (white and grey). This only has
    // to be done, if there was a cursor previously.
    if (!lastCursorPosition)
    {
        // Needed, because wxGridCellCoords do not
        // declare a operatorbool
    }
    else
    {
        // New column is different
        if (nCol != lastCursorPosition.GetCol())
        {
            for (int i = 0; i < this->GetRows()-1; i++)
            {
                if (i == nRow && lastCursorPosition.GetCol()+1 != this->GetCols())
                    continue;

                if (this->GetRowLabelValue(i) == "#" && lastCursorPosition.GetCol()+1 != this->GetCols())
                    this->SetCellBackgroundColour(i, lastCursorPosition.GetCol(), HeadlineColor);
                else if (lastCursorPosition.GetCol()+1 == this->GetCols())
                    this->SetCellBackgroundColour(i, lastCursorPosition.GetCol(), FrameColor);
                else
                    this->SetCellBackgroundColour(i, lastCursorPosition.GetCol(), *wxWHITE);
            }
        }

        // New row is different
        if (nRow != lastCursorPosition.GetRow())
        {
            for (int j = 0; j < this->GetCols()-1; j++)
            {
                if (j == nCol && lastCursorPosition.GetRow()+1 != this->GetRows())
                    continue;

                if (this->GetRowLabelValue(lastCursorPosition.GetRow()) == "#")
                    this->SetCellBackgroundColour(lastCursorPosition.GetRow(), j, HeadlineColor);
                else if (lastCursorPosition.GetRow()+1 == this->GetRows())
                    this->SetCellBackgroundColour(lastCursorPosition.GetRow(), j, FrameColor);
                else
                    this->SetCellBackgroundColour(lastCursorPosition.GetRow(), j, *wxWHITE);
            }
        }
    }

    // Colour the newly selected column or row (mostly it is only one
    // of both, which has to drawn. This saves time).
    if (!lastCursorPosition)
    {
        // In this case, no column was selected. Colour the
        // column first
        for (int i = 0; i < this->GetRows()-1; i++)
        {
            if (this->GetRowLabelValue(i) == "#")
                this->SetCellBackgroundColour(i, nCol, HighlightHeadlineColor);
            else
                this->SetCellBackgroundColour(i, nCol, HighlightColor);
        }

        // Colour the row
        for (int j = 0; j < this->GetCols()-1; j++)
        {
            if (this->GetRowLabelValue(nRow) == "#")
                this->SetCellBackgroundColour(nRow, j, HighlightHeadlineColor);
            else
                this->SetCellBackgroundColour(nRow, j, HighlightColor);
        }
    }
    else
    {
        // In this case the column is different
        if (lastCursorPosition.GetCol() != nCol)
        {
            for (int i = 0; i < this->GetRows()-1; i++)
            {
                if (this->GetRowLabelValue(i) == "#")
                    this->SetCellBackgroundColour(i, nCol, HighlightHeadlineColor);
                else
                    this->SetCellBackgroundColour(i, nCol, HighlightColor);
            }
        }

        // In this case the row is different
        if (lastCursorPosition.GetRow() != nRow)
        {
            for (int j = 0; j < this->GetCols()-1; j++)
            {
                if (this->GetRowLabelValue(nRow) == "#")
                    this->SetCellBackgroundColour(nRow, j, HighlightHeadlineColor);
                else
                    this->SetCellBackgroundColour(nRow, j, HighlightColor);
            }
        }
    }

    lastCursorPosition = wxGridCellCoords(nRow, nCol);
    EndBatch();
    this->Refresh();
}

// This member function creates the needed space
// in the grid, which is needed to paste data
wxGridCellCoords TableViewer::CreateEmptyGridSpace(int rows, int headrows, int cols, bool useCursor)
{
    wxGridCellCoords topLeft(headrows,0);

    if (useCursor)
    {
        // We use the cursor in this case (this
        // implies overriding existent data)
        topLeft = m_lastRightClick;

        if (this->GetCols()-1 < topLeft.GetCol() + cols)
            this->AppendCols(topLeft.GetCol()+cols-this->GetCols()+1, true);

        if (this->GetRows()-1 < topLeft.GetRow() + rows + headrows)
            this->AppendRows(topLeft.GetRow()+rows + headrows-this->GetRows()+1, true);
    }
    else
    {
        // In this case, we search for empty columns
        // to insert the data
        for (int i = this->GetCols()-1; i >= 0; i--)
        {
            if (!isEmptyCol(i))
            {
                if (this->GetCols()-i-1 < cols+1)
                    this->AppendCols(cols-(this->GetCols()-(i+1))+1);

                topLeft.SetCol(i+1);
                break;
            }

            if (!i)
            {
                if (this->GetCols()-1 < cols)
                    this->AppendCols(cols-(this->GetCols()-1));

                topLeft.SetCol(0);
            }
        }

        // Now create the needed space
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
    }

    // Redraw the grey frame
    updateFrame();

    return topLeft;
}

// Return the cell value as double
double TableViewer::CellToDouble(int row, int col)
{
    if (GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_FLOAT))
        return GetTable()->GetValueAsDouble(row, col);
    else if (!nFirstNumRow && GetCellValue(row, col)[0] != '"' && isNumerical(GetCellValue(row, col).ToStdString()))
    {
        return atof(GetCellValue(row, col).ToStdString().c_str());
    }
    return NAN;
}

// This member function calculates the minimal
// value of the selected cells
double TableViewer::calculateMin(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    double dMin = NAN;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
            if (isnan(dMin) || CellToDouble(i, j) < dMin)
                dMin = CellToDouble(i,j);
        }
    }

    return dMin;
}

// This member function calculates the maximal
// value of the selected cells
double TableViewer::calculateMax(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    double dMax = NAN;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
            if (isnan(dMax) || CellToDouble(i, j) > dMax)
                dMax = CellToDouble(i,j);
        }
    }

    return dMax;
}

// This member function calculates the sum
// of the selected cells
double TableViewer::calculateSum(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    double dSum = 0;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
            if (!isnan(CellToDouble(i, j)))
                dSum += CellToDouble(i,j);
        }
    }

    return dSum;
}

// This member function calculates the average
// of the selected cells
double TableViewer::calculateAvg(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    double dSum = 0;
    int nCount = 0;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j <= bottomRight.GetCol(); j++)
        {
            if (!isnan(CellToDouble(i, j)))
            {
                nCount++;
                dSum += CellToDouble(i, j);
            }
        }
    }

    if (nCount)
        return dSum / nCount;

    return 0.0;
}

// This member function updates the status bar of
// the surrounding ViewerFrame
void TableViewer::updateStatusBar(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight, wxGridCellCoords* cursor /*= nullptr*/)
{
    // Get the dimensions
    wxString dim = "Dim: ";
    dim << this->GetRowLabelValue(this->GetRows()-2) << "x" << this->GetCols()-1;

    // Get the current cursor position
    wxString sel = "Cur: ";
    if (cursor)
        sel << this->GetRowLabelValue(cursor->GetRow()) << "," << cursor->GetCol()+1;
    else
        sel << "--,--";

    // Calculate the simple statistics
    wxString statustext = "Min: " + toString(calculateMin(topLeft, bottomRight), 5);
    statustext << " | Max: " << toString(calculateMax(topLeft, bottomRight), 5);
    statustext << " | Sum: " << toString(calculateSum(topLeft, bottomRight), 5);
    statustext << " | Avg: " << toString(calculateAvg(topLeft, bottomRight), 5);

    // Set the status bar valuey
    m_statusBar->SetStatusText(dim);
    m_statusBar->SetStatusText(sel, 1);
    m_statusBar->SetStatusText(statustext, 2);
}

// This member function returns the contents of
// the selected cells and replaces whitespaces
// with underscores on-the-fly
wxString TableViewer::copyCell(int row, int col)
{
    wxString cell = this->GetCellValue(row, col);

    if (cell[0] == '"')
        return cell;

    while (cell.find(' ') != string::npos)
        cell[cell.find(' ')] = '_';

    return cell;
}

// This member function replaces the german
// comma decimal sign with the dot used in
// anglo-american notation
void TableViewer::replaceDecimalSign(wxString& text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        if (text[i] == ',')
            text[i] = '.';
    }
}

// This member function replaces the tabulator
// character with whitespaces
void TableViewer::replaceTabSign(wxString& text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        if (text[i] == '\t')
            text[i] = ' ';
    }
}

// This member function creates a zero-element
// table to visualize empty tables (or
// non-existent ones)
void TableViewer::createZeroElementTable()
{
    SetTable(new GridNumeReTable(NumeRe::Table(1, 1)), true);

    nFirstNumRow = 1;

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
}

// This member function transformes the data obtained
// by the clipboard into a table-like layout
vector<wxString> TableViewer::getLinesFromPaste(const wxString& data)
{
    vector<wxString> vPaste;
    wxString sClipboard = data;
    wxString sLine = "";

    // Endless loop
    while (true)
    {
        // Abort, if the clipboards contents indicate an
        // empty table
        if (!sClipboard.length() || sClipboard == "\n")
            break;

        // Get the next line
        sLine = sClipboard.substr(0, sClipboard.find('\n'));

        // Remove the carriage return character
        if (sLine.length() && sLine[sLine.length()-1] == (char)13)
            sLine.erase(sLine.length()-1);

        // Remove the obtained line from the clipboard
        if (sClipboard.find('\n') != string::npos)
            sClipboard.erase(0, sClipboard.find('\n')+1);
        else
            sClipboard.clear();

        // Replace whitespaces with underscores, if the current
        // line also contains tabulator characters and it is a
        // non-numerical line
        if (!isNumerical(sLine.ToStdString()) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ' ')
                    sLine[i] = '_';
            }
        }

        // Now replace the tabulator characters with
        // whitespaces
        replaceTabSign(sLine);

        // Ignore empty lines
        if (sLine.find_first_not_of(' ') == string::npos)
            continue;

        // Replace the decimal sign, if the line is numerical,
        // otherwise try to detect, whether the comma is used
        // to separate the columns
        if (isNumerical(sLine.ToStdString()) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
            replaceDecimalSign(sLine);
        else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
        {
            // The semicolon is used to separate the columns
            // in this case
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                    sLine[i] = '.';

                if (sLine[i] == ';')
                {
                    sLine[i] = ' ';
                }
            }
        }
        else
        {
            // The comma is used to separate the columns
            // in this case
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                {
                    sLine[i] = ' ';
                }
            }
        }

        // Add the decoded line to the vector table
        vPaste.push_back(sLine);
    }

    return vPaste;
}

// This member function is the data setter for string
// and cluster tables
void TableViewer::SetData(NumeRe::Container<string>& _stringTable)
{
    if (!_stringTable.getCols())
    {
        createZeroElementTable();
        return;
    }

    this->CreateGrid(_stringTable.getRows()+1, _stringTable.getCols()+1);

    // String tables and clusters do not have a headline
    nFirstNumRow = 0;

    for (size_t i = 0; i < _stringTable.getRows()+1; i++)
    {
        this->SetRowLabelValue(i, GetRowLabelValue(i));

        for (size_t j = 0; j < _stringTable.getCols()+1; j++)
        {
            if (i == _stringTable.getRows() || j == _stringTable.getCols())
            {
                this->SetColLabelValue(j, toString(j+1));
                continue;
            }

            if (_stringTable.get(i, j).length())
                this->SetCellValue(i, j, _stringTable.get(i, j));
        }
    }

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
}

// This member function is the data setter
// function. It will create an internal GridNumeReTable
// object, which will provide the data for the grid
void TableViewer::SetData(NumeRe::Table& _table)
{
    // Create an empty table, if necessary
    if (_table.isEmpty())
    {
        createZeroElementTable();
        return;
    }

    // Store the number headlines and create the data
    // providing object
    nFirstNumRow = _table.getHeadCount();
    SetTable(new GridNumeReTable(std::move(_table)), true);

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
}

// This member function declares the table to be
// read-only and enables the context menu entries,
// if the table is set to be not read-only
void TableViewer::SetTableReadOnly(bool isReadOnly)
{
    readOnly = isReadOnly;

    if (!readOnly)
    {
        m_popUpMenu.Append(ID_MENU_PASTE, _guilang.get("GUI_PASTE_TABLE_CONTENTS"));
        m_popUpMenu.Append(ID_MENU_PASTE_HERE, _guilang.get("GUI_PASTE_TABLE_CONTENTS_HERE"));
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

// This member function creates an empty table of some size
// It's declared as deprecated
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

// This member function displays the context menu for
// cells
void TableViewer::OnCellRightClick(wxGridEvent& event)
{
    m_lastRightClick = wxGridCellCoords(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        for (int i = ID_MENU_INSERT_ROW; i <= ID_MENU_REMOVE_CELL; i++)
            m_popUpMenu.Enable(i,true);

        m_popUpMenu.Enable(ID_MENU_PASTE_HERE, true);
    }

    PopupMenu(&m_popUpMenu, event.GetPosition());
}

// This member function displays the context menu for
// labels
void TableViewer::OnLabelRightClick(wxGridEvent& event)
{
    m_lastRightClick = wxGridCellCoords(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        // Enable the row entries for row labels and
        // the column entries for columns
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
        m_popUpMenu.Enable(ID_MENU_PASTE_HERE, false);
    }

    PopupMenu(&m_popUpMenu, event.GetPosition());
}

// This member function is the menu command event handler
// function. It will redirect the control to the specified
// functions.
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
        case ID_MENU_PASTE_HERE:
            pasteContents(true);
            break;
    }
}

// This member function processes the insertion of new
// empty cells, columns or rows
void TableViewer::insertElement(int id)
{
    if (id == ID_MENU_INSERT_ROW)
    {
        // New row
        if (m_lastRightClick.GetRow() < (int)nFirstNumRow)
            nFirstNumRow++;

        InsertRows(m_lastRightClick.GetRow());
    }
    else if (id == ID_MENU_INSERT_COL)
    {
        // New column
        InsertCols(m_lastRightClick.GetCol());
    }
    else
    {
        // New cell -> will append an empty line
        // and move the trailing cells one cell
        // downwards
        int nLastLine = findLastElement(m_lastRightClick.GetCol());

        if (nLastLine+2 == GetRows())
            AppendRows();

        for (int i = nLastLine; i >= m_lastRightClick.GetRow(); i--)
        {
            SetCellValue(i+1, m_lastRightClick.GetCol(), GetCellValue(i, m_lastRightClick.GetCol()));
        }

        SetCellValue(m_lastRightClick.GetRow(), m_lastRightClick.GetCol(), "");
    }

    updateFrame();
}

// This member function processes the removing of cells,
// columns or rows
void TableViewer::removeElement(int id)
{
    if (id == ID_MENU_REMOVE_ROW)
    {
        // Remove row
        if (m_lastRightClick.GetRow() < (int)nFirstNumRow)
            nFirstNumRow--;

        DeleteRows(m_lastRightClick.GetRow());
    }
    else if (id == ID_MENU_REMOVE_COL)
    {
        // Remove column
        DeleteCols(m_lastRightClick.GetCol());
    }
    else
    {
        // Remove cell -> implies that the trailing cells
        // will move one cell upwards
        int nLastLine = findLastElement(m_lastRightClick.GetCol());

        for (int i = m_lastRightClick.GetRow(); i < nLastLine; i++)
        {
            this->SetCellValue(i, m_lastRightClick.GetCol(), this->GetCellValue(i+1, m_lastRightClick.GetCol()));
        }

        this->SetCellValue(nLastLine, m_lastRightClick.GetCol(), "");
    }

    updateFrame();
}

// This member function returns the internal NumeRe::Table
// from the data provider object
// Note: the data provider is most likely not in a valid
// state after a call to this function
NumeRe::Table TableViewer::GetData()
{
    return static_cast<GridNumeReTable*>(GetTable())->getTable();
}
