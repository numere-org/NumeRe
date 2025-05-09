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
#include "../NumeReWindow.h"
#include "gridtable.hpp"
#include "tableeditpanel.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../kernel/core/datamanagement/tablecolumn.hpp"
#include "../../kernel/core/io/file.hpp"
#include "../../kernel/core/io/logger.hpp"
#include "../../kernel/kernel.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/tokenzr.h>
#include <wx/infobar.h>

#include <memory>

#include "cellattributes.hpp"

#define STATUSBAR_PRECISION 5
#define MAXIMAL_RENDERING_SIZE 5000


extern Language _guilang;

BEGIN_EVENT_TABLE(TableViewer, wxGrid)
    EVT_KEY_DOWN                (TableViewer::OnKeyDown)
    EVT_CHAR                    (TableViewer::OnChar)
    EVT_GRID_CELL_CHANGING      (TableViewer::OnCellChange)
    EVT_GRID_CELL_RIGHT_CLICK   (TableViewer::OnCellRightClick)
    EVT_GRID_CELL_LEFT_DCLICK   (TableViewer::OnCellDoubleClick)
    EVT_GRID_LABEL_RIGHT_CLICK  (TableViewer::OnLabelRightClick)
    EVT_GRID_LABEL_LEFT_DCLICK  (TableViewer::OnLabelDoubleClick)
    EVT_MENU_RANGE              (ID_MENU_SAVE, ID_MENU_TABLE_END, TableViewer::OnMenu)
    EVT_GRID_SELECT_CELL        (TableViewer::OnCellSelect)
    EVT_GRID_RANGE_SELECT       (TableViewer::OnCellRangeSelect)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param statusbar wxStatusBar*
