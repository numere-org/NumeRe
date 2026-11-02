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
#include "grouppanel.hpp"
#include "../../kernel/core/datamanagement/container.hpp"
#include "../NumeReWindow.h"
#include "../../common/Options.h"
#include "../stringconv.hpp"

#define ID_TABLEEDIT_OK 10101
#define ID_TABLEEDIT_CANCEL 10102


BEGIN_EVENT_TABLE(TablePanel, wxPanel)
    EVT_CLOSE (TablePanel::OnClose)
    EVT_AUINOTEBOOK_PAGE_CHANGED(-1, TablePanel::OnSheetChange)
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
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param statusbar wxStatusBar*
/// \param topWindow NumeReWindow*
/// \param readOnly bool
///
/////////////////////////////////////////////////
TablePanel::TablePanel(wxWindow* parent, wxFrame* topLevel, wxWindowID id, wxStatusBar* statusbar, NumeReWindow* topWindow, bool readOnly) : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC | wxTAB_TRAVERSAL)
{
    m_topWindow = topWindow;
    m_topLevelFrame = topLevel;
    m_statusBar = statusbar;
    SyntaxStyles uiTheme = topWindow->getOptions()->GetSyntaxStyle(Options::UI_THEME);
    statusbar->SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::STATUSBAR));
    SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::PANEL));

    finished = false;
    m_terminal = nullptr;
    m_infoBar = new wxInfoBar(this);

    wxBoxSizer* globalSizer = new wxBoxSizer(wxVERTICAL);
    globalSizer->Add(m_infoBar, 0, wxEXPAND);

    vsizer = new wxBoxSizer(wxVERTICAL);
    hsizer = new wxBoxSizer(wxHORIZONTAL);
    m_commentField = new TextField(this, wxID_ANY, wxEmptyString, wxSize(200, -1),
                                   wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH2 | wxBORDER_STATIC | wxTE_AUTO_URL | (readOnly ? wxTE_READONLY : 0));
    m_sourceField = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_READONLY | wxBORDER_THEME);
    m_lastSaveText = new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_LASTSAVE"));

    m_sheets = new ViewerBook(this,
                              wxID_ANY,
                              uiTheme.foreground.ChangeLightness(Options::PANEL),
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_WINDOWLIST_BUTTON);

    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_DESCRIPTION")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_commentField, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(new wxStaticText(this, wxID_ANY, _guilang.get("GUI_TABLEPANEL_SOURCE")), 0, wxALIGN_LEFT | wxBOTTOM, 5);
    vsizer->Add(m_sourceField, 0, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxBOTTOM, 5);
    vsizer->Add(m_lastSaveText, 0, wxALIGN_LEFT, 0);

    hsizer->Add(vsizer, 0, wxEXPAND | wxALL, 5);
    hsizer->Add(m_sheets, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 0);

    globalSizer->Add(hsizer, 1, wxEXPAND);

    SetSizer(globalSizer);
    PostSizeEvent();
}


/////////////////////////////////////////////////
/// \brief Show a TableView instance.
///
/// \param _table NumeRe::TableView&
/// \param sName const std::string&
/// \param sIntName const std::string&
/// \param readOnly bool
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::show(NumeRe::TableView& _table, const std::string& sName, const std::string& sIntName, bool readOnly)
{
    SyntaxStyles uiTheme = m_topWindow->getOptions()->GetSyntaxStyle(Options::UI_THEME);

    for (size_t k = 0; k < _table.getSheets(); k++)
    {
        TableViewer* grid = new TableViewer(m_sheets, wxID_ANY, m_statusBar, this, m_topWindow,
                                            wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
        grid->SetTableReadOnly(readOnly);
        grid->SetLabelBackgroundColour(uiTheme.foreground.ChangeLightness(Options::GRIDLABELS));
        grid->SetData(_table, k, sName, sIntName);

        m_sheets->AddPage(grid, toString(k+1) + ": " + wxFromUtf8(_table.getTableData().getSheet(k).m_sheetName));
    }

    m_tableDisplayName = sName;
    m_sheets->SetSelection(0);
}


/////////////////////////////////////////////////
/// \brief Show a container.
///
/// \param _table NumeRe::Container<std::string>&
/// \param sName const std::string&
/// \param sIntName const std::string&
/// \param readOnly bool
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::show(NumeRe::Container<std::string>& _table, const std::string& sName, const std::string& sIntName, bool readOnly)
{
    SyntaxStyles uiTheme = m_topWindow->getOptions()->GetSyntaxStyle(Options::UI_THEME);

    TableViewer* grid = new TableViewer(m_sheets, wxID_ANY, m_statusBar, this, m_topWindow,
                                        wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
    grid->SetTableReadOnly(readOnly);
    grid->SetLabelBackgroundColour(uiTheme.foreground.ChangeLightness(Options::GRIDLABELS));
    grid->SetData(_table, sName, sIntName);

    m_sheets->AddPage(grid, "1: Sheet_1");

    m_tableDisplayName = sName;
    m_sheets->SetSelection(0);
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
    m_commentField->SetMarkupText(wxFromUtf8(ensureValidUtf8(meta.comment)));
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
    return wxToUtf8(m_commentField->GetValue());
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
    wxMenuBar* menuBar = m_topLevelFrame->GetMenuBar();

    if (!menuBar)
    {
        menuBar = new wxMenuBar();
        m_topLevelFrame->SetMenuBar(menuBar);
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
    return m_topLevelFrame;
}


/////////////////////////////////////////////////
/// \brief Indicate that the table panel is ready.
///
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::ready()
{
    m_commentField->ShowPosition(0);
    Update();
    Refresh();
}


/////////////////////////////////////////////////
/// \brief Returns the currently displayed
/// TableViewer instance.
///
/// \return TableViewer*
///
/////////////////////////////////////////////////
TableViewer* TablePanel::getCurrentPage()
{
    return static_cast<TableViewer*>(m_sheets->GetPage(m_sheets->GetSelection()));
}


/////////////////////////////////////////////////
/// \brief Returns a combination of the table
/// name and the name of the currently visible
/// table sheet to be used within the window
/// title.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString TablePanel::getTableAndSheetName()
{
    if (!m_sheets->GetPageCount())
        return m_tableDisplayName;

    return m_tableDisplayName + "  [" + m_sheets->GetPageText(m_sheets->GetSelection()) + "]";
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
    getCurrentPage()->finalize();

    if (!finished)
    {
        finished = true;
        m_parent->Close();
        event.Skip();
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for the sheet change
/// event.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void TablePanel::OnSheetChange(wxAuiNotebookEvent& event)
{
    if (!m_statusBar)
        return;

    m_topLevelFrame->SetTitle("NumeRe-DataViewer: " + getTableAndSheetName());
    getCurrentPage()->refreshStatusBar();
}





/////////////////////////////////////////////////
/// \brief Constructor for the table edit panel,
/// which creates also the buttons at the top of
/// the window.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param statusbar wxStatusBar*
/// \param topWindow NumeReWindow*
///
/////////////////////////////////////////////////
TableEditPanel::TableEditPanel(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, NumeReWindow* topWindow) : TablePanel(parent, static_cast<wxFrame*>(parent), id, statusbar, topWindow, false)
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
    getCurrentPage()->finalize();
    m_terminal->passEditedTable(getCurrentPage()->GetData());
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
    getCurrentPage()->finalize();
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
    getCurrentPage()->finalize();

    if (!finished)
    {
        m_terminal->cancelTableEdit();
        finished = true;
        m_parent->Close();
        event.Skip();
    }

}


