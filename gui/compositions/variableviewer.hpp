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


#ifndef VARIABLEVIEWER_HPP
#define VARIABLEVIEWER_HPP

#include "../controls/treelistctrl.h"
//#include <wx/treelist.h>
#include <map>
#include <vector>
#include <string>

class NumeReWindow;

class VariableViewer : public wxcode::wxTreeListCtrl
{
    private:
        bool debuggerMode;
        std::vector<std::string> vLastVarSet;
        int nDataFieldSize;
        bool bExpandedState[6];
        wxTreeItemId numRoot;
        wxTreeItemId stringRoot;
        wxTreeItemId tableRoot;
        wxTreeItemId clusterRoot;
        wxTreeItemId argumentRoot;
        wxTreeItemId globalRoot;
        wxTreeItemId selectedID;

        NumeReWindow* mainWindow;

        bool checkPresence(const std::string& sVar);
        bool checkSpecialVals(const std::string& sVar);
        wxTreeItemId AppendVariable(wxTreeItemId rootNode, std::string sVar);
        void ClearTree();
        void HandleDebugActions(const std::vector<std::string>& vVarList);
        wxString GetInternalName(wxTreeItemId id);

        void OnMenuEvent(wxCommandEvent& event);

        void OnNewTable();
        void OnShowTable(const wxString& table, const wxString& tableDisplayName);
        void OnEditTable(const wxString& table);
        void OnRenameTable(const wxString& table);
        void OnRemoveTable(const wxString& table);
        void OnSaveTable(const wxString& table);
        void OnSaveasTable(const wxString& table);
        void OnCopyValue(const wxString& value);


    public:
        VariableViewer(wxWindow* parent, NumeReWindow* mainWin, int fieldsize = 300, bool debugMode = false);

        void ExpandAll();
        void OnRightClick(wxTreeEvent& event);
        void OnDoubleClick(wxTreeEvent& event);

        void setDebuggerMode(bool mode = true);
        void UpdateVariables(const std::vector<std::string>& vVarList, size_t nNumerics, size_t nStrings, size_t nTables, size_t nClusters, size_t nArguments = 0, size_t nGlobals = 0);

        DECLARE_EVENT_TABLE();
};


#endif // VARIABLEVIEWER_HPP

