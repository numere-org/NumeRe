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

#ifndef UTF8LIB_HPP
#define UTF8LIB_HPP

#include "stringview.hpp"
#include <vector>

size_t getUtf8ByteLen(char c);
std::string utf8ToAnsi(const std::string& sString);
std::string ansiToUtf8(const std::string& sString);
size_t countUnicodePoints(StringView sString);
size_t findCharStart(StringView sString, size_t pos);
size_t findClosestCharStart(StringView sString, size_t pos);
size_t findNthCharStart(StringView sString, size_t nthChar);
bool isValidUtf8Sequence(const std::string& sString);
std::string ensureValidUtf8(std::string sString);

std::vector<std::string> splitUtf8Chars(StringView sStr);
size_t matchesAny(StringView sString, const std::vector<std::string>& vUtf8Chars, size_t p);

#endif // UTF8LIB_HPP

