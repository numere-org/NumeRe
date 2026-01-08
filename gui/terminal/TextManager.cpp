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
    : m_parent(parent), m_viewportWidth(width), m_viewportHeight(height), m_maxWidth(maxWidth), m_maxHeight(maxHeight), m_virtualCursor(0)
{
    if (m_parent != nullptr)
        Reset();
}


/////////////////////////////////////////////////
/// \brief Destructor will reset the internal buffer
/////////////////////////////////////////////////
TextManager::~TextManager()
{
    Reset();
}


/////////////////////////////////////////////////
/// \brief This is the read-only print function
///
/// \param sLine const wxString&
/// \return void
///
/// Adds the passed stringt to the internal managed
/// text, stores it as read-only text and triggers
/// the rendering section
/////////////////////////////////////////////////
void TextManager::printOutput(const wxString& sLine)
{
    // Create a new line if the buffer is empty
    if (!m_managedText.size())
        newLine();

    // Copy the line and determine, whether the current line
    // contains the error signature (character #15)
    wxString _sLine = sLine;
    bool isErrorLine = _sLine.find((wxUniChar)15) != std::string::npos;

    // Remove the error signature
    while (_sLine.find((wxUniChar)15) != std::string::npos)
        _sLine.erase(_sLine.find((wxUniChar)15), 1);

    // Append the line to the current line
    m_managedText.back() += CharacterVector(_sLine, KERNEL_TEXT);

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(-1);

    // Update the colors and render the layout
    updateColors(isErrorLine);
    renderLayout();
}


/////////////////////////////////////////////////
/// \brief This is the user input function
///
/// \param sLine const wxString&
/// \param logicalpos size_t
/// \return void
///
/// Adds the passed stringt to the internal managed
/// text, stores it as user-given text and triggers
/// the rendering section
/////////////////////////////////////////////////
void TextManager::insertInput(const wxString& sLine, size_t logicalpos /*= std::string::npos*/)
{
    // Ensure that the logical position is a valid position
    if (logicalpos == std::string::npos)
        logicalpos = m_managedText.back().length();

    // Insert the text at the desired position
    m_managedText.back().insert(logicalpos, sLine, EDITABLE_TEXT);

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(-1);

    // Update colors and render the layout
    updateColors();
    renderLayout();
}


/////////////////////////////////////////////////
/// \brief Convert a logical cursor to a view cursor
///
/// \param logCursor const LogicalCursor&
/// \return ViewCursor
///
/// This member function converts the passed logical
/// cursor (line, position) into a view cursor (x, y)
/// for the current viewport size.
/////////////////////////////////////////////////
ViewCursor TextManager::toViewCursor(const LogicalCursor& logCursor) const
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
                    return ViewCursor(j, i - (m_topLine - m_numLinesScrolledUp));
            }
        }

        // If it is not valid, it probably is a hanging indent
        if (!m_renderedBlock[i].coords[0] && m_renderedBlock[i].coords[m_indentDepth].line == logCursor.line)
        {
            for (size_t j = m_indentDepth; j < m_renderedBlock[i].coords.size(); j++)
            {
                if (m_renderedBlock[i].coords[j].pos == logCursor.pos)
                    return ViewCursor(j, i - (m_topLine - m_numLinesScrolledUp));
            }
        }
    }

    // return an invalid cursor, if nothing was found
    return ViewCursor();
}


/////////////////////////////////////////////////
/// \brief Returns the current cursor position as view cursor
///
/// \return ViewCursor
///
/// This function returns the current input cursor
/// position (which is a logical cursor) and returns
/// it as a view cursor
/////////////////////////////////////////////////
ViewCursor TextManager::getCurrentViewPos() const
{
    // If the text was not rendered yet, return an invalid cursor
    if (!m_renderedBlock.size())
        return ViewCursor();

    // return the converted logical cursor
    return toViewCursor(getCurrentLogicalPos());
}


/////////////////////////////////////////////////
/// \brief Convert a view cursor into a logical cursor
///
/// \param viewCursor const ViewCursor&
/// \return LogicalCursor
///
/// This member function converts the passed view
/// cursor (x,y) into a logical cursor (line,position)
/// depending on the current viewport size.
/////////////////////////////////////////////////
LogicalCursor TextManager::toLogicalCursor(const ViewCursor& viewCursor) const
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


