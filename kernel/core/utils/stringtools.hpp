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
    GET_ONLY_DATE = 0x2,
    GET_AS_TIMESTAMP = 0x4,
    GET_WITH_TEXT = 0x8,
    GET_MILLISECONDS = 0x10,
    GET_FULL_PRECISION = 0x20,
    GET_UNBIASED_TIME = 0x40,
    GET_SHORTEST = 0x80
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

    NUM_AMBIGIOUS_DOT = 0x1000,
    NUM_AMBIGIOUS_COM = 0x2000,

    NUM_INVALID = 0x10000
};

/////////////////////////////////////////////////
/// \brief Structure to get number Format of String or several strings
///        For each Number in a/several string iterate through string,
///        at first digit of number call startParseNumber with current position in string,
///        at every found seperator (',';'.';' ') call addSeperator(...) and at last digit
///        call endParseNumber(idx). It will return the Number format of the current number,
///        When a common type of several Strings should be found use this return value with the
///        vote(int) function for each string and getFormat() to get the overall Format in the End.
/////////////////////////////////////////////////
struct NumberFormatsVoter
{
    private:
    enum
    {
        FMT_DEC_EU,
        FMT_DEC_US,
        FMT_K_EU,
        FMT_K_US,
        FMT_K_SPACE,
        FMT_AMBIGIOUS_DOT,
        FMT_AMBIGIOUS_COM,
        FMT_INVALID,
        FMT_COUNT
    };

    int m_num_format_votes[FMT_COUNT];
    int m_last_idx = -1;
    std::string m_tape;
    int m_curr_format = 0;

    static const inline std::map<std::string, int> num_format_lookup =
    {
        {".", NUM_DECIMAL_US}, {",", NUM_DECIMAL_EU},   // CASE 5: Starting Sepetator -> must be DECIMAL
        {">.", NUM_DECIMAL_US}, {">,", NUM_DECIMAL_EU}, // CASE 4: one ceperator and left is > 3
        {".>", NUM_DECIMAL_US}, {",>", NUM_DECIMAL_EU}, {".<", NUM_DECIMAL_US}, {",<", NUM_DECIMAL_EU},  // CASE 2: one ceperator and right is != 3
        {".=.", NUM_K_EU}, {",=,", NUM_K_US}, {" = ", NUM_K_SPACE},  //CASE 3 same seperator more than once, must be thousands
        {",=.", NUM_K_US | NUM_DECIMAL_US}, {" =.", NUM_K_SPACE | NUM_DECIMAL_US}, {".=,", NUM_K_EU | NUM_DECIMAL_EU}, {" =,", NUM_K_SPACE | NUM_DECIMAL_EU}, // CASE 1 two diff seperators, sec should be decimal, first thousands
        {" =", NUM_K_SPACE}, // Since SPACE is only possible thousands seperator, here we have a unique solution
        {".=", NUM_AMBIGIOUS_DOT}, {",=", NUM_AMBIGIOUS_COM},  // These can be both thousands or decimal sep
        // All below here is invalid
        {".>,", NUM_INVALID}, {".>.", NUM_INVALID}, {".> ", NUM_INVALID}, {".<,", NUM_INVALID}, {".<.", NUM_INVALID}, {".< ", NUM_INVALID},
        {",>,", NUM_INVALID}, {",>.", NUM_INVALID}, {",> ", NUM_INVALID}, {",<,", NUM_INVALID}, {",<.", NUM_INVALID}, {",< ", NUM_INVALID},
        {" >,", NUM_INVALID}, {" >.", NUM_INVALID}, {" > ", NUM_INVALID}, {" <,", NUM_INVALID}, {" <.", NUM_INVALID}, {" < ", NUM_INVALID}
    };

    /////////////////////////////////////////////////
    /// \brief This function is for lookup if current Tape is a valid or invalid number Format
    ///
    /// \param key std::string
    /// \return int
    ///
    /////////////////////////////////////////////////
    int checkNumFormat(const std::string& key) const
    {
        auto elem = num_format_lookup.find(key);
        if(elem != num_format_lookup.end())
            return elem->second;

        return 0;
    }

