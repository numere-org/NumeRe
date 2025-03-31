/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "codeformatter.hpp"
#include "editor.h"

using namespace std;

/////////////////////////////////////////////////
/// \brief Inserts text and returns the length of
/// the insertion.
///
/// \param nPosition int
/// \param sText const wxString&
/// \return int
///
/// This is a simple helper function to simplify
/// keeping track of the actual insertion position.
/////////////////////////////////////////////////
int CodeFormatter::insertTextAndMove(int nPosition, const wxString& sText)
{
    m_editor->InsertText(nPosition, sText);
    return sText.length();
}


/////////////////////////////////////////////////
/// \brief Automatically indents the code based
/// upon its hierarchy.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void CodeFormatter::IndentCode(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
    if (nFirstLine < 0)
        nFirstLine = 0;

    if (nLastLine <= 0 || nLastLine > m_editor->GetLineCount())
        nLastLine = m_editor->GetLineCount();

    m_editor->SetTabWidth(4);

    // If we do not start at the beginning of the document,
    // then keep the latest indentation
    int nIndentCount = nFirstLine > 0 ? m_editor->GetLineIndentation(nFirstLine) / 4 : 0;
    int nCurrentIndent = 0;

    int singleLineIndent = 0;
    int nLastSingleIndent = 0;
    m_editor->BeginUndoAction();

    // Go through the complete document
    for (int i = nFirstLine; i < nLastLine; i++)
    {
        nLastSingleIndent = singleLineIndent;
        singleLineIndent = 0;
        int pos = m_editor->PositionFromLine(i);

        // Ignore line comments
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, pos))
            continue;

        // Ignore block comments
        while (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, pos) && pos < m_editor->GetLineEndPosition(nLastLine))
            pos++;

        if (pos > m_editor->GetLineEndPosition(i) - 1)
            continue;

        // Determine the current indentation level
        nCurrentIndent = determineIndentationLevel(i, singleLineIndent);

        if (m_editor->getFileType() == FILE_CPP && nLastSingleIndent && nCurrentIndent)
            nLastSingleIndent = 0;

        // Apply the indentation level
        if (!nCurrentIndent && singleLineIndent < 0)
        {
            singleLineIndent = 0;
            m_editor->SetLineIndentation(i, 4 * (nIndentCount - 1));
        }
        else if (!nCurrentIndent)
            m_editor->SetLineIndentation(i, 4 * (nIndentCount + nLastSingleIndent));
        else if (nCurrentIndent < 0)
        {
            nIndentCount += nCurrentIndent;
            m_editor->SetLineIndentation(i, 4 * nIndentCount);
        }
        else
        {
            m_editor->SetLineIndentation(i, 4 * nIndentCount);
            nIndentCount += nCurrentIndent;
        }

        if (m_editor->GetCurrentLine() == i
                && m_editor->GetCurrentPos() - pos < nIndentCount
                && !m_editor->HasSelection()
                && m_editor->GetTextRange(pos, m_editor->GetLineEndPosition(i)).find_first_not_of(" \t\n\r") == string::npos)
            m_editor->GotoPos(pos + nIndentCount);
    }

    m_editor->EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Determines the indentation level of the
/// selected line.
///
/// \param nLine int
/// \param singleLineIndent int&
/// \return int
///
/// This function determines the indentation level
/// of the code at the selected line by delegating
/// this task to the language specific calculator.
/////////////////////////////////////////////////
int CodeFormatter::determineIndentationLevel(int nLine, int& singleLineIndent)
{
    // Use the correct indentation level calculator
    // for the current language
    if (m_editor->getFileType() == FILE_NSCR || m_editor->getFileType() == FILE_NPRC)
        return determineIndentationLevelNSCR(nLine, singleLineIndent);
    else if (m_editor->getFileType() == FILE_MATLAB)
        return determineIndentationLevelMATLAB(nLine, singleLineIndent);
    else if (m_editor->getFileType() == FILE_CPP)
        return determineIndentationLevelCPP(nLine, singleLineIndent);

    return 0;
}


