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
#include "tableeditpanel.hpp"
#include "cellvalueshader.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../kernel/core/datamanagement/tablecolumn.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/tokenzr.h>
#include <wx/renderer.h>

#define STATUSBAR_PRECISION 5
#define MAXIMAL_RENDERING_SIZE 5000

// Define the standard colors
static wxColour HeadlineColor = wxColour(192, 192, 192);
static wxColour FrameColor = wxColour(230, 230, 230);
static wxColour HighlightColor = wxColour(192, 227, 248);
static wxColour HighlightHeadlineColor = wxColour(131, 200, 241);

static double calculateLuminosity(const wxColour& c)
{
    return c.Red() * 0.299 + c.Green() * 0.587 + c.Blue() * 0.114;
}


/////////////////////////////////////////////////
/// \brief This class represents an extension to
/// the usual cell string renderer to provide
/// functionalities to highlight the cursor
/// position and automatically update the
/// surrounding frame.
/////////////////////////////////////////////////
class AdvStringCellRenderer : public wxGridCellStringRenderer
{
    protected:
        CellValueShader m_shader;

        bool isHeadLine(const wxGrid& grid, int row)
        {
            return grid.GetRowLabelValue(row) == "#";
        }

        bool isFrame(const wxGrid& grid, int row, int col)
        {
            return col+1 == grid.GetNumberCols() || row+1 == grid.GetNumberRows();
        }

        bool isPartOfCursor(const wxGrid& grid, int row, int col)
        {
            return (grid.GetCursorColumn() == col || grid.GetCursorRow() == row)
                && !isFrame(grid, row, col);
        }

        bool hasCustomColor()
        {
            return m_shader.isActive();
        }

        wxGridCellAttr* createHighlightedAttr(const wxGridCellAttr& attr, const wxGrid& grid, int row, int col)
        {
            wxGridCellAttr* highlightAttr;

            if (isHeadLine(grid, row))
            {
                highlightAttr = attr.Clone();
                highlightAttr->SetBackgroundColour(HighlightHeadlineColor);
                highlightAttr->SetFont(highlightAttr->GetFont().Bold());
                highlightAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
            }
            else if (hasCustomColor())
            {
                highlightAttr = createCustomColorAttr(attr, grid, row, col);
                //static double highlightLuminosity = calculateLuminosity(HighlightColor);
                double bgLuminosity = calculateLuminosity(highlightAttr->GetBackgroundColour());
//                double factor = (bgLuminosity/highlightLuminosity-1.0)*0.8 + 1.0;
                double factor = ((bgLuminosity / 255.0 * 0.8) + 0.2);

                highlightAttr->SetBackgroundColour(wxColour(std::min(255.0, HighlightColor.Red() * factor),
                                                            std::min(255.0, HighlightColor.Green() * factor),
                                                            std::min(255.0, HighlightColor.Blue() * factor)));
            }
            else
            {
                highlightAttr = attr.Clone();
                highlightAttr->SetBackgroundColour(HighlightColor);
            }

            return highlightAttr;
        }

        wxGridCellAttr* createFrameAttr(const wxGridCellAttr& attr)
        {
            wxGridCellAttr* frameAttr = attr.Clone();
            frameAttr->SetBackgroundColour(FrameColor);
            return frameAttr;
        }

        wxGridCellAttr* createHeadlineAttr(const wxGridCellAttr& attr)
        {
            wxGridCellAttr* headlineAttr = attr.Clone();
            headlineAttr->SetBackgroundColour(HeadlineColor);
            headlineAttr->SetFont(headlineAttr->GetFont().Bold());
            headlineAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
            return headlineAttr;
        }

