
// Copyright Timothy Miller, 1999

#ifndef INCLUDED_GTERM_H
#define INCLUDED_GTERM_H

#ifdef __GNUG__
#pragma interface
#endif

#include <string.h>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <queue>
#include <deque>
#include <string>
//#include "../common/FixedSizeQueue.h"
#include "TextManager.h"

#include "../../kernel/syntax.hpp"

using namespace std;

#define MAXWIDTH 320
#define MAXHEIGHT 500

#define GTERM_PC


class GenericTerminal;
typedef void (GenericTerminal::*StateFunc)();


/////////////////////////////////////////////////
/// \brief An implementation of a generic
/// terminal, which has to be specialized in the
/// child classes.
/////////////////////////////////////////////////
class GenericTerminal
{
    public:
        // mode flags
        enum
        {
            BOLD=0x1,
            BLINK=0x2,
            UNDERLINE=0x4,
            INVERSE=0x8,
            NOEOLWRAP=0x10,
            CURSORAPPMODE=0x20,
            CURSORRELATIVE=0x40,
            NEWLINE=0x80,
            INSERT=0x100,
            KEYAPPMODE=0x200,
            DEFERUPDATE=0x400,
            DESTRUCTBS=0x800,
            TEXTONLY=0x1000,
            LOCALECHO=0x2000,
            CURSORINVISIBLE=0x4000,
#ifdef GTERM_PC
//          ,PC=0x10000
            PC=0x8000,
#endif // GTERM_PC
            SELECTED=0x8000	// flag to indicate a char is selected
        } MODES;

        enum
        {
            RESETTAB=0x1,
            RESETCURSOR=0x2
        };

    private:
        // terminal info
        int width, height, scroll_top, scroll_bot;

        // The text buffer and renderer
        TextManager tm;

        // terminal state
        ViewCursor termCursor;
        int fg_color, bg_color;
        int mode_flags;

        // utility functions
        void clear_area(int start_x, int start_y, int end_x, int end_y);
        int calc_color(int fg, int bg, int flags);
        void move_cursor(int x, int y);

        // action parameters
        int nparam, param[30];
        string sInput_Data;
        int data_len;
        NumeReSyntax _syntax;

        // terminal actions
    protected:
        int doing_update;
        // move cursor by clicking
        void move_cursor_editable_area(int x, int y);
        void update_changes();
        void normal_input();
        void normal_output();

        // autocomp params
        int nTabStartPos;
        int nCursorUpDownStartPos;
        string sAutoCompWordStart;
        string sAutoCompList;
        void resetAutoComp(int mode);
        string getProcNameSpace();

        // non-printing characters
        void cr(), lf(), ff(), bell(), tab();
        bool bs(), del();
        bool delSelected();

        // escape sequence actions
        void reset();

        bool cursor_left();
        bool cursor_down();
        bool cursor_right();
        bool cursor_up();
        bool ctrl_left();
        bool ctrl_right();
        bool home();
        bool end();
        bool front();
        bool back();

        void erase_line();
        void erase_usercontent_line();

    public:
        GenericTerminal(int w, int h);
        virtual ~GenericTerminal();

        NumeReSyntax* getSyntax(){return &_syntax;}

        // function to control terminal
        virtual void ProcessInput(int len, const string& sData);
        virtual void ProcessOutput(int len, const string& sData);
        virtual void ResizeTerminal(int _width, int _height);
        int Width() const
        {
            return width;
        }
        int Height() const
        {
            return height;
        }
        virtual void Update();
        virtual void Reset();

        bool Scroll(int numLines, bool scrollUp);
        int GetScrollHeight();
        int GetScrollPosition();
        TextManager* GetTM();

        void SetTerminalHistory(int size);

        bool IsScrolledUp();
        int GetMode() const
        {
            return mode_flags;
        }
        void SetMode(int mode)
        {
            mode_flags = mode;
        }
        void set_mode_flag(int flag);
        void clear_mode_flag(int flag);

        // manditory child-supplied functions
        virtual void DrawText(int fg_color, int bg_color, int flags,
                              int x, int y, const string& sText) = 0;
        virtual void DrawCursor(int fg_color, int bg_color, int flags,
                                int x, int y, unsigned char c) = 0;

        // optional child-supplied functions
        virtual void ClearChars(int bg_color, int x, int y, int w, int h) { }
        virtual void SendBack(int len, char *data) { }
        virtual void SendBack(char *data)
        {
            SendBack(strlen(data), data);
        }
        virtual void ModeChange(int state) { }
        virtual void Bell() { }

        virtual int IsSelected(int x, int y);
        virtual void Select(int x, int y, int select);
        virtual unsigned char GetChar(int x, int y);
        string get_selected_text();
};

#endif
