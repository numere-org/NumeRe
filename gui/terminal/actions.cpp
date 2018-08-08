// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include "../../common/debug.h"

#ifdef _DEBUG

#define new DEBUG_NEW

#endif

#define GTERM_PC
// For efficiency, this grabs all printing characters from buffer, up to
// the end of the line or end of buffer

//////////////////////////////////////////////////////////////////////////////
///  private normal_input
///  Handles most of the input stuff for the GenericTerminal
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GenericTerminal::normal_input()
{
    // Convert the current view cursor position into
    // the corresponding logical position
	LogicalCursor cursor = tm.toLogicalCursor(termCursor);

	// Ensure that the cursor is valid
	if (!cursor)
        cursor = tm.getCurrentLogicalPos();

    // Insert the text at the cursor's position
	tm.insertInput(sInput_Data, cursor.pos);

	// Advance the cursor
	cursor.pos += sInput_Data.length();

	// Clear the input buffer
	sInput_Data.clear();

	// Get the new view cursor position from the logical cursor
	termCursor = tm.toViewCursor(cursor);

	// If the view cursor is not valid, get the
	// view cursor of the last position
	if (!termCursor)
        termCursor = tm.getCurrentViewPos();
}

/** \brief This member function is for printing the kernel's output to the console
 *
 * \return void
 *
 */
void GenericTerminal::normal_output()
{
    // Get the output data
	string sInput = sInput_Data;

	wxLogDebug(sInput.c_str());

	// As long as the output data has a length
	do
	{
	    // Print the output until the first control character
		tm.printOutput(sInput.substr(0, sInput.find_first_of("\n\r\t")));

		// Store the position of the first control character
		size_t nPos = sInput.find_first_of("\n\r\t");

		// If the position is valid
		if (nPos != string::npos)
		{
		    // Evalute the control character
		    switch (sInput[nPos])
		    {
                case '\n':
                    tm.newLine();
                    break;
                case '\t':
                    tm.tab();
                    break;
                case '\r':
                    tm.eraseLine();
                    break;
		    }

		    // Erase everything in front of and including
		    // the control character
            sInput.erase(0, nPos + 1);
		}
		else
		{
			sInput.clear();
		}

	}
	while (sInput.length());

	// Set the view cursor to the last position
	// and clear the input data
	termCursor = tm.getCurrentViewPos();
	sInput_Data.clear();
}

// Reset the current autocompletion list and the corresponding variables
void GenericTerminal::resetAutoComp()
{
	if (nTabStartPos == -1)
		return;
	nTabStartPos = -1;
	sAutoCompList.clear();
	sAutoCompWordStart.clear();
}

