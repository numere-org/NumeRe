
// Copyright Timothy Miller, 1999

#ifdef __GNUG__
#pragma implementation "gterm.hpp"
#endif

#include "gterm.hpp"
#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/filename.h>

/////////////////////////////////////////////////
/// \brief Simple wrapper around update_changes()
///
/// \return void
///
/////////////////////////////////////////////////
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
void GenericTerminal::ProcessInput(int len, const std::string& sData)
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


/////////////////////////////////////////////////
/// \brief Processes output returned from the
/// kernel and hands it over to the internal
/// buffer.
///
/// \param len int
/// \param sData const string&
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::ProcessOutput(int len, const std::string& sData)
{
    // Copy the input
	data_len += sData.length();
	sInput_Data += sData;

	// Evaluate the input and update the GUI
	normal_output();
	update_changes();
	CalltipCancel();
	return;
}


/////////////////////////////////////////////////
/// \brief Simple wrapper around reset().
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::Reset()
{
	reset();
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual ResizeTerminal
///  Resizes the terminal's display.  This ONLY changes what's being displayed, and
///  does NOT change the size as far as the server is concerned!
///
///  @param  _width  int  The new width, in characters
///  @param  _height int  The new height, in characters
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::ResizeTerminal(int _width, int _height)
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
    wxLogDebug("Resizing terminal: _width = %d, _height = %d", _width, _height);
#endif

    // Perform the resize in the internal buffer
	tm.Resize(_width, _height);

	// If the cursor is valid and has a non-zero position
    if (cursor && (cursor.pos || cursor.line))
    {
        // Use it as the new terminal view cursor
        termCursor = tm.toViewCursor(cursor);
        if (!termCursor)
            termCursor = tm.getCurrentViewPos();
    }

	width = _width;

    // Does the height change?
	if (_height != height)
    {
        height = _height;

        // Update the scroll indicators
        scroll_bot = height - 1;
        if (scroll_top >= height)
            scroll_top = 0;
    }

    // Update the GUI
	update_changes();
}


/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param w int
/// \param h int
///
/////////////////////////////////////////////////
GenericTerminal::GenericTerminal(int w, int h) : width(w), height(h)
{
	doing_update = 0;

	// Create the text manager
	tm = TextManager(this, w, h, MAXWIDTH, MAXHEIGHT);

	// Load the syntax
	wxFileName f(wxStandardPaths::Get().GetExecutablePath());
	_syntax.loadSyntax(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).ToStdString());

	// Set the terminal view cursor to top left
	// To make the cursor valid, we have to instantiate it with zeros
	termCursor = ViewCursor(0, 0);
	mode_flags = 0;
	reset();
	resetAutoComp(RESETCURSOR | RESETTAB);
}


/////////////////////////////////////////////////
/// \brief Empty destructor.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Gets the character at the selected
/// location.
///
/// \param x int
/// \param y int
/// \return unsigned char
///
/////////////////////////////////////////////////
unsigned char GenericTerminal::GetChar(int x, int y)
{
	if (x >= 0 && x < Width() && y >= 0 && y < Height())
		return tm.GetCharAdjusted(y, x);

	return 0;
}


/////////////////////////////////////////////////
/// \brief Gets the selected text (if any).
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string GenericTerminal::get_selected_text()
{
	return tm.getSelectedText();
}


/////////////////////////////////////////////////
/// \brief Get a pointer to the internal text
/// buffer.
///
/// \return TextManager*
///
/////////////////////////////////////////////////
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
		handle_calltip(termCursor.x, termCursor.y);
		return true;
	}
	return false;
}


/////////////////////////////////////////////////
/// \brief Determine, whether the terminal is
/// scrolled up.
///
/// \return bool
///
/////////////////////////////////////////////////
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
		scrollHeight = maxSize;

	return scrollHeight;
}


/////////////////////////////////////////////////
/// \brief Returns the current scroll position.
///
/// \return int
///
/////////////////////////////////////////////////
int GenericTerminal::GetScrollPosition()
{
	return tm.GetNumLinesScrolled();
}


/////////////////////////////////////////////////
/// \brief Set the terminal buffer size (not the
/// length of the input history). The length of
/// the history (i.e. the terminal length) is
/// determined by the width of the terminal and
/// the number of calculated line breaks.
///
/// \param size int
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::SetTerminalHistory(int size)
{
	tm.SetMaxSize(size);
}

