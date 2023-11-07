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
#include <map>

#include "datetimetools.hpp"

// Forward declaration
class Settings;
class StringView;
class MutableStringView;

/////////////////////////////////////////////////
/// \brief Structure containing the german umlauts. The
/// lower field will contain lower case umlauts,
/// upper field contains the upper case umlauts.
/////////////////////////////////////////////////
struct Umlauts
{
    std::string lower;
    std::string upper;

    // Constructor fills the fields with the corresponding
    // character codes (eg \x94 is a Hex value for (char)148)
    Umlauts() : lower("\xE4\xF6\xFC\x84\x94\x81\xDF\xB0\xB5\xE1\xA7\xE6"), upper("\xC4\xD6\xDC\x8E\x99\x9A") {}

    /////////////////////////////////////////////////
    /// \brief Determine, whether this character is
    /// an umlaut.
    ///
    /// \param c char
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isUmlaut(char c)
    {
        return lower.find(c) != std::string::npos || upper.find(c) != std::string::npos;
    }

    /////////////////////////////////////////////////
    /// \brief Convert an umlaut to uppercase or
    /// return the same character.
    ///
    /// \param c char
    /// \return char
    ///
    /////////////////////////////////////////////////
    char toUpper(char c)
    {
        size_t p = lower.find(c);

        if (p < upper.length())
            return upper[p];

        return c;
    }

    /////////////////////////////////////////////////
    /// \brief Convert an umlaut to lowercase or
    /// return the same character.
    ///
    /// \param c char
    /// \return char
    ///
    /////////////////////////////////////////////////
    char toLower(char c)
    {
        size_t p = upper.find(c);

        if (p < lower.length())
            return lower[p];

        return c;
    }
};

enum TIMESTAMP
{
    GET_ONLY_TIME = 0x1,
    GET_AS_TIMESTAMP = 0x2,
    GET_WITH_TEXT = 0x4,
    GET_MILLISECONDS = 0x8,
    GET_FULL_PRECISION = 0x10,
    GET_UNBIASED_TIME = 0x20
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

enum NumberFormat
{
    NUM_NONE = 0x0,

    NUM_DECIMAL_EU = 0x1,  // '.' as thousand seperator & ',' as decimal seperator
    NUM_DECIMAL_US = 0x2,   // ',' as thousand seperator & '.' as decimal seperator

    NUM_K_SPACE = 0x100,
    NUM_K_EU = 0x200,
    NUM_K_US = 0x400,

    NUM_INVALID = 0x1000,

    NUM_FORMAT_COUNT = 6
};

struct NumberFormatsVoter
{ // Preferring ANSI brace format ;-)
    // necessary attributes here
    int num_format_votes[NUM_FORMAT_COUNT];

    std::vector<char> sep;
    int m_last_idx = -1;
    std::string m_tape;
    int curr_format = 0;

    std::map<std::string, int> num_format_lookup =
        {{".", NUM_DECIMAL_US}, {",", NUM_DECIMAL_EU},   // CASE 5: Starting Sepetator -> must be DECIMAL  TODO check only last also
         {">.", NUM_DECIMAL_US}, {">,", NUM_DECIMAL_EU}, // CASE 4: one ceperator and left is > 3
         {".>", NUM_DECIMAL_US}, {",>", NUM_DECIMAL_EU}, {".<", NUM_DECIMAL_US}, {",<", NUM_DECIMAL_EU},  // CASE 2: one ceperator and right is != 3
         {".=.", NUM_K_EU}, {",=,", NUM_K_US}, {" = ", NUM_K_SPACE},  //CASE 3 same seperator more than once, must be thousands
         {",=.", NUM_K_US | NUM_DECIMAL_US}, {" =.", NUM_K_SPACE | NUM_DECIMAL_US}, {".=,", NUM_K_EU | NUM_DECIMAL_EU}, {" =,", NUM_K_SPACE | NUM_DECIMAL_EU} // CASE 1 two diff seperators, sec should be decimal, first thousands
         };

    NumberFormatsVoter() {
        for(size_t i = 0; i < NUM_FORMAT_COUNT; i++)
            num_format_votes[i] = 0;
    }

    int checkNumFormat(std::string key){
        auto elem = num_format_lookup.find(key);

        if(elem != num_format_lookup.end())
            return num_format_lookup[key];

        return 0;
    }

    void pushInbetween(int idx){
        int digs_inbetween = idx - m_last_idx;
        if(digs_inbetween > 0) {
            if(digs_inbetween < 3)
                m_tape.push_back('<');
            else if(digs_inbetween > 3)
                m_tape.push_back('>');
            else
                m_tape.push_back('=');
        }
    }

    void startParseNumber(int idx) {
        m_last_idx = idx-1; //todo wie handeln wir 1ste und letzte, da diese nicht seperatoren sind
        m_tape = "";
        curr_format = 0;
    }

