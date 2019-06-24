/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include "TextManager.h"

#include <iostream>
#include <stdlib.h>
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include "gterm.hpp"
#include "../../common/debug.h"

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
TextManager::TextManager(GenericTerminal* parent, int width, int height, int maxWidth, int maxHeight /* = 50 */)
	: m_parent(parent), m_viewportWidth(width), m_viewportHeight(height), m_maxWidth(maxWidth), m_maxHeight(maxHeight)
{
	if (m_parent != NULL)
	{
		Reset();
	}

}

// Destructor will reset the internal buffer
TextManager::~TextManager()
{
    Reset();
}

// Print some text to the current output line
// This is the read-only print function
void TextManager::printOutput(const string& _sLine)
{
    // Create a new line if the buffer is empty
	if (!m_managedText.size())
		newLine();

    // Copy the line and determine, whether the current line
    // contains the error signature (character #15)
    string sLine = _sLine;
    bool isErrorLine = sLine.find((char)15) != string::npos;

    // Remove the error signature
    while (sLine.find((char)15) != string::npos)
        sLine.erase(sLine.find((char)15), 1);

    // Append the line to the current line
	m_managedText.back() += CharacterVector(sLine, KERNEL_TEXT);

	// Update the colors and render the layout
	updateColors(isErrorLine);
	renderLayout();
}

// Insert user text to the current input line
// This is the user input function
void TextManager::insertInput(const string& sLine, size_t logicalpos /*= string::npos*/)
{
    // Ensure that the logical position is a valid position
	if (logicalpos == string::npos)
		logicalpos = m_managedText.back().length();

    // Insert the text at the desired position
	m_managedText.back().insert(logicalpos, sLine, EDITABLE_TEXT);

	// Update colors and render the layout
	updateColors();
	renderLayout();
}

// Convert the logical cursor to the view cursor in the current viewport setting
ViewCursor TextManager::toViewCursor(const LogicalCursor& logCursor)
{
    // Ensure that the cursor is valid
	if (!logCursor)
        return ViewCursor();

    // Find the logical cursor in the rendered layout
    // Start at the line of the logical cursor, because this is the first
    // possible line in the rendered layout
	for (size_t i = logCursor.line; i < m_renderedBlock.size(); i++)
	{
		if (!m_renderedBlock[i].coords.size())
			break;

        // Ensure that the first coordinate in the line is valid
		if (m_renderedBlock[i].coords[0] && m_renderedBlock[i].coords[0].line == logCursor.line)
		{
			for (size_t j = 0; j < m_renderedBlock[i].coords.size(); j++)
			{
				if (m_renderedBlock[i].coords[j].pos == logCursor.pos)
				{
					return ViewCursor(j, i - (m_topLine - m_numLinesScrolledUp));
				}
			}
		}

		// If it is not valid, it probably is a hanging indent
		if (!m_renderedBlock[i].coords[0] && m_renderedBlock[i].coords[m_indentDepth].line == logCursor.line)
        {
            for (size_t j = m_indentDepth; j < m_renderedBlock[i].coords.size(); j++)
			{
				if (m_renderedBlock[i].coords[j].pos == logCursor.pos)
				{
					return ViewCursor(j, i - (m_topLine - m_numLinesScrolledUp));
				}
			}

        }
	}

	// return an invalid cursor, if nothing was found
	return ViewCursor();
}

// This member function will return the view cursor coordinates at the current text
// input position
ViewCursor TextManager::getCurrentViewPos()
{
    // If the text was not rendered yet, return an invalid cursor
    if (!m_renderedBlock.size())
        return ViewCursor();

    // return the converted logical cursor
    return toViewCursor(getCurrentLogicalPos());
}

