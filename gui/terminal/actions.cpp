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
///  Handles most of the input stuff for GTerm
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GTerm::normal_input()
{
    int n, n_taken, i, c, y;
    bool bNextLine = false;
    string sRemainingInput = "";
    if (sInput_Data.find('\n') != string::npos)
    {
        sRemainingInput = sInput_Data.substr(sInput_Data.find('\n')+1);
        if (!sRemainingInput.length())
            bNextLine = true;
        sInput_Data.erase(sInput_Data.find('\n'));
    }
    //char test = 0;
#if 0

    char str[100];

#endif

    //if (* input_data<32)
    //n = abs((unsigned char)sInput_Data[0]);

    if (abs((unsigned char)sInput_Data[0])<32 && (unsigned char)sInput_Data[0] != '\r' && (unsigned char)sInput_Data[0] != '\t')
        return;

    if (cursor_x >= width)
    {
        if (mode_flags & NOEOLWRAP)
        {
            cursor_x = width - 1;
        }
        else
        {
            if (m_numCommandLines == -1)
                m_numCommandLines = 1;
            else if (m_numCommandLines != -1)
                m_numCommandLines++;
            next_line();
        }
    }

    n = 0;

    if (mode_flags & NOEOLWRAP)
    {
        //while (input_data[n]>31 && n<data_len)
        while ((abs((unsigned char)sInput_Data[n]) > 31 || sInput_Data[n] == '\r' || sInput_Data[n] == '\t') && n < sInput_Data.length())
            n++;

        n_taken = n;

        if (cursor_x + n>=width)
            n = width - cursor_x;
    }
    else
    {
        //while (input_data[n]>31 && n<data_len && cursor_x + n<width)
        while ((abs((unsigned char)sInput_Data[n]) > 31 || sInput_Data[n] == '\r' || sInput_Data[n] == '\t') && n < sInput_Data.length())//&& cursor_x + n<width)
            n++;

        n_taken = n;
    }

    if (mode_flags & INSERT)
    {
        changed_line(cursor_y, cursor_x, width - 1);
    }
    else
    {
        changed_line(cursor_y, cursor_x, cursor_x + n - 1);
    }

	// IMPORTANT Here's where the text pointer gets assigned.
    y = linenumbers[cursor_y] * MAXWIDTH;

	int altY = m_nextLineCounter * MAXWIDTH;

	// MPE: moves the text after the cursor to the right N spaces (inserting)
    if (mode_flags & INSERT)
    {
        for (i = width - 1; i>=cursor_x + n; i--)
        {
            //text[y + i] = text[y + i - n];

			char c = tm.GetCharAdjusted(cursor_y, i - n);
			tm.SetCharAdjusted(cursor_y, i, c, tm.IsEditable(cursor_y, i-n));

            //color[y + i] = color[y + i - n];

			unsigned short tempcolor = tm.GetColorAdjusted(cursor_y, i - n);
			tm.SetColorAdjusted(cursor_y, i, tempcolor);
        }
    }
    c = calc_color(fg_color, bg_color, mode_flags);

	// MPE: inserts the new received text, overwriting what was there already
    for (i = 0; i<n; i++)
    {
        //test = sInput_Data[i];
        //test;
        //text[y + cursor_x] = input_data[i];
		//tm.SetCharAdjusted(cursor_y, cursor_x, input_data[i]);
		if (sInput_Data[i] == '\r')
		{
            cr();
            continue;
		}
		if (sInput_Data[i] == '\t')
		{
            visualtab();
            continue;
		}
		tm.SetCharAdjusted(cursor_y, cursor_x, sInput_Data[i], true);

        //color[y + cursor_x] = c;
		tm.SetColorAdjusted(cursor_y, cursor_x, c);
        cursor_x++;
        if (i+1 == n)
        {
            string sLine = tm.GetLineAdjusted(cursor_y);

            string colors = _syntax.highlightLine(sLine);

            for (unsigned int j = 0; j < colors.length(); j++)
            {
                if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_COMMAND)
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD | UNDERLINE));
                else if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_FUNCTION
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_CONSTANT
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_SPECIALVAL
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_METHODS
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_PROCEDURE)
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD));
                else
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags));
            }

            changed_line(cursor_y, 0, cursor_x);
        }
        if (cursor_x >= width)
        {
            if (m_numCommandLines == -1 && n == 1)
                m_numCommandLines = 1;
            else if (m_numCommandLines != -1)
                m_numCommandLines++;
            next_line();
        }
    }

    if (sRemainingInput.length())
    {
        next_line();
        sInput_Data = sRemainingInput;
        normal_input();
    }
    else if (bNextLine)
        next_line();
    tm.SetEditable(cursor_y, cursor_x);

    //input_data += n_taken - 1;
    //data_len -= n_taken - 1;
}


