
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


#ifndef STYLEDTEXTFILE_HPP
#define STYLEDTEXTFILE_HPP

#include <string>
#include <vector>
#include <utility>

/////////////////////////////////////////////////
/// \brief This class represents a text file in
/// memory (e.g. a code file). This class will
/// try to lex the loaded file and detect simple
/// comment blocks. It defaults to NumeRe code
/// files, however the comment character
/// sequences can be adapted using the reStyle()
/// method.
///
/// The interface resembles the wxStyledTextCtrl
/// interface for simplified portability.
/////////////////////////////////////////////////
class StyledTextFile
{
    public:
        enum Style
        {
            STYLE_ERROR = -1,
            DEFAULT = 0,
            COMMENT_LINE,
            COMMENT_DOC_LINE,
            BLOCK_START,
            STRING,
            COMMENT_BLOCK,
            COMMENT_DOC_BLOCK
        };

    private:
        std::vector<std::pair<size_t, std::string>> vFileContents;
        std::vector<std::vector<Style>> vStyles;

        std::string sFileName;

        std::string sCommentLine;
        std::string sDocCommentLine;
        std::string sCommentBlockStart;
        std::string sDocCommentBlockStart;
        std::string sBlockEnd;

        void load();
        void lex();

    public:
        StyledTextFile(const std::string& fileName);

        std::string getFileName() const;

        std::string getTextRange(int nPos1, int nPos2) const;
        std::string getLine(size_t line) const;
        int getLastPosition() const;
        int getLinesCount() const;
        int PositionFromLine(size_t line) const;
        int LineFromPosition(size_t pos) const;
        int getLineEndPosition(size_t line) const;
        int getLineStartPosition(size_t line) const;

        Style getStyleAt(size_t pos) const;

        void reStyle(const std::string& sComLine,
                     const std::string& sDocComLine,
                     const std::string& sComBlockStart,
                     const std::string& sDocComBlockStart,
                     const std::string& sComBlockEnd);
};

#endif // STYLEDTEXTFILE_HPP