    /////////////////////////////////////////////////
    /// \brief This function pushes digits # inbetween seperators
    ///
    /// \param idx int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void pushInbetween(int idx)
    {
        int digs_inbetween = idx - m_last_idx;
        if(digs_inbetween > 0)
        {
            if(digs_inbetween < 3)
                m_tape.push_back('<');
            else if(digs_inbetween > 3)
                m_tape.push_back('>');
            else
                m_tape.push_back('=');
        }
    }

    public:
    /////////////////////////////////////////////////
    /// \brief Default Constructor of NumberFormatsVoter
    ///
    ///
    /////////////////////////////////////////////////
    NumberFormatsVoter()
    {
        for(size_t i = 0; i < FMT_COUNT; i++)
            m_num_format_votes[i] = 0;
    }

    /////////////////////////////////////////////////
    /// \brief This function starts parse of a number, start with first non 0 of number
    ///
    /// \param idx int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void startParseNumber(int idx)
    {
        m_last_idx = idx-1;
        m_tape = "";
        m_curr_format = 0;
    }

    /////////////////////////////////////////////////
    /// \brief This function is called for every found seperator and adds to the tape
    ///
    /// \param sep char
    /// \param idx int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void addSeperator(char sep, int idx)
    {
        pushInbetween(idx-1);
        m_tape.push_back(sep);
        m_last_idx = idx;

        // max 3 symbols at ones in tape
        if(m_tape.length() > 3)
            m_tape.erase(0, m_tape.length()-3);

        m_curr_format |= checkNumFormat(m_tape);
    }

    /////////////////////////////////////////////////
    /// \brief This function is called when last char of a number is reached during parsing
    ///
    /// \param idx int
    /// \return int
    ///
    /////////////////////////////////////////////////
    int endParseNumber(int idx)
    {
        // we only want to check for the last 2 or 1
        m_tape = m_tape.back();
        pushInbetween(idx);
        m_curr_format |= checkNumFormat(m_tape);

        m_last_idx = idx;
        m_tape = "";
        return m_curr_format;
    }

