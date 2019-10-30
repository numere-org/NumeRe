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
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor. Creates the UI elements
/// and calls the dependency walker.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param title const wxString&
/// \param mainfile const std::string&
/// \param lib ProcedureLibrary&
/// \param style long
///
/////////////////////////////////////////////////
DependencyDialog::DependencyDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::string& mainfile, ProcedureLibrary& lib, long style) : wxDialog(parent, id, title, wxDefaultPosition, wxSize(-1, 450), style)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create the UI elements
    m_dependencyTree = new wxcode::wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT);
    m_dependencyTree->AddColumn(_guilang.get("GUI_DEPDLG_TREE"), GetClientSize().GetWidth());
    vsizer->Add(m_dependencyTree, 1, wxEXPAND | wxALL, 5);
    vsizer->Add(CreateButtonSizer(wxOK), 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    SetSizer(vsizer);

    // Calculate the dependencies
    map<string, DependencyList> mDeps;
    string sMainProc = calculateDependencies(lib, mainfile, mDeps);

    // Fill the dependency tree with the calculated
    // dependencies
    fillDependencyTree(sMainProc, mDeps);
}


/////////////////////////////////////////////////
/// \brief This private member function calculates
/// the dependencies of the current selected main
/// file. It will fill the map with additional
/// items calculated from the called procedures.
///
/// \param lib ProcedureLibrary&
/// \param mainfile const std::string&
/// \param std::map<std::string
/// \param mDeps DependencyList>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DependencyDialog::calculateDependencies(ProcedureLibrary& lib, const std::string& mainfile, std::map<std::string, DependencyList>& mDeps)
{
    // Get the dependencies
    Dependencies* dep = lib.getProcedureContents(replacePathSeparator(mainfile))->getDependencies();
    string sMainProc = dep->getMainProcedure();
    bool restart = false;

    // Insert the dependencies into the main map
    mDeps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());

    // Get the iterator to the begin of the map
    auto iter = mDeps.begin();

    // Go through the map
    while (iter != mDeps.end())
    {
        restart = false;

        // Go through all dependencies
        for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
        {
            if (mDeps.find(listiter->getProcedureName()) == mDeps.end() && listiter->getProcedureName().find("thisfile~") == string::npos)
            {
                dep = lib.getProcedureContents(listiter->getFileName())->getDependencies();

                if (dep->getDependencyMap().size())
                {
                    mDeps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());
                    iter = mDeps.begin();
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

    return sMainProc;
}


/////////////////////////////////////////////////
/// \brief This private member function fills the
/// tree in the UI with the calculated dependencies.
/// It will call the function
/// DependencyDialog::insertChilds() recursively
/// to fill the childs of a procedure call.
///
/// \param sMainProcedure const std::string&
/// \param std::map<std::string
/// \param mDeps DependencyList>&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::fillDependencyTree(const std::string& sMainProcedure, std::map<std::string, DependencyList>& mDeps)
{
    // Find the current main procedure
    auto iter = mDeps.find(sMainProcedure);

    // Ensure that the main procedure exists and that it is found
    // in the dependency maps
    if (iter == mDeps.end())
    {
        m_dependencyTree->AddRoot("ERROR: No main procedure found.");
        return;
    }

    wxTreeItemId root = m_dependencyTree->AddRoot(sMainProcedure + "()");

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
        insertChilds(item, listiter->getProcedureName(), mDeps);
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
/// \param sParentProcedure const std::string&
/// \param std::map<std::string
/// \param mDeps DependencyList>&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::insertChilds(wxTreeItemId item, const std::string& sParentProcedure, std::map<std::string, DependencyList>& mDeps)
{
    // Find the current main procedure
    auto iter = mDeps.find(sParentProcedure);

    // Return, if the current procedure is not found
    if (iter == mDeps.end())
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
            insertChilds(currItem, listiter->getProcedureName(), mDeps);
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
/// \param sCurrProc const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool DependencyDialog::findInParents(wxTreeItemId item, const std::string& sCurrProc)
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

