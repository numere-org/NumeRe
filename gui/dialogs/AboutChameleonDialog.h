/////////////////////////////////////////////////////////////////////////////
// Name:        AboutChameleonDialog.h
// Purpose:
// Author:
// Modified by:
// Created:     04/20/04 01:22:14
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTCHAMELEONDIALOG_H_
#define _ABOUTCHAMELEONDIALOG_H_

#ifdef __GNUG__
#pragma interface "AboutChameleonDialog.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include <wx/wx.h>
#include "wx/notebook.h"
#include "../globals.hpp"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define ID_NOTEBOOK 10001
#define ID_PROGPANEL 10006
#define ID_TEAMPANEL 10002
#define ID_CREDITSPANEL 10003
#define ID_STATSPANEL 10004
#define ID_BUTTONOK 10007
#define SYMBOL_ABOUTCHAMELEONDIALOG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ABOUTCHAMELEONDIALOG_TITLE _("About Chameleon")
#define SYMBOL_ABOUTCHAMELEONDIALOG_IDNAME ID_DIALOG
#define SYMBOL_ABOUTCHAMELEONDIALOG_SIZE wxSize(410*g_pixelScale, 470*g_pixelScale)
#define SYMBOL_ABOUTCHAMELEONDIALOG_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * AboutChameleonDialog class declaration
 */

class AboutChameleonDialog : public wxDialog
{
    DECLARE_CLASS( AboutChameleonDialog )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AboutChameleonDialog( );
    AboutChameleonDialog( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTCHAMELEONDIALOG_IDNAME, const wxString& caption = SYMBOL_ABOUTCHAMELEONDIALOG_TITLE, const wxPoint& pos = SYMBOL_ABOUTCHAMELEONDIALOG_POSITION, const wxSize& size = SYMBOL_ABOUTCHAMELEONDIALOG_SIZE, long style = SYMBOL_ABOUTCHAMELEONDIALOG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTCHAMELEONDIALOG_IDNAME, const wxString& caption = SYMBOL_ABOUTCHAMELEONDIALOG_TITLE, const wxPoint& pos = SYMBOL_ABOUTCHAMELEONDIALOG_POSITION, const wxSize& size = SYMBOL_ABOUTCHAMELEONDIALOG_SIZE, long style = SYMBOL_ABOUTCHAMELEONDIALOG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AboutChameleonDialog event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTONOK
    void OnButtonOKClick( wxCommandEvent& event );

////@end AboutChameleonDialog event handler declarations

////@begin AboutChameleonDialog member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AboutChameleonDialog member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AboutChameleonDialog member variables
    wxBoxSizer* m_sizerProgram;
    wxStaticText* m_lblVersion;
////@end AboutChameleonDialog member variables
};

#endif
    // _ABOUTCHAMELEONDIALOG_H_
