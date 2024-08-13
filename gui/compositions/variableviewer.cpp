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

#include "variableviewer.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../common/datastructures.h"
#include "../../common/Options.h"
#include "../NumeReWindow.h"
#include <wx/menu.h>
#include <wx/dialog.h>
#include <wx/clipbrd.h>

#define DIMCOLUMN 1
#define CLASSCOLUMN 2
#define VALUECOLUMN 3
#define SIZECOLUMN 4

extern Language _guilang;
using namespace wxcode;

struct VarData : public wxTreeItemData
{
    std::string sInternalName;

    VarData(const std::string name) : sInternalName(name) {}
    virtual ~VarData() override {}
};




BEGIN_EVENT_TABLE(VariableViewer, wxcode::wxTreeListCtrl)
    EVT_TREE_ITEM_RIGHT_CLICK(-1, VariableViewer::OnRightClick)
    EVT_TREE_ITEM_ACTIVATED(-1, VariableViewer::OnDoubleClick)
    EVT_MENU_RANGE(ID_VARVIEWER_NEW, ID_VARVIEWER_COPYVALUE, VariableViewer::OnMenuEvent)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param parent wxWindow*
/// \param mainWin NumeReWindow*
/// \param fieldsize int
/// \param debugMode bool
///
/////////////////////////////////////////////////
VariableViewer::VariableViewer(wxWindow* parent, NumeReWindow* mainWin, int fieldsize, bool debugMode) : wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_NO_LINES | wxTR_HIDE_ROOT)
{
    selectedID = wxTreeItemId();

    nDataFieldSize = fieldsize;
    mainWindow = mainWin;

    // Create the default columns
    AddColumn(_guilang.get("GUI_VARVIEWER_NAME"), 150);
    AddColumn(_guilang.get("GUI_VARVIEWER_DIM"), 80, wxALIGN_RIGHT);
    AddColumn(_guilang.get("GUI_VARVIEWER_CLASS"), 55);
    AddColumn(_guilang.get("GUI_VARVIEWER_VALUE"), fieldsize - (!debugMode)*50);

    if (!debugMode)
        AddColumn(_guilang.get("GUI_VARVIEWER_SIZE"), 120);

    // Create root node
    AddRoot("ROOT");

    // Create variable class nodes
    numRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_VARS"));
    stringRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_STRINGS"));
    clusterRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_CLUSTERS"));
    tableRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_TABLES"));
    SetItemText(tableRoot, VALUECOLUMN, " {min, ..., max}");
    SetItemText(clusterRoot, VALUECOLUMN, " {first, ..., last}");

    // Make the variable class nodes bold
    SetItemBold(numRoot, true);
    SetItemBold(stringRoot, true);
    SetItemBold(tableRoot, true);
    SetItemBold(clusterRoot, true);

    for (size_t i = 0; i < 6; i++)
    {
        bExpandedState[i] = true;
    }

    setDebuggerMode(debugMode);
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether a
/// variable was already part of the previous
/// variable set (only used in debug mode).
///
/// \param sVar const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool VariableViewer::checkPresence(const std::string& sVar)
{
    for (size_t i = 0; i < vLastVarSet.size(); i++)
    {
        if (vLastVarSet[i] == sVar)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member functions checks for
/// special variable names, to highlight them
/// (not used in debug mode).
///
/// \param sVar const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool VariableViewer::checkSpecialVals(const std::string& sVar)
{
    if (sVar.substr(0, sVar.find('\t')) == "data()")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "table()")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "string()")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "x")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "y")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "z")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "t")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "nrows" || sVar.substr(0, sVar.find('\t')) == "nlines")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "ncols")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "nlen")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "ans" || sVar.substr(0, sVar.find('\t')) == "ans{}")
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function splits the passed
/// variable at tabulator characters and adds its
/// contents to the current tree item. It will
/// also create the tooltip for this item.
///
/// \param rootNode wxTreeItemId
/// \param sVar std::string
/// \return wxTreeItemId
///
/////////////////////////////////////////////////
wxTreeItemId VariableViewer::AppendVariable(wxTreeItemId rootNode, std::string sVar)
{
    wxString tooltip;

    wxTreeItemId currentItem = AppendItem(rootNode, sVar.substr(0, sVar.find('\t')));
    tooltip = sVar.substr(0, sVar.find('\t'));
    sVar.erase(0, sVar.find('\t')+1);

    SetItemText(currentItem, DIMCOLUMN, sVar.substr(0, sVar.find('\t')) + " ");
    sVar.erase(0, sVar.find('\t')+1);

    SetItemText(currentItem, CLASSCOLUMN, " " + sVar.substr(0, sVar.find('\t')));
    sVar.erase(0, sVar.find('\t')+1);

    SetItemText(currentItem, VALUECOLUMN, " " + sVar.substr(0, sVar.find('\t')));

    // Create the tooltip and set it
    size_t pos = sVar.find('\t');

    if (pos < 2e3)
        tooltip += " = " + sVar.substr(0, pos);
    else
        tooltip += " = " + sVar.substr(0, 1000) + "[...]" + sVar.substr(pos-1000, pos);

    SetItemToolTip(currentItem, tooltip);

    // Write values to the size column
    if (!debuggerMode)
    {
        SetItemText(currentItem, SIZECOLUMN, " " + sVar.substr(sVar.rfind('\t')+1));
        sVar.erase(sVar.rfind('\t'));
    }

    // Set the internal variable's name as a
    // VarData object
    // NOTE: STRING::RFIND is necessary to avoid issues with
    // tabulator characters in the VALUECOLUMN
    SetItemData(currentItem, new VarData(sVar.substr(sVar.rfind('\t')+1)));

    return currentItem;
}