/////////////////////////////////////////////////
/// \brief Determines the indentation level of the
/// selected line of NumeRe code.
///
/// \param nLine int
/// \param singleLineIndent int&
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::determineIndentationLevelNSCR(int nLine, int& singleLineIndent)
{
    int nIndentCount = 0;

    int nLineStart = m_editor->PositionFromLine(nLine);
    int nLineEnd = m_editor->GetLineEndPosition(nLine);

    // Search through the line
    for (int i = nLineStart; i < nLineEnd; i++)
    {
        // Examine flow control statements
        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_COMMAND || m_editor->GetStyleAt(i) == wxSTC_NSCR_PROCEDURE_COMMANDS)
        {
            wxString word = m_editor->GetTextRange(i, m_editor->WordEndPosition(i + 1, true));

            if (m_editor->isBlockEnd(word) != wxNOT_FOUND)
                nIndentCount--;
            else if (m_editor->isBlockStart(word, false) != wxNOT_FOUND)
                nIndentCount++;
            else if (m_editor->isBlockMiddle(word) != wxNOT_FOUND)
                singleLineIndent = -1;

            i += word.length();
        }

        // Examine install sections
        if (m_editor->getFileType() == FILE_NSCR && m_editor->GetStyleAt(i) == wxSTC_NSCR_INSTALL)
        {
            wxString word;

            if (m_editor->GetCharAt(i) == '<' && m_editor->FindText(i, nLineEnd, ">") != -1)
            {
                word = m_editor->GetTextRange(i, m_editor->WordEndPosition(i + 2, true) + 1);

                if (m_editor->isBlockStart(word, false) != wxNOT_FOUND)
                    nIndentCount++;
                else if (m_editor->isBlockEnd(word) != wxNOT_FOUND)
                    nIndentCount--;
            }

            if (word.length())
                i += word.length() - 1;
        }

        // Examine line continuations
        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_OPERATORS && m_editor->GetTextRange(i, i + 2) == "\\\\")
            singleLineIndent = 1;
    }

    return nIndentCount;
}


/////////////////////////////////////////////////
/// \brief Determines the indentation level of the
/// selected line of MATLAB code.
///
/// \param nLine int
/// \param singleLineIndent int&
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::determineIndentationLevelMATLAB(int nLine, int& singleLineIndent)
{
    int nIndentCount = 0;

    int nLineStart = m_editor->PositionFromLine(nLine);
    int nLineEnd = m_editor->GetLineEndPosition(nLine);

    for (int i = nLineStart; i < nLineEnd; i++)
    {
        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
        {
            wxString word = m_editor->GetTextRange(i, m_editor->WordEndPosition(i + 1, true));

            if (m_editor->isBlockEnd(word) != wxNOT_FOUND)
                nIndentCount--;
            else if (m_editor->isBlockStart(word, false) != wxNOT_FOUND)
                nIndentCount++;
            else if (m_editor->isBlockMiddle(word) != wxNOT_FOUND)
                singleLineIndent = -1;

            i += word.length();
        }

        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_OPERATOR && m_editor->GetTextRange(i, i + 3) == "...")
            singleLineIndent = 1;
    }

    return nIndentCount;
}


/////////////////////////////////////////////////
/// \brief Determines the indentation level of the
/// selected line of C++ code.
///
/// \param nLine int
/// \param singleLineIndent int&
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::determineIndentationLevelCPP(int nLine, int& singleLineIndent)
{
    int nIndentCount = 0;

    int nLineStart = m_editor->PositionFromLine(nLine);
    int nLineEnd = m_editor->GetLineEndPosition(nLine);

    for (int i = nLineStart; i < nLineEnd; i++)
    {
        if (m_editor->GetStyleAt(i) == wxSTC_C_OPERATOR)
        {
            if (m_editor->GetCharAt(i) == '{')
                nIndentCount++;

            if (m_editor->GetCharAt(i) == '}')
                nIndentCount--;
        }

        if (m_editor->GetStyleAt(i) == wxSTC_C_WORD)
        {
            wxString word = m_editor->GetTextRange(i, m_editor->WordEndPosition(i + 1, true));

            if (word == "private" || word == "protected" || word == "public" || word == "case" || word == "default")
                singleLineIndent = -1;

            if (word == "if" || word == "else" || word == "for" || word == "while")
                singleLineIndent = 1;

            i += word.length();
        }
    }

    return nIndentCount;
}