// Convert the view cursor to the logical cursor for the current viewport setting
LogicalCursor TextManager::toLogicalCursor(const ViewCursor& viewCursor)
{
    // Go to the corresponding x and y positions in the rendered layout
    // block and return the contained coordinates
	if (viewCursor && m_renderedBlock.size() > m_topLine - m_numLinesScrolledUp + viewCursor.y)
	{
	    // Ensure that the current line is long enough
		if (m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewCursor.y].coords.size() > viewCursor.x)
			return m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewCursor.y].coords[viewCursor.x];
	}

	// Return an invalid cursor, if the corresponding position may not be found
	return LogicalCursor();
}

// This member function will return the logical position of the current
// text input position
LogicalCursor TextManager::getCurrentLogicalPos()
{
    // Ensure that there's text in the buffer
    if (!m_managedText.size())
        return LogicalCursor();

    // Construct a valid cursor with the dimensions of the
    // text buffer
    return LogicalCursor(m_managedText.back().length(), m_managedText.size()-1);
}

// Return the rendered line for the current viewport setting
string TextManager::getRenderedString(size_t viewLine)
{
    // Return an empty line, if the line is not valid
	if (viewLine + m_topLine - m_numLinesScrolledUp >= m_renderedBlock.size() || viewLine > (size_t)m_viewportHeight)
		return "";

	return m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].sLine;
}

// Get the colors for the current viewport setting
vector<unsigned short> TextManager::getRenderedColors(size_t viewLine)
{
    // Return an empty vector, if the line is not valid
	if (viewLine + m_topLine - m_numLinesScrolledUp >= m_renderedBlock.size() || viewLine > (size_t)m_viewportHeight)
		return vector<unsigned short>();

    // Copy the current color line
    vector<unsigned short> colors = m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].colors;

    // Copy, whether there is a part of the current line is selected
    for (size_t i = 0; i < m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].coords.size(); i++)
    {
        if (m_managedText[m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].coords[i].line][m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].coords[i].pos].isSelected())
            colors[i] |= GenericTerminal::SELECTED;
    }

    // return the colors
	return colors;
}

// Calculate the colors
int TextManager::calc_color( int fg, int bg, int flags )
{
	return (flags & 15) | (fg << 4) | (bg << 8);
}

