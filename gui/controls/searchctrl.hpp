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

#ifndef SEARCHCTRL_HPP
#define SEARCHCTRL_HPP

#include <wx/wx.h>

// Implementation of a generic search control based
// upon a combo box
class SearchCtrl : public wxComboBox
{
    private:
        bool textChangeMutex;
        wxString textEntryValue;

    protected:
        // Virtual functions to interact with the data
        // model
        virtual void selectItem(const wxString& value);
        virtual wxArrayString getCandidates(const wxString& enteredText);

    public:
        SearchCtrl(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString) : wxComboBox(parent, id, value, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_SORT), textChangeMutex(false) {}

        // Event handler functions
        void OnItemSelect(wxCommandEvent& event);
        void OnTextChange(wxCommandEvent& event);
        void OnPopup(wxCommandEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // SEARCHCTRL_HPP

