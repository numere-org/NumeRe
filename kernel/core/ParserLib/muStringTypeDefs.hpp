/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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


#ifndef MUSTRINGTYPEDEFS_HPP
#define MUSTRINGTYPEDEFS_HPP

#include <string>
#include <sstream>

namespace mu
{
    /** \brief The stringtype used by the parser.

      Depends on wether UNICODE is used or not.
    */
    typedef std::string string_type;

    /** \brief The character type used by the parser.

      Depends on wether UNICODE is used or not.
    */
    typedef string_type::value_type char_type;

    /** \brief Typedef for easily using stringstream that respect the parser stringtype. */
    typedef std::basic_stringstream<char_type,
            std::char_traits<char_type>,
            std::allocator<char_type> > stringstream_type;
}

#endif // MUSTRINGTYPEDEFS_HPP

