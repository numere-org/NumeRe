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

#include "searchctrl.hpp"
#include <wx/dnd.h>

#define HIGHLIGHTCOLOR wxColour(0,0,139)


wxBEGIN_EVENT_TABLE(SearchCtrlPopup, wxListView)
    EVT_LEFT_UP(SearchCtrlPopup::OnMouseUp)
    EVT_MOTION(SearchCtrlPopup::OnMouseMove)
    EVT_KEY_DOWN(SearchCtrlPopup::OnKeyEvent)
    EVT_LIST_BEGIN_DRAG(-1, SearchCtrlPopup::OnDragStart)
wxEND_EVENT_TABLE()

/////////////////////////////////////////////////
/// \brief This private member function splits
/// the passed string at every '~' character in
/// single strings, which can be used to fill the
/// columns of this control.
///
/// \param sString wxString
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString SearchCtrlPopup::split(wxString sString)
{
    wxArrayString ret;

    // Append a '~' char, which makes the
    // splitting more easy
    sString += '~';

    // Split the string in single strings
    do
    {
        ret.Add(sString.substr(0, sString.find('~')));
        sString.erase(0, sString.find('~')+1);
    }
    while (sString.length());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Returns the string representation of
/// the selected item after the popup had been
/// closed.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString SearchCtrlPopup::GetStringValue() const
{
    if (m_ListId >= 0 && m_ListId < GetItemCount())
        return wxListView::GetItemText(m_ListId);

    return wxEmptyString;
}


/////////////////////////////////////////////////
/// \brief This event handler is necessary to
/// pass the keycodes, which are accidentally
/// catched in this control, to the actual text
/// entry of the composed control.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrlPopup::OnKeyEvent(wxKeyEvent& event)
{
    wxChar chr = event.GetUnicodeKey();

    //wxLogDebug("Key: %d", chr);

    // Uppercase
    if (chr != WXK_NONE)
    {
        if (event.ControlDown() && event.AltDown())
        {
            switch (chr)
            {
                case 'Q':
                    m_combo->WriteText('@');
                    break;
                case '+':
                    m_combo->WriteText('~');
                    break;
                case '7':
                    m_combo->WriteText('{');
                    break;
                case '8':
                    m_combo->WriteText('[');
                    break;
                case '9':
                    m_combo->WriteText(']');
                    break;
                case '0':
                    m_combo->WriteText('}');
                    break;
                case 223:
                    m_combo->WriteText('\\');
                    break;
                case '<':
                    m_combo->WriteText('|');
                    break;
            }
        }
        else if (chr >='A' && chr < WXK_DELETE)
        {
            if (event.ShiftDown())
                m_combo->WriteText(chr);
            else
                m_combo->WriteText(wxChar(chr + 'a'-'A'));
        }
        else if (chr < 32 || chr == WXK_DELETE)
        {
            switch (chr)
            {
                case WXK_BACK:
                    m_combo->Remove(m_combo->GetInsertionPoint()-1, m_combo->GetInsertionPoint());
                    break;
                case WXK_DELETE:
                    m_combo->Remove(m_combo->GetInsertionPoint(), m_combo->GetInsertionPoint()+1);
                    break;
                case WXK_ESCAPE:
                    Dismiss();
                    break;
                case WXK_RETURN:
                    if (GetItemCount() && !GetFirstSelected())
                        m_ListId = 0;
                    else if (GetFirstSelected())
                        m_ListId = GetFirstSelected();

                    Dismiss();
                    break;
            }
        }
        else
        {
            if (!event.ShiftDown())
            {
                switch (chr)
                {
                    case 246: // ö
                        m_combo->WriteText((char)0xF6);
                        break;
                    case 228: // ä
                        m_combo->WriteText((char)0xE4);
                        break;
                    case 252: // ü
                        m_combo->WriteText((char)0xFC);
                        break;
                    case 223: // ß
                        m_combo->WriteText((char)0xDF);
                        break;
                    default:
                        m_combo->WriteText(chr);
                }
            }
            else
            {
                switch (chr)
                {
                    case '-':
                        m_combo->WriteText('_');
                        break;
                    case '.':
                        m_combo->WriteText(':');
                        break;
                    case ',':
                        m_combo->WriteText(';');
                        break;
                    case '<':
                        m_combo->WriteText('>');
                        break;
                    case '#':
                        m_combo->WriteText('\'');
                        break;
                    case '+':
                        m_combo->WriteText('*');
                        break;
                    case '1':
                        m_combo->WriteText('!');
                        break;
                    case '2':
                        m_combo->WriteText('"');
                        break;
                    case '3':
                        m_combo->WriteText('§');
                        break;
                    case '4':
                        m_combo->WriteText('$');
                        break;
                    case '5':
                        m_combo->WriteText('%');
                        break;
                    case '6':
                        m_combo->WriteText('&');
                        break;
                    case '7':
                        m_combo->WriteText('/');
                        break;
                    case '8':
                        m_combo->WriteText('(');
                        break;
                    case '9':
                        m_combo->WriteText(')');
                        break;
                    case '0':
                        m_combo->WriteText('=');
                        break;
                    case 246: // ö
                        m_combo->WriteText((char)0xD6);
                        break;
                    case 228: // ä
                        m_combo->WriteText((char)0xC4);
                        break;
                    case 252: // ü
                        m_combo->WriteText((char)0xDC);
                        break;
                    case 223: // ß
                        m_combo->WriteText('?');
                        break;
                }
            }
        }
    }
    else
    {
        switch (event.GetKeyCode())
        {
            case WXK_UP:
                if (!GetSelectedItemCount() || !GetFirstSelected())
                    Select(GetItemCount()-1);
                else
                    Select(GetFirstSelected()-1);

                EnsureVisible(GetFirstSelected());
                break;
            case WXK_LEFT:
                m_combo->SetInsertionPoint(m_combo->GetInsertionPoint()-1);
                break;
            case WXK_DOWN:
                if (!GetSelectedItemCount() || GetFirstSelected() == GetItemCount()-1)
                    Select(0);
                else
                    Select(GetFirstSelected()+1);

                EnsureVisible(GetFirstSelected());
                break;
            case WXK_RIGHT:
                m_combo->SetInsertionPoint(m_combo->GetInsertionPoint()+1);
                break;
            case WXK_HOME:
                m_combo->SetInsertionPoint(0);
                break;
            case WXK_END:
                m_combo->SetInsertionPointEnd();
                break;
            case WXK_NUMPAD_DIVIDE:
                m_combo->WriteText('/');
                break;
            case WXK_NUMPAD_MULTIPLY:
                m_combo->WriteText('*');
                break;
            case WXK_NUMPAD_ADD:
                m_combo->WriteText('+');
                break;
            case WXK_NUMPAD_SUBTRACT:
                m_combo->WriteText('-');
                break;
        }
    }

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This event handler fires, if the user
/// double clicks on an item in the popup list.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrlPopup::OnMouseUp(wxMouseEvent& WXUNUSED(event))
{
    m_ListId = wxListView::GetFirstSelected();
    Dismiss();
}


/////////////////////////////////////////////////
/// \brief This event handler provides the mouse
/// hover effect in the popup list.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrlPopup::OnMouseMove(wxMouseEvent& event)
{
    int flags = 0;
    int id = HitTest(event.GetPosition(), flags, nullptr);

    if (id != wxNOT_FOUND && (flags & wxLIST_HITTEST_ONITEM) && wxListView::GetFirstSelected() != id)
    {
        wxListView::Select(id);

        if (m_enableColors && m_callTipHighlight.length() && GetItemTextColour(id) == HIGHLIGHTCOLOR)
            SetToolTip(m_callTipHighlight);
        else if (m_callTip.length())
            SetToolTip(m_callTip);
    }
}


/////////////////////////////////////////////////
/// \brief This event handler is the drag-drop
/// handler for the search results.
///
/// \param event wxListEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrlPopup::OnDragStart(wxListEvent& event)
{
    wxString token = static_cast<SearchCtrl*>(m_combo)->getDragDropText(event.GetText());

    if (!m_enableDragDrop)
    {
        event.Veto();
        return;
    }

    m_combo->Clear();
    wxTextDataObject _dataObject(token);
    wxDropSource dragSource(this);
    dragSource.SetData(_dataObject);
    dragSource.DoDragDrop(wxDrag_AllowMove);

    m_ListId = wxNOT_FOUND;
    Dismiss();
}


/////////////////////////////////////////////////
/// \brief A set function to update the contents
/// in the displayed list.
///
/// \param stringArray wxArrayString&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrlPopup::Set(wxArrayString& stringArray)
{
    // Clear everything
    ClearAll();

    // Create the columns
    for (size_t i = 0; i < m_sizes.size(); i++)
        AppendColumn(wxEmptyString, wxLIST_FORMAT_LEFT, m_sizes[i]);

    // Fill the lines with the passed strings,
    // either in single- or multi-column fashion
    for (size_t i = 0; i < stringArray.size(); i++)
    {
        if (m_sizes.size() == 1)
            InsertItem(i, stringArray[i]);
        else
        {
            wxArrayString strings = split(stringArray[i]);
            InsertItem(i, strings[0]);

            for (size_t j = 1; j < std::min(m_sizes.size(), strings.size()); j++)
                SetItem(i, j, strings[j]);

            if (m_enableColors && strings[0].find('(') != std::string::npos)
                SetItemTextColour(i, HIGHLIGHTCOLOR);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the id of the selected element
/// (if the user selected something). wxNOT_FOUND
/// otherwise.
///
/// \return int
///
/////////////////////////////////////////////////
int SearchCtrlPopup::GetSelectedId() const
{
    return m_ListId;
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////


BEGIN_EVENT_TABLE(SearchCtrl, wxComboCtrl)
    EVT_COMBOBOX_CLOSEUP(-1, SearchCtrl::OnItemSelect)
    EVT_TEXT(-1, SearchCtrl::OnTextChange)
    EVT_ENTER_WINDOW(SearchCtrl::OnMouseEnter)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Child classes may override this member
/// function to do something, when the user
/// selects an item in the popup.
///
/// \param value const wxString&
/// \return bool true on success, false otherwise.
///
/////////////////////////////////////////////////
bool SearchCtrl::selectItem(const wxString& value)
{
    // Do nothing in this implementation, because we have no
    // container assigned
    return true;
}


/////////////////////////////////////////////////
/// \brief Child classes may override this member
/// function to provide a custom string for the
/// built-in drag-drop functionality.
///
/// \param value const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString SearchCtrl::getDragDropText(const wxString& value)
{
    return value;
}


/////////////////////////////////////////////////
/// \brief Child classes must override this
/// member function to provide the candidates to
/// be filled into the popup based upon the
/// letters entered in the text entry window.
///
/// \param enteredText const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString SearchCtrl::getCandidates(const wxString& enteredText)
{
    // Will only return a copy of the entered text
    wxArrayString stringArray(1, &enteredText);

    return stringArray;
}


/////////////////////////////////////////////////
/// \brief This event handler function will be
/// fired, if the user selects a proposed string
/// in the opened dropdown list.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrl::OnItemSelect(wxCommandEvent& event)
{
    wxString value = GetValue();

    if (value.length() && popUp->GetSelectedId() != wxNOT_FOUND)
    {
        if (selectItem(value))
            Clear();
    }
}


/////////////////////////////////////////////////
/// \brief This event handler function will be
/// fired, if the user changes the text in the
/// control. It will provide a dropdown menu with
/// possible candidates matching to the provided
/// search string.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrl::OnTextChange(wxCommandEvent& event)
{
    textEntryValue = GetValue();

    if (textEntryValue.length() > 2 && !textChangeMutex)
    {
        textChangeMutex = true;

        // Get the candidates matching to the entered text
        wxArrayString candidates = getCandidates(textEntryValue);

        // If the candidates exist, enter them in the
        // list part and show it
        if (candidates.size())
        {
            int insertionPoint = GetInsertionPoint();
            popUp->Set(candidates);
            SetValue(textEntryValue);

            if (IsPopupWindowState(wxComboCtrl::Hidden))
                Popup();

            SelectNone();
            SetInsertionPoint(insertionPoint);
        }

        textChangeMutex = false;
    }
}


/////////////////////////////////////////////////
/// \brief This event handler will show the drop
/// down list, if the text field contains data
/// and the mouse enters the text field.
/// Replacement for the buggy EVT_SET_FOCUS
/// event.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrl::OnMouseEnter(wxMouseEvent& event)
{
    if (GetValue().length() > 2 && IsPopupWindowState(wxComboCtrl::Hidden))
        Popup();

    SelectNone();
    SetInsertionPointEnd();
}

