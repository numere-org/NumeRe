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
#include "../../common/datastructures.h"
#include "../NumeReWindow.h"
#include <wx/menu.h>
#include <wx/dialog.h>

#define DIMCOLUMN 1
#define CLASSCOLUMN 2
#define VALUECOLUMN 3

std::string toString(int nNum);

extern Language _guilang;
using namespace wxcode;

struct VarData : public wxTreeItemData
{
    std::string sInternalName;

    VarData(const std::string name) : sInternalName(name) {}
};




BEGIN_EVENT_TABLE(VariableViewer, wxcode::wxTreeListCtrl)
    EVT_TREE_ITEM_RIGHT_CLICK(-1, VariableViewer::OnRightClick)
    EVT_TREE_ITEM_ACTIVATED(-1, VariableViewer::OnDoubleClick)
    EVT_MENU_RANGE(ID_VARVIEWER_NEW, ID_VARVIEWER_SAVEAS, VariableViewer::OnMenuEvent)
END_EVENT_TABLE()


// Constructor
VariableViewer::VariableViewer(wxWindow* parent, NumeReWindow* mainWin, int fieldsize) : wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_NO_LINES | wxTR_HIDE_ROOT)
{
    debuggerMode = false;
    selectedID = wxTreeItemId();

    nDataFieldSize = fieldsize;
    mainWindow = mainWin;

    // Create the default columns
    AddColumn(_guilang.get("GUI_VARVIEWER_NAME"), 150);
    AddColumn(_guilang.get("GUI_VARVIEWER_DIM"), 80, wxALIGN_RIGHT);
    AddColumn(_guilang.get("GUI_VARVIEWER_CLASS"), 55);
    AddColumn(_guilang.get("GUI_VARVIEWER_VALUE"), fieldsize);

    // Create root node
    AddRoot("ROOT");

    // Create variable class nodes
    numRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_VARS"));
    stringRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_STRINGS"));
    tableRoot = AppendItem(GetRootItem(), _guilang.get("GUI_VARVIEWER_TABLES"));
    SetItemText(tableRoot, VALUECOLUMN, " {min, ..., max}");

    // Make the variable class nodes bold
    SetItemBold(numRoot, true);
    SetItemBold(stringRoot, true);
    SetItemBold(tableRoot, true);
}

// This member function checks, whether a variable was already
// part of the previous variable set (only used in debug mode)
bool VariableViewer::checkPresence(const std::string& sVar)
{
    for (size_t i = 0; i < vLastVarSet.size(); i++)
    {
        if (vLastVarSet[i] == sVar)
            return true;
    }

    return false;
}

// This member functions checks for special variable names,
// which is used to highlight them out of the debug mode
bool VariableViewer::checkSpecialVals(const std::string& sVar)
{
    if (sVar.substr(0, sVar.find('\t')) == "data()")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "cache()")
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
    else if (sVar.substr(0, sVar.find('\t')) == "nlines")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "ncols")
        return true;
    else if (sVar.substr(0, sVar.find('\t')) == "ans")
        return true;

    return false;
}

// This member function splits the passed variable at
// tabulator characters and adds its contents to the
// current tree item. It will also create the tooltip
// for this item
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
    tooltip += " = " + sVar.substr(0, sVar.find('\t'));
    SetItemToolTip(currentItem, tooltip);

    // Set the internal variable's name as a
    // VarData object
    SetItemData(currentItem, new VarData(sVar.substr(sVar.find('\t')+1)));

    return currentItem;
}

// A simple helper function to clean the tree
void VariableViewer::ClearTree()
{
    DeleteChildren(numRoot);
    DeleteChildren(stringRoot);
    DeleteChildren(tableRoot);
}

// This member function handles every task, which
// is specific to the debug mode after a variable update
void VariableViewer::HandleDebugActions(const std::vector<std::string>& vVarList)
{
    if (!debuggerMode)
        return;

    vLastVarSet = vVarList;
}

wxString VariableViewer::GetInternalName(wxTreeItemId id)
{
    return static_cast<VarData*>(GetItemData(id))->sInternalName;
}

// This member function handles the menu events created
// from the popup menu and redirects the control to the
// corresponding functions
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
    }
}

// This member function displays a text entry dialog
// to enter the new table names and sends the corresponding
// command to the kernel
void VariableViewer::OnNewTable()
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_NEWTABLE_QUESTION"), _guilang.get("GUI_VARVIEWER_NEWTABLE"), "table()");

    if (textEntry.ShowModal() != wxID_OK)
        return;

    wxString tables = textEntry.GetValue();

    // Ensure that the user entered the necessary parentheses
    if (tables.find(',') != string::npos)
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
            if (tables.find(',') != string::npos)
                tables.erase(0, tables.find(',')+1);
            else
                break;
        }

        tables = tablesTemp;
    }
    else if (tables.find("()") == string::npos)
    {
        // Add the needed parentheses directly at the end of
        // the table name string
        tables.insert(tables.find_last_not_of(' ')+1, "()");
    }

    mainWindow->pass_command("new " + tables + " -free");
}

