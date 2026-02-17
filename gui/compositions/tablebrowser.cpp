/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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


#include "tablebrowser.hpp"
#include "tableeditpanel.hpp"
#include "tableviewer.hpp"
#include "../NumeReWindow.h"
#include "../guilang.hpp"


BEGIN_EVENT_TABLE(TableBrowser, ViewerFrame)
    //EVT_TREE_SEL_CHANGED(-1, DocumentationBrowser::OnTreeClick)
    //EVT_MENU_RANGE(EVENTID_HELP_START, EVENTID_HELP_END-1, DocumentationBrowser::OnToolbarEvent)
    EVT_AUINOTEBOOK_PAGE_CHANGED(-1, TableBrowser::onPageChange)
    EVT_MENU_RANGE              (TableViewer::ID_MENU_SAVE, TableViewer::ID_MENU_TABLE_END, TableBrowser::onMenuEvent)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Create a TableBrowser window.
///
/// \param parent wxWindow*
/// \param topWindow NumeReWindow*
///
/////////////////////////////////////////////////
TableBrowser::TableBrowser(wxWindow* parent, NumeReWindow* topWindow) : ViewerFrame(parent, "NumeRe-DataViewer: TableBrowser", topWindow->getOptions()->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0)
{
    m_topWindow = topWindow;
    SyntaxStyles uiTheme = topWindow->getOptions()->GetSyntaxStyle(Options::UI_THEME);

    m_tabs = new ViewerBook(this,
                            wxID_ANY,
                            uiTheme.foreground,
                            wxDefaultPosition,
                            wxDefaultSize,
                            wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_MIDDLE_CLICK_CLOSE | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_WINDOWLIST_BUTTON);

    m_tabs->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &TableBrowser::onPageClose, this);

    CreateStatusBar(3)->SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::STATUSBAR));
    SetBackgroundColour(uiTheme.foreground.ChangeLightness(Options::PANEL));

    createMenuBar();
    SetIcon(topWindow->getStandardIcon());
    SetSize(1280, 640);
    Show();
    SetFocus();
}


