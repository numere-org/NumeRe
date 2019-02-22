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

// Constructor
GroupPanel::GroupPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : wxPanel(parent, id, pos, size, style)
{
    verticalSizer = nullptr;
    horizontalSizer = nullptr;

    // Create the horizontal and the vertical sizers
    horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    verticalSizer = new wxBoxSizer(wxVERTICAL);

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

// Member function to create a group (a static
// box) in the panel
wxStaticBoxSizer* GroupPanel::createGroup(const wxString& sGroupName, int orient)
{
    // Create a new static box sizer
    wxStaticBoxSizer* groupSizer = new wxStaticBoxSizer(orient, this, sGroupName);

    // Add the group to the main sizer
    verticalSizer->Add(groupSizer, 0, wxEXPAND | wxALL, 5);

    return groupSizer;
}

