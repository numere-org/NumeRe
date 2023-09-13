/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#ifndef CELLATTRIBUTES_HPP
#define CELLATTRIBUTES_HPP

#include <wx/renderer.h>
#include <wx/generic/grideditors.h>

#include "cellvalueshader.hpp"

#define DATESTRINGLEN 24

// Define the standard colors
static wxColour HeadlineColor = wxColour(192, 192, 192);
static wxColour FrameColor = wxColour(230, 230, 230);
static wxColour HighlightColor = wxColour(192, 227, 248);
static wxColour HighlightTextColor = wxColour(226, 242, 252);
//static wxColour HighlightHeadlineColor = wxColour(131, 200, 241);
static wxColour HighlightHeadlineColor = wxColour(80, 176, 235);

/////////////////////////////////////////////////
/// \brief Calculates the luminosity of the
/// passed colour.
///
/// \param c const wxColour&
/// \return double
///
/////////////////////////////////////////////////
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
class AdvStringCellRenderer : public wxGridCellAutoWrapStringRenderer
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
            int rows, cols;
            grid.GetCellSize(row, col, &rows, &cols);

            int cursorRow = grid.GetCursorRow();
            int cursorCol = grid.GetCursorColumn();

            int cursorRows, cursorCols;
            grid.GetCellSize(cursorRow, cursorCol, &cursorRows, &cursorCols);

            return (cursorRow == row || (col <= cursorCol && cursorCol < col+cols) || (cursorCol <= col && col < cursorCol+cursorCols))
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
                highlightAttr->SetTextColour(HighlightTextColor);

                int rows, cols;

                if (grid.GetCellSize(row, col, &rows, &cols) == wxGrid::CellSpan_Main)
                    highlightAttr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
                else
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

        wxGridCellAttr* createHeadlineAttr(const wxGridCellAttr& attr, const wxGrid& grid, int row, int col)
        {
            wxGridCellAttr* headlineAttr = attr.Clone();
            headlineAttr->SetBackgroundColour(HeadlineColor);
            headlineAttr->SetFont(headlineAttr->GetFont().Bold());

            int rows, cols;

            if (grid.GetCellSize(row, col, &rows, &cols) == wxGrid::CellSpan_Main)
                headlineAttr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
            else
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

                if (grid.GetCellValue(row, col).length() > DATESTRINGLEN)
                    wxGridCellAutoWrapStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                else
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
                wxGridCellAttr* newAttr = createHeadlineAttr(attr, grid, row, col);

                if (grid.GetCellValue(row, col).length() > DATESTRINGLEN)
                    wxGridCellAutoWrapStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                else
                    wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);

                newAttr->DecRef();
            }
            else if (hasCustomColor())
            {
                wxGridCellAttr* newAttr = createCustomColorAttr(attr, grid, row, col);

                if (grid.GetCellValue(row, col).length() > DATESTRINGLEN)
                    wxGridCellAutoWrapStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);
                else
                    wxGridCellStringRenderer::Draw(grid, *newAttr, dc, rect, row, col, isSelected);

                newAttr->DecRef();
            }
            else
            {
                if (grid.GetCellValue(row, col).length() > DATESTRINGLEN)
                    wxGridCellAutoWrapStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
                else
                    wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
            }
        }

        virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col)
        {
            if (grid.GetCellValue(row, col).length() > DATESTRINGLEN)
                return wxGridCellAutoWrapStringRenderer::GetBestSize(grid, attr, dc, row, col);
            else
                return wxGridCellStringRenderer::GetBestSize(grid, attr, dc, row, col);
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
            if (grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL) && !isHeadLine(grid, row))
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


