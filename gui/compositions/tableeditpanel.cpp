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
#include "../../kernel/core/datamanagement/container.hpp"

#define ID_TABLEEDIT_OK 10101
#define ID_TABLEEDIT_CANCEL 10102


BEGIN_EVENT_TABLE(TablePanel, wxPanel)
    EVT_CLOSE (TablePanel::OnClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(TableEditPanel, TablePanel)
    EVT_BUTTON(ID_TABLEEDIT_OK, TableEditPanel::OnButtonOk)
    EVT_BUTTON(ID_TABLEEDIT_CANCEL, TableEditPanel::OnButtonCancel)
    EVT_CLOSE (TableEditPanel::OnClose)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor for the generic table
/// panel.
///
/// \param parent wxFrame*
/// \param id wxWindowID
/// \param statusbar wxStatusBar*
/// \param readOnly bool
///
/////////////////////////////////////////////////
TablePanel::TablePanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar, bool readOnly) : wxPanel(parent, id)
{
    finished = false;

    vsizer = new wxBoxSizer(wxVERTICAL);
    hsizer = new wxBoxSizer(wxHORIZONTAL);
    m_commentField = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1),
                                    wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH | wxBORDER_STATIC | (readOnly ? wxTE_READONLY : 0));
    m_sourceField = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_READONLY | wxBORDER_THEME);
    m_lastSaveText = new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_LASTSAVE"));

    grid = new TableViewer(this, wxID_ANY, statusbar, this, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
    grid->SetTableReadOnly(readOnly);

    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_DESCRIPTION")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_commentField, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_SOURCE")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_sourceField, 0, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(m_lastSaveText, 0, wxALIGN_LEFT, 0);

    hsizer->Add(vsizer, 0, wxEXPAND | wxALL, 5);
    hsizer->Add(grid, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);

    this->SetSizer(hsizer);
}


/////////////////////////////////////////////////
/// \brief Update the panel with the passed table
/// meta data.
///
/// \param meta const NumeRe::TableMetaData&
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::update(const NumeRe::TableMetaData& meta)
{
    m_commentField->SetValue(meta.comment);
    m_sourceField->SetValue(meta.source);
    m_lastSaveText->SetLabel(_guilang.get("GUI_TABLEPANEL_LASTSAVE", toString(meta.lastSavedTime, GET_WITH_TEXT)));
}


/////////////////////////////////////////////////
/// \brief Returns the comment listed in the
/// documentation field.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string TablePanel::getComment() const
{
    return m_commentField->GetValue().ToStdString();
}


/////////////////////////////////////////////////
/// \brief Event handler for the CLOSE event.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::OnClose(wxCloseEvent& event)
{
    if (!finished)
    {
        finished = true;
        m_parent->Close();
        event.Skip();
    }
}





/////////////////////////////////////////////////
/// \brief Constructor for the table edit panel,
/// which creates also the buttons at the top of
/// the window.
///
/// \param parent wxFrame*
/// \param id wxWindowID
/// \param statusbar wxStatusBar*
///
/////////////////////////////////////////////////
TableEditPanel::TableEditPanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar) : TablePanel(parent, id, statusbar, false)
{
	m_terminal = nullptr;

    wxBoxSizer* buttonsizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* button_ok = new wxButton(this, ID_TABLEEDIT_OK, _guilang.get("GUI_OPTIONS_OK"));
    wxButton* button_cancel = new wxButton(this, ID_TABLEEDIT_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"));

    buttonsizer->Add(button_ok, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    buttonsizer->Add(button_cancel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT , 5);

    vsizer->Prepend(buttonsizer, 0, wxALIGN_LEFT | wxEXPAND | wxBOTTOM, 5);
    vsizer->Layout();
}


/////////////////////////////////////////////////
/// \brief Event handler for the OK button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableEditPanel::OnButtonOk(wxCommandEvent& event)
{
    m_terminal->passEditedTable(grid->GetData());
    finished = true;
    m_parent->Close();
}


/////////////////////////////////////////////////
/// \brief Event handler for the CANCEL button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableEditPanel::OnButtonCancel(wxCommandEvent& event)
{
    m_terminal->cancelTableEdit();
    finished = true;
    m_parent->Close();
}


/////////////////////////////////////////////////
/// \brief Event handler for the CLOSE event.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableEditPanel::OnClose(wxCloseEvent& event)
{
    if (!finished)
    {
        m_terminal->cancelTableEdit();
        finished = true;
        m_parent->Close();
        event.Skip();
    }
}


