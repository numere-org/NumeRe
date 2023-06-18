// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include <wx/log.h>

extern Language _guilang;

using namespace std;


/////////////////////////////////////////////////
/// \brief Returns the encoded color bitlist.
///
/// \param fg int
/// \param bg int
/// \param flags int
/// \return int
///
/////////////////////////////////////////////////
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
	for (int i = 0; i < height; i++)
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
	if ( termCursor.x >= (size_t)width )
		termCursor.x = width - 1;

    termCursor.move(x, y);

    if (!tm.toLogicalCursor(termCursor))
        termCursor = tm.getCurrentViewPos();

    handle_calltip(termCursor.x, termCursor.y);
}


/////////////////////////////////////////////////
/// \brief Returns true, if a color corresponds
/// to a syntax element, which has a (context,
/// e.g. a function) tooltip.
///
/// \param c short int
/// \param inParens bool
/// \return bool
///
/////////////////////////////////////////////////
static inline bool hasContextToolTip(short int c, bool inParens)
{
    return c == NumeReSyntax::SYNTAX_COMMAND
        || (c == NumeReSyntax::SYNTAX_FUNCTION && inParens)
        || (c == NumeReSyntax::SYNTAX_METHODS && inParens)
        || (c == NumeReSyntax::SYNTAX_PROCEDURE && inParens);
}


/////////////////////////////////////////////////
/// \brief Returns true, if a color corresponds
/// to a syntax element, which doesn't have a
/// (context, e.g. a function) tooltip.
///
/// \param c short int
/// \return bool
///
/////////////////////////////////////////////////
static inline bool hasNonContextToolTip(short int c)
{
    return c == NumeReSyntax::SYNTAX_OPTION
        || c == NumeReSyntax::SYNTAX_CONSTANT;
}


/////////////////////////////////////////////////
/// \brief Check, whether a calltip is needed and
/// select the corresponding text from the
/// CallTipProvider.
///
/// \param x int
/// \param y int
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::handle_calltip(int x, int y)
{
    // Do nothing, if the terminal is scrolled up
    if (IsScrolledUp())
    {
        CalltipCancel();
        return;
    }

    // if the position is on a relevant syntax element
    // create a calltip for this element, otherwise
    // dismiss it
    std::string sLine = tm.getRenderedString(y);
    std::vector<unsigned short> vColors = tm.getRenderedColors(y);

    if ((int)vColors.size() <= x || x < 0)
    {
        CalltipCancel();
        return;
    }

    // If we're not directly on a tool-tip specific element,
    // then we'll search a context tooltip left of the current
    // position.
    if (!hasContextToolTip((vColors[x] >> 4) & 0xf, true) // to enable actual detection, we set us to be in a parenthesis
        && !hasNonContextToolTip((vColors[x] >> 4) & 0xf))
    {
        bool isInParens = false;
        size_t cursor_x = x;

        while (x >= 0)
        {
            if (((vColors[x] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR && (sLine[x] == '(' || sLine[x] =='{'))
                isInParens = getMatchingParenthesis(StringView(sLine, x)) >= cursor_x-x;

            // Functions and commands
            if (hasContextToolTip((vColors[x] >> 4) & 0xf, isInParens))
                break;

            // options and constants (only near cursor)
            if (cursor_x-x <= 2
                && hasNonContextToolTip((vColors[x] >> 4) & 0xf)
                && (sLine[cursor_x] == ' ' || ((vColors[cursor_x] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR))
                break;

            x--;
        }
    }

    // If we found nothing, return
    if (x < 0)
    {
        CalltipCancel();
        return;
    }

    int posStart = x;
    size_t posEnd = x;

    while (posStart > 0 && vColors[posStart-1] == vColors[x])
        posStart--;

    while (posEnd < vColors.size() && vColors[posEnd] == vColors[x])
        posEnd++;

    std::string sSyntaxElement = sLine.substr(posStart, posEnd - posStart);
    NumeRe::CallTip _cTip;

    // Determine the type of the color
    switch ((vColors[x] >> 4) & 0xf)
    {
        case NumeReSyntax::SYNTAX_COMMAND:
            _cTip = m_tipProvider.getCommand(sSyntaxElement);
            break;
        case NumeReSyntax::SYNTAX_FUNCTION:
            _cTip = m_tipProvider.getFunction(sSyntaxElement);
            break;
        case NumeReSyntax::SYNTAX_PROCEDURE:
            _cTip = m_tipProvider.getProcedure(sSyntaxElement);
            break;
        case NumeReSyntax::SYNTAX_METHODS:
            _cTip = m_tipProvider.getMethod(sSyntaxElement);
            break;
        case NumeReSyntax::SYNTAX_CONSTANT:
            _cTip = m_tipProvider.getConstant(sSyntaxElement);
            break;
        case NumeReSyntax::SYNTAX_OPTION:
            _cTip = m_tipProvider.getOption(sSyntaxElement);
            break;
        default:
            CalltipCancel();
            return;
    }

    // Do not display empty calltips
    if (!_cTip.sDefinition.length())
    {
        CalltipCancel();
        return;
    }

    Calltip(posStart, y, _cTip);
}


/////////////////////////////////////////////////
/// \brief Detect the type of the symbol left to
/// the selected position (is used to detect the
/// type of the method root variable).
///
/// \param x int
/// \param y int
/// \return NumeReSyntax::SyntaxColors
///
/////////////////////////////////////////////////
NumeReSyntax::SyntaxColors GenericTerminal::get_method_root_type(int x, int y)
{
    // Get the rendered line and the corresponding syntax colors
    std::string sLine = tm.getRenderedString(y);
    std::vector<unsigned short> vColors = tm.getRenderedColors(y);

    if ((int)vColors.size() <= x || x < 1)
        return NumeReSyntax::SYNTAX_METHODS;

    // Should replicate the logic from the editor
    if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_STD)
        return NumeReSyntax::SYNTAX_STD;
    else if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_METHODS
             || (x > 3
                 && sLine.substr(x-2,2) == "()"
                 && (((vColors[x-3] >> 4) & 0xf) == NumeReSyntax::SYNTAX_STD
                     || ((vColors[x-3] >> 4) & 0xf) == NumeReSyntax::SYNTAX_SPECIALVAL)))
        return NumeReSyntax::SYNTAX_TABLE;

    return NumeReSyntax::SYNTAX_METHODS;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to a location, if
/// this location is editable.
///
/// \param x int
/// \param y int
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::move_cursor_editable_area( int x, int y )
{
	if (tm.IsEditable(y, x))
		move_cursor(x, y);
}


/////////////////////////////////////////////////
/// \brief Sets a mode flag (only used to make
/// the cursor invisble).
///
/// \param flag int
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::set_mode_flag( int flag )
{
	mode_flags |= flag;

	ModeChange(mode_flags);
}


/////////////////////////////////////////////////
/// \brief Clears a mode flag (mainly used to
/// make the cursor visible again).
///
/// \param flag int
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::clear_mode_flag( int flag )
{
	mode_flags &= ~flag;

	ModeChange(mode_flags);
}


