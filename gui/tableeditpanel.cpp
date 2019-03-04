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

#include "tableeditpanel.hpp"
#include "../kernel/core/datamanagement/container.hpp"

#define ID_TABLEEDIT_OK 10101
#define ID_TABLEEDIT_CANCEL 10102

BEGIN_EVENT_TABLE(TableEditPanel, wxPanel)
    EVT_BUTTON(ID_TABLEEDIT_OK, TableEditPanel::OnButtonOk)
    EVT_BUTTON(ID_TABLEEDIT_CANCEL, TableEditPanel::OnButtonCancel)
    EVT_CLOSE (TableEditPanel::OnClose)
END_EVENT_TABLE()

// creates the controls
TableEditPanel::TableEditPanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar) : wxPanel(parent, id)
{
    finished = false;
    vsizer = new wxBoxSizer(wxVERTICAL);
    hsizer = new wxBoxSizer(wxHORIZONTAL);

    grid = new TableViewer(this, wxID_ANY, statusbar, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);

    wxButton* button_ok = new wxButton(this, ID_TABLEEDIT_OK, _guilang.get("GUI_OPTIONS_OK"));
    wxButton* button_cancel = new wxButton(this, ID_TABLEEDIT_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"));

    hsizer->Add(button_ok, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    hsizer->Add(button_cancel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    vsizer->Add(hsizer, 0, wxALIGN_LEFT, 5);
    vsizer->Add(grid, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);
    this->SetSizer(vsizer);
}

void TableEditPanel::OnButtonOk(wxCommandEvent& event)
{
    m_terminal->passEditedTable(grid->GetData());
    finished = true;
    m_parent->Close();
}

void TableEditPanel::OnButtonCancel(wxCommandEvent& event)
{
    m_terminal->cancelTableEdit();
    finished = true;
    m_parent->Close();
}

void TableEditPanel::OnClose(wxCloseEvent& event)
{
    if (!finished)
    {
        m_terminal->cancelTableEdit();
        finished = true;
    }
    m_parent->Destroy();
    event.Skip();
}

/*void TableEditPanel::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
    else
        event.Skip();
}*/