// This member function displays the selected table
void VariableViewer::OnShowTable(const wxString& table, const wxString& tableDisplayName)
{
    mainWindow->showTable(table, tableDisplayName);
}

// This member function displays a text entry dialog
// to choose a new name for the selected table
void VariableViewer::OnRenameTable(const wxString& table)
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_NEWNAME_QUESTION"), _guilang.get("GUI_VARVIEWER_NEWNAME"), table);

    if (textEntry.ShowModal() != wxID_OK)
        return;

    mainWindow->pass_command("rename " + table + ", " + textEntry.GetValue());
}

// This member function removes the selected table
void VariableViewer::OnRemoveTable(const wxString& table)
{
    mainWindow->pass_command("remove " + table);
}

// This member function saves the selected table
void VariableViewer::OnSaveTable(const wxString& table)
{
    mainWindow->pass_command("save " + table);
}

// This member function displays a text entry dialog
// ot choose the file name for the selected table
void VariableViewer::OnSaveasTable(const wxString& table)
{
    wxTextEntryDialog textEntry(this, _guilang.get("GUI_VARVIEWER_SAVENAME_QUESTION"), _guilang.get("GUI_VARVIEWER_SAVENAME"), table);

    if (textEntry.ShowModal() != wxID_OK)
        return;

    mainWindow->pass_command("save " + table + " -file=\"" + textEntry.GetValue() + "\"");
}

// This member function expands all nodes, which
// contain child nodes.
void VariableViewer::ExpandAll()
{
    if (HasChildren(numRoot))
        Expand(numRoot);

    if (HasChildren(stringRoot))
        Expand(stringRoot);

    if (HasChildren(tableRoot))
        Expand(tableRoot);
}

// This member function creates the pop-up menu
// in the var viewer case (it does nothing in
// debugger mode and also nothing for non-tables).
void VariableViewer::OnRightClick(wxTreeEvent& event)
{
    // do nothing in the debugger case
    if (debuggerMode)
        return;

    // do nothing, if the parent node is not the
    // table root node
    if (GetItemParent(event.GetItem()) != tableRoot)
        return;

    // Get the name of the table and determine, whether
    // the table may be modified
    wxString itemLabel = GetItemText(event.GetItem());
    bool bMayBeModified = itemLabel != "data()" && itemLabel != "string()" && itemLabel != "cache()";

    // Create the menu
    wxMenu popUpmenu;

    // Append commons
    popUpmenu.Append(ID_VARVIEWER_NEW, _guilang.get("GUI_VARVIEWER_MENU_NEWTABLE"));
    popUpmenu.AppendSeparator();
    popUpmenu.Append(ID_VARVIEWER_SHOW, _guilang.get("GUI_VARVIEWER_MENU_SHOWVALUE"));
    popUpmenu.Append(ID_VARVIEWER_RENAME, _guilang.get("GUI_VARVIEWER_MENU_RENAME"));
    popUpmenu.Append(ID_VARVIEWER_REMOVE, _guilang.get("GUI_VARVIEWER_MENU_REMOVE"));

    // Add stuff, for non-string tables
    if (itemLabel != "string()")
    {
        popUpmenu.AppendSeparator();
        popUpmenu.Append(ID_VARVIEWER_SAVE, _guilang.get("GUI_VARVIEWER_MENU_SAVE"));
        popUpmenu.Append(ID_VARVIEWER_SAVEAS, _guilang.get("GUI_VARVIEWER_MENU_SAVEAS"));
    }

    // Disable menu items for tables, which
    // cannot be modified
    if (!bMayBeModified)
    {
        popUpmenu.Enable(ID_VARVIEWER_RENAME, false);
        popUpmenu.Enable(ID_VARVIEWER_REMOVE, false);
    }

    // Store the selected item for the menu event handler
    selectedID = event.GetItem();

    // Display the menu
    PopupMenu(&popUpmenu, event.GetPoint());
}

// This event handler displays the selected table
void VariableViewer::OnDoubleClick(wxTreeEvent& event)
{
    if (GetItemParent(event.GetItem()) != tableRoot)
        return;

    OnShowTable(GetInternalName(event.GetItem()), GetItemText(event.GetItem()));
}

// This member function is used to update the variable
// list, which is displayed by this control.
void VariableViewer::UpdateVariables(const std::vector<std::string>& vVarList, size_t nNumerics, size_t nStrings, size_t nTables)
{
    // Clear the tree first
    ClearTree();

    // Show the numbers of variables in the current group
    SetItemText(numRoot, DIMCOLUMN, "[" + toString(nNumerics) + "] ");
    SetItemText(stringRoot, DIMCOLUMN, "[" + toString(nStrings) + "] ");
    SetItemText(tableRoot, DIMCOLUMN, "[" + toString(nTables) + "] ");

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
        else
        {
            // Append a variable to the tables list
            currentItem = AppendVariable(tableRoot, vVarList[i]);
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