    /////////////////////////////////////////////////
    /// \brief This function is called when last char of a number is reached during parsing
    ///        and the result is directly used for voting
    ///
    /// \param idx int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void endParseAndVote(int idx){
        vote(endParseNumber(idx));
    }

    /////////////////////////////////////////////////
    /// \brief This function is called to make a vote with the given number Type
    ///
    /// \param numType int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void vote(int numType)
    {
        if((numType & NUM_DECIMAL_EU && numType & NUM_K_US) ||
           (numType & NUM_DECIMAL_US && numType & NUM_K_EU))
        {
            m_num_format_votes[FMT_INVALID]++;  // INVALID Combination
        }
        else
        {
            if(numType & NUM_DECIMAL_EU)
                m_num_format_votes[FMT_DEC_EU]++;
            else if(numType & NUM_DECIMAL_US)
                m_num_format_votes[FMT_DEC_US]++;

            if(numType & NUM_K_EU)
                m_num_format_votes[FMT_K_EU]++;
            else if(numType & NUM_K_US)
                m_num_format_votes[FMT_K_US]++;
            else if(numType & NUM_K_SPACE)
                m_num_format_votes[FMT_K_SPACE]++;
            else if(numType & NUM_AMBIGIOUS_DOT)
                m_num_format_votes[FMT_AMBIGIOUS_DOT]++;
            else if(numType & NUM_AMBIGIOUS_COM)
                m_num_format_votes[FMT_AMBIGIOUS_COM]++;

            if(numType & NUM_INVALID)
                m_num_format_votes[FMT_INVALID]++;
        }
    }

    /////////////////////////////////////////////////
    /// \brief This function returns the current Format given by votes
    ///
    /// \return int
    ///
    /////////////////////////////////////////////////
    int getFormat()
    {
        if (m_num_format_votes[FMT_INVALID])
            return NUM_INVALID;

        int cnt_set = 0;
        int num_format = 0;

        if (m_num_format_votes[FMT_K_EU])
        {
            num_format |= NUM_K_EU;
            cnt_set++;
        }

        if (m_num_format_votes[FMT_K_US])
        {
            num_format |= NUM_K_US;
            cnt_set++;
        }

        if (m_num_format_votes[FMT_K_SPACE])
        {
            num_format |= NUM_K_SPACE;
            cnt_set++;
        }

        if (cnt_set > 1)
            return NUM_INVALID;

        // If no thousands seperator was set increase cnt, to also handle the case where both decimal separators are set invalidly.
        if (cnt_set == 0)
            cnt_set++;

        // If we have some ambigious votes that correspont to unique decimals use these to set thousands seperator.
        if (m_num_format_votes[FMT_DEC_EU])
        {
            num_format |= NUM_DECIMAL_EU | (m_num_format_votes[FMT_AMBIGIOUS_DOT] ? NUM_K_EU : 0);
            cnt_set++;
        }

        if (m_num_format_votes[FMT_DEC_US])
        {
            num_format |= NUM_DECIMAL_US | (m_num_format_votes[FMT_AMBIGIOUS_COM] ? NUM_K_US : 0);
            cnt_set++;
        }

        if (cnt_set > 2)
            return NUM_INVALID;

        // We're happy, if we detected at least the decimals
        if (num_format & (NUM_DECIMAL_EU | NUM_DECIMAL_US))
            return num_format;

        // This is the fallback section, if we had no votes on
        // decimals or only ambiguous votes. We use heuristics
        // to solve this problem.
        //
        // Use thousands to detect decimals
        if (num_format & (NUM_K_EU | NUM_K_SPACE))
            return num_format | NUM_DECIMAL_EU;

        if (num_format & NUM_K_US)
            return num_format | NUM_DECIMAL_US;

        // Do we have ambigous votes? Handle them as decimals, if only comma => it is EU
        if (m_num_format_votes[FMT_AMBIGIOUS_COM] && !m_num_format_votes[FMT_AMBIGIOUS_DOT])
            return NUM_K_EU | NUM_DECIMAL_EU;

        // If both or only dot it is US, which is fallback for everything
        return NUM_DECIMAL_US | NUM_K_US;
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
std::string formatDuration(double dDuration);
std::string formatRfc5322(sys_time_point tp);

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
std::string strfill(const std::string& sString, size_t nWidth, char cFill = ' ', bool limit = false);
std::string strlfill(const std::string& sString, size_t nWidth, char cFill = ' ');

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
double versionToFloat(std::string);
std::string floatToVersion(double);

bool isConvertible(const std::string& sStr, ConvertibleType type = CONVTYPE_VALUE, NumberFormatsVoter* voter = nullptr);
int detectTimeDateFormat(const std::string&);

std::string toSystemCodePage(std::string sOutput);
std::string fromSystemCodePage(std::string sOutput);

void replaceAll(MutableStringView sToModify, StringView sToRep, StringView sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
void replaceAll(MutableStringView sToModify, const char* sToRep, const char* sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);
std::string strRepeat(const std::string& sStr, int nCount);
std::string replaceControlCharacters(std::string sToModify);
std::string utf8parser(const std::string& sString);
std::string utf8ToAnsi(const std::string& sString);
std::string ansiToUtf8(const std::string& sString);

std::string replacePathSeparator(const std::string& __sPath);
std::string getTimeStamp(bool bGetStamp = true);
std::vector<std::string> split(const std::string& sStr, char cSplit);
std::string ellipsize(const std::string& sLongString, size_t nMaxStringLength = 1024);

bool isEqualStripped(StringView str1, StringView str2);
std::string removeMaskedStrings(const std::string& sString);
std::string removeQuotationMarks(const std::string& sString);

void strChangeNumberFormat(std::string &sNum, int numFormat);

std::string encode_base_n(const std::string& sToEncode, bool isFile, int n);
std::string decode_base_n(const std::string& sToDecode, int n);

#endif // STRINGTOOLS_HPP