/////////////////////////////////////////////////
/// \brief This class represents the grid cell
/// editor which automatically selects the
/// necessary edit control for the underlying
/// column data type.
/////////////////////////////////////////////////
class CombinedCellEditor : public wxGridCellEditor
{
    public:
        /////////////////////////////////////////////////
        /// \brief Construct this editor for the current
        /// wxGrid.
        ///
        /// \param grid wxGrid*
        ///
        /////////////////////////////////////////////////
        CombinedCellEditor(wxGrid* grid) : m_grid(grid), m_finished(false) {}

        /////////////////////////////////////////////////
        /// \brief Create the necessary edit controls.
        ///
        /// \param parent wxWindow*
        /// \param id wxWindowID
        /// \param evtHandler wxEvtHandler*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler) override
        {
            int style = wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB | wxBORDER_NONE | wxTE_MULTILINE | wxTE_RICH;

            // Create the text control
            m_text = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, style);
            m_text->SetMargins(0, 0);
            m_text->Hide();

            // The text control is the default control
            m_control = m_text;

            // Create the check box
            m_checkBox = new wxCheckBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            m_checkBox->Hide();

            // Create the combo box with an empty set of choices
            m_comboBox = new wxComboBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxArrayString(), style);
            m_comboBox->Hide();

            // Bind the ENTER key event of the combo box to the event
            // handler in this class
            m_comboBox->Bind(wxEVT_TEXT_ENTER, CombinedCellEditor::OnEnterKey, this);

            wxGridCellEditor::Create(parent, id, evtHandler);
        }

        /////////////////////////////////////////////////
        /// \brief Set size and position of the controls.
        ///
        /// \param _rect const wxRect&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void SetSize(const wxRect& _rect) override
        {
            SetTextCtrlSize(_rect);
            SetCheckBoxSize(_rect);
        }

        /////////////////////////////////////////////////
        /// \brief Paint the background.
        ///
        /// \param dc wxDC&
        /// \param rectCell const wxRect&
        /// \param attr const wxGridCellAttr&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void PaintBackground(wxDC& dc, const wxRect& rectCell, const wxGridCellAttr& attr) override
        {
            wxGridCellEditor::PaintBackground(dc, rectCell, attr);
        }

        /////////////////////////////////////////////////
        /// \brief Determine, whether the pressed key is
        /// accepted by this editor and will start the
        /// editing process.
        ///
        /// \param event wxKeyEvent&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool IsAcceptedKey(wxKeyEvent& event) override
        {
            switch (event.GetKeyCode())
            {
                case WXK_DELETE:
                case WXK_BACK:
                case WXK_SPACE:
                case '+':
                case '-':
                    return true;

                default:
                    return wxGridCellEditor::IsAcceptedKey(event);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Begin the editing process. Will select
        /// the correct edit control depending on the
        /// underlying column data type.
        ///
        /// \param row int
        /// \param col int
        /// \param grid wxGrid*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void BeginEdit(int row, int col, wxGrid* grid) override
        {
            // Get the underlying table of the grid
            GridNumeReTable* _tab = static_cast<GridNumeReTable*>(grid->GetTable());

            // Get the value of the current cell as string
            m_value = _tab->GetValue(row, col);
            m_finished = false;

            // Get the column types of the table
            std::vector<int> vTypes = _tab->getColumnTypes();

            // Determine, whether the current row is a headline row
            bool isHead = grid->GetRowLabelValue(row) == "#";

            // Select the correct edit control from the column types and
            // depending on whether the current row is a headline row
            if (!isHead
                && (int)vTypes.size() > col
                && vTypes[col] == TableColumn::TYPE_LOGICAL)
            {
                // Select the check box
                m_control = m_checkBox;
                m_checkBox->SetValue(m_value == "true");
            }
            else if (!isHead
                     && (int)vTypes.size() > col
                     && vTypes[col] == TableColumn::TYPE_CATEGORICAL)
            {
                // Select the combo box
                m_control = m_comboBox;

                // Get the categories and pass them into a
                // wxArrayString instance
                const std::vector<std::string>& vCategories = _tab->getCategories(col);
                wxArrayString cats;

                for (const auto& c : vCategories)
                    cats.Add(c);

                // Set the categories as choices for the current
                // cell
                m_comboBox->Set(cats);
                wxGridCellEditorEvtHandler* evtHandler = nullptr;

                if (m_comboBox)
                    evtHandler = wxDynamicCast(m_comboBox->GetEventHandler(), wxGridCellEditorEvtHandler);

                // Don't immediately end if we get a kill focus event within BeginEdit
                if (evtHandler)
                    evtHandler->SetInSetFocus(true);

                Reset(); // this updates combo box to correspond to m_value

                if (evtHandler)
                {
                    // When dropping down the menu, a kill focus event
                    // happens after this point, so we can't reset the flag yet.
                    evtHandler->SetInSetFocus(false);
                }
            }
            else // All other cases and the headline
            {
                // Select the text control, which is the default control
                m_control = m_text;
                m_text->SetValue(m_value);
                m_text->SetInsertionPointEnd();
                m_text->SelectAll();
            }

            // Show the selected control and give it the keyboard
            // focus to allow direct interaction
            m_control->Show();
            m_control->SetFocus();
        }

        /////////////////////////////////////////////////
        /// \brief End the editing process. Will store
        /// the value of the edit control internally and
        /// reset the edit control to its original state.
        ///
        /// \param row int
        /// \param col int
        /// \param grid const wxGrid*
        /// \param oldval const wxString&
        /// \param newval wxString*
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString *newval) override
        {
            // Do not call this function twice
            if (m_finished)
                return false;

            m_finished = true;

            // Get the value from the control
            if (m_control == m_text)
            {
                const wxString value = m_text->GetValue();

                if (value == m_value)
                    return false;

                m_value = value;
            }
            else if (m_control == m_checkBox)
            {
                // Reset the control
                m_control = m_text;
                bool value = m_checkBox->GetValue();

                if (toString(value) == m_value)
                    return false;

                m_value = toString(value);
            }
            else if (m_control == m_comboBox)
            {
                // Reset the control
                m_control = m_text;
                const wxString value = m_comboBox->GetValue();

                if (value == m_value)
                    return false;

                m_value = value;
            }
            else
                return false;

            // Return the new value, if the used supplied a
            // pointer
            if (newval)
                *newval = m_value;

            return true;
        }

        /////////////////////////////////////////////////
        /// \brief Called after EndEdit if the user did
        /// not cancel the process and will store the
        /// value in the correct cell.
        ///
        /// \param row int
        /// \param col int
        /// \param grid wxGrid*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void ApplyEdit(int row, int col, wxGrid* grid) override
        {
            grid->GetTable()->SetValue(row, col, m_value);
            m_value.clear();
        }

        /////////////////////////////////////////////////
        /// \brief Reset the control to its initial state.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void Reset() override
        {
            if (m_control == m_text)
            {
                m_text->SetValue(m_value);
                m_text->SetInsertionPointEnd();
            }
            else if (m_control == m_checkBox)
                m_checkBox->SetValue(m_value == "true");
            else if (m_control == m_comboBox)
            {
                m_comboBox->SetValue(m_value);
                m_comboBox->SetInsertionPointEnd();
            }
        }

        /////////////////////////////////////////////////
        /// \brief Called after BeginEdit if the user
        /// clicked on this cell to handle the click
        /// event.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void StartingClick() override
        {
            if (m_control == m_checkBox)
            {
                m_checkBox->SetValue(!m_checkBox->GetValue());
                finalize(false);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Called after BeginEdit to give this
        /// control the possibility to respond to the
        /// initial key.
        ///
        /// \param event wxKeyEvent&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void StartingKey(wxKeyEvent& event) override
        {
            // Since this is now happening in the EVT_CHAR event EmulateKeyPress is no
            // longer an appropriate way to get the character into the text control.
            // Do it ourselves instead.  We know that if we get this far that we have
            // a valid character, so not a whole lot of testing needs to be done.
            if (m_control == m_text || m_control == m_comboBox)
            {
                // Handle text ctrl and combo box commonly
                wxTextEntry* textField = dynamic_cast<wxTextEntry*>(m_control);

                // Ensure that the cast was successful
                if (!textField)
                    return;

                int ch;
                bool isPrintable = false;

                ch = event.GetUnicodeKey();

                if (ch != WXK_NONE)
                    isPrintable = true;
                else
                {
                    ch = event.GetKeyCode();
                    isPrintable = ch >= WXK_SPACE && ch < WXK_START;
                }

                // Evaluate the pressed key
                switch (ch)
                {
                    case WXK_DELETE:
                        // Delete the initial character when starting to edit with DELETE.
                        textField->Remove(0, 1);
                        break;

                    case WXK_BACK:
                        // Delete the last character when starting to edit with BACKSPACE.
                        {
                            const long pos = textField->GetLastPosition();
                            textField->Remove(pos - 1, pos);
                        }
                        break;

                    case WXK_ESCAPE:
                        Reset();
                        break;

                    default:
                        if (isPrintable)
                            textField->WriteText(static_cast<wxChar>(ch));
                        break;
                }
            }
            else if (m_control == m_checkBox)
            {
                int keycode = event.GetKeyCode();

                // Evaluate the pressed key
                switch (keycode)
                {
                    case WXK_SPACE:
                        // Toggle
                        m_checkBox->SetValue(!m_checkBox->GetValue());
                        break;

                    case '+':
                        // Set true
                        m_checkBox->SetValue(true);
                        break;

                    case '-':
                        // Set false
                        m_checkBox->SetValue(false);
                        break;

                    case WXK_ESCAPE:
                        Reset();
                        break;

                    default:
                        // The user wants to change the column type. We'll
                        // hide the checkbox, enable the text control and
                        // re-call this function
                        m_checkBox->Hide();
                        m_control = m_text;
                        m_text->Show();
                        m_text->SetValue(m_value);
                        m_text->SetInsertionPointEnd();
                        m_text->SelectAll();
                        m_text->SetFocus();
                        StartingKey(event);
                }
            }
        }

        /////////////////////////////////////////////////
        /// \brief We do not handle the return key as a
        /// character.
        ///
        /// \param event wxKeyEvent&
        /// \return virtual void
        ///
        /////////////////////////////////////////////////
        virtual void HandleReturn(wxKeyEvent& event) override
        {
            event.Skip();
        }

        /////////////////////////////////////////////////
        /// \brief Get a copy of this editor instance.
        ///
        /// \return wxGridCellEditor*
        ///
        /////////////////////////////////////////////////
        virtual wxGridCellEditor *Clone() const override
        {
            return new CombinedCellEditor(m_grid);
        }

        /////////////////////////////////////////////////
        /// \brief Get the value stored in this editor.
        ///
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        virtual wxString GetValue() const override
        {
            return m_value;
        }

        /////////////////////////////////////////////////
        /// \brief Show or hide the edit control. Will in
        /// fact only hide the controls. Showing them is
        /// done in BeginEdit.
        ///
        /// \param show bool
        /// \param attr wxGridCellAttr*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void Show(bool show, wxGridCellAttr *attr = nullptr) override
        {
            if (!show)
            {
                // Only hide here
                m_text->Hide();
                m_checkBox->Hide();
                m_comboBox->Hide();
            }

            if (show)
            {
                wxColour colBg = attr ? attr->GetBackgroundColour() : *wxLIGHT_GREY;
                m_checkBox->SetBackgroundColour(colBg);
            }
        }

    protected:
        wxTextCtrl* m_text;
        wxCheckBox* m_checkBox;
        wxComboBox* m_comboBox;
        wxGrid* m_grid;
        wxString m_value;
        bool m_finished;

        /////////////////////////////////////////////////
        /// \brief Set the size and position of the text
        /// control and the combo box.
        ///
        /// \param _rect const wxRect&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetTextCtrlSize(const wxRect& _rect)
        {
            wxRect rect(_rect);
            wxRect textRect(_rect);
            const wxSize bestSize = m_comboBox->GetBestSize();
            const wxCoord diffY = bestSize.GetHeight() - rect.GetHeight();

            if (diffY > 0)
            {
                // Do make it tall enough.
                rect.height += diffY;

                // Also centre the effective rectangle vertically with respect to the
                // original one.
                rect.y -= diffY/2;
            }

            // Make the edit control large enough to allow for internal margins
            //
            if (textRect.x == 0)
                textRect.x += 2;
            else
                textRect.x += 3;

            if (textRect.y == 0)
                textRect.y += 2;
            else
                textRect.y += 3;

            textRect.width -= 2;
            textRect.height -= 2;
            rect.width += 2;

            m_text->SetSize(textRect, wxSIZE_ALLOW_MINUS_ONE);
            m_comboBox->SetSize(rect, wxSIZE_ALLOW_MINUS_ONE);
        }

        /////////////////////////////////////////////////
        /// \brief Set size and position of the check
        /// box.
        ///
        /// \param _rect const wxRect&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetCheckBoxSize(const wxRect& _rect)
        {
            bool resize = false;
            wxSize checkBoxSize = m_checkBox->GetSize();
            wxCoord minSize = wxMin(_rect.width, _rect.height);

            // check if the checkbox is not too big/small for this cell
            wxSize sizeBest = m_checkBox->GetBestSize();

            if (!(checkBoxSize == sizeBest))
            {
                // reset to default size if it had been made smaller
                checkBoxSize = sizeBest;
                resize = true;
            }

            if (checkBoxSize.x >= minSize || checkBoxSize.y >= minSize)
            {
                // leave 1 pixel margin
                checkBoxSize.x = checkBoxSize.y = minSize - 2;
                resize = true;
            }

            if (resize)
                m_checkBox->SetSize(checkBoxSize);

            // position it in the centre of the rectangle (TODO: support alignment?)
            // here too, but in other way
            checkBoxSize.x -= 2;
            checkBoxSize.y -= 2;

            int hAlign = wxALIGN_CENTRE;
            int vAlign = wxALIGN_CENTRE;

            if (GetCellAttr())
                GetCellAttr()->GetAlignment(&hAlign, &vAlign);

            int x = 0, y = 0;

            if (hAlign == wxALIGN_LEFT)
            {
                x = _rect.x + 2;
                x += 2;
                y = _rect.y + _rect.height / 2 - checkBoxSize.y / 2;
            }
            else if (hAlign == wxALIGN_RIGHT)
            {
                x = _rect.x + _rect.width - checkBoxSize.x - 2;
                y = _rect.y + _rect.height / 2 - checkBoxSize.y / 2;
            }
            else if (hAlign == wxALIGN_CENTRE)
            {
                x = 1 + _rect.x + _rect.width / 2 - checkBoxSize.x / 2;
                y = _rect.y + _rect.height / 2 - checkBoxSize.y / 2;
            }

            m_checkBox->Move(x, y);
        }

        /////////////////////////////////////////////////
        /// \brief Respond to ENTER key events created by
        /// the combo box control.
        ///
        /// \param event wxCommandEvent&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void OnEnterKey(wxCommandEvent& event)
        {
           finalize(true);
        }

        /////////////////////////////////////////////////
        /// \brief Inform the grid to finalize the
        /// editing process.
        ///
        /// \param moveCursor bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        void finalize(bool moveCursor)
        {
            m_grid->SaveEditControlValue();
            m_grid->HideCellEditControl();

            if (moveCursor)
                m_grid->MoveCursorDown(false);

            m_grid->GetGridWindow()->SetFocus();
        }
};



#endif // CELLATTRIBUTES_HPP

