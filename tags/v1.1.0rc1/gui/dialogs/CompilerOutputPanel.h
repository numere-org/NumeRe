/////////////////////////////////////////////////////////////////////////////
// Name:        CompilerOutputPanel.h
// Purpose:
// Author:
// Modified by:
// Created:     04/12/04 19:39:08
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _COMPILEROUTPUTPANEL_H_
#define _COMPILEROUTPUTPANEL_H_

#ifdef __GNUG__
#pragma interface "CompilerOutputPanel.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
#include <wx/wx.h>
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define ID_COMPILERGRID 10002
#define ID_COMPILERTEXT 10001
#define SYMBOL_COMPILEROUTPUTPANEL_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxDIALOG_MODAL
#define SYMBOL_COMPILEROUTPUTPANEL_TITLE _("CompilerOutputPanel")
#define SYMBOL_COMPILEROUTPUTPANEL_IDNAME ID_DIALOG
#define SYMBOL_COMPILEROUTPUTPANEL_SIZE wxSize(400, 300)
#define SYMBOL_COMPILEROUTPUTPANEL_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

class CompilerEvent;
class wxGridEvent;
class NumeReWindow;
class Options;

/*!
 * CompilerOutputPanel class declaration
 */

class CompilerOutputPanel: public wxPanel
{
    DECLARE_CLASS( CompilerOutputPanel )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CompilerOutputPanel( );
    CompilerOutputPanel( wxWindow* parent, NumeReWindow* mainFrame, Options* options, wxWindowID id = SYMBOL_COMPILEROUTPUTPANEL_IDNAME, const wxString& caption = SYMBOL_COMPILEROUTPUTPANEL_TITLE, const wxPoint& pos = SYMBOL_COMPILEROUTPUTPANEL_POSITION, const wxSize& size = SYMBOL_COMPILEROUTPUTPANEL_SIZE, long style = SYMBOL_COMPILEROUTPUTPANEL_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_COMPILEROUTPUTPANEL_IDNAME, const wxString& caption = SYMBOL_COMPILEROUTPUTPANEL_TITLE, const wxPoint& pos = SYMBOL_COMPILEROUTPUTPANEL_POSITION, const wxSize& size = SYMBOL_COMPILEROUTPUTPANEL_SIZE, long style = SYMBOL_COMPILEROUTPUTPANEL_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CompilerOutputPanel event handler declarations

////@end CompilerOutputPanel event handler declarations

////@begin CompilerOutputPanel member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CompilerOutputPanel member function declarations

	virtual void SetFocus();

	bool IsAdvanced() { return m_isAdvanced; }

	void Stuff(wxWindow* parent, NumeReWindow* mainFrame, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style );

	void ClearOutput();
	void SetAdvanced(bool advanced);

	void OnCompilerStart(CompilerEvent& event);
	void OnCompilerProblem(CompilerEvent &event);
	void OnCompilerEnd(CompilerEvent &event);
	void OnGridDoubleClick(wxGridEvent &event);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CompilerOutputPanel member variables
    wxBoxSizer* m_sizer;
    wxGrid* m_grid;
    wxTextCtrl* m_textbox;
////@end CompilerOutputPanel member variables

	NumeReWindow* m_mainFrame;
	Options* m_options;

	bool m_isAdvanced;
	int m_numErrors;
};

#endif
    // _COMPILEROUTPUTPANEL_H_
