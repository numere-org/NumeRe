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
#define ID_DEBUG_STEP 10203
#define ID_DEBUG_STACK_VIEWER 10204

extern Language _guilang;

BEGIN_EVENT_TABLE(DebugViewer, ViewerFrame)
    EVT_CLOSE (DebugViewer::OnClose)
    EVT_LIST_ITEM_ACTIVATED(ID_DEBUG_STACK_VIEWER, DebugViewer::OnStackItemActivate)
    EVT_MENU_RANGE(ID_DEBUG_CONTINUE, ID_DEBUG_STEP, DebugViewer::OnMenuEvent)
END_EVENT_TABLE()

DebugViewer::DebugViewer(wxWindow* parent, const wxString& title) : ViewerFrame(parent, title)
{
    wxToolBar* tb = CreateToolBar(wxTB_HORZ_TEXT);
    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_CONTINUE, _guilang.get("DBG_CONTINUE"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR));
    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_STEP, _guilang.get("DBG_STEP"), wxArtProvider::GetBitmap(wxART_GOTO_LAST, wxART_TOOLBAR));
    tb->AddStretchableSpace();
    tb->AddTool(ID_DEBUG_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"), wxArtProvider::GetBitmap(wxART_QUIT, wxART_TOOLBAR));
    //tb->AddTool(-1, "Label", wxArtProvider::GetBitmap(wxART_GOTO_FIRST, wxART_TOOLBAR));
    tb->Realize();
    b_transferredControl = false;
    // initialize the controls
    wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* exprBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_EXPR"));
    wxStaticBoxSizer* errorBox = new wxStaticBoxSizer(wxVERTICAL, panel, _guilang.get("DBG_MODULE"));
    wxStaticBoxSizer* stackBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_STACKTRACE"));
    wxStaticBoxSizer* varBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_LOCALS"));

    m_expression = new wxTextCtrl(exprBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_RICH);
    m_errorMessage = new wxTextCtrl(errorBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    wxFont font;
    font.SetNativeFontInfoUserDesc("consolas 10");
    m_expression->SetFont(font);
    m_expression->SetBackgroundColour(*wxWHITE);
    m_errorMessage->SetForegroundColour(*wxRED);

    m_stacktrace = new wxListCtrl(stackBox->GetStaticBox(), ID_DEBUG_STACK_VIEWER, wxDefaultPosition, wxDefaultSize /*wxSize(400, -1)*/, wxLC_REPORT);
    m_stacktrace->AppendColumn("  ");
    m_stacktrace->AppendColumn("Stack");
    m_stacktrace->AppendColumn(_guilang.get("DBG_LINENO"), wxLIST_FORMAT_RIGHT);
    m_stacktrace->AppendColumn(_guilang.get("DBG_FILE"));

    m_varViewer = new VariableViewer(varBox->GetStaticBox(), (NumeReWindow*)parent);
    m_varViewer->setDebuggerMode(true);

    //moduleBox->Add(m_expression, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND);
    //moduleBox->AddSpacer(10);
    exprBox->Add(m_expression, 1, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND);
    errorBox->Add(m_errorMessage, 1, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND);
    stackBox->Add(m_stacktrace, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);
    varBox->Add(m_varViewer, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);

    hsizer->Add(stackBox, 2, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    hsizer->AddSpacer(10);
    hsizer->Add(errorBox, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 0);
    //vsizer->AddSpacer(5);
    vsizer->Add(exprBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 5);
    vsizer->Add(hsizer, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->Add(varBox, 2, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);
    //vsizer->Add(hsizer, 3, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    panel->SetSizer(vsizer);
    m_expression->SetFocus();
}

string DebugViewer::removeControlSymbols(string sCommandLine)
{
    while (sCommandLine.find("\\$") != string::npos)
        sCommandLine.erase(sCommandLine.find("\\$"),1);
    return sCommandLine;
}

void DebugViewer::OnStackItemActivate(wxListEvent& event)
{
    getInformationByStackId(m_stacktrace->GetItemCount() - event.GetIndex() - 1);
}

void DebugViewer::getInformationByStackId(size_t id)
{
    vector<string> vModuleInfo;
    vector<string> vNumVars;
    vector<string> vStringVars;
    vector<string> vTables;

    {
        wxCriticalSectionLocker lock(m_terminal->m_kernelCS);
        NumeReDebugger& _debugger = m_terminal->_kernel.getDebugger();

        _debugger.select(id);

        vModuleInfo = _debugger.getModuleInformations();
        vNumVars = _debugger.getNumVars();
        vStringVars = _debugger.getStringVars();
        vTables = _debugger.getTables();
    }

    for (int i = 0; i < m_stacktrace->GetItemCount(); i++)
    {
        if (m_stacktrace->GetItemCount() - 1 - i == id)
            m_stacktrace->SetItemText(i, "->");
        else
            m_stacktrace->SetItemText(i, "");
    }

    m_expression->Clear();
    m_expression->SetDefaultStyle(wxTextAttr(wxColour(128, 128, 128)));
    m_expression->AppendText("@("+vModuleInfo[2]+") |  ");
    m_expression->SetDefaultStyle(wxTextAttr(*wxBLACK));
    m_expression->AppendText(removeControlSymbols(vModuleInfo[0]));

    m_errorMessage->SetValue(vModuleInfo[3]);

    size_t n_num = vNumVars.size();
    size_t s_num = vStringVars.size();
    size_t t_num = vTables.size();

    vNumVars.insert(vNumVars.end(), vStringVars.begin(), vStringVars.end());
    vNumVars.insert(vNumVars.end(), vTables.begin(), vTables.end());

    m_varViewer->UpdateVariables(vNumVars, n_num, s_num, t_num);
}

void DebugViewer::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_DEBUG_CONTINUE:
            if (b_transferredControl)
                m_terminal->continueDebug();

            break;
        case ID_DEBUG_CANCEL:
            if (b_transferredControl)
            {
                m_terminal->CancelCalculation();
                m_terminal->continueDebug();
            }

            break;
        case ID_DEBUG_STEP:
            if (b_transferredControl)
                m_terminal->stepDebug();

            break;
    }
}


void DebugViewer::setDebugInfo(const wxString& title, const vector<string>& vStack)
{
    this->SetTitle(title);
    b_transferredControl = true;

    m_stacktrace->DeleteAllItems();

    for (size_t i = 0; i < vStack.size(); i++)
    {
        m_stacktrace->InsertItem(i, "");
        m_stacktrace->SetItem(i, 1, vStack[i].substr(0, vStack[i].find('\t')));
        m_stacktrace->SetItem(i, 2, vStack[i].substr(vStack[i].rfind('\t')+1));
        m_stacktrace->SetItem(i, 3, vStack[i].substr(vStack[i].find('\t')+1, vStack[i].rfind('\t') - vStack[i].find('\t') - 1));
    }

    m_stacktrace->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_stacktrace->SetColumnWidth(1, wxLIST_AUTOSIZE);
    m_stacktrace->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
    m_stacktrace->SetColumnWidth(3, wxLIST_AUTOSIZE);

    if (vStack.size())
        getInformationByStackId(vStack.size()-1);
    else
        getInformationByStackId(0);
}

void DebugViewer::OnClose(wxCloseEvent& event)
{
    if (b_transferredControl)
        m_terminal->continueDebug();
    this->Hide();
    event.Veto();
}
