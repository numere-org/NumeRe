// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include "../guilang.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include <wx/log.h>

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
		wxString line = tm.getRenderedString(i);
		std::vector<unsigned short> colors = tm.getRenderedColors(i);
		size_t lastPos = 0;

		// Print all characters together, which have the same color
		// (= a token in this context)
		for (size_t j = 1; j < line.length(); j++)
		{
			if (colors[j] != colors[j - 1])
			{
				wxString sSubstr = line.substr(lastPos, j - lastPos);
				c = colors[j - 1];
				DrawText((c >> 4) & 0xf, (c >> 8) & 0xf, c, lastPos, i, sSubstr);
				lastPos = j;
			}
		}

		// Manually print the last token
		if (line.length())
		{
			wxString sSubstr = line.substr(lastPos);
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
		wxChar cursorChar = tm.GetCharAdjusted(termCursor.y, x);

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
    wxString sLine = tm.getRenderedString(y);
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
            if (((vColors[x] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR && (sLine[x] == '(' || sLine[x] == '{'))
                isInParens = getMatchingParenthesis(StringView(sLine.ToAscii().data(), x)) >= cursor_x-x;

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

    std::string sSyntaxElement = sLine.substr(posStart, posEnd - posStart).ToAscii().data();
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
            _cTip = m_tipProvider.getMethod(sSyntaxElement, get_method_root_type(posStart-1, y).first);
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
/// \return std::pair<std::string, bool>
///
/////////////////////////////////////////////////
std::pair<std::string, bool> GenericTerminal::get_method_root_type(int x, int y)
{
    // Get the rendered line and the corresponding syntax colors
    wxString sLine = tm.getRenderedString(y);
    std::vector<unsigned short> vColors = tm.getRenderedColors(y);
    bool isVect = false;
    std::string varType = "";

    if ((int)vColors.size() <= x || x < 1)
        return std::make_pair(varType, isVect);

    // Should replicate the logic from the editor
    if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_STD)
    {
        int posStart = x-1;

        while (posStart > 0 && vColors[posStart-1] == vColors[x-1] && isalnum(sLine[posStart-1]))
            posStart--;

        varType = getVariableType(sLine.substr(posStart, x - posStart).ToAscii().data());
        isVect = varType.find('{') != std::string::npos || varType == "cluster";

        if (varType.front() == '{')
            varType = varType.substr(1, varType.length()-2);
    }
    else if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_STRING)
        varType = "string";
    else if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_METHODS)
    {
        // Examine method return values
        size_t p = sLine.rfind('.', x-1);
        std::string sReturnValue = m_tipProvider.getMethodReturnValue(sLine.substr(p+1, x-p-1).ToAscii().data(), get_method_root_type(p, y).first);

        if (sReturnValue.find("{}") != std::string::npos || sReturnValue.find("{*}") != std::string::npos)
        {
            varType = "table";
            isVect = true;
        }
        else
        {
            if (sReturnValue.find("STR") != std::string::npos)
                varType = "string";
            else if (sReturnValue.find("CAT") != std::string::npos)
                varType = "category";
            else if (sReturnValue.find("DCT") != std::string::npos)
                varType = "dictstruct";
            else if (sReturnValue.find("OBJ") != std::string::npos
                     || sReturnValue.find("ARG") != std::string::npos
                     || sReturnValue.find("CST") != std::string::npos)
                varType = "*";

            isVect = sReturnValue.find('{') != std::string::npos || sReturnValue.find("CST") != std::string::npos;
        }
    }
    else if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR && sLine[x-1] == '}')
    {
        // Examine vector braces. Find start position first
        int x0 = x-2;
        int p = 1;

        while (x0 > 0)
        {
            if (((vColors[x0] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR)
            {
                p += (sLine[x0] == '}') - (sLine[x0] == '{') + (sLine[x0] == ')') - (sLine[x0] == '(');

                if (!p && sLine[x0] == '{')
                    break;
            }

            x0--;
        }

        if (!p && sLine[x0] == '{')
        {
            int prevStyle = ((vColors[x0-1] >> 4) & 0xf);

            varType = ((prevStyle == NumeReSyntax::SYNTAX_STD
                        || prevStyle == NumeReSyntax::SYNTAX_SPECIALVAL)
                       && isalnum(sLine[x0-1]))
                || sLine[x0-1] == '#'
                || sLine.substr(x0, x-1-x0).find('"') != std::string::npos
                ? "string" : "*";
        }

        isVect = true;
    }
    else if (((vColors[x-1] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR && sLine[x-1] == ')')
    {
        // Examine parentheses. Find start position first
        int x0 = x-2;
        int p = 1;

        while (x0 > 0)
        {
            if (((vColors[x0] >> 4) & 0xf) == NumeReSyntax::SYNTAX_OPERATOR)
            {
                p += (sLine[x0] == '}') - (sLine[x0] == '{') + (sLine[x0] == ')') - (sLine[x0] == '(');

                if (!p && sLine[x0] == '(')
                    break;
            }

            x0--;
        }

        if (!p && sLine[x0] == '(')
        {
            int prevStyle = ((vColors[x0-1] >> 4) & 0xf);

            if (prevStyle == NumeReSyntax::SYNTAX_FUNCTION
                || prevStyle == NumeReSyntax::SYNTAX_METHODS
                || prevStyle == NumeReSyntax::SYNTAX_PROCEDURE)
            {
                int posStart = x0-1;

                while (posStart > 0 && vColors[posStart-1] == vColors[x0-1])
                    posStart--;

                std::string sSymbolName = sLine.substr(posStart, x0 - posStart).ToAscii().data();
                std::string sReturnValue;

                // Determine the type of the color
                switch (prevStyle)
                {
                    case NumeReSyntax::SYNTAX_FUNCTION:
                        sReturnValue = m_tipProvider.getFunctionReturnValue(sSymbolName);
                        break;
                    case NumeReSyntax::SYNTAX_PROCEDURE:
                        sReturnValue = m_tipProvider.getProcedureReturnValue(sSymbolName);
                        break;
                    case NumeReSyntax::SYNTAX_METHODS:
                        sReturnValue = m_tipProvider.getMethodReturnValue(sSymbolName, get_method_root_type(posStart-1, y).first);
                        break;
                }

                if (sReturnValue.find("{}") != std::string::npos || sReturnValue.find("{*}") != std::string::npos)
                {
                    varType = "table";
                    isVect = true;
                }
                else
                {
                    if (sReturnValue.find("STR") != std::string::npos)
                        varType = "string";
                    else if (sReturnValue.find("CAT") != std::string::npos)
                        varType = "category";
                    else if (sReturnValue.find("DCT") != std::string::npos)
                        varType = "dictstruct";
                    else if (sReturnValue.find("OBJ") != std::string::npos)
                        varType = "object." + sSymbolName;
                    else if (sReturnValue.find("ARG") != std::string::npos
                             || sReturnValue.find("CST") != std::string::npos)
                        varType = "*";

                    isVect = sReturnValue.find('{') != std::string::npos || sReturnValue.find("CST") != std::string::npos;
                }
            }
            else if (prevStyle == NumeReSyntax::SYNTAX_OPERATOR && sLine[x0-1] == '#')
                varType = "string";
            else if (prevStyle == NumeReSyntax::SYNTAX_STD || prevStyle == NumeReSyntax::SYNTAX_SPECIALVAL)
            {
                varType = sLine.substr(x0, x-x0) == "()" ? "table" : "*";
                isVect = true;
            }
        }
    }

    return std::make_pair(varType, isVect);
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


