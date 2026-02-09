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

#ifndef STRINGVIEW_HPP
#define STRINGVIEW_HPP

#include <string>
#include <cstring>
#include <stdexcept>

/////////////////////////////////////////////////
/// \brief This class is a base class for all
/// string view classes.
///
/// It gathers the common const
/// operations like finding and equality
/// operators. It can be instantated directly,
/// but it will do nothing due to a missing
/// pointer to the viewed std::string.
///
/// \note String view classes are neither thread
/// safe nor do they update, when the data source
/// has been altered in other locations. They can
/// only be used in single-thread contexts and
/// should be considered as immutable. The
/// MutableStringView class can handle some
/// modifications but will probably invalidate
/// all other string views using the same data
/// source while modifying the data.
/////////////////////////////////////////////////
class StringViewBase
{
    protected:
        size_t m_start;
        size_t m_len;

        /////////////////////////////////////////////////
        /// \brief This private member function
        /// evaluates, whether the passed length is part
        /// of the viewed section and adapts the length
        /// correspondingly.
        ///
        /// \param pos size_t starting position
        /// \param len size_t
        /// \return size_t the new length
        ///
        /////////////////////////////////////////////////
        inline size_t validizeLength(size_t pos, size_t len) const
        {
            if (len == std::string::npos || pos+len > m_len)
                len = m_len - pos;

            return len;
        }

