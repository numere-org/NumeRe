
// Copyright Timothy Miller, 1999

#ifdef __GNUG__
    #pragma implementation "gterm.hpp"
#endif

#include "gterm.hpp"
#include "../common/debug.h"
#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/filename.h>

#ifdef _DEBUG

#define new DEBUG_NEW

#endif

void GTerm::Update()
{
	update_changes();
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ProcessInput
///  The guts of the GTerm state machine.  All the state functions are called from here.
///
///  @param  len  int             The number of characters to process.
///  @param  data unsigned char * The text to process.
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//void GTerm::ProcessInput(int len, unsigned const char *data)
void GTerm::ProcessInput(int len, const string& sData)
{
//printf("ProcessInput called...\n");
	int i;
	StateOption *last_state;

	data_len = sData.length();
	sInput_Data = sData;
	//sInput_Data = string((const char*)data);
	//input_data = data;
	//strcpy((char*)input_data,(const char*)data);
	mode_flags |= DESTRUCTBS;
	mode_flags |= INSERT;
	normal_input();
	update_changes();
	return;

	/*while (data_len) {
//printf("ProcessInput() processing %d...\n", *input_data);
		i = 0;
		while (current_state[i].byte != -1 &&
		       current_state[i].byte != *input_data) i++;

		// action must be allowed to redirect state change
		last_state = current_state+i;
		current_state = last_state->next_state;
		if (last_state->action)
			(this->*(last_state->action))();
		input_data++;
		data_len--;
	}

    normal_input();
	if (!(mode_flags & DEFERUPDATE) ||
	    (pending_scroll > scroll_bot-scroll_top)) update_changes();*/
}

void GTerm::Reset()
{
	reset();
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ExposeArea
///  Tells GTerm to redraw a given area.
///
///  @param  x    int  The starting x position, in characters
///  @param  y    int  The starting y position, in characters
///  @param  w    int  The width of the area, in characters
///  @param  h    int  The height of the area, in characters
///
///  @return void
///
///  @author Timothy Miller @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void GTerm::ExposeArea(int x, int y, int w, int h)
{
	int i;
	for (i=0; i<h; i++)
        changed_line(i+y, x, x+w-1);
	if (!(mode_flags & DEFERUPDATE))
        update_changes();
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
void GTerm::ResizeTerminal(int w, int h)
{
	int cx = 0, cy = 0;
	//clear_area(min(width,w), 0, MAXWIDTH-1, MAXHEIGHT-1);
	//clear_area(0, min(height,h), min(width,w)-1, MAXHEIGHT-1);


	//clear_area(0, )
	tm.Resize(w, h);
	if (h == height)
	{
        w = width;
        ExposeArea(0,0,width-1,height-1);
        return;
	}
	cy = -height + h + cursor_y; /// TODO: Fixme
	if (cy < 0)
        cy = 0;
	width = w;
	height = h;
	scroll_bot = height-1;
	if (scroll_top >= height) scroll_top = 0;
	cx = min(width-1, cursor_x);
	//cy = min(height-1, cursor_y);
	move_cursor(cx, cy);

	ExposeArea(0, 0, width - 1, height - 1);
	//Update();
}

GTerm::GTerm(int w, int h) : width(w), height(h)
{
	int i;
	m_nextLineCounter = 0;

	doing_update = 0;
/*
	string blankline;
	blankline.resize(MAXWIDTH, ' ');
	textq = deque<string>(MAXHEIGHT, blankline);

	alttext = new unsigned char[MAXWIDTH * MAXHEIGHT];

	stringtext.clear();

	stringtext.resize(MAXWIDTH * MAXHEIGHT, ' ');
*/
	TextManager temptm(this, w, h, MAXWIDTH, MAXHEIGHT);

	tm = temptm;
	wxFileName f(wxStandardPaths::Get().GetExecutablePath());
	_syntax.loadSyntax(f.GetPath(true).ToStdString());


	// could make this dynamic
	text = new unsigned char[MAXWIDTH*MAXHEIGHT];
	color = new unsigned short[MAXWIDTH*MAXHEIGHT];

	for (i=0; i<MAXHEIGHT; i++) {
		// make it draw whole terminal to start
		dirty_startx[i] = 0;
		dirty_endx[i] = MAXWIDTH-1;
	}

#ifdef GTERM_PC
        pc_machinename = new char[7];
        strcpy(pc_machinename, "pcterm");
#endif
	cursor_x = 0;
	cursor_y = 0;
	save_x = 0;
	save_y = 0;
        mode_flags = 0;
	reset();
	resetAutoComp();
}

GTerm::~GTerm()
{
	//delete[] alttext;
	delete[] text;
	delete[] color;
#ifdef GTERM_PC
        if(pc_machinename)
          delete[] pc_machinename;
#endif // GTERM_PC
}

#ifdef GTERM_PC
void
GTerm::SetMachineName(char *machinename)
{
  if(pc_machinename)
    delete pc_machinename;

  pc_machinename = new char[strlen(machinename) + 1];
  strcpy(pc_machinename, machinename);
}
#endif // GTERM_PC

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
GTerm::IsSelected(int x, int y)
{
  if(color && x >= 0 && x < Width() && y >= 0 && y < Height())
    //return color[(linenumbers[y] * MAXWIDTH) + x] & SELECTED;
    return (tm.GetColorAdjusted(y, x) & SELECTED);
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
GTerm::Select(int x, int y, int select)
{
	unsigned short firstColor = tm.GetColorAdjusted(0, 0);
  if(color && x >= 0 && x < Width() && y >= 0 && y < Height())
  //if( (firstColor != 0) && x >= 0 && x < Width() && y >= 0 && y < Height())
  {
	  int idx1 = linenumbers[y];
	  int idx2 = idx1 * MAXWIDTH;
	  int idx3 = idx2 + x;
    if(select)
	{
      //color[(linenumbers[y] * MAXWIDTH) + x] |= SELECTED;
	  color[idx3] |= SELECTED;
	  int tempcolor = tm.GetColorAdjusted(y, x);
	  tempcolor |= SELECTED;
	  tm.SetColorAdjusted(y, x, tempcolor);
	}
    else
	{
     // color[(linenumbers[y] * MAXWIDTH) + x] &= ~SELECTED;
	 color[idx3] &= ~SELECTED;
	 int tempcolor = tm.GetColorAdjusted(y, x);
	 tempcolor &= ~SELECTED;
	 tm.SetColorAdjusted(y, x, tempcolor);
	}
    changed_line(y, x, x);
//    update_changes();
  }
}

unsigned char
GTerm::GetChar(int x, int y)
{
  if(text && x >= 0 && x < Width() && y >= 0 && y < Height())
    //return text[(linenumbers[y] * MAXWIDTH) + x];
	return tm.GetCharAdjusted(y, x);

  return 0;
}

TextManager* GTerm::GetTM()
{
	return &tm;
}

int GTerm::GetColor()
{
	return calc_color(fg_color, bg_color, mode_flags);
}

void GTerm::DecodeColor(int color, int &fg_color, int &bg_color)
{
	fg_color = (color >> 4) & 0xf;
	bg_color = (color >> 8) & 0xf;
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
void GTerm::Scroll(int numLines, bool scrollUp)
{
	tm.Scroll(numLines, scrollUp);
	ExposeArea(0, 0, width, height);
	update_changes();
}

bool GTerm::IsScrolledUp()
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
int GTerm::GetScrollHeight()
{
	int scrollHeight = tm.GetLinesReceived();

	int maxSize = tm.GetMaxSize();
	if(scrollHeight > maxSize)
	{
		scrollHeight = maxSize;
	}
	else
	{
		scrollHeight--;
	}

	return scrollHeight;
}

int GTerm::GetScrollPosition()
{
	return tm.GetNumLinesScrolled();
}

void GTerm::SetTerminalHistory(int size)
{
	tm.SetMaxSize(size);
}