/////////////////////////////////////////////////
/// \brief Returns the current cursor position as logical cursor
///
/// \return LogicalCursor
///
/// This function returns the current input cursor
/// position (which is already a logical cursor).
/// This position is extracted from the current
/// position in the last line of the managed text
/////////////////////////////////////////////////
LogicalCursor TextManager::getCurrentLogicalPos() const
{
    // Ensure that there's text in the buffer
    if (!m_managedText.size())
        return LogicalCursor();

    // Construct a valid cursor with the dimensions of the
    // text buffer
    return LogicalCursor(m_managedText.back().length(), m_managedText.size()-1);
}


/////////////////////////////////////////////////
/// \brief Return the rendered line for the current viewport setting
///
/// \param viewLine size_t
/// \return wxString
///
/// This function returns the rendered line, which
/// goes into the selected viewLine. This function
/// requires that the managed text is already
/// rendered
/////////////////////////////////////////////////
wxString TextManager::getRenderedString(size_t viewLine) const
{
    // Return an empty line, if the line is not valid
    if (viewLine + m_topLine - m_numLinesScrolledUp >= m_renderedBlock.size() || viewLine > (size_t)m_viewportHeight)
        return "";

    return m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].sLine;
}


/////////////////////////////////////////////////
/// \brief Return the rendered colors for the selected viewport line
///
/// \param viewLine size_t
/// \return std::vector<unsigned short>
///
/// This function returns the colors for the rendered
/// line, which goes into the selected viewLine.
/// This function requires that the managed text is
/// already rendered
/////////////////////////////////////////////////
std::vector<unsigned short> TextManager::getRenderedColors(size_t viewLine) const
{
    // Return an empty vector, if the line is not valid
    if (viewLine + m_topLine - m_numLinesScrolledUp >= m_renderedBlock.size() || viewLine > (size_t)m_viewportHeight)
        return std::vector<unsigned short>();

    // Copy the current color line
    std::vector<unsigned short> colors = m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].colors;

    // Copy, whether there is a part of the current line is selected
    for (size_t i = 0; i < m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].coords.size(); i++)
    {
        LogicalCursor coords = m_renderedBlock[m_topLine - m_numLinesScrolledUp + viewLine].coords[i];

        if (m_managedText[coords.line].size() > coords.pos && m_managedText[coords.line][coords.pos].isSelected())
            colors[i] |= GenericTerminal::SELECTED;
    }

    // return the colors
    return colors;
}


/////////////////////////////////////////////////
/// \brief Helper function for converting the colors into a single int
///
/// \param fg int Color values ranging from 0-15
/// \param bg int Color values ranging from 0-15
/// \param flags int
/// \return int
///
/////////////////////////////////////////////////
int TextManager::calc_color( int fg, int bg, int flags )
{
    return (flags & 15) | (fg << 4) | (bg << 8);
}


/////////////////////////////////////////////////
/// \brief Update the colors for the current line
///
/// \param isErrorLine bool
/// \return void
///
/// This function determines the text snippets in
/// the last line of the managed text and creates
/// a color string for those snippets
/////////////////////////////////////////////////
void TextManager::updateColors(bool isErrorLine /*= false*/)
{
    // Get a pointer to the current syntax lexer
    NumeReSyntax* syntax = m_parent->getSyntax();
    std::string sColors;

    // If the current line shall be an error line, highlight it
    // correspondingly. Otherwise style it with the usual
    // lexer function
    if (isErrorLine)
        sColors = syntax->highlightError(m_managedText.back().toString().ToAscii().data());
    else
        sColors = syntax->highlightLine(m_managedText.back().toString().ToAscii().data());

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
        else if ((int)(sColors[i] - '0') == NumeReSyntax::SYNTAX_COMMENT)
            m_managedText.back()[i].setColor(calc_color((int)(sColors[i] - '0'), 0, GenericTerminal::BOLD));
        else
            m_managedText.back()[i].setColor(calc_color((int)(sColors[i] - '0'), 0, 0));
    }
}


