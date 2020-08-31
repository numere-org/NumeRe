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

BEGIN_EVENT_TABLE(SearchCtrl, wxComboBox)
    EVT_COMBOBOX(-1, SearchCtrl::OnItemSelect)
    EVT_TEXT(-1, SearchCtrl::OnTextChange)
    EVT_COMBOBOX_DROPDOWN(-1, SearchCtrl::OnPopup)
END_EVENT_TABLE()

void SearchCtrl::selectItem(const wxString& value)
{
    // Do nothing in this implementation, because we have no
    // container assigned
}

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

    if (value.length())
    {
        selectItem(value);
        Clear();
        SetCursor(wxCURSOR_IBEAM);
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
            Set(candidates);
            SetValue(textEntryValue);
            SetInsertionPointEnd();

            SetCursor(wxCURSOR_ARROW);
            Popup();
        }

        textChangeMutex = false;
    }
}


/////////////////////////////////////////////////
/// \brief This event handling function will be
/// fired, if the dropdown list is opened. It
/// will simply remove the automatic selection of
/// the control to avoid overtyping.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void SearchCtrl::OnPopup(wxCommandEvent& event)
{
    textChangeMutex = true;
    SetInsertionPointEnd();
    SelectNone();
    SetCursor(wxCURSOR_ARROW);
    textChangeMutex = false;
}

