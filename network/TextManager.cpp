#include "TextManager.h"

#include <iostream>
#include <stdlib.h>
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include "gterm.hpp"
#include "../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

//////////////////////////////////////////////////////////////////////////////
///  public constructor TextManager
///  It's a constructor.  Yay.
///
///  @param  parent    GTerm * [=0] The GTerm that owns this textmanager
///  @param  width     int     [=80] The starting width in characters
///  @param  height    int     [=24] The starting height in characters
///  @param  maxWidth  int     [=160] The maximum possible characters in a line
///  @param  maxHeight int     [=100] The maximum lines of history to store
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
TextManager::TextManager(GTerm* parent, int width, int height, int maxWidth, int maxHeight /* = 50 */)
	:m_parent(parent), m_viewportWidth(width), m_viewportHeight(height), m_maxWidth(maxWidth), m_maxHeight(maxHeight)
{
	if(m_parent != NULL)
	{
		Reset();
	}

}

//////////////////////////////////////////////////////////////////////////////
///  public Reset
///  Clears out the text and resets everything
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::Reset()
{
	m_blankline.resize(m_maxWidth, ' ');
	m_text = deque<string>(m_maxHeight, m_blankline);

	// black background, white text
	m_blankColor = 112;
	vector<unsigned short> colorline(m_maxWidth, m_blankColor);
	m_color = deque< vector< unsigned short> >(m_maxHeight, colorline);

	m_bottomLine = m_maxHeight - 1;
	m_topLine = m_bottomLine - m_viewportHeight + 1;
	m_numLinesScrolledUp = 0;

	m_cursorLine = 0;
	m_linesReceived = 1;
}

//////////////////////////////////////////////////////////////////////////////
///  public overloaded AddNewLine
///  Inserts a new blank line at the bottom
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::AddNewLine()
{
	AddNewLine(m_blankline);
}

