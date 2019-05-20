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

#include "textoutputdialog.hpp"


TextOutputDialog::TextOutputDialog(wxWindow* parent, wxWindowID id, const wxString& title, long style) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, style)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    m_textctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    vsizer->Add(m_textctrl, 1, wxEXPAND | wxALL, 5);

    vsizer->Add(CreateButtonSizer(wxOK), 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    SetSizer(vsizer);
}

void TextOutputDialog::push_text(const wxString& text)
{
    m_textctrl->AppendText(text);
}

