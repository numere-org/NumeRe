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

#include "documentationbrowser.hpp"
#include "compositions/helpviewer.hpp"
#include "NumeReWindow.h"
#include "../common/datastructures.h"
#include "controls/treesearchctrl.hpp"
#include "controls/treedata.hpp"
#include "compositions/treepanel.hpp"
#include "../kernel/core/ui/language.hpp"
#include <vector>
#include <string>
#include <wx/artprov.h>

extern Language _guilang;

using namespace std;

BEGIN_EVENT_TABLE(DocumentationBrowser, ViewerFrame)
    EVT_TREE_SEL_CHANGED(-1, DocumentationBrowser::OnTreeClick)
    EVT_MENU_RANGE(EVENTID_HELP_START, EVENTID_HELP_END-1, DocumentationBrowser::OnToolbarEvent)
    EVT_AUINOTEBOOK_PAGE_CHANGED(-1, DocumentationBrowser::onPageChange)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Documentation browser constructor.
///
/// \param parent wxWindow*
/// \param titletemplate const wxString&
/// \param mainwindow NumeReWindow*
///
/////////////////////////////////////////////////
DocumentationBrowser::DocumentationBrowser(wxWindow* parent, const wxString& titletemplate, NumeReWindow* mainwindow) : ViewerFrame(parent, titletemplate)
{
    // Obtain the program root directory and create a new
    // IconManager object using this information
    wxString programPath = mainwindow->getProgramFolder();
    m_manager = new IconManager(programPath);

    // Create the status bar and the window splitter
    this->CreateStatusBar();
    prepareToolbar();
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxBORDER_THEME);

    // Create the tree and the viewer objects as childs
    // of the window splitter
    TreePanel* treePanel = new TreePanel(splitter, wxID_ANY);
    m_doctree = new wxTreeCtrl(treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_SINGLE | wxTR_FULL_ROW_HIGHLIGHT | wxTR_NO_LINES | wxTR_TWIST_BUTTONS);
    m_doctree->SetImageList(m_manager->GetImageList());
    TreeSearchCtrl* treeSearchCtrl = new TreeSearchCtrl(treePanel,
                                                        wxID_ANY,
                                                        _guilang.get("GUI_SEARCH_DOCUMENTATION"),
                                                        _guilang.get("GUI_SEARCH_CALLTIP_TREE"),
                                                        m_doctree,
                                                        true);
    treePanel->AddWindows(treeSearchCtrl, m_doctree);

    m_docTabs = new ViewerBook(splitter,
                               wxID_ANY,
                               wxDefaultPosition,
                               wxDefaultSize,
                               wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_MIDDLE_CLICK_CLOSE | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_WINDOWLIST_BUTTON);

    // Bind the mouse middle event handler to this class
    //m_docTabs->Bind(wxEVT_AUINOTEBOOK_TAB_MIDDLE_UP, &DocumentationBrowser::OnMiddleClick, this);
    m_docTabs->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &DocumentationBrowser::onPageClose, this);
    m_titleTemplate = titletemplate;

    // Set a reasonable window size and the window icon
    this->SetSize(1050, 800);
    this->SetIcon(mainwindow->getStandardIcon());

    // Split the view using the tree and the viewer
    splitter->SplitVertically(treePanel, m_docTabs, 150);

    // Fill the index into the tree
    fillDocTree(mainwindow);

    this->Show();
    this->SetFocus();
}


/////////////////////////////////////////////////
/// \brief Documentation browser destructor.
/// Removes the created IconManager object.
/////////////////////////////////////////////////
DocumentationBrowser::~DocumentationBrowser()
{
    delete m_manager;
}


/////////////////////////////////////////////////
/// \brief Public interface to set the start page.
///
/// \param docId const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool DocumentationBrowser::SetStartPage(const wxString& docId)
{
    return createNewPage(docId);
}


/////////////////////////////////////////////////
/// \brief Private member function to prepare the
/// toolbar of the main frame.
///
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::prepareToolbar()
{
    // Create a new tool bar
    wxToolBar* tb = this->CreateToolBar();

    // Fill the tool bar with tools
    tb->AddTool(ID_HELP_HOME, _guilang.get("GUI_TB_DOCBROWSER_HOME"), wxArtProvider::GetBitmap(wxART_GO_HOME, wxART_TOOLBAR), _guilang.get("GUI_TB_DOCBROWSER_HOME"));
    tb->AddTool(ID_HELP_INDEX, _guilang.get("GUI_TB_DOCBROWSER_INDEX"), wxArtProvider::GetBitmap(wxART_LIST_VIEW, wxART_TOOLBAR), _guilang.get("GUI_TB_DOCBROWSER_INDEX"));
    tb->AddSeparator();
    tb->AddTool(ID_HELP_GO_BACK, _guilang.get("GUI_TB_DOCBROWSER_BACK"), wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), _guilang.get("GUI_TB_DOCBROWSER_BACK"));
    tb->AddTool(ID_HELP_GO_FORWARD, _guilang.get("GUI_TB_DOCBROWSER_FORWARD"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), _guilang.get("GUI_TB_DOCBROWSER_FORWARD"));
    tb->AddSeparator();
    tb->AddTool(ID_HELP_PRINT, _guilang.get("GUI_TB_DOCBROWSER_PRINT"), wxArtProvider::GetBitmap(wxART_PRINT, wxART_TOOLBAR), _guilang.get("GUI_TB_DOCBROWSER_PRINT"));
