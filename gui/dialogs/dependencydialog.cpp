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

#include "dependencydialog.hpp"
#include "../NumeReWindow.h"
#include "../../kernel/core/utils/tools.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;

using namespace std;

BEGIN_EVENT_TABLE(DependencyDialog, wxDialog)
    EVT_TREE_ITEM_ACTIVATED(-1, DependencyDialog::OnItemActivate)
    EVT_TREE_ITEM_RIGHT_CLICK(-1, DependencyDialog::OnItemRightClick)
    EVT_MENU(ID_DEPENDENCYDIALOG_EXPORTDOT, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_FOLDALL, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_FOLDITEM, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_UNFOLDALL, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_UNFOLDITEM, DependencyDialog::OnMenuEvent)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor. Creates the UI elements
/// and calls the dependency walker.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param title const wxString&
/// \param mainfile const string&
/// \param lib ProcedureLibrary&
/// \param style long
///
/////////////////////////////////////////////////
DependencyDialog::DependencyDialog(wxWindow* parent, wxWindowID id, const wxString& title, const string& mainfile, ProcedureLibrary& lib, long style) : wxDialog(parent, id, title, wxDefaultPosition, wxSize(-1, 450), style)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create the UI elements
    m_dependencyTree = new wxcode::wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT);
    m_dependencyTree->AddColumn(_guilang.get("GUI_DEPDLG_TREE"), GetClientSize().GetWidth());
    vsizer->Add(m_dependencyTree, 1, wxEXPAND | wxALL, 5);
    vsizer->Add(CreateButtonSizer(wxOK), 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    SetSizer(vsizer);

    // Calculate the dependencies
    calculateDependencies(lib, mainfile);

    // Fill the dependency tree with the calculated
    // dependencies
    fillDependencyTree();
}


/////////////////////////////////////////////////
/// \brief This private member function calculates
/// the dependencies of the current selected main
/// file. It will fill the map with additional
/// items calculated from the called procedures.
///
/// \param lib ProcedureLibrary&
/// \param mainfile const string&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::calculateDependencies(ProcedureLibrary& lib, const string& mainfile)
{
    // Get the dependencies
    Dependencies* dep = lib.getProcedureContents(replacePathSeparator(mainfile))->getDependencies();
    m_mainProcedure = dep->getMainProcedure();

    // Insert the dependencies into the main map
    m_deps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());

    // Get the iterator to the begin of the map
    auto iter = m_deps.begin();

    // Go through the map
    while (iter != m_deps.end())
    {
        bool restart = false;

        // Go through all dependencies
        for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
        {
            if (m_deps.find(listiter->getProcedureName()) == m_deps.end() && listiter->getProcedureName().find("thisfile~") == string::npos)
            {
                dep = lib.getProcedureContents(listiter->getFileName())->getDependencies();

                if (dep->getDependencyMap().size())
                {
                    m_deps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());
                    iter = m_deps.begin();
                    restart = true;
                    break;
                }
            }
        }

        // Only increment the iterator, if we do not
        // need to restart the process
        if (!restart)
            ++iter;
    }
}


/////////////////////////////////////////////////
/// \brief This private member function fills the
/// tree in the UI with the calculated dependencies.
/// It will call the function
/// DependencyDialog::insertChilds() recursively
/// to fill the childs of a procedure call.
///
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::fillDependencyTree()
{
    // Find the current main procedure
    auto iter = m_deps.find(m_mainProcedure);

    // Ensure that the main procedure exists and that it is found
    // in the dependency maps
    if (iter == m_deps.end())
    {
        m_dependencyTree->AddRoot("ERROR: No main procedure found.");
        return;
    }

    wxTreeItemId root = m_dependencyTree->AddRoot(m_mainProcedure + "()");

    // Go through the list of calls
    for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
    {
        wxTreeItemId item = m_dependencyTree->AppendItem(root, listiter->getProcedureName() + "()");

        // Colour thisfile namespace calls in grey
        if (listiter->getProcedureName().find("thisfile~") != string::npos)
        {
            m_dependencyTree->SetItemTextColour(item, wxColour(128, 128, 128));
            m_dependencyTree->SetItemFont(item, GetFont().MakeItalic());
        }

        // Insert the child calls to the current procedure call
        insertChilds(item, listiter->getProcedureName());
    }

    // Expand the root node
    m_dependencyTree->Expand(root);
}