/////////////////////////////////////////////////
/// \brief A simple helper function to clean the
/// tree.
///
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::ClearTree()
{
    if (HasChildren(numRoot))
        bExpandedState[0] = IsExpanded(numRoot);

    if (HasChildren(stringRoot))
        bExpandedState[1] = IsExpanded(stringRoot);

    if (HasChildren(tableRoot))
        bExpandedState[2] = IsExpanded(tableRoot);

    if (HasChildren(clusterRoot))
        bExpandedState[3] = IsExpanded(clusterRoot);

    DeleteChildren(numRoot);
    DeleteChildren(stringRoot);
    DeleteChildren(tableRoot);
    DeleteChildren(clusterRoot);

    if (argumentRoot.IsOk())
    {
        if (HasChildren(argumentRoot))
            bExpandedState[4] = IsExpanded(argumentRoot);

        DeleteChildren(argumentRoot);
    }

    if (globalRoot.IsOk())
    {
        if (HasChildren(globalRoot))
            bExpandedState[5] = IsExpanded(globalRoot);

        DeleteChildren(globalRoot);
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles every
/// task, which is specific to the debug mode
/// after a variable update.
///
/// \param vVarList const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::HandleDebugActions(const std::vector<std::string>& vVarList)
{
    if (!debuggerMode)
        return;

    vLastVarSet = vVarList;
}


/////////////////////////////////////////////////
/// \brief Returns the internal variable name of
/// the selected variable.
///
/// \param id wxTreeItemId
/// \return wxString
///
/////////////////////////////////////////////////
wxString VariableViewer::GetInternalName(wxTreeItemId id)
{
    return static_cast<VarData*>(GetItemData(id))->sInternalName;
}


/////////////////////////////////////////////////
/// \brief This member function handles the menu
/// events created from the popup menu and
/// redirects the control to the corresponding
/// functions.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_VARVIEWER_NEW:
            OnNewTable();
            break;
        case ID_VARVIEWER_SHOW:
            OnShowTable(GetInternalName(selectedID), GetItemText(selectedID));
            break;
        case ID_VARVIEWER_EDIT:
            OnEditTable(GetInternalName(selectedID));
            break;
        case ID_VARVIEWER_REMOVE:
            OnRemoveTable(GetItemText(selectedID));
            break;
        case ID_VARVIEWER_RENAME:
            OnRenameTable(GetItemText(selectedID));
            break;
        case ID_VARVIEWER_SAVE:
            OnSaveTable(GetItemText(selectedID));
            break;
        case ID_VARVIEWER_SAVEAS:
            OnSaveasTable(GetItemText(selectedID));
            break;
        case ID_VARVIEWER_COPYVALUE:
            OnCopyValue(GetItemText(selectedID) + " =" + GetItemText(selectedID, VALUECOLUMN));
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays a text
/// entry dialog to enter the new table names and
/// sends the corresponding command to the kernel.
///
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnNewTable()
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_NEWTABLE_QUESTION"), _guilang.get("GUI_VARVIEWER_NEWTABLE"), "table()");

    if (textEntry.ShowModal() != wxID_OK)
        return;

    wxString tables = textEntry.GetValue();

    // Ensure that the user entered the necessary parentheses
    if (tables.find(',') != std::string::npos)
    {
        wxString tablesTemp;

        // Examine each table name
        while (tables.length())
        {
            if (tablesTemp.length())
                tablesTemp += ", ";

            // Get the next table name
            tablesTemp += tables.substr(0, tables.find(','));

            // Add the needed parentheses directly at the end of
            // the table name string
            if (tablesTemp.substr(tablesTemp.find_last_not_of(' ')-1, 2) != "()")
                tablesTemp.insert(tablesTemp.find_last_not_of(' ')+1, "()");

            // Cut off the current table name
            if (tables.find(',') != std::string::npos)
                tables.erase(0, tables.find(',')+1);
            else
                break;
        }

        tables = tablesTemp;
    }
    else if (tables.find("()") == std::string::npos)
    {
        // Add the needed parentheses directly at the end of
        // the table name string
        tables.insert(tables.find_last_not_of(' ')+1, "()");
    }

    mainWindow->pass_command("new " + tables + " -free");
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// selected table.
///
/// \param table const wxString&
/// \param tableDisplayName const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnShowTable(const wxString& table, const wxString& tableDisplayName)
{
    mainWindow->showTable(table, tableDisplayName);
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// selected table for editing.
///
/// \param table const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnEditTable(const wxString& table)
{
    mainWindow->pass_command("edit " + table);
}


/////////////////////////////////////////////////
/// \brief This member function displays a text
/// entry dialog to choose a new name for the
/// selected table.
///
/// \param table const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnRenameTable(const wxString& table)
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_NEWNAME_QUESTION"), _guilang.get("GUI_VARVIEWER_NEWNAME"), table);

    if (textEntry.ShowModal() != wxID_OK)
        return;

    mainWindow->pass_command("rename " + table + ", " + textEntry.GetValue());
}


/////////////////////////////////////////////////
/// \brief This member function removes the
/// selected table.
///
/// \param table const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnRemoveTable(const wxString& table)
{
    mainWindow->pass_command("remove " + table);
}


/////////////////////////////////////////////////
/// \brief This member function saves the
/// selected table.
///
/// \param table const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnSaveTable(const wxString& table)
{
    mainWindow->pass_command("save " + table);
}


/////////////////////////////////////////////////
/// \brief This member function displays a text
/// entry dialog to choose the file name for the
/// selected table, which is then used to create
/// a save file containing the table data.
///
/// \param table const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnSaveasTable(const wxString& table)
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_SAVENAME_QUESTION"), _guilang.get("GUI_VARVIEWER_SAVENAME"), table.substr(0, table.find('(')));

    if (textEntry.ShowModal() != wxID_OK)
        return;

    mainWindow->pass_command("save " + table + " -file=\"" + textEntry.GetValue() + "\"");
}


/////////////////////////////////////////////////
/// \brief Copies the selected text to the clip
/// board.
///
/// \param value const wxString&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnCopyValue(const wxString& value)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(value));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function expands all
/// nodes, which contain child nodes.
///
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::ExpandAll()
{
    if (HasChildren(numRoot) && bExpandedState[0])
        Expand(numRoot);

    if (HasChildren(stringRoot) && bExpandedState[1])
        Expand(stringRoot);

    if (HasChildren(tableRoot) && bExpandedState[2])
        Expand(tableRoot);

    if (HasChildren(clusterRoot) && bExpandedState[3])
        Expand(clusterRoot);

    if (argumentRoot.IsOk() && HasChildren(argumentRoot) && bExpandedState[4])
        Expand(argumentRoot);

    if (globalRoot.IsOk() && HasChildren(globalRoot) && bExpandedState[5])
        Expand(globalRoot);
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// pop-up menu in the var viewer case (it does
/// nothing in debugger mode and also nothing for
/// non-tables).
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnRightClick(wxTreeEvent& event)
{
    if (!event.GetItem().IsOk() || GetItemParent(event.GetItem()) == GetRootItem())
        return;

    // Get the name of the table and determine, whether
    // the table may be modified
    bool isTable = GetItemParent(event.GetItem()) == tableRoot || GetItemText(event.GetItem()).find("()") != std::string::npos;
    wxString itemLabel = GetItemText(event.GetItem());
    bool bMayBeModified = itemLabel != "string()" && itemLabel != "table()";

    // Create the menu
    wxMenu popUpmenu;

    if (isTable && !debuggerMode)
    {
        // Append commons
        popUpmenu.Append(ID_VARVIEWER_NEW, _guilang.get("GUI_VARVIEWER_MENU_NEWTABLE"));
        popUpmenu.AppendSeparator();
        popUpmenu.Append(ID_VARVIEWER_SHOW, _guilang.get("GUI_VARVIEWER_MENU_SHOWVALUE"));
        popUpmenu.Append(ID_VARVIEWER_EDIT, _guilang.get("GUI_VARVIEWER_MENU_EDITVALUE"));
        popUpmenu.Append(ID_VARVIEWER_RENAME, _guilang.get("GUI_VARVIEWER_MENU_RENAME"));
        popUpmenu.Append(ID_VARVIEWER_REMOVE, _guilang.get("GUI_VARVIEWER_MENU_REMOVE"));

        popUpmenu.AppendSeparator();
        popUpmenu.Append(ID_VARVIEWER_SAVE, _guilang.get("GUI_VARVIEWER_MENU_SAVE"));
        popUpmenu.Append(ID_VARVIEWER_SAVEAS, _guilang.get("GUI_VARVIEWER_MENU_SAVEAS"));

        // Disable menu items for tables, which
        // cannot be modified
        if (!bMayBeModified)
        {
            popUpmenu.Enable(ID_VARVIEWER_RENAME, false);
            popUpmenu.Enable(ID_VARVIEWER_REMOVE, false);
        }
    }
    else if (!isTable)
        popUpmenu.Append(ID_VARVIEWER_COPYVALUE, _guilang.get("GUI_VARVIEWER_MENU_COPYVALUE"));


    // Store the selected item for the menu event handler
    selectedID = event.GetItem();

    // Display the menu
    if (!isTable || !debuggerMode)
        PopupMenu(&popUpmenu, event.GetPoint());
}


/////////////////////////////////////////////////
/// \brief This event handler displays the
/// selected table.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::OnDoubleClick(wxTreeEvent& event)
{
    if (GetItemParent(event.GetItem()) != tableRoot && GetItemParent(event.GetItem()) != clusterRoot)
    {
        // In the debugger mode it's possible that the arguments
        // or the globals contain table variables, which should
        // also be viewable
        if (debuggerMode && (GetItemParent(event.GetItem()) == argumentRoot || GetItemParent(event.GetItem()) == globalRoot))
        {
            if (GetItemText(event.GetItem()).find("()") != std::string::npos || GetItemText(event.GetItem()).find("{}") != std::string::npos)
                OnShowTable(GetInternalName(event.GetItem()), GetItemText(event.GetItem()));
            else if (GetInternalName(event.GetItem()).find("{}") != std::string::npos) // Fix for macros and templates
                OnShowTable(GetInternalName(event.GetItem()), GetItemText(event.GetItem()) + "@CST{}");
            else if (GetInternalName(event.GetItem()).find("()") != std::string::npos) // Fix for macros and templates
                OnShowTable(GetInternalName(event.GetItem()), GetItemText(event.GetItem()) + "@TAB()");
        }

        return;
    }

    OnShowTable(GetInternalName(event.GetItem()), GetItemText(event.GetItem()));
}


/////////////////////////////////////////////////
/// \brief This member function creates or
/// removes unneeded tree root items and handles
/// the debugger mode.
///
/// \param mode bool
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::setDebuggerMode(bool mode)
{
    debuggerMode = mode;

    if (mode)
    {
        SetColumnWidth(CLASSCOLUMN, 75);

        // Create or remove the procedure argument
        // root item
        if (!argumentRoot.IsOk() && mainWindow->getOptions()->GetShowProcedureArguments())
        {
            argumentRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_ARGUMENTS"));
            SetItemBold(argumentRoot, true);
        }
        else if (!mainWindow->getOptions()->GetShowProcedureArguments())
        {
            Delete(argumentRoot);
            argumentRoot.Unset();
        }

        // Create or remove the global variable
        // root item
        if (!globalRoot.IsOk() && mainWindow->getOptions()->GetShowGlobalVariables())
        {
            globalRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_GLOBALS"));
            SetItemBold(globalRoot, true);
        }
        else if (!mainWindow->getOptions()->GetShowGlobalVariables())
        {
            Delete(globalRoot);
            globalRoot.Unset();
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function is used to update
/// the variable list, which is displayed by this
/// control.
///
/// \param vVarList const std::vector<std::string>&
/// \param nNumerics size_t
/// \param nStrings size_t
/// \param nTables size_t
/// \param nClusters size_t
/// \param nArguments size_t
/// \param nGlobals size_t
/// \return void
///
/////////////////////////////////////////////////
void VariableViewer::UpdateVariables(const std::vector<std::string>& vVarList, size_t nNumerics, size_t nStrings, size_t nTables, size_t nClusters, size_t nArguments, size_t nGlobals)
{
    // Clear the tree first
    ClearTree();

    // Show the numbers of variables in the current group
    SetItemText(numRoot, DIMCOLUMN, "[" + toString(nNumerics) + "] ");
    SetItemText(stringRoot, DIMCOLUMN, "[" + toString(nStrings) + "] ");
    SetItemText(tableRoot, DIMCOLUMN, "[" + toString(nTables) + "] ");
    SetItemText(clusterRoot, DIMCOLUMN, "[" + toString(nClusters) + "] ");

    if (argumentRoot.IsOk())
    {
        SetItemText(argumentRoot, DIMCOLUMN, "[" + toString(nArguments) + "] ");
    }

    if (globalRoot.IsOk())
    {
        SetItemText(globalRoot, DIMCOLUMN, "[" + toString(nGlobals) + "] ");
    }

    wxTreeItemId currentItem;

    // Go through the list of all passed variables
    for (size_t i = 0; i < vVarList.size(); i++)
    {
        if (i < nNumerics)
        {
            // Append a variable to the numerics list
            currentItem = AppendVariable(numRoot, vVarList[i]);
        }
        else if (i < nStrings+nNumerics)
        {
            // Append a variable to the string list
            currentItem = AppendVariable(stringRoot, vVarList[i]);
        }
        else if (i < nStrings+nNumerics+nTables)
        {
            // Append a variable to the tables list
            currentItem = AppendVariable(tableRoot, vVarList[i]);
        }
        else if (i < nStrings+nNumerics+nTables+nClusters)
        {
            // Append a variable to the tables list
            currentItem = AppendVariable(clusterRoot, vVarList[i]);
        }
        else if (argumentRoot.IsOk() && i < nStrings+nNumerics+nTables+nClusters+nArguments)
        {
            // Append a variable to the tables list
            currentItem = AppendVariable(argumentRoot, vVarList[i]);
        }
        else if (globalRoot.IsOk() && i < nStrings+nNumerics+nTables+nClusters+nArguments+nGlobals)
        {
            // Append a variable to the tables list
            currentItem = AppendVariable(globalRoot, vVarList[i]);
        }

        // Perform all necessary highlighting options
        if (debuggerMode && !checkPresence(vVarList[i]))
            SetItemTextColour(currentItem, *wxRED);
        if (!debuggerMode && checkSpecialVals(vVarList[i]))
        {
            SetItemTextColour(currentItem, wxColour(0, 0, 192));
        }
    }

    // Expand the tree
    ExpandAll();

    // Handle debug mode specific tasks
    HandleDebugActions(vVarList);
}

