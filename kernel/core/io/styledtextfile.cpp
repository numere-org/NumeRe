/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#include <fstream>
#include <sstream>

#include "styledtextfile.hpp"

/////////////////////////////////////////////////
/// \brief Splits the contents of a stream into
/// individual lines while keeping character
/// positions.
///
/// \param stream std::istream&
/// \param vSplitted std::vector<std::pair<size_t, std::string>>&
/// \return void
///
/////////////////////////////////////////////////
static void split(std::istream& stream, std::vector<std::pair<size_t, std::string>>& vSplitted)
{
    std::string sLine;

    if (!stream.good())
        return;

    vSplitted.clear();

    // Read the complete stream
    while (!stream.eof())
    {
        std::getline(stream, sLine);

        // Store also the character positions at the
        // start of the current line
        if (!vSplitted.size())
            vSplitted.push_back(std::make_pair(0, sLine));
        else
            vSplitted.push_back(std::make_pair(vSplitted.back().first + vSplitted.back().second.length() + 2, sLine));
    }
}


/////////////////////////////////////////////////
/// \brief This method loads the specified file
/// to memory, while keeping the character
/// positions.
///
/// \return void
///
/////////////////////////////////////////////////
void StyledTextFile::load()
{
    std::ifstream file(sFileName);

    split(file, vFileContents);
}


/////////////////////////////////////////////////
/// \brief This method parses the already loaded
/// file into a memory representation with kept
/// character positions.
///
/// \param fileContents const std::string&
/// \return void
///
/////////////////////////////////////////////////
void StyledTextFile::parse(const std::string& fileContents)
{
    std::istringstream file(fileContents);

    split(file, vFileContents);
}


