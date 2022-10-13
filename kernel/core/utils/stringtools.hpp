/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef STRINGTOOLS_HPP
#define STRINGTOOLS_HPP

#include <string>
#include <complex>
#include <vector>

#include "datetimetools.hpp"

// Forward declaration
class Settings;
class StringView;

enum TIMESTAMP
{
    GET_ONLY_TIME = 0x1,
    GET_AS_TIMESTAMP = 0x2,
    GET_WITH_TEXT = 0x4,
    GET_MILLISECONDS = 0x8,
    GET_FULL_PRECISION = 0x10
};

enum ConvertibleType
{
    CONVTYPE_NONE,
    CONVTYPE_VALUE,
    CONVTYPE_DATE_TIME,
    CONVTYPE_LOGICAL
};

enum TimeDateFormat
{
    TD_NONE = 0x0,
    TD_HHMMSS = 0x1,
    TD_HHMM = 0x2,

    TD_YYMMDD = 0x100,
    TD_DDMMYY = 0x200,
    TD_DDMM = 0x400,

    TD_UTC = 0x10000,

    TD_SEP_COLON = 0x100000,
    TD_SEP_DOT = 0x200000,
    TD_SEP_MINUS = 0x400000,
    TD_SEP_SLASH = 0x800000
};

std::string toString(int nNumber, const Settings& _option);
std::string toString(double dNumber, const Settings& _option);
std::string toString(double dNumber, int nPrecision);
std::string toString(const std::complex<double>& dNumber, int nPrecision);
std::string toString(int);
std::string toString(__time64_t tTime, int timeStampFlags);
std::string toString(sys_time_point tp, int timeStampFlags);
std::string toString(long long int nNumber);
std::string toString(size_t nNumber);
std::string toCmdString(double dNumber);
std::string toCmdString(const std::complex<double>& dNumber);
std::string toString(bool bBoolean);
std::string toHexString(int nNumber);
std::string toString(const std::vector<int>& vVector);
std::vector<int> toVector(std::string sString);
std::string condenseText(const std::string& sText);
std::string truncString(const std::string& sText, size_t nMaxChars);

std::string wcstombs(const std::wstring& wStr);
void StripSpaces(std::string&);

std::string toInternalString(std::string sStr);
std::string toExternalString(std::string sStr);

std::string toLowerCase(const std::string& sUpperCase);
std::string toUpperCase(const std::string& sLowerCase);
int StrToInt(const std::string&);
double StrToDb(const std::string&);
double StrToLogical(const std::string&);
std::complex<double> StrToCmplx(const std::string&);
sys_time_point StrToTime(const std::string&);

bool isConvertible(const std::string& sStr, ConvertibleType type = CONVTYPE_VALUE);
int detectTimeDateFormat(const std::string&);

std::string toSystemCodePage(std::string sOutput);
std::string fromSystemCodePage(std::string sOutput);

void replaceAll(std::string& sToModify, const std::string& sToRep, const std::string& sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
void replaceAll(std::string& sToModify, const char* sToRep, const char* sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
std::string replaceControlCharacters(std::string sToModify);
std::string utf8parser(const std::string& sString);

std::string replacePathSeparator(const std::string& __sPath);
std::string getTimeStamp(bool bGetStamp = true);
std::vector<std::string> split(const std::string& sStr, char cSplit);
std::string ellipsize(const std::string& sLongString, size_t nMaxStringLength = 1024);

bool isEqualStripped(StringView str1, StringView str2);

#endif // STRINGTOOLS_HPP


