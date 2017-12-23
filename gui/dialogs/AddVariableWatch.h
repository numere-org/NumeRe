/////////////////////////////////////////////////////////////////////////////
// Name:        AddVariableWatch.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/04/04 18:47:20
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ADDVARIABLEWATCH_H_
#define _ADDVARIABLEWATCH_H_

#ifdef __GNUG__
#pragma interface "AddVariableWatch.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/wx.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_ADDVARIABLEWATCHDIALOG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ADDVARIABLEWATCHDIALOG_TITLE _("Add Variable Watch")
#define SYMBOL_ADDVARIABLEWATCHDIALOG_IDNAME ID_DIALOG
#define SYMBOL_ADDVARIABLEWATCHDIALOG_SIZE wxSize(400, 300)
#define SYMBOL_ADDVARIABLEWATCHDIALOG_POSITION wxPoint(0, 0)
#define ID_TEXTCTRL 10002
#define ID_TEXTCTRL1 10004
#define ID_CHECKBOX2 10005
#define ID_TEXTCTRL2 10006
#define ID_DOADDWATCH 10001
#define ID_CANCELWATCH 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * AddVariableWatchDialog class declaration
 */

class AddVariableWatchDialog: public wxDialog
{    
    DECLARE_CLASS( AddVariableWatchDialog )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AddVariableWatchDialog( );
    AddVariableWatchDialog( wxWindow* parent, wxWindowID id = SYMBOL_ADDVARIABLEWATCHDIALOG_IDNAME, const wxString& caption = SYMBOL_ADDVARIABLEWATCHDIALOG_TITLE, const wxPoint& pos = SYMBOL_ADDVARIABLEWATCHDIALOG_POSITION, const wxSize& size = SYMBOL_ADDVARIABLEWATCHDIALOG_SIZE, long style = SYMBOL_ADDVARIABLEWATCHDIALOG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ADDVARIABLEWATCHDIALOG_IDNAME, const wxString& caption = SYMBOL_ADDVARIABLEWATCHDIALOG_TITLE, const wxPoint& pos = SYMBOL_ADDVARIABLEWATCHDIALOG_POSITION, const wxSize& size = SYMBOL_ADDVARIABLEWATCHDIALOG_SIZE, long style = SYMBOL_ADDVARIABLEWATCHDIALOG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AddVariableWatchDialog event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DOADDWATCH
    void OnDoaddwatchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CANCELWATCH
    void OnCancelwatchClick( wxCommandEvent& event );

////@end AddVariableWatchDialog event handler declarations

////@begin AddVariableWatchDialog member function declarations

////@end AddVariableWatchDialog member function declarations

	wxString GetVariableName() { return m_varname->GetValue(); }
	wxString GetFunctionName() { return m_funcName->GetValue(); }
	wxString GetClassName() { return m_classname->GetValue(); }

	bool FunctionInClass() { return m_isClassMember->IsChecked(); }

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AddVariableWatchDialog member variables
    wxTextCtrl* m_varname;
    wxTextCtrl* m_funcName;
    wxCheckBox* m_isClassMember;
    wxTextCtrl* m_classname;
////@end AddVariableWatchDialog member variables
};

#endif
    // _ADDVARIABLEWATCH_H_
