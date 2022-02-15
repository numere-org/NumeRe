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

#ifndef DEPENDENCYDIALOG_HPP
#define DEPENDENCYDIALOG_HPP

#include <wx/wx.h>
#include <string>
#include <map>
#include <vector>
#include "../controls/treelistctrl.h"
#include "../../kernel/core/procedure/procedurelibrary.hpp"
#include "../../kernel/core/procedure/dependency.hpp"
#include "../../common/datastructures.h"

/////////////////////////////////////////////////
/// \brief This class represents a dialog showing
/// the dependencies of a selected procedure file.
/////////////////////////////////////////////////
class DependencyDialog : public wxDialog
{
    private:
        wxcode::wxTreeListCtrl* m_dependencyTree;
        wxTreeItemId m_selectedItem;
        std::string m_mainProcedure;
        std::map<std::string, DependencyList> m_deps;

        void calculateDependencies(ProcedureLibrary& lib, const std::string& mainfile);
        void fillDependencyTree();
        void insertChilds(wxTreeItemId item, const std::string& sParentProcedure);
        bool findInParents(wxTreeItemId item, const std::string& sCurrProc);

        void CollapseAll(wxTreeItemId item);
        std::vector<std::string> parseNameSpace(std::string sNameSpace) const;
        int calculateClusterLevel(const std::vector<std::string>& sCurrentNameSpace, const std::vector<std::string>& sNewNameSpace);
        void CreateDotFile();

        void OnItemActivate(wxTreeEvent& event);
        void OnItemRightClick(wxTreeEvent& event);
        void OnMenuEvent(wxCommandEvent& event);

    public:
        DependencyDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::string& mainfile, ProcedureLibrary& lib, long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        DECLARE_EVENT_TABLE();
};


#endif // DEPENDENCYDIALOG_HPP


