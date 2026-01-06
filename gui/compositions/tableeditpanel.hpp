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
#include <wx/infobar.h>
#include "tableviewer.hpp"
#include "../terminal/terminal.hpp"
#include "../guilang.hpp"

class TextField;
class NumeReWindow;

/////////////////////////////////////////////////
/// \brief Generic table panel, which also
/// contains all meta UI elements.
/////////////////////////////////////////////////
class TablePanel : public wxPanel
{
    protected:
        TextField* m_commentField;
        wxTextCtrl* m_sourceField;
        wxBoxSizer* vsizer;
        wxBoxSizer* hsizer;
        wxStaticText* m_lastSaveText;
        wxInfoBar* m_infoBar;
        bool finished;

        NumeReTerminal* m_terminal;
        wxFrame* m_topLevelFrame;

    public:
        TableViewer* grid;

        TablePanel(wxWindow* parent, wxFrame* toplevel, wxWindowID id, wxStatusBar* statusbar, NumeReWindow* topWindow, bool readOnly = true);
        void update(const NumeRe::TableMetaData& meta);
        void SetTerminal(NumeReTerminal* term) {m_terminal = term;}
        NumeReTerminal* GetTerminal() {return m_terminal;};
        std::string getComment() const;
        void showMessage(const wxString& msg);
        void dismissMessage();
        wxMenuBar* getMenuBar();
        wxFrame* getFrame();
        void ready();

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
    public:
        TableEditPanel(wxWindow* parent, wxWindowID id, wxStatusBar* statusbar, NumeReWindow* topWindow);

        void OnButtonOk(wxCommandEvent& event);
        void OnButtonCancel(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);

        DECLARE_EVENT_TABLE();
};

#endif // TABLEEDITPANEL_HPP