// Update the colors for the current line
void TextManager::updateColors(bool isErrorLine /*= false*/)
{
    // Get a pointer to the current syntax lexer
	NumeReSyntax* syntax = m_parent->getSyntax();
	string sColors;

	// If the current line shall be an error line, highlight it
	// correspondingly. Otherwise style it with the usual
	// lexer function
	if (isErrorLine)
        sColors = syntax->highlightError(m_managedText.back().toString());
    else
        sColors = syntax->highlightLine(m_managedText.back().toString());

    // Convert the string characters to the correct color codes
	for (size_t i = 0; i < sColors.length(); i++)
	{
		if ((int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_COMMAND)
			m_managedText.back()[i].setColor(calc_color((int)(sColors[i] - '0'), 0, GenericTerminal::BOLD | GenericTerminal::UNDERLINE));
		else if ((int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_FUNCTION
				 || (int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_CONSTANT
				 || (int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_SPECIALVAL
				 || (int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_METHODS
				 || (int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_PROCEDURE)
			m_managedText.back()[i].setColor(calc_color((int)(sColors[i] - '0'), 0, GenericTerminal::BOLD));
		else
			m_managedText.back()[i].setColor(calc_color((int)(sColors[i] - '0'), 0, 0));
	}
}

// Render the stored text lines into a terminal layout
void TextManager::renderLayout()
{
	if (m_renderedBlock.size())
		m_renderedBlock.clear();

	// Go through the complete container
	for (size_t i = 0; i < m_managedText.size(); i++)
	{
		size_t breakpos;
		size_t lastbreakpos = 0;
		bool firstline = true;
		LogicalCursor cursor(0, i);

		// Break the text lines at reasonable locations
		do
		{
			RenderedLine line;

			// If the last break position is non-zero, this is not a first line
			if (lastbreakpos)
                firstline = false;

            // Get the new break position
			breakpos = findNextLinebreak(m_managedText[i].toString(), lastbreakpos);

			// If it's not the first line, add the indent here
			if (!firstline)
            {
                line.sLine = "|   ";
                line.colors.assign(m_indentDepth, calc_color(7, 0, 0));
                line.coords.assign(m_indentDepth, LogicalCursor());
            }

			// Assign text and colors
			line.sLine += m_managedText[i].substr(lastbreakpos, breakpos - lastbreakpos);
			vector<unsigned short> colors = m_managedText[i].subcolors(lastbreakpos, breakpos - lastbreakpos);
			line.colors.insert(line.colors.end(), colors.begin(), colors.end());

			// The last line has an additional white space character,
			// which is used as input location (to ensure that the cursor is valid)
			if (i + 1 == m_managedText.size())
			{
				line.sLine += " ";
				line.colors.push_back(calc_color(7, 0, 0));
			}

			// Assign the logical cursors
			for (size_t j = 0; j < line.sLine.length() - m_indentDepth*(!firstline); j++)
			{
				cursor.pos = j + lastbreakpos;
				line.coords.push_back(cursor);
			}

			lastbreakpos = breakpos;

			// Store the rendered line
			m_renderedBlock.push_back(line);
		}
		while (lastbreakpos < m_managedText[i].length());
	}

	// Calculate new topline, because it depends on the size of the rendered block
	m_topLine = m_renderedBlock.size() - m_viewportHeight;
	if (m_topLine < 0)
		m_topLine = 0;
}

// Perform actual linebreaks
size_t TextManager::findNextLinebreak(const string& currentLine, size_t currentLinebreak)
{
    // If the current line is shorter than the current viewport width,
    // return the length of the current line as next break position
	if (m_viewportWidth + currentLinebreak - m_indentDepth * (bool)currentLinebreak > currentLine.length())
		return currentLine.length();

    // Store the valid line break characters
    static string sValidLinebreaks = "+- ,;.*/<>=!";

    // Find the next possible break position
    for (int i = currentLinebreak + m_viewportWidth - 1 - m_indentDepth * (bool)currentLinebreak; i >= (int)currentLinebreak; i--)
    {
        // If the current character marks a valid line break position,
        // return the character after it
        if (sValidLinebreaks.find(currentLine[i]) != string::npos)
            return i+1;
    }

    // The fall back solution, if no valid position is found
	return currentLinebreak + m_viewportWidth - m_indentDepth * (bool)currentLinebreak;
}

// Perform a tab character
size_t TextManager::tab()
{
    // If the buffer is empty add an empty line first
    if (!m_managedText.size())
        newLine();

    // Get the length of the current input line
    size_t currentPos = m_managedText.back().length();

    // Determine the length of the current tab, which is
    // between 1 and m_tabLength
    size_t tabLength = m_tabLength - (currentPos % m_tabLength);

    // Append the length of the tab as
    // whitespaces to the text buffer
    m_managedText.back().append(tabLength, ' ');

    // Return the calculated tab length
    return tabLength;
}

// Adds a new line
void TextManager::newLine()
{
    // Add a new line to all buffers and fill it with a reasonable value
	m_managedText.push_back(CharacterVector("", EDITABLE_TEXT));
	//m_color.push_back(vector<unsigned short>(1, calc_color(7, 0, 0)));

	// Ensure that the buffer is not larger than the desired history length
	while (m_managedText.size() > (size_t)m_maxHeight)
	{
		m_managedText.pop_front();
	}

	// render layout, get new top line and reset the virtual cursor line
	renderLayout();
	ResetVirtualCursorLine();
}

// This member function performs a backspace operation
void TextManager::backspace(const LogicalCursor& logCursor)
{
    // Ensure that the cursor is valid
    if (!logCursor)
        return;

    // Apply a backspace
	if (logCursor.pos)
	{
	    // In the current line
		//m_text[logCursor.line].erase(logCursor.pos - 1, 1);
		m_managedText[logCursor.line].erase(m_managedText[logCursor.line].begin() + logCursor.pos - 1);
		//m_color[logCursor.line].erase(m_color[logCursor.line].begin() + logCursor.pos - 1);
	}
	else
	{
	    // go to the previous line
		//m_text[logCursor.line - 1].pop_back();
		m_managedText[logCursor.line - 1].pop_back();
		//m_color[logCursor.line - 1].pop_back();
	}

	// Update the colors and render the layout
	updateColors();
	renderLayout();
}

// This member function completely erases the current line
void TextManager::eraseLine()
{
    // Do nothing if the buffer is empty
    if (!m_managedText.size())
        return;

    // Clear the lines and fill them with a
    // reasonable default content (just like in
    // the new line function)
    m_managedText.back().clear();
    /*m_text.back() += "";

    m_color.back().clear();
    m_color.back().push_back(calc_color(7, 0, 0));

    m_userText.back().clear();
    m_userText.back().push_back(EDITABLE_TEXT);*/

    // Render the layout
    renderLayout();
}

// This member function is used to clear the range between the two view cursors.
// It may even clear the characters, which are spread across multiple lines
bool TextManager::clearRange(const ViewCursor& cursor1, const ViewCursor& cursor2)
{
    // Transform the view cursors to logical cursors
    LogicalCursor logCursor1 = toLogicalCursor(cursor1);
    LogicalCursor logCursor2 = toLogicalCursor(cursor2);

    // Ensure that the cursors are both valid
    if (!logCursor1 || !logCursor2)
        return false;

    // Ensure that their order is correct
    if (logCursor1 > logCursor2)
    {
        LogicalCursor temp = logCursor1;
        logCursor1 = logCursor2;
        logCursor2 = temp;
    }

    // As long as the second cursor is at a larger position than the first character
    while (logCursor2 > logCursor1)
    {
        if (logCursor1.line != logCursor2.line)
        {
            // The cursors are in different lines
            // Erase all from the first to the cursor's position
            //m_text[logCursor2.line].erase(0, logCursor2.pos);
            m_managedText[logCursor2.line].erase(m_managedText[logCursor2].begin(), m_managedText[logCursor2].begin()+logCursor2.pos);
            //m_color[logCursor2.line].erase(m_color[logCursor2].begin(), m_color[logCursor2].begin()+logCursor2.pos);

            // If the text buffer of the current line is empty,
            // erase the overall line from all three buffers
            if (!m_managedText[logCursor2.line].length())
            {
                //m_text.erase(m_text.begin()+logCursor2.line);
                m_managedText.erase(m_managedText.begin()+logCursor2.line);
                //m_color.erase(m_color.begin()+logCursor2.line);
            }

            // set the cursor to the last position of the previous line
            logCursor2.line--;
            logCursor2.pos = m_managedText[logCursor2.line].length();
        }
        else
        {
            // The cursors are in the same line
            //m_text[logCursor2.line].erase(logCursor1.pos, logCursor2.pos - logCursor1.pos);
            m_managedText[logCursor2.line].erase(m_managedText[logCursor2.line].begin() + logCursor1.pos, m_managedText[logCursor2.line].begin() + logCursor2.pos);
            //m_color[logCursor2.line].erase(m_color[logCursor2.line].begin() + logCursor1.pos, m_color[logCursor2.line].begin() + logCursor2.pos);
            break;
        }
    }

    // Update colors and render the layout
    updateColors();
    renderLayout();

    return true;
}

// This member function is used to (de-)select the character at the position of
// the passed view cursor
void TextManager::selectText(const ViewCursor& viewCursor, bool bSelect /*= true*/)
{
    // Convert the view cursor to a logical cursor
    LogicalCursor cursor = toLogicalCursor(viewCursor);

    // Ensure that the cursor is valid
    if (!cursor)
        return;

    // Select or deselect the pointer character
    if (bSelect)
        m_managedText[cursor.line][cursor.pos].select();
    else
        m_managedText[cursor.line][cursor.pos].unselect();
}

// This member function unselects the whole text at once
void TextManager::unselectAll()
{
    for (size_t i = 0; i < m_managedText.size(); i++)
    {
        for (size_t j = 0; j < m_managedText[i].length(); j++)
        {
            m_managedText[i][j].unselect();
        }
    }
}

// This member function returns true if the character at the position of
// the passed view cursor is selected
bool TextManager::isSelected(const ViewCursor& viewCursor)
{
    // Convert the view cursor into a logical cursor
    LogicalCursor cursor = toLogicalCursor(viewCursor);

    // Ensure that the cursor is valid
    if (!cursor)
        return false;

    // Return true, if the color line at the current contains the SELECTED flag
    return m_managedText[cursor.line][cursor.pos].isSelected();
}

// This member function returns the selected text
string TextManager::getSelectedText()
{
    string sText;

    // Find every selected character in the text buffer
    for (size_t i = 0; i < m_managedText.size(); i++)
    {
        for (size_t j = 0; j < m_managedText[i].size(); j++)
        {
            if (m_managedText[i][j].isSelected())
            {
                sText += m_managedText[i][j].m_char;
            }
        }

        // Append a new line character after each new line
        if (sText.length() && sText.back() != '\n')
            sText += '\n';
    }

    // Remove all trailing new line characters
    while (sText.back() == '\n')
        sText.pop_back();

    // return the selected text
    return sText;
}

// This member function will return the contents of the current user
// input line
string TextManager::getCurrentInputLine()
{
    // Ensure that the buffer is available and that there's user text
    if (!m_managedText.size() || !m_managedText.back().back().editable())
        return "";

    string sInput;

    // Find the beginning of the user text
    for (int i = m_managedText.back().size()-1; i >= 0; i--)
    {
        if (!m_managedText.back()[i].editable())
        {
            sInput = m_managedText.back().substr(i+1);
            break;
        }
    }

    return sInput;
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
	m_managedText.clear();
	//m_color.clear();
	//m_userText.clear();
	m_renderedBlock.clear();


	m_bottomLine = m_maxHeight - 1;
	m_topLine = m_bottomLine - m_viewportHeight + 1;
	m_numLinesScrolledUp = 0;

	m_linesReceived = 1;

	m_tabLength = 8;
	m_indentDepth = 4;
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
string TextManager::operator [](int index)
{
	return getRenderedString(index);
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

	if (scrollUp)
	{
		// skip out if we're scrolled all the way up
		if (m_topLine - m_numLinesScrolledUp == 0)
		{
			return false;
		}

		// If one wants to scrol more lines than available,
		// then restrict them
		if (m_topLine < actualLinesToScroll)
		{
			actualLinesToScroll = m_topLine;
		}

		// This is the maximal possible distance
		int limiter = m_topLine - m_numLinesScrolledUp;
		if (actualLinesToScroll > limiter)
		{
			actualLinesToScroll = limiter;
		}

		// scroll up
		m_numLinesScrolledUp += actualLinesToScroll;
	}
	else
	{
	    // Ignore, if we're completely scrolled down
		if (m_numLinesScrolledUp <= 0)
		{
			return false;
		}

		// These are the possible lines to scroll down
		int linesBelow = m_renderedBlock.size() - (m_topLine + m_viewportHeight - m_numLinesScrolledUp);

		// Limit the lines to scroll down
		if ( linesBelow < actualLinesToScroll)
		{
			actualLinesToScroll = linesBelow;
		}

		// scroll down
		m_numLinesScrolledUp -= actualLinesToScroll;

		// Ensure that we're not negatively scrolled
		if (m_numLinesScrolledUp < 0)
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
	return m_managedText.size();
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
    // Get the current top line
    // We've to explicitly create a ViewCursor with (0,0),
    // because otherwise the cursor won't be valid
    LogicalCursor cursor = toLogicalCursor(ViewCursor(0, 0));

    // Store the new dimensions
	m_viewportHeight = height;
	m_viewportWidth = width;

	// Render the new layout
	renderLayout();

	// If the terminal is scrolled up, get the corresponding top line
	// and use it to determine the new number of scrolled lines
	int nNewTopLine = 0;
	if (m_numLinesScrolledUp > 0)
	{
        for (size_t i = 0; i < m_renderedBlock.size(); i++)
        {
            if (m_renderedBlock[i].coords.size() && m_renderedBlock[i].coords[0].line == cursor.line)
            {
                for (size_t j = 0; j < m_renderedBlock[i].coords.size(); j++)
                {
                    if (m_renderedBlock[i].coords[j].pos == cursor.pos)
                    {
                        // If we found the current position, we break the loop
                        nNewTopLine = i;
                        break;
                    }
                }
            }
            // Break if the variable is non-zero
            if (nNewTopLine)
                break;
        }
	}

	// If a top line was found, calculate the new number
	// of scrolled lines
    if (nNewTopLine)
    {
        m_numLinesScrolledUp = nNewTopLine - m_topLine;
        if (m_numLinesScrolledUp < 0)
            m_numLinesScrolledUp = 0;
    }
}

// This member function gets the next history line
// depending on the bool vcursorup
string TextManager::GetInputHistory(bool vcursorup)
{
	if (vcursorup)
	{
	    // scroll up
	    // decrement the virtual cursor
		m_virtualCursor--;

		if (m_virtualCursor >= (int)m_managedText.size())
            m_virtualCursor = m_managedText.size() - 1;

		// While the virtual cursor is non-zero
		while (m_virtualCursor)
		{
		    // find the next user text and return it
			for (size_t i = 0; i < m_managedText[m_virtualCursor].size(); i++)
			{
				if (m_managedText[m_virtualCursor][i].userText())
				{
					for (size_t j = i; j < m_managedText[m_virtualCursor].size(); j++)
					{
						if (!m_managedText[m_virtualCursor][j].userText())
						{
							return m_managedText[m_virtualCursor].substr(i, j - i - 1);
						}
					}
					return m_managedText[m_virtualCursor].substr(i);
				}
			}

			// decrement the virtual cursor
			m_virtualCursor--;
		}

		// Fallback
		return GetInputHistory(false);
	}
	else
	{
	    // Return an empty string, if the virtual cursor is the lowest possible line
		if (m_virtualCursor + 1 >= (int)m_managedText.size())
			return "";

        // increment the virtual cursor
		m_virtualCursor++;

		// While the virtual cursor is smaller than the number of lines in the text buffer
		while (m_virtualCursor < (int)m_managedText.size())
		{
		    // find the next user text and return it
			for (size_t i = 0; i < m_managedText[m_virtualCursor].size(); i++)
			{
				if (m_managedText[m_virtualCursor][i].userText())
				{
					for (size_t j = i; j < m_managedText[m_virtualCursor].size(); j++)
					{
						if (!m_managedText[m_virtualCursor][j].userText())
						{
							return m_managedText[m_virtualCursor].substr(i, j - i - 1);
						}
					}
					return m_managedText[m_virtualCursor].substr(i);
				}
			}

			// Increment the virtual cursor
			m_virtualCursor++;
		}

		// Return an empty string, if the virtual cursor is the lowest possible line
		if (m_virtualCursor + 1 >= (int)m_managedText.size())
			return "";
	}

	// Fallback
	return "";
}

// This member function extracts the text at the position (x0, y) until (x1, y)
string TextManager::GetTextRange(int y, int x0, int x1)
{
    // Convert the coordinates to a logical cursor
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x0, y));

	// ensure that the cursor is valid
	if (!cursor)
        return "";

    // Return the extracted string
	return m_managedText[cursor.line].substr(cursor.pos, x1 - x0);
}

// This member function returns the word, which
// contains the character at (x,y)
string TextManager::GetWordAt(int y, int x)
{
    // Convert the coordinates to a logical cursor
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

	// ensure that the cursor is valid
	if (!cursor)
        return "";

    // Get the text at the corresponnding line
	string sWord = m_managedText[cursor.line].toString();

	// Find the start of the word and erase everything in front of it
	for (int pos = cursor.pos; pos >= 0; pos--)
	{
		if (isalnum(sWord[pos]) || sWord[pos] == '_')
			continue;
		sWord.erase(0, pos + 1);
		break;
	}

	// Find the end of the word and return it as a new string
	for (size_t pos = 0; pos < sWord.length(); pos++)
	{
		if (isalnum(sWord[pos]) || sWord[pos] == '_')
			continue;
		return sWord.substr(0, pos);
	}

	// Fallback
	return "";
}

// This member function returns the word start, which
// contains the character at (x,y)
string TextManager::GetWordStartAt(int y, int x)
{
    // Convert the coordinates to a logical cursor
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

	// Ensure that the cursor is valid
	if (!cursor)
        return "";

    // Get the line until the position
	string sWord = m_managedText[cursor.line].substr(0, cursor.pos);

	// Find the start of the word and return it as a new string
	for (int pos = cursor.pos - 1; pos >= 0; pos--)
	{
		if (isalnum(sWord[pos]) || sWord[pos] == '_')
			continue;
		return sWord.substr(pos + 1);
	}

	// Fallback
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
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));
	return GetCharLogical(cursor);
}

char TextManager::GetCharLogical(const LogicalCursor& cursor)
{
    if (!cursor)
        return ' ';
	if (cursor.pos >= m_managedText[cursor.line].size())
		return ' '; // default color
	return m_managedText[cursor.line][cursor.pos].m_char;
}

// This member function evaluates, whether the character at (x,y)
// belongs to a user text
bool TextManager::IsUserText(int y, int x)
{
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

	if (!cursor)
        return false;

	if (cursor.pos == m_managedText[cursor.line].size())
		return true;
	if (cursor.pos > m_managedText[cursor.line].size())
		return false;
	return m_managedText[cursor.line][cursor.pos].userText();
}

// This member function evaluates, whether the character at (x,y)
// is an editable text
bool TextManager::IsEditable(int y, int x)
{
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

	if (!cursor)
        return false;

	if (cursor.pos == m_managedText[cursor.line].size())
		return true;
	if (cursor.pos > m_managedText[cursor.line].size())
		return false;
	return m_managedText[cursor.line][cursor.pos].editable();
}

// This member function evaluates, whether the character at the
// logical cursor's position is an editable text
bool TextManager::IsEditableLogical(LogicalCursor& logCursor)
{
    if (!logCursor)
        return false;

	if (logCursor.pos == m_managedText[logCursor.line].size())
		return true;
	if (logCursor.pos > m_managedText[logCursor.line].size())
		return false;
	return m_managedText[logCursor.line][logCursor.pos].editable();
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
	return m_managedText[y][x].getColor();
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
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

	if (!cursor)
        return 112;

	if (cursor.pos >= m_managedText[cursor.line].size())
		return 112; // default color
	return m_managedText[cursor.line][cursor.pos].getColor();
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
	LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

    if (!cursor)
        return;

	if (cursor.pos >= m_managedText[cursor.line].size())
		return;
	m_managedText[cursor.line][cursor.pos].setColor(value);
}

// This member function will switch every editable text to a
// simple user text
void TextManager::ChangeEditableState()
{
	for (size_t i = 0; i < m_managedText.size(); i++)
	{
		for (size_t j = 0; j < m_managedText[i].size(); j++)
			if (m_managedText[i][j].editable())
				m_managedText[i][j].makeUserText();
	}
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
	if (newSize < m_viewportHeight || newSize == m_maxHeight)
	{
		return;
	}

	if (m_managedText.size())
    {
        if (newSize < m_maxHeight)
        {
            int linesToPitch = m_maxHeight - newSize;

            for (int i = 0; i < linesToPitch; i++)
            {
                if (!m_managedText.size())
                    break;

                m_managedText.pop_front();
            }
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
	return m_renderedBlock.size();
}

int TextManager::GetMaxSize()
{
	return m_maxHeight;
}