/////////////////////////////////////////////////
/// \brief This method runs a lexer over the
/// loaded document to obtain styling information.
///
/// \return void
///
/////////////////////////////////////////////////
void StyledTextFile::lex()
{
    vStyles.clear();

    Style lastStyle = DEFAULT;

    // Go through the document
    for (size_t i = 0; i < vFileContents.size(); i++)
    {
        // Create a new line in the style table
        if (vFileContents[i].second.length())
            vStyles.push_back(std::vector<Style>(vFileContents[i].second.length()+2, DEFAULT));
        else // Fallback for empty lines, which will otherwise cancel comment blocks
            vStyles.push_back(std::vector<Style>(2, lastStyle));

        // Detect changes in the styles
        for (size_t j = 0; j < vFileContents[i].second.length(); j++)
        {
            // Are we currently in default mode?
            if (lastStyle == DEFAULT)
            {
                size_t offset = 0;

                if (sDocCommentLine.length()
                    && vFileContents[i].second.substr(j, sDocCommentLine.length()) == sDocCommentLine)
                {
                    for (; j < vStyles[i].size(); j++)
                        vStyles[i][j] = COMMENT_DOC_LINE;
                }
                else if (sCommentLine.length()
                         && vFileContents[i].second.substr(j, sCommentLine.length()) == sCommentLine)
                {
                    for (; j < vStyles[i].size(); j++)
                        vStyles[i][j] = COMMENT_LINE;
                }
                else if (useStrings
                         && vFileContents[i].second.substr(j, sStringMarks.length()) == sStringMarks)
                {
                    lastStyle = STRING;
                    offset = sStringMarks.length();
                }
                else if (sDocCommentBlockStart.length()
                         && vFileContents[i].second.substr(j, sDocCommentBlockStart.length()) == sDocCommentBlockStart)
                {
                    lastStyle = COMMENT_DOC_BLOCK;
                    offset = sDocCommentBlockStart.length();
                }
                else if (sCommentBlockStart.length()
                         && vFileContents[i].second.substr(j, sCommentBlockStart.length()) == sCommentBlockStart)
                {
                    lastStyle = COMMENT_BLOCK;
                    offset = sCommentBlockStart.length();
                }

                // Increment the position, if the current
                // style starts a block style
                if (lastStyle > BLOCK_START)
                {
                    for (size_t n = 0; n < offset; j++, n++)
                        vStyles[i][j] = lastStyle;
                }
            }

            // Is the current style part of any
            // block style (possible lasting
            // over multiple lines)
            if (lastStyle > BLOCK_START)
            {
                size_t pos;
                size_t blockEndLength;

                // Try to find the end of the current
                // block style
                if (lastStyle == STRING)
                {
                    blockEndLength = 1;
                    pos = j;

                    while ((pos = vFileContents[i].second.find(sStringMarks, pos)) != std::string::npos)
                    {
                        if (!pos || vFileContents[i].second[pos-1] != '\\' || (pos > 1 && vFileContents[i].second[pos-2] == '\\'))
                            break;

                        pos++;
                    }
                }
                else
                {
                    pos = vFileContents[i].second.find(sBlockEnd, j);
                    blockEndLength = sBlockEnd.length();
                }

                // Apply the styling to the current line
                // (style everything, if nothing was found,
                // otherwise until the detected position)
                if (pos == std::string::npos)
                {
                    for (; j < vStyles[i].size(); j++)
                        vStyles[i][j] = lastStyle;
                }
                else
                {
                    for (; j < pos+blockEndLength; j++)
                        vStyles[i][j] = lastStyle;

                    lastStyle = DEFAULT;
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief StyledTextFile constructor. Will load
/// and lex the specified file using NumeRe code.
///
/// \param fileName const std::string&
/// \param fileContents const std::string&
/// optional parameter containing the already
/// loaded file contents.
///
/////////////////////////////////////////////////
StyledTextFile::StyledTextFile(const std::string& fileName, const std::string& fileContents) : sFileName(fileName)
{
    sCommentLine = "##";
    sDocCommentLine = "##!";
    sCommentBlockStart = "#*";
    sDocCommentBlockStart = "#*!";
    sBlockEnd = "*#";
    sStringMarks = "\"";
    useStrings = true;

    // Either parse the passed file contents or
    // load the file contents directly from file.
    if (fileContents.length())
        parse(fileContents);
    else
        load();

    lex();
}


/////////////////////////////////////////////////
/// \brief Returns the filename of the
/// respresented file in memory.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string StyledTextFile::getFileName() const
{
    return sFileName;
}


/////////////////////////////////////////////////
/// \brief This method returns the text between
/// the passed two positions (including possible
/// line termination characters).
///
/// \param pos1 int
/// \param pos2 int
/// \return std::string
///
/////////////////////////////////////////////////
std::string StyledTextFile::getTextRange(int pos1, int pos2) const
{
    // Find the start and end line
    int line1 = LineFromPosition(pos1);
    int line2 = LineFromPosition(pos2);

    if (line1 == -1 || line2 == -1)
        return "";

    // Find the positions in the lines
    pos1 -= vFileContents[line1].first;
    pos2 -= vFileContents[line2].first;

    std::string sTextRange;

    // Extract the contents into a single string
    if (line1 == line2)
        sTextRange = (vFileContents[line1].second + "\r\n").substr(pos1, pos2-pos1);
    else
    {
        sTextRange = (vFileContents[line1].second + "\r\n").substr(pos1);

        for (int line = line1+1; line < line2; line++)
            sTextRange += vFileContents[line].second + "\r\n";

        sTextRange += (vFileContents[line2].second + "\r\n").substr(0, pos2);
    }

    return sTextRange;
}


/////////////////////////////////////////////////
/// \brief Returns the selected line (without the
/// line termination characters).
///
/// \param line size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StyledTextFile::getLine(size_t line) const
{
    if (line < vFileContents.size())
        return vFileContents[line].second;

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the selected line (without the
/// line termination characters and without any
/// comments).
///
/// \param line size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StyledTextFile::getStrippedLine(size_t line) const
{
    if (line < vFileContents.size())
    {
        std::string sLine;

        for (size_t i = 0; i < vFileContents[line].second.length(); i++)
        {
            if (vStyles[line][i] == StyledTextFile::DEFAULT || vStyles[line][i] == StyledTextFile::STRING)
                sLine.push_back(vFileContents[line].second[i]);
        }

        return sLine;
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the last printable character
/// position in the currently loaded file.
///
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::getLastPosition() const
{
    return getLinesCount() ? vFileContents.back().first + vFileContents.back().second.length() : -1;
}


/////////////////////////////////////////////////
/// \brief Returns the number of lines in the
/// current loaded file.
///
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::getLinesCount() const
{
    return vFileContents.size();
}


/////////////////////////////////////////////////
/// \brief Returns the position of the first
/// character in the selected line.
///
/// \param line size_t
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::PositionFromLine(size_t line) const
{
    return getLineStartPosition(line);
}


/////////////////////////////////////////////////
/// \brief Returns the line number containing the
/// selected character position.
///
/// \param pos size_t
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::LineFromPosition(size_t pos) const
{
    for (size_t line = 0; line < vFileContents.size(); line++)
    {
        if (pos >= vFileContents[line].first && pos < vFileContents[line].first + vFileContents[line].second.length()+2)
            return line;
    }

    return -1;
}


/////////////////////////////////////////////////
/// \brief Returns the position of the last
/// printable character in the selected line
/// (before any line termination characters).
///
/// \param line size_t
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::getLineEndPosition(size_t line) const
{
    if (line < vFileContents.size())
        return vFileContents[line].first + vFileContents[line].second.length();

    return -1;
}


/////////////////////////////////////////////////
/// \brief Returns the position of the first
/// character in the selected line.
///
/// \param line size_t
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::getLineStartPosition(size_t line) const
{
    if (line < vFileContents.size())
        return vFileContents[line].first;

    return -1;
}


/////////////////////////////////////////////////
/// \brief Finds the first line of the current
/// documentation comment or -1, if the current
/// line does not contain any documentation
/// comment.
///
/// \param line size_t
/// \return int
///
/////////////////////////////////////////////////
int StyledTextFile::findDocStartLine(size_t line) const
{
    size_t pos = vFileContents[line].second.find_first_not_of(" \t");

    if (pos == std::string::npos
        || (getStyleAt(pos + vFileContents[line].first) != COMMENT_DOC_BLOCK
            && getStyleAt(pos + vFileContents[line].first) != COMMENT_DOC_LINE))
        return -1;

    long long int nPos = pos + vFileContents[line].first;
    size_t nWhitespace = 0;

    while (nPos >= 0 && (getStyleAt(nPos) == COMMENT_DOC_BLOCK || getStyleAt(nPos) == COMMENT_DOC_LINE || getCharAt(nPos) == ' ' || getCharAt(nPos) == '\t'))
    {
        if ((getStyleAt(nPos) != COMMENT_DOC_BLOCK && getStyleAt(nPos) != COMMENT_DOC_LINE) && (getCharAt(nPos) == ' ' || getCharAt(nPos) == '\t'))
            nWhitespace++;
        else
            nWhitespace = 0;

        nPos--;
    }

    return LineFromPosition(nPos+nWhitespace);
}


/////////////////////////////////////////////////
/// \brief Returns the style at the current
/// selected character position.
///
/// \param pos size_t
/// \return StyledTextFile::Style
///
/////////////////////////////////////////////////
StyledTextFile::Style StyledTextFile::getStyleAt(size_t pos) const
{
    int line = LineFromPosition(pos);

    if (line != -1)
        return vStyles[line][pos - getLineStartPosition(line)];

    return STYLE_ERROR;
}


/////////////////////////////////////////////////
/// \brief Returns the character located at the
/// position pos.
///
/// \param pos size_t
/// \return char
///
/////////////////////////////////////////////////
char StyledTextFile::getCharAt(size_t pos) const
{
    int line = LineFromPosition(pos);

    if (line != -1)
        return vFileContents[line].second[pos - vFileContents[line].first];

    return '\0';
}


/////////////////////////////////////////////////
/// \brief Can be used to change the code style
/// detection sequences and to re-apply the lexer
/// to the currently loaded file.
///
/// \param sComLine const std::string&
/// \param sDocComLine const std::string&
/// \param sComBlockStart const std::string&
/// \param sDocComBlockStart const std::string&
/// \param sComBlockEnd const std::string&
/// \param sStrMarks const std::string&
/// \param strings bool
/// \return void
///
/////////////////////////////////////////////////
void StyledTextFile::reStyle(const std::string& sComLine,
                             const std::string& sDocComLine,
                             const std::string& sComBlockStart,
                             const std::string& sDocComBlockStart,
                             const std::string& sComBlockEnd,
                             const std::string& sStrMarks,
                             bool strings)
{
    sCommentLine = sComLine;
    sDocCommentLine = sDocComLine;
    sCommentBlockStart = sComBlockStart;
    sDocCommentBlockStart = sComBlockStart;
    sBlockEnd = sComBlockEnd;
    sStringMarks = sStrMarks;
    useStrings = strings;

    lex();
}



