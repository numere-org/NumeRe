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
#include "../NumeReWindow.h"

#define ID_DEBUG_CONTINUE 10201
#define ID_DEBUG_CANCEL 10202
#define ID_DEBUG_STEP 10203
#define ID_DEBUG_STEPOVER 10204
#define ID_DEBUG_LEAVE 10205

extern Language _guilang;

BEGIN_EVENT_TABLE(DebugViewer, ViewerFrame)
    EVT_CLOSE (DebugViewer::OnClose)
    EVT_LIST_ITEM_ACTIVATED(-1, DebugViewer::OnStackItemActivate)
    EVT_MENU_RANGE(ID_DEBUG_CONTINUE, ID_DEBUG_LEAVE, DebugViewer::OnMenuEvent)
END_EVENT_TABLE()

// Constructor. Creates the necessary controls of the debugger window
DebugViewer::DebugViewer(wxWindow* parent, Options* _options, const wxString& title) : ViewerFrame(parent, title)
{
    m_options = _options;
    nLineColumn = 0;
    nModuleColumn = 0;

    // Create the toolbar
    initializeToolbar();

    // Create the status bar
    int widths[] = {-2, -1};
    wxStatusBar* sb = CreateStatusBar(2);
    sb->SetStatusWidths(2, widths);

    b_transferredControl = false;

    // initialize the controls: create the panel
    wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_THEME);

    // Create vertical and horizontal sizers
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

    // Create static box sizers for the main
    // GUI elements
    wxStaticBoxSizer* exprBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_EXPR"));
    wxStaticBoxSizer* errorBox = new wxStaticBoxSizer(wxVERTICAL, panel, _guilang.get("DBG_MODULE"));
    wxStaticBoxSizer* stackBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_STACKTRACE"));
    wxStaticBoxSizer* varBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _guilang.get("DBG_LOCALS"));

    // Create the expression and the error message
    // text contrls
    m_expression = new wxTextCtrl(exprBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_RICH);
    m_errorMessage = new wxTextCtrl(errorBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    // Change font and colour of the two text controls
    wxFont font;
    font.SetNativeFontInfoUserDesc("consolas 10");
    m_expression->SetFont(font);
    m_expression->SetBackgroundColour(*wxWHITE);
    m_errorMessage->SetForegroundColour(*wxRED);

    // Create the stack trace list control and add
    // four columns
    m_stacktrace = new wxListCtrl(stackBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize /*wxSize(400, -1)*/, wxLC_REPORT);
    m_stacktrace->AppendColumn("  ");
    m_stacktrace->AppendColumn("Stack");
    nLineColumn = m_stacktrace->AppendColumn(_guilang.get("DBG_LINENO"), wxLIST_FORMAT_RIGHT);
    nModuleColumn = m_stacktrace->AppendColumn(_guilang.get("DBG_FILE"));

    if (!m_options->GetShowLinesInStackTrace())
        m_stacktrace->SetColumnWidth(nLineColumn, 0);
    if (!m_options->GetShowModulesInStackTrace())
        m_stacktrace->SetColumnWidth(nModuleColumn, 0);

    // Create the variable viewer in debugger mode
    m_varViewer = new VariableViewer(varBox->GetStaticBox(), (NumeReWindow*)parent);
    m_varViewer->setDebuggerMode(true);

    // Add the GUI elements to the static box sizers
    exprBox->Add(m_expression, 1, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND);
    errorBox->Add(m_errorMessage, 1, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND);
    stackBox->Add(m_stacktrace, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);
    varBox->Add(m_varViewer, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL);

    // Add the static box sizers to the
    // horizontal and vertical box sizers
    // of the panel
    hsizer->Add(stackBox, 2, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 0);
    hsizer->AddSpacer(10);
    hsizer->Add(errorBox, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 0);
    vsizer->Add(exprBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 5);
    vsizer->Add(hsizer, 1, wxALIGN_CENTER_HORIZONTAL | wxEXPAND | wxALL, 5);
    vsizer->Add(varBox, 2, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);

    // Set the main sizer and focus the expression
    // text control
    panel->SetSizer(vsizer);
    m_expression->SetFocus();
}