void GTerm::normal_output()
{
    int n, n_taken, i, c, y;
    bool bNextLine = false;
    bool bErrorMessage = false;
    string sRemainingInput = "";
    if (sInput_Data.find('\n') != string::npos)
    {
        sRemainingInput = sInput_Data.substr(sInput_Data.find('\n')+1);
        if (!sRemainingInput.length())
            bNextLine = true;
        sInput_Data.erase(sInput_Data.find('\n'));
    }
    if (sInput_Data.find((char)15) != string::npos)
    {
        bErrorMessage = true;
        sInput_Data.erase(sInput_Data.find((char)15), 1);
    }
    //char test = 0;
#if 0

    char str[100];

#endif

    //if (* input_data<32)
    //n = abs((unsigned char)sInput_Data[0]);

    if (abs((unsigned char)sInput_Data[0]) < 32 && (unsigned char)sInput_Data[0] != '\r' && (unsigned char)sInput_Data[0] != '\t')
        return;

    if (cursor_x >= width)
    {
        if (mode_flags & NOEOLWRAP)
        {
            cursor_x = width - 1;
        }
        else
        {
            if (m_numCommandLines == -1)
                m_numCommandLines = 1;
            else if (m_numCommandLines != -1)
                m_numCommandLines++;
            next_line();
        }
    }

    n = 0;

    if (mode_flags & NOEOLWRAP)
    {
        //while (input_data[n]>31 && n<data_len)
        while ((abs((unsigned char)sInput_Data[n]) > 31 || sInput_Data[n] == '\r' || sInput_Data[n] == '\t') && n < sInput_Data.length())
            n++;

        n_taken = n;

        if (cursor_x + n>=width)
            n = width - cursor_x;
    }
    else
    {
        //while (input_data[n]>31 && n<data_len && cursor_x + n<width)
        while ((abs((unsigned char)sInput_Data[n]) > 31 || sInput_Data[n] == '\r' || sInput_Data[n] == '\t') && n < sInput_Data.length())//&& cursor_x + n<width)
            n++;

        n_taken = n;
    }

#if 0

    memcpy(str, input_data, n);
    str[n] = 0;
    //printf("Processing %d characters (%d): %s\n", n, str[0], str);

#endif

    if (mode_flags & INSERT)
    {
        changed_line(cursor_y, cursor_x, width - 1);
    }
    else
    {
        changed_line(cursor_y, cursor_x, cursor_x + n - 1);
    }

	// IMPORTANT Here's where the text pointer gets assigned.
    y = linenumbers[cursor_y] * MAXWIDTH;

	int altY = m_nextLineCounter * MAXWIDTH;

	// MPE: moves the text after the cursor to the right N spaces (inserting)
    if (mode_flags & INSERT)
    {
        for (i = width - 1; i>=cursor_x + n; i--)
        {
            //text[y + i] = text[y + i - n];

			char c = tm.GetCharAdjusted(cursor_y, i - n);
			tm.SetCharAdjusted(cursor_y, i, c);

            //color[y + i] = color[y + i - n];

			unsigned short tempcolor = tm.GetColorAdjusted(cursor_y, i - n);
			tm.SetColorAdjusted(cursor_y, i, tempcolor);
        }
    }
    c = calc_color(fg_color, bg_color, mode_flags);

	// MPE: inserts the new received text, overwriting what was there already
    for (i = 0; i < n; i++)
    {
        //test = sInput_Data[i];
        //test;
        //text[y + cursor_x] = input_data[i];
		//tm.SetCharAdjusted(cursor_y, cursor_x, input_data[i]);
		if (sInput_Data[i] == '\r')
		{
            cr();
            continue;
		}
		if (sInput_Data[i] == '\t')
		{
            visualtab();
            continue;
		}
		tm.SetCharAdjusted(cursor_y, cursor_x, sInput_Data[i]);

        //color[y + cursor_x] = c;
		tm.SetColorAdjusted(cursor_y, cursor_x, c);
        cursor_x++;
        if (i+1 == n)
        {
            string sLine = tm.GetLineAdjusted(cursor_y);

            string colors = _syntax.highlightLine(sLine);
            if (bErrorMessage)
                colors.replace(0, string::npos, sLine.length(), '0' + NumeReSyntax::SYNTAX_OPERATOR);

            for (unsigned int j = 0; j < colors.length(); j++)
            {
                if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_COMMAND)
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD | UNDERLINE));
                else if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_FUNCTION
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_CONSTANT
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_SPECIALVAL
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_METHODS
                    || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_PROCEDURE)
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD));
                else
                    tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags));
            }

            changed_line(cursor_y, 0, cursor_x);
        }
        if (cursor_x >= width)
        {
            if (m_numCommandLines == -1 && n == 1)
                m_numCommandLines = 1;
            else if (m_numCommandLines != -1)
                m_numCommandLines++;
            next_line();
        }
    }

    if (sRemainingInput.length())
    {
        next_line();
        sInput_Data = sRemainingInput;
        normal_output();
    }
    else if (bNextLine)
        next_line();
    tm.SetEditable(cursor_y, cursor_x);
    tm.ResetVirtualCursorLine();

    //input_data += n_taken - 1;
    //data_len -= n_taken - 1;
}

