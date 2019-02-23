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

#include "renamesymbolsdialog.hpp"
#include "grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;

RenameSymbolsDialog::RenameSymbolsDialog(wxWindow* parent, wxWindowID id, const wxString& title, const wxString& defaultval) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER | wxSTAY_ON_TOP)
{
    m_replaceInComments = nullptr;
    m_replaceInWholeFile = nullptr;
    m_replaceBeforeCursor = nullptr;
    m_replaceAfterCursor = nullptr;
    m_replaceName = nullptr;

    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vSizer);

    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxSize(350, 200));
    vSizer->Add(panel, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    m_replaceName = panel->CreateTextInput(panel, panel->getVerticalSizer(), _guilang.get("GUI_DLG_RENAMESYMBOLS_QUESTION"), defaultval);

    panel->AddSpacer();

    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_DLG_RENAMESYMBOLS_SETTINGS"));

    m_replaceInComments = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACECOMMENTS"));
    m_replaceInComments->SetValue(false);

    m_replaceInWholeFile = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEWHOLEFILE"));
    m_replaceInWholeFile->SetValue(false);

    m_replaceBeforeCursor = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEBEFOREPOS"));
    m_replaceBeforeCursor->SetValue(true);

    m_replaceAfterCursor = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEAFTERPOS"));
    m_replaceAfterCursor->SetValue(true);

    vSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    GetSizer()->SetSizeHints(this);

    this->Centre();
}

