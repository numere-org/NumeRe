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
#include "../NumeReWindow.h"
#include "../../common/Options.h"

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
TablePanel::TablePanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar, bool readOnly) : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC | wxTAB_TRAVERSAL)
{
    SyntaxStyles uiTheme = static_cast<NumeReWindow*>(parent->GetParent())->getOptions()->GetSyntaxStyle(Options::UI_THEME);
    statusbar->SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::STATUSBAR));
    SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::PANEL));

    finished = false;
    m_terminal = nullptr;
    m_infoBar = new wxInfoBar(this);

    wxBoxSizer* globalSizer = new wxBoxSizer(wxVERTICAL);
    globalSizer->Add(m_infoBar, 0, wxEXPAND);

    vsizer = new wxBoxSizer(wxVERTICAL);
    hsizer = new wxBoxSizer(wxHORIZONTAL);
    m_commentField = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1),
                                    wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH | wxBORDER_STATIC | (readOnly ? wxTE_READONLY : 0));
    m_sourceField = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_READONLY | wxBORDER_THEME);
    m_lastSaveText = new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_LASTSAVE"));

    grid = new TableViewer(this, wxID_ANY, statusbar, this, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
    grid->SetTableReadOnly(readOnly);
    grid->SetLabelBackgroundColour(uiTheme.foreground.ChangeLightness(Options::GRIDLABELS));

    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_DESCRIPTION")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_commentField, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_SOURCE")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_sourceField, 0, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(m_lastSaveText, 0, wxALIGN_LEFT, 0);

    hsizer->Add(vsizer, 0, wxEXPAND | wxALL, 5);
    hsizer->Add(grid, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    globalSizer->Add(hsizer, 1, wxEXPAND);

    SetSizer(globalSizer);
    PostSizeEvent();
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
/// \brief Show a message in the info bar.
///
/// \param msg const wxString&
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::showMessage(const wxString& msg)
{
    if (m_infoBar)
        m_infoBar->ShowMessage(msg);
}


/////////////////////////////////////////////////
/// \brief Dismiss the message in the info bar.
///
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::dismissMessage()
{
    if (m_infoBar)
        m_infoBar->Dismiss();
}


/////////////////////////////////////////////////
/// \brief Returns a pointer to the menu bar of
/// the parent frame. If no menu bar exists, a
/// new one is created.
///
/// \return wxMenuBar*
///
/////////////////////////////////////////////////
wxMenuBar* TablePanel::getMenuBar()
{
    wxMenuBar* menuBar = static_cast<wxFrame*>(m_parent)->GetMenuBar();

    if (!menuBar)
    {
        menuBar = new wxMenuBar();
        static_cast<wxFrame*>(m_parent)->SetMenuBar(menuBar);
    }

    return menuBar;
}


/////////////////////////////////////////////////
/// \brief Returns a pointer to the parent frame.
///
/// \return wxFrame*
///
/////////////////////////////////////////////////
wxFrame* TablePanel::getFrame()
{
    return static_cast<wxFrame*>(m_parent);
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
    grid->finalize();

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
    grid->finalize();
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
    grid->finalize();
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
    grid->finalize();

    if (!finished)
    {
        m_terminal->cancelTableEdit();
        finished = true;
        m_parent->Close();
        event.Skip();
    }

}


