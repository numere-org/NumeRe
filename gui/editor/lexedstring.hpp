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

#ifndef LEXEDSTRING_HPP
#define LEXEDSTRING_HPP

#include <string>
#include <vector>


/////////////////////////////////////////////////
/// \brief This struct represents a single lexed
/// string token together with its style.
/////////////////////////////////////////////////
struct LexedString
{
    std::string m_str;
    int m_style;


    /////////////////////////////////////////////////
    /// \brief Equality comparison operator.
    ///
    /// \param lexString const LexedString&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool operator==(const LexedString& lexString) const
    {
        return lexString.m_style == m_style && lexString.m_str == m_str;
    }

    bool is(int style) const;
    bool is(const std::vector<int>& styleList) const;
    bool isAssignmentOperator() const;
};




/////////////////////////////////////////////////
/// \brief This class represents a single lexed
/// line composed out of multiple string tokens
/// with their styles. It adds a safety
/// functionality to return always a lexed string
/// reference although the code may read beyond
/// the available tokens.
/////////////////////////////////////////////////
class LexedLine
{
    private:
        std::vector<LexedString> m_line;
        static const LexedString BUFFER;

    public:
        /////////////////////////////////////////////////
        /// \brief Construct an instance from a vector of
        /// LexedString items.
        ///
        /// \param line const std::vector<LexedString>
        ///
        /////////////////////////////////////////////////
        LexedLine(const std::vector<LexedString>& line) : m_line(line)
        { }

        const LexedString& operator[](size_t i) const;

        /////////////////////////////////////////////////
        /// \brief Returns the number of tokens in this
        /// instance.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t size() const
        {
            return m_line.size();
        }

        std::vector<LexedString>::const_iterator begin() const;
        std::vector<LexedString>::const_iterator end() const;
        std::vector<LexedString>::const_iterator find(const LexedString& token) const;

        void advanceToClosingParens(size_t& pos, std::string openP, std::string closeP) const;
        void advanceToNextExpression(size_t& pos) const;

        std::string dump() const;
};






#endif // LEXEDSTRING_HPP