// This member function evalutes the procedure signature and returns its namespace
// Will only be called from GTerm::tab()
string GenericTerminal::getProcNameSpace()
{
	string sNameSpace;

	// Get the position of the current autocompletion start
	int nNameSpacePos = nTabStartPos - 1;

	// Find the start of the current procedure syntax
	while (nNameSpacePos && ((tm.GetColorAdjusted(termCursor.y, nNameSpacePos - 1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE && tm.GetCharAdjusted(termCursor.y, nNameSpacePos - 1) != '$')
		nNameSpacePos--;

	// Get the text between the start and the current autocompletion start
	sNameSpace = tm.GetTextRange(termCursor.y, nNameSpacePos, nTabStartPos);

	// If the obtained namespace contains the (possible) procedure
	// name, then erase this part, because we only want to have the
	// current namespace
	if (sNameSpace.find(sAutoCompWordStart) != string::npos)
		sNameSpace.erase(sNameSpace.find(sAutoCompWordStart));

	// return the evaluted namespace
	return sNameSpace;
}

// Carriage return
void GenericTerminal::cr()
{
	move_cursor(0, termCursor.y);
}

// Line feed
void GenericTerminal::lf()
{
	tm.newLine();

	if (termCursor.y < scroll_bot)
	{
		move_cursor(termCursor.x, termCursor.y + 1);
	}
}

// Form feed (not used)
void GenericTerminal::ff()
{
	clear_area(0, scroll_top, width - 1, scroll_bot);

	move_cursor(0, scroll_top);
}

// Evalute the tab key -> Try to autocomplete the current input
void GenericTerminal::tab()
{
    // Determine, whether this is the first tab key press (in this case
    // the tab start position is equal to -1)
	if (nTabStartPos == -1)
	{
		nTabStartPos = termCursor.x;

		// Get the word start from the terminal
		sAutoCompWordStart = tm.GetWordStartAt(termCursor.y, termCursor.x);

		// There are different autocompletion lists for procedures and every other syntax element
		if (((tm.GetColorAdjusted(termCursor.y, termCursor.x - 1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE)
		{
			string sNameSpace = getProcNameSpace();
			sAutoCompList = _syntax.getProcAutoCompList(sAutoCompWordStart, "", sNameSpace);
		}
		else
			sAutoCompList = _syntax.getAutoCompList(sAutoCompWordStart);

        // Reset the autocompletion, if no completion was found or the word start is too short
		if (!sAutoCompList.length() || !sAutoCompWordStart.length())
		{
			resetAutoComp();
			return;
		}
	}
	else
	{
	    // This is not the first time the tab key was pressed
	    // If the autocompletion list is empty, recreate it here
		if (!sAutoCompList.length())
		{
			if (((tm.GetColorAdjusted(termCursor.y, nTabStartPos - 1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE)
			{
				string sNameSpace = getProcNameSpace();
				sAutoCompList = _syntax.getProcAutoCompList(sAutoCompWordStart, "", sNameSpace);
			}
			else
				sAutoCompList = _syntax.getAutoCompList(sAutoCompWordStart);
		}

		// clear the completed part and move the cursor to the tab start position
		clear_area(nTabStartPos, termCursor.y, termCursor.x, termCursor.y);
		move_cursor(nTabStartPos, termCursor.y);
	}

	// Get the next autocompletion proposal, store it as input and remove it from the list
	sInput_Data = sAutoCompList.substr(sAutoCompWordStart.length(), sAutoCompList.find('?') - sAutoCompWordStart.length());
	data_len = sInput_Data.length();
	sAutoCompList.erase(0, sAutoCompList.find(' ') + 1);

	// Process the input
	normal_input();
}

// backspace
bool GenericTerminal::bs()
{
    // Convert the current view cursor to a logical cursor
	LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    // Ensure that the position is editable and perform the backspace
	if (termCursor.x > 0 && tm.IsEditable(termCursor.y, termCursor.x - 1))
	{
		tm.backspace(cursor);
	}
	else
		return false;

    // move the cursor to the left
	cursor--;

	// Convert it to a new view cursor
	termCursor = tm.toViewCursor(cursor);

	// If the view cursor is not valid, use the current input position
    if (!termCursor)
        termCursor = tm.getCurrentViewPos();

    // Update the GUI and return
	update_changes();
	return true;
}

// Delete
bool GenericTerminal::del()
{
    // Convert the current view cursor to a logical cursor
	LogicalCursor cursor = tm.toLogicalCursor(termCursor);

	// Move the cursor to the right
	cursor++;

	// Ensure that the position to the right is editable and perform the backspace
	if (termCursor.x > 0 && tm.IsEditable(termCursor.y, termCursor.x + 1))
	{
		tm.backspace(cursor);
	}
	else
		return false;

    // The view cursor stays at its position so
    // only update the GUI and return
	update_changes();
	return true;
}

// bell sound (not used)
void GenericTerminal::bell()
{
	Bell();
}

// Resets the terminal so that it starts with an empty buffer
void GenericTerminal::reset()
{
	bg_color = 0;
	fg_color = 7;
	scroll_top = 0;
	scroll_bot = height - 1;

	clear_mode_flag(NOEOLWRAP | CURSORAPPMODE | CURSORRELATIVE | NEWLINE | INSERT | UNDERLINE | BLINK | KEYAPPMODE
					| CURSORINVISIBLE);

	clear_area(0, 0, width - 1, height - 1);
	move_cursor(0, 0);

	tm.Reset();
}

// Moves the cursor to the left
bool GenericTerminal::cursor_left()
{
	if (!termCursor.x || !tm.IsEditable(termCursor.y, termCursor.x - 1))
		return false;

	if (termCursor--)
        return true;
    return false;
}

// Moves the cursor to the right
bool GenericTerminal::cursor_right()
{
	if (termCursor.x + 1 >= width || !tm.IsEditable(termCursor.y, termCursor.x + 1))
		return false;

	termCursor++;

	return true;
}

// Either moves the cursor up or performs a history jump
bool GenericTerminal::cursor_up()
{
	if (!tm.IsEditable(termCursor.y - 1, termCursor.x))
	{
	    // Perform a history jump
		string sHistory = tm.GetInputHistory(true);
		if (sHistory.length())
		{
			erase_usercontent_line();
			sInput_Data = sHistory;
			data_len = sHistory.length();
			normal_input();
			return true;
		}
		return false;
	}

	// Move the cursor up
	int n, y;

	n = param[0];

	if (n < 1)
		n = 1;

	y = termCursor.y - n;

	if (y < 0)
		y = 0;

	move_cursor(termCursor.x, y);
	return true;
}

// Either moves the cursor down or performs a history jump
bool GenericTerminal::cursor_down()
{
	if (!tm.IsEditable(termCursor.y + 1, termCursor.x))
	{
	    // Perform a history jump
		string sHistory = tm.GetInputHistory(false);
		if (sHistory.length())
		{
			erase_usercontent_line();
			sInput_Data = sHistory;
			data_len = sHistory.length();
			normal_input();
			return true;
		}
		return false;
	}

	// Move the cursor down
	int n, y;

	n = param[0];

	if (n < 1)
		n = 1;

	y = termCursor.y + n;

	if (y >= height)
		y = height - 1;

	move_cursor(termCursor.x, y);
	return true;
}

// Moves the cursor to the left most position in the current line
bool GenericTerminal::home()
{
	int n = termCursor.x;

	// Search the last not editable character from the right
	while (tm.IsEditable(termCursor.y, n - 1))
		n--;

    // Don't do anything, if the cursor is already there
	if (n == termCursor.x)
		return false;

    // Move the cursor
	move_cursor(n, termCursor.y);
	return true;
}

// Moves the cursor to the rightmost position in the current line
bool GenericTerminal::end()
{
	int n = termCursor.x;

	// Search the first not editable character from the left
	while (tm.IsEditable(termCursor.y, n + 1))
		n++;

    // Don't do anything, if the cursor is already there
	if (n == termCursor.x)
		return false;

    // Move the cursor
	move_cursor(n, termCursor.y);
	return true;
}

// Erases the current line in the internal buffer
void GenericTerminal::erase_line()
{
    tm.eraseLine();
}

// Erases alle user-written contents from the current line
void GenericTerminal::erase_usercontent_line()
{
	if (!tm.IsEditable(termCursor.y, termCursor.x))
		return;

    // Convert the current view cursor into a logical cursor
	LogicalCursor cursor = tm.toLogicalCursor(termCursor);
	cursor.pos--;

	// While the current character is editable, erase it
	while (tm.IsEditableLogical(cursor))
	{
		cursor.pos++;
		tm.backspace(cursor);
		cursor.pos -= 2;
	}

	// Get the new view cursor
	termCursor = tm.toViewCursor(cursor);
    termCursor.x++;

    // If the view cursor is not valid, use the current input location
    if (!termCursor)
        termCursor = tm.getCurrentViewPos();
}
