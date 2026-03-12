/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#include "lexedstring.hpp"
#include <wx/stc/stc.h>
#include "../../kernel/core/io/logger.hpp"

const LexedString LexedLine::BUFFER{"", 0};


/////////////////////////////////////////////////
/// \brief Check, if this token has the passed
/// style.
///
/// \param style int
/// \return bool
///
/////////////////////////////////////////////////
bool LexedString::is(int style) const
{
    return style == m_style;
}


/////////////////////////////////////////////////
/// \brief Check, if this token has one of the
/// passed styles.
/// \overload
///
/// \param styleList const std::vector<int>&
/// \return bool
///
/////////////////////////////////////////////////
bool LexedString::is(const std::vector<int>& styleList) const
{
    for (int style : styleList)
    {
        if (style == m_style)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Returns true, if this token represents
/// an assignment operator.
///
/// \return bool
///
/////////////////////////////////////////////////
bool LexedString::isAssignmentOperator() const
{
    return m_style == wxSTC_NSCR_OPERATORS
        && (m_str == "="
            || m_str == "+="
            || m_str == "-="
            || m_str == "*="
            || m_str == "**="
            || m_str == "/="
            || m_str == "^="
            || m_str == "->"
            || m_str == ":=");
}





/////////////////////////////////////////////////
/// \brief Return a constant reference to the
/// i-th token in this line. Will always return a
/// valid instance, even of accessed beyond the
/// number of available tokens.
///
/// \param i size_t
/// \return const LexedString&
///
/////////////////////////////////////////////////
const LexedString& LexedLine::operator[](size_t i) const
{
    if (i >= m_line.size())
    {
        g_logger.warning("Accessed out-of-bound-element " + std::to_string(i) + " in line:");
        g_logger.warning(dump());
        return BUFFER;
    }

    return m_line[i];
}


/////////////////////////////////////////////////
/// \brief Return a const iterator to the
/// beginning of the line.
///
/// \return std::vector<LexedString>::const_iterator
///
/////////////////////////////////////////////////
std::vector<LexedString>::const_iterator LexedLine::begin() const
{
    return m_line.begin();
}


/////////////////////////////////////////////////
/// \brief Return a const iterator to the end of
/// the line.
///
/// \return std::vector<LexedString>::const_iterator
///
/////////////////////////////////////////////////
std::vector<LexedString>::const_iterator LexedLine::end() const
{
    return m_line.end();
}


/////////////////////////////////////////////////
/// \brief Find a specified token in this line.
/// Will return a const iterator to its position,
/// if found, or LexedLine::end(), if nothing has
/// been found.
///
/// \param token const LexedString&
/// \return std::vector<LexedString>::const_iterator
///
/////////////////////////////////////////////////
std::vector<LexedString>::const_iterator LexedLine::find(const LexedString& token) const
{
    return std::find(m_line.begin(), m_line.end(), token);
}


/////////////////////////////////////////////////
/// \brief Advance the passed position to the
/// end of the selected parenthesis pair.
///
/// \param pos size_t&
/// \param openP std::string
/// \param closeP std::string
/// \return void
///
/////////////////////////////////////////////////
void LexedLine::advanceToClosingParens(size_t& pos, std::string openP, std::string closeP) const
{
    int p = 1;

    for (; pos < size(); pos++)
    {
        if (!m_line[pos].is(wxSTC_NSCR_OPERATORS))
            continue;

        if (m_line[pos].m_str == openP)
            p++;

        if (m_line[pos].m_str == closeP)
            p--;

        if (!p)
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Advance the passed position to the
/// next expression part starting at a free comma.
///
/// \param pos size_t&
/// \return void
///
/////////////////////////////////////////////////
void LexedLine::advanceToNextExpression(size_t& pos) const
{
    for (; pos < size(); pos++)
    {
        if (!m_line[pos].is(wxSTC_NSCR_OPERATORS))
            continue;

        if (m_line[pos].m_str == ",")
            return;
        else if (m_line[pos].m_str == "(")
        {
            pos++;
            advanceToClosingParens(pos, "(", ")");
        }
        else if (m_line[pos].m_str == "[")
        {
            pos++;
            advanceToClosingParens(pos, "[", "]");
        }
        else if (m_line[pos].m_str == "{")
        {
            pos++;
            advanceToClosingParens(pos, "{", "}");
        }
    }
}


/////////////////////////////////////////////////
/// \brief Dump this instance into a string ready
/// to be printed.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string LexedLine::dump() const
{
    std::string dump;

    for (const auto& tk : m_line)
    {
//        dump += "<`" + tk.m_str + "`," + std::to_string(tk.m_style) + ">";
        dump += "<`" + tk.m_str + "`>";
    }

    return dump;
}