/////////////////////////////////////////////////
/// \brief This function renders the layout
///
/// \return void
///
/// This function renders the layout for the
/// current terminal size using the internally
/// managed text. The rendered layout is cached,
/// so only the missing lines have to be re-
/// calculated
/////////////////////////////////////////////////
void TextManager::renderLayout()
{
    size_t firstLineToRender = 0;

    // Get the last rendered line of the current rendered
    // block
    if (m_renderedBlock.size())
        firstLineToRender = m_renderedBlock.back().coords.back().line + 1;

    // If the last rendered line is larger than the current
    // available managed text, clear it completely and
    // render a new layout
    if (firstLineToRender >= m_managedText.size())
    {
        firstLineToRender = 0;
        m_renderedBlock.clear();
    }

    // Go through the complete container
    for (size_t i = firstLineToRender; i < m_managedText.size(); i++)
    {
        size_t lastbreakpos = 0;
        bool firstline = true;
        LogicalCursor cursor(0, i);

        // Buffer the current line to avoid multiple memory-heavy constructions
        wxString sCurrentLine = m_managedText[i].toString();

        // Break the text lines at reasonable locations
        do
        {
            RenderedLine rLine;

            // If the last break position is non-zero, this is not a first line
            if (lastbreakpos)
                firstline = false;

            // Get the new break position
            size_t breakpos = findNextLinebreak(sCurrentLine, lastbreakpos);

            // If it's not the first line, add the indent here
            if (!firstline)
            {
                rLine.sLine = "|   ";
                rLine.colors.assign(m_indentDepth, calc_color(7, 0, 0));
                rLine.coords.assign(m_indentDepth, LogicalCursor());
            }

            // Assign text and colors
            rLine.sLine += sCurrentLine.substr(lastbreakpos, breakpos - lastbreakpos);
            std::vector<unsigned short> colors = m_managedText[i].subcolors(lastbreakpos, breakpos - lastbreakpos);
            rLine.colors.insert(rLine.colors.end(), colors.begin(), colors.end());

            // The last line has an additional white space character,
            // which is used as input location (to ensure that the cursor is valid)
            if (i + 1 == m_managedText.size())
            {
                rLine.sLine += " ";
                rLine.colors.push_back(calc_color(7, 0, 0));
            }

            // Assign the logical cursors
            for (size_t j = 0; j < rLine.sLine.length() - m_indentDepth*(!firstline); j++)
            {
                cursor.pos = j + lastbreakpos;
                rLine.coords.push_back(cursor);
            }

            lastbreakpos = breakpos;

            // Store the rendered line
            m_renderedBlock.push_back(rLine);
        }
        while (lastbreakpos < m_managedText[i].length());
    }

    // Calculate new topline, because it depends on the size of the rendered block
    m_topLine = m_renderedBlock.size() - m_viewportHeight;

    if (m_topLine < 0)
        m_topLine = 0;
}


