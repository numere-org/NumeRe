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
#include <vector>
#include <string>

using namespace std;

BEGIN_EVENT_TABLE(DocumentationBrowser, ViewerFrame)
    EVT_TREE_SEL_CHANGED(-1, DocumentationBrowser::OnTreeClick)
END_EVENT_TABLE()

// Documentation browser constructor
DocumentationBrowser::DocumentationBrowser(wxWindow* parent, const wxString& titletemplate, NumeReWindow* mainwindow) : ViewerFrame(parent, titletemplate)
{
    // Obtain the program root directory and create a new
    // IconManager object using this information
    wxString programPath = mainwindow->getProgramFolder();
    m_manager = new IconManager(mainwindow->getProgramFolder());

    // Create the status bar and the window splitter
    this->CreateStatusBar();
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxBORDER_THEME);

    // Create the tree and the viewer objects as childs
    // of the window splitter
    m_doctree = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_SINGLE | wxTR_FULL_ROW_HIGHLIGHT | wxTR_NO_LINES | wxTR_TWIST_BUTTONS);
    m_doctree->SetImageList(m_manager->GetImageList());
    m_viewer = new HelpViewer(splitter, mainwindow);
    m_viewer->SetRelatedFrame(this, titletemplate);
    m_viewer->SetRelatedStatusBar(0);

    // Set a reasonable window size and the window icon
    this->SetSize(1050, 800);
    this->SetIcon(wxIcon(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));

    // Split the view using the tree and the viewer
    splitter->SplitVertically(m_doctree, m_viewer, 150);

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