/////////////////////////////////////////////////
/// \brief Applies automatic formatting to the
/// selected code lines.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/// Formatting is done by delegating this taks to
/// the specialized formatting function. It also
/// indents the code automatically based upon its
/// hierarchy.
/////////////////////////////////////////////////
void CodeFormatter::FormatCode(int nFirstLine, int nLastLine)
{
    if (nFirstLine < 0)
        nFirstLine = 0;

    if (nLastLine <= 0 || nLastLine > m_editor->GetLineCount())
        nLastLine = m_editor->GetLineCount();

    // Format the code
    if (m_editor->getFileType() == FILE_NSCR || m_editor->getFileType() == FILE_NPRC)
        ApplyAutoFormatNSCR(nFirstLine, nLastLine);
    else if (m_editor->getFileType() == FILE_MATLAB)
        ApplyAutoFormatMATLAB(nFirstLine, nLastLine);
    else if (m_editor->getFileType() == FILE_CPP)
        ApplyAutoFormatCPP(nFirstLine, nLastLine);

    // Indent the code
    IndentCode(nFirstLine, m_editor->LineFromPosition(m_editor->GetLineEndPosition(nLastLine)));
}


/////////////////////////////////////////////////
/// \brief Applies autoformatting to NumeRe code.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void CodeFormatter::ApplyAutoFormatNSCR(int nFirstLine, int nLastLine)
{
    if (m_editor->getFileType() != FILE_NSCR && m_editor->getFileType() != FILE_NPRC)
        return;

    m_editor->BeginUndoAction();

    int nFirstPosition = m_editor->PositionFromLine(nFirstLine);
    int nLastPosition = m_editor->GetLineEndPosition(nLastLine);
    int nIndentationLevel = (m_editor->getFileType() == FILE_NPRC) ? -1 : 0;

    for (int i = nFirstPosition; i < nLastPosition; i++)
    {
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i)
            || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i))
            continue;

        if (m_editor->GetCharAt(i) == '\r')
            continue;

        if (m_editor->GetCharAt(i) == '\n')
        {
            // Need to refresh the styling bc. that doesn't get updated
            // automatically (problem for brace-match and such)
            m_editor->Colourise(i, nLastPosition);
            continue;
        }

        // Jump over operator keywords
        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_OPERATOR_KEYWORDS
            || (m_editor->GetStyleAt(i) == wxSTC_NSCR_OPERATORS && m_editor->GetTextRange(i, i + 1) == "<>"))
        {
            for (; i < nLastPosition; i++)
            {
                if ((m_editor->GetStyleAt(i) != wxSTC_NSCR_STRING && (m_editor->GetCharAt(i) == ' '
                                                                      || m_editor->GetCharAt(i) == ';'))
                                                                      || m_editor->GetCharAt(i) == '\r'
                                                                      || m_editor->GetCharAt(i) == '\n')
                    break;
            }
        }

        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_OPERATORS)
        {
            int currentChar = m_editor->GetCharAt(i);
            int prevChar = m_editor->GetCharAt(i - 1);
            int nextChar = m_editor->GetCharAt(i + 1);
            static std::string sParens = "(){}[]";

            if (currentChar == '(' && m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_COMMAND)
                nLastPosition += insertTextAndMove(i, " ");
            else if (currentChar == '('
                     && (m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_FUNCTION
                         || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_CUSTOM_FUNCTION
                         || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES
                         || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_METHOD
                         || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_PREDEFS))
            {
                // Apply logic to the contents of the function parentheses
                nLastPosition += formatFunctionParentheses(i, wxSTC_NSCR_OPERATORS);
            }
            else if (sParens.find(currentChar) != std::string::npos)
                continue;
            else if (currentChar == ',' && nextChar != ' ')
                nLastPosition += insertTextAndMove(i + 1, " ");
            else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
            {
                if (nextChar != ' ')
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (prevChar != ' ')
                    nLastPosition += insertTextAndMove(i, " ");
            }
            else if (currentChar == '<' || currentChar == '>' || currentChar == '!' || currentChar == '=')
            {
                static std::string sLeadingChars = " (=+-!*/^<>:|";

                if (currentChar == '='
                        && (m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_OPTION
                            || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_OPTION
                            || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_COMMAND))
                    continue;

                if (nextChar != ' '
                    && nextChar != '='
                    && nextChar != '>'
                    && currentChar != '!'
                    && !(currentChar == '>' && prevChar == '<')
                    && m_editor->GetStyleAt(i - 1) != wxSTC_NSCR_OPTION
                    && m_editor->GetStyleAt(i + 1) != wxSTC_NSCR_OPTION)
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (sLeadingChars.find(prevChar) == std::string::npos && !(currentChar == '<' && nextChar == '>'))
                    nLastPosition += insertTextAndMove(i, " ");
            }

            nLastPosition += formatOperators(i, prevChar, currentChar, nextChar, true);
        }

        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_COMMAND)
        {
            int nPos1 = i;

            while (m_editor->GetStyleAt(i + 1) == wxSTC_NSCR_COMMAND)
                i++;

            wxString command = m_editor->GetTextRange(nPos1, i + 1);
            int nCurrentLineStart = m_editor->PositionFromLine(m_editor->LineFromPosition(nPos1));
            int nCurrentLineEnd = m_editor->GetLineEndPosition(m_editor->LineFromPosition(nPos1));

            if (command == "global" || command == "load" || command == "append")
            {
                i = nCurrentLineEnd;
                continue;
            }
            else if (command == "set")
            {
                for (; i <= nCurrentLineEnd; i++)
                {
                    if (m_editor->GetStyleAt(i) == wxSTC_NSCR_OPERATORS && m_editor->GetCharAt(i) == '[')
                    {
                        int bracepos = m_editor->BraceMatch(i);

                        if (bracepos > 0)
                        {
                            for (; i < bracepos; i++)
                            {
                                if (m_editor->GetCharAt(i) == ',' && m_editor->GetCharAt(i + 1) != ' ')
                                    nLastPosition += insertTextAndMove(i + 1, " ");
                            }
                        }
                    }
                }
            }

            // Line break after block statement
            if (command == "if"
                || command == "elseif"
                || command == "for"
                || command == "switch"
                || command == "while")
            {
                int parenStart = m_editor->FindText(i, nCurrentLineEnd, "(");

                if (parenStart > 0)
                {
                    int parenEnd = m_editor->BraceMatch(parenStart);

                    if (parenEnd > 0)
                    {
                        int nextVisibleCharAfterParens = m_editor->GetTextRange(parenEnd + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t");

                        if (nextVisibleCharAfterParens != wxNOT_FOUND
                            && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, parenEnd + 1 + nextVisibleCharAfterParens)
                            && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, parenEnd + 1 + nextVisibleCharAfterParens))
                            nLastPosition += insertTextAndMove(parenEnd + 1, "\r\n");
                    }
                }
            }
            else if (command == "else" || command == "try")
            {
                int nextVisibleCharAfterCommand = m_editor->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t");

                if (nextVisibleCharAfterCommand != wxNOT_FOUND
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i + 1 + nextVisibleCharAfterCommand)
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i + 1 + nextVisibleCharAfterCommand))
                    nLastPosition += insertTextAndMove(i + 1, "\r\n");
            }
            else if (command == "case" || command == "default" || command == "catch")
            {
                int nColon = m_editor->FindText(i, nCurrentLineEnd, ":");
                int nextVisibleCharAfterColon = m_editor->GetTextRange(nColon + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t");

                if (nextVisibleCharAfterColon != wxNOT_FOUND
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, nColon + 1 + nextVisibleCharAfterColon)
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, nColon + 1 + nextVisibleCharAfterColon))
                    nLastPosition += insertTextAndMove(nColon + 1, "\r\n");
            }

            // Line break in front of block statement
            if (m_editor->getBlockID(command) != wxNOT_FOUND)
            {
                if (m_editor->GetTextRange(nCurrentLineStart, nPos1).find_first_not_of(" \t") != std::string::npos)
                {
                    nLastPosition += insertTextAndMove(nPos1, "\r\n");
                    i += 2;
                }
            }

            if (m_editor->isBlockStart(command, false) != wxNOT_FOUND)
            {
                if (nIndentationLevel <= 0)
                {
                    int nLine = m_editor->LineFromPosition(i);
                    int position = m_editor->PositionFromLine(nLine - 1);

                    while (m_editor->GetCharAt(position) == ' ' || m_editor->GetCharAt(position) == '\t')
                        position++;

                    if (nLine
                        && m_editor->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != std::string::npos
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, m_editor->PositionFromLine(nLine - 1))
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, position))
                    {
                        nLastPosition += insertTextAndMove(m_editor->PositionFromLine(nLine), "\r\n");
                        i += 2;
                    }
                }

                nIndentationLevel++;
            }

            if (m_editor->isBlockEnd(command) != wxNOT_FOUND)
            {
                nIndentationLevel--;

                if (nIndentationLevel <= 0)
                {
                    int nLine = m_editor->LineFromPosition(i);

                    if (nLine < m_editor->GetLineCount() - 1
                        && m_editor->GetLine(nLine + 1).find_first_not_of(" \t\r\n") != std::string::npos)
                        nLastPosition += insertTextAndMove(m_editor->PositionFromLine(nLine + 1), "\r\n");
                }
            }
        }

        if (m_editor->GetStyleAt(i) == wxSTC_NSCR_STRING)
            nLastPosition += formatStrings(i, true);
    }

    m_editor->EndUndoAction();
    m_editor->Colourise(0, -1);
}