// This private member function creates the toolbar
// of the debugger window
void DebugViewer::initializeToolbar()
{
    // Get the frame toolbar
    wxToolBar* tb = CreateToolBar(wxTB_HORZ_TEXT);

    // Get the application path
    wxString appPath = static_cast<NumeReWindow*>(GetParent())->getProgramFolder();

    wxBitmap dbgContinue(appPath + "\\icons\\dbgrun.png", wxBITMAP_TYPE_PNG);
    wxBitmap dbgStep(appPath + "\\icons\\dbgrunto.png", wxBITMAP_TYPE_PNG);
    wxBitmap dbgLeave(appPath + "\\icons\\dbgstepout.png", wxBITMAP_TYPE_PNG);
    wxBitmap dbgStepOver(appPath + "\\icons\\dbgnexti.png", wxBITMAP_TYPE_PNG);
    wxBitmap dbgAbort(appPath + "\\icons\\dbgstop.png", wxBITMAP_TYPE_PNG);

    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_CONTINUE, _guilang.get("DBG_CONTINUE"), dbgContinue, dbgContinue, wxITEM_NORMAL, _guilang.get("DBG_CONTINUE_HLP"), _guilang.get("DBG_CONTINUE_HLP"));
    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_STEP, _guilang.get("DBG_STEP"), dbgStep, dbgStep, wxITEM_NORMAL, _guilang.get("DBG_STEP_HLP"), _guilang.get("DBG_STEP_HLP"));
    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_STEPOVER, _guilang.get("DBG_STEPOVER"), dbgStepOver, dbgStepOver, wxITEM_NORMAL, _guilang.get("DBG_STEPOVER_HLP"), _guilang.get("DBG_STEPOVER_HLP"));
    tb->AddSeparator();
    tb->AddTool(ID_DEBUG_LEAVE, _guilang.get("DBG_LEAVE"), dbgLeave, dbgLeave, wxITEM_NORMAL, _guilang.get("DBG_LEAVE_HLP"), _guilang.get("DBG_LEAVE_HLP"));
    tb->AddStretchableSpace();
    tb->AddTool(ID_DEBUG_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"), dbgAbort);

    // Actually create the toolbar
    tb->Realize();
}

// This private member function updates the expression element
// in the debugger window and changes the colour of the characters
// correspondingly
void DebugViewer::setExpression(const string& sLineNumber, const string& sExpression)
{
    string sColours = m_terminal->getSyntax()->highlightLine("|<- " + sExpression).substr(4);

    m_expression->Clear();
    m_expression->SetDefaultStyle(wxTextAttr(wxColour(128, 128, 128)));
    m_expression->AppendText("@("+sLineNumber+") |  ");

    for (size_t i = 0; i < sColours.length(); i++)
    {
        wxFont font;
        font.SetNativeFontInfoUserDesc("consolas 10");

        if (sColours[i] - '0' == NumeReSyntax::SYNTAX_COMMAND || sColours[i] - '0' == NumeReSyntax::SYNTAX_NPRC_COMMAND)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::COMMAND).foreground, wxNullColour, font.MakeBold()));
        else if (sColours[i] - '0' == NumeReSyntax::SYNTAX_FUNCTION)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::FUNCTION).foreground, wxNullColour, font.MakeBold()));
        else if (sColours[i] - '0' == NumeReSyntax::SYNTAX_PROCEDURE)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::PROCEDURE).foreground, wxNullColour, font.MakeBold()));
        else if (sColours[i] - '0' == NumeReSyntax::SYNTAX_STRING)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::STRING).foreground, wxNullColour, font));
        else if (sColours[i] - '0' == NumeReSyntax::SYNTAX_METHODS)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::METHODS).foreground, wxNullColour, font.MakeBold()));
        else if (sColours[i] - '0' == NumeReSyntax::SYNTAX_OPERATOR)
            m_expression->SetDefaultStyle(wxTextAttr(m_options->GetSyntaxStyle(Options::OPERATOR).foreground, wxNullColour, font));
        else
            m_expression->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));

        m_expression->AppendText(sExpression.substr(i, 1));
    }
}

// This member function is the event handler function
// for double-clicking on an stack item
void DebugViewer::OnStackItemActivate(wxListEvent& event)
{
    getInformationByStackId(m_stacktrace->GetItemCount() - event.GetIndex() - 1);
}

// This private member function gets the debugger information
// from the selected stack id
void DebugViewer::getInformationByStackId(size_t id)
{
    vector<string> vModuleInfo;
    vector<string> vNumVars;
    vector<string> vStringVars;
    vector<string> vTables;
    vector<string> vArguments;
    vector<string> vGlobals;

    // Read the information from the kernel by locking
    // the critical section
    {
        wxCriticalSectionLocker lock(m_terminal->m_kernelCS);
        NumeReDebugger& _debugger = m_terminal->_kernel.getDebugger();

        // Select the corresponding stack id
        _debugger.select(id);

        // Read the information from the debugger
        vModuleInfo = _debugger.getModuleInformations();
        vNumVars = _debugger.getNumVars();
        vStringVars = _debugger.getStringVars();
        vTables = _debugger.getTables();

        if (m_options->GetShowProcedureArguments())
            vArguments = _debugger.getArguments();

        if (m_options->GetShowGlobalVariables())
            vGlobals = _debugger.getGlobals();
    }

    // Mark the current selected stack item in the stack
    // trace list control
    for (int i = 0; i < m_stacktrace->GetItemCount(); i++)
    {
        if (m_stacktrace->GetItemCount() - 1 - i == id)
            m_stacktrace->SetItemText(i, "->");
        else
            m_stacktrace->SetItemText(i, "");
    }

    // Update the status bar
    GetStatusBar()->SetStatusText(vModuleInfo[1] + " @ " + vModuleInfo[2]);

    // Add line number and expression to the expression
    // text control and change the colour for line number
    setExpression(vModuleInfo[2], vModuleInfo[0]);

    // Set the error message
    m_errorMessage->SetValue(vModuleInfo[3]);

    // Store the sizes of the vectors
    size_t n_num = vNumVars.size();
    size_t s_num = vStringVars.size();
    size_t t_num = vTables.size();
    size_t a_num = vArguments.size();
    size_t g_num = vGlobals.size();

    // Create the variable vector for the
    // variable viewer
    vNumVars.insert(vNumVars.end(), vStringVars.begin(), vStringVars.end());
    vNumVars.insert(vNumVars.end(), vTables.begin(), vTables.end());
    vNumVars.insert(vNumVars.end(), vArguments.begin(), vArguments.end());
    vNumVars.insert(vNumVars.end(), vGlobals.begin(), vGlobals.end());

    // Update the variable viewer
    m_varViewer->UpdateVariables(vNumVars, n_num, s_num, t_num, a_num, g_num);
}