void GTerm::resetAutoComp()
{
    if (nTabStartPos == -1)
        return;
    nTabStartPos = -1;
    sAutoCompList.clear();
    sAutoCompWordStart.clear();
}

string GTerm::getProcNameSpace()
{
    string sNameSpace;
    int nNameSpacePos = nTabStartPos-1;
    while (nNameSpacePos && ((tm.GetColorAdjusted(cursor_y, nNameSpacePos-1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE && tm.GetCharAdjusted(cursor_y , nNameSpacePos-1) != '$')
        nNameSpacePos--;
    sNameSpace = tm.GetTextRange(cursor_y, nNameSpacePos, nTabStartPos);
    if (sNameSpace.find(sAutoCompWordStart) != string::npos)
        sNameSpace.erase(sNameSpace.find(sAutoCompWordStart));
    return sNameSpace;
}

void GTerm::cr()
{
    move_cursor(0, cursor_y);
}

void GTerm::lf()
{
    if (cursor_y<scroll_bot)
    {
        move_cursor(cursor_x, cursor_y + 1);
    }
    else
    {
        scroll_region(scroll_top, scroll_bot, 1);
        /*if (m_firstLineOfCommand != -1)
            m_firstLineOfCommand--;*/
    }

	tm.CursorDown();
}

void GTerm::ff()
{
    clear_area(0, scroll_top, width - 1, scroll_bot);

    move_cursor(0, scroll_top);
}

void GTerm::tab()
{
    if (nTabStartPos == -1)
    {
        nTabStartPos = cursor_x;
        sAutoCompWordStart = tm.GetWordStartAt(cursor_y, cursor_x);
        if (((tm.GetColorAdjusted(cursor_y, cursor_x-1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE)
        {
            string sNameSpace = getProcNameSpace();
            sAutoCompList = _syntax.getProcAutoCompList(sAutoCompWordStart, "", sNameSpace);
        }
        else
            sAutoCompList = _syntax.getAutoCompList(sAutoCompWordStart);

        if (!sAutoCompList.length() || !sAutoCompWordStart.length())
        {
            resetAutoComp();
            return;
        }
    }
    else
    {
        if (!sAutoCompList.length())
        {
            if (((tm.GetColorAdjusted(cursor_y, nTabStartPos-1) >> 4) & 0xf) == NumeReSyntax::SYNTAX_PROCEDURE)
            {
                string sNameSpace = getProcNameSpace();
                sAutoCompList = _syntax.getProcAutoCompList(sAutoCompWordStart, "", sNameSpace);
            }
            else
                sAutoCompList = _syntax.getAutoCompList(sAutoCompWordStart);
        }
        clear_area(nTabStartPos, cursor_y, cursor_x, cursor_y);
        move_cursor(nTabStartPos, cursor_y);
    }
    sInput_Data = sAutoCompList.substr(sAutoCompWordStart.length(), sAutoCompList.find('?')-sAutoCompWordStart.length());
    data_len = sInput_Data.length();
    sAutoCompList.erase(0, sAutoCompList.find(' ')+1);
    normal_input();
}

void GTerm::visualtab()
{
    int i, x = 0;

    for (i = cursor_x + 1; i < width && !x; i++)
    {
        if (tab_stops[i])
            x = i;
    }
    if (!x)
        x = (cursor_x + 8) & -8;

    if (x<width)
    {
        move_cursor(x, cursor_y);
    }
    else
    {
        if (mode_flags & NOEOLWRAP)
        {
            move_cursor(width - 1, cursor_y);
        }
        else
        {
            next_line();
        }
    }
}

bool GTerm::bs()
{
    if (mode_flags & DESTRUCTBS && cursor_x > 0 && tm.IsEditable(cursor_y, cursor_x-1))
    {
        clear_area(cursor_x-1, cursor_y, cursor_x, cursor_y);
        /*if (!tm.IsEditable(cursor_y, cursor_x+1))
            tm.UnsetEditable(cursor_y, cursor_x);*/
    }
    else
        return false;
    if (cursor_x>0 && tm.IsEditable(cursor_y, cursor_x-1))
        move_cursor(cursor_x - 1, cursor_y);
    string sLine = tm.GetLineAdjusted(cursor_y);

    string colors = _syntax.highlightLine(sLine);

    for (unsigned int j = 0; j < colors.length(); j++)
    {
        if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_COMMAND)
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD | UNDERLINE));
        else if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_FUNCTION
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_CONSTANT
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_SPECIALVAL
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_METHODS
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_PROCEDURE)
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD));
        else
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags));
    }

    changed_line(cursor_y, cursor_x-1, cursor_x+1);
    return true;
}