/////////////////////////////////////////////////
/// \brief Creates the menu bar for the table
/// browser.
///
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::createMenuBar()
{
    wxMenuBar* menuBar = new wxMenuBar();

    // Create the file menu
    wxMenu* menuFile = new wxMenu();

    menuFile->Append(TableViewer::ID_MENU_SAVE, _guilang.get("GUI_MENU_SAVEFILE"));
    menuFile->Append(TableViewer::ID_MENU_SAVE_AS, _guilang.get("GUI_MENU_SAVEFILEAS"));

    menuBar->Append(menuFile, _guilang.get("GUI_MENU_FILE"));

    // Create the edit menu
    wxMenu* menuEdit = new wxMenu();

    menuEdit->Append(TableViewer::ID_MENU_COPY, _guilang.get("GUI_COPY_TABLE_CONTENTS") + "\tCtrl-C");
    menuEdit->Append(TableViewer::ID_MENU_CUT, _guilang.get("GUI_CUT_TABLE_CONTENTS") + "\tCtrl-X");
    menuEdit->Append(TableViewer::ID_MENU_PASTE, _guilang.get("GUI_PASTE_TABLE_CONTENTS") + "\tCtrl-V");
    menuEdit->Append(TableViewer::ID_MENU_PASTE_HERE, _guilang.get("GUI_PASTE_TABLE_CONTENTS_HERE") + "\tCtrl-Shift-V");
    menuEdit->AppendSeparator();
    menuEdit->Append(TableViewer::ID_MENU_INSERT_ROW, _guilang.get("GUI_INSERT_TABLE_ROW"));
    menuEdit->Append(TableViewer::ID_MENU_INSERT_COL, _guilang.get("GUI_INSERT_TABLE_COL"));
    menuEdit->Append(TableViewer::ID_MENU_INSERT_CELL, _guilang.get("GUI_INSERT_TABLE_CELL"));
    menuEdit->AppendSeparator();
    menuEdit->Append(TableViewer::ID_MENU_REMOVE_ROW, _guilang.get("GUI_REMOVE_TABLE_ROW"));
    menuEdit->Append(TableViewer::ID_MENU_REMOVE_COL, _guilang.get("GUI_REMOVE_TABLE_COL"));
    menuEdit->Append(TableViewer::ID_MENU_REMOVE_CELL, _guilang.get("GUI_REMOVE_TABLE_CELL"));

    menuEdit->Enable(TableViewer::ID_MENU_CUT, false);
    menuEdit->Enable(TableViewer::ID_MENU_PASTE, false);
    menuEdit->Enable(TableViewer::ID_MENU_PASTE_HERE, false);
    menuEdit->Enable(TableViewer::ID_MENU_INSERT_ROW, false);
    menuEdit->Enable(TableViewer::ID_MENU_INSERT_COL, false);
    menuEdit->Enable(TableViewer::ID_MENU_INSERT_CELL, false);
    menuEdit->Enable(TableViewer::ID_MENU_REMOVE_ROW, false);
    menuEdit->Enable(TableViewer::ID_MENU_REMOVE_COL, false);
    menuEdit->Enable(TableViewer::ID_MENU_REMOVE_CELL, false);

    menuBar->Append(menuEdit, _guilang.get("GUI_MENU_EDIT"));

    // Create the columns submenu
    wxMenu* columnMenu = new wxMenu();

    columnMenu->Append(TableViewer::ID_MENU_SORT_COL_ASC, _guilang.get("GUI_TABLE_SORT_ASC"));
    columnMenu->Append(TableViewer::ID_MENU_SORT_COL_DESC, _guilang.get("GUI_TABLE_SORT_DESC"));
    columnMenu->Append(TableViewer::ID_MENU_SORT_COL_CLEAR, _guilang.get("GUI_TABLE_SORT_CLEAR"));
    columnMenu->AppendSeparator();
    columnMenu->Append(TableViewer::ID_MENU_FILTER, _guilang.get("GUI_TABLE_FILTER"));
    columnMenu->Append(TableViewer::ID_MENU_DELETE_FILTER, _guilang.get("GUI_TABLE_DELETE_FILTER"));
    columnMenu->AppendSeparator();
    columnMenu->Append(TableViewer::ID_MENU_CHANGE_COL_TYPE, _guilang.get("GUI_TABLE_CHANGE_COL_TYPE") + "\tCtrl-T");

    // Create the tools menu
    wxMenu* menuTools = new wxMenu();

    menuTools->Append(TableViewer::ID_MENU_RELOAD, _guilang.get("GUI_TABLE_RELOAD") + "\tCtrl-R");
    menuTools->Append(wxID_ANY, _guilang.get("GUI_TABLE_COLUMN"), columnMenu);
    menuTools->Append(TableViewer::ID_MENU_CVS, _guilang.get("GUI_TABLE_CVS") + "\tCtrl-Shift-F");

    menuBar->Append(menuTools, _guilang.get("GUI_MENU_TOOLS"));
    SetMenuBar(menuBar);
}