/////////////////////////////////////////////////
/// \brief Removes parts of the already rendered block
///
/// \param linesToDelete int
/// \return void
///
/// This private member function removes parts of the already rendered
/// block depending on the number of lines to delete. The number of lines
/// corresponds to the text lines in the managed block. If the number is
/// positive, the lines are deleted from the front, if the number is negative,
/// then the lines are deleted from the back
/////////////////////////////////////////////////
void TextManager::synchronizeRenderedBlock(int linesToDelete)
{
    if (m_renderedBlock.size() && linesToDelete)
    {
        // If the number of lines are larger than the
        // available lines, simply clear the whole block
        if (m_renderedBlock.size() <= abs(linesToDelete))
        {
            m_renderedBlock.clear();
            return;
        }

        if (linesToDelete > 0)
        {
            // Delete lines from the front
            while (m_renderedBlock.size()
                   && (!m_renderedBlock.front().coords.size() || m_renderedBlock.front().coords.back().line < (size_t)linesToDelete))
            {
                m_renderedBlock.pop_front();
            }

            // Update the line numbers in the coords field,
            // because the leading lines have been removed and
            // the algorithm requires that the line numbering
            // starts from zero
            for (size_t i = 0; i < m_renderedBlock.size(); i++)
            {
                for (size_t j = 0; j < m_renderedBlock[i].coords.size(); j++)
                {
                    if (m_renderedBlock[i].coords[j].isValid)
                        m_renderedBlock[i].coords[j].line -= linesToDelete;
                }
            }
        }
        else
        {
            // Delete lines from the back
            //
            // Find the corresponding line from the back
            linesToDelete = (int)m_renderedBlock.back().coords.back().line + linesToDelete;

            while (m_renderedBlock.size()
                   && (!m_renderedBlock.back().coords.size() || m_renderedBlock.back().coords.back().line > (size_t)linesToDelete))
            {
                m_renderedBlock.pop_back();
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Find the next linebreak position
///
/// \param currentLine const wxString&
/// \param currentLinebreak size_t
/// \return size_t
///
/// This function finds the next possible linebreak
/// position from the current position using a simple
/// heuristic
/////////////////////////////////////////////////
size_t TextManager::findNextLinebreak(const wxString& currentLine, size_t currentLinebreak) const
{
    size_t nLastPossibleChar = m_viewportWidth + currentLinebreak
                                - m_indentDepth * (bool)currentLinebreak;

    // If the current line is shorter than the current viewport width,
    // return the length of the current line as next break position
    if (nLastPossibleChar > currentLine.length())
        return currentLine.length();

    // Store the valid line break characters
    static wxString sValidLinebreaks = "+- ,;.*/<>=!";

    // Find the next possible break position
    for (int i = nLastPossibleChar-1; i >= (int)currentLinebreak; i--)
    {
        // If the current character marks a valid line break position,
        // return the character after it
        if (sValidLinebreaks.find(currentLine[i]) != std::string::npos)
            return i+1;
    }

    // The fall back solution, if no valid position is found
    return nLastPossibleChar;
}


/////////////////////////////////////////////////
/// \brief Insert a tab character at the current position
///
/// \return size_t
///
/////////////////////////////////////////////////
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

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(-1);

    // Return the calculated tab length
    return tabLength;
}


/////////////////////////////////////////////////
/// \brief Adds a new line to the current managed text
///
/// \return void
///
/// This function adds a new line to the internally
/// managed text. If the managed text grows larger
/// than the selected size, the first lines are
/// dropped.
/////////////////////////////////////////////////
void TextManager::newLine()
{
    // Add a new line to all buffers and fill it with a reasonable value
    m_managedText.push_back(CharacterVector("", EDITABLE_TEXT));

    // Ensure that the buffer is not larger than the desired history length
    while (m_managedText.size() > (size_t)m_maxHeight)
    {
        m_managedText.pop_front();

        // Synchronize the rendered layout by deleting the
        // first line
        synchronizeRenderedBlock(1);
    }

    // Remove the last line due to the additional input character
    // INFO: Disabled due to not necessary double renders
    //synchronizeRenderedBlock(-1);

    // render layout, get new top line and reset the virtual cursor line
    renderLayout();
    ResetVirtualCursorLine();
}


/////////////////////////////////////////////////
/// \brief Performs a backspace operation
///
/// \param logCursor const LogicalCursor&
/// \return void
///
/// This member function performs a backspace operation
/// at the passed logical cursor position. Backspaces
/// are only allowed in the current input line, which
/// is the last line of the managed text (but not
/// necessarily the last line of the viewport line)
/////////////////////////////////////////////////
void TextManager::backspace(const LogicalCursor& logCursor)
{
    // Ensure that the cursor is valid
    if (!logCursor)
        return;

    // Apply a backspace
    if (logCursor.pos)
    {
        // In the current line
        m_managedText[logCursor.line].erase(m_managedText[logCursor.line].begin() + logCursor.pos - 1);
    }
    else
    {
        // go to the previous line
        m_managedText[logCursor.line - 1].pop_back();
    }

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(-1);

    // Update the colors and render the layout
    updateColors();
    renderLayout();
}


/////////////////////////////////////////////////
/// \brief Erase the current line
///
/// \return void
///
/// This member function erases the current line
/// (i.e. the last line of the managed text)
/// completely. This is used, if the printed
/// string starts with a carriage return character
/////////////////////////////////////////////////
void TextManager::eraseLine()
{
    // Do nothing if the buffer is empty
    if (!m_managedText.size())
        return;

    // Clear the lines and fill them with a
    // reasonable default content (just like in
    // the new line function)
    m_managedText.back().clear();

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(-1);

    // Render the layout
    renderLayout();
}


/////////////////////////////////////////////////
/// \brief Clears the range between two view cursors
///
/// \param cursor1 const ViewCursor&
/// \param cursor2 const ViewCursor&
/// \return bool
///
/// This member function is used to clear the range
/// between the two view cursors. It may even clear
/// the characters, which are spread across multiple
/// lines
/////////////////////////////////////////////////
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

    // Synchronize the rendered layout by deleting the
    // current line
    synchronizeRenderedBlock(std::min(logCursor1.line, logCursor2.line) - m_managedText.size() - 1);

    // As long as the second cursor is at a larger position than the first character
    while (logCursor2 > logCursor1)
    {
        if (logCursor1.line != logCursor2.line)
        {
            // The cursors are in different lines
            // Erase all from the first to the cursor's position
            m_managedText[logCursor2.line].erase(m_managedText[logCursor2].begin(),
                                                 m_managedText[logCursor2].begin()+logCursor2.pos);

            // If the text buffer of the current line is empty,
            // erase the overall line from all three buffers
            if (!m_managedText[logCursor2.line].length())
                m_managedText.erase(m_managedText.begin()+logCursor2.line);

            // set the cursor to the last position of the previous line
            logCursor2.line--;
            logCursor2.pos = m_managedText[logCursor2.line].length();
        }
        else
        {
            // The cursors are in the same line
            m_managedText[logCursor2.line].erase(m_managedText[logCursor2.line].begin() + logCursor1.pos,
                                                 m_managedText[logCursor2.line].begin() + logCursor2.pos);
            break;
        }
    }

    // Update colors and render the layout
    updateColors();
    renderLayout();

    return true;
}


/////////////////////////////////////////////////
/// \brief Selects the text at the view cursor position
///
/// \param viewCursor const ViewCursor&
/// \param bSelect bool
/// \return void
///
/// This member function is used to (de-)select
/// the character at the position of the passed
/// view cursor
/////////////////////////////////////////////////
void TextManager::selectText(const ViewCursor& viewCursor, bool bSelect /*= true*/)
{
    // Convert the view cursor to a logical cursor
    LogicalCursor cursor = toLogicalCursor(viewCursor);

    // Ensure that the cursor is valid
    if (!cursor
        || m_managedText.size() <= cursor.line
        || m_managedText[cursor.line].size() <= cursor.pos)
        return;

    // Select or deselect the pointer character
    if (bSelect)
        m_managedText[cursor.line][cursor.pos].select();
    else
        m_managedText[cursor.line][cursor.pos].unselect();
}


/////////////////////////////////////////////////
/// \brief This member function unselects the whole text at once
///
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Determines, whether the pointed character is selected
///
/// \param viewCursor const ViewCursor&
/// \return bool
///
/// This member function returns true, if the character
/// pointed by the passed view cursor is selected
/////////////////////////////////////////////////
bool TextManager::isSelected(const ViewCursor& viewCursor) const
{
    // Convert the view cursor into a logical cursor and
    // check for selection
    return isSelectedLogical(toLogicalCursor(viewCursor));
}


/////////////////////////////////////////////////
/// \brief Determines, whether the pointed
/// character is selected.
///
/// \param cursor const LogicalCursor&
/// \return bool
///
/////////////////////////////////////////////////
bool TextManager::isSelectedLogical(const LogicalCursor& cursor) const
{
    // Ensure that the cursor is valid
    if (!cursor
        || m_managedText.size() <= cursor.line
        || m_managedText[cursor.line].size() <= cursor.pos)
        return false;

    // Return true, if the color line at the current contains the SELECTED flag
    return m_managedText[cursor.line][cursor.pos].isSelected();
}


/////////////////////////////////////////////////
/// \brief This member function returns the selected text
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString TextManager::getSelectedText() const
{
    wxString sText;

    // Find every selected character in the text buffer
    for (size_t i = 0; i < m_managedText.size(); i++)
    {
        for (size_t j = 0; j < m_managedText[i].size(); j++)
        {
            if (m_managedText[i][j].isSelected())
                sText += m_managedText[i][j].m_char;
        }

        // Append a new line character after each new line
        if (sText.length() && sText.Last() != '\n')
            sText += '\n';
    }

    // Remove all trailing new line characters
    while (sText.length() && sText.Last() == '\n')
        sText.RemoveLast();

    // return the selected text
    return sText;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of the input line
///
/// \return wxString
///
/// This member function will return the contents
/// of the current user input line
/////////////////////////////////////////////////
wxString TextManager::getCurrentInputLine() const
{
    // Ensure that the buffer is available and that there's user text
    if (!m_managedText.size() || !m_managedText.back().back().editable())
        return "";

    wxString sInput;

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


/////////////////////////////////////////////////
/// \brief This member function returns the
/// contents of the line before the current input
/// line.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString TextManager::getPreviousLine() const
{
    // Ensure that the buffer is available and that there's user text
    if (m_managedText.size() < 2)
        return "";

    return m_managedText[m_managedText.size()-2].toString();
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
    m_renderedBlock.clear();

    m_bottomLine = m_maxHeight - 1;
    m_topLine = m_bottomLine - m_viewportHeight + 1;
    m_numLinesScrolledUp = 0;

    m_linesReceived = 1;

    m_tabLength = 4;
    m_indentDepth = 4;
}


//////////////////////////////////////////////////////////////////////////////
///  public operator[]
///  Allows access to a given line
///
///  @param  index    int  The index of the line to retrieve (0 to number of lines displayed)
///
///  @return wxString  The line at that index
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString TextManager::operator[](int index)
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
            return false;

        // If one wants to scrol more lines than available,
        // then restrict them
        if (m_topLine < actualLinesToScroll)
            actualLinesToScroll = m_topLine;

        // This is the maximal possible distance
        int limiter = m_topLine - m_numLinesScrolledUp;

        if (actualLinesToScroll > limiter)
            actualLinesToScroll = limiter;

        // scroll up
        m_numLinesScrolledUp += actualLinesToScroll;
    }
    else
    {
        // Ignore, if we're completely scrolled down
        if (m_numLinesScrolledUp <= 0)
            return false;

        // These are the possible lines to scroll down
        int linesBelow = m_renderedBlock.size() - (m_topLine + m_viewportHeight - m_numLinesScrolledUp);

        // Limit the lines to scroll down
        if ( linesBelow < actualLinesToScroll)
            actualLinesToScroll = linesBelow;

        // scroll down
        m_numLinesScrolledUp -= actualLinesToScroll;

        // Ensure that we're not negatively scrolled
        if (m_numLinesScrolledUp < 0)
            m_numLinesScrolledUp = 0;
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
int TextManager::GetSize() const
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

    // Clear the current layout and render
    // the layout completely new
    m_renderedBlock.clear();
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


/////////////////////////////////////////////////
/// \brief Get the next history line
///
/// \param vcursorup bool
/// \return wxString
///
/// This member function gets the next history line
/// depending on the bool vcursorup
/////////////////////////////////////////////////
wxString TextManager::GetInputHistory(bool vcursorup)
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
                            return m_managedText[m_virtualCursor].substr(i, j - i - 1);
                    }

                    return m_managedText[m_virtualCursor].substr(i);
                }
            }

            // decrement the virtual cursor
            m_virtualCursor--;
        }

        // Fallback
        m_virtualCursor++;
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
                            return m_managedText[m_virtualCursor].substr(i, j - i - 1);
                    }

                    return m_managedText[m_virtualCursor].substr(i);
                }
            }

            // Increment the virtual cursor
            m_virtualCursor++;
        }

        // Return an empty string, if the virtual cursor is the lowest possible line
        if (m_virtualCursor + 1u >= m_managedText.size())
            return "";
    }

    // Fallback
    return "";
}