/////////////////////////////////////////////////
/// \brief Applies autoformatting to MATLAB code.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void CodeFormatter::ApplyAutoFormatMATLAB(int nFirstLine, int nLastLine)
{
    if (m_editor->getFileType() != FILE_MATLAB)
        return;

    m_editor->BeginUndoAction();

    int nFirstPosition = m_editor->PositionFromLine(nFirstLine);
    int nLastPosition = m_editor->GetLineEndPosition(nLastLine);
    int nIndentationLevel = -1;

    for (int i = nFirstPosition; i < nLastPosition; i++)
    {
        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_COMMENT)
            continue;

        if (m_editor->GetCharAt(i) == '\r'
            || m_editor->GetCharAt(i) == '\n')
            continue;

        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_OPERATOR)
        {
            int currentChar = m_editor->GetCharAt(i);
            int prevChar = m_editor->GetCharAt(i - 1);
            int nextChar = m_editor->GetCharAt(i + 1);
            static std::string sParens = "(){}[]";

            if (currentChar == '('
                && (m_editor->GetStyleAt(i - 1) == wxSTC_MATLAB_IDENTIFIER
                    || m_editor->GetStyleAt(i - 1) == wxSTC_MATLAB_KEYWORD))
            {
                // Apply logic to the contents of the function parentheses
                nLastPosition += formatFunctionParentheses(i, wxSTC_MATLAB_OPERATOR);
            }
            else if (sParens.find(currentChar) != string::npos)
                continue;
            else if (currentChar == ',' && nextChar != ' ')
                nLastPosition += insertTextAndMove(i + 1, " ");
            else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
            {
                if (nextChar != ' ')
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (prevChar != ' ')
                    nLastPosition += insertTextAndMove(i, " ");
            }
            else if (currentChar == '<' || currentChar == '>' || currentChar == '~' || currentChar == '=')
            {
                static std::string sLeadingChars = " (=+-*/^<>:~";

                if (nextChar != ' ' && nextChar != '=' && nextChar != '>' && currentChar != '~' && !(currentChar == '>' && prevChar == '<'))
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (sLeadingChars.find(prevChar) == string::npos && !(currentChar == '<' && nextChar == '>'))
                    nLastPosition += insertTextAndMove(i, " ");
            }

            nLastPosition += formatOperators(i, prevChar, currentChar, nextChar, false);
        }

        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
        {
            int nPos1 = i;

            while (m_editor->GetStyleAt(i + 1) == wxSTC_MATLAB_KEYWORD)
                i++;

            wxString command = m_editor->GetTextRange(nPos1, i + 1);
            int nCurrentLineStart = m_editor->PositionFromLine(m_editor->LineFromPosition(nPos1));
            int nCurrentLineEnd = m_editor->GetLineEndPosition(m_editor->LineFromPosition(nPos1));

            if (command == "if"
                || command == "elseif"
                || command == "switch"
                || command == "case"
                || command == "for"
                || command == "while")
            {
                if (m_editor->GetCharAt(i + 1) != ' ')
                    nCurrentLineEnd += insertTextAndMove(i + 1, " ");

                int parens = i + 1;

                while (m_editor->GetCharAt(parens) == ' ')
                    parens++;

                if (parens != '(' && parens != '[' && parens != '{')
                    parens = -1;

                if (parens > 0)
                {
                    parens = m_editor->BraceMatch(parens);

                    if (parens > 0)
                    {
                        if (m_editor->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
                            nLastPosition += insertTextAndMove(parens + 1, "\r\n");
                    }
                }
                else
                {
                    parens = m_editor->FindText(i, nCurrentLineEnd, ";");

                    if (parens > 0)
                    {
                        if (m_editor->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
                            nLastPosition += insertTextAndMove(parens + 1, "\r\n");
                    }
                }
            }
            else if (command == "else")
            {
                if (m_editor->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
                    nLastPosition += insertTextAndMove(i + 1, "\r\n");
            }

            if (command == "if"
                || command == "for"
                || command == "else"
                || command == "elseif"
                || command == "switch"
                || command == "case"
                || command == "do"
                || command == "otherwise"
                || command == "try"
                || command == "catch"
                || command == "until"
                || command == "while"
                || command == "end")
            {
                if (m_editor->GetTextRange(nCurrentLineStart, nPos1).find_first_not_of(" \t") != string::npos)
                {
                    nLastPosition += insertTextAndMove(nPos1, "\r\n");
                    i += 2;
                }
            }

            if (command == "if"
                || command == "for"
                || command == "while"
                || command == "try"
                || command == "do"
                || command == "function"
                || command == "switch"
                || command == "case")
            {
                if (nIndentationLevel <= 0)
                {
                    int nLine = m_editor->LineFromPosition(i);
                    int position = m_editor->PositionFromLine(nLine - 1);

                    while (m_editor->GetCharAt(position) == ' ' || m_editor->GetCharAt(position) == '\t')
                        position++;

                    if (nLine
                        && m_editor->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != string::npos
                        && m_editor->GetStyleAt(m_editor->PositionFromLine(nLine - 1)) != wxSTC_MATLAB_COMMENT
                        && m_editor->GetStyleAt(position) != wxSTC_MATLAB_COMMENT)
                    {
                        nLastPosition += insertTextAndMove(m_editor->PositionFromLine(nLine), "\r\n");
                        i += 2;
                    }
                }

                nIndentationLevel++;
            }

            if (command == "end")
            {
                nIndentationLevel--;

                if (nIndentationLevel <= 0)
                {
                    int nLine = m_editor->LineFromPosition(i);

                    if (nLine < m_editor->GetLineCount() - 1
                            && m_editor->GetLine(nLine + 1).find_first_not_of(" \t\r\n") != string::npos)
                        nLastPosition += insertTextAndMove(m_editor->PositionFromLine(nLine + 1), "\r\n");
                }
            }
        }

        if (m_editor->GetStyleAt(i) == wxSTC_MATLAB_STRING)
            nLastPosition += formatStrings(i, false);
    }

    m_editor->EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Applies autoformatting to C++ code.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void CodeFormatter::ApplyAutoFormatCPP(int nFirstLine, int nLastLine)
{
    if (m_editor->getFileType() != FILE_CPP)
        return;

    m_editor->BeginUndoAction();

    int nFirstPosition = m_editor->PositionFromLine(nFirstLine);
    int nLastPosition = m_editor->GetLineEndPosition(nLastLine);
    int nIndentationLevel = -1;

    for (int i = nFirstPosition; i < nLastPosition; i++)
    {
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i)
            || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i))
            continue;

        if (m_editor->GetCharAt(i) == '\r'
            || m_editor->GetCharAt(i) == '\n')
            continue;

        if (m_editor->GetStyleAt(i) == wxSTC_C_OPERATOR)
        {
            int currentChar = m_editor->GetCharAt(i);
            int prevChar = m_editor->GetCharAt(i - 1);
            int nextChar = m_editor->GetCharAt(i + 1);
            int nCurrentLineStart = m_editor->PositionFromLine(m_editor->LineFromPosition(i));
            int nCurrentLineEnd = m_editor->GetLineEndPosition(m_editor->LineFromPosition(i));
            static std::string sParens = "()[]";

            if (currentChar == '-' && nextChar == '>')
            {
                i++;
                continue;
            }

            if (currentChar == '(' && m_editor->GetStyleAt(i - 1) == wxSTC_C_WORD)
                nLastPosition += insertTextAndMove(i, " ");
            else if (currentChar == '('
                     && (m_editor->GetStyleAt(i - 1) == wxSTC_C_WORD2))
            {
                // Apply logic to the contents of the function parentheses
                nLastPosition += formatFunctionParentheses(i, wxSTC_C_OPERATOR);
            }
            else if (sParens.find(currentChar) != string::npos)
                continue;
            else if (currentChar == '{' || currentChar == '}')
            {
                if (currentChar == '{')
                    nIndentationLevel++;

                if (currentChar == '}')
                    nIndentationLevel--;

                if (m_editor->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \t\r\n") != string::npos)
                {
                    nLastPosition += 2;
                    insertTextAndMove(i + 1, "\r\n");
                }

                if (m_editor->GetTextRange(nCurrentLineStart, i).find_first_not_of(" \t") != string::npos)
                {
                    nLastPosition += 2;
                    i += insertTextAndMove(i, "\r\n");
                }
            }
            else if (currentChar == ',' && nextChar != ' ')
                nLastPosition += insertTextAndMove(i + 1, " ");
            else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
            {
                if (nextChar != ' ')
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (prevChar != ' ')
                    nLastPosition += insertTextAndMove(i, " ");
            }
            else if (currentChar == '<' || currentChar == '>' || currentChar == '!' || currentChar == '=')
            {
                static std::string sLeadingChars = " (=+-!*/^<>:";

                if (currentChar == '<'
                    && (m_editor->GetStyleAt(i - 1) == wxSTC_C_WORD || m_editor->GetStyleAt(i - 1) == wxSTC_C_WORD))
                    continue;

                if (currentChar == '>'
                    && (m_editor->GetStyleAt(i - 1) == wxSTC_C_WORD || m_editor->GetStyleAt(i + 1) == wxSTC_C_WORD))
                    continue;

                if (nextChar != ' '
                    && nextChar != '='
                    && nextChar != '>'
                    && currentChar != '!'
                    && !(currentChar == '>' && prevChar == '<'))
                    nLastPosition += insertTextAndMove(i + 1, " ");

                if (sLeadingChars.find(prevChar) == string::npos && !(currentChar == '<' && nextChar == '>'))
                    nLastPosition += insertTextAndMove(i, " ");
            }

            nLastPosition += formatOperators(i, prevChar, currentChar, nextChar, false);
        }

        if (m_editor->GetStyleAt(i) == wxSTC_C_WORD)
        {
            int nPos1 = i;

            while (m_editor->GetStyleAt(i + 1) == wxSTC_C_WORD)
                i++;

            wxString command = m_editor->GetTextRange(nPos1, i + 1);
            int nCurrentLineStart = m_editor->PositionFromLine(m_editor->LineFromPosition(nPos1));
            int nCurrentLineEnd = m_editor->GetLineEndPosition(m_editor->LineFromPosition(nPos1));

            if (command == "if"
                || command == "switch"
                || command == "for"
                || command == "while")
            {
                int parens = i;
                parens = m_editor->FindText(i, nCurrentLineEnd, "(");

                if (parens > 0)
                {
                    parens = m_editor->BraceMatch(parens);

                    if (parens > 0)
                    {
                        int nextVisibleCharAfterParens = m_editor->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t;");

                        if (nextVisibleCharAfterParens != wxNOT_FOUND
                            && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, parens + 1 + nextVisibleCharAfterParens)
                            && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, parens + 1 + nextVisibleCharAfterParens))
                            nLastPosition += insertTextAndMove(parens + 1, "\r\n");
                    }
                }
            }
            else if (command == "else")
            {
                int nextVisibleCharAfterCommand = m_editor->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t");

                if (nextVisibleCharAfterCommand != wxNOT_FOUND
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, i + 1 + nextVisibleCharAfterCommand)
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i + 1 + nextVisibleCharAfterCommand)
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i + 1 + nextVisibleCharAfterCommand))
                    nLastPosition += insertTextAndMove(i + 1, "\r\n");
            }

            if (command == "if"
                || command == "for"
                || command == "while"
                || command == "do"
                || command == "try")
            {
                if (command == "while" && m_editor->GetTextRange(i + 1, nCurrentLineEnd).find(';') != string::npos)
                    continue;

                if (command == "if" && m_editor->GetTextRange(nCurrentLineStart, i).find("else") != string::npos)
                    continue;

                if (nIndentationLevel <= 0)
                {
                    int nLine = m_editor->LineFromPosition(i);
                    int position = m_editor->PositionFromLine(nLine - 1);

                    while (m_editor->GetCharAt(position) == ' ' || m_editor->GetCharAt(position) == '\t')
                        position++;

                    if (nLine
                        && m_editor->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != string::npos
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, m_editor->PositionFromLine(nLine - 1))
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, m_editor->PositionFromLine(nLine - 1))
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, position))
                    {
                        nLastPosition += insertTextAndMove(m_editor->PositionFromLine(nLine), "\r\n");
                        i += 2;
                    }
                }
            }
        }

        if (m_editor->GetStyleAt(i) == wxSTC_C_STRING)
            nLastPosition += formatStrings(i, false);
    }

    m_editor->EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Formats the contents of function
