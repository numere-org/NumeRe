
// Copyright Timothy Miller, 1999

#ifdef __GNUG__
#pragma implementation "gterm.hpp"
#endif

#include "gterm.hpp"
#include "../../common/debug.h"
#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/filename.h>

#ifdef _DEBUG

#define new DEBUG_NEW

#endif

void GenericTerminal::Update()
{
	update_changes();
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ProcessInput
///  Processes the input of the GenericTerminal and hands it over to the internal buffer
///
///  @param  len  int             The number of characters to process.
///  @param  data unsigned char * The text to process.
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::ProcessInput(int len, const string& sData)
{
    // Copy the input
	data_len = sData.length();
	sInput_Data = sData;

	mode_flags |= DESTRUCTBS;
	mode_flags |= INSERT;

	// Evaluate the input and update the GUI
	normal_input();
	update_changes();
	return;

}

/** \brief Processes output returned from the kernel and hands it over to the internal buffer
 *
 * \param len int
 * \param sData const string&
 * \return void
 *
 */
void GenericTerminal::ProcessOutput(int len, const string& sData)
{
    // Copy the input
	data_len += sData.length();
	sInput_Data += sData;

	// Evaluate the input and update the GUI
	normal_output();
	update_changes();
	return;
}

void GenericTerminal::Reset()
{
	reset();
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ResizeTerminal
///  Resizes the terminal's display.  This ONLY changes what's being displayed, and
///  does NOT change the size as far as the server is concerned!
///
///  @param  width  int  The new width, in characters
///  @param  height int  The new height, in characters
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::ResizeTerminal(int w, int h)
{
    // Create an invalid cursor
    LogicalCursor cursor;

    // If the current  terminal is not scrolled up
    // use the current view cursor
    if (!IsScrolledUp())
    {
        cursor = tm.toLogicalCursor(termCursor);
    }

#ifdef DO_LOG
    wxLogDebug("Resizing terminal: w = %d, h = %d", w, h);
#endif

    // Perform the resize in the internal buffer
	tm.Resize(w, h);

	// If the cursor is valid and has a non-zero position
    if (cursor && (cursor.pos || cursor.line))
    {
        // Use it as the new terminal view cursor
        termCursor = tm.toViewCursor(cursor);
        if (!termCursor)
            termCursor = tm.getCurrentViewPos();
    }

	width = w;

    // Does the height change?
	if (h != height)
    {
        height = h;

        // Update the scroll indicators
        scroll_bot = height - 1;
        if (scroll_top >= height)
            scroll_top = 0;
    }

    // Update the GUI
	update_changes();
}


/** \brief Constructor
 *
 * \param w int
 * \param h int
 *
 */
GenericTerminal::GenericTerminal(int w, int h) : width(w), height(h)
{
	doing_update = 0;

	// Create the text manager
	tm = TextManager(this, w, h, MAXWIDTH, MAXHEIGHT);

	// Load the syntax
	wxFileName f(wxStandardPaths::Get().GetExecutablePath());
	_syntax.loadSyntax(f.GetPath(true).ToStdString());

	// Set the terminal view cursor to top left
	// To make the cursor valid, we have to instantiate it with zeros
	termCursor = ViewCursor(0, 0);
	mode_flags = 0;
	reset();
	resetAutoComp();
}

GenericTerminal::~GenericTerminal()
{
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual IsSelected
///  Checks if a given character is selected
///
///  @param  x   int  The x coordinate of the character to check
///  @param  y   int  The y coordinate of the character to check
///
///  @return int Greater than zero if the character is selected
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int
GenericTerminal::IsSelected(int x, int y)
{
	if (x >= 0 && x < Width() && y >= 0 && y < Height())
		return tm.isSelected(ViewCursor(x, y));
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual Select
///  Selects a given character
///
///  @param  x      int  The x coordinate of the character
///  @param  y      int  The y coordinate of the character
///  @param  select int  Effectively a boolean
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
GenericTerminal::Select(int x, int y, int select)
{
	if (x >= 0 && x < Width() && y >= 0 && y < Height())
	{
	    tm.selectText(ViewCursor(x, y), select);
	}
}

// Gets the character at the selected location
unsigned char GenericTerminal::GetChar(int x, int y)
{
	if (x >= 0 && x < Width() && y >= 0 && y < Height())
		return tm.GetCharAdjusted(y, x);

	return 0;
}

// Gets the selected text (if any)
string GenericTerminal::get_selected_text()
{
	return tm.getSelectedText();
}

// Get a pointer to the internal text buffer
TextManager* GenericTerminal::GetTM()
{
	return &tm;
}

//////////////////////////////////////////////////////////////////////////////
///  public Scroll
///  Scrolls the terminal a given number of lines
///
///  @param  numLines int   How many lines to scroll
///  @param  scrollUp bool  True to scroll up, false to scroll down
///
///  @return void
///
///  @author Mark Erikson @date 04-25-2004
//////// comment generated by Comment Maker from www.FeinSoftware.com /////////
bool GenericTerminal::Scroll(int numLines, bool scrollUp)
{
    // Scroll and update the GUI
	if (tm.Scroll(numLines, scrollUp))
	{
		update_changes();
		return true;
	}
	return false;
}

// Determine, whether the terminal is scrolled up
bool GenericTerminal::IsScrolledUp()
{
	return (tm.GetNumLinesScrolled() != 0);
}

//////////////////////////////////////////////////////////////////////////////
///  public GetScrollHeight
///  Calculates and returns a value used to determine what the size of the scrollbar thumb should be
///
///  @return int The number of lines received, capped at MAXHEIGHT
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int GenericTerminal::GetScrollHeight()
{
	int scrollHeight = tm.GetLinesReceived();

	int maxSize = tm.GetMaxSize();
	if (scrollHeight > maxSize)
	{
		scrollHeight = maxSize;
	}
	else
	{
		scrollHeight--;
	}

	return scrollHeight;
}

// Return the current scroll position
int GenericTerminal::GetScrollPosition()
{
	return tm.GetNumLinesScrolled();
}

// Set the terminal buffer size (not the length of the input history)
void GenericTerminal::SetTerminalHistory(int size)
{
	tm.SetMaxSize(size);
}
