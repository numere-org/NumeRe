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

#include "viewerframe.hpp"
#include "../terminal/terminal.hpp"
#include "../../common/Options.h"
#include "variableviewer.hpp"
//#include <wx/treelist.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <string>
#include <vector>

class DebugViewer : public ViewerFrame
{
    private:
        VariableViewer* m_varViewer;
        wxTextCtrl* m_expression;
        wxTextCtrl* m_lineNumber;
        wxTextCtrl* m_errorMessage;
        wxListCtrl* m_stacktrace;
        NumeReTerminal* m_terminal;
        Options* m_options;

        bool b_transferredControl;
        int nLineColumn;
        int nModuleColumn;

        void setExpression(const std::string& sLineNumber, const std::string& sExpression);

        void getInformationByStackId(size_t id);
        void OnStackItemActivate(wxListEvent& event);
        void OnMenuEvent(wxCommandEvent& event);

        void EnableDebugger(bool enable);

    public:
        DebugViewer(wxWindow* parent, Options* _options, const wxString& title = "NumeRe: Debugger");

        void initializeToolbar();
        void updateSettings();
        void setTerminal(NumeReTerminal* term) {m_terminal = term;}
        void setDebugInfo(const wxString& title, const std::vector<std::string>& vStack);
        void OnClose(wxCloseEvent& event);
        void OnDebugCancel();
        void OnExecutionFinished();

        bool hasControl() const
        {
            return b_transferredControl;
        }

    DECLARE_EVENT_TABLE();
};


#endif // DEBUGVIEWER_HPP

