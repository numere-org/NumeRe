/////////////////////////////////////////////////////////////////////////////
// Name:        wxTermContainer.h
// Purpose:
// Author:
// Modified by:
// Created:     03/18/04 01:08:08
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXTERMCONTAINER_H_
#define _WXTERMCONTAINER_H_

#ifdef __GNUG__
#pragma interface "wxTermContainer.h"
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
class NumeReTerminal;
////@end forward declarations
class wxSizer;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
//#define ID_PANEL 10001
#define ID_SCROLLBAR 10002
////@end control identifiers

/*!
 * wxTermContainer class declaration
 */

class wxTermContainer: public wxPanel
{
    DECLARE_CLASS( wxTermContainer )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxTermContainer( );
    wxTermContainer( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxBORDER_STATIC );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxBORDER_STATIC );

    /// Creates the controls and sizers
    void CreateControls();

	void SetTerminal(NumeReTerminal* terminal);

////@begin wxTermContainer event handler declarations
    /// wxEVT_SCROLL_LINEUP event handler for ID_SCROLLBAR
    void OnScrollbarScrollLineUp( wxScrollEvent& event );

    /// wxEVT_SCROLL_LINEDOWN event handler for ID_SCROLLBAR
    void OnScrollbarScrollLineDown( wxScrollEvent& event );

    /// wxEVT_SCROLL_PAGEUP event handler for ID_SCROLLBAR
    void OnScrollbarScrollPageUp( wxScrollEvent& event );

    /// wxEVT_SCROLL_PAGEDOWN event handler for ID_SCROLLBAR
    void OnScrollbarScrollPageDown( wxScrollEvent& event );

    /// wxEVT_SCROLL_THUMBTRACK event handler for ID_SCROLLBAR
    void OnScrollbarScrollThumbtrack( wxScrollEvent& event );

    /// wxEVT_MOUSEWHEEL event handler
    void OnWheelScroll(wxMouseEvent& event);

////@end wxTermContainer event handler declarations

	void OnSize(wxSizeEvent &event);
	void OnUpdateUI(wxUpdateUIEvent &event);

////@begin wxTermContainer member function declarations
////@end wxTermContainer member function declarations

	void SetTerminalHistory(int newSize);
	void SetCaretBlinkTime(int newTime);

    /// Should we show tooltips?
    static bool ShowToolTips();

private:

////@begin wxTermContainer member variables
    NumeReTerminal* m_terminal;
    wxScrollBar* m_scrollbar;
////@end wxTermContainer member variables


	int m_lastThumbPosition;
	wxBoxSizer* m_sizer;
	int m_lastLinesReceived;

	void updateThumbPosition();
};

#endif