/// parentheses.
///
/// \param pos int&
/// \param operatorStyleNum int
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::formatFunctionParentheses(int& pos, int operatorStyleNum)
{
    int nParens = m_editor->BraceMatch(pos);
    int offset = 0;

    if (nParens > 0)
    {
        for (; pos < nParens; pos++)
        {
            if (m_editor->GetStyleAt(pos) == operatorStyleNum)
            {
                char currentChar = m_editor->GetCharAt(pos);
                char nextChar = m_editor->GetCharAt(pos + 1);
                char prevChar = m_editor->GetCharAt(pos - 1);

                if (currentChar == ',' && nextChar != ' ')
                {
                    offset += insertTextAndMove(pos + 1, " ");
                    nParens++;
                }

                if (currentChar == '=' && nextChar != ' ' && m_editor->GetStyleAt(pos + 1) != operatorStyleNum)
                {
                    offset += insertTextAndMove(pos + 1, " ");
                    nParens++;
                }

                if (currentChar == '=' && prevChar != ' ' && m_editor->GetStyleAt(pos - 1) != operatorStyleNum)
                {
                    offset += insertTextAndMove(pos, " ");
                    nParens++;
                }
            }
        }
    }

    return offset;
}


/////////////////////////////////////////////////
/// \brief Helper to format binary operators
///
/// \param pos int
/// \param prevChar char
/// \param currentChar char
/// \param nextChar char
/// \param isNSCR bool
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::formatOperators(int pos, char prevChar, char currentChar, char nextChar, bool isNSCR)
{
    int offset = 0;

    if (currentChar == '+' || currentChar == '-')
    {
        if (nextChar != ' '
            && nextChar != currentChar
            && nextChar != '='
            && nextChar != ';'
            && prevChar != '('
            && prevChar != '{'
            && prevChar != '['
            && m_editor->GetCharAt(pos - 2) != ','
            && m_editor->GetCharAt(pos - 2) != '='
            && (!isNSCR || (m_editor->GetStyleAt(pos + 1) != wxSTC_NSCR_COMMAND
                            && m_editor->GetStyleAt(pos + 1) != wxSTC_NSCR_OPTION
                            && m_editor->GetStyleAt(pos - 1) != wxSTC_NSCR_COMMAND
                            && m_editor->GetStyleAt(pos - 1) != wxSTC_NSCR_OPTION
                            && nextChar != '>')))
            offset += insertTextAndMove(pos + 1, " ");

        if (prevChar != ' '
            && prevChar != currentChar
            && nextChar != currentChar
            && prevChar != '('
            && prevChar != '['
            && prevChar != '{')
            offset += insertTextAndMove(pos, " ");
    }
    else if (currentChar == '&' || currentChar == '|')
    {
        if (nextChar != ' ' && nextChar != currentChar && (!isNSCR || nextChar != '>'))
            offset += insertTextAndMove(pos + 1, " ");

        if (prevChar != ' ' && prevChar != currentChar)
            offset += insertTextAndMove(pos, " ");
    }
    else if ((currentChar == '*' || currentChar == '/' || currentChar == '^')
             && nextChar == '='
             && prevChar != ' '
             && nextChar != currentChar
             && prevChar != currentChar)
        offset += insertTextAndMove(pos, " ");

    return offset;
}


