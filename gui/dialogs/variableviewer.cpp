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

#define DIMCOLUMN 1
#define CLASSCOLUMN 2
#define VALUECOLUMN 3

std::string toString(int nNum);

extern Language _guilang;
using namespace wxcode;

VariableViewer::VariableViewer(wxWindow* parent, int fieldsize)
{
    debuggerMode = false;

    nDataFieldSize = fieldsize;

    control = new wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_NO_LINES | wxTR_HIDE_ROOT);

    control->AddColumn(_guilang.get("GUI_VARVIEWER_NAME"), 150);
    control->AddColumn(_guilang.get("GUI_VARVIEWER_DIM"), 60, wxALIGN_RIGHT);
    control->AddColumn(_guilang.get("GUI_VARVIEWER_CLASS"), 55);
    control->AddColumn(_guilang.get("GUI_VARVIEWER_VALUE"), fieldsize);

    control->AddRoot("ROOT");

    numRoot = control->AppendItem(control->GetRootItem(), _guilang.get("GUI_VARVIEWER_VARS"));
    stringRoot = control->AppendItem(control->GetRootItem(), _guilang.get("GUI_VARVIEWER_STRINGS"));
    tableRoot = control->AppendItem(control->GetRootItem(), _guilang.get("GUI_VARVIEWER_TABLES"));

    control->SetItemBold(numRoot, true);
    control->SetItemBold(stringRoot, true);
    control->SetItemBold(tableRoot, true);
}

bool VariableViewer::checkPresence(const std::string& sVar)
{
    for (size_t i = 0; i < vLastVarSet.size(); i++)
    {
        if (vLastVarSet[i] == sVar)
            return true;
    }

    return false;
}

wxTreeItemId VariableViewer::AppendVariable(wxTreeItemId rootNode, std::string sVar)
{
    wxTreeItemId currentItem = control->AppendItem(rootNode, sVar.substr(0, sVar.find('\t')));
    sVar.erase(0, sVar.find('\t')+1);
    control->SetItemText(currentItem, DIMCOLUMN, sVar.substr(0, sVar.find('\t')) + " ");
    sVar.erase(0, sVar.find('\t')+1);
    control->SetItemText(currentItem, CLASSCOLUMN, " " + sVar.substr(0, sVar.find('\t')));
    control->SetItemText(currentItem, VALUECOLUMN, " " + sVar.substr(sVar.find('\t')+1));

    return currentItem;
}

void VariableViewer::ClearTree()
{
    control->DeleteChildren(numRoot);
    control->DeleteChildren(stringRoot);
    control->DeleteChildren(tableRoot);
}

void VariableViewer::HandleDebugActions(const std::vector<std::string>& vVarList)
{
    if (!debuggerMode)
        return;

    vLastVarSet = vVarList;
}

void VariableViewer::ExpandAll()
{
    if (control->HasChildren(numRoot))
        control->Expand(numRoot);

    if (control->HasChildren(stringRoot))
        control->Expand(stringRoot);

    if (control->HasChildren(tableRoot))
        control->Expand(tableRoot);
}

void VariableViewer::UpdateVariables(const std::vector<std::string>& vVarList, size_t nNumerics, size_t nStrings, size_t nTables)
{
    ClearTree();

    control->SetItemText(numRoot, DIMCOLUMN, "[" + toString(nNumerics) + "] ");
    control->SetItemText(stringRoot, DIMCOLUMN, "[" + toString(nStrings) + "] ");
    control->SetItemText(tableRoot, DIMCOLUMN, "[" + toString(nTables) + "] ");

    control->SetItemText(tableRoot, VALUECOLUMN, " {min, ..., max}");

    wxTreeItemId currentItem;

    for (size_t i = 0; i < vVarList.size(); i++)
    {
        if (i < nNumerics)
        {
            currentItem = AppendVariable(numRoot, vVarList[i]);
        }
        else if (i < nStrings+nNumerics)
        {
            currentItem = AppendVariable(stringRoot, vVarList[i]);
        }
        else
        {
            currentItem = AppendVariable(tableRoot, vVarList[i]);
        }

        if (debuggerMode && !checkPresence(vVarList[i]))
            control->SetItemTextColour(currentItem, *wxRED);
    }

    ExpandAll();

    HandleDebugActions(vVarList);
}