// This member function is the event handler routine
// for the toolbar functions
void DebugViewer::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_DEBUG_CONTINUE:
            if (b_transferredControl)
                m_terminal->continueDebug();

            GetStatusBar()->SetStatusText(_guilang.get("DBG_CONTINUING"), 1);

            break;
        case ID_DEBUG_CANCEL:
            if (b_transferredControl)
            {
                m_terminal->CancelCalculation();
                m_terminal->continueDebug();
            }

            GetStatusBar()->SetStatusText(_guilang.get("DBG_ABORTED"), 1);

            break;
        case ID_DEBUG_STEP:
            if (b_transferredControl)
                m_terminal->stepDebug();

            GetStatusBar()->SetStatusText(_guilang.get("DBG_CONTINUING"), 1);

            break;
        case ID_DEBUG_STEPOVER:
            if (b_transferredControl)
                m_terminal->stepOverDebug();

            GetStatusBar()->SetStatusText(_guilang.get("DBG_CONTINUING"), 1);

            break;
        case ID_DEBUG_LEAVE:
            if (b_transferredControl)
                m_terminal->leaveDebug();

            GetStatusBar()->SetStatusText(_guilang.get("DBG_CONTINUING"), 1);

            break;
    }
}

// This member function should be called after the user modified
// the application settings. It will propagate the necessary application
// settings into the debugger
void DebugViewer::updateSettings()
{
    // Update the variable viewer
    m_varViewer->setDebuggerMode(true);

    // Hide or show the line column of the stack trace
    if (!m_options->GetShowLinesInStackTrace())
        m_stacktrace->SetColumnWidth(nLineColumn, 0);
    else
        m_stacktrace->SetColumnWidth(nLineColumn, wxLIST_AUTOSIZE_USEHEADER);

    // Hide or show the module column of the stack trace
    if (!m_options->GetShowModulesInStackTrace())
        m_stacktrace->SetColumnWidth(nModuleColumn, 0);
    else
        m_stacktrace->SetColumnWidth(nModuleColumn, wxLIST_AUTOSIZE_USEHEADER);
}

// This member function is used from the main window
// to update the debugger window with the information of
// the current debug event (aka the new stack)
void DebugViewer::setDebugInfo(const wxString& title, const vector<string>& vStack)
{
    // Update the title
    this->SetTitle(title);
    b_transferredControl = true;

    GetStatusBar()->SetStatusText(_guilang.get("DBG_STOPPED"), 1);

    // Remove all previous stack items
    m_stacktrace->DeleteAllItems();

    // Update the stack trace with file
    // names and line numbers
    for (size_t i = 0; i < vStack.size(); i++)
    {
        m_stacktrace->InsertItem(i, "");
        m_stacktrace->SetItem(i, 1, vStack[i].substr(0, vStack[i].find('\t')));
        m_stacktrace->SetItem(i, nLineColumn, vStack[i].substr(vStack[i].rfind('\t')+1));
        m_stacktrace->SetItem(i, nModuleColumn, vStack[i].substr(vStack[i].find('\t')+1, vStack[i].rfind('\t') - vStack[i].find('\t') - 1));
    }

    // Auto-size the corresponding columns
    m_stacktrace->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_stacktrace->SetColumnWidth(1, wxLIST_AUTOSIZE);

    if (m_options->GetShowLinesInStackTrace())
        m_stacktrace->SetColumnWidth(nLineColumn, wxLIST_AUTOSIZE_USEHEADER);
    else
        m_stacktrace->SetColumnWidth(nLineColumn, 0);

    if (m_options->GetShowModulesInStackTrace())
        m_stacktrace->SetColumnWidth(nModuleColumn, wxLIST_AUTOSIZE);
    else
        m_stacktrace->SetColumnWidth(nModuleColumn, 0);

    // Get the debugger information for the
    // top stack element
    if (vStack.size())
        getInformationByStackId(vStack.size()-1);
    else
        getInformationByStackId(0);
}

// This member function is called upon closing the
// debugger window: it won't be destroyed but hidden
// and the kernel will get a CONTINUE signal
void DebugViewer::OnClose(wxCloseEvent& event)
{
    if (b_transferredControl)
        m_terminal->continueDebug();

    this->Hide();
    event.Veto();
}