        wxGridCellAttr* createCustomColorAttr(const wxGridCellAttr& attr, const wxGrid& grid, int row, int col)
        {
            wxGridCellAttr* customAttr = attr.Clone();

            if (grid.GetTable()->CanGetValueAs(row, col, "complex"))
                customAttr->SetBackgroundColour(m_shader.getColour(*static_cast<mu::value_type*>(grid.GetTable()->GetValueAsCustom(row, col, "complex"))));
            else if (grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) || grid.GetTable()->CanGetValueAs(row, col, "datetime"))
                customAttr->SetBackgroundColour(m_shader.getColour(mu::value_type(grid.GetTable()->GetValueAsDouble(row, col))));
            else if (grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL))
                customAttr->SetBackgroundColour(m_shader.getColour(mu::value_type(grid.GetTable()->GetValueAsBool(row, col))));
            else
                customAttr->SetBackgroundColour(m_shader.getColour(grid.GetTable()->GetValue(row, col)));

            // Calculate luminosity and correct text colour, if necessary
            const wxColour& bgColour = customAttr->GetBackgroundColour();
            double luminosity = calculateLuminosity(bgColour);

            if (luminosity < 128)
            {
                luminosity = 255;//std::min(255.0, 255 - luminosity + 10);
                customAttr->SetTextColour(wxColour(luminosity, luminosity, luminosity));
            }

            return customAttr;
        }

    public:
        AdvStringCellRenderer(const CellValueShader shader = CellValueShader()) : m_shader(shader) {}

        virtual void Draw(wxGrid& grid,
                          wxGridCellAttr& attr,
                          wxDC& dc,
                          const wxRect& rect,
                          int row, int col,
                          bool isSelected)
        {
            if (isPartOfCursor(grid, row, col))
            {
                wxGridCellAttr* newAttr = createHighlightedAttr(attr, grid, row, col);
                wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                newAttr->DecRef();
            }
            else if (isFrame(grid, row, col))
            {
                wxGridCellAttr* newAttr = createFrameAttr(attr);
                wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                newAttr->DecRef();
            }
            else if (isHeadLine(grid, row))
            {
                wxGridCellAttr* newAttr = createHeadlineAttr(attr);
                wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                newAttr->DecRef();
            }
            else if (hasCustomColor())
            {
                wxGridCellAttr* newAttr = createCustomColorAttr(attr, grid, row, col);
                wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                newAttr->DecRef();
            }
            else
                wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
        }

        virtual wxGridCellRenderer *Clone() const
            { return new AdvStringCellRenderer(m_shader); }
};


/////////////////////////////////////////////////
/// \brief This class represents a special
/// renderer for three-state booleans, i.e.
/// booleans, which may have a undefined (e.g.
/// NAN) value.
/////////////////////////////////////////////////
class AdvBooleanCellRenderer : public AdvStringCellRenderer
{
    public:
        AdvBooleanCellRenderer(const CellValueShader& shader = CellValueShader()) : AdvStringCellRenderer(shader) {}

