/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2013  Erik Haenel et al.

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

#include "listeditdialog.hpp"
#include "../compositions/grouppanel.hpp"

BEGIN_EVENT_TABLE(ListEditDialog, wxDialog)
    EVT_LIST_ITEM_ACTIVATED(-1, ListEditDialog::OnLabelEdit)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief ListEditDialog constructor
///
/// \param parent wxWindow*
/// \param listEntries const wxArrayString&
/// \param title const wxString&
/// \param desc const wxString&
///
/////////////////////////////////////////////////
ListEditDialog::ListEditDialog(wxWindow* parent, const wxArrayString& listEntries, const wxString& title, const wxString& desc) : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP)
{
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    m_list = panel->CreateListView(panel, panel->getMainSizer(), wxLC_REPORT | wxLC_EDIT_LABELS);

    wxBoxSizer* buttonSizer = panel->createGroup(wxHORIZONTAL);
    panel->CreateButton(panel, buttonSizer, "OK", wxID_OK);
    panel->CreateButton(panel, buttonSizer, "Cancel", wxID_CANCEL);

    m_list->AppendColumn(desc, wxLIST_FORMAT_LEFT, GetSize().x-30);

    for (size_t i = 0; i < listEntries.size(); i++)
    {
        m_list->InsertItem(i, listEntries[i]);
    }

    CenterOnScreen();
}


/////////////////////////////////////////////////
/// \brief Get the edited entries.
///
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString ListEditDialog::getEntries() const
{
    wxArrayString listEntries;

    for (int i = 0; i < m_list->GetItemCount(); i++)
    {
        listEntries.Add(m_list->GetItemText(i));
    }

    return listEntries;
}


/////////////////////////////////////////////////
/// \brief Event handler starting the edit
/// process if the user double clicks on a label
/// in the list.
///
/// \param event wxListEvent&
/// \return void
///
/////////////////////////////////////////////////
void ListEditDialog::OnLabelEdit(wxListEvent& event)
{
    m_list->EditLabel(event.GetItem().GetId());
}

