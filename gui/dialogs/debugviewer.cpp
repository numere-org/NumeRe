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

#include "debugviewer.hpp"
#include <wx/statbox.h>
#include "../../kernel/core/ui/language.hpp"

#define ID_DEBUG_CONTINUE 10201
#define ID_DEBUG_CANCEL 10202

extern Language _guilang;

BEGIN_EVENT_TABLE(DebugViewer, ViewerFrame)
    EVT_BUTTON(ID_DEBUG_CONTINUE, DebugViewer::OnButtonContinue)
    EVT_BUTTON(ID_DEBUG_CANCEL, DebugViewer::OnButtonCancel)
    EVT_CLOSE (DebugViewer::OnClose)
END_EVENT_TABLE()

DebugViewer::DebugViewer(wxWindow* parent, const wxString& title) : ViewerFrame(parent, title)
{
    b_transferredControl = false;
    // initialize the controls
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* buttonsizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* moduleBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_MODULE"));
    wxStaticBoxSizer* stackBox = new wxStaticBoxSizer(wxHORIZONTAL,panel, _guilang.get("DBG_STACKTRACE"));
    wxStaticBoxSizer* varBox = new wxStaticBoxSizer(wxHORIZONTAL,panel, _guilang.get("DBG_LOCALS"));

    m_moduleinfos = new wxListCtrl(moduleBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(600, 100), wxLC_REPORT);
    m_moduleinfos->AppendColumn(_guilang.get("DBG_TYPE"));
    m_moduleinfos->AppendColumn(_guilang.get("DBG_VALUE"));
    m_moduleinfos->InsertItem(0,_guilang.get("DBG_EXPR"));
    m_moduleinfos->InsertItem(1,_guilang.get("DBG_FILE"));
    m_moduleinfos->InsertItem(2,_guilang.get("DBG_LINENO"));
    m_moduleinfos->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_stacktrace = new wxListCtrl(stackBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(300, -1), wxLC_REPORT);
    m_stacktrace->AppendColumn("Stack");

    m_varViewer = new VariableViewer(varBox->GetStaticBox(), (NumeReWindow*)parent);
    m_varViewer->setDebuggerMode(true);

    moduleBox->Add(m_moduleinfos, 1, wxALIGN_CENTER_HORIZONTAL | wxALL);
    stackBox->Add(m_stacktrace, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);
    varBox->Add(m_varViewer, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);

    wxButton* buttonContinue = new wxButton(panel, ID_DEBUG_CONTINUE, _guilang.get("DBG_CONTINUE"));
    wxButton* buttonCancel = new wxButton(panel, ID_DEBUG_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"));

    buttonsizer->Add(buttonContinue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonsizer->Add(buttonCancel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    vsizer->Add(moduleBox, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->AddSpacer(10);
    hsizer->Add(stackBox, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    hsizer->AddSpacer(5);
    hsizer->Add(varBox, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    vsizer->Add(hsizer, 3, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->Add(buttonsizer, 0, wxALIGN_RIGHT, 5);
    panel->SetSizer(vsizer);
}

string DebugViewer::removeControlSymbols(string sCommandLine)
{
    while (sCommandLine.find("\\$") != string::npos)
        sCommandLine.erase(sCommandLine.find("\\$"),1);
    return sCommandLine;
}

void DebugViewer::setDebugInfo(const wxString& title, const vector<string>& vModuleInfo, const vector<string>& vStack, const vector<string>& vVarList, size_t n_num, size_t s_num, size_t t_num)
{
    this->SetTitle(title);
    b_transferredControl = true;
    m_moduleinfos->SetItem(0,1,this->removeControlSymbols(vModuleInfo[0]));
    m_moduleinfos->SetItem(1,1,vModuleInfo[1]);
    m_moduleinfos->SetItem(2,1,vModuleInfo[2]);
    m_moduleinfos->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_stacktrace->DeleteAllItems();

    for (size_t i = 0; i < vStack.size(); i++)
    {
        m_stacktrace->InsertItem(i, vStack[i]);
    }
    m_stacktrace->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_varViewer->UpdateVariables(vVarList, n_num, s_num, t_num);
}

void DebugViewer::OnButtonContinue(wxCommandEvent& event)
{
    if (b_transferredControl)
        m_terminal->continueDebug();
}

void DebugViewer::OnButtonCancel(wxCommandEvent& event)
{
    if (b_transferredControl)
    {
        m_terminal->CancelCalculation();
        m_terminal->continueDebug();
    }
}

void DebugViewer::OnClose(wxCloseEvent& event)
{
    if (b_transferredControl)
        m_terminal->continueDebug();
    this->Hide();
    event.Veto();
}
