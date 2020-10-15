/////////////////////////////////////////////////////////////////////////////
// Name:        wxTermContainer.cpp
// Purpose:
// Author:
// Modified by:
// Created:     03/18/04 01:08:08
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "wxTermContainer.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
#include "wx/scrolbar.h"
////@end includes

#include "wxTermContainer.h"
#include "../terminal/terminal.hpp"

#include "../../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

////@begin XPM images

////@end XPM images

/*!
 * wxTermContainer type definition
 */

IMPLEMENT_CLASS( wxTermContainer, wxPanel )

/*!
 * wxTermContainer event table definition
 */

BEGIN_EVENT_TABLE( wxTermContainer, wxPanel )

////@begin wxTermContainer event table entries
    EVT_COMMAND_SCROLL_LINEUP( ID_SCROLLBAR, wxTermContainer::OnScrollbarScrollLineUp )
    EVT_COMMAND_SCROLL_LINEDOWN( ID_SCROLLBAR, wxTermContainer::OnScrollbarScrollLineDown )
    EVT_COMMAND_SCROLL_PAGEUP( ID_SCROLLBAR, wxTermContainer::OnScrollbarScrollPageUp )
    EVT_COMMAND_SCROLL_PAGEDOWN( ID_SCROLLBAR, wxTermContainer::OnScrollbarScrollPageDown )
    EVT_COMMAND_SCROLL_THUMBTRACK( ID_SCROLLBAR, wxTermContainer::OnScrollbarScrollThumbtrack )
    EVT_MOUSEWHEEL(wxTermContainer::OnWheelScroll)

////@end wxTermContainer event table entries

  //EVT_SIZE						(wxTermContainer::OnSize)
  EVT_UPDATE_UI					(ID_SCROLLBAR, wxTermContainer::OnUpdateUI)

END_EVENT_TABLE()

/*!
 * wxTermContainer constructors
 */

wxTermContainer::wxTermContainer( ) : m_terminal(nullptr), m_scrollbar(nullptr), m_lastThumbPosition(0), m_sizer(nullptr), m_lastLinesReceived(0)
{
}

wxTermContainer::wxTermContainer( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
: m_terminal(nullptr)
{
    Create(parent, id, pos, size, style);

	m_lastLinesReceived = 0;
	m_lastThumbPosition = 0;

}

/*!
 * wxTermContainer creator
 */

bool wxTermContainer::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxTermContainer member initialisation
////@end wxTermContainer member initialisation


////@begin wxTermContainer creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    Centre();
////@end wxTermContainer creation
    return TRUE;
}

/*!
 * Control creation for wxTermContainer
 */

void wxTermContainer::CreateControls()
{
////@begin wxTermContainer content construction

    wxTermContainer* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxHORIZONTAL);
	m_sizer = item2;
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

	/*
    wxTerm* item3 = new wxTerm( item1, ID_PANEL, wxDefaultPosition, wxSize(379, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    m_terminal = item3;
    item2->Add(item3, 1, wxGROW, 5);
	*/

	//item2->Add(m_terminal, 1, wxGROW, 0);

    wxScrollBar* item4 = new wxScrollBar( item1, ID_SCROLLBAR, wxPoint(this->GetSize().GetX() - 18, 0), wxSize(18, -1), wxSB_VERTICAL );
    m_scrollbar = item4;
    item4->SetScrollbar(90, 10, 300, 10);
    item2->Add(item4, 0, wxGROW, 5);

////@end wxTermContainer content construction
}


/////////////////////////////////////////////////
/// \brief Updates the thumb position after every
/// scroll operation except of the click-drag
/// scrolling.
///
/// \return void
///
/////////////////////////////////////////////////
void wxTermContainer::updateThumbPosition()
{
    m_lastThumbPosition = m_terminal->GetScrollHeight() - m_terminal->Height() - m_terminal->GetScrollPosition();
}