/////////////////////////////////////////////////
/// \brief This private member function is called
/// recursively to fill the childs of a procedure
/// call. The recursion is stopped at the end of
/// a branch or if the branch itself is a
/// recursion.
///
/// \param item wxTreeItemId
/// \param sParentProcedure const string&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::insertChilds(wxTreeItemId item, const string& sParentProcedure)
{
    // Find the current main procedure
    auto iter = m_deps.find(sParentProcedure);

    // Return, if the current procedure is not found
    if (iter == m_deps.end())
        return;

    for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
    {
        wxTreeItemId currItem;

        // If the current procedure is already part of the branch, then
        // simply add this call. Otherwise recurse to append its childs
        if (findInParents(item, listiter->getProcedureName() + "()"))
            currItem = m_dependencyTree->AppendItem(item, listiter->getProcedureName() + "()");
        else
        {
            currItem = m_dependencyTree->AppendItem(item, listiter->getProcedureName() + "()");
            insertChilds(currItem, listiter->getProcedureName());
        }

        // Colour thisfile namespace calls in grey
        if (listiter->getProcedureName().find("thisfile~") != string::npos)
        {
            m_dependencyTree->SetItemTextColour(currItem, wxColour(128, 128, 128));
            m_dependencyTree->SetItemFont(currItem, GetFont().MakeItalic());
        }

    }
}


