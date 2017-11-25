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
#include "../../kernel/core/language.hpp"

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

    m_stacktrace = new wxListCtrl(stackBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(200, 400), wxLC_REPORT);
    m_stacktrace->AppendColumn("Stack");

    m_vartreelist = new wxTreeListCtrl(varBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_NO_LINES | wxTR_HIDE_ROOT);

    moduleBox->Add(m_moduleinfos, 1, wxALIGN_CENTER_HORIZONTAL | wxALL);
    stackBox->Add(m_stacktrace, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);
    varBox->Add(m_vartreelist, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);

    m_vartreelist->AddColumn(_guilang.get("DBG_NAME"), 200);
    m_vartreelist->AddColumn(_guilang.get("DBG_VALUE"), 600);
    m_vartreelist->AddRoot("ROOT");

    wxButton* buttonContinue = new wxButton(panel, ID_DEBUG_CONTINUE, _guilang.get("DBG_CONTINUE"));
    wxButton* buttonCancel = new wxButton(panel, ID_DEBUG_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"));

    buttonsizer->Add(buttonContinue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonsizer->Add(buttonCancel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    vsizer->Add(moduleBox, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->AddSpacer(10);
    hsizer->Add(stackBox, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    hsizer->AddSpacer(5);
    hsizer->Add(varBox, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    vsizer->Add(hsizer, 3, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->Add(buttonsizer, 0, wxALIGN_RIGHT, 5);
    panel->SetSizer(vsizer);
}

bool DebugViewer::checkPresence(const string& sVar)
{
    for (size_t i = 0; i < vLastVarSet.size(); i++)
    {
        if (vLastVarSet[i] == sVar)
            return true;
    }
    return false;
}

string DebugViewer::removeControlSymbols(string sCommandLine)
{
    while (sCommandLine.find("\\$") != string::npos)
        sCommandLine.erase(sCommandLine.find("\\$"),1);
    return sCommandLine;
}

void DebugViewer::setDebugInfo(const wxString& title, const vector<string>& vModuleInfo, const vector<string>& vStack, const vector<string>& vVarList, size_t n_num)
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


    m_vartreelist->DeleteChildren(m_vartreelist->GetRootItem());

    //size_t firstcol = 0, secondcol = 0;
    wxTreeItemId numroot = m_vartreelist->AppendItem(m_vartreelist->GetRootItem(), _guilang.get("DBG_LOCALVARS"));
    wxTreeItemId stringroot = m_vartreelist->AppendItem(m_vartreelist->GetRootItem(), _guilang.get("DBG_LOCALSTRINGS"));

    m_vartreelist->SetItemText(numroot, 1, "[" + toString(n_num) + "]");
    m_vartreelist->SetItemText(stringroot, 1, "[" + toString(vVarList.size()-n_num) + "]");

    m_vartreelist->SetItemBold(numroot, true);
    m_vartreelist->SetItemBold(stringroot, true);

    /*firstcol = 1.5*GetTextExtent(_guilang.get("DBG_LOCALVARS")).GetWidth();
    if (firstcol < 1.5*GetTextExtent(_guilang.get("DBG_LOCALSTRINGS")).GetWidth())
        firstcol = 1.5*GetTextExtent(_guilang.get("DBG_LOCALSTRINGS")).GetWidth();*/

    wxTreeItemId currentItem;
    for (size_t i = 0; i < vVarList.size(); i++)
    {
        if (i < n_num)
        {
            currentItem = m_vartreelist->AppendItem(numroot, vVarList[i].substr(0, vVarList[i].find('\t')));
            m_vartreelist->SetItemText(currentItem, 1, vVarList[i].substr(vVarList[i].find('\t')+1));
        }
        else
        {
            currentItem = m_vartreelist->AppendItem(stringroot, vVarList[i].substr(0, vVarList[i].find('\t')));
            m_vartreelist->SetItemText(currentItem, 1, vVarList[i].substr(vVarList[i].find('\t')+1));
        }

        if (!checkPresence(vVarList[i]))
            m_vartreelist->SetItemTextColour(currentItem, *wxRED);

        /*if (firstcol < GetTextExtent(vVarList[i].substr(0, vVarList[i].find('\t'))).GetWidth()+m_vartreelist->GetIndent())
            firstcol = GetTextExtent(vVarList[i].substr(0, vVarList[i].find('\t'))).GetWidth()+m_vartreelist->GetIndent();

        if (secondcol < GetTextExtent(vVarList[i].substr(vVarList[i].find('\t')+1)).GetWidth())
            secondcol = GetTextExtent(vVarList[i].substr(vVarList[i].find('\t')+1)).GetWidth();*/
    }

    /*m_vartreelist->SetColumnWidth(0, firstcol);
    m_vartreelist->SetColumnWidth(1, secondcol*1.4);

    m_vartreelist->Refresh();*/

    if (m_vartreelist->HasChildren(numroot))
        m_vartreelist->Expand(numroot);
    if (m_vartreelist->HasChildren(stringroot))
        m_vartreelist->Expand(stringroot);
    vLastVarSet = vVarList;
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
