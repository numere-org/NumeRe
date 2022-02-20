// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include <wx/log.h>

#define GTERM_PC
// For efficiency, this grabs all printing characters from buffer, up to
// the end of the line or end of buffer

// Forward declaration
std::string toLowerCase(const std::string&);

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

    handle_calltip(termCursor.x, termCursor.y);
}


/////////////////////////////////////////////////
/// \brief This member function is for printing
/// the kernel's output to the console.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::normal_output()
{
    // Get the output data
    string sInput = sInput_Data;

#ifdef DO_LOG
    wxLogDebug("%s", sInput.c_str());
#endif

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


/////////////////////////////////////////////////
/// \brief Reset the current autocompletion list
/// and the corresponding variables.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::resetAutoComp(int mode)
{
    // Reset the cursor position after using the up down keys
    if (mode & RESETCURSOR)
    {
        nCursorUpDownStartPos = -1;
        tm.ResetVirtualCursorLine();
    }

    // Reset the cursor position for the tab key
    if (!(mode & RESETTAB) || nTabStartPos == -1)
        return;
    nTabStartPos = -1;

    sAutoCompList.clear();
    sAutoCompWordStart.clear();
}


/////////////////////////////////////////////////
/// \brief This member function evalutes the
/// procedure signature and returns its
/// namespace. Will only be called from
/// GTerm::tab().
///
/// \return string
///
/////////////////////////////////////////////////
string GenericTerminal::getProcNameSpace()
{
    string sNameSpace;

    // Get the position of the current autocompletion start
    int nNameSpacePos = nTabStartPos - 1;

    // Find the start of the current procedure syntax
    while (nNameSpacePos && ((tm.GetColorAdjusted(termCursor.y, nNameSpacePos - 1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE && tm.GetCharAdjusted(termCursor.y, nNameSpacePos - 1) != '$')
        nNameSpacePos--;

    // Get the text between the start and the current autocompletion start
    sNameSpace = toLowerCase(tm.GetTextRange(termCursor.y, nNameSpacePos, nTabStartPos));

    // If the obtained namespace contains the (possible) procedure
    // name, then erase this part, because we only want to have the
    // current namespace
    if (sNameSpace.find(sAutoCompWordStart) != string::npos)
        sNameSpace.erase(sNameSpace.rfind(sAutoCompWordStart));

    // return the evaluted namespace
    return sNameSpace;
}


/////////////////////////////////////////////////
/// \brief Insert a carriage return.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::cr()
{
    move_cursor(0, termCursor.y);
}


/////////////////////////////////////////////////
/// \brief Insert a line feed.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::lf()
{
    tm.newLine();

    if (termCursor.y < (size_t)scroll_bot)
    {
        move_cursor(termCursor.x, termCursor.y + 1);
    }
}


/////////////////////////////////////////////////
/// \brief Insert a form feed (not used).
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::ff()
{
    clear_area(0, scroll_top, width - 1, scroll_bot);

    move_cursor(0, scroll_top);
}


/////////////////////////////////////////////////
/// \brief Evaluate the tab key (do not insert a
/// tab character but try to autocomplete the
/// current input).
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::tab()
{
    // Determine, whether this is the first tab key press (in this case
    // the tab start position is equal to -1)
    if (nTabStartPos == -1)
    {
        nTabStartPos = termCursor.x;

        // Get the word start from the terminal
        sAutoCompWordStart = toLowerCase(tm.GetWordStartAt(termCursor.y, termCursor.x));

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
            resetAutoComp(RESETCURSOR | RESETTAB);
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
    }

    // clear the word start and move the cursor back to the word start
    // (word start cleared to avoid character case issues)
    clear_area(nTabStartPos - sAutoCompWordStart.length(), termCursor.y, termCursor.x, termCursor.y);
    move_cursor(nTabStartPos - sAutoCompWordStart.length(), termCursor.y);

    // Get the next autocompletion proposal, store it as input and remove it from the list
    sInput_Data = sAutoCompList.substr(0, sAutoCompList.find('?'));
    data_len = sInput_Data.length();
    sAutoCompList.erase(0, sAutoCompList.find(' ') + 1);

    // Process the input
    normal_input();
}


/////////////////////////////////////////////////
/// \brief Perform a backspace operation.
///
/// \return bool
///
/////////////////////////////////////////////////
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

    handle_calltip(termCursor.x, termCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Perform a delete key operation.
///
/// \return bool
///
/////////////////////////////////////////////////
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

    handle_calltip(termCursor.x, termCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Delete a selected block.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::delSelected()
{
    if (!tm.IsEditable(termCursor.y, termCursor.x) || !tm.isSelected(termCursor))
        return false;

    LogicalCursor curStart = tm.toLogicalCursor(termCursor);
    LogicalCursor curEnd = tm.toLogicalCursor(termCursor);

    // Reverse the starting point (if selected left of
    // the current cursor)
    while (tm.IsEditableLogical(curStart) && tm.isSelectedLogical(curStart) && curStart.pos)
        curStart--;

    curStart++;

    // Advance the ending point (if selected right of
    // the current cursor)
    while (tm.IsEditableLogical(curEnd) && tm.isSelectedLogical(curEnd) && curEnd < tm.getCurrentLogicalPos())
        curEnd++;

    tm.clearRange(tm.toViewCursor(curStart), tm.toViewCursor(curEnd));

    termCursor = tm.toViewCursor(curStart);
    update_changes();

    handle_calltip(termCursor.x, termCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Perform a bell sound (not used).
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::bell()
{
    Bell();
}


/////////////////////////////////////////////////
/// \brief Resets the terminal, so that it starts
/// with an empty buffer.
///
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Moves the cursor to the left.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::cursor_left()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    if (!cursor--)
        return false;

    if (!tm.IsEditableLogical(cursor))
        return false;

    termCursor = tm.toViewCursor(cursor);

    handle_calltip(termCursor.x, termCursor.y);

    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to the right.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::cursor_right()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    cursor++;

    if (!tm.IsEditableLogical(cursor))
        return false;

    termCursor = tm.toViewCursor(cursor);

    handle_calltip(termCursor.x, termCursor.y);

    return true;
}


/////////////////////////////////////////////////
/// \brief Either moves the cursor up or performs
/// a history jump.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::cursor_up()
{
    // Store the cursor position
    if (nCursorUpDownStartPos == -1)
        nCursorUpDownStartPos = tm.toLogicalCursor(termCursor).pos - 4;

    if (!tm.IsEditable(termCursor.y - 1, termCursor.x))
    {
        // Get the last history entry
        std::string sHistory = tm.GetInputHistory(true);

        // Get the current user input line
        std::string currentLine = tm.getCurrentInputLine();

        while (sHistory.length())
        {
            wxLogDebug(std::to_string(nCursorUpDownStartPos).c_str());
            if (sHistory != currentLine && sHistory.substr(0, nCursorUpDownStartPos) == currentLine.substr(0, nCursorUpDownStartPos))
            {
                erase_usercontent_line();
                sInput_Data = sHistory;
                data_len = sHistory.length();
                normal_input();
                return true;
            }
            sHistory = tm.GetInputHistory(true);
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


/////////////////////////////////////////////////
/// \brief Either moves the cursor down or
/// performs a history jump.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::cursor_down()
{
    // Store the cursor position
    if (nCursorUpDownStartPos == -1)
        nCursorUpDownStartPos = tm.toLogicalCursor(termCursor).pos - 4;

    if (!tm.IsEditable(termCursor.y + 1, termCursor.x))
    {
        // Get the last history entry
        std::string sHistory = tm.GetInputHistory(false);

        // Get the current user input line
        std::string currentLine = tm.getCurrentInputLine();

        while (sHistory.length())
        {
            if (sHistory != currentLine && sHistory.substr(0, nCursorUpDownStartPos) == currentLine.substr(0, nCursorUpDownStartPos))
            {
                erase_usercontent_line();
                sInput_Data = sHistory;
                data_len = sHistory.length();
                normal_input();
                return true;
            }
            sHistory = tm.GetInputHistory(false);
        }

        // Reset to original input if no other result was found
        if ((int)currentLine.length() > nCursorUpDownStartPos)
        {
            erase_usercontent_line();
            sInput_Data = currentLine.substr(0, nCursorUpDownStartPos);
            data_len = nCursorUpDownStartPos;
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


/////////////////////////////////////////////////
/// \brief Moves the cursor one word to the left.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::ctrl_left()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    // If already at the beginning of a word, go one
    // position to the left
    if (!isalnum(tm.GetCharLogical(cursor - 1)))
        cursor--;

    // Always go to the first non-whitespace character
    while (!isalnum(tm.GetCharLogical(cursor)) && tm.IsEditableLogical(cursor))
        cursor--;

    // Search now the first whitespace character to the
    // left
    while (isalnum(tm.GetCharLogical(cursor)) && tm.IsEditableLogical(cursor))
        cursor--;

    // Go back to the first word character
    cursor++;

    if (!tm.IsEditableLogical(cursor))
        return false;

    termCursor = tm.toViewCursor(cursor);
    handle_calltip(termCursor.x, termCursor.y);

    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor one word to the
/// right.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::ctrl_right()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    // Search for the next whitespace character to the
    // right
    while (isalnum(tm.GetCharLogical(cursor)) && tm.IsEditableLogical(cursor))
        cursor++;

    // Go to the first word character
    while (!isalnum(tm.GetCharLogical(cursor)) && tm.IsEditableLogical(cursor))
        cursor++;

    if (!tm.IsEditableLogical(cursor))
        return false;

    termCursor = tm.toViewCursor(cursor);
    handle_calltip(termCursor.x, termCursor.y);

    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to the leftmost
/// position in the current line.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::home()
{
    int n = termCursor.x;

    // Search the last not editable character from the right
    while (tm.IsEditable(termCursor.y, n - 1))
        n--;

    // Don't do anything, if the cursor is already there
    if (n == (int)termCursor.x)
        return front();

    // Move the cursor
    move_cursor(n, termCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to the rightmost
/// position in the current line.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::end()
{
    size_t n = termCursor.x;

    // Search the first not editable character from the left
    while (tm.IsEditable(termCursor.y, n + 1))
        n++;

    // Don't do anything, if the cursor is already there
    if (n == termCursor.x)
        return back();

    // Move the cursor
    move_cursor(n, termCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to the leftmost
/// position in the whole input.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::front()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    // Search the last not editable character from the right
    while (tm.IsEditableLogical(cursor))
        cursor--;

    // Move the cursor to the first editable position
    cursor++;

    // Create a corresponding view cursor
    ViewCursor vCursor = tm.toViewCursor(cursor);

    // Ensure that the cursor is different
    if (vCursor == termCursor)
        return false;

    // Move the cursor
    move_cursor(vCursor.x, vCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Moves the cursor to the rightmost
/// position in the whole input.
///
/// \return bool
///
/////////////////////////////////////////////////
bool GenericTerminal::back()
{
    LogicalCursor cursor = tm.toLogicalCursor(termCursor);

    // Search the last not editable character from the right
    while (tm.IsEditableLogical(cursor))
        cursor++;

    // Move the cursor to the last editable position
    cursor--;

    // Create a corresponding view cursor
    ViewCursor vCursor = tm.toViewCursor(cursor);

    // Ensure that the cursor is different
    if (vCursor == termCursor)
        return false;

    // Move the cursor
    move_cursor(vCursor.x, vCursor.y);
    return true;
}


/////////////////////////////////////////////////
/// \brief Erases the current line in the
/// internal buffer.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::erase_line()
{
    tm.eraseLine();
}


/////////////////////////////////////////////////
/// \brief Erases alle user-written contents from
/// the current line.
///
/// \return void
///
/////////////////////////////////////////////////
void GenericTerminal::erase_usercontent_line()
{
    // Go to the very last position first
    back();

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

    handle_calltip(termCursor.x, termCursor.y);
}