/////////////////////////////////////////////////
/// \brief Extracts the text between the positions
///
/// \param y int
/// \param x0 int
/// \param x1 int
/// \return wxString
///
/////////////////////////////////////////////////
wxString TextManager::GetTextRange(int y, int x0, int x1) const
{
    // Convert the coordinates to a logical cursor
    LogicalCursor cursor = toLogicalCursor(ViewCursor(x0, y));

    // ensure that the cursor is valid
    if (!cursor)
        return "";

    // Return the extracted string
    return m_managedText[cursor.line].substr(cursor.pos, x1 - x0);
}


/////////////////////////////////////////////////
/// \brief Returns the word at the passed position
///
/// \param y int
/// \param x int
/// \return wxString
///
/// This member function returns the word, which
/// contains the character at (x,y)
/////////////////////////////////////////////////
wxString TextManager::GetWordAt(int y, int x) const
{
    // Convert the coordinates to a logical cursor
    LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

    // ensure that the cursor is valid
    if (!cursor)
        return "";

    // Get the text at the corresponnding line
    wxString sWord = m_managedText[cursor.line].toString();

    // Find the start of the word and erase everything in front of it
    for (int pos = cursor.pos; pos >= 0; pos--)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_') ///
            continue;

        sWord.erase(0, pos + 1);
        break;
    }

    // Find the end of the word and return it as a new string
    for (size_t pos = 0; pos < sWord.length(); pos++)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_') ///
            continue;

        return sWord.substr(0, pos);
    }

    // Fallback
    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the word start at the passed position