bool GTerm::del()
{
    if (mode_flags & DESTRUCTBS && cursor_x > 0 && tm.IsEditable(cursor_y, cursor_x) && tm.IsEditable(cursor_y, cursor_x+1))
    {
        clear_area(cursor_x, cursor_y, cursor_x+1, cursor_y);
        /*if (!tm.IsEditable(cursor_y, cursor_x+1))
            tm.UnsetEditable(cursor_y, cursor_x);*/
    }
    else
        return false;

    string sLine = tm.GetLineAdjusted(cursor_y);

    string colors = _syntax.highlightLine(sLine);

    for (unsigned int j = 0; j < colors.length(); j++)
    {
        if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_COMMAND)
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD | UNDERLINE));
        else if ((int)(colors[j]-'0') == NumeReSyntax::SYNTAX_FUNCTION
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_CONSTANT
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_SPECIALVAL
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_METHODS
            || (int)(colors[j]-'0') == NumeReSyntax::SYNTAX_PROCEDURE)
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags | BOLD));
        else
            tm.SetColorAdjusted(cursor_y, j, calc_color((int)(colors[j]-'0'), bg_color, mode_flags));
    }

    changed_line(cursor_y, cursor_x-1, cursor_x+1);
    return true;
}

void GTerm::bell() { Bell();
}

void GTerm::clear_param()
{
    nparam = 0;

    memset(param, 0, sizeof(param));
    q_mode = 0;
    got_param = 0;
}

void GTerm::keypad_numeric() { clear_mode_flag(KEYAPPMODE);
}

void GTerm::keypad_application() { set_mode_flag(KEYAPPMODE);
}

void GTerm::save_cursor()
{
    save_attrib = mode_flags;

    save_x = cursor_x;
    save_y = cursor_y;
}

void GTerm::restore_cursor()
{
    mode_flags = (mode_flags & ~15) | (save_attrib & 15);

    move_cursor(save_x, save_y);
}

void GTerm::set_tab() { tab_stops[cursor_x] = 1;
}

void GTerm::index_down() { lf();
}

void GTerm::next_line()
{
    lf();

    cr();
}

void GTerm::index_up()
{
    if (cursor_y>scroll_top)
    {
        move_cursor(cursor_x, cursor_y - 1);
    }
    else
    {
        scroll_region(scroll_top, scroll_bot, -1);

		tm.Scroll(1, true);
    }
}

void GTerm::reset()
{
    int i;

    pending_scroll = 0;
    bg_color = 0;
    fg_color = 7;
    scroll_top = 0;
    scroll_bot = height - 1;

    for (i = 0; i<MAXHEIGHT; i++)
        linenumbers[i] = i;

    memset(tab_stops, 0, sizeof(tab_stops));
    current_state = GTerm::normal_state;

    clear_mode_flag(NOEOLWRAP | CURSORAPPMODE | CURSORRELATIVE | NEWLINE | INSERT | UNDERLINE | BLINK | KEYAPPMODE
                              | CURSORINVISIBLE);

    clear_area(0, 0, width - 1, height - 1);
    move_cursor(0, 0);

	tm.Reset();
}

void GTerm::set_q_mode() { q_mode = 1;
}

// The verification test used some strange sequence which was
// ^[[61"p
// in a function called set_level,
// but it didn't explain the meaning.  Just in case I ever find out,
// and just so that it doesn't leave garbage on the screen, I accept
// the quote and mark a flag.
void GTerm::set_quote_mode() { quote_mode = 1;
}

// for performance, this grabs all digits
void GTerm::param_digit()
{
    got_param = 1;

    param[nparam] = param[nparam] * 10 + ( * input_data)-'0';
}

void GTerm::next_param() { nparam++;
}

bool GTerm::cursor_left()
{
    if (!cursor_x || !tm.IsEditable(cursor_y, cursor_x-1))
        return false;
    int n, x;

    n = param[0];

    if (n<1)
        n = 1;

    x = cursor_x - n;

    if (x<0)
        x = 0;

    move_cursor(x, cursor_y);
    return true;
}

bool GTerm::cursor_right()
{
    if (cursor_x+1 >= width || !tm.IsEditable(cursor_y, cursor_x+1))
        return false;
    int n, x;

    n = param[0];

    if (n<1)
        n = 1;

    x = cursor_x + n;

    if (x>=width)
        x = width - 1;

    move_cursor(x, cursor_y);
    return true;
}

