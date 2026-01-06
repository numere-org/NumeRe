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


#ifndef STRINGCONV_HPP
#define STRINGCONV_HPP

#include <wx/string.h>
#include <string>

/////////////////////////////////////////////////
/// \brief Convert a wxString to a UTF8-encoded
/// std::string.
///
/// \param s const wxString&
/// \return std::string
///
/////////////////////////////////////////////////
inline std::string wxToUtf8(const wxString& s)
{
    return std::string(s.ToUTF8().data());
}


/////////////////////////////////////////////////
/// \brief Convert a UTF-encoded std::string to a
/// wxString.
///
/// \param s const std::string&
/// \return wxString
///
/////////////////////////////////////////////////
inline wxString wxFromUtf8(const std::string& s)
{
    return wxString::FromUTF8(s.c_str(), s.length());
}

#endif // STRINGCONV_HPP


