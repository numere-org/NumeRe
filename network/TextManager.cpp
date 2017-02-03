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
	m_color = deque<vector<unsigned short> >(m_maxHeight, colorline);
	vector<short> usertext(m_maxWidth, 0);
	m_userText = deque<vector<short> >(m_maxHeight, usertext);

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
	vector<short> usertext;
	usertext.resize(m_maxWidth, 0);
	m_userText.push_back(usertext);

	m_linesReceived++;

	// make sure that we no more than m_maxHeight lines stored
	if((int)m_text.size() > m_maxHeight)
	{
		m_text.pop_front();
		m_color.pop_front();
		m_userText.pop_front();
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
bool TextManager::Scroll(int numLines, bool scrollUp)
{
	int actualLinesToScroll = numLines;

	if(scrollUp)
	{
		// skip out if we're scrolled all the way up
		if( ( (m_topLine - m_numLinesScrolledUp) == 0) ||
			( (m_numLinesScrolledUp + m_viewportHeight) >= (m_linesReceived -1)))
		{
			return false;
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
			return false;
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
	return true;
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
            m_userText[i].resize(width, 0);
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
	//int nsize = m_text.size();
	return m_text[actualLine];
}

string TextManager::GetInputHistory(bool vcursorup)
{
    int nActualLine = AdjustIndex(m_virtualCursor);
    if (vcursorup)
    {
        m_virtualCursor--;
        nActualLine--;
        while (nActualLine)
        {
            for (size_t i = 0; i < m_userText[nActualLine].size(); i++)
            {
                if (m_userText[nActualLine][i] == 1)
                {
                    for (size_t j = i; j < m_userText[nActualLine].size(); j++)
                    {
                        if (m_userText[nActualLine][j] == 0)
                        {
                            return m_text[nActualLine].substr(i, j-i-1);
                        }
                    }
                    return m_text[nActualLine].substr(i);
                }
            }
            m_virtualCursor--;
            nActualLine--;
        }
        return GetInputHistory(false);
    }
    else
    {
        if (m_virtualCursor == m_cursorLine)
            return "";
        m_virtualCursor++;
        nActualLine++;
        while (nActualLine < (int)m_userText.size() && m_virtualCursor < m_cursorLine)
        {
            for (size_t i = 0; i < m_userText[nActualLine].size(); i++)
            {
                if (m_userText[nActualLine][i] == 1)
                {
                    for (size_t j = i; j < m_userText[nActualLine].size(); j++)
                    {
                        if (m_userText[nActualLine][j] == 0)
                        {
                            return m_text[nActualLine].substr(i, j-i-1);
                        }
                    }
                    return m_text[nActualLine].substr(i);
                }
            }
            nActualLine++;
            m_virtualCursor++;
        }
        if (m_virtualCursor == m_cursorLine)
            return "";
    }
    return "";
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
void TextManager::SetCharAdjusted(int y, int x, char c, bool isUserInput)
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
	m_userText[actualLine][x] = 2*isUserInput;
}



string TextManager::GetWordAt(int y, int x)
{
    int actualLine = AdjustIndex(y);
    if (actualLine > m_maxHeight || y >= m_viewportHeight)
        return "";
    string sWord = m_text[actualLine];
    for (int pos = x; pos >= 0; pos--)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_')
            continue;
        sWord.erase(0,pos+1);
        break;
    }
    for (size_t pos = 0; pos < sWord.length(); pos++)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_')
            continue;
        return sWord.substr(0,pos);
    }
    return "";
}

string TextManager::GetWordStartAt(int y, int x)
{
    if (x < 2)
        return "";
    int actualLine = AdjustIndex(y);
    if (actualLine > m_maxHeight || y >= m_viewportHeight)
        return "";
    string sWord = m_text[actualLine].substr(0,x);
    for (int pos = x-1; pos >= 0; pos--)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_')
            continue;
        return sWord.substr(pos+1);
    }
    return "";
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

bool TextManager::IsUserText(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return false;
	}

	return m_userText[actualLine][x];
}

bool TextManager::IsEditable(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return false;
	}
    //int n = m_userText[actualLine][x];
	return (m_userText[actualLine][x] == 2);
}

void TextManager::SetEditable(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return;
	}

	m_userText[actualLine][x] = 2;
}

void TextManager::UnsetEditable(int y, int x)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return;
	}

	m_userText[actualLine][x] = 0;
}

void TextManager::RemoveEditableArea(int y, int x, size_t nLength)
{
	int actualLine = AdjustIndex(y);

	if( (actualLine > m_maxHeight) || (y >= m_viewportHeight))
	{
		wxLogDebug("Bad Y value in TextManager::GetCharAdjusted.  y = %d, viewport height = %d", y, m_viewportHeight);
		return;
	}
    auto iter_first = m_userText[actualLine].begin() + x;
    auto iter_last = m_userText[actualLine].begin() + x + nLength;
	m_userText[actualLine].erase(iter_first, iter_last);
	m_userText[actualLine].resize(m_userText[actualLine].size()+nLength, 0);
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

void TextManager::ChangeEditableState()
{
    for (size_t i = 0; i < m_userText.size(); i++)
    {
        for (size_t j = 0; j < m_userText[i].size(); j++)
            if (m_userText[i][j] == 2)
                m_userText[i][j] = 1;
    }
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
			m_userText.pop_front();
			m_text.pop_front();
		}
	}

	m_maxHeight = newSize;
	m_bottomLine = m_maxHeight - 1;
	m_topLine = m_bottomLine - m_viewportHeight + 1;
	if (m_linesReceived > m_maxHeight)
        m_linesReceived = m_maxHeight;
    if (m_numLinesScrolledUp > m_maxHeight)
        m_numLinesScrolledUp = m_maxHeight;
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
