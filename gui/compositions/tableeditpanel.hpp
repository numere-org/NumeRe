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
#include "../terminal/terminal.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;

/////////////////////////////////////////////////
/// \brief Generic table panel, which also
/// contains all meta UI elements.
/////////////////////////////////////////////////
class TablePanel : public wxPanel
{
    protected:
        wxTextCtrl* m_commentField;
        wxTextCtrl* m_sourceField;
        wxBoxSizer* vsizer;
        wxBoxSizer* hsizer;
        wxStaticText* m_lastSaveText;
        bool finished;

    public:
        TableViewer* grid;

        TablePanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar, bool readOnly = true);
        void update(const NumeRe::TableMetaData& meta);
        std::string getComment() const;

        void OnClose(wxCloseEvent& event);

        DECLARE_EVENT_TABLE();
};



/////////////////////////////////////////////////
/// \brief A table panel with editing
/// functionalities. Creates APPLY and CANCEL
/// buttons and provides their event handlers.
/////////////////////////////////////////////////
class TableEditPanel : public TablePanel
{
    private:
        NumeReTerminal* m_terminal;

    public:
        TableEditPanel(wxFrame* parent, wxWindowID id, wxStatusBar* statusbar);

        void SetTerminal(NumeReTerminal* term) {m_terminal = term;}

        void OnButtonOk(wxCommandEvent& event);
        void OnButtonCancel(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);

        DECLARE_EVENT_TABLE();
};

#endif // TABLEEDITPANEL_HPP

