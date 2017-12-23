/////////////////////////////////////////////////////////////////////////////
// Name:        AddVariableWatch.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/04/04 18:47:20
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "AddVariableWatch.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "AddVariableWatch.h"

////@begin XPM images
////@end XPM images

/*!
 * AddVariableWatchDialog type definition
 */

IMPLEMENT_CLASS( AddVariableWatchDialog, wxDialog )

/*!
 * AddVariableWatchDialog event table definition
 */

BEGIN_EVENT_TABLE( AddVariableWatchDialog, wxDialog )

////@begin AddVariableWatchDialog event table entries
    EVT_BUTTON( ID_DOADDWATCH, AddVariableWatchDialog::OnDoaddwatchClick )

    EVT_BUTTON( ID_CANCELWATCH, AddVariableWatchDialog::OnCancelwatchClick )

////@end AddVariableWatchDialog event table entries

END_EVENT_TABLE()

/*!
 * AddVariableWatchDialog constructors
 */

AddVariableWatchDialog::AddVariableWatchDialog( )
{
}

AddVariableWatchDialog::AddVariableWatchDialog( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * AddVariableWatchDialog creator
 */

bool AddVariableWatchDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AddVariableWatchDialog member initialisation
    m_varname = NULL;
    m_funcName = NULL;
    m_isClassMember = NULL;
    m_classname = NULL;
////@end AddVariableWatchDialog member initialisation

////@begin AddVariableWatchDialog creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end AddVariableWatchDialog creation
    return TRUE;
}

/*!
 * Control creation for AddVariableWatchDialog
 */

void AddVariableWatchDialog::CreateControls()
{    
////@begin AddVariableWatchDialog content construction

    AddVariableWatchDialog* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxVERTICAL);
    item2->Add(item3, 0, wxGROW|wxALL, 5);

    wxStaticText* item4 = new wxStaticText( item1, wxID_STATIC, _("Variable name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item5 = new wxTextCtrl( item1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    m_varname = item5;
    item3->Add(item5, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item6 = new wxBoxSizer(wxVERTICAL);
    item2->Add(item6, 0, wxGROW|wxALL, 5);

    wxStaticText* item7 = new wxStaticText( item1, wxID_STATIC, _("Function name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item7, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item8 = new wxTextCtrl( item1, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    m_funcName = item8;
    item6->Add(item8, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item9 = new wxBoxSizer(wxVERTICAL);
    item2->Add(item9, 0, wxGROW|wxALL, 5);

    wxCheckBox* item10 = new wxCheckBox( item1, ID_CHECKBOX2, _("Function is part of class:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_isClassMember = item10;
    item10->SetValue(FALSE);
    item9->Add(item10, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxTextCtrl* item11 = new wxTextCtrl( item1, ID_TEXTCTRL2, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    m_classname = item11;
    item9->Add(item11, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item12 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item12, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton* item13 = new wxButton( item1, ID_DOADDWATCH, _("Add watch"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item14 = new wxButton( item1, ID_CANCELWATCH, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end AddVariableWatchDialog content construction
}

/*!
 * Should we show tooltips?
 */

bool AddVariableWatchDialog::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void AddVariableWatchDialog::OnDoaddwatchClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

    EndModal(wxOK);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void AddVariableWatchDialog::OnCancelwatchClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

    EndModal(wxCANCEL);
}


