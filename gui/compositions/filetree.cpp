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

#include "filetree.hpp"
#include "../globals.hpp"

BEGIN_EVENT_TABLE(FileTree, wxTreeCtrl)
//    EVT_ENTER_WINDOW    (FileTree::OnEnter)
END_EVENT_TABLE()

void FileTree::OnEnter(wxMouseEvent& event)
{
    if (g_findReplace != nullptr && g_findReplace->IsShown())
    {
        event.Skip();
        return;
    }
    this->SetFocus();
    event.Skip();
}

void FileTree::SetDnDHighlight(const wxTreeItemId& itemToHighLight)
{
    if (itemToHighLight == m_currentHighLight)
        return;

    if (m_currentHighLight.IsOk())
    {
        this->SetItemDropHighlight(m_currentHighLight, false);
        m_currentHighLight = wxTreeItemId();
    }
    if (itemToHighLight.IsOk())
    {
        this->SetItemDropHighlight(itemToHighLight);
        m_currentHighLight = itemToHighLight;
    }
}
