// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include "../../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Returns the encoded color bitlist
int GenericTerminal::calc_color( int fg, int bg, int flags )
{
	return (flags & 15) | (fg << 4) | (bg << 8);
}

//////////////////////////////////////////////////////////////////////////////
///  private update_changes
///  The main update function. Is responsible for preparing the text, which
///  shall be printed on screen
///
///  @return void
///
///  @author Timothy Miller @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::update_changes ()
{
	int c;
	int x;

	// prevent recursion for scrolls which cause exposures
	if (doing_update)
		return;
#ifdef DO_LOG
    wxLogDebug("Updating Terminal");
#endif

	doing_update = 1;

	// Go through the current height (number of lines) of the terminal
	for (size_t i = 0; i < height; i++)
	{
	    // Get the rendered string and the rendered color bitlists for
	    // the current line
		string line = tm.getRenderedString(i);
		vector<unsigned short> colors = tm.getRenderedColors(i);
		size_t lastPos = 0;

		// Print all characters together, which have the same color
		// (= a token in this context)
		for (size_t j = 1; j < line.length(); j++)
		{
			if (colors[j] != colors[j - 1])
			{
				string sSubstr = line.substr(lastPos, j - lastPos);
				c = colors[j - 1];
				DrawText((c >> 4) & 0xf, (c >> 8) & 0xf, c, lastPos, i, sSubstr);
				lastPos = j;
			}
		}

		// Manually print the last token
		if (line.length())
		{
			string sSubstr = line.substr(lastPos);
			c = colors[lastPos];
			DrawText((c >> 4) & 0xf, (c >> 8) & 0xf, c, lastPos, i, sSubstr);
		}

		// Clear the remaining line (otherwise one would read old text here)
		ClearChars(bg_color, line.length(), i, width - line.length(), 1);
	}

	// Draw the cursor
	if ( !(mode_flags & CURSORINVISIBLE) )
	{
	    // Get its x coordinate and ensure that it is valid
		x = termCursor.x;
		if ( x >= width )
			x = width - 1;

        // Get color and the character at the position of the cursor
		c = tm.GetColorAdjusted(termCursor.y, x);
		unsigned char cursorChar = tm.GetCharAdjusted(termCursor.y, x);

		// Draw the cursor
        DrawCursor((c >> 4) & 0xf, (c >> 8) & 0xf, c & 15, x, termCursor.y, cursorChar);
	}
	doing_update = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  private clear_area
///  Clears out a given area on the screen
///
///  @param  start_x int  The starting x coordinate
///  @param  start_y int  The starting y coordinate
///  @param  end_x   int  The ending x coordinate
///  @param  end_y   int  The ending y coordinate
///
///  @return void
///
///  @author Timothy Miller @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::clear_area( int start_x, int start_y, int end_x, int end_y )
{
    tm.clearRange(ViewCursor(start_x, start_y), ViewCursor(end_x, end_y));
}

//////////////////////////////////////////////////////////////////////////////
///  private move_cursor
///  Moves the cursor to a new location
///
///  @param  x    int  The new x coordinate
///  @param  y    int  The new y coordinate
///
///  @return void
///
///  @author Timothy Miller @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::move_cursor( int x, int y )
{
	if ( termCursor.x >= width )
		termCursor.x = width - 1;

    termCursor.move(x, y);

    if (!tm.toLogicalCursor(termCursor))
        termCursor = tm.getCurrentViewPos();
}

// Moves the cursor to a location, if this location is editable
void GenericTerminal::move_cursor_editable_area( int x, int y )
{
	if (tm.IsEditable(y, x))
		move_cursor(x, y);
}

// Sets a mode flag (only used to make the cursor invisble)
void GenericTerminal::set_mode_flag( int flag )
{
	mode_flags |= flag;

	ModeChange(mode_flags);
}

// Clears a mode flag (mainly used to make the cursor visible again)
void GenericTerminal::clear_mode_flag( int flag )
{
	mode_flags &= ~flag;

	ModeChange(mode_flags);
}