//////////////////////////////////////////////////////////////////////////////
///  public overloaded AddNewLine
///  Adds a new line to the bottom
///
///  @param  newline string  The new line to insert
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::AddNewLine(string newline)
{
	if(newline.size() != m_maxWidth)
	{
		newline.resize(m_maxWidth, ' ');
	}
	m_text.push_back(newline);

	vector<unsigned short> linecolors;
	linecolors.resize(m_maxWidth, m_blankColor);
	m_color.push_back(linecolors);

	m_linesReceived++;

	// make sure that we no more than m_maxHeight lines stored
	if((int)m_text.size() > m_maxHeight)
	{
		m_text.pop_front();
		m_color.pop_front();
	}
	else
	{
		// cursor tracking
		if(m_bottomLine < (m_maxHeight - 1))
		{
			m_bottomLine++;
			m_topLine = m_bottomLine - m_viewportHeight + 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public overloaded AddNewLine
///  Adds a new line to the bottom
///
///  @param  newline string  The new line to insert
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::AppendChar(char c)
{
    if (!m_text.size())
    {
        m_text.push_back(string("<- ") + c);
        return;
	}
	/*if (!m_text[m_text.size()-1].length())
        m_text[m_text.size()-1] += "<- ";
	m_text[m_text.size()-1] += c;*/
	m_text[m_maxHeight-1][0] = '<';
	m_text[m_maxHeight-1][1] = '-';
	m_text[m_maxHeight-1][2] = ' ';
	m_text[m_maxHeight-1][3] = c;
}

//////////////////////////////////////////////////////////////////////////////
///  public operator []
///  Allows access to a given line
///
///  @param  index    int  The index of the line to retrieve (0 to number of lines displayed)
///
///  @return string & The line at that index
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
string &TextManager::operator [](int index)
{
	return GetLineAdjusted(index);
}

//////////////////////////////////////////////////////////////////////////////
///  public Scroll
///  Scrolls the viewport the given number of lines up or down
///
///  @param  numLines int   The number of lines to scroll
///  @param  scrollUp bool  True to scroll up, false to scroll down
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::Scroll(int numLines, bool scrollUp)
{
	int actualLinesToScroll = numLines;

	if(scrollUp)
	{
		// skip out if we're scrolled all the way up
		if( ( (m_topLine - m_numLinesScrolledUp) == 0) ||
			( (m_numLinesScrolledUp + m_viewportHeight) >= (m_linesReceived -1)))
		{
			return;
		}
		if(m_topLine < actualLinesToScroll)
		{
			actualLinesToScroll = m_topLine;
		}

		int limiter = m_topLine - m_numLinesScrolledUp;
		if(actualLinesToScroll > limiter)
		{
			actualLinesToScroll = limiter;
		}

		m_numLinesScrolledUp += actualLinesToScroll;
	}
	else
	{
		if( (m_bottomLine - m_numLinesScrolledUp) == (m_text.size() - 1))
		{
			return;
		}

		int linesBelow = m_text.size() - (m_bottomLine - m_numLinesScrolledUp) - 1;
		if( linesBelow < actualLinesToScroll)
		{
			actualLinesToScroll = linesBelow;
		}
		m_numLinesScrolledUp -= actualLinesToScroll;

		if(m_numLinesScrolledUp < 0)
		{
			m_numLinesScrolledUp = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public GetSize
///  Returns the number of lines stored
///
///  @return int The number of lines stored
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int TextManager::GetSize()
{
	return m_text.size();
}

//////////////////////////////////////////////////////////////////////////////
///  public Resize
///  Resizes the stored text to a given number of characters wide and high
///
///  @param  width  int  The new number of characters displayed
///  @param  height int  The new number of lines displayed
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::Resize(int width, int height)
{
	if (height == m_viewportHeight && width <= m_maxWidth)
	{
		return;
	}
	if (width > m_maxWidth)
	{
        for (int i = 0; i < m_maxHeight; i++)
        {
            m_text[i].resize(width, ' ');
            m_color[i].resize(width, m_blankColor);
        }
        m_maxWidth = width;
	}

	if (m_viewportHeight == height)
        return;

	int oldCursorLocation = m_topLine + m_cursorLine;

	int oldViewportHeight = m_viewportHeight;
	m_viewportHeight = height;

	m_bottomLine = m_maxHeight - 1;
	m_topLine = m_bottomLine - m_viewportHeight + 1;

	//int newCursorLine;

	// TODO Need to do some more recalculations... this case here
	// is going to ripple off into stuff like SetCursorLine and so on
	// Hmm... maybe set m_topLine to the new cursor line and pop
	// off the bottom lines as necessary?
	if(m_topLine > oldCursorLocation)
	{
	}
    //m_cursorLine += m_viewportHeight-oldViewportHeight;
        //SetCursorLine(m_topLine+m_cursorLine);

	if(m_numLinesScrolledUp > 0)
	{
		int numLinesDifference = m_viewportHeight - oldViewportHeight;

		if(numLinesDifference > 0)
		{
			m_numLinesScrolledUp -= numLinesDifference;

			if(m_numLinesScrolledUp < 0)
			{
				m_numLinesScrolledUp = 0;
			}
		}
	}

}


//////////////////////////////////////////////////////////////////////////////
///  public GetLine
///  Retrieves a line without any adjustments for the viewport
///
///  @param  index    int  The absolute value of the line to retrieve (0 to (maxheight - 1))
///
///  @return string & The string at that location
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
string& TextManager::GetLine(int index)
{
	return m_text[index];
}

//////////////////////////////////////////////////////////////////////////////
///  public GetLineAdjusted
///  Returns a line from within the viewport
///
///  @param  index    int  The index of the line to retrieve (0 to (lines displayed - 1))
///
///  @return string & The string at that location
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
string& TextManager::GetLineAdjusted(int index)
{
	int actualLine = AdjustIndex(index);
	return m_text[actualLine];
}

//////////////////////////////////////////////////////////////////////////////
///  public SetLine
///  Sets a line without any adjustments for the viewport
///
///  @param  index int     The absolute index of the line to set
///  @param  line  string  The line to set
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetLine(int index, string line)
{
	m_text[index] = line;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetLineAdjusted
///  Sets a line within the viewport
///
///  @param  index int     The index within the viewport of the line to set
///  @param  line  string  The line to set
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetLineAdjusted(int index, string line)
{
	int actualLine = AdjustIndex(index);

	m_text[actualLine] = line;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetCharAdjusted
///  Sets a character, adjusted for the viewport
///
///  @param  y    int   The y position within the viewport
///  @param  x    int   The x position on that line
///  @param  c    char  The character to set
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetCharAdjusted(int y, int x, char c)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		// if we're here, then there's a good chance that the server thinks we're
		// doing 80x24, when we're actually something smaller.  At the moment, tough.
		wxLogDebug("Bad Y value in TextManagerM::SetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return;
	}
	m_text[actualLine][x] = c;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetCharAdjusted
///  Gets a character, adjusted for the viewport
///
///  @param  y    int  The line within the viewport that the character is on
///  @param  x    int  The character position within that line
///
///  @return char The character at that position
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
char TextManager::GetCharAdjusted(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return ' ';
	}

	return m_text[actualLine][x];
}

//////////////////////////////////////////////////////////////////////////////
///  public GetColor
///  Gets an encoded color without adjusting for the viewport
///
///  @param  y              int  The line number
///  @param  x              int  The character on that line
///
///  @return unsigned short The encoded color
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
unsigned short TextManager::GetColor(int y, int x)
{
	return m_color[y][x];
}

//////////////////////////////////////////////////////////////////////////////
///  public GetColorAdjusted
///  Gets an encoded color, adjusted for the viewport
///
///  @param  y              int  The line number in the viewport
///  @param  x              int  The character on that line
///
///  @return unsigned short The encoded color
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
unsigned short TextManager::GetColorAdjusted(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TM::GCA.  y = %d, viewport height = %d", y, m_viewportHeight);
		return 0;
	}
	//return 7;
	return m_color[actualLine][x];
}

//////////////////////////////////////////////////////////////////////////////
///  public SetColor
///  Sets an encoded color without adjusting for the viewport
///
///  @param  y     int             The unadjusted line
///  @param  x     int             The character position on that line
///  @param  value unsigned short  The new encoded color
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetColor(int y, int x, unsigned short value)
{
	m_color[y][x] = value;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetColorAdjusted
///  Sets an encoded color, adjusting the location for the viewport
///
///  @param  y     int             The line within the viewport
///  @param  x     int             The character on that line
///  @param  value unsigned short  The new encoded color
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetColorAdjusted(int y, int x, unsigned short value)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight) || x > m_color[actualLine].size())
	{
		wxLogDebug("Bad Y value in TM::SCA.  y = %d, viewport height = %d, x = %d", y, m_viewportHeight, x);
		return;
	}

	/*if(value != m_blankColor)
	{
		int q = 42;
	}*/
	m_color[actualLine][x] = value;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetCursorLine
///  Sets the line that the cursor should be on
///
///  @param  line int  The new line for the cursor
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetCursorLine(int line)
{
	if(line >= m_viewportHeight)
	{
		string bottomString = GetLineAdjusted(m_viewportHeight - 1);

		if(bottomString != m_blankline)
		{
			int numNewLines = line - m_viewportHeight + 1;
			for(int i = 0; i < numNewLines; i++)
			{
				AddNewLine();
			}
		}

		line = m_viewportHeight - 1;
	}
	else if(line < 0)
	{
		line = 0;
	}

	m_cursorLine = line;
}

//////////////////////////////////////////////////////////////////////////////
///  public CursorDown
///  Moves the cursor down one line
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::CursorDown()
{
	SetCursorLine(m_cursorLine + 1);

	m_linesReceived++;
}

//////////////////////////////////////////////////////////////////////////////
///  public CursorUp
///  Moves the cursor up a line
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::CursorUp()
{
	SetCursorLine(m_cursorLine - 1);
}

//////////////////////////////////////////////////////////////////////////////
///  private AdjustIndex
///  Adjusts an index within the viewport to the absolute index in the main arrays
///
///  @param  index int  The unadjusted index
///
///  @return int   The adjusted index
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int TextManager::AdjustIndex(int index)
{
	int adjustedIndex = m_topLine + index - m_numLinesScrolledUp;
	return adjustedIndex;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetNumLinesScrolled
///  Returns the number of lines scrolled upwards
///
///  @return int The number of lines scrolled up
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int TextManager::GetNumLinesScrolled()
{
	return m_numLinesScrolledUp;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetMaxSize
///  Sets the maximum number of lines of history
///
///  @param  newSize int  The new maximum size
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void TextManager::SetMaxSize(int newSize)
{
	if(newSize < m_viewportHeight)
	{
		return;
	}

	if(newSize < m_maxHeight)
	{
		int linesToPitch = m_maxHeight - newSize;

		for(int i = 0; i < linesToPitch; i++)
		{
			m_color.pop_front();
			m_text.pop_front();
		}
	}

	m_maxHeight = newSize;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetLinesReceived
///  Returns the number of lines received
///
///  @return int The number of lines received
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int TextManager::GetLinesReceived()
{
	return m_linesReceived;
}

int TextManager::GetMaxSize()
{
	return m_maxHeight;
}