/////////////////////////////////////////////////
/// \brief Find the selected table on any of the
/// available open pages.
///
/// \param tableDisplayName const std::string&
/// \param sIntName const std::string&
/// \return int
///
/////////////////////////////////////////////////
int TableBrowser::findPage(const std::string& tableDisplayName, const std::string& sIntName)
{
    for (int i = 0; i < m_tabs->GetPageCount(); i++)
    {
        if (m_tabs->GetPageText(i) == wxFromUtf8(tableDisplayName))
        {
            std::string _sIntName;

            if (m_panelTypes[i] == TYPE_TABLEPANEL)
                _sIntName = static_cast<TablePanel*>(m_tabs->GetPage(i))->grid->GetInternalName();
            else
                _sIntName = static_cast<TableViewer*>(m_tabs->GetPage(i))->GetInternalName();

            if (_sIntName == sIntName)
                return i;
        }
    }

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of a cluster.
///
/// \param _stringTable NumeRe::Container<std::string>
/// \param tableDisplayName const std::string&
/// \param sIntName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::openTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName, const std::string& sIntName)
{
    int pageNum = findPage(tableDisplayName, sIntName);

    // If the page is already open
    if (pageNum != wxNOT_FOUND && m_panelTypes[pageNum] == TYPE_TABLEVIEWER)
    {
        static_cast<TableViewer*>(m_tabs->GetPage(pageNum))->SetData(_stringTable, tableDisplayName, sIntName);

        if (pageNum != m_tabs->GetSelection())
            m_tabs->ChangeSelection(pageNum);
        else
        {
            m_tabs->Refresh();
            m_tabs->Layout();
        }

        return;
    }

    TableViewer* grid = new TableViewer(m_tabs, wxID_ANY, GetStatusBar(), nullptr, m_topWindow,
                                        wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
    grid->SetData(_stringTable, tableDisplayName, sIntName);
    SyntaxStyles uiTheme = m_topWindow->getOptions()->GetSyntaxStyle(Options::UI_THEME);
    grid->SetLabelBackgroundColour(uiTheme.foreground.ChangeLightness(Options::GRIDLABELS));

    m_panelTypes.push_back(TYPE_TABLEVIEWER);
    m_tabs->AddPage(grid, wxFromUtf8(tableDisplayName), true);
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of a usual table.
///
/// \param _table NumeRe::Table
/// \param tableDisplayName const std::string&
/// \param sIntName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::openTable(NumeRe::Table _table, const std::string& tableDisplayName, const std::string& sIntName)
{
    int pageNum = findPage(tableDisplayName, sIntName);

    // If the page is already open
    if (pageNum != wxNOT_FOUND && m_panelTypes[pageNum] == TYPE_TABLEPANEL)
    {
        static_cast<TablePanel*>(m_tabs->GetPage(pageNum))->grid->SetData(_table, tableDisplayName, sIntName);

        if (pageNum != m_tabs->GetSelection())
            m_tabs->ChangeSelection(pageNum);
        else
        {
            m_tabs->Refresh();
            m_tabs->Layout();
        }

        return;
    }

    TablePanel* panel = new TablePanel(m_tabs, this, wxID_ANY, GetStatusBar(), m_topWindow);
    panel->SetTerminal(m_topWindow->getTerminal());
    panel->grid->SetData(_table, tableDisplayName, sIntName);

    m_panelTypes.push_back(TYPE_TABLEPANEL);
    m_tabs->AddPage(panel, wxFromUtf8(tableDisplayName), true);

    panel->ready();
}


/////////////////////////////////////////////////
/// \brief Event handler for tab changes.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::onPageChange(wxAuiNotebookEvent& event)
{
    SetTitle("NumeRe-DataViewer: " + m_tabs->GetPageText(event.GetSelection()));

    if (m_panelTypes[m_tabs->GetSelection()] == TYPE_TABLEPANEL)
        static_cast<TablePanel*>(m_tabs->GetPage(m_tabs->GetSelection()))->grid->refreshStatusBar();
    else
        static_cast<TableViewer*>(m_tabs->GetPage(m_tabs->GetSelection()))->refreshStatusBar();
}


/////////////////////////////////////////////////
/// \brief Event handler for tab closes.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::onPageClose(wxAuiNotebookEvent& event)
{
    // Is this the only page?
    if (m_tabs->GetPageCount() == 1)
        Close();

    if ((size_t)event.GetSelection() < m_panelTypes.size())
        m_panelTypes.erase(m_panelTypes.begin()+event.GetSelection());

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Forwarding event handler for all menu
/// events.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void TableBrowser::onMenuEvent(wxCommandEvent& event)
{
    if (m_panelTypes[m_tabs->GetSelection()] == TYPE_TABLEPANEL)
        static_cast<TablePanel*>(m_tabs->GetPage(m_tabs->GetSelection()))->grid->OnMenu(event);
    else
        static_cast<TableViewer*>(m_tabs->GetPage(m_tabs->GetSelection()))->OnMenu(event);
}

