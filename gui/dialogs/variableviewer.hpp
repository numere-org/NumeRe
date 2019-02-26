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

#include "treelistctrl.h"
//#include <wx/treelist.h>
#include <map>
#include <vector>
#include <string>

class VariableViewer
{
    private:
        bool debuggerMode;
        std::vector<std::string> vLastVarSet;
        int nDataFieldSize;
        wxTreeItemId numRoot;
        wxTreeItemId stringRoot;
        wxTreeItemId tableRoot;

        bool checkPresence(const std::string& sVar);
        bool checkSpecialVals(const std::string& sVar);
        wxTreeItemId AppendVariable(wxTreeItemId rootNode, std::string sVar);
        void ClearTree();
        void HandleDebugActions(const std::vector<std::string>& vVarList);


    public:
        wxcode::wxTreeListCtrl* control;
        void ExpandAll();

        VariableViewer(wxWindow* parent, int fieldsize = 600);

        void setDebuggerMode(bool mode = true)
        {
            debuggerMode = mode;
        }

        void UpdateVariables(const std::vector<std::string>& vVarList, size_t nNumerics, size_t nStrings, size_t nTables);
};


#endif // VARIABLEVIEWER_HPP