        /////////////////////////////////////////////////
        /// \brief This member function checks, whether
        /// the passed (absolute) position is part of the
        /// viewed string section. Is mostly used in
        /// string find operations.
        ///
        /// \param pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool validAbsolutePosition(size_t pos) const
        {
            return pos >= m_start && pos < m_start + m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Private helper function to determine,
        /// whether a character is a parser delimiter.
        ///
        /// \param c char
        /// \return bool
        ///
        /////////////////////////////////////////////////
        static bool is_parser_delim(char c)
        {
            // Characters converted to a single logical expression
            return c >= 32 && c <= 126 && c != 36 && c != 46
                && (c < 48 || c > 57)
                && (c < 64 || c > 90)
                && (c < 95 || c > 122);
        }

        /////////////////////////////////////////////////
        /// \brief Private helper function to determine,
        /// whether a character is a delimiter.
        ///
        /// \param c char
        /// \return bool
        ///
        /////////////////////////////////////////////////
        static bool is_std_delim(char c)
        {
            // Characters converted to a single logical expression
            return c >= 32 && c <= 125 && c != 36 && c != 46
                && (c < 48 || c > 57)
                && (c < 64 || c > 90)
                && (c < 95 || c > 122);

            // Should be identical to:
            //static const string sDELIMITER = "+-*/ ()={}^&|!<>,\\%#[]?:\";";
        }

    public:

        /////////////////////////////////////////////////
        /// \brief Defines the delimiter types.
        /////////////////////////////////////////////////
        enum DelimiterType
        {
            STD_DELIMITER,
            PARSER_DELIMITER,
            STRING_DELIMITER,
            STRVAR_DELIMITER
        };

        /////////////////////////////////////////////////
        /// \brief StringViewBase default constructor.
        /////////////////////////////////////////////////
        StringViewBase() : m_start(0), m_len(0) {}

        /////////////////////////////////////////////////
        /// \brief Reset function.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear()
        {
            m_start = 0;
            m_len = 0;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// pointer to the viewed string. Is only used
        /// internally.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const
        {
            return nullptr;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the equality operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator==(const StringViewBase& view) const
        {
            const std::string* thisString = getData();
            const std::string* viewString = view.getData();

            if (thisString && viewString)
                return thisString->compare(m_start, m_len, *viewString, view.m_start, view.m_len) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the equality operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator==(const std::string& sString) const
        {
            const std::string* thisString = getData();

            if (thisString)
                return thisString->compare(m_start, m_len, sString) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the equality operator using a const char*.
        ///
        /// \param sString const char*
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator==(const char* sString) const
        {
            const std::string* thisString = getData();

            if (thisString)
                return thisString->compare(m_start, m_len, sString) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the inequality operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator!=(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) != 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the inequality operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator!=(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) != 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the inequality operator using a const
        /// char*.
        ///
        /// \param sString const char*
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator!=(const char* sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) != 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) < 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) < 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less-equal operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<=(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) <= 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less-equal operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<=(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) <= 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) > 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) > 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater-equal operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>=(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) >= 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater-equal operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>=(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) >= 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the concatenation operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string operator+(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return std::string(begin(), end()).append(view.begin(), view.end());

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the concatenation operator using a const
        /// std::string instance.
        ///
        /// \param view const StringViewBase&
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string operator+(const std::string& sString) const
        {
            if (getData())
                return std::string(begin(), end()).append(sString);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides a const
        /// char reference to the first character in the
        /// viewed section.
        ///
        /// \return const char&
        ///
        /////////////////////////////////////////////////
        inline const char& front() const
        {
            if (getData())
                return getData()->at(m_start);

            throw std::out_of_range("StringView::front");
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides a const
        /// char reference to the last character in the
        /// viewed section.
        ///
        /// \return const char&
        ///
        /////////////////////////////////////////////////
        inline const char& back() const
        {
            if (getData())
                return getData()->at(m_start+m_len-1);

            throw std::out_of_range("StringView::back");
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides an
        /// iterator to the beginning of the viewed
        /// section of the internal string.
        ///
        /// \return std::string::const_iterator
        ///
        /////////////////////////////////////////////////
        inline std::string::const_iterator begin() const
        {
            if (getData())
                return getData()->begin() + m_start;

            return std::string::const_iterator();
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides an
        /// iterator to the end of the viewed section of
        /// the internal string.
        ///
        /// \return std::string::const_iterator
        ///
        /////////////////////////////////////////////////
        inline std::string::const_iterator end() const
        {
            if (getData())
                return getData()->begin() + m_start + m_len;

            return std::string::const_iterator();
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string has
        /// the same character sequence as the passed
        /// string starting from the selected position.
        ///
        /// \param other const StringViewBase&
        /// \param pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool match(const StringViewBase& other, size_t pos = 0) const
        {
            const std::string* thisString = getData();
            const std::string* viewString = other.getData();

            if (thisString && viewString)
                return thisString->compare(m_start+pos, std::min(other.m_len, m_len-pos),
                                           *viewString, other.m_start, other.m_len) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string has
        /// the same character sequence as the passed
        /// string starting from the selected position.
        ///
        /// \param other const std::string&
        /// \param pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool match(const std::string& other, size_t pos = 0) const
        {
            const std::string* thisString = getData();

            if (thisString)
                return thisString->compare(m_start+pos, std::min(other.length(), m_len-pos), other) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string has
        /// the same character sequence as the passed
        /// string starting from the selected position.
        ///
        /// \param other const char*
        /// \param pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool match(const char* other, size_t pos = 0) const
        {
            const std::string* thisString = getData();

            if (thisString)
                return thisString->compare(m_start+pos, std::min(std::strlen(other), m_len-pos), other) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string
        /// starts with the passed string.
        ///
        /// \param other const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool starts_with(const StringViewBase& other) const
        {
            const std::string* thisString = getData();
            const std::string* viewString = other.getData();

            if (thisString && viewString)
                return thisString->compare(m_start, std::min(other.m_len, m_len),
                                           *viewString, other.m_start, other.m_len) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string
        /// starts with the passed string.
        ///
        /// \param other const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool starts_with(const std::string& other) const
        {
            const std::string* thisString = getData();

            if (thisString)
                return thisString->compare(m_start, std::min(other.length(), m_len), other) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string
        /// ends with the passed string.
        ///
        /// \param other const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool ends_with(const StringViewBase& other) const
        {
            const std::string* thisString = getData();
            const std::string* viewString = other.getData();

            if (thisString && viewString)
            {
                if (m_len < other.m_len)
                    return false;

                return thisString->compare(m_start+m_len-other.m_len, other.m_len,
                                           *viewString, other.m_start, other.m_len) == 0;
            }

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the viewed string
        /// ends with the passed string.
        ///
        /// \param other const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool ends_with(const std::string& other) const
        {
            const std::string* thisString = getData();

            if (thisString)
            {
                if (m_len < other.length())
                    return false;

                return thisString->compare(m_start+m_len-other.length(), other.length(), other) == 0;
            }

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// remove characters from the front of the
        /// viewed section.
        ///
        /// \param len size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void trim_front(size_t len)
        {
            if (len < m_len)
            {
                m_start += len;
                m_len -= len;
            }
            else
                clear();
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// remove characters from the back of the
        /// viewed section.
        ///
        /// \param len size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void trim_back(size_t len)
        {
            if (len < m_len)
                m_len -= len;
            else
                clear();
        }

        /////////////////////////////////////////////////
        /// \brief Remove all characters after the
        /// selected position from the view.
        ///
        /// \param pos size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void remove_from(size_t pos)
        {
            if (pos > 0 && getData())
                m_len = pos;
            else
                clear();
        }

        /////////////////////////////////////////////////
        /// \brief This member function shrinks the
        /// viewed section to remove all leading or
        /// trailing whitespace characters. This is the
        /// corresponding member function to
        /// StripSpaces(std::string&).
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void strip()
        {
            const std::string* data = getData();

            if (!data)
                return;

            // Strip leading
            while (m_len && isblank(data->operator[](m_start)))
            {
                m_start++;
                m_len--;
            }

            if (!m_len)
            {
                clear();
                return;
            }

            // Strip trailing
            while (m_len && isblank(data->operator[](m_start+m_len-1)))
            {
                m_len--;
            }
        }

        /////////////////////////////////////////////////
        /// \brief This member function simply returns
        /// the length of the viewed section.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        inline size_t length() const
        {
            return getData() ? m_len : 0;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a copy of
        /// the viewed section of the string
        /// (via std::string::substr). Note that this is
        /// an inefficient operation.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string to_string() const
        {
            if (getData())
                return getData()->substr(m_start, m_len);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the complete viewed string
        /// from its original source.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string get_viewed_string() const
        {
            if (getData())
                return *getData();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the offset between the
        /// complete viewed string the current string
        /// view.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        inline size_t get_offset() const
        {
            return m_start;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find(char c, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::rfind()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rfind(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->rfind(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::rfind()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rfind(char c, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->rfind(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_of(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_of()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_of(char c, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_of(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_not_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_not_of(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_not_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_not_of()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_not_of(char c, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_not_of(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_of(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_of()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_of(char c, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_of(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_not_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_not_of(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_not_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_not_of()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_not_of(char c, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_not_of(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Checks, whether the selected sequence
        /// of characters in the view is delimited by the
        /// selected set of delimiters.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param type DelimiterType
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool is_delimited_sequence(size_t pos = 0, size_t len = std::string::npos, DelimiterType type = STD_DELIMITER) const
        {
            const std::string* data = getData();

            if (data)
            {
                len = validizeLength(pos, len);
                size_t prevChar = m_start+pos-1;
                size_t nextChar = m_start+pos+len;

                switch (type)
                {
                    case STD_DELIMITER:
                        return (!pos || is_std_delim(data->operator[](prevChar)))
                            && (len+pos == m_len || is_std_delim(data->operator[](nextChar)));

                    case STRING_DELIMITER:
                        return (!pos || is_std_delim(data->operator[](prevChar)))
                            && (len+pos == m_len || is_std_delim(data->operator[](nextChar)) || data->operator[](nextChar) == '.');

                    case STRVAR_DELIMITER:
                        return (!pos || is_std_delim(data->operator[](prevChar)))
                            && (len+pos == m_len || (is_std_delim(data->operator[](nextChar))
                                                 && data->operator[](nextChar) != '(') || data->operator[](nextChar) == '.');

                    case PARSER_DELIMITER:
                        return (!pos || is_parser_delim(data->operator[](prevChar)))
                            && (len+pos == m_len || is_parser_delim(data->operator[](nextChar)));
                }
            }

            return false;
        }
};


/////////////////////////////////////////////////
/// \brief Inverse concatenation operator for a
/// string instance with a StringViewBase
/// instance.
///
/// \param sString const std::string&
/// \param view const StringViewBase&
/// \return std::string
///
/////////////////////////////////////////////////
inline std::string operator+(const std::string& sString, const StringViewBase& view)
{
    return sString + view.to_string();
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the equality operator using a const
/// std::string instance.
///
/// \param sString const std::string&
/// \param view const StringViewBase&
/// \return bool
///
/////////////////////////////////////////////////
inline bool operator==(const std::string& sString, const StringViewBase& view)
{
    return view == sString;
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the equality operator using a const char*.
///
/// \param sString const char*
/// \param view const StringViewBase&
/// \return bool
///
/////////////////////////////////////////////////
inline bool operator==(const char* sString, const StringViewBase& view)
{
    return view == sString;
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the inequality operator using a const
/// std::string instance.
///
/// \param sString const std::string&
/// \param view const StringViewBase&
/// \return bool
///
/////////////////////////////////////////////////
inline bool operator!=(const std::string& sString, const StringViewBase& view)
{
    return view != sString;
}


/////////////////////////////////////////////////
/// \brief This member function is an overload
/// for the inequality operator using a const
/// char*.
///
/// \param sString const char*
/// \param view const StringViewBase&
/// \return bool
///
/////////////////////////////////////////////////
inline bool operator!=(const char* sString, const StringViewBase& view)
{
    return view != sString;
}




// Forward declaration for friendship
class StringView;


/////////////////////////////////////////////////
/// \brief This class is a mutable version of a
/// string view. It can be used to replace single
/// characters or entire parts of the string.
/// It's possible to convert a MutableStringView
/// into a (const) StringView but not the other
/// way around.
/////////////////////////////////////////////////
class MutableStringView : public StringViewBase
{
    private:
        friend class StringView;
        std::string* m_data;

        /////////////////////////////////////////////////
        /// \brief Private constructor used by the
        /// subview member function.
        ///
        /// \param data std::string*
        /// \param start size_t
        /// \param len size_t
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string* data, size_t start, size_t len) : StringViewBase(), m_data(data)
        {
            m_start = start;
            m_len = len;
        }

    protected:
        /////////////////////////////////////////////////
        /// \brief Override to return a pointer to the
        /// internal string.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const override
        {
            return m_data;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from
        /// another MutableStringView instance.
        ///
        /// \param view MutableStringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(MutableStringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// std::string pointer.
        ///
        /// \param data std::string*
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(std::string* data)
        {
            if (!data)
                return;

            m_data = data;
            m_start = 0;
            m_len = m_data->length();
        }

    public:
        /////////////////////////////////////////////////
        /// \brief MutableStringView default constructor.
        /////////////////////////////////////////////////
        MutableStringView() : StringViewBase(), m_data(nullptr) {}

        /////////////////////////////////////////////////
        /// \brief MutableStringView constructor from a
        /// std::string pointer.
        ///
        /// \param data std::string*
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string* data) : MutableStringView()
        {
            assign(data);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView constructor from a
        /// (non-const) std::string reference.
        ///
        /// \param data std::string&
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string& data) : MutableStringView()
        {
            assign(&data);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView copy constrcutor.
        ///
        /// \param view MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView(MutableStringView& view) : MutableStringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView move constructor.
        ///
        /// \param view MutableStringView&&
        ///
        /////////////////////////////////////////////////
        MutableStringView(MutableStringView&& view)
        {
            m_data = std::move(view.m_data);
            m_start = std::move(view.m_start);
            m_len = std::move(view.m_len);
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for another
        /// MutableStringView instance.
        ///
        /// \param view MutableStringView
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(MutableStringView view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a std::string
        /// pointer.
        ///
        /// \param data std::string*
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(std::string* data)
        {
            assign(data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a std::string
        /// (non-const) reference.
        ///
        /// \param data std::string&
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(std::string& data)
        {
            assign(&data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Override to clear the internal pointer
        /// as well.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear() override
        {
            m_data = nullptr;
            m_start = 0;
            m_len = 0;
        }

        /////////////////////////////////////////////////
        /// \brief Implementation of the random access
        /// operator. Returns a (non-const) char
        /// reference.
        ///
        /// \param pos size_t
        /// \return char&
        /// \remark No boundary checks are performed.
        ///
        /////////////////////////////////////////////////
        inline char& operator[](size_t pos)
        {
            return m_data->operator[](m_start+pos);
        }

        /////////////////////////////////////////////////
        /// \brief This member function creates a new
        /// MutableStringView class instance using the
        /// selected position and length as a new viewed
        /// part. The position has to be part of the
        /// viewed section of this instance. This
        /// function can be used to replace
        /// std::string::substr.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \return MutableStringView
        ///
        /////////////////////////////////////////////////
        MutableStringView subview(size_t pos = 0, size_t len = std::string::npos) const
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                return MutableStringView(m_data, m_start+pos, len);
            }

            return MutableStringView();
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const std::string&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const std::string& s)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                m_data->replace(m_start+pos, len, s);
                m_len += s.length() - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string. This function allows to select a
        /// smaller part of the replacing string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const std::string&
        /// \param subpos size_t
        /// \param sublen size_t
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const std::string& s, size_t subpos, size_t sublen)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);

                if (subpos + sublen > s.length())
                    sublen = s.length() - subpos;

                m_data->replace(m_start+pos, len, s, subpos, sublen);
                m_len += sublen - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const char*
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const char* s)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                m_data->replace(m_start+pos, len, s);
                m_len += std::strlen(s) - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string. This function allows to select a
        /// smaller part of the replacing string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const char*
        /// \param subpos size_t
        /// \param sublen size_t
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const char* s, size_t subpos, size_t sublen)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);

                if (subpos + sublen > std::strlen(s))
                    sublen = std::strlen(s) - subpos;

                m_data->replace(m_start+pos, len, s, subpos, sublen);
                m_len += sublen - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// StringViewBase.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param view const StringViewBase&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const StringViewBase& view)
        {
            return replace(pos, len, view.to_string());
        }

        /////////////////////////////////////////////////
        /// \brief This member function inserts a string
        /// at the selected position.
        ///
        /// \param pos size_t
        /// \param s const std::string&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& insert(size_t pos, const std::string& s)
        {
            if (m_data && pos < m_len)
            {
                m_data->insert(m_start+pos, s);
                m_len += s.length();
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function inserts a string
        /// at the selected position.
        ///
        /// \param pos size_t
        /// \param s const char*
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& insert(size_t pos, const char* s)
        {
            if (m_data && pos < m_len)
            {
                m_data->insert(m_start+pos, s);
                m_len += std::strlen(s);
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function inserts a passed
        /// StringViewBase at the selected position.
        ///
        /// \param pos size_t
        /// \param view const StringViewBase&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& insert(size_t pos, const StringViewBase& view)
        {
            return insert(pos, view.to_string());
        }

};


/////////////////////////////////////////////////
/// \brief This class is the immutable (const)
/// version of a string view. It can be
/// constructed from a MutableStringView, but
/// cannot be used to construct a mutable
/// version.
/////////////////////////////////////////////////
class StringView : public StringViewBase
{
    private:
        const std::string* m_data;

        /////////////////////////////////////////////////
        /// \brief Private constructor used by the
        /// subview member function.
        ///
        /// \param data std::string* const
        /// \param start size_t
        /// \param len size_t
        ///
        /////////////////////////////////////////////////
        StringView(const std::string* data, size_t start, size_t len) : StringViewBase(), m_data(data)
        {
            m_start = start;
            m_len = len;
        }

    protected:
        /////////////////////////////////////////////////
        /// \brief Override to return a pointer to the
        /// internal string.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const override
        {
            return m_data;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from
        /// another StringView instance.
        ///
        /// \param view const StringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const StringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// MutableStringView instance.
        ///
        /// \param view const MutableStringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const MutableStringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// const std::string pointer.
        ///
        /// \param data const std::string*
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const std::string* data)
        {
            if (!data)
                return;

            m_data = data;
            m_start = 0;
            m_len = m_data->length();
        }

    public:
        /////////////////////////////////////////////////
        /// \brief StringView default constructor.
        /////////////////////////////////////////////////
        StringView() : StringViewBase(), m_data(nullptr) {}

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a const
        /// std::string pointer.
        ///
        /// \param data const std::string*
        ///
        /////////////////////////////////////////////////
        StringView(const std::string* data) : StringView()
        {
            assign(data);
        }

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a const
        /// std::string reference.
        ///
        /// \param data const std::string&
        ///
        /////////////////////////////////////////////////
        StringView(const std::string& data) : StringView()
        {
            assign(&data);
        }

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a const
        /// std::string reference, a start and a length
        ///
        /// \param data const std::string&
        /// \param start size_t
        /// \param len size_t
        ///
        /////////////////////////////////////////////////
        StringView(const std::string& data, size_t start, size_t len = std::string::npos) : StringView()
        {
            assign(&data);

            if (m_data && start < m_len)
            {
                len = validizeLength(start, len);
                m_start = start;
                m_len = len;
            }
        }

        /////////////////////////////////////////////////
        /// \brief StringView copy constructor.
        ///
        /// \param view const StringView&
        ///
        /////////////////////////////////////////////////
        StringView(const StringView& view) : StringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a
        /// MutableStringView class instance.
        ///
        /// \param view const MutableStringView&
        ///
        /////////////////////////////////////////////////
        StringView(const MutableStringView& view) : StringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief StringView move constructor.
        ///
        /// \param view StringView&&
        ///
        /////////////////////////////////////////////////
        StringView(StringView&& view)
        {
            m_data = std::move(view.m_data);
            m_start = std::move(view.m_start);
            m_len = std::move(view.m_len);
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for another
        /// StringView instance.
        ///
        /// \param view const StringView&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const StringView& view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a
        /// MutableStringView class instance.
        ///
        /// \param view const MutableStringView&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const MutableStringView& view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a const
        /// std::string pointer.
        ///
        /// \param data const std::string*
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const std::string* data)
        {
            assign(data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a const
        /// std::string reference.
        ///
        /// \param data const std::string&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const std::string& data)
        {
            assign(&data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Override to clear the internal pointer
        /// as well.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear() override
        {
            m_data = nullptr;
            m_start = 0;
            m_len = 0;
        }

        /////////////////////////////////////////////////
        /// \brief Random access operator, returning a
        /// const char reference.
        ///
        /// \param pos size_t
        /// \return const char&
        /// \remark No boundary checks are performed.
        ///
        /////////////////////////////////////////////////
        inline const char& operator[](size_t pos) const
        {
            return m_data->operator[](m_start+pos);
        }

        /////////////////////////////////////////////////
        /// \brief This member function creates a new
        /// StringView class instance using the selected
        /// position and length as a new viewed part. The
        /// position has to be part of the viewed section
        /// of this instance. This function can be used
        /// to replace std::string::substr.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \return StringView
        ///
        /////////////////////////////////////////////////
        StringView subview(size_t pos = 0, size_t len = std::string::npos) const
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                return StringView(m_data, m_start+pos, len);
            }

            return StringView();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// MutableStringView instance with the data of
        /// this instance.
        ///
        /// \return MutableStringView
        /// \warning Only use this method, if you know,
        /// what you're doing.
        ///
        /////////////////////////////////////////////////
        MutableStringView make_mutable() const
        {
            if (m_data)
                return MutableStringView(const_cast<std::string*>(m_data), m_start, m_len);

            return MutableStringView();
        }

};




#endif // STRINGVIEW_HPP