bool GTerm::cursor_up()
{
    if (!tm.IsEditable(cursor_y-1, cursor_x))
    {
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

    int n, y;

    n = param[0];

    if (n<1)
        n = 1;

    y = cursor_y - n;

    if (y<0)
        y = 0;

    move_cursor(cursor_x, y);
    return true;
}

bool GTerm::cursor_down()
{
    if (!tm.IsEditable(cursor_y+1, cursor_x))
    {
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
    int n, y;

    n = param[0];

    if (n<1)
        n = 1;

    y = cursor_y + n;

    if (y>=height)
        y = height - 1;

    move_cursor(cursor_x, y);
    return true;
}

bool GTerm::home()
{
    int n = cursor_x;
    while (tm.IsEditable(cursor_y, n-1))
        n--;
    if (n == cursor_x)
        return false;
    move_cursor(n, cursor_y);
    return true;
}

bool GTerm::end()
{
    int n = cursor_x;
    while (tm.IsEditable(cursor_y, n+1))
        n++;
    if (n == cursor_x)
        return false;
    move_cursor(n, cursor_y);
    return true;
}

void GTerm::cursor_position()
{
    int x, y;

    x = param[1];

    if (x<1)
        x = 1;

    y = param[0];

    if (y<1)
        y = 1;

    if (mode_flags & CURSORRELATIVE)
    {
        move_cursor(x - 1, y - 1 + scroll_top);
    }
    else
    {
        move_cursor(x - 1, y - 1);
    }
}

void GTerm::device_attrib()
{
    char * str = "\033[?1;2c";

    //ProcessOutput(strlen(str), (unsigned char * )str);
}

void GTerm::delete_char()
{
    int n, mx;

    n = param[0];

    if (n<1)
        n = 1;

    mx = width - cursor_x;

    if (n>=mx)
    {
        clear_area(cursor_x, cursor_y, width - 1, cursor_y);
    }
    else
    {
        shift_text(cursor_y, cursor_x, width - 1, -n);
    }
}

void GTerm::set_mode() // h
{
    switch (param[0] + 1000 * q_mode)
    {
        case 1007:
            clear_mode_flag(NOEOLWRAP);
            break;

        case 1001:
            set_mode_flag(CURSORAPPMODE);
            break;

        case 1006:
            set_mode_flag(CURSORRELATIVE);
            break;

        case 4:
            set_mode_flag(INSERT);
            break;

        case 1003:
            RequestSizeChange(132, height);
            break;

        case 20:
            set_mode_flag(NEWLINE);
            break;

        case 12:
            clear_mode_flag(LOCALECHO);
            break;

        case 1025:
            clear_mode_flag(CURSORINVISIBLE);
            move_cursor(cursor_x, cursor_y);
            break;
    }
}

void GTerm::clear_mode() // l
{
    switch (param[0] + 1000 * q_mode)
    {
        case 1007:
            set_mode_flag(NOEOLWRAP);
            break;

        case 1001:
            clear_mode_flag(CURSORAPPMODE);
            break;

        case 1006:
            clear_mode_flag(CURSORRELATIVE);
            break;

        case 4:
            clear_mode_flag(INSERT);
            break;

        case 1003:
            RequestSizeChange(80, height);
            break;

        case 20:
            clear_mode_flag(NEWLINE);
            break;

        case 1002:
            current_state = vt52_normal_state;
            break;

        case 12:
            set_mode_flag(LOCALECHO);
            break;

        case 1025:
            set_mode_flag(CURSORINVISIBLE);
            break;
            move_cursor(cursor_x, cursor_y);
            break;
    }
}

void GTerm::request_param()
{
    char str[40];

    sprintf(str, "\033[%d;1;1;120;120;1;0x", param[0] + 2);
    //ProcessOutput(strlen(str), (unsigned char * )str);
}

void GTerm::set_margins()
{
    int t, b;

    t = param[0];

    if (t<1)
        t = 1;

    b = param[1];

    if (b<1)
        b = height;

    if (pending_scroll)
        update_changes();

    scroll_top = t - 1;
    scroll_bot = b - 1;

    if (cursor_y<scroll_top)
        move_cursor(cursor_x, scroll_top);

    if (cursor_y>scroll_bot)
        move_cursor(cursor_x, scroll_bot);
}

void GTerm::delete_line()
{
    int n, mx;

    n = param[0];

    if (n<1)
        n = 1;

    mx = scroll_bot - cursor_y + 1;

    if (n>=mx)
    {
        clear_area(0, cursor_y, width - 1, scroll_bot);
    }
    else
    {
        scroll_region(cursor_y, scroll_bot, n);
    }
}

void GTerm::status_report()
{
    char str[20];

    if (param[0] == 5)
    {
        char * str = "\033[0n";

        //ProcessOutput(strlen(str), (unsigned char * )str);
    }
    else if (param[0] == 6)
    {
        sprintf(str, "\033[%d;%dR", cursor_y + 1, cursor_x + 1);

        //ProcessOutput(strlen(str), (unsigned char * )str);
    }
}

void GTerm::erase_display()
{
    switch (param[0])
    {
        case 0:
            clear_area(cursor_x, cursor_y, width - 1, cursor_y);

            if (cursor_y<height - 1)
			{
                clear_area(0, cursor_y + 1, width - 1, height - 1);
			}
            break;

        case 1:
            clear_area(0, cursor_y, cursor_x, cursor_y);

            if (cursor_y>0)
			{
                clear_area(0, 0, width - 1, cursor_y - 1);
			}
            break;

        case 2:
            clear_area(0, 0, width - 1, height - 1);
            break;
    }
}

void GTerm::erase_line()
{
    clear_area(0, cursor_y, width-1, cursor_y);
    move_cursor(0,cursor_y);
    /*switch (param[0])
    {
        case 0:
            clear_area(cursor_x, cursor_y, width - 1, cursor_y);
            break;

        case 1:
            clear_area(0, cursor_y, cursor_x, cursor_y);
            break;

        case 2:
            clear_area(0, cursor_y, width - 1, cursor_y);
            break;
    }*/
}

void GTerm::erase_usercontent_line()
{
    if (!tm.IsEditable(cursor_y, cursor_x))
        return;

    int nFirstPos = -1;
    for (size_t i = 0; i < tm.GetLineAdjusted(cursor_y).length(); i++)
    {
        if (nFirstPos < 0 && tm.IsEditable(cursor_y, i))
            nFirstPos = i;
        if (nFirstPos >= 0 && !tm.IsEditable(cursor_y, i))
        {
            clear_area(nFirstPos, cursor_y, i, cursor_y);
            move_cursor(nFirstPos, cursor_y);
            return;
        }
    }
}

void GTerm::insert_line()
{
    int n, mx;

    n = param[0];

    if (n<1)
        n = 1;

    mx = scroll_bot - cursor_y + 1;

    if (n>=mx)
    {
        clear_area(0, cursor_y, width - 1, scroll_bot);
    }
    else
    {
        scroll_region(cursor_y, scroll_bot, -n);
    }
}

void GTerm::set_colors()
{
    int n;

    if (!nparam && param[0] == 0)
    {
        clear_mode_flag(15);

        fg_color = 7;
        bg_color = 0;
        return;
    }

    for (n = 0; n<=nparam; n++)
    {
        if (param[n] / 10 == 4)
        {
            bg_color = param[n] % 10;
        }
        else if (param[n] / 10 == 3)
        {
            fg_color = param[n] % 10;
        }
        else
            switch (param[n])
            {
                case 0:
                    clear_mode_flag(15);
                    fg_color = 7;
                    bg_color = 0;
                    break;

                case 1:
                    set_mode_flag(BOLD);
                    break;

                case 4:
                    set_mode_flag(UNDERLINE);
                    break;

                case 5:
                    set_mode_flag(BLINK);
                    break;

                case 7:
                    set_mode_flag(INVERSE);
                    break;
            }
    }
}

void GTerm::clear_tab()
{
    if (param[0] == 3)
    {
        memset(tab_stops, 0, sizeof(tab_stops));
    }
    else if (param[0] == 0)
    {
        tab_stops[cursor_x] = 0;
    }
}

void GTerm::insert_char()
{
    int n, mx;

    n = param[0];

    if (n<1)
        n = 1;

    mx = width - cursor_x;

    if (n>=mx)
    {
        clear_area(cursor_x, cursor_y, width - 1, cursor_y);
    }
    else
    {
        shift_text(cursor_y, cursor_x, width - 1, n);
    }
}

void GTerm::screen_align()
{
    int y, yp, x, c;

    c = calc_color(7, 0, 0);

    for (y = 0; y<height; y++)
    {
        yp = linenumbers[y] * MAXWIDTH;
		int altY = m_nextLineCounter * MAXWIDTH;

        changed_line(y, 0, width - 1);

        for (x = 0; x<width; x++)
        {
            //text[yp + x] = 'E';

			tm[y][x] = 'E';

            //color[yp + x] = c;

			tm.SetColorAdjusted(y, x, c);
        }
    }
}

void GTerm::erase_char()
{
    int n, mx;

    n = param[0];

    if (n<1)
        n = 1;

    mx = width - cursor_x;

    if (n>mx)
        n = mx;

    clear_area(cursor_x, cursor_y, cursor_x + n - 1, cursor_y);
	tm[cursor_y].replace(cursor_x, n - 1, n -1, ' ');
}

void GTerm::vt52_cursory()
{
    // store y coordinate
    param[0] = ( * input_data)-32;

    if (param[0]<0)
        param[0] = 0;

    if (param[0]>=height)
        param[0] = height - 1;
}

void GTerm::vt52_cursorx()
{
    int x;

    x = ( * input_data)-32;

    if (x<0)
        x = 0;

    if (x>=width)
        x = width - 1;

    move_cursor(x, param[0]);
}

void GTerm::vt52_ident()
{
    char * str = "\033/Z";

    //ProcessOutput(strlen(str), (unsigned char * )str);
}

#ifdef GTERM_PC

void GTerm::pc_begin( void )
{
    //printf("pc_begin...\n");
    set_mode_flag(PC);

    //printf("pc_begin: mode_flags = %x\n", mode_flags);
    //ProcessOutput((unsigned int)strlen(pc_machinename) + 1, (unsigned char * )pc_machinename);
    pc_oldWidth = Width();
    pc_oldHeight = Height();
    ResizeTerminal(80, 25);
    update_changes();
}

void GTerm::pc_end( void )
{
    //  printf("pc_end...\n");
    clear_mode_flag(PC);

    ResizeTerminal(pc_oldWidth, pc_oldHeight);
    update_changes();
}

void GTerm::pc_cmd( void )
{
    pc_curcmd = * input_data;

    //printf("pc_cmd: pc_curcmd = %d...\n", pc_curcmd);
    pc_argcount = 0;

    switch (pc_curcmd)
    {
        case GTERM_PC_CMD_CURONOFF: // <on/off>
            pc_numargs = 1;
            break;

        case GTERM_PC_CMD_MOVECURSOR: // <x> <y>
            pc_numargs = 2;
            break;

        case GTERM_PC_CMD_PUTTEXT: // <x> <y> <wid> <len>
            pc_numargs = 4;
            break;

        case GTERM_PC_CMD_WRITE: // <x> <y> <wid> <attr>
            pc_numargs = 4;
            break;

        case GTERM_PC_CMD_MOVETEXT: // <sx> <sy> <wid> <len> <dx> <dy>
            pc_numargs = 6;
            break;

        case GTERM_PC_CMD_BEEP:
            Bell();
            break;

        case GTERM_PC_CMD_SELECTPRINTER:
            pc_numargs = 1;
            break;

        case GTERM_PC_CMD_PRINTCHAR:
            pc_numargs = 1;
            break;

        case GTERM_PC_CMD_PRINTCHARS:
            pc_numargs = 2;
            break;

        default:
            current_state = pc_cmd_state;
            break;
    }
}

void GTerm::pc_arg( void )
{
    int i, yp, yp2;

    //printf("pc_arg: pc_curcmd = %d...\n", pc_curcmd);

    pc_args[pc_argcount++ ] = * input_data;

    if (pc_argcount == pc_numargs)
    {
        switch (pc_curcmd)
        {
            case GTERM_PC_CMD_CURONOFF:
                //printf("pc_arg: curonoff got %d\n", *input_data);
                if (* input_data)
                    clear_mode_flag(CURSORINVISIBLE);
                else
                    set_mode_flag(CURSORINVISIBLE);

                current_state = pc_cmd_state;
                changed_line(cursor_y, cursor_x, cursor_x);
                break;

            case GTERM_PC_CMD_MOVECURSOR:
                //printf("pc_arg: movecursor (%d, %d)\n", pc_args[0], pc_args[1]);
                move_cursor(pc_args[0], pc_args[1]);
                current_state = pc_cmd_state;
                break;

            case GTERM_PC_CMD_PUTTEXT:
                //printf("pc_arg: puttext got %d, %d, %d, %d\n", pc_args[0], pc_args[1], pc_args[2], pc_args[3]);
                pc_numdata = pc_args[2] * pc_args[3] * 2;
                pc_datacount = 0;
                pc_curx = pc_args[0];
                pc_cury = pc_args[1];

                if (pc_numdata)
                    current_state = pc_data_state;
                else
                    current_state = pc_cmd_state;

                break;

            case GTERM_PC_CMD_WRITE:
                //printf("pc_arg: write got %d, %d, %d, %d\n", pc_args[0], pc_args[1], pc_args[2], pc_args[3]);
                pc_numdata = pc_args[2];
                pc_datacount = 0;
                pc_curx = pc_args[0];
                pc_cury = pc_args[1];

                if (pc_numdata)
                    current_state = pc_data_state;
                else
                    current_state = pc_cmd_state;

                break;

            case GTERM_PC_CMD_MOVETEXT: // <sx> <sy> <wid> <len> <dx> <dy>
                if (pc_args[1]<pc_args[5])
                {
                    for (i = 0; i<pc_args[3]; i++)
                    {
						int idx1 = pc_args[1] + i;
						yp = linenumbers[idx1] * MAXWIDTH;

						int idx2 = pc_args[5] + i;
						yp2 = linenumbers[idx2] * MAXWIDTH;

                        memmove(& text[yp2 + pc_args[4]], & text[yp + pc_args[0]], pc_args[2]);

						string substring = tm[idx1].substr(pc_args[0], pc_args[2]);
						tm[idx2].replace(pc_args[4], pc_args[2], substring);

                        memmove(& color[yp2 + pc_args[4]], & color[yp + pc_args[0]], pc_args[2]);
						// TODO Doesn't seem to get here in normal use, but probably ought to
						//		eventually figure out what's going on so I can manipulate the text manager here
                        changed_line(pc_args[5] + i, pc_args[4], pc_args[4] + pc_args[2] - 1);
                    }
                }
                else
                {
                    for (i = pc_args[3] - 1; i>=0; i--)
                    {
						int idx1 = pc_args[1] + i;
						yp = linenumbers[idx1] * MAXWIDTH;

						int idx2 = pc_args[5] + i;
						yp2 = linenumbers[idx2] * MAXWIDTH;

                        memmove(& text[yp2 + pc_args[4]], & text[yp + pc_args[0]], pc_args[2]);

						string substring = tm[idx1].substr(pc_args[0], pc_args[2]);
						tm[idx2].replace(pc_args[4], pc_args[2], substring);

                        memmove(& color[yp2 + pc_args[4]], & color[yp + pc_args[0]], pc_args[2]);
						// TODO ditto as with previous
                        changed_line(pc_args[5] + i, pc_args[4], pc_args[4] + pc_args[2] - 1);
                    }
                }

                current_state = pc_cmd_state;
                break;

            case GTERM_PC_CMD_SELECTPRINTER:
                pc_numdata = pc_args[0];
                pc_datacount = 0;
                memset(pc_printername, 0, sizeof(pc_printername));

                if (pc_numdata)
                    current_state = pc_data_state;
                else
                {
                    SelectPrinter("");

                    current_state = pc_cmd_state;
                }

                break;

            case GTERM_PC_CMD_PRINTCHAR:
                PrintChars(1, & pc_args[0]);
                current_state = pc_cmd_state;
                break;

            case GTERM_PC_CMD_PRINTCHARS:
                pc_numdata = (pc_args[0] << 8) + pc_args[1];
                pc_datacount = 0;

                if (pc_numdata)
                    current_state = pc_data_state;
                else
                    current_state = pc_cmd_state;

                break;
        }
    }
}

void GTerm::pc_data( void )
{
    int yp;
	int altY;

    //printf("pc_data: pc_curcmd = %d, pc_datacount = %d, pc_numdata = %d, pc_curx = %d, pc_cur_y = %d...\n", pc_curcmd, pc_datacount, pc_numdata, pc_curx, pc_cury);
    switch (pc_curcmd)
    {
        case GTERM_PC_CMD_PUTTEXT:
            yp = linenumbers[pc_cury] * MAXWIDTH;

			altY = m_nextLineCounter * MAXWIDTH;

            if (!(pc_datacount & 1))
            {
                //printf("pc_data: got char %d\n", *input_data);
                //text[yp + pc_curx] = * input_data;

				tm[pc_cury][pc_curx] = *input_data;
            }
            else
            {
                //printf("pc_data: got attr %d\n", *input_data);
                //color[yp + pc_curx] = * input_data << 4;
				tm.SetColorAdjusted(pc_cury, pc_curx, (*input_data << 4));
            }

            if (pc_datacount & 1)
            {
                changed_line(pc_cury, pc_args[0], pc_curx);

                pc_curx++;

                if (pc_curx == pc_args[0] + pc_args[2])
                {
                    pc_curx = pc_args[0];

                    pc_cury++;
                }
            }

            break;

        case GTERM_PC_CMD_WRITE:
            yp = linenumbers[pc_cury] * MAXWIDTH;
			altY = m_nextLineCounter * MAXWIDTH;

            //text[yp + pc_curx] = * input_data;
			tm[pc_cury][pc_curx] = *input_data;

            //color[yp + pc_curx] = (unsigned short)pc_args[3] << 4;
			tm.SetColorAdjusted(pc_cury, pc_curx, ((unsigned short)pc_args[3] << 4) );
            changed_line(pc_cury, pc_args[0], pc_curx);
            pc_curx++;
            break;

        case GTERM_PC_CMD_SELECTPRINTER:
            if (pc_datacount<GTERM_PC_MAXPRINTERNAME - 1)
                pc_printername[pc_datacount] = * input_data;

            if (pc_datacount == pc_numdata - 1)
                SelectPrinter(pc_printername);

            break;

        case GTERM_PC_CMD_PRINTCHARS:
            PrintChars(1, input_data);
            break;
    }

    pc_datacount++;

    if (pc_datacount == pc_numdata)
        current_state = pc_cmd_state;
}

#endif // GTERM_PC