    // draw a check mark or nothing
        virtual void Draw(wxGrid& grid,
                          wxGridCellAttr& attr,
                          wxDC& dc,
                          const wxRect& rect,
                          int row, int col,
                          bool isSelected)
        {
            if (grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL))
            {
                if (isPartOfCursor(grid, row, col))
                {
                    wxGridCellAttr* newAttr = createHighlightedAttr(attr, grid, row, col);
                    wxGridCellRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                    newAttr->DecRef();
                }
                else if (isFrame(grid, row, col))
                {
                    wxGridCellAttr* newAttr = createFrameAttr(attr);
                    wxGridCellRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                    newAttr->DecRef();
                }
                else if (isHeadLine(grid, row))
                {
                    wxGridCellAttr* newAttr = createHeadlineAttr(attr);
                    wxGridCellRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                    newAttr->DecRef();
                }
                else if (hasCustomColor())
                {
                    wxGridCellAttr* newAttr = createCustomColorAttr(attr, grid, row, col);
                    wxGridCellRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                    newAttr->DecRef();
                }
                else
                    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

                // draw a check mark in the centre (ignoring alignment - TODO)
                wxSize size = GetBestSize(grid, attr, dc, row, col);

                // don't draw outside the cell
                wxCoord minSize = wxMin(rect.width, rect.height);
                if ( size.x >= minSize || size.y >= minSize )
                {
                    // and even leave (at least) 1 pixel margin
                    size.x = size.y = minSize;
                }

                // draw a border around checkmark
                int vAlign, hAlign;
                attr.GetAlignment(&hAlign, &vAlign);

                wxRect rectBorder;
                if (hAlign == wxALIGN_CENTRE)
                {
                    rectBorder.x = rect.x + rect.width / 2 - size.x / 2;
                    rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
                    rectBorder.width = size.x;
                    rectBorder.height = size.y;
                }
                else if (hAlign == wxALIGN_LEFT)
                {
                    rectBorder.x = rect.x + 2;
                    rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
                    rectBorder.width = size.x;
                    rectBorder.height = size.y;
                }
                else if (hAlign == wxALIGN_RIGHT)
                {
                    rectBorder.x = rect.x + rect.width - size.x - 2;
                    rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
                    rectBorder.width = size.x;
                    rectBorder.height = size.y;
                }

                bool value = grid.GetTable()->GetValueAsBool(row, col);
                int flags = 0;

                if (value)
                    flags |= wxCONTROL_CHECKED;

                wxRendererNative::Get().DrawCheckBox( &grid, dc, rectBorder, flags );
            }
            else
                AdvStringCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
        }

        // return the checkmark size
        virtual wxSize GetBestSize(wxGrid& grid,
                                   wxGridCellAttr& attr,
                                   wxDC& dc,
                                   int row, int col)
        {
            // Calculate only once bc, "---" is wider than the checkmark
            if (!bestSize.x)
                bestSize = DoGetBestSize(attr, dc, "---");

            return bestSize;
        }

        virtual wxGridCellRenderer *Clone() const
            { return new AdvBooleanCellRenderer(m_shader); }

    private:
        static wxSize bestSize;
};


wxSize AdvBooleanCellRenderer::bestSize;


extern Language _guilang;

