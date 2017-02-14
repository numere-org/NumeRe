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

#ifndef TABLEEDITPANEL_HPP
#define TABLEEDITPANEL_HPP

#include <wx/wx.h>
#include <wx/panel.h>
#include "tableviewer.hpp"
#include "wxterm.h"
#include "../kernel/core/language.hpp"

extern Language _guilang;

class TableEditPanel : public wxPanel
{
    private:
        wxTerm* m_terminal;
        wxBoxSizer* vsizer;
        wxBoxSizer* hsizer;
        bool finished;

    public:
        TableViewer* grid;

        TableEditPanel(wxFrame* parent, wxWindowID id);

        void SetTerminal(wxTerm* term) {m_terminal = term;}

        void OnButtonOk(wxCommandEvent& event);
        void OnButtonCancel(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);

        DECLARE_EVENT_TABLE();
};

#endif // TABLEEDITPANEL_HPP

