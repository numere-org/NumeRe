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
#include "NumeReWindow.h"
#include "../common/datastructures.h"
#include "controls/treesearchctrl.hpp"
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
END_EVENT_TABLE()

// Documentation browser constructor
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
    TreeSearchCtrl* treeSearchCtrl = new TreeSearchCtrl(treePanel, wxID_ANY, _guilang.get("GUI_SEARCH_DOCUMENTATION"), _guilang.get("GUI_SEARCH_CALLTIP_TREE"), m_doctree);
    treePanel->AddWindows(treeSearchCtrl, m_doctree);

    m_viewer = new HelpViewer(splitter, mainwindow);
    m_viewer->SetRelatedFrame(this, titletemplate);
    m_viewer->SetRelatedStatusBar(0);

    // Set a reasonable window size and the window icon
    this->SetSize(1050, 800);
    this->SetIcon(wxIcon(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));

    // Split the view using the tree and the viewer
    splitter->SplitVertically(treePanel, m_viewer, 150);

    // Fill the index into the tree
    fillDocTree(mainwindow);

    this->Show();
    this->SetFocus();
}

// Destructor, removes the created IconManager object
DocumentationBrowser::~DocumentationBrowser()
{
    delete m_manager;
}

// Public interface to set the start page
bool DocumentationBrowser::SetStartPage(const wxString& docId)
{
    return m_viewer->ShowPageOnItem(docId);
}

// Private member function to prepare the toolbar of the window
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

// This private member function prepares the index tree of the documentation browser
void DocumentationBrowser::fillDocTree(NumeReWindow* mainwindow)
{
    // Get the documentation index from the kernel
    vector<string> vIndex = mainwindow->GetDocIndex();

    // Search for the icon ID of the document icon and prepare the tree root
    int iconId = m_manager->GetIconIndex("DOCUMENT");
    wxTreeItemId root = m_doctree->AddRoot("Index", m_manager->GetIconIndex("WORKPLACE"));

    // Append the index items using the document icon
    for (size_t i = 0; i < vIndex.size(); i++)
        m_doctree->AppendItem(root, vIndex[i], iconId);

    // Expand the top-level node
    m_doctree->Expand(root);
}

// Event handler function to load the documentation article describing
// the clicked item
void DocumentationBrowser::OnTreeClick(wxTreeEvent& event)
{
    m_viewer->ShowPageOnItem(m_doctree->GetItemText(event.GetItem()));
}

// Event handler function for clicks on the toolbar of the current window.
// Redirects all clicks to the HelpViewer class
void DocumentationBrowser::OnToolbarEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_HELP_HOME:
            m_viewer->GoHome();
            break;
        case ID_HELP_INDEX:
            m_viewer->GoIndex();
            break;
        case ID_HELP_GO_BACK:
            m_viewer->HistoryGoBack();
            break;
        case ID_HELP_GO_FORWARD:
            m_viewer->HistoryGoForward();
            break;
        case ID_HELP_PRINT:
            m_viewer->Print();
            break;
    }
}

