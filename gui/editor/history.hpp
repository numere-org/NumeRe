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


#ifndef HISTORY_HPP
#define HISTORY_HPP

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/menu.h>
#include <wx/dynarray.h>
#include "editor.h"
#include "../terminal/terminal.hpp"
#include "../../common/datastructures.h"
#include "../../kernel/syntax.hpp"

#define RM_WS_BOTH 0
#define RM_WS_FRONT 1
#define RM_WS_BACK 2

class NumeReHistory : public NumeReEditor
{
public:
	NumeReHistory(NumeReWindow* mframe, Options* options,
					wxWindow *parent, wxWindowID id, NumeReSyntax* __syntax, NumeReTerminal* __terminal, const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize, long style = 0,
					const wxString& name = wxSTCNameStr);
	~NumeReHistory();

    void UpdateSyntaxHighlighting(bool forceUpdate = false);
    void AddToHistory(const wxString& commandstring);
    void OnMarginClick(wxStyledTextEvent &event);
    void OnMouseDblClk(wxMouseEvent& event);
    void OnMouseDn(wxMouseEvent &event);
    void OnRightClick(wxMouseEvent &event);
    void OnMenuEvent(wxCommandEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

private:
    void deleteLine();
    void copyLine();
    void loadHistory();
    void saveHistory();
    void addHeader();
    void applyFoldPoints();

	NumeReSyntax* _syntax;
	NumeReTerminal* m_terminal;
	NumeReWindow* m_mainframe;
	int m_clickedLine;

	DECLARE_EVENT_TABLE()
};


//BEGIN_EVENT_TABLE(ChameleonEditor, wxStyledTextCtrl)

#endif
