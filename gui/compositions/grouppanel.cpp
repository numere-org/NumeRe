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

#include "grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;
#define ELEMENT_BORDER 5

// Constructor
GroupPanel::GroupPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : wxScrolledWindow(parent, id, pos, size, style | wxVSCROLL)
{
    verticalSizer = nullptr;
    horizontalSizer = nullptr;

    // Create the horizontal and the vertical sizers
    horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    verticalSizer = new wxBoxSizer(wxVERTICAL);

    // Enable a scrollbar, if it is needed. The
    // scrolling units are 20px per scroll
    //SetScrollbars(0, 20, 0, 100);

    // Set the horizontal sizer as main
    // sizer for the panel
    this->SetSizer(horizontalSizer);

    // Add the vertical sizer as a subsizer
    // of the horzizontal sizer
    horizontalSizer->Add(verticalSizer, 1, wxALIGN_TOP | wxALL, 0);
}

// Return the pointer to the vertical sizer
wxBoxSizer* GroupPanel::getVerticalSizer()
{
    return verticalSizer;
}

// Return the pointer to the horizontal sizer
wxBoxSizer* GroupPanel::getHorizontalSizer()
{
    return horizontalSizer;
}

// Add extra space between the last added (main) element
// and the next element to be added.
void GroupPanel::AddSpacer(int nSize)
{
    verticalSizer->AddSpacer(nSize);
}

// Member function to create a group (a static
// box) in the panel
wxStaticBoxSizer* GroupPanel::createGroup(const wxString& sGroupName, int orient)
{
    // Create a new static box sizer
    wxStaticBoxSizer* groupSizer = new wxStaticBoxSizer(orient, this, sGroupName);

    // Add the group to the main sizer
    verticalSizer->Add(groupSizer, 0, wxEXPAND | wxALL, ELEMENT_BORDER);

    return groupSizer;
}

// This member function creates the
// layout for a path input dialog including
// the "choose" button
wxTextCtrl* GroupPanel::CreatePathInput(wxWindow* parent, wxSizer* sizer, const wxString& description, int buttonID)
{
    // Create the text above the input line
    wxStaticText* inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(inputStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, ELEMENT_BORDER);

    // Create a horizontal sizer for the input
    // line and the buttoon
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(hSizer, wxALIGN_LEFT);

    // Create the input line
    wxTextCtrl* textCtrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(310, -1), wxTE_PROCESS_ENTER);

    // Create the button
    wxButton* button = new wxButton(parent, buttonID, _guilang.get("GUI_OPTIONS_CHOOSE"));

    // Add both to the horizontal sizer
    hSizer->Add(textCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);
    hSizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);

    return textCtrl;
}

// This member function creates the
// layout for a text input
wxTextCtrl* GroupPanel::CreateTextInput(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxString& sDefault, int nStyle)
{
    // Create the text above the input line
    wxStaticText* inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(inputStaticText, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);

    // Create the input line
    wxTextCtrl* textCtrl = new wxTextCtrl(parent, wxID_ANY, sDefault, wxDefaultPosition, wxSize(310, -1), nStyle);
    sizer->Add(textCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);

    return textCtrl;
}

// This member function creates the
// layout for a usual checkbox
wxCheckBox* GroupPanel::CreateCheckBox(wxWindow* parent, wxSizer* sizer, const wxString& description)
{
    // Create the checkbox and assign it to the passed sizer
    wxCheckBox* checkBox = new wxCheckBox(parent, wxID_ANY, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);

    return checkBox;
}

// This member function creates the
// layout for a spin control including the
// assigned text
wxSpinCtrl* GroupPanel::CreateSpinControl(wxWindow* parent, wxSizer* sizer, const wxString& description, int nMin, int nMax, int nInitial)
{
    // Create a horizontal sizer for the
    // spin control and its assigned text
    wxBoxSizer* spinCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(spinCtrlSizer, 0, wxALIGN_LEFT|wxALL, 0);

    // Create the spin control
    wxSpinCtrl* spinCtrl = new wxSpinCtrl(parent, wxID_ANY, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, nMin, nMax, nInitial);

    // Create the assigned static text
    wxStaticText* spinCtrlStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);

    // Add both to the horizontal sizer
    spinCtrlSizer->Add(spinCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);
    spinCtrlSizer->Add(spinCtrlStaticText, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);

    return spinCtrl;
}

// This member function creates the
// layout for a listview control
wxListView* GroupPanel::CreateListView(wxWindow* parent, wxSizer* sizer, int nStyle /*= wxLC_REPORT*/, wxSize size /*= wxDefaultSize*/)
{
    // Create the listview and assign it to the passed sizer
    wxListView* listView = new wxListView(parent, wxID_ANY, wxDefaultPosition, size, nStyle);
    sizer->Add(listView, 1, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);

    return listView;
}

