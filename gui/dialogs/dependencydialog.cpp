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
#include "../../kernel/kernel.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../guilang.hpp"
#include "../../kernel/core/ui/winlayout.hpp"


using namespace std;

BEGIN_EVENT_TABLE(DependencyDialog, wxDialog)
    EVT_TREE_ITEM_ACTIVATED(-1, DependencyDialog::OnItemActivate)
    EVT_TREE_ITEM_RIGHT_CLICK(-1, DependencyDialog::OnItemRightClick)
    EVT_TREE_SEL_CHANGED(-1, DependencyDialog::OnItemSelected)
    EVT_TREE_SEL_CHANGING(-1, DependencyDialog::OnItemSelected)
    EVT_MENU(ID_DEPENDENCYDIALOG_EXPORTDOT, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_FOLDALL, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_FOLDITEM, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_UNFOLDALL, DependencyDialog::OnMenuEvent)
    EVT_MENU(ID_DEPENDENCYDIALOG_UNFOLDITEM, DependencyDialog::OnMenuEvent)
END_EVENT_TABLE()

#define WINDOWHEIGHT 550
#define WINDOWWIDTH 500

/////////////////////////////////////////////////
/// \brief Static helper function to convert
/// procedure file names to procedure names.
///
/// \param fileName std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string fileNameToProcedureName(std::string fileName)
{
    static Procedure& _interpreter = NumeReKernel::getInstance()->getProcedureInterpreter();
    std::string procPath = _interpreter.getPath();

    if (procPath.back() != '/')
        procPath += '/';

    if (fileName.starts_with(procPath))
    {
        fileName.erase(0, procPath.length());
        replaceAll(fileName, "/", "~");

        if (fileName.find('~') == std::string::npos)
            return "$main~" + fileName.substr(0, fileName.rfind('.'));

        return "$" + fileName.substr(0, fileName.rfind('.'));
    }

    return "$'" + fileName.substr(0, fileName.rfind('.')) + "'";
}


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
DependencyDialog::DependencyDialog(wxWindow* parent, wxWindowID id, const wxString& title, const string& mainfile, ProcedureLibrary& lib, long style) : wxDialog(parent, id, title, wxDefaultPosition, wxSize(WINDOWWIDTH, WINDOWHEIGHT), style)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create the UI elements
    m_dependencyTree = new wxcode::wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                  wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT);
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
    Dependencies* dep;

    if (mainfile.ends_with(".nlyt"))
    {
        m_mainProcedure = replacePathSeparator(mainfile);
        std::set<std::string> procs = getEventProcedures(m_mainProcedure);

        for (const std::string& proc : procs)
        {
            dep = lib.getProcedureContents(proc)->getDependencies();

            if (dep->getDependencyMap().size())
                m_deps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());
        }
    }
    else
    {
        dep = lib.getProcedureContents(replacePathSeparator(mainfile))->getDependencies();
        m_mainProcedure = dep->getMainProcedure();

        // Insert the dependencies into the main map
        m_deps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());
    }

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
                if (listiter->getType() == Dependency::NPRC)
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
                else if (listiter->getType() == Dependency::NLYT)
                {
                    std::set<std::string> procs = getEventProcedures(listiter->getFileName());

                    for (std::string proc : procs)
                    {
                        std::string procName = fileNameToProcedureName(proc);

                        if (m_deps.find(procName) != m_deps.end())
                            continue;

                        dep = lib.getProcedureContents(proc)->getDependencies();

                        if (dep->getDependencyMap().size())
                        {
                            m_deps.insert(dep->getDependencyMap().begin(), dep->getDependencyMap().end());
                            restart = true;
                        }
                    }

                    if (restart)
                    {
                        iter = m_deps.begin();
                        break;
                    }
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
    if (m_mainProcedure.ends_with(".nlyt"))
    {
        wxTreeItemId root = m_dependencyTree->AddRoot("window " + m_mainProcedure);
        m_dependencyTree->SetItemTextColour(root, wxColour(0, 0, 128));
        m_dependencyTree->SetItemFont(root, GetFont().MakeItalic());

        std::set<std::string> procs = getEventProcedures(m_mainProcedure);

        for (std::string proc : procs)
        {
            proc = fileNameToProcedureName(proc);
            wxTreeItemId item = m_dependencyTree->AppendItem(root, proc + "()");

            // Insert the child calls to the current procedure call
            insertChilds(item, proc);
        }

        return;
    }

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
        if (listiter->getType() == Dependency::NPRC)
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
        else if (listiter->getType() == Dependency::NLYT)
        {
            wxTreeItemId nlytRoot = m_dependencyTree->AppendItem(root, listiter->getProcedureName());
            m_dependencyTree->SetItemTextColour(nlytRoot, wxColour(0, 0, 128));
            m_dependencyTree->SetItemFont(nlytRoot, GetFont().MakeItalic());

            std::set<std::string> procs = getEventProcedures(listiter->getFileName());

            for (std::string proc : procs)
            {
                proc = fileNameToProcedureName(proc);
                wxTreeItemId item = m_dependencyTree->AppendItem(nlytRoot, proc + "()");

                // Insert the child calls to the current procedure call
                insertChilds(item, proc);
            }
        }
        else
            m_dependencyTree->AppendItem(root, listiter->getProcedureName());
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

        if (listiter->getType() == Dependency::NPRC)
        {
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
        else if (listiter->getType() == Dependency::NLYT)
        {
            wxTreeItemId nlytRoot = m_dependencyTree->AppendItem(item, listiter->getProcedureName());
            m_dependencyTree->SetItemTextColour(nlytRoot, wxColour(00, 00, 128));
            m_dependencyTree->SetItemFont(nlytRoot, GetFont().MakeItalic());

            std::set<std::string> procs = getEventProcedures(listiter->getFileName());

            for (std::string proc : procs)
            {
                proc = fileNameToProcedureName(proc);
                currItem = m_dependencyTree->AppendItem(nlytRoot, proc + "()");

                // Insert the child calls to the current procedure call
                insertChilds(currItem, proc);
            }
        }
        else
            m_dependencyTree->AppendItem(item, listiter->getProcedureName());
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
    for (; nNameSpaces < (int)sCurrentNameSpace.size(); nNameSpaces++)
    {
        // New one is shorter? Return
        if ((int)sNewNameSpace.size() <= nNameSpaces)
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

    // NLYT as a main file needs some pre-processing
    if (m_mainProcedure.ends_with(".nlyt"))
    {
        std::string window = "window " + m_mainProcedure;
        std::string sProcPath = NumeReKernel::getInstance()->getSettings().getProcPath();

        replaceAll(window, sProcPath.c_str(), "<procpath>");
        replaceAll(window, "\"", "'");

        if (window.find("<procpath>") != std::string::npos)
        {
            std::string sNamespace = window.substr(window.find("<procpath>")+11, window.length() - window.find("<procpath>") - 11 - 1);
            replaceAll(sNamespace, "/", "~");
            mProcedures["$" + sNamespace.substr(0, sNamespace.rfind('~')+1)].push_back(window);
        }

        std::set<std::string> procs = getEventProcedures(m_mainProcedure);

        for (std::string proc : procs)
        {
            proc = fileNameToProcedureName(proc);
            mProcedures[proc.substr(0, proc.find_last_of("~/")+1)].push_back(proc);

            sDotFileContent += "\t\"" + window + "\" -> \"" + proc + "()\" [label=\"<<event>>\" fontname=\"Consolas\"]\n";
        }
    }

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

            if (called->getType() == Dependency::NPRC)
            {
                const std::string& proc = called->getProcedureName();
                mProcedures[proc.substr(0, proc.find_last_of("~/")+1)].push_back(proc);

                if (proc.find("::thisfile~") != std::string::npos)
                    sDotFileContent += "\t\"" + caller->first + "()\" -> \"" + proc + "()\" [color=darkgoldenrod]\n";
                else if (caller->first.substr(0, caller->first.rfind('~')) == proc.substr(0, proc.rfind('~')))
                    sDotFileContent += "\t\"" + caller->first + "()\" -> \"" + proc + "()\" [color=lightslategray]\n";
                else
                    sDotFileContent += "\t\"" + caller->first + "()\" -> \"" + proc + "()\"\n";
            }
            else if (called->getType() == Dependency::NLYT)
            {
                std::string window = called->getProcedureName();
                replaceAll(window, "\"", "'");

                if (window.find("<procpath>") != std::string::npos)
                {
                    std::string sNamespace = window.substr(window.find("<procpath>")+11, window.length() - window.find("<procpath>") - 11 - 1);
                    replaceAll(sNamespace, "/", "~");
                    mProcedures["$" + sNamespace.substr(0, sNamespace.rfind('~')+1)].push_back(window);
                }

                sDotFileContent += "\t\"" + caller->first + "()\" -> \"" + window + "\" [color=midnightblue label=\"<<create>>\" fontname=\"Consolas\" style=dashed fontcolor=midnightblue]\n";

                std::set<std::string> procs = getEventProcedures(called->getFileName());

                for (std::string proc : procs)
                {
                    proc = fileNameToProcedureName(proc);
                    mProcedures[proc.substr(0, proc.find_last_of("~/")+1)].push_back(proc);

                    sDotFileContent += "\t\"" + window + "\" -> \"" + proc + "()\" [label=\"<<event>>\" fontname=\"Consolas\"]\n";
                }
            }
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

        for (int i = nCurrentClusterLevel; i < (int)vCurrentNameSpace.size(); i++)
        {
            // Write the cluster header
            if (vCurrentNameSpace[i].find("::thisfile") != std::string::npos)
                sClusterDefinition += "\n" + std::string(i+1, '\t')
                    + "subgraph cluster_" + toString(nClusterindex) + "\n"
                    + std::string(i+1, '\t') + "{\n"
                    + std::string(i+2, '\t') + "style=filled\n"
                    + std::string(i+2, '\t') + "color=lightsteelblue\n"
                    + std::string(i+2, '\t') + "fillcolor=floralwhite\n"
                    + std::string(i+2, '\t') + "node [fillcolor=\"cornsilk\"]\n"
                    + std::string(i+2, '\t') + "label=\"" + vCurrentNameSpace[i] + "\"\n";
            else
                sClusterDefinition += "\n" + std::string(i+1, '\t')
                    + "subgraph cluster_" + toString(nClusterindex) + "\n"
                    + std::string(i+1, '\t') + "{\n"
                    + std::string(i+2, '\t') + "style=rounded\n"
                    + std::string(i+2, '\t') + "color=mediumslateblue\n"
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
        {
            if (procedure->starts_with("window "))
                sClusterDefinition += "\"" + *procedure + "\" [fillcolor=lightblue fontcolor=midnightblue] ";
            else
                sClusterDefinition += "\"" + *procedure + "()\" ";
        }

    }

    // Close all opened clusters
    for (int i = nCurrentClusterLevel; i > 0; i--)
        sClusterDefinition += "\n" + std::string(i, '\t') + "}";

    // Present a file save dialog to the user
    wxFileDialog saveDialog(this, _guilang.get("GUI_DLG_SAVE"), "", "dependency.dot", _guilang.get("COMMON_FILETYPE_DOT") + " (*.dot)|*.dot", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL)
        return;

    // Open the file stream
    boost::nowide::ofstream file(wxToUtf8(saveDialog.GetPath()));

    // If the stream could be opened, stream the prepared contents
    // of the DOT file to the opened file
    if (file.good())
    {
        file << "digraph ProcedureDependency\n{\n\tratio=0.4\n\tfontname=\"Consolas\"\n\tnode [fontname=\"Consolas\" fontcolor=\"maroon\" style=\"filled\" fillcolor=\"palegoldenrod\"]";
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
    if (!event.GetItem().IsOk())
        return;

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


/////////////////////////////////////////////////
/// \brief This private member function handles
/// highlighting all occurrences of an item
/// selected in the dependency viewer
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void DependencyDialog::OnItemSelected(wxTreeEvent& event)
{
    if (!event.GetItem().IsOk())
        return;

    wxTreeItemId currItem = m_dependencyTree->GetRootItem();
    wxString NAME = m_dependencyTree->GetItemText(event.GetItem());

    while ((currItem = m_dependencyTree->GetNext(currItem)).IsOk())
    {
        wxString helper = m_dependencyTree->GetItemText(currItem);
        if (m_dependencyTree->GetItemText(currItem) == NAME)
            m_dependencyTree->SetItemBackgroundColour(currItem, wxColour(192, 227, 248));
        else
            m_dependencyTree->SetItemBackgroundColour(currItem, *wxWHITE);
    }
}