/////////////////////////////////////////////////
/// \brief Helper to format characters around
/// string literals.
///
/// \param pos int
/// \param isNSCR bool
/// \return int
///
/////////////////////////////////////////////////
int CodeFormatter::formatStrings(int pos, bool isNSCR)
{
    int style = m_editor->GetStyleAt(pos);
    int offset = 0;
    char prevChar = m_editor->GetCharAt(pos - 1);
    char nextChar = m_editor->GetCharAt(pos + 1);

    if (m_editor->GetStyleAt(pos + 1) != style
        && m_editor->GetLineEndPosition(m_editor->LineFromPosition(pos)) != pos + 1
        && nextChar != ' '
        && nextChar != ','
        && nextChar != ';'
        && nextChar != ')'
        && nextChar != ']'
        && nextChar != '}')
        offset += insertTextAndMove(pos + 1, " ");

    if (m_editor->GetStyleAt(pos - 1) != style
        && m_editor->PositionFromLine(m_editor->LineFromPosition(pos)) != pos
        && prevChar != ' '
        && prevChar != '('
        && prevChar != '['
        && prevChar != '{'
        && (!isNSCR || !(prevChar == '=' && m_editor->GetStyleAt(pos - 2) == wxSTC_NSCR_OPTION)))
        offset += insertTextAndMove(pos, " ");

    return offset;
}



