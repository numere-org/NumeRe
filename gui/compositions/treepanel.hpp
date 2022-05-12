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

#ifndef TREEPANEL_HPP
#define TREEPANEL_HPP

#include <wx/wx.h>

// This class specializes the generic search control
// to interact with a wxTreeCtrl
class TreePanel : public wxPanel
{
    public:
        TreePanel(wxWindow* parent, wxWindowID id) : wxPanel(parent, id) {}

        void AddWindows(wxWindow* searchbar, wxWindow* tree)
        {
            wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
            wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

            vsizer->Add(searchbar, 0, wxEXPAND, 0);
            vsizer->AddSpacer(2);
            vsizer->Add(tree, 1, wxEXPAND, 0);

            hsizer->Add(vsizer, 1, wxEXPAND, 0);
            SetSizer(hsizer);
        }
};


#endif // TREEPANEL_HPP