/////////////////////////////////////////////////
/// \brief This private member function searches
/// the procedure call in the parents of the
/// current branch.
///
/// \param item wxTreeItemId
/// \param sCurrProc const string&
/// \return bool
///
/////////////////////////////////////////////////
bool DependencyDialog::findInParents(wxTreeItemId item, const string& sCurrProc)
{
    // Is the current node already equal?
    if (m_dependencyTree->GetItemText(item) == sCurrProc)
        return true;

    // As long as there are further parents
    // try to match the current string
    while (m_dependencyTree->GetItemParent(item).IsOk())
    {
        item = m_dependencyTree->GetItemParent(item);

        if (m_dependencyTree->GetItemText(item) == sCurrProc)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// collapse the current item and all corresponding
/// subitems.
///
/// \param item wxTreeItemId
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::CollapseAll(wxTreeItemId item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_dependencyTree->GetFirstChild(item, cookie);

    while (child.IsOk())
    {
        CollapseAll(child);
        child = m_dependencyTree->GetNextSibling(child);
    }

    m_dependencyTree->Collapse(item);
}


/////////////////////////////////////////////////
/// \brief Convert a name space into a vector of
/// single namespaces for easier comparison.
///
/// \param sNameSpace std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> DependencyDialog::parseNameSpace(std::string sNameSpace) const
{
    std::vector<std::string> vNameSpace;

    if (sNameSpace.front() == '$')
        sNameSpace.erase(0, 1);

    while (sNameSpace.length())
    {
        size_t pos = sNameSpace.find('~');
        vNameSpace.push_back(sNameSpace.substr(0, pos));
        sNameSpace.erase(0, pos);

        if (pos != std::string::npos)
            sNameSpace.erase(0, 1);
    }

    return vNameSpace;
}


/////////////////////////////////////////////////
/// \brief This private member function calculates
/// the level of nested clusters needed to
/// correctly visualize the nested namespaces.
///
/// \param sCurrentNameSpace const std::vector<std::string>&
/// \param sNewNameSpace const std::vector<std::string>&
/// \return int
///
/////////////////////////////////////////////////
int DependencyDialog::calculateClusterLevel(const std::vector<std::string>& sCurrentNameSpace, const std::vector<std::string>& sNewNameSpace)
{
    int nNameSpaces = 0;

    // Go through the namespaces and compare them
    for (; nNameSpaces < sCurrentNameSpace.size(); nNameSpaces++)
    {
        // New one is shorter? Return
        if (sNewNameSpace.size() <= nNameSpaces)
            return nNameSpaces+1;

        // Characters differ? Return
        if (sCurrentNameSpace[nNameSpaces] != sNewNameSpace[nNameSpaces])
            return nNameSpaces+1;
    }

    // New one is longer? Return the namespace counter + 1
    if (sNewNameSpace.size() > sCurrentNameSpace.size())
        return sNewNameSpace.size();

    return nNameSpaces;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of GraphViz DOT files.
///
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::CreateDotFile()
{
    std::string sClusterDefinition;
    std::string sDotFileContent;
    std::map<std::string, std::list<std::string> > mProcedures;

    // Fill the cluster map with the procedures and prepare the graph
    // list for DOT
    for (auto caller = m_deps.begin(); caller != m_deps.end(); ++caller)
    {
        for (auto called = caller->second.begin(); called != caller->second.end(); ++called)
        {
            if (caller->first.find_last_of("~/") != std::string::npos)
                mProcedures[caller->first.substr(0, caller->first.find_last_of("~/")+1)].push_back(caller->first);
            else
                mProcedures[caller->first].push_back(caller->first);

            mProcedures[called->getProcedureName().substr(0, called->getProcedureName().find_last_of("~/")+1)].push_back(called->getProcedureName());

            sDotFileContent += "\t\"" + caller->first + "()\" -> \"" + called->getProcedureName() + "()\"\n";
        }
    }

    std::vector<std::string> vCurrentNameSpace;
    std::vector<std::string> vNameSpace;
    int nClusterindex = 0;
    int nCurrentClusterLevel = 0;

    // Prepare the cluster list by calculating the cluster level of
    // the current namespace and nest it in a previous cluster
    for (auto iter = mProcedures.begin(); iter != mProcedures.end(); ++iter)
    {
        std::string sNameSpace = iter->first;

        if (sNameSpace.find_last_of("~/") != std::string::npos)
            sNameSpace.erase(sNameSpace.find_last_of("~/")+1);

        vNameSpace = parseNameSpace(sNameSpace);

        int nNewClusterLevel = calculateClusterLevel(vCurrentNameSpace, vNameSpace);

        // Deduce, whether the new cluster is nested or
        // an independent one
        if (nNewClusterLevel < nCurrentClusterLevel)
        {
            sClusterDefinition += "\n";

            for (int i = nCurrentClusterLevel-nNewClusterLevel; i >= 0; i--)
                sClusterDefinition += std::string(i+nNewClusterLevel, '\t') + "}\n";

            nCurrentClusterLevel = nNewClusterLevel-1;

            // Minimal level is 0 in this case
            if (nCurrentClusterLevel < 0)
                nCurrentClusterLevel = 0;
        }
        else if (nNewClusterLevel == nCurrentClusterLevel)
        {
            sClusterDefinition += "\n" + std::string(nNewClusterLevel, '\t') + "}\n";
            nCurrentClusterLevel--;
        }
        else
            sClusterDefinition += "\n";

        vCurrentNameSpace = vNameSpace;

        for (int i = nCurrentClusterLevel; i < vCurrentNameSpace.size(); i++)
        {
            // Write the cluster header
            sClusterDefinition += "\n" + std::string(i+1, '\t')
                + "subgraph cluster_" + toString(nClusterindex) + "\n"
                + std::string(i+1, '\t') + "{\n"
                + std::string(i+2, '\t') + "style=rounded\n"
                + std::string(i+2, '\t') + "label=\"" + vCurrentNameSpace[i] + "\"\n";
            nClusterindex++;
        }

        sClusterDefinition += std::string(vCurrentNameSpace.size()+1, '\t');
        nCurrentClusterLevel = vCurrentNameSpace.size();

        // Ensure that the procedure list is unique
        iter->second.sort();
        iter->second.unique();

        // Add the procedures to the current namespace
        for (auto procedure = iter->second.begin(); procedure != iter->second.end(); ++procedure)
            sClusterDefinition += "\"" + *procedure + "()\" ";

    }

    // Close all opened clusters
    for (int i = nCurrentClusterLevel; i > 0; i--)
        sClusterDefinition += "\n" + std::string(i, '\t') + "}";

    // Present a file save dialog to the user
    wxFileDialog saveDialog(this, _guilang.get("GUI_DLG_SAVE"), "", "dependency.dot", _guilang.get("COMMON_FILETYPE_DOT") + " (*.dot)|*.dot", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL)
        return;

    // Open the file stream
    ofstream file(saveDialog.GetPath().ToStdString().c_str());

    // If the stream could be opened, stream the prepared contents
    // of the DOT file to the opened file
    if (file.good())
    {
        file << "digraph ProcedureDependency\n{\n\tratio=0.4\n";
        file << sClusterDefinition << "\n\n";
        file << sDotFileContent << "}\n";
    }

    file.close();
}


/////////////////////////////////////////////////
/// \brief This private member function is the
/// event handler for double clicking on an item
/// in the dependency tree.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::OnItemActivate(wxTreeEvent& event)
{
    NumeReWindow* main = static_cast<NumeReWindow*>(this->GetParent());

    wxString procedureName = m_dependencyTree->GetItemText(event.GetItem());
    procedureName.erase(procedureName.find('('));

    // Procedures in the "thisfile" namespace can be found by first calling the main
    // procedure and then jumping to the correct local routine
    if (procedureName.find("::thisfile~") != string::npos)
    {
        main->FindAndOpenProcedure(procedureName.substr(0, procedureName.find("::thisfile~")));
        main->FindAndOpenProcedure("$" + procedureName.substr(procedureName.find("::thisfile~")+2));
    }
    else
        main->FindAndOpenProcedure(procedureName);
}


/////////////////////////////////////////////////
/// \brief This private member function shows the
/// popup menu on a right click on any tree item.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::OnItemRightClick(wxTreeEvent& event)
{
    m_selectedItem = event.GetItem();

    wxMenu popupMenu;

    popupMenu.Append(ID_DEPENDENCYDIALOG_FOLDALL, _guilang.get("GUI_DEPDLG_FOLDALL"));
    popupMenu.Append(ID_DEPENDENCYDIALOG_UNFOLDALL, _guilang.get("GUI_DEPDLG_UNFOLDALL"));
    popupMenu.Append(ID_DEPENDENCYDIALOG_FOLDITEM, _guilang.get("GUI_DEPDLG_FOLDITEM"));
    popupMenu.Append(ID_DEPENDENCYDIALOG_UNFOLDITEM, _guilang.get("GUI_DEPDLG_UNFOLDITEM"));
    popupMenu.AppendSeparator();
    popupMenu.Append(ID_DEPENDENCYDIALOG_EXPORTDOT, _guilang.get("GUI_DEPDLG_EXPORTDOT"));

    m_dependencyTree->PopupMenu(&popupMenu, event.GetPoint());
}


/////////////////////////////////////////////////
/// \brief This private member function handles
/// all menu events yielded by clicking on an
/// item of the popup menu.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_DEPENDENCYDIALOG_FOLDALL:
            CollapseAll(m_dependencyTree->GetRootItem());
            m_dependencyTree->Expand(m_dependencyTree->GetRootItem());
            break;
        case ID_DEPENDENCYDIALOG_UNFOLDALL:
            m_dependencyTree->ExpandAll(m_dependencyTree->GetRootItem());
            break;
        case ID_DEPENDENCYDIALOG_FOLDITEM:
            CollapseAll(m_selectedItem);
            break;
        case ID_DEPENDENCYDIALOG_UNFOLDITEM:
            m_dependencyTree->ExpandAll(m_selectedItem);
            break;
        case ID_DEPENDENCYDIALOG_EXPORTDOT:
            CreateDotFile();
            break;
    }
}