BEGIN_EVENT_TABLE(TableViewer, wxGrid)
    EVT_KEY_DOWN                (TableViewer::OnKeyDown)
    EVT_CHAR                    (TableViewer::OnChar)
    EVT_GRID_CELL_CHANGING      (TableViewer::OnCellChange)
    EVT_GRID_CELL_RIGHT_CLICK   (TableViewer::OnCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK  (TableViewer::OnLabelRightClick)
    EVT_GRID_LABEL_LEFT_DCLICK  (TableViewer::OnLabelDoubleClick)
    EVT_MENU_RANGE              (ID_MENU_INSERT_ROW, ID_MENU_CVS, TableViewer::OnMenu)
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
TableViewer::TableViewer(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, TablePanel* parentPanel, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
            : wxGrid(parent, id, pos, size, style, name), nHeight(600), nWidth(800), nFirstNumRow(1), readOnly(true)
{
    // Cells are always aligned right and centered vertically
    SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
    SetDefaultRenderer(new AdvStringCellRenderer);

    // Prepare the context menu
    m_popUpMenu.Append(ID_MENU_CVS, _guilang.get("GUI_TABLE_CVS"));
    m_popUpMenu.AppendSeparator();
    m_popUpMenu.Append(ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS"));

    // prepare the status bar
    m_statusBar = statusbar;
    m_parentPanel = parentPanel;

    isGridNumeReTable = false;

    if (m_statusBar)
    {
        int widths[3] = {120, 120, -1};
        m_statusBar->SetStatusWidths(3, widths);
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
    }

    // Search the boundaries and color the frame correspondingly
    for (int i = 0; i < GetNumberRows(); i++)
    {
        for (int j = 0; j < GetNumberCols(); j++)
        {
            if (!i && isGridNumeReTable && (int)m_currentColTypes.size() > j)
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
            }

            if (!i && j+1 == GetNumberCols())
            {
                wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                          wxALIGN_RIGHT, wxALIGN_CENTER);
                attr->SetRenderer(new AdvStringCellRenderer);
                SetColAttr(j, attr);
            }

            if (i < (int)nFirstNumRow && j < GetNumberCols()-1)
            {
                // Headlines
                SetCellRenderer(i, j, new AdvStringCellRenderer);
            }
            else if (!isGridNumeReTable && GetCellValue(i, j)[0] == '"')
            {
                SetCellAlignment(wxALIGN_LEFT, i, j);
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
    else if (event.GetKeyCode() == WXK_DELETE)
    {
        if (!readOnly)
            deleteSelection();
    }
    else if (event.ControlDown() && event.ShiftDown())
    {
        if (event.GetKeyCode() == 'C')
            copyContents();
        else if (event.GetKeyCode() == 'V')
            pasteContents();
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
    updateStatusBar(coords, coords, &coords);
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
        if (selectedCells.size() < 2)
        {
            selectedCells.Add(event.GetTopLeftCoords());
            selectedCells.Add(event.GetBottomRightCoords());
        }
        else
        {
            selectedCells[0].Set(std::min(selectedCells[0].GetRow(), event.GetTopLeftCoords().GetRow()),
                                 std::min(selectedCells[0].GetCol(), event.GetTopLeftCoords().GetCol()));
            selectedCells[1].Set(std::max(selectedCells[1].GetRow(), event.GetBottomRightCoords().GetRow()),
                                 std::max(selectedCells[1].GetCol(), event.GetBottomRightCoords().GetCol()));
        }

        updateStatusBar(selectedCells[0], selectedCells[1]);
    }
    else
        selectedCells.Clear();
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

            if (i < GetRows()-1)
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

    // Search the boundaries and color the frame correspondingly
    for (int i = 0; i < GetNumberRows(); i++)
    {
        if (i < (int)nFirstNumRow && col+1 < GetNumberCols())
        {
            // Headlines
            SetCellRenderer(i, col, new AdvStringCellRenderer);
            SetCellFont(i, col, GetCellFont(i, col).MakeBold());
            SetCellBackgroundColour(i, col, HeadlineColor);
            SetCellAlignment(wxALIGN_LEFT, i, col);
        }
        else if (!isGridNumeReTable) // The other case is handled column-wise
        {
            if (GetCellValue(i, col)[0] == '"')
                SetCellAlignment(wxALIGN_LEFT, i, col);
            else
                SetCellAlignment(wxALIGN_RIGHT, i, col);
        }
        else
            break;
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
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type TableViewer::CellToCmplx(int row, int col)
{
    if (isGridNumeReTable)
        return *static_cast<mu::value_type*>(GetTable()->GetValueAsCustom(row, col, "complex"));

    if (GetCellValue(row, col)[0] != '"' && isNumerical(GetCellValue(row, col).ToStdString()))
        return StrToCmplx(GetCellValue(row, col).ToStdString());

    return NAN;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// minimal value of the selected cells.
///
/// \param topLeft const wxGridCellCoords&
/// \param bottomRight const wxGridCellCoords&
/// \return double
///
/////////////////////////////////////////////////
double TableViewer::calculateMin(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->min(topLeft.GetRow(), topLeft.GetCol(), bottomRight.GetRow(), bottomRight.GetCol());

    double dMin = NAN;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
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
/// \param topLeft const wxGridCellCoords&
/// \param bottomRight const wxGridCellCoords&
/// \return double
///
/////////////////////////////////////////////////
double TableViewer::calculateMax(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->max(topLeft.GetRow(), topLeft.GetCol(), bottomRight.GetRow(), bottomRight.GetCol());

    double dMax = NAN;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
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
/// \param topLeft const wxGridCellCoords&
/// \param bottomRight const wxGridCellCoords&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type TableViewer::calculateSum(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->sum(topLeft.GetRow(), topLeft.GetCol(), bottomRight.GetRow(), bottomRight.GetCol());

    mu::value_type dSum = 0;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j<= bottomRight.GetCol(); j++)
        {
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
/// \param topLeft const wxGridCellCoords&
/// \param bottomRight const wxGridCellCoords&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type TableViewer::calculateAvg(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight)
{
    if (isGridNumeReTable)
        return static_cast<GridNumeReTable*>(GetTable())->avg(topLeft.GetRow(), topLeft.GetCol(), bottomRight.GetRow(), bottomRight.GetCol());

    mu::value_type dSum = 0;
    int nCount = 0;

    for (int i = topLeft.GetRow(); i <= bottomRight.GetRow(); i++)
    {
        for (int j = topLeft.GetCol(); j <= bottomRight.GetCol(); j++)
        {
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
void TableViewer::updateStatusBar(const wxGridCellCoords& topLeft, const wxGridCellCoords& bottomRight, wxGridCellCoords* cursor /*= nullptr*/)
{
    if (!m_statusBar)
        return;

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
    wxString statustext = "Min: " + toString(calculateMin(topLeft, bottomRight), STATUSBAR_PRECISION);
    statustext << " | Max: " << toString(calculateMax(topLeft, bottomRight), STATUSBAR_PRECISION);
    statustext << " | Sum: " << toString(calculateSum(topLeft, bottomRight), 2*STATUSBAR_PRECISION);
    statustext << " | Avg: " << toString(calculateAvg(topLeft, bottomRight), 2*STATUSBAR_PRECISION);

    // Set the status bar valuey
    m_statusBar->SetStatusText(dim);
    m_statusBar->SetStatusText(sel, 1);
    m_statusBar->SetStatusText(statustext, 2);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// contents of the selected cells and replaces
/// whitespaces with underscores on-the-fly.
///
/// \param row int
/// \param col int
/// \return wxString
///
/////////////////////////////////////////////////
wxString TableViewer::copyCell(int row, int col)
{
    return this->GetCellValue(row, col);
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
    SetTable(new GridNumeReTable(NumeRe::Table(1, 1)), true);

    isGridNumeReTable = true;
    nFirstNumRow = 1;

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
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
            for (unsigned int i = 1; i < sLine.length()-1; i++)
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
        else if (!tabSeparated)
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


/////////////////////////////////////////////////
/// \brief This member function is the data
/// setter for string and cluster tables.
///
/// \param _stringTable NumeRe::Container<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetData(NumeRe::Container<std::string>& _stringTable)
{
    if (!_stringTable.getCols() || !_stringTable.getRows())
    {
        createZeroElementTable();
        return;
    }

    this->CreateGrid(_stringTable.getRows()+1, _stringTable.getCols()+1);

    // String tables and clusters do not have a headline
    nFirstNumRow = 0;
    isGridNumeReTable = false;

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
                this->SetCellValue(i, j, replaceControlCharacters(_stringTable.get(i, j)));
        }
    }

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
}


/////////////////////////////////////////////////
/// \brief This member function is the data
/// setter function. It will create an internal
/// GridNumeReTable object, which will provide
/// the data for the grid.
///
/// \param _table NumeRe::Table&
/// \return void
///
/////////////////////////////////////////////////
void TableViewer::SetData(NumeRe::Table& _table)
{
    if (m_parentPanel)
        m_parentPanel->update(_table.getMetaData());

    // Create an empty table, if necessary
    if (_table.isEmpty() || !_table.getLines())
    {
        createZeroElementTable();
        return;
    }

    // Store the number headlines and create the data
    // providing object
    nFirstNumRow = _table.getHeadCount();
    SetTable(new GridNumeReTable(std::move(_table)), true);
    isGridNumeReTable = true;

    layoutGrid();

    updateStatusBar(wxGridCellCoords(0,0), wxGridCellCoords(this->GetRows()-1, this->GetCols()-1));
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
    m_lastRightClick = wxGridCellCoords(event.GetRow(), event.GetCol());

    if (!readOnly)
    {
        for (int i = ID_MENU_INSERT_ROW; i <= ID_MENU_REMOVE_CELL; i++)
            m_popUpMenu.Enable(i,true);

        m_popUpMenu.Enable(ID_MENU_PASTE_HERE, true);
    }

    PopupMenu(&m_popUpMenu, event.GetPosition());
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
        case ID_MENU_CVS:
        {
            double minVal;
            double maxVal;

            // More diffcult: more than one cell selected
            if (GetSelectedCells().size())
            {
                // Non-block selection
                //wxGridCellCoordsArray cellarray = GetSelectedCells();
                //
                //for (size_t i = 0; i < cellarray.size(); i++)
                //{
                //    sSelection += copyCell(cellarray[i].GetRow(), cellarray[i].GetCol());
                //
                //    if (i < cellarray.size()-1)
                //        sSelection += "\t";
                //}
            }
            else if (GetSelectionBlockTopLeft().size() && GetSelectionBlockBottomRight().size())
            {
                // Block selection
                wxGridCellCoordsArray topleftarray = GetSelectionBlockTopLeft();
                wxGridCellCoordsArray bottomrightarray = GetSelectionBlockBottomRight();

                minVal = calculateMin(topleftarray[0], bottomrightarray[0]);
                maxVal = calculateMax(topleftarray[0], bottomrightarray[0]);

                CellValueShaderDialog dialog(this, minVal, maxVal);

                if (dialog.ShowModal() == wxID_OK)
                {
                    // Whole columns are selected
                    if (topleftarray[0].GetRow() <= (int)nFirstNumRow && bottomrightarray[0].GetRow()+2 >= GetRows())
                    {
                        for (int j = topleftarray[0].GetCol(); j <= bottomrightarray[0].GetCol(); j++)
                        {
                            int h_align, v_align;
                            GetCellAlignment(nFirstNumRow, j, &h_align, &v_align);

                            wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                                      h_align, wxALIGN_CENTER);

                            if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                                attr->SetRenderer(new AdvBooleanCellRenderer(dialog.getShader()));
                            else
                                attr->SetRenderer(new AdvStringCellRenderer(dialog.getShader()));

                            SetColAttr(j, attr, true);

                            for (size_t i = 0; i < nFirstNumRow; i++)
                                SetCellRenderer(i, j, new AdvStringCellRenderer);
                        }
                    }
                    else
                    {
                        for (int i = topleftarray[0].GetRow(); i <= bottomrightarray[0].GetRow(); i++)
                        {
                            for (int j = topleftarray[0].GetCol(); j <= bottomrightarray[0].GetCol(); j++)
                            {
                                if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                                    SetCellRenderer(i, j, new AdvBooleanCellRenderer(dialog.getShader()));
                                else
                                    SetCellRenderer(i, j, new AdvStringCellRenderer(dialog.getShader()));
                            }
                        }
                    }

                }

            }
            else if (GetSelectedCols().size())
            {
                // Multiple selected columns
                wxArrayInt colarray = GetSelectedCols();

                minVal = calculateMin(wxGridCellCoords(0, colarray[0]), wxGridCellCoords(GetRows()-1, colarray[colarray.size()-1]));
                maxVal = calculateMax(wxGridCellCoords(0, colarray[0]), wxGridCellCoords(GetRows()-1, colarray[colarray.size()-1]));

                CellValueShaderDialog dialog(this, minVal, maxVal);

                if (dialog.ShowModal() == wxID_OK)
                {
                    // Whole columns are selected
                    for (size_t j = 0; j < colarray.size(); j++)
                    {
                        int h_align, v_align;
                        GetCellAlignment(nFirstNumRow, j, &h_align, &v_align);

                        wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                                  h_align, wxALIGN_CENTER);

                        if (m_currentColTypes.size() > (size_t)colarray[j] && m_currentColTypes[colarray[j]] == TableColumn::TYPE_LOGICAL)
                            attr->SetRenderer(new AdvBooleanCellRenderer(dialog.getShader()));
                        else
                            attr->SetRenderer(new AdvStringCellRenderer(dialog.getShader()));

                        SetColAttr(colarray[j], attr, true);

                        for (size_t i = 0; i < nFirstNumRow; i++)
                            SetCellRenderer(i, colarray[j], new AdvStringCellRenderer);
                    }
                }
            }
            else if (GetSelectedRows().size())
            {
                // Multiple selected rows
                wxArrayInt rowarray = GetSelectedRows();

                minVal = calculateMin(wxGridCellCoords(rowarray[0], 0), wxGridCellCoords(rowarray[rowarray.size()-1], GetCols()-1));
                maxVal = calculateMax(wxGridCellCoords(rowarray[0], 0), wxGridCellCoords(rowarray[rowarray.size()-1], GetCols()-1));

                CellValueShaderDialog dialog(this, minVal, maxVal);

                if (dialog.ShowModal() == wxID_OK)
                {
                    for (size_t i = 0; i < rowarray.size(); i++)
                    {
                        for (int j = 0; j < GetCols()-1; j++)
                        {
                            if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                                SetCellRenderer(rowarray[i], j, new AdvBooleanCellRenderer(dialog.getShader()));
                            else
                                SetCellRenderer(rowarray[i], j, new AdvStringCellRenderer(dialog.getShader()));
                        }
                    }
                }
            }
            else
            {
                minVal = calculateMin(wxGridCellCoords(0, 0), wxGridCellCoords(GetRows()-1, GetCols()-1));
                maxVal = calculateMax(wxGridCellCoords(0, 0), wxGridCellCoords(GetRows()-1, GetCols()-1));

                CellValueShaderDialog dialog(this, minVal, maxVal);

                if (dialog.ShowModal() == wxID_OK)
                {
                    // Whole columns are selected
                    for (int j = 0; j < GetCols()-1; j++)
                    {
                        int h_align, v_align;
                        GetCellAlignment(nFirstNumRow, j, &h_align, &v_align);

                        wxGridCellAttr* attr = new wxGridCellAttr(*wxBLACK, *wxWHITE, this->GetDefaultCellFont(),
                                                                  h_align, wxALIGN_CENTER);

                        if (m_currentColTypes.size() > (size_t)j && m_currentColTypes[j] == TableColumn::TYPE_LOGICAL)
                            attr->SetRenderer(new AdvBooleanCellRenderer(dialog.getShader()));
                        else
                            attr->SetRenderer(new AdvStringCellRenderer(dialog.getShader()));

                        SetColAttr(j, attr, true);

                        for (size_t i = 0; i < nFirstNumRow; i++)
                            SetCellRenderer(i, j, new AdvStringCellRenderer);
                    }
                }
            }

            Refresh();

            break;
        }
    }
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


/////////////////////////////////////////////////
/// \brief Returns the values of all selected
/// cells as a string list.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString TableViewer::getSelectedValues()
{
    // Simple case: only one cell
    if (!(GetSelectedCells().size()
        || GetSelectedCols().size()
        || GetSelectedRows().size()
        || GetSelectionBlockTopLeft().size()
        || GetSelectionBlockBottomRight().size()))
        return this->GetCellValue(this->GetCursorRow(), this->GetCursorColumn());

    wxString values;

    // More difficult: multiple selections
    if (GetSelectedCells().size())
    {
        // not a block layout
        wxGridCellCoordsArray cellarray = GetSelectedCells();

        for (size_t i = 0; i < cellarray.size(); i++)
        {
            values += this->GetCellValue(cellarray[i]) + ",";
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
                values += GetCellValue(i, j) + ",";
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
                values += this->GetCellValue(i, colarray[j]) + ",";
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
                values += this->GetCellValue(rowarray[i], j) + ",";
            }
        }
    }

    if (values.length())
        return values.substr(0, values.length()-1);

    return "";
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