///
/// \param y int
/// \param x int
/// \return wxString
///
/// This member function returns the word start, which
/// contains the character at (x,y)
/////////////////////////////////////////////////
wxString TextManager::GetWordStartAt(int y, int x) const
{
    // Convert the coordinates to a logical cursor
    LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));

    // Ensure that the cursor is valid
    if (!cursor)
        return "";

    // Get the line until the position
    wxString sWord = m_managedText[cursor.line].substr(0, cursor.pos);

    // Find the start of the word and return it as a new string
    for (int pos = cursor.pos - 1; pos >= 0; pos--)
    {
        if (isalnum(sWord[pos]) || sWord[pos] == '_') ///
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
///  @return wxUniChar The character at that position
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxUniChar TextManager::GetCharAdjusted(int y, int x) const
{
    LogicalCursor cursor = toLogicalCursor(ViewCursor(x, y));
    return GetCharLogical(cursor);
}


/////////////////////////////////////////////////
/// \brief Returns the character at the logical position
///
/// \param cursor const LogicalCursor&
/// \return wxUniChar
///
/////////////////////////////////////////////////
wxUniChar TextManager::GetCharLogical(const LogicalCursor& cursor) const
{
    if (!cursor)
        return ' ';

    if (cursor.pos >= m_managedText[cursor.line].size())
        return ' '; // default color

    return m_managedText[cursor.line][cursor.pos].m_char;
}


/////////////////////////////////////////////////
/// \brief Determines, whether the character at (x,y) is a user text
///
/// \param y int
/// \param x int
/// \return bool
///
/////////////////////////////////////////////////
bool TextManager::IsUserText(int y, int x) const
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


/////////////////////////////////////////////////
/// \brief Determines, whether the character at (x,y) is editable text
///
/// \param y int
/// \param x int
/// \return bool
///
/////////////////////////////////////////////////
bool TextManager::IsEditable(int y, int x) const
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


/////////////////////////////////////////////////
/// \brief Determines, whether the character at the logical position is editable text
///
/// \param logCursor const LogicalCursor&
/// \return bool
///
/////////////////////////////////////////////////
bool TextManager::IsEditableLogical(const LogicalCursor& logCursor) const
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
unsigned short TextManager::GetColor(int y, int x) const
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
unsigned short TextManager::GetColorAdjusted(int y, int x) const
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


/////////////////////////////////////////////////
/// \brief Removes the editable flag from the managed text
///
/// \return void
///
/// This member function will switch every editable
/// text to a simple user text
/////////////////////////////////////////////////
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
int TextManager::AdjustIndex(int index) const
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
int TextManager::GetNumLinesScrolled() const
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
        return;

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

    // Clear the rendered block
    m_renderedBlock.clear();
}


//////////////////////////////////////////////////////////////////////////////
///  public GetLinesReceived
///  Returns the number of lines received
///
///  @return int The number of lines received
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int TextManager::GetLinesReceived() const
{
    return m_renderedBlock.size();
}


/////////////////////////////////////////////////
/// \brief Returns the buffer size of the terminal
///
/// \return int
///
/////////////////////////////////////////////////
int TextManager::GetMaxSize() const
{
    return m_maxHeight;
}




