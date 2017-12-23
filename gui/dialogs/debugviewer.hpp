/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#ifndef DEBUGVIEWER_HPP
#define DEBUGVIEWER_HPP

#include "../viewerframe.hpp"
#include "../terminal/wxterm.h"
#include "treelistctrl.h"
//#include <wx/treelist.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <string>
#include <vector>

class DebugViewer : public ViewerFrame
{
    private:
        wxTreeListCtrl* m_vartreelist;
        wxListCtrl* m_moduleinfos;
        wxListCtrl* m_stacktrace;
        wxTerm* m_terminal;
        bool b_transferredControl;
        vector<string> vLastVarSet;

        bool checkPresence(const string& sVar);
        string removeControlSymbols(string sCommandLine);

    public:
        DebugViewer(wxWindow* parent, const wxString& title = "NumeRe: Debugger");

        void setTerminal(wxTerm* term) {m_terminal = term;}

        void setDebugInfo(const wxString& title, const vector<string>& vModuleInfo, const vector<string>& vStack, const vector<string>& vVarList, size_t n_num = 0); //n_num = number of numerical vars => strings may be calculated

        void OnButtonContinue(wxCommandEvent& event);
        void OnButtonCancel(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE();
};


#endif // DEBUGVIEWER_HPP

