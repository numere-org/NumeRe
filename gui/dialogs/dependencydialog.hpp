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
#include "../controls/treelistctrl.h"
#include "../../kernel/core/procedure/procedurelibrary.hpp"
#include "../../kernel/core/procedure/dependency.hpp"

/////////////////////////////////////////////////
/// \brief This class represents a dialog showing
/// the dependencies of a selected procedure file.
/////////////////////////////////////////////////
class DependencyDialog : public wxDialog
{
    private:
        wxcode::wxTreeListCtrl* m_dependencyTree;

        std::string calculateDependencies(ProcedureLibrary& lib, const std::string& mainfile, std::map<std::string, DependencyList>& mDeps);
        void fillDependencyTree(const std::string& sMainProcedure, std::map<std::string, DependencyList>& mDeps);
        void insertChilds(wxTreeItemId item, const std::string& sParentProcedure, std::map<std::string, DependencyList>& mDeps);
        bool findInParents(wxTreeItemId item, const std::string& sCurrProc);

        void OnItemActivate(wxTreeEvent& event);

    public:
        DependencyDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::string& mainfile, ProcedureLibrary& lib, long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        DECLARE_EVENT_TABLE();
};


#endif // DEPENDENCYDIALOG_HPP