    void addSeperator(char sep, int idx) {
        pushInbetween(idx-1);
        m_tape.push_back(sep);
        m_last_idx = idx;

        // max 3 symbols at ones in tape
        while(m_tape.length() > 3)
            m_tape.erase(m_tape.begin());

        curr_format |= checkNumFormat(m_tape);
    }

    int endParseNumber(int idx) {

        // we only want to check for the last 2/1
        m_tape = m_tape.back();

        pushInbetween(idx); //since normally the function is called when seperator appears
        curr_format |= checkNumFormat(m_tape);

        m_last_idx = idx;
        m_tape = "";
        return curr_format;
    }

    void vote(int numType)
    {
        if((numType & NUM_DECIMAL_EU && numType & NUM_K_US) ||
           (numType & NUM_DECIMAL_US && numType & NUM_K_EU)) {
            num_format_votes[5]++;  // INVALID Combination
        } else {
            if(numType & NUM_DECIMAL_EU)
                num_format_votes[0]++;
            else if(numType & NUM_DECIMAL_US)
                num_format_votes[1]++;

            if(numType & NUM_K_EU)
                num_format_votes[2]++;
            else if(numType & NUM_K_US)
                num_format_votes[3]++;
            else if(numType & NUM_K_SPACE)
                num_format_votes[4]++;

            if(numType & NUM_INVALID)
                num_format_votes[5]++;
        }
    }

    int getFormat()
    {
        int num_format = 0;

        // NEW check voting
        if(num_format_votes[5] > 0) {  // if one invalid, all invalid ?
            return NUM_INVALID;
        }

        if((num_format_votes[2] > num_format_votes[3]) && (num_format_votes[2] > num_format_votes[4]))
            num_format |= NUM_K_EU;
        else if((num_format_votes[3] > num_format_votes[2]) && (num_format_votes[3] > num_format_votes[4]))
            num_format |= NUM_K_US;
        else if(num_format_votes[4] > 0)
            num_format |= NUM_K_SPACE;

        int add = 0;
        if(num_format == 0)
            add = 1;

        if(num_format_votes[0] > num_format_votes[1])
            num_format |= NUM_DECIMAL_EU | (NUM_K_EU * add);
        else if(num_format_votes[1] > 0)
            num_format |= NUM_DECIMAL_US | (NUM_K_US * add);

        return num_format;
    }

};


std::string toString(int nNumber, const Settings& _option);
std::string toString(double dNumber, const Settings& _option);
std::string toString(double dNumber, int nPrecision = 7);
std::string toString(const std::complex<double>& dNumber, int nPrecision = 7);
std::string toString(int);
std::string toString(__time64_t tTime, int timeStampFlags);
std::string toString(sys_time_point tp, int timeStampFlags);
std::string toString(long long int nNumber);
std::string toString(size_t nNumber);
std::string toCmdString(double dNumber);
std::string toCmdString(const std::complex<double>& dNumber);
std::string toString(bool bBoolean);
std::string toHexString(int nNumber);
std::string toHexString(size_t nNumber);

/////////////////////////////////////////////////
/// \brief This function converts a std::vector
/// into a std::string, where the components are
/// translated into "{x,y,z...}"
///
/// \param vVector const std::vector<T>&
/// \return std::string
///
/////////////////////////////////////////////////
template <class T>
std::string toString(const std::vector<T>& vVector)
{
    std::string sString;

    for (size_t i = 0; i < vVector.size(); i++)
    {
        sString += toString(vVector[i]) + ",";
    }

    if (!sString.size())
        return "{}";

    sString.back() = '}';

    return "{" + sString;
}

std::vector<std::string> toStrVector(std::string sString);
std::vector<int> toIntVector(std::string sString);
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
size_t versionToInt(std::string);
std::string intToVersion(size_t);

bool isConvertible(const std::string& sStr, ConvertibleType type = CONVTYPE_VALUE, NumberFormatsVoter* voter = nullptr);
int detectTimeDateFormat(const std::string&);

std::string toSystemCodePage(std::string sOutput);
std::string fromSystemCodePage(std::string sOutput);

void replaceAll(MutableStringView sToModify, StringView sToRep, StringView sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
void replaceAll(MutableStringView sToModify, const char* sToRep, const char* sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
std::string replaceControlCharacters(std::string sToModify);
std::string utf8parser(const std::string& sString);
std::string utf8ToAnsi(const std::string& sString);
std::string ansiToUtf8(const std::string& sString);

std::string replacePathSeparator(const std::string& __sPath);
std::string getTimeStamp(bool bGetStamp = true);
std::vector<std::string> split(const std::string& sStr, char cSplit);
std::string ellipsize(const std::string& sLongString, size_t nMaxStringLength = 1024);

bool isEqualStripped(StringView str1, StringView str2);

// NEW
extern int last_num_format;
//extern int num_format_votes[NUM_FORMAT_COUNT];
//static void voteNumType(int numType);
void strChangeNumberFormat(std::string &sNum, int numFormat);

#endif // STRINGTOOLS_HPP