//    tb->AddTool(ID_HELP_HOME, _guilang.get("GUI_TB_DOCBROWSER_FORWARD"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR));

    // Display the toolbar
    tb->Realize();
}


/////////////////////////////////////////////////
/// \brief This private member function prepares
/// the index tree of the documentation browser.
///
/// \param mainwindow NumeReWindow*
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::fillDocTree(NumeReWindow* mainwindow)
{
    // Get the documentation index from the kernel
    std::vector<std::string> vIndex = mainwindow->GetDocIndex();

    // Search for the icon ID of the document icon and prepare the tree root
    int iconId = m_manager->GetIconIndex("DOCUMENT");
    wxTreeItemId root = m_doctree->AddRoot("Index", m_manager->GetIconIndex("WORKPLACE"));

    // Append the index items using the document icon
    for (size_t i = 0; i < vIndex.size(); i++)
        m_doctree->AppendItem(root,
                              vIndex[i].substr(0, vIndex[i].find(' ')),
                              iconId, iconId,
                              new ToolTipTreeData(vIndex[i].substr(vIndex[i].find(' ')+1)));

    // Expand the top-level node
    m_doctree->Expand(root);
}


/////////////////////////////////////////////////
/// \brief Finds and selects the page with the
/// passed title or returns false.
///
/// \param title const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool DocumentationBrowser::findAndSelectPage(const wxString& title)
{
    for (size_t i = 0; i < m_docTabs->GetPageCount(); i++)
    {
        if (m_docTabs->GetPageText(i) == title)
        {
            m_docTabs->SetSelection(i);
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Event handler function to load the
/// documentation article describing the clicked
/// item.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::OnTreeClick(wxTreeEvent& event)
{
    HelpViewer* viewer = static_cast<HelpViewer*>(m_docTabs->GetCurrentPage());

    if (viewer)
    {
        viewer->ShowPageOnItem(m_doctree->GetItemText(event.GetItem()));
        setCurrentTabText(viewer->GetOpenedPageTitle());
    }
}


/////////////////////////////////////////////////
/// \brief Event handler function for clicks on
/// the toolbar of the current window. Redirects
/// all clicks to the HelpViewer class of the
/// currently opened page.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::OnToolbarEvent(wxCommandEvent& event)
{
    HelpViewer* viewer = static_cast<HelpViewer*>(m_docTabs->GetCurrentPage());

    if (!viewer)
        return;

    switch (event.GetId())
    {
        case ID_HELP_HOME:
            viewer->GoHome();
            break;
        case ID_HELP_INDEX:
            viewer->GoIndex();
            break;
        case ID_HELP_GO_BACK:
            viewer->HistoryGoBack();
            break;
        case ID_HELP_GO_FORWARD:
            viewer->HistoryGoForward();
            break;
        case ID_HELP_PRINT:
            viewer->Print();
            break;
    }

    setCurrentTabText(viewer->GetOpenedPageTitle());
}


/////////////////////////////////////////////////
/// \brief Create a new viewer in a new page.
///
/// \param docId const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool DocumentationBrowser::createNewPage(const wxString& docId)
{
    NumeReWindow* mainFrame = static_cast<NumeReWindow*>(m_parent);
    wxString title;

    // Find an already opened page with the same title
    if (docId.substr(0, 15) != "<!DOCTYPE html>")
        title = mainFrame->GetDocContent(docId); // Get the page content from the kernel
    else
        title = docId;

    // Extract the title from the HTML page
    title.erase(title.find("</title>"));
    title.erase(0, title.find("<title>")+7);

    // If the page already exists, we select it and
    // leave this method
    if (findAndSelectPage(title))
        return true;

    HelpViewer* viewer = new HelpViewer(m_docTabs, mainFrame, this);
    viewer->SetRelatedFrame(this, m_titleTemplate);
    viewer->SetRelatedStatusBar(0);

    m_docTabs->AddPage(viewer, docId, true);
    return viewer->ShowPageOnItem(docId);
}


/////////////////////////////////////////////////
/// \brief Change the text displayed on the
/// current tab.
///
/// \param text const wxString&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::setCurrentTabText(const wxString& text)
{
    if (m_docTabs->GetPageCount())
        m_docTabs->SetPageText(m_docTabs->GetSelection(), text);
}


/////////////////////////////////////////////////
/// \brief Event handler function called, when
/// the user switches the tabs.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::onPageChange(wxAuiNotebookEvent& event)
{
    SetTitle(wxString::Format(m_titleTemplate, m_docTabs->GetPageText(event.GetSelection())));
}


/////////////////////////////////////////////////
/// \brief Event handler called, when the user
/// closes the tab in some way.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationBrowser::onPageClose(wxAuiNotebookEvent& event)
{
    // Is this the only page?
    if (m_docTabs->GetPageCount() == 1)
        Close();

    event.Skip();
}



