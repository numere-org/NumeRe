/////////////////////////////////////////////////////////////////////////////
// Name:        VariableWatchPanel.h
// Purpose:
// Author:
// Modified by:
// Created:     03/29/04 21:43:35
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _VARIABLEWATCHPANEL_H_
#define _VARIABLEWATCHPANEL_H_

#ifdef __GNUG__
#pragma interface "VariableWatchPanel.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/listctrl.h"
////@end includes
//#include <wx/treelist.h>
#include "treelistctrl.h"
#include "../../common/tree.h"

//#include  <wx/dynarray.h>
#include "../../common/datastructures.h"
#include "../../common/DebugEvent.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxListCtrl;
////@end forward declarations
class NumeReWindow;
typedef tree<wxString> ParseTree;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_VARWATCHDIALOG 10000
#define ID_LISTCTRL 10001
#define ID_ADDWATCH 10002
#define ID_REMOVEWATCH 10003
#define ID_CLEARALLWATCHES 10004
#define SYMBOL_VARIABLEWATCHPANEL_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxDIALOG_MODAL
#define SYMBOL_VARIABLEWATCHPANEL_TITLE _("VariableWatch")
#define SYMBOL_VARIABLEWATCHPANEL_IDNAME ID_VARWATCHDIALOG
#define SYMBOL_VARIABLEWATCHPANEL_SIZE wxDefaultSize
#define SYMBOL_VARIABLEWATCHPANEL_POSITION wxDefaultPosition
////@end control identifiers
#define ID_TREELIST 10099

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * VariableWatchPanel class declaration
 */

class VariableWatchPanel: public wxPanel
{
    DECLARE_CLASS( VariableWatchPanel )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    VariableWatchPanel( );
    VariableWatchPanel( wxWindow* parent, wxEvtHandler* mainframe, wxWindowID id = SYMBOL_VARIABLEWATCHPANEL_IDNAME, const wxPoint& pos = SYMBOL_VARIABLEWATCHPANEL_POSITION, const wxSize& size = SYMBOL_VARIABLEWATCHPANEL_SIZE, long style = SYMBOL_VARIABLEWATCHPANEL_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_VARIABLEWATCHPANEL_IDNAME, const wxPoint& pos = SYMBOL_VARIABLEWATCHPANEL_POSITION, const wxSize& size = SYMBOL_VARIABLEWATCHPANEL_SIZE, long style = SYMBOL_VARIABLEWATCHPANEL_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin VariableWatchPanel event handler declarations

    /// wxEVT_SIZE event handler for ID_VARWATCHDIALOG
    void OnSize( wxSizeEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ADDWATCH
    void OnAddwatchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_REMOVEWATCH
    void OnRemovewatchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLEARALLWATCHES
    void OnClearallwatchesClick( wxCommandEvent& event );

////@end VariableWatchPanel event handler declarations

////@begin VariableWatchPanel member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end VariableWatchPanel member function declarations

	void AddWatch();
	void RemoveWatch();
	void ClearVariableValues();
	void UpdateVariableInfo(wxDebugEvent event);
    /// Should we show tooltips?
    static bool ShowToolTips();


	void AddWatchedVariables(wxDebugEvent debug);
	void DebuggerExited();
	void DisplayParsedValue(wxTreeItemId currentNode, ParseTree& tree);

	void SetEventHandler(wxEvtHandler* eventHandler) { m_parentEventHandler = eventHandler; }

	void TestParsing();

////@begin VariableWatchPanel member variables
    wxListCtrl* m_list;
////@end VariableWatchPanel member variables

	wxTreeListCtrl* m_tree;

private:
	wxEvtHandler* m_parentEventHandler;

	IntIntHashmap m_gdbVariableMappings;

	//wxArrayString
};

#endif
    // _VARIABLEWATCHPANEL_H_
