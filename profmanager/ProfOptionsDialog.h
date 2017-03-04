/////////////////////////////////////////////////////////////////////////////
// Name:        ProfOptionsDialog.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/09/04 19:36:08
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROFOPTIONSDIALOG_H_
#define _PROFOPTIONSDIALOG_H_

#ifdef __GNUG__
#pragma interface "ProfOptionsDialog.h"
#endif

/*!
 * Includes
 */

////@begin includes
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
#define ID_CHECKLISTBOX 10001
#define ID_TXTCODE 10002
#define ID_GENERATE 10003
#define ID_RANDOMIZE 10004
#define ID_BUTTON 10006
#define ID_BUTTON1 10007
#define ID_BUTTON2 10009
#define ID_TEXTCTRL 10008
#define ID_EXITBUTTON 10005
////@end control identifiers

/*!
 * ProfOptionsDialog class declaration
 */

class ProfOptionsDialog: public wxDialog
{    
    DECLARE_CLASS( ProfOptionsDialog )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ProfOptionsDialog( );
    ProfOptionsDialog( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Chameleon Options Manager"), 
					const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
					long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Chameleon Options Manager"), 
				const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
				long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ProfOptionsDialog event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GENERATE
    void OnGenerateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
    void OnButtonCheckAllClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
    void OnButtonUncheckAllClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON2
    void OnDecodeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_EXITBUTTON
    void OnExitbuttonClick( wxCommandEvent& event );

////@end ProfOptionsDialog event handler declarations

	void OnQuit(wxCloseEvent &event);

////@begin ProfOptionsDialog member function declarations

////@end ProfOptionsDialog member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ProfOptionsDialog member variables
    wxCheckListBox* m_chklstModules;
    wxTextCtrl* m_txtGeneratedCode;
    wxButton* m_genButton;
    wxCheckBox* m_chkRandomize;
    wxButton* m_butCheckAll;
    wxButton* m_butUncheckAll;
    wxButton* m_decodeButton;
    wxTextCtrl* m_txtDecode;
    wxButton* m_exitButton;
////@end ProfOptionsDialog member variables
};

#endif
    // _PROFOPTIONSDIALOG_H_