/// \param parentPanel TablePanel*
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
/// \param name const wxString&
///
/////////////////////////////////////////////////
TableViewer::TableViewer(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, TablePanel* parentPanel, NumeReWindow* window, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
            : wxGrid(parent, id, pos, size, style, name), nHeight(600), nWidth(800), nFirstNumRow(1), readOnly(true), m_numereWindow(window)
{
    // Cells are always aligned right and centered vertically
    SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
    SetDefaultRenderer(new AdvStringCellRenderer);
    EnableGridLines(NumeReKernel::getInstance()->getSettings().getSetting(SETTING_B_SHOWGRIDLINES).active());
    SetDefaultRowSize(24);
    SetDefaultColSize(95);

    // Prepare the context menu
    wxMenu* columnMenu = new wxMenu();

    columnMenu->Append(ID_MENU_SORT_COL_ASC, _guilang.get("GUI_TABLE_SORT_ASC"));
    columnMenu->Append(ID_MENU_SORT_COL_DESC, _guilang.get("GUI_TABLE_SORT_DESC"));
    columnMenu->Append(ID_MENU_SORT_COL_CLEAR, _guilang.get("GUI_TABLE_SORT_CLEAR"));
    columnMenu->AppendSeparator();
    columnMenu->Append(ID_MENU_FILTER, _guilang.get("GUI_TABLE_FILTER"));
    columnMenu->Append(ID_MENU_DELETE_FILTER, _guilang.get("GUI_TABLE_DELETE_FILTER"));
    columnMenu->AppendSeparator();
    columnMenu->Append(ID_MENU_CHANGE_COL_TYPE, _guilang.get("GUI_TABLE_CHANGE_COL_TYPE"));

    m_popUpMenu.Append(ID_MENU_COLUMNS, _guilang.get("GUI_TABLE_COLUMN"), columnMenu);
    m_popUpMenu.Append(ID_MENU_CVS, _guilang.get("GUI_TABLE_CVS"));
    m_popUpMenu.AppendSeparator();
    m_popUpMenu.Append(ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS"));

    // prepare the status bar
    m_statusBar = statusbar;
    m_parentPanel = parentPanel;

    isGridNumeReTable = false;
    isSilentCursorMove = false;

    if (m_statusBar)
    {
        int widths[3] = {120, 120, -1};
        m_statusBar->SetStatusWidths(3, widths);
        createMenuBar();
    }
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// layout the initial grid after the data table
/// was set.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::layoutGrid()
{
    BeginBatch();

    // Enable editing, if the table was not
    // set to read-only mode
    EnableEditing(!readOnly);

    if (isGridNumeReTable)
    {
        GridNumeReTable* gridtab = static_cast<GridNumeReTable*>(GetTable());
        m_currentColTypes = gridtab->getColumnTypes();

        for (int j = 0; j < GetNumberCols(); j++)
        {
            if ((int)m_currentColTypes.size() > j)
            {
                wxGridCellAttr* attr = nullptr;

                if (m_currentColTypes[j] == TableColumn::TYPE_STRING)
                {
                    attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                              wxALIGN_LEFT, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                }
                else if (m_currentColTypes[j] == TableColumn::TYPE_CATEGORICAL)
                {
                    attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                              wxALIGN_CENTER, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                }
                else if (m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                {
                    attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                              wxALIGN_CENTER, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvBooleanCellRenderer);
                }
                else
                {
                    attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                              wxALIGN_RIGHT, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                }

                SetColAttr(j, attr);

                if (m_currentColTypes[j] == TableColumn::TYPE_DATETIME)
                {
                    //AutoSizeColumn(j);
                    SetColSize(j, 120);
                    SetColMinimalWidth(j, 120);
                }
            }
        }

        // Set the default editor for this grid
        SetDefaultEditor(new CombinedCellEditor(this));
    }
    else
    {
        // Search the boundaries and color the frame correspondingly
        for (int i = 0; i < GetNumberRows(); i++)
        {
            for (int j = 0; j < GetNumberCols(); j++)
            {
                if (i >= (int)nFirstNumRow && GetCellValue(i, j)[0] == '"')
                    SetCellAlignment(wxALIGN_LEFT, i, j);
            }
        }

    }

    // temporary test for header grouping
    if (readOnly && NumeReKernel::getInstance()->getSettings().getSetting(SETTING_B_AUTOGROUPCOLS).active())
        groupHeaders(0, GetNumberCols(), 0);

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


/////////////////////////////////////////////////
/// \brief Move the cursor around with the arrow
/// keys also considering the modifier keys (i.e.
/// keeping or adding to a rectangular selection).
///
/// \param key int
/// \param shiftDown bool
/// \param controlDown bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::moveCursor(int key, bool shiftDown, bool controlDown)
{
    if (shiftDown && (!IsSelection() || !selectionStart))
        selectionStart = wxGridCellCoords(GetCursorRow(), GetCursorColumn());

    // CTRL down -> move to the end of the blocks
    if (controlDown)
    {
        switch (key)
        {
            case WXK_UP:
                MoveCursorUpBlock(false);
                break;
            case WXK_DOWN:
                MoveCursorDownBlock(false);
                break;
            case WXK_LEFT:
                MoveCursorLeftBlock(false);
                break;
            case WXK_RIGHT:
                MoveCursorRightBlock(false);
                break;
        }
    }
    else
    {
        // Otherwise just move one cell
        switch (key)
        {
            case WXK_UP:
                MoveCursorUp(false);
                break;
            case WXK_DOWN:
                MoveCursorDown(false);
                break;
            case WXK_LEFT:
                MoveCursorLeft(false);
                break;
            case WXK_RIGHT:
                MoveCursorRight(false);
                break;
        }
    }

    // If the shift key is pressed, extend the selection
    if (shiftDown)
        SelectBlock(selectionStart, wxGridCellCoords(GetCursorRow(), GetCursorColumn()));
    else
        selectionStart.Set(-1, -1);
}


/////////////////////////////////////////////////
/// \brief This member function tracks the
/// entered keys and processes a keybord
/// navigation.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the table
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
    else if (event.GetKeyCode() == WXK_UP
             || event.GetKeyCode() == WXK_DOWN
             || event.GetKeyCode() == WXK_LEFT
             || event.GetKeyCode() == WXK_RIGHT)
        moveCursor(event.GetKeyCode(), event.ShiftDown(), event.ControlDown());
    else if (event.ControlDown())
    {
        if (event.ShiftDown() && event.GetKeyCode() == 'C')
            copyContents();
        else if (event.ShiftDown() && event.GetKeyCode() == 'V')
            pasteContents();
        else if (event.GetKeyCode() == 'A')
            SelectAll();
    }
    else if (event.GetKeyCode() == WXK_RETURN)
        MoveCursorDown(false);
    else if (event.GetKeyCode() == WXK_TAB)
        MoveCursorRight(false);
    else if (event.GetKeyCode() == WXK_DELETE)
    {
        if (!readOnly)
            deleteSelection();
    }
    else
        event.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function appends necessary
/// columns or rows, if the user entered a
/// character in the last column or the last row.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnChar(wxKeyEvent& event)
{
    if (readOnly)
        return;

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


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handler for entering this window. It will
/// activate the focus and bring it to the front.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function processes the
/// values entered in the table and updates the
/// frame after entering (due to a possible
/// created new headline row).
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnCellChange(wxGridEvent& event)
{
    SetCellValue(event.GetRow(), event.GetCol(), event.GetString());

    //updateFrame();
    UpdateColumnAlignment(GetGridCursorCol());
    event.Veto();
}


/////////////////////////////////////////////////
/// \brief This member function will highlight
/// the cursor position in the grid and update
/// the status bar correspondingly.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnCellSelect(wxGridEvent& event)
{
    wxGridCellCoords coords(event.GetRow(), event.GetCol());
    updateStatusBar(wxGridCellCoordsContainer(coords, coords), &coords);
    Refresh();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function will calculate
/// the simple statistics if the user selected a
/// range of cells in the table.
///
/// \param event wxGridRangeSelectEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnCellRangeSelect(wxGridRangeSelectEvent& event)
{
    if (event.Selecting())
    {
        if (event.GetTopLeftCoords() == wxGridCellCoords(0, 0)
            && event.GetBottomRightCoords() == wxGridCellCoords(GetRows()-1, GetCols()-1))
        {
            updateStatusBar(wxGridCellCoordsContainer(event.GetTopLeftCoords(), event.GetBottomRightCoords(), this));
            selectedCells.Clear();
        }
        else if (!selectedCells.size())
        {
            for (int i = event.GetTopLeftCoords().GetRow(); i <= event.GetBottomRightCoords().GetRow(); i++)
            {
                for (int j = event.GetTopLeftCoords().GetCol(); j <= event.GetBottomRightCoords().GetCol(); j++)
                {
                    selectedCells.Add(wxGridCellCoords(i, j));
                }
            }

            updateStatusBar(wxGridCellCoordsContainer(event.GetTopLeftCoords(), event.GetBottomRightCoords(), this));
        }
        else
        {
            for (int i = event.GetTopLeftCoords().GetRow(); i <= event.GetBottomRightCoords().GetRow(); i++)
            {
                for (int j = event.GetTopLeftCoords().GetCol(); j <= event.GetBottomRightCoords().GetCol(); j++)
                {
                    selectedCells.Add(wxGridCellCoords(i, j));
                }
            }

            updateStatusBar(wxGridCellCoordsContainer(selectedCells, this));
        }
    }
    else
        selectedCells.Clear();

    // Reset the keyboard selection mode
    if (wxGetMouseState().LeftIsDown())
        selectionStart.Set(-1, -1);
}


/////////////////////////////////////////////////
/// \brief This member function will autosize the
/// columns, if the user double clicked on their
/// labels.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnLabelDoubleClick(wxGridEvent& event)
{
    if (event.GetCol() >= 0)
        AutoSizeColumn(event.GetCol());
    else if (event.GetRow() >= 0)
        AutoSizeRow(event.GetRow());
    else
    {
        int elements = GetRows() * GetCols();

        if (elements > 1500 && m_parentPanel)
            m_parentPanel->showMessage("Autosizing cells. Please wait ...");

        AutoSize();

        if (elements > 1500 && m_parentPanel)
            m_parentPanel->dismissMessage();
    }

    m_parent->Layout();
}


/////////////////////////////////////////////////
/// \brief This member function will search the
/// last non-empty cell in the selected column.
///
/// \param nCol int
/// \return int
///
/////////////////////////////////////////////////
int TableViewer::findLastElement(int nCol)
{
    for (int i = this->GetRows()-1; i >= 0; i--)
    {
        if (GetCellValue(i, nCol).length() && GetCellValue(i, nCol) != "---")
            return i;
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief This member function is called every
/// time the grid itself changes to draw the
/// frame colours.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::updateFrame()
{
    wxFont font = this->GetCellFont(0,0);
    font.SetWeight(wxFONTWEIGHT_NORMAL);

    if (GetRows() > MAXIMAL_RENDERING_SIZE || GetCols() > MAXIMAL_RENDERING_SIZE)
        return;


    for (int i = 0; i < this->GetRows(); i++)
    {
        for (int j = 0; j < this->GetCols(); j++)
        {
            if (i+1 == this->GetRows() || j+1 == this->GetCols())
            {
                // Surrounding frame
                SetCellBackgroundColour(i, j, FrameColor);
            }
            else if (i < (int)nFirstNumRow)
            {
                // Headline
                SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                SetCellBackgroundColour(i, j, HeadlineColor);
                SetCellAlignment(wxALIGN_LEFT, i, j);
            }
            else if ((i+2 == this->GetRows() || j+2 == this->GetCols())
                     && GetCellBackgroundColour(i, j) != HighlightColor)
            {
                // Cells near the boundary, which might have been modified
                SetCellBackgroundColour(i, j, *wxWHITE);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function will delete the
/// contents of the selected cells by setting
/// them to NaN.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::deleteSelection()
{
    wxGridCellCoordsContainer coordsContainer;

    // Handle all possible selection types
    if (GetSelectedCells().size()) // not a block layout
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCells(), this);
    else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size()) // block layout
        coordsContainer = wxGridCellCoordsContainer(GetSelectionBlockTopLeft()[0], GetSelectionBlockBottomRight()[0], this);
    else if (GetSelectedCols().size()) // multiple selected columns
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCols(), GetRows()-1, false, this);
    else if (GetSelectedRows().size()) // multiple selected rows
        coordsContainer = wxGridCellCoordsContainer(GetSelectedRows(), GetCols()-1, true, this);
    else
    {
        // single cell: overwrite and return
        SetCellValue(GetCursorRow(), GetCursorColumn(), "");
        return;
    }

    // Get the extent of the container
    const wxGridCellsExtent& cellsExtent = coordsContainer.getExtent();

    // Go through the extent and handle all selected columns
    for (int i = cellsExtent.m_topleft.GetRow(); i <= cellsExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellsExtent.m_topleft.GetCol(); j <= cellsExtent.m_bottomright.GetCol(); j++)
        {
            if (coordsContainer.contains(i, j))
                SetCellValue(i, j, "");
        }
    }
}


/////////////////////////////////////////////////
/// \brief This is a simple helper function to
/// determine, whether the entered cell value is
/// a numerical value.
///
/// \param sCell const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool TableViewer::isNumerical(const std::string& sCell)
{
    static std::string sNums = "0123456789,.eE+-* INFinf";
    return sCell.find_first_not_of(sNums) == std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This member function will determine,
/// whether the selected column is completely
/// empty or not.
///
/// \param col int
/// \return bool
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This member function is a simple
/// helper to replace underscores with
/// whitespaces.
///
/// \param sStr const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString TableViewer::replaceCtrlChars(const wxString& sStr)
{
    wxString sReturn = sStr;

    while (sReturn.find('_') != std::string::npos)
        sReturn[sReturn.find('_')] = ' ';

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This member function will copy the
/// contents of the selected cells to the
/// clipboard.
///
/// \return void
/// \note Maybe it should consider the language
/// and convert the decimal dots correspondingly?
///
/////////////////////////////////////////////////
void TableViewer::copyContents()
{
    wxString sSelection;

    // Simple case: only one cell selected
    if (!(GetSelectedCells().size() || GetSelectedCols().size() || GetSelectedRows().size() || GetSelectionBlockTopLeft().size() || GetSelectionBlockBottomRight().size()))
        sSelection = GetCellValue(GetCursorRow(), GetCursorColumn());

    // More diffcult: more than one cell selected
    if (GetSelectedCells().size())
    {
        // Non-block selection
        if (isGridNumeReTable)
            sSelection = static_cast<GridNumeReTable*>(GetTable())->serialize(wxGridCellCoordsContainer(GetSelectedCells(), this));
        else
        {
            wxGridCellCoordsArray cellarray = GetSelectedCells();

            for (size_t i = 0; i < cellarray.size(); i++)
            {
                sSelection += GetCellValue(cellarray[i]);

                if (i < cellarray.size()-1)
                    sSelection += "\t";
            }
        }
    }
    else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size())
    {
        // Block selection
        wxGridCellCoordsArray topleftarray = GetSelectionBlockTopLeft();
        wxGridCellCoordsArray bottomrightarray = GetSelectionBlockBottomRight();

        if (isGridNumeReTable)
            sSelection = static_cast<GridNumeReTable*>(GetTable())->serialize(wxGridCellCoordsContainer(topleftarray[0],
                                                                                                        bottomrightarray[0],
                                                                                                        this));
        else
        {
            for (int i = topleftarray[0].GetRow(); i <= bottomrightarray[0].GetRow(); i++)
            {
                for (int j = topleftarray[0].GetCol(); j <= bottomrightarray[0].GetCol(); j++)
                {
                    sSelection += GetCellValue(i,j);

                    if (j < bottomrightarray[0].GetCol())
                        sSelection += "\t";
                }

                if (i < bottomrightarray[0].GetRow())
                    sSelection += "\n";
            }
        }
    }
    else if (GetSelectedCols().size())
    {
        // Multiple selected columns
        if (isGridNumeReTable)
            sSelection = static_cast<GridNumeReTable*>(GetTable())->serialize(wxGridCellCoordsContainer(GetSelectedCols(),
                                                                                                        GetRows()-1, false, this));
        else
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

                if (i < GetRows()-1)
                    sSelection += "\n";
            }
        }
    }
    else if (GetSelectedRows().size())
    {
        // Multiple selected rows
        if (isGridNumeReTable)
            sSelection = static_cast<GridNumeReTable*>(GetTable())->serialize(wxGridCellCoordsContainer(GetSelectedRows(),
                                                                                                        GetCols()-1, true, this));
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
    }

    if (!sSelection.length())
        return;

    // Open the clipboard and store the selection
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(sSelection));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the case
/// that the user tries to paste text contents,
/// which may be be converted into a table.
///
/// \param useCursor bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::pasteContents(bool useCursor)
{
    std::vector<wxString> vTableData;

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

    if (nLines && !useCursor && !isNumerical(vTableData.front().ToStdString()))
    {
        nSkip++;
        nLines--;
    }

    // Return if the number of lines is zero
    if (nLines <= 0)
        return;

    // Get the number of columns in the data table
    for (size_t i = 0; i < vTableData.size(); i++)
    {
        wxStringTokenizer tok(vTableData[i], " ");

        if (nCols < (int)tok.CountTokens())
            nCols = (int)tok.CountTokens();
    }

    // Create a place in the grid to paste the data
    wxGridCellCoords topleft = CreateEmptyGridSpace(nLines, nSkip, nCols, useCursor);

    // Go to the whole data table
    for (size_t i = 0; i < vTableData.size(); i++)
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
            sLine.Replace("\1", " ");

            // Remove trailing percentage signs
            if (sLine[sLine.length()-1] == '%')
                sLine.erase(sLine.length()-1);

            // Set the value to the correct cell
            this->SetCellValue(topleft.GetRow()+i-nSkip, topleft.GetCol()+j, sLine);
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


/////////////////////////////////////////////////
/// \brief Apply a conditional cell colour scheme
/// to the selected cells.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::applyConditionalCellColourScheme()
{
    wxGridCellCoordsContainer coordsContainer;

    // Handle all possible selection types
    if (GetSelectedCells().size()) // not a block layout
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCells(), this);
    else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size()) // block layout
        coordsContainer = wxGridCellCoordsContainer(GetSelectionBlockTopLeft()[0], GetSelectionBlockBottomRight()[0], this);
    else if (GetSelectedCols().size()) // multiple selected columns
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCols(), GetRows()-1, false, this);
    else if (GetSelectedRows().size()) // multiple selected rows
        coordsContainer = wxGridCellCoordsContainer(GetSelectedRows(), GetCols()-1, true, this);
    else
        coordsContainer = wxGridCellCoordsContainer(wxGridCellCoords(0, 0), wxGridCellCoords(GetRows()-1, GetCols()-1), this);

    double minVal = calculateMin(coordsContainer);
    double maxVal = calculateMax(coordsContainer);

    CellValueShaderDialog dialog(this, minVal, maxVal);

    if (dialog.ShowModal() == wxID_OK)
        conditionalFormat(coordsContainer, dialog.getShader());
}


/////////////////////////////////////////////////
/// \brief Create a filter for a column and apply
/// all filters automatically.
///
/// \param col int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::createFilter(int col)
{
    if (col >= GetNumberCols()-1)
        return;

    // Ensure that we have enough filters available
    if ((int)m_filter.size() <= col)
        m_filter.resize(col+1);

    // Open up the corresponding dialog
    CellFilterDialog dialog(this, m_filter[col]);

    if (dialog.ShowModal() != wxID_OK)
        return;

    m_filter[col] = dialog.getCondition();

    applyFilter();
}


/////////////////////////////////////////////////
/// \brief Delete the filter for a column and
/// re-apply the remaining ones.
///
/// \param col int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::deleteFilter(int col)
{
    if (col >= GetNumberCols()-1 || (size_t)col >= m_filter.size())
        return;

    // Delete the filter by resetting it
    m_filter[col].reset();

    applyFilter();
}


/////////////////////////////////////////////////
/// \brief Apply all filters to the table data.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::applyFilter()
{
    BeginBatch();

    // Mark the filtered columns
    for (int j = 0; j < GetNumberCols(); j++)
    {
        wxString label = GetColLabelValue(j);

        if ((size_t)j < m_filter.size() && m_filter[j].m_type != CellFilterCondition::CT_NONE)
            SetColLabelValue(j, label[label.length()-1] != L'\uE16E' ? label + L" \uE16E" : label);
        else
            SetColLabelValue(j, label[label.length()-1] != L'\uE16E' ? label : label.substr(0, label.length()-2));
    }

    // Apply the filter to all rows
    for (int i = nFirstNumRow; i < GetNumberRows()-1; i++)
    {
        bool show = true;

        for (size_t j = 0; j < std::min(m_filter.size(), (size_t)GetNumberCols()-1); j++)
        {
            if (m_filter[j].m_type != CellFilterCondition::CT_NONE)
            {
                mu::Value v = get(i, j);

                if ((v.isString() && !m_filter[j].eval(v.getStr()).first)
                    || (!v.isString() && !m_filter[j].eval(v.getNum().asCF64()).first))
                {
                    show = false;
                    break;
                }
            }
        }

        if (show)
            ShowRow(i);
        else
            HideRow(i);
    }

    EndBatch();
}


/////////////////////////////////////////////////
/// \brief Sort according a specified column.
///
/// \param col int
/// \param ascending bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::sortCol(int col, bool ascending)
{
    // A \u2BC5
    // V \u2BC6
    if (isGridNumeReTable && col < GetNumberCols()-1)
    {
        static_cast<GridNumeReTable*>(GetTable())->sortCol(col, ascending);
        Refresh();

        for (int j = 0; j < GetNumberCols(); j++)
        {
            wxString label = GetColLabelValue(j);

            if (label[0] == L'\u2BC5' || label[0] == L'\u2BC6')
                SetColLabelValue(j, label.substr(2));
        }

        SetColLabelValue(col, (ascending ? L"\u2BC5 " : L"\u2BC6 ") + GetColLabelValue(col));

        applyFilter();
    }
}


/////////////////////////////////////////////////
/// \brief Clear any sorting applied to any
/// column.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::clearSort()
{
    if (isGridNumeReTable)
    {
        static_cast<GridNumeReTable*>(GetTable())->removeSort();
        Refresh();

        for (int j = 0; j < GetNumberCols(); j++)
        {
            wxString label = GetColLabelValue(j);

            if (label[0] == L'\u2BC5' || label[0] == L'\u2BC6')
                SetColLabelValue(j, label.substr(2));
        }

        applyFilter();
    }
}


/////////////////////////////////////////////////
/// \brief Conditional format the respective
/// cells.
///
/// \param cells const wxGridCellCoordsContainer&
/// \param shader const CellValueShader&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::conditionalFormat(const wxGridCellCoordsContainer& cells, const CellValueShader& shader)
{
    // Get the extent of the container
    const wxGridCellsExtent& cellsExtent = cells.getExtent();

    // Whole columns are selected
    if ((cells.isBlock() || cells.columnsSelected())
        && allRowsShown()
        && cellsExtent.m_topleft.GetRow() <= (int)nFirstNumRow
        && cellsExtent.m_bottomright.GetRow()+2 >= GetRows())
    {
        for (int j = cellsExtent.m_topleft.GetCol(); j <= cellsExtent.m_bottomright.GetCol(); j++)
        {
            if (!cells.contains(cellsExtent.m_topleft.GetRow(), j))
                continue;

            int h_align, v_align;
            GetCellAlignment(nFirstNumRow, j, &h_align, &v_align);

            wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                      h_align, wxALIGN_CENTER);

            if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                attr->SetRenderer(new AdvBooleanCellRenderer(shader));
            else
                attr->SetRenderer(new AdvStringCellRenderer(shader));

            SetColAttr(j, attr, true);
        }
    }
    else
    {
        for (int i = cellsExtent.m_topleft.GetRow(); i <= cellsExtent.m_bottomright.GetRow(); i++)
        {
            for (int j = cellsExtent.m_topleft.GetCol(); j <= cellsExtent.m_bottomright.GetCol(); j++)
            {
                if (!cells.contains(i, j))
                    continue;

                if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                    SetCellRenderer(i, j, new AdvBooleanCellRenderer(shader));
                else
                    SetCellRenderer(i, j, new AdvStringCellRenderer(shader));
            }
        }
    }

    if (readOnly)
        groupHeaders(0, GetNumberCols(), 0);

    // Refresh the window to redraw all cells
    Refresh();
}


/////////////////////////////////////////////////
/// \brief Changes the alignment of a whole
/// column to reflect its internal type.
///
/// \param col int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::UpdateColumnAlignment(int col)
{
    std::vector<int> vTypes;

    if (col+1 == GetNumberCols())
        return;

    if (isGridNumeReTable)
    {
        GridNumeReTable* gridtab = static_cast<GridNumeReTable*>(GetTable());
        vTypes = gridtab->getColumnTypes();

        if (col < (int)vTypes.size())
        {
            if (col >= (int)m_currentColTypes.size() || m_currentColTypes[col] != vTypes[col])
            {
                if (vTypes[col] == TableColumn::TYPE_STRING)
                {
                    wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                              wxALIGN_LEFT, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                    SetColAttr(col, attr);
                }
                else if (vTypes[col] == TableColumn::TYPE_CATEGORICAL)
                {
                    wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                              wxALIGN_CENTER, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                    SetColAttr(col, attr);
                }
                else if (vTypes[col] == TableColumn::TYPE_LOGICAL)
                {
                    wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                              wxALIGN_CENTER, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvBooleanCellRenderer);
                    SetColAttr(col, attr);
                }
                else // All other cases
                {
                    wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                              wxALIGN_RIGHT, wxALIGN_CENTER);
                    attr->SetRenderer(new AdvStringCellRenderer);
                    SetColAttr(col, attr);
                }
            }
        }

        m_currentColTypes = vTypes;
    }
    else
    {
        // Search the boundaries and color the frame correspondingly
        for (int i = (int)nFirstNumRow; i < GetNumberRows(); i++)
        {
            if (GetCellValue(i, col)[0] == '"')
                SetCellAlignment(wxALIGN_LEFT, i, col);
            else
                SetCellAlignment(wxALIGN_RIGHT, i, col);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// needed space in the grid, which is needed to
/// paste data.
///
/// \param rows int
/// \param headrows int
/// \param cols int
/// \param useCursor bool
/// \return wxGridCellCoords
///
/////////////////////////////////////////////////
wxGridCellCoords TableViewer::CreateEmptyGridSpace(int rows, int headrows, int cols, bool useCursor)
{
    wxGridCellCoords topLeft(headrows,0);

    if (useCursor)
    {
        // We use the cursor in this case (this
        // implies overriding existent data)
        topLeft = m_lastRightClick;

        while (this->GetRowLabelValue(topLeft.GetRow()) == "#")
            topLeft.SetRow(topLeft.GetRow()+1);

        if (this->GetCols()-1 < topLeft.GetCol() + cols)
            this->AppendCols(topLeft.GetCol() + cols - this->GetCols()+1, true);

        if (this->GetRows()-1 < topLeft.GetRow() + rows)
            this->AppendRows(topLeft.GetRow() + rows - this->GetRows()+1, true);
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


/////////////////////////////////////////////////
/// \brief Return the cell value as value_type.
///
/// \param row int
/// \param col int
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> TableViewer::CellToCmplx(int row, int col)
{
    if (isGridNumeReTable)
        return *static_cast<std::complex<double>*>(GetTable()->GetValueAsCustom(row, col, "complex"));

    std::string cellValue = GetCellValue(row, col).ToStdString();

    if (cellValue.front() != '"')
    {
        if (isConvertible(cellValue, CONVTYPE_VALUE))
            return StrToCmplx(cellValue);

        if (isConvertible(cellValue, CONVTYPE_LOGICAL))
            return StrToLogical(cellValue);

        if (isConvertible(cellValue, CONVTYPE_DATE_TIME))
            return to_double(StrToTime(cellValue));
    }

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Return the cell value as a mu::Value
///
/// \param row int
/// \param col int
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value TableViewer::get(int row, int col)
{
    if (isGridNumeReTable)
        return *static_cast<mu::Value*>(GetTable()->GetValueAsCustom(row, col, "mu::Value"));

    std::string cellValue = GetCellValue(row, col).ToStdString();

    if (cellValue.front() != '"')
    {
        if (isConvertible(cellValue, CONVTYPE_VALUE))
            return mu::Value(StrToCmplx(cellValue));

        if (isConvertible(cellValue, CONVTYPE_LOGICAL))
            return mu::Value(StrToLogical(cellValue));

        if (isConvertible(cellValue, CONVTYPE_DATE_TIME))
            return mu::Value(StrToTime(cellValue));
    }

    return mu::Value(toInternalString(cellValue));;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// minimal value of the selected cells.
///
/// \param coords const wxGridCellCoordsContainer&
/// \return double
///
/////////////////////////////////////////////////
double TableViewer::calculateMin(const wxGridCellCoordsContainer& coords)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->min(coords);

    const wxGridCellsExtent& cellExtent = coords.getExtent();
    double dMin = NAN;

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            if (isnan(dMin) || CellToCmplx(i, j).real() < dMin)
                dMin = CellToCmplx(i, j).real();
        }
    }

    return dMin;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// maximal value of the selected cells.
///
/// \param coords const wxGridCellCoordsContainer&
/// \return double
///
/////////////////////////////////////////////////
double TableViewer::calculateMax(const wxGridCellCoordsContainer& coords)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->max(coords);

    const wxGridCellsExtent& cellExtent = coords.getExtent();
    double dMax = NAN;

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            if (isnan(dMax) || CellToCmplx(i, j).real() > dMax)
                dMax = CellToCmplx(i,j).real();
        }
    }

    return dMax;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// sum of the selected cells.
///
/// \param coords const wxGridCellCoordsContainer&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> TableViewer::calculateSum(const wxGridCellCoordsContainer& coords)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->sum(coords);

    const wxGridCellsExtent& cellExtent = coords.getExtent();
    std::complex<double> dSum = 0;

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            if (!mu::isnan(CellToCmplx(i, j)))
                dSum += CellToCmplx(i,j);
        }
    }

    return dSum;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// average of the selected cells.
///
/// \param coords const wxGridCellCoordsContainer&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> TableViewer::calculateAvg(const wxGridCellCoordsContainer& coords)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->avg(coords);

    const wxGridCellsExtent& cellExtent = coords.getExtent();
    std::complex<double> dSum = 0;
    int nCount = 0;

    for (int i = cellExtent.m_topleft.GetRow(); i <= cellExtent.m_bottomright.GetRow(); i++)
    {
        for (int j = cellExtent.m_topleft.GetCol(); j <= cellExtent.m_bottomright.GetCol(); j++)
        {
            if (!coords.contains(i, j))
                continue;

            if (!mu::isnan(CellToCmplx(i, j)))
            {
                nCount++;
                dSum += CellToCmplx(i, j);
            }
        }
    }

    if (nCount)
        return dSum / (double)nCount;

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This member function updates the
/// status bar of the enclosing ViewerFrame.
///
/// \param topLeft const wxGridCellCoords&
/// \param bottomRight const wxGridCellCoords&
/// \param cursor wxGridCellCoords*
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::updateStatusBar(const wxGridCellCoordsContainer& coords, wxGridCellCoords* cursor /*= nullptr*/)
{
    if (!m_statusBar)
        return;

    // Get the dimensions
    wxString dim = "Dim: ";
    dim << GetRows()-nFirstNumRow-1 << "x" << GetCols()-1;

    // Get the current cursor position
    wxString sel = "Cur: ";
    if (cursor)
        sel << this->GetRowLabelValue(cursor->GetRow()) << "," << cursor->GetCol()+1;
    else
        sel << "--,--";

    // Calculate the simple statistics
    bool isDateTime = false;
    wxString colTypes;
    wxGridCellsExtent selectedExtent = coords.getExtent();

    if (isGridNumeReTable)
    {
        std::vector<int> vTypes = static_cast<GridNumeReTable*>(GetTable())->getColumnTypes();
        std::set<int> types;

        for (int col = selectedExtent.m_topleft.GetCol(); col <= selectedExtent.m_bottomright.GetCol(); col++)
        {
            if (vTypes.size() > (size_t)col)
                types.insert(vTypes[col]);
        }

        if (types.size() == 1u && *types.begin() == TableColumn::TYPE_DATETIME)
            isDateTime = true;

        for (int type : types)
        {
            if (colTypes.length())
                colTypes += ", ";

            colTypes += TableColumn::typeToString(TableColumn::ColumnType(type));
        }
    }

    wxString statustext;

    double dMin = NAN;
    double dMax = NAN;
    std::complex<double> dSum = NAN;
    std::complex<double> dAvg = NAN;

    if ((selectedExtent.m_bottomright.GetCol() - selectedExtent.m_topleft.GetCol() + 1)
        * (selectedExtent.m_bottomright.GetRow() - selectedExtent.m_topleft.GetRow() + 1) < 2000000)
    {
        dMin = calculateMin(coords);
        dMax = calculateMax(coords);
        dSum = calculateSum(coords);
        dAvg = calculateAvg(coords);
    }

    if (isDateTime && !isnan(dMin))
    {
        statustext = "[ " + toString(to_timePoint(dMin), GET_SHORTEST);
        statustext << " : " << toString(to_timePoint(dMax), GET_SHORTEST);
        statustext << L" ]  |  \u0394t " << formatDuration(dMax-dMin);
        statustext << L"  |  \u03A3t " << formatDuration(dSum.real());
        statustext << L"  |  t\u0305 " << toString(to_timePoint(dAvg.real()), GET_SHORTEST);
    }
    else
    {
        statustext = "[ " + toString(dMin, STATUSBAR_PRECISION);
        statustext << " : " << toString(dMax, STATUSBAR_PRECISION);
        statustext << L" ]  |  \u0394x " << toString(dMax-dMin, STATUSBAR_PRECISION);
        statustext << L"  |  \u03A3x " << toString(dSum, 2*STATUSBAR_PRECISION);
        statustext << L"  |  x\u0305 " << toString(dAvg, 2*STATUSBAR_PRECISION);
    }

    if (colTypes.length())
        statustext << "  |  Type: " << colTypes;

    if (coords.size() == 1)
        statustext << "  |  Value: " << GetCellValue(selectedExtent.m_topleft);
    else
        statustext << "  |  Values: {" << GetCellValue(selectedExtent.m_topleft) << ", ..., "
                   << GetCellValue(selectedExtent.m_bottomright) << "}";

    // Set the status bar valuey
    m_statusBar->SetStatusText(dim);
    m_statusBar->SetStatusText(sel, 1);
    m_statusBar->SetStatusText(statustext, 2);
}


/////////////////////////////////////////////////
/// \brief Creates a menu bar in the top frame.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::createMenuBar()
{
    if (!m_parentPanel)
        return;

    wxMenuBar* menuBar = m_parentPanel->getMenuBar();

    // Create the file menu
    wxMenu* menuFile = new wxMenu();

    menuFile->Append(ID_MENU_SAVE, _guilang.get("GUI_MENU_SAVEFILE"));
    menuFile->Append(ID_MENU_SAVE_AS, _guilang.get("GUI_MENU_SAVEFILEAS"));

    menuBar->Append(menuFile, _guilang.get("GUI_MENU_FILE"));

    // Create the edit menu
    wxMenu* menuEdit = new wxMenu();

    menuEdit->Append(ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS") + "\tCtrl-C");
    menuEdit->Append(ID_MENU_CUT, _guilang.get("GUI_CUT_TABLE_CONTENTS") + "\tCtrl-X");
    menuEdit->Append(ID_MENU_PASTE, _guilang.get("GUI_PASTE_TABLE_CONTENTS") + "\tCtrl-V");
    menuEdit->Append(ID_MENU_PASTE_HERE, _guilang.get("GUI_PASTE_TABLE_CONTENTS_HERE") + "\tCtrl-Shift-V");
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_MENU_INSERT_ROW, _guilang.get("GUI_INSERT_TABLE_ROW"));
    menuEdit->Append(ID_MENU_INSERT_COL, _guilang.get("GUI_INSERT_TABLE_COL"));
    menuEdit->Append(ID_MENU_INSERT_CELL, _guilang.get("GUI_INSERT_TABLE_CELL"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_MENU_REMOVE_ROW, _guilang.get("GUI_REMOVE_TABLE_ROW"));
    menuEdit->Append(ID_MENU_REMOVE_COL, _guilang.get("GUI_REMOVE_TABLE_COL"));
    menuEdit->Append(ID_MENU_REMOVE_CELL, _guilang.get("GUI_REMOVE_TABLE_CELL"));

    menuBar->Append(menuEdit, _guilang.get("GUI_MENU_EDIT"));

    // Create the columns submenu
    wxMenu* columnMenu = new wxMenu();

    columnMenu->Append(ID_MENU_SORT_COL_ASC, _guilang.get("GUI_TABLE_SORT_ASC"));
    columnMenu->Append(ID_MENU_SORT_COL_DESC, _guilang.get("GUI_TABLE_SORT_DESC"));
    columnMenu->Append(ID_MENU_SORT_COL_CLEAR, _guilang.get("GUI_TABLE_SORT_CLEAR"));
    columnMenu->AppendSeparator();
    columnMenu->Append(ID_MENU_FILTER, _guilang.get("GUI_TABLE_FILTER"));
    columnMenu->Append(ID_MENU_DELETE_FILTER, _guilang.get("GUI_TABLE_DELETE_FILTER"));
    columnMenu->AppendSeparator();
    columnMenu->Append(ID_MENU_CHANGE_COL_TYPE, _guilang.get("GUI_TABLE_CHANGE_COL_TYPE") + "\tCtrl-T");

    // Create the tools menu
    wxMenu* menuTools = new wxMenu();

    menuTools->Append(ID_MENU_RELOAD, _guilang.get("GUI_TABLE_RELOAD") + "\tCtrl-R");
    menuTools->Append(wxID_ANY, _guilang.get("GUI_TABLE_COLUMN"), columnMenu);
    menuTools->Append(ID_MENU_CVS, _guilang.get("GUI_TABLE_CVS") + "\tCtrl-Shift-F");

    menuBar->Append(menuTools, _guilang.get("GUI_MENU_TOOLS"));

    wxFrame* parFrame = m_parentPanel->getFrame();
    parFrame->Bind(wxEVT_MENU, &TableViewer::OnMenu, this);
}


/////////////////////////////////////////////////
/// \brief This member function replaces the
/// german comma decimal sign with the dot used
/// in anglo-american notation.
///
/// \param text wxString&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::replaceDecimalSign(wxString& text)
{
    wxArrayString toks = wxStringTokenize(text);
    text.clear();

    for (size_t i = 0; i < toks.size(); i++)
    {
        if (isNumerical(toks[i].ToStdString()))
            toks[i].Replace(",", ".");

        if (text.length())
            text += "  ";

        text += toks[i];
    }
}


/////////////////////////////////////////////////
/// \brief This member function replaces the
/// tabulator character with whitespaces.
///
/// \param text wxString&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::replaceTabSign(wxString& text)
{
    if (!text.length())
        return;

    bool bKeepColumns = false;

    // Determine, if columns shall be kept
    if (text.find(' ') == std::string::npos)
    {
        bKeepColumns = true;

        if (text[0] == '\t')
            text.insert(0, "---");
    }

    size_t pos = 0;

    // Replace tabulators with whitespace and
    // an empty cell, if necessary
    while ((pos = text.find('\t')) != std::string::npos)
    {
        text[pos] = ' ';

        if (bKeepColumns && pos+1 < text.length() && text[pos+1] == '\t')
            text.insert(pos + 1, "---");
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates a zero-
/// element table to visualize empty tables (or
/// non-existent ones).
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::createZeroElementTable()
{
    SetTable(new GridNumeReTable(NumeRe::Table(1, 1),
                                 NumeReKernel::getInstance()->getSettings().getSetting(SETTING_B_SHOWQMARKS).active()), true);

    isGridNumeReTable = true;
    nFirstNumRow = 1;

    layoutGrid();

    updateStatusBar(wxGridCellCoordsContainer(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-2, this->GetCols()-2)));
}


/////////////////////////////////////////////////
/// \brief This member function transformes the
/// data obtained by the clipboard into a table-
/// like layout.
///
/// \param data const wxString&
/// \return std::vector<wxString>
///
/////////////////////////////////////////////////
std::vector<wxString> TableViewer::getLinesFromPaste(const wxString& data)
{
    std::vector<wxString> vPaste;
    wxString sClipboard = data;
    wxString sLine;

    // Endless loop
    while (true)
    {
        bool tabSeparated = false;
        // Abort, if the clipboards contents indicate an
        // empty table
        if (!sClipboard.length() || sClipboard == "\n")
            break;

        // Get the next line
        sLine = sClipboard.substr(0, sClipboard.find('\n'));

        // Remove the carriage return character
        if (sLine.length() && sLine[sLine.length()-1] == (char)13)
            sLine.erase(sLine.length()-1);

        // determine the separation mode
        if (sLine.find('\t') != std::string::npos)
            tabSeparated = true;

        // Remove the obtained line from the clipboard
        if (sClipboard.find('\n') != std::string::npos)
            sClipboard.erase(0, sClipboard.find('\n')+1);
        else
            sClipboard.clear();

        // Replace whitespaces with underscores, if the current
        // line also contains tabulator characters and it is a
        // non-numerical line
        if (!isNumerical(sLine.ToStdString())
            && (tabSeparated || sLine.find("  ") != std::string::npos))
        {
            for (size_t i = 1; i < sLine.length()-1; i++)
            {
                if (sLine[i] == ' ' && sLine[i-1] != ' ' && sLine[i+1] != ' ')
                    sLine[i] = '\1';
            }
        }

        // Now replace the tabulator characters with
        // whitespaces
        replaceTabSign(sLine);

        // Ignore empty lines
        if (sLine.find_first_not_of(' ') == std::string::npos)
            continue;

        // Replace the decimal sign, if the line is numerical,
        // otherwise try to detect, whether the comma is used
        // to separate the columns
        if (sLine.find(',') != std::string::npos && (sLine.find('.') == std::string::npos || tabSeparated))
            replaceDecimalSign(sLine);
        else if (!tabSeparated && sLine.find(',') != std::string::npos && sLine.find(';') != std::string::npos)
        {
            // The semicolon is used to separate the columns
            // in this case
            for (size_t i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                    sLine[i] = '.';

                if (sLine[i] == ';')
                {
                    sLine[i] = ' ';
                }
            }
        }
        else if (!tabSeparated)
        {
            // The comma is used to separate the columns
            // in this case
            for (size_t i = 0; i < sLine.length(); i++)
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


/////////////////////////////////////////////////
/// \brief This member function is the data
/// setter for string and cluster tables.
///
/// \param _stringTable NumeRe::Container<std::string>&
/// \param sName const std::string&
/// \param sIntName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetData(NumeRe::Container<std::string>& _stringTable, const std::string& sName, const std::string& sIntName)
{
    m_displayName = sName;
    m_intName = sIntName;

    if (!_stringTable.getCols() || !_stringTable.getRows())
    {
        createZeroElementTable();
        return;
    }

    CreateGrid(_stringTable.getRows()+1, _stringTable.getCols()+1);

    // String tables and clusters do not have a headline
    nFirstNumRow = 0;
    isGridNumeReTable = false;

    for (size_t i = 0; i < _stringTable.getRows()+1; i++)
    {
        SetRowLabelValue(i, GetRowLabelValue(i));

        for (size_t j = 0; j < _stringTable.getCols()+1; j++)
        {
            if (i == _stringTable.getRows() || j == _stringTable.getCols())
            {
                SetColLabelValue(j, toString(j+1));
                continue;
            }

            if (_stringTable.get(i, j).length())
                SetCellValue(i, j, replaceControlCharacters(_stringTable.get(i, j)));
        }
    }

    if (m_parentPanel)
    {
        wxMenuBar* menuBar = m_parentPanel->getMenuBar();

        wxMenu* toolsMenu = menuBar->GetMenu(menuBar->FindMenu(_guilang.get("GUI_MENU_TOOLS")));

        if (toolsMenu)
            toolsMenu->Enable(ID_MENU_CHANGE_COL_TYPE, false);
    }

    layoutGrid();

    updateStatusBar(wxGridCellCoordsContainer(wxGridCellCoords(0,0), wxGridCellCoords(GetRows()-2, GetCols()-2)));
}


/////////////////////////////////////////////////
/// \brief This member function is the data
/// setter function. It will create an internal
/// GridNumeReTable object, which will provide
/// the data for the grid.
///
/// \param _table NumeRe::Table&
/// \param sName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetData(NumeRe::Table& _table, const std::string& sName, const std::string& sIntName)
{
    if (m_parentPanel)
        m_parentPanel->update(_table.getMetaData());

    m_displayName = sName;
    m_intName = sIntName;

    // Create an empty table, if necessary
    if (_table.isEmpty() || !_table.getLines())
    {
        createZeroElementTable();
        return;
    }

    // Store the number headlines and create the data
    // providing object
    nFirstNumRow = _table.getHeadCount();
    SetTable(new GridNumeReTable(std::move(_table),
                                 NumeReKernel::getInstance()->getSettings().getSetting(SETTING_B_SHOWQMARKS).active()), true);
    isGridNumeReTable = true;
    layoutGrid();

    updateStatusBar(wxGridCellCoordsContainer(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-2, this->GetCols()-2)));
}


/////////////////////////////////////////////////
/// \brief This member function declares the
/// table to be read-only and enables the context
/// menu entries, if the table is set to be not
/// read-only.
///
/// \param isReadOnly bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetTableReadOnly(bool isReadOnly)
{
    readOnly = isReadOnly;
    EnableEditing(!isReadOnly);

    if (!readOnly)
    {
        if (m_popUpMenu.FindItem(ID_MENU_CUT))
        {
            for (int i = 0; i < (int)nFirstNumRow; i++)
            {
                for (int j = 0; j < GetNumberCols(); j++)
                {
                    SetCellSize(i, j, 1, 1);
                }
            }
            m_popUpMenu.Enable(ID_MENU_CUT, true);
            m_popUpMenu.Enable(ID_MENU_PASTE, true);
            m_popUpMenu.Enable(ID_MENU_PASTE_HERE, true);
            m_popUpMenu.Enable(ID_MENU_INSERT_ROW, true);
            m_popUpMenu.Enable(ID_MENU_INSERT_COL, true);
            m_popUpMenu.Enable(ID_MENU_INSERT_CELL, true);
            m_popUpMenu.Enable(ID_MENU_REMOVE_ROW, true);
            m_popUpMenu.Enable(ID_MENU_REMOVE_COL, true);
            m_popUpMenu.Enable(ID_MENU_REMOVE_CELL, true);
        }
        else
        {
            m_popUpMenu.Append(ID_MENU_CUT, _guilang.get("GUI_CUT_TABLE_CONTENTS"));
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

        if (m_parentPanel)
        {
            wxMenuBar* menuBar = m_parentPanel->getMenuBar();

            wxMenu* toolsMenu = menuBar->GetMenu(menuBar->FindMenu(_guilang.get("GUI_MENU_TOOLS")));

            if (toolsMenu)
                toolsMenu->Enable(ID_MENU_RELOAD, false);
        }
    }
    else
    {
        if (m_popUpMenu.FindItem(ID_MENU_CUT))
        {
            groupHeaders(0, GetNumberCols(), 0);
            m_popUpMenu.Enable(ID_MENU_CUT, false);
            m_popUpMenu.Enable(ID_MENU_PASTE, false);
            m_popUpMenu.Enable(ID_MENU_PASTE_HERE, false);
            m_popUpMenu.Enable(ID_MENU_INSERT_ROW, false);
            m_popUpMenu.Enable(ID_MENU_INSERT_COL, false);
            m_popUpMenu.Enable(ID_MENU_INSERT_CELL, false);
            m_popUpMenu.Enable(ID_MENU_REMOVE_ROW, false);
            m_popUpMenu.Enable(ID_MENU_REMOVE_COL, false);
            m_popUpMenu.Enable(ID_MENU_REMOVE_CELL, false);
        }

        if (m_parentPanel)
        {
            wxMenuBar* menuBar = m_parentPanel->getMenuBar();

            wxMenu* editMenu = menuBar->GetMenu(menuBar->FindMenu(_guilang.get("GUI_MENU_EDIT")));

            if (editMenu)
            {
                editMenu->Enable(ID_MENU_CUT, false);
                editMenu->Enable(ID_MENU_PASTE, false);
                editMenu->Enable(ID_MENU_PASTE_HERE, false);
                editMenu->Enable(ID_MENU_INSERT_ROW, false);
                editMenu->Enable(ID_MENU_INSERT_COL, false);
                editMenu->Enable(ID_MENU_INSERT_CELL, false);
                editMenu->Enable(ID_MENU_REMOVE_ROW, false);
                editMenu->Enable(ID_MENU_REMOVE_COL, false);
                editMenu->Enable(ID_MENU_REMOVE_CELL, false);
            }
        }
    }

    Refresh();
}


/////////////////////////////////////////////////
/// \brief Set the grid cursor while storint the
/// state that the cursor move was intended to be
/// silent.
///
/// \param row int
/// \param col int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetGridCursorSilent(int row, int col)
{
    isSilentCursorMove = true;
    SetGridCursor(row, col);
}


/////////////////////////////////////////////////
/// \brief This member function creates an empty
/// table of some size.
///
/// \param rows size_t
/// \param cols size_t
/// \return void
/// \deprecated It's declared as deprecated
///
/////////////////////////////////////////////////
void TableViewer::SetDefaultSize(size_t rows, size_t cols)
{
    CreateGrid(rows+1,cols+1);

    for (size_t i = 0; i < rows+1; i++)
    {
        SetRowLabelValue(i, GetRowLabelValue(i));

        for (size_t j = 0; j < cols+1; j++)
        {
            if (i < 1)
            {
                SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTRE);
                SetCellFont(i, j, this->GetCellFont(i, j).MakeBold());
                SetCellBackgroundColour(i, j, *wxLIGHT_GREY);
            }
            else if (i == rows || j == cols)
            {
                SetColLabelValue(j, GetColLabelValue(j));
                SetCellBackgroundColour(i, j, wxColor(230,230,230));
                SetReadOnly(i, j, readOnly);
                continue;
            }
            else
                SetCellAlignment(i, j, wxALIGN_RIGHT, wxALIGN_CENTER);

            SetReadOnly(i, j, readOnly);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// context menu for cells.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnCellRightClick(wxGridEvent& event)
{
    m_lastRightClick.Set(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        for (int i = ID_MENU_INSERT_ROW; i <= ID_MENU_REMOVE_CELL; i++)
            m_popUpMenu.Enable(i,true);

        m_popUpMenu.Enable(ID_MENU_PASTE_HERE, true);
    }

    m_popUpMenu.Enable(ID_MENU_COLUMNS, true);
    m_popUpMenu.Enable(ID_MENU_DELETE_FILTER,
                       event.GetCol() < (int)m_filter.size() && m_filter[event.GetCol()].m_type != CellFilterCondition::CT_NONE);

    m_popUpMenu.Enable(ID_MENU_SORT_COL_ASC, isGridNumeReTable);
    m_popUpMenu.Enable(ID_MENU_SORT_COL_DESC, isGridNumeReTable);
    m_popUpMenu.Enable(ID_MENU_SORT_COL_CLEAR, isGridNumeReTable && static_cast<GridNumeReTable*>(GetTable())->isSorted());

    int id = GetPopupMenuSelectionFromUser(m_popUpMenu, event.GetPosition());

    if (id == wxID_NONE)
        m_lastRightClick.Set(-1, -1);
    else
    {
        wxCommandEvent evt(wxEVT_MENU, id);
        GetEventHandler()->ProcessEvent(evt);
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for cell double clicks.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnCellDoubleClick(wxGridEvent& event)
{
    if (isGridNumeReTable || !m_numereWindow)
    {
        event.Skip();
        return;
    }

    wxString cellValue = GetCellValue(event.GetRow(), event.GetCol());

    if (cellValue.length() && cellValue.Matches("{* x * *}"))
    {
        m_numereWindow->showTable(m_intName + ".sel(" + toString(event.GetRow()+1) + ")",
                                  m_displayName + ".sel(" + toString(event.GetRow()+1) + ")");
    }

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// context menu for labels.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnLabelRightClick(wxGridEvent& event)
{
    m_lastRightClick.Set(event.GetRow(), event.GetCol());

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

    if (event.GetCol() != -1)
    {
        m_popUpMenu.Enable(ID_MENU_COLUMNS, true);
        m_popUpMenu.Enable(ID_MENU_DELETE_FILTER,
                           event.GetCol() < (int)m_filter.size() && m_filter[event.GetCol()].m_type != CellFilterCondition::CT_NONE);

        m_popUpMenu.Enable(ID_MENU_SORT_COL_ASC, isGridNumeReTable);
        m_popUpMenu.Enable(ID_MENU_SORT_COL_DESC, isGridNumeReTable);
        m_popUpMenu.Enable(ID_MENU_SORT_COL_CLEAR, isGridNumeReTable && static_cast<GridNumeReTable*>(GetTable())->isSorted());
    }
    else
        m_popUpMenu.Enable(ID_MENU_COLUMNS, false);

    int id = GetPopupMenuSelectionFromUser(m_popUpMenu, event.GetPosition());

    if (id == wxID_NONE)
        m_lastRightClick.Set(-1, -1);
    else
    {
        wxCommandEvent evt(wxEVT_MENU, id);
        GetEventHandler()->ProcessEvent(evt);
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the menu
/// command event handler function. It will
/// redirect the control to the specified
/// functions.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::OnMenu(wxCommandEvent& event)
{
    if (m_lastRightClick.GetRow() == -1 && m_lastRightClick.GetCol() == -1)
        m_lastRightClick.Set(GetCursorRow(), GetCursorColumn());

    switch (event.GetId())
    {
        case ID_MENU_SAVE:
            saveTable();
            break;
        case ID_MENU_SAVE_AS:
            saveTable(true);
            break;
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
        case ID_MENU_CUT:
            copyContents();

            if (!readOnly)
                deleteSelection();

            break;
        case ID_MENU_PASTE:
            pasteContents();
            break;
        case ID_MENU_PASTE_HERE:
            pasteContents(true);
            break;
        case ID_MENU_RELOAD:
            reloadTable();
            break;
        case ID_MENU_CHANGE_COL_TYPE:
            changeColType();
            break;
        case ID_MENU_CVS:
            applyConditionalCellColourScheme();
            break;
        case ID_MENU_FILTER:
            createFilter(m_lastRightClick.GetCol());
            break;
        case ID_MENU_DELETE_FILTER:
            deleteFilter(m_lastRightClick.GetCol());
            break;
        case ID_MENU_SORT_COL_ASC:
            sortCol(m_lastRightClick.GetCol(), true);
            break;
        case ID_MENU_SORT_COL_DESC:
            sortCol(m_lastRightClick.GetCol(), false);
            break;
        case ID_MENU_SORT_COL_CLEAR:
            clearSort();
            break;
    }

    m_lastRightClick.Set(-1, -1);
}


/////////////////////////////////////////////////
/// \brief This member function processes the
/// insertion of new empty cells, columns or rows.
///
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::insertElement(int id)
{
    if (id == ID_MENU_INSERT_ROW)
    {
        // New row
        if (m_lastRightClick.GetRow() < (int)nFirstNumRow)
            return; // disabled for now //nFirstNumRow++;

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


/////////////////////////////////////////////////
/// \brief This member function processes the
/// removing of cells, columns or rows.
///
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::removeElement(int id)
{
    if (id == ID_MENU_REMOVE_ROW)
    {
        // Remove row
        if (m_lastRightClick.GetRow() < (int)nFirstNumRow)
            return; // disabled for now //nFirstNumRow--;

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


/////////////////////////////////////////////////
/// \brief Saves the currently displayed table
/// directly to the selected file.
///
/// \param saveAs bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::saveTable(bool saveAs)
{
    if (!isGridNumeReTable || !m_parentPanel)
        return;

    NumeReTerminal* term = m_parentPanel->GetTerminal();
    std::vector<std::string> vPaths = term->getPathSettings();
    std::string filename = m_displayName.substr(0, m_displayName.find_first_of("({")) + ".ndat";

    // Get the filename from the user
    if (saveAs || !filename.length())
    {
        wxFileDialog fd(this, _guilang.get("GUI_VARVIEWER_SAVENAME"), vPaths[SAVEPATH], filename,
                        _guilang.get("COMMON_FILETYPE_NDAT") + " (*.ndat)|*.ndat|"
                        + _guilang.get("COMMON_FILETYPE_DAT") + " (*.dat)|*.dat|"
                        + _guilang.get("COMMON_FILETYPE_TXT") + " (*.txt)|*.txt|"
                        + _guilang.get("COMMON_FILETYPE_CSV") + " (*.csv)|*.csv|"
                        + _guilang.get("COMMON_FILETYPE_XLS") + " (*.xls)|*.xls|"
                        + _guilang.get("COMMON_FILETYPE_TEX") + " (*.tex)|*.tex|"
                        + _guilang.get("COMMON_FILETYPE_MD") + " (*.md)|*.md|"
                        + _guilang.get("COMMON_FILETYPE_HTML") + " (*.html,*.htm)|*.html;*.htm",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (fd.ShowModal() == wxID_CANCEL)
            return;

        filename = fd.GetPath().ToStdString();
    }
    else
        filename = vPaths[SAVEPATH] + "/" + filename;

    // Try to open the file type
    std::unique_ptr<NumeRe::GenericFile> file(NumeRe::getFileByType(filename));

    if (!file)
    {
        wxMessageBox("Cannot save to this file: " + filename, "NumeRe: Error", wxID_OK | wxICON_ERROR, this);
        return;
    }

    // Get a reference to the table and copy the
    // necessary fields to the file
    NumeRe::Table& table = static_cast<GridNumeReTable*>(GetTable())->getTableRef();
    file->setDimensions(table.getLines(), table.getCols());
    file->setData(&table.getTableData(), table.getLines(), table.getCols());
    file->setTableName(m_displayName.substr(0, m_displayName.find_first_of("({")));
    file->setTextfilePrecision(7);

    if (file->getExtension() == "ndat")
        static_cast<NumeRe::NumeReDataFile*>(file.get())->setComment(table.getMetaData().comment);

    // Try to write to the file
    try
    {
        file->write();
    }
    catch (...)
    {
        wxMessageBox("Cannot save to this file: " + filename, "NumeRe: Error", wxID_OK | wxICON_ERROR, this);
        return;
    }
}


/////////////////////////////////////////////////
/// \brief Reloads the currently displayed table
/// data from the kernel.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::reloadTable()
{
    if (!m_parentPanel)
        return;

    NumeReTerminal* term = m_parentPanel->GetTerminal();

    if (isGridNumeReTable)
    {
        NumeRe::Table _tab = term->getTable(m_intName);
        SetData(_tab, m_displayName, m_intName);
    }
    else
    {
        NumeRe::Container<std::string> _strTab = term->getStringTable(m_intName);
        SetData(_strTab, m_displayName, m_intName);
    }
}


/////////////////////////////////////////////////
/// \brief Enables the switching of the column
/// types of the selected columns.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::changeColType()
{
    if (!isGridNumeReTable)
        return;

    wxGridCellCoordsContainer coordsContainer;

    // Handle all possible selection types
    if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size()) // block layout
        coordsContainer = wxGridCellCoordsContainer(GetSelectionBlockTopLeft()[0], GetSelectionBlockBottomRight()[0], this);
    else if (GetSelectedCols().size()) // multiple selected columns
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCols(), GetRows()-1, false, this);
    else if (GetSelectedCells().size())
        coordsContainer = wxGridCellCoordsContainer(GetSelectedCells(), this);
    else
        coordsContainer = wxGridCellCoordsContainer(wxArrayInt(1, GetCursorColumn()), GetRows()-1, false, this);

    // Get the target column type
    std::vector<std::string> vTypes = TableColumn::getTypesAsString();
    wxArrayString types;

    for (const auto& t : vTypes)
    {
        types.Add(t);
    }

    wxString strType = wxGetSingleChoice(_guilang.get("GUI_TABLE_CHANGE_TYPE"), _guilang.get("GUI_TABLE_CHANGE_TYPE_HEAD"), types, 0, this);

    if (!strType.length())
        return;

    // Get the type ID
    TableColumn::ColumnType type = TableColumn::stringToType(strType.ToStdString());

    // Change the column types
    wxGridCellsExtent _extent = coordsContainer.getExtent();
    NumeRe::Table& _table = static_cast<GridNumeReTable*>(GetTable())->getTableRef();

    for (int j = _extent.m_topleft.GetCol(); j <= _extent.m_bottomright.GetCol(); j++)
    {
        if (!coordsContainer.contains(0, j))
            continue;

        // Change the typ of this column
        if (_table.setColumnType(j, type))
        {
            // We have to refresh the alignment and to update the
            // renderers as well
            UpdateColumnAlignment(j);
        }
    }

    // Refresh now, bc. we probably changed the renderers
    Refresh();
}


/////////////////////////////////////////////////
/// \brief Ensure that the editors are all
/// closed.
///
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::finalize()
{
    SaveEditControlValue();
}


/////////////////////////////////////////////////
/// \brief Group headers between start and ending
/// column in the selected row together. Will
/// call itself recursively to apply hierarchical
/// grouping to the following rows of the table
/// headers.
///
/// \param startCol int
/// \param endCol int
/// \param row int
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::groupHeaders(int startCol, int endCol, int row)
{
    // Go through the columns
    for (int j = startCol; j < endCol; j++)
    {
        // Get the value of the current column header
        wxString startColValue = GetCellValue(row, j);

        // We only want grouping for header columns with
        // contents
        if (startColValue.length())
        {
            // Find the first column with new contents
            for (int n = j+1; n < endCol; n++)
            {
                if (GetCellValue(row, n) != startColValue)
                {
                    // If at least two columns fit together,
                    // group them
                    if (n-j > 1)
                    {
                        SetCellSize(row, j, 1, n-j);

                        // If there are further rows, call
                        // this function recursively for the grouped
                        // set of columns
                        if (row+1 < (int)nFirstNumRow)
                            groupHeaders(j, n, row+1);
                    }

                    j = n-1;
                    break;
                }

                // Last one, still equal
                if (n+1 == endCol)
                {
                    // If at least two columns fit together,
                    // group them
                    if (n-j >= 1)
                    {
                        SetCellSize(row, j, 1, endCol-j);

                        // If there are further rows, call
                        // this function recursively for the grouped
                        // set of columns
                        if (row+1 < (int)nFirstNumRow)
                            groupHeaders(j, endCol, row+1);
                    }

                    return;
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Was the last cursor move a silent move?
///
/// \return bool
///
/////////////////////////////////////////////////
bool TableViewer::isSilentSelection()
{
    bool isSilent = isSilentCursorMove;
    isSilentCursorMove = false;
    return isSilent;
}


/////////////////////////////////////////////////
/// \brief Enable or disable the quoation marks
/// around strings. Only works, if the grid is
/// using a GridNumeReTable as base.
///
/// \param enable bool
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::enableQuotationMarks(bool enable)
{
    if (isGridNumeReTable)
        static_cast<GridNumeReTable*>(GetTable())->enableQuotationMarks(enable);
}


/////////////////////////////////////////////////
/// \brief Returns the values of all selected
/// cells as a string list.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array TableViewer::getSelectedValues()
{
    // Simple case: only one cell
    if (!(GetSelectedCells().size()
        || GetSelectedCols().size()
        || GetSelectedRows().size()
        || GetSelectionBlockTopLeft().size()
        || GetSelectionBlockBottomRight().size()))
        return get(GetCursorRow(), GetCursorColumn());

    mu::Array values;

    // More difficult: multiple selections
    if (GetSelectedCells().size())
    {
        // not a block layout
        wxGridCellCoordsArray cellarray = GetSelectedCells();

        for (size_t i = 0; i < cellarray.size(); i++)
        {
            values.push_back(get(cellarray[i].GetRow(), cellarray[i].GetCol()));
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
                values.push_back(get(i, j));
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
                values.push_back(get(i, colarray[j]));
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
                values.push_back(get(rowarray[i], j));
            }
        }
    }

    return values;
}


/////////////////////////////////////////////////
/// \brief Converts the grid rows to the internal
/// table rows.
///
/// \param gridrow int
/// \return int
///
/////////////////////////////////////////////////
int TableViewer::GetInternalRows(int gridrow) const
{
    if (isGridNumeReTable)
        gridrow -= (int)static_cast<GridNumeReTable*>(GetTable())->getTableRef().getHeadCount();
    else
        gridrow -= (int)nFirstNumRow;

    if (gridrow < 0)
        return -1;

    return gridrow;
}


/////////////////////////////////////////////////
/// \brief Converts the grid rows to the external
/// table rows.
///
/// \param gridrow int
/// \return int
///
/////////////////////////////////////////////////
int TableViewer::GetExternalRows(int gridrow) const
{
    if (isGridNumeReTable)
        gridrow += (int)static_cast<GridNumeReTable*>(GetTable())->getTableRef().getHeadCount();
    else
        gridrow += (int)nFirstNumRow;

    if (gridrow >= GetNumberRows())
        return GetNumberRows()-1;

    return gridrow;
}


/////////////////////////////////////////////////
/// \brief Return, whether all data rows are
/// currently shown and no rows are hidden.
///
/// \return bool
///
/////////////////////////////////////////////////
bool TableViewer::allRowsShown() const
{
    for (int i = nFirstNumRow; i < GetRows()-1; i++)
    {
        if (!IsRowShown(i))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Return, whether all data columns are
/// currently shown and no columns are hidden.
///
/// \return bool
///
/////////////////////////////////////////////////
bool TableViewer::allColsShown() const
{
    for (int j = 0; j < GetCols()-1; j++)
    {
        if (!IsColShown(j))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Calculates the minimal size necessary
/// to display the whole table.
///
/// \return wxSize
///
/////////////////////////////////////////////////
wxSize TableViewer::calculateMinSize() const
{
    wxSize minSize;

    minSize.y = GetRowHeight(0) * GetNumberRows() + GetColLabelSize() + 5;
    minSize.x = GetColSize(0) * GetNumberCols() + GetRowLabelSize() + 5;

    return minSize;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// internal NumeRe::Table from the data provider
/// object.
///
/// \return NumeRe::Table
/// \note The data provider is most likely not in
/// a valid state after a call to this function.
///
/////////////////////////////////////////////////
NumeRe::Table TableViewer::GetData()
{
    NumeRe::Table table = static_cast<GridNumeReTable*>(GetTable())->getTable();

    if (!readOnly && m_parentPanel)
        table.setComment(m_parentPanel->getComment());

    return table;
}


/////////////////////////////////////////////////
/// \brief This member function returns a safe
/// copy of the internal NumeRe::Table.
///
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table TableViewer::GetDataCopy()
{
    NumeRe::Table tableCopy = static_cast<GridNumeReTable*>(GetTable())->getTableCopy();

    if (!readOnly && m_parentPanel)
        tableCopy.setComment(m_parentPanel->getComment());

    return tableCopy;
}