/*!
 * wxEVT_SCROLL_LINEUP event handler for ID_SCROLLBAR
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnScrollbarScrollLineUp
///  Scrolls the terminal up a single line when the user clicks the up button
///
///  @param  event wxScrollEvent & The generated scroll event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnScrollbarScrollLineUp( wxScrollEvent& event )
{
    event.Skip();

	//wxLogDebug("OnScrollbarScrollPageUp()");
	m_terminal->ScrollTerminal(1, true);
	updateThumbPosition();
}

void wxTermContainer::OnWheelScroll(wxMouseEvent& event)
{
    m_terminal->ScrollTerminal(3, (event.GetWheelRotation() > 0));
    updateThumbPosition();
}

/*!
 * wxEVT_SCROLL_LINEDOWN event handler for ID_SCROLLBAR
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnScrollbarScrollLineDown
///  Scrolls the terminal down a single line when the user clicks the down button
///
///  @param  event wxScrollEvent & The generated scroll event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnScrollbarScrollLineDown( wxScrollEvent& event )
{
    event.Skip();
	m_terminal->ScrollTerminal(1, false);
    updateThumbPosition();
}

/*!
 * wxEVT_SCROLL_PAGEUP event handler for ID_SCROLLBAR
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnScrollbarScrollPageUp
///  Scrolls the terminal up a "page" when the user clicks in the scrollbar
///
///  @param  event wxScrollEvent & The generated scroll event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnScrollbarScrollPageUp( wxScrollEvent& event )
{
    event.Skip();
	m_terminal->ScrollTerminal(m_terminal->Height(), true);
    updateThumbPosition();
}

/*!
 * wxEVT_SCROLL_PAGEDOWN event handler for ID_SCROLLBAR
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnScrollbarScrollPageDown
///  Scrolls the terminal down a "page" when the user clicks in the scrollbar
///
///  @param  event wxScrollEvent & The generated scroll event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnScrollbarScrollPageDown( wxScrollEvent& event )
{
    event.Skip();
	m_terminal->ScrollTerminal(m_terminal->Height(), false);
	updateThumbPosition();
}

/*!
 * wxEVT_SCROLL_THUMBTRACK event handler for ID_SCROLLBAR
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnScrollbarScrollThumbtrack
///  Scrolls the terminal as the user drags the scroll thumb
///
///  @param  event wxScrollEvent & The generated scroll event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnScrollbarScrollThumbtrack( wxScrollEvent& event )
{
    event.Skip();
	//wxLogDebug("Scroll event.  Value: %d", event.GetPosition());

	int newThumbPosition = event.GetPosition();

	int linesToScroll;
	bool scrollUp = (newThumbPosition < m_lastThumbPosition);

	if(scrollUp)
	{
		linesToScroll = m_lastThumbPosition - newThumbPosition;
	}
	else
	{
		linesToScroll = newThumbPosition - m_lastThumbPosition;
	}

	m_lastThumbPosition = newThumbPosition;

	m_terminal->ScrollTerminal(linesToScroll, scrollUp);
}

/*!
 * Should we show tooltips?
 */

bool wxTermContainer::ShowToolTips()
{
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetTerminal
///  Assigns a terminal widget to this container
///
///  @param  terminal wxTerm * The terminal to use
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::SetTerminal(NumeReTerminal* terminal)
{
	m_terminal = terminal;

	m_sizer->Prepend(m_terminal, 1, wxGROW, 0);

	int termHeight = m_terminal->Height();
	m_scrollbar->SetScrollbar(100 - termHeight, termHeight, 100, termHeight);
}


/////////////////////////////////////////////////
/// \brief Empty OnSize event handler.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxTermContainer::OnSize(wxSizeEvent &event)
{
	event.Skip();
}

//////////////////////////////////////////////////////////////////////////////
///  public OnUpdateUI
///  Updates the scrollbar position, based on the amount of text received by the terminal
///
///  @param  event wxUpdateUIEvent & The generated UI event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTermContainer::OnUpdateUI(wxUpdateUIEvent &event)
{
	int scrollHeight = m_terminal->GetScrollHeight();

	int thumbSize = m_terminal->Height();

	if (scrollHeight <= thumbSize)
	{
		m_scrollbar->Disable();
		return;
	}

	m_scrollbar->Enable();

	int thumbPosition = scrollHeight - thumbSize - m_terminal->GetScrollPosition();

	if (scrollHeight != m_lastLinesReceived)
	{
		//wxLogDebug("linesReceived: %d, thumbSize: %d, thumbPosition: %d", scrollHeight, thumbSize, thumbPosition);
		m_lastLinesReceived = scrollHeight;
		m_lastThumbPosition = thumbPosition;
	}

	if (thumbPosition < 0)
		thumbPosition = 0;

	event.Enable(true);
	m_scrollbar->SetScrollbar(thumbPosition, thumbSize, scrollHeight, thumbSize);
}


/////////////////////////////////////////////////
/// \brief Changes the size of the terminal
/// history buffer.
///
/// \param newSize int
/// \return void
///
/////////////////////////////////////////////////
void wxTermContainer::SetTerminalHistory(int newSize)
{
	m_terminal->SetTerminalHistory(newSize);
}


/////////////////////////////////////////////////
/// \brief Changes the blinking duration of the
/// terminal caret.
///
/// \param newTime int
/// \return void
///
/////////////////////////////////////////////////
void wxTermContainer::SetCaretBlinkTime(int newTime)
{
	m_terminal->SetCursorBlinkRate(newTime);
}

