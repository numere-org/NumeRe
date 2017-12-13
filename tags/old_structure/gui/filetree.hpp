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

#ifndef FILETREE_HPP
#define FILETREE_HPP

#include <wx/wx.h>
#include <wx/treectrl.h>

class FileTree : public wxTreeCtrl
{
    public:
        FileTree(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_DEFAULT_STYLE, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTreeCtrlNameStr) : wxTreeCtrl(parent, id, pos, size, style, validator, name) {};

    private:
        void OnEnter(wxMouseEvent& event);

    DECLARE_EVENT_TABLE();
};

#endif // FILETREE_HPP

