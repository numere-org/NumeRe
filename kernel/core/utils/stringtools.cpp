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

#include "stringtools.hpp"
#include "../settings.hpp"
#include "../structures.hpp"

#include <fast_float/fast_float.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <regex>

// Forward declarations
std::string getNextArgument(std::string& sArgList, bool bCut);
double intPower(double, int);

// toString function implementations
// There's an overwrite for mostly every variable type
//
/////////////////////////////////////////////////
/// \brief Converts an integer to a string.
///
/// \param nNumber int
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(int nNumber, const Settings& _option)
{
    return toString((double)nNumber, _option);  // Unnoetig das nochmal zu schreiben. Rufen wir die andere Funktion mit einer expliziten Konvertierung auf
}


/////////////////////////////////////////////////
/// \brief Converts a double to a string using
/// the precision from the Settings object.
///
/// \param dNumber double
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(double dNumber, const Settings& _option)
{
    return toString(dNumber, _option.getPrecision());   // Auf den eigentlichen string wird dann mit der Methode ostringstream::str() zugegriffen
}


/////////////////////////////////////////////////
/// \brief Converts a double to a string using
/// the passed precision.
///
/// \param dNumber double
/// \param nPrecision int
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(double dNumber, int nPrecision)
{
    std::ostringstream Temp;
    Temp.precision(nPrecision);
    Temp << dNumber;
    return Temp.str();
}

/////////////////////////////////////////////////
/// \brief Converts a complex number into a
/// printable string.
///
/// \param dNumber const std::complex<double>&
/// \param nPrecision int
/// \return sstd::tring
///
/////////////////////////////////////////////////
std::string toString(const std::complex<double>& dNumber, int nPrecision)
{
    if (std::isnan(dNumber.real()) && std::isnan(dNumber.imag()))
        return "nan";

    std::ostringstream Temp;
    Temp.precision(std::rint(nPrecision / (dNumber.real() != 0.0 && dNumber.imag() != 0.0 && !std::isnan(dNumber.imag()) ? 2 : 1)));

    if (dNumber.real() || !dNumber.imag())
        Temp << dNumber.real();

    if (dNumber.imag())
    {
        if ((dNumber.imag() > 0.0 || std::isnan(dNumber.imag())) && dNumber.real() != 0.0)
            Temp << "+";

        Temp << dNumber.imag() << (std::isnan(dNumber.imag()) || std::isinf(dNumber.imag()) ? " i" : "i");
    }

    return Temp.str();
}


/////////////////////////////////////////////////
/// \brief Converts an integer to a string
/// without the Settings bloat.
///
/// \param nNumber int
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(int nNumber)
{
    return toString((long long int)nNumber);
}


/////////////////////////////////////////////////
/// \brief Converts an unsigned integer to a
/// string.
///
/// \param nNumber size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(size_t nNumber)
{
    return toString((long long int)nNumber);
}


/////////////////////////////////////////////////
/// \brief Converts a time_t to a string. Will
/// actually print a timestamp.
///
/// \param tTime __time64_t
/// \param timeStampFlags int
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(__time64_t tTime, int timeStampFlags)
{
    tm* ltm = _localtime64(&tTime);

    if (!ltm)
        return "";

    time_zone tz = getCurrentTimeZone();

    std::ostringstream timeStream;

    if (!(timeStampFlags & GET_ONLY_TIME))
    {
        timeStream << 1900+ltm->tm_year << "-"; //YYYY-
        timeStream << std::setfill('0') << std::setw(2) << ltm->tm_mon + 1 << "-"; // MM-
        timeStream << std::setfill('0') << std::setw(2) << ltm->tm_mday; 	// DD

        if (timeStampFlags & GET_AS_TIMESTAMP)
            timeStream << "_";
        else
        {
            timeStream << ", ";	// Komma im regulaeren Datum

            if (timeStampFlags & GET_WITH_TEXT)
            {
                if (_lang.get("TOOLS_TIMESTAMP_AT") == "TOOLS_TIMESTAMP_AT")
                    timeStream << "at ";
                else
                    timeStream << _lang.get("TOOLS_TIMESTAMP_AT") << " ";
            }
        }
    }

    if (timeStampFlags & GET_ONLY_TIME)
        timeStream << std::setfill('0') << std::setw(2) << ltm->tm_hour + tz.Bias.count()/60; 	// hh
    else
        timeStream << std::setfill('0') << std::setw(2) << ltm->tm_hour; 	// hh

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";		// ':' im regulaeren Datum

    timeStream << std::setfill('0') << std::setw(2) << ltm->tm_min;	// mm

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";

    timeStream << std::setfill('0') << std::setw(2) << ltm->tm_sec;	// ss

    return timeStream.str();
}


/////////////////////////////////////////////////
/// \brief Converts a sys_time_point to a string.
///
/// \param tp sys_time_point
/// \param timeStampFlags int
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(sys_time_point tp, int timeStampFlags)
{
    time_stamp ltm = getTimeStampFromTimePoint(tp);
    time_zone tz = getCurrentTimeZone();
    std::ostringstream timeStream;

    if (!(timeStampFlags & GET_ONLY_TIME))
    {
        timeStream << ltm.m_ymd.year() << "-"; //YYYY-
        timeStream << std::setfill('0') << std::setw(2) << unsigned(ltm.m_ymd.month()) << "-"; // MM-
        timeStream << ltm.m_ymd.day(); 	// DD

        if (timeStampFlags & GET_AS_TIMESTAMP)
            timeStream << "_";
        else
        {
            timeStream << ", ";	// Komma im regulaeren Datum

            if (timeStampFlags & GET_WITH_TEXT)
            {
                if (_lang.get("TOOLS_TIMESTAMP_AT") == "TOOLS_TIMESTAMP_AT")
                    timeStream << "at ";
                else
                    timeStream << _lang.get("TOOLS_TIMESTAMP_AT") << " ";
            }
        }
    }

    if (timeStampFlags & GET_ONLY_TIME && !(timeStampFlags & GET_UNBIASED_TIME))
        timeStream << std::setfill('0') << std::setw(2) << ltm.m_hours.count() + (tz.Bias + tz.DayLightBias).count()/60; 	// hh
    else
        timeStream << std::setfill('0') << std::setw(2) << ltm.m_hours.count(); 	// hh

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";		// ':' im regulaeren Datum

    timeStream << std::setfill('0') << std::setw(2) << ltm.m_minutes.count();	// mm

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";

    timeStream << std::setfill('0') << std::setw(2) << ltm.m_seconds.count();	// ss

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
    {
        if (timeStampFlags & (GET_MILLISECONDS | GET_FULL_PRECISION)
            || ltm.m_millisecs.count())
            timeStream << "." << std::setfill('0') << std::setw(3) << ltm.m_millisecs.count();

        if (timeStampFlags & GET_FULL_PRECISION)
            timeStream << std::setfill('0') << std::setw(3) << ltm.m_microsecs.count();
    }

    return timeStream.str();
}


/////////////////////////////////////////////////
/// \brief Converts a long long int to a string.
///
/// \param nNumber long longint
/// \return string
///
/////////////////////////////////////////////////
std::string toString(long long int nNumber)
{
    std::ostringstream Temp;
    Temp << nNumber;
    return Temp.str();
}


/////////////////////////////////////////////////
/// \brief Converts a boolean to a string by
/// actually converting the numerical value to
/// \c "true" and \c "false".
///
/// \param bBoolean bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(bool bBoolean)
{
    if (bBoolean)
        return "true";
    else
        return "false";
}


/////////////////////////////////////////////////
/// \brief Converts a numerical value into a
/// "full" precision string.
///
/// \param dNumber double
/// \return std::string
///
/////////////////////////////////////////////////
std::string toCmdString(double dNumber)
{
    std::ostringstream Temp;
    Temp.precision(20);
    Temp << dNumber;
    return Temp.str();
}


/////////////////////////////////////////////////
/// \brief Converts a numerical value into a
/// "full" precision string
///
/// \param dNumber const std::complex<double>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toCmdString(const std::complex<double>& dNumber)
{
    return toString(dNumber, 20);
}


/////////////////////////////////////////////////
/// \brief Converts an integer to a hexadecimal
/// number printed as string.
///
/// \param nNumber int
/// \return std::string
///
/////////////////////////////////////////////////
std::string toHexString(int nNumber)
{
    std::ostringstream Temp;
    Temp << std::hex << nNumber;
    return "0x" + Temp.str();
}


/////////////////////////////////////////////////
/// \brief Converts an integer to a hexadecimal
/// number printed as string.
///
/// \param nNumber size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string toHexString(size_t nNumber)
{
    std::ostringstream Temp;
    Temp << std::hex << nNumber;
    return "0x" + Temp.str();
}


/////////////////////////////////////////////////
/// \brief This function converts a std::string
/// into a std::vector, where the string shall be
/// passed as "{x,y,z,...}"
///
/// \param sString std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> toStrVector(std::string sString)
{
    std::vector<std::string> vVector;

    if (sString == "{}" || !sString.length())
        return vVector;

    if (sString.front() == '{' && sString.back() == '}')
    {
        sString.erase(0, 1);
        sString.pop_back();
    }

    while (sString.length())
    {
        vVector.push_back(getNextArgument(sString, true));
    }

    return vVector;
}


/////////////////////////////////////////////////
/// \brief This function converts a std::string
/// into a std::vector, where the string shall be
/// passed as "{x,y,z,...}"
///
/// \param sString std::string
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> toIntVector(std::string sString)
{
    std::vector<int> vVector;
    std::vector<std::string> vStrVect = toStrVector(sString);

    if (!vStrVect.size())
        return vVector;

    for (const auto& s : vStrVect)
    {
        vVector.push_back(StrToInt(s));
    }

    return vVector;
}


/////////////////////////////////////////////////
/// \brief Removes vowels and umlauts from the
/// passed string.
///
/// \param sText const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string condenseText(const std::string& sText)
{
    std::string sReturn = sText;
    static std::string sToErase = " AaEeIiOoUu\xC4\xE4\xD6\xF6\xDC\xFC\xDFYy";
    for (size_t i = 0; i < sReturn.length(); i++)
    {
        if (sToErase.find(sReturn[i]) != std::string::npos
                || sReturn[i] == 142
                || sReturn[i] == 132
                || sReturn[i] == 153
                || sReturn[i] == 148
                || sReturn[i] == 154
                || sReturn[i] == 129
                || sReturn[i] == 225)
        {
            sReturn.erase(i, 1);
            i--;
        }
    }
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function truncates the string to
/// the passed max length (including a trailing
/// ellipsis) or returns it unmodified, if it is
/// shorter.
///
/// \param sText const std::string&
/// \param nMaxChars size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string truncString(const std::string& sText, size_t nMaxChars)
{
    if (sText.length() <= nMaxChars)
        return sText;

    return sText.substr(0, nMaxChars-3) + "...";
}


/////////////////////////////////////////////////
/// \brief Removes leading and trailing white
/// spaces and tabulator characters.
///
/// \param sToStrip std::string&
/// \return void
///
/////////////////////////////////////////////////
void StripSpaces(std::string& sToStrip)
{
    if (!sToStrip.length())
        return;

    sToStrip.erase(0, sToStrip.find_first_not_of(" \t"));

    if (sToStrip.length() && (sToStrip.back() == ' ' || sToStrip.back() == '\t'))
        sToStrip.erase(sToStrip.find_last_not_of(" \t") + 1);
}


/////////////////////////////////////////////////
/// \brief Converts a string into an integer.
///
/// \param sString const std::string&
/// \return int
///
/////////////////////////////////////////////////
int StrToInt(const std::string& sString)
{
    return atoi(sString.c_str());
}


/////////////////////////////////////////////////
/// \brief Converts a string into a double.
///
/// \param sString const std::string&
/// \return double
///
/////////////////////////////////////////////////
double StrToDb(const std::string& sString)
{
    return atof(sString.c_str());
}


/////////////////////////////////////////////////
/// \brief Converts a string into a double
/// considering logical values.
///
/// \param sString const std::string&
/// \return double
///
/////////////////////////////////////////////////
double StrToLogical(const std::string& sString)
{
    if (toLowerCase(sString) == "true" || sString == "1")
        return 1.0;
    else if (toLowerCase(sString) == "false" || sString == "0")
        return 0.0;

    return StrToCmplx(sString).real();
}


/////////////////////////////////////////////////
/// \brief Converts a string into a complex
/// number.
///
/// \param sString std::string& const
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> StrToCmplx(const std::string& sString)
{
    double re, im;

    //if (!isConvertible(sString, CONVTYPE_VALUE) || sString == "---") <-- Should be obsolete due to the fact that it should be checked in advance
    if (sString == "---"
        || (sString.length() < 4 && (toLowerCase(sString) == "na" || toLowerCase(sString) == "n/a")))
        return NAN;

    const char* pStart = sString.c_str();
    const char* pEnd = pStart + sString.length();
    int oprt = 0;

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (pStart < pEnd && (*pStart == ' ' || *pStart == '+'))
        pStart++;

    if (pStart >= pEnd)
        return NAN; // End of Input

    // read 1st value
    fast_float::from_chars_result res = fast_float::from_chars(pStart, pEnd, re);

    // Error code might imply "i" or "-i"
    if (res.ec == std::errc::invalid_argument)
    {
        // Check, if there are other characters
        while (res.ptr < pEnd && (*res.ptr == '-' || *res.ptr == ' ' || tolower(*res.ptr) == 'i'))
            res.ptr++;

        // Did we find any? If no, it is +/-i
        if (res.ptr >= pEnd)
            return std::complex<double>(0, *pStart == '-' ? -1 : 1);

        // We did find other character sequences
        return NAN;
    }

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (res.ptr < pEnd && *res.ptr == ' ')
        res.ptr++;

    if (res.ptr >= pEnd)
        return re; // End of Input

    if (*res.ptr == '-' || *res.ptr == '+')
    {
        oprt = *res.ptr == '-' ? -1 : 1;
        res.ptr++;
    }

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (res.ptr < pEnd && *res.ptr == ' ')
        res.ptr++;

    if (res.ptr >= pEnd)
        return re; // End of Input

    // Is it the actual imaginary value?
    if (tolower(*res.ptr) == 'i'
        && oprt
        && (res.ptr+1 >= pEnd || tolower(*(res.ptr+1)) != 'n'))
        return std::complex<double>(re, oprt);

    if ((tolower(*res.ptr) == 'i' && (res.ptr+1 >= pEnd || tolower(*(res.ptr+1)) != 'n')) || *res.ptr == '*')
        return std::complex<double>(0.0, re);

    // read 2nd value
    res = fast_float::from_chars(res.ptr, pEnd, im);

    // Error
    if (res.ec == std::errc::invalid_argument)
        return re;

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (res.ptr < pEnd && *res.ptr == ' ')
        res.ptr++;

    if (tolower(*res.ptr) != 'i' && *res.ptr != '*')
    { // ERROR or premature end of input
        return re;
    }

    return std::complex<double>(re, oprt*im);
}


/////////////////////////////////////////////////
/// \brief Convert a string to a sys_time_point.
///
/// \param sString const std::string&
/// \return sys_time_point
///
/////////////////////////////////////////////////
sys_time_point StrToTime(const std::string& sString)
{
    int format = detectTimeDateFormat(sString);

    if (format == TD_NONE)
        return sys_time_point();

    // Try to detect dates
    size_t pos = sString.find_first_not_of(" \t");

    time_stamp timeStruct = getTimeStampFromTimePoint(sys_time_now());
    timeStruct.m_hours = std::chrono::hours::zero();
    timeStruct.m_minutes = std::chrono::minutes::zero();
    timeStruct.m_seconds = std::chrono::seconds::zero();
    timeStruct.m_millisecs = std::chrono::milliseconds::zero();
    timeStruct.m_microsecs = std::chrono::microseconds::zero();

    const char* DIGITS = "0123456789";

    // Contains time
    if (format & TD_HHMM || format & TD_HHMMSS)
    {
        std::vector<std::string> toks = split(sString, ':');

        bool isFirst = sString.find(':') < 3 + pos;

        for (size_t i = 0; i < toks.size(); i++)
        {
            // Catch milliseconds written as s.i or ss.iii
            StripSpaces(toks[i]);

            if (i == 2 && toks[i].length() >= 3 && (toks[i][1] == '.' || toks[i][2] == '.'))
            {
                size_t dot = toks[i].find('.');
                toks.insert(toks.begin()+i+1, toks[i].substr(dot+1));
                toks[i].erase(dot);
            }

            // Remove leading of trailing non-time characters
            if (toks[i].find_first_not_of(DIGITS) != std::string::npos)
            {
                if ((isFirst && i+1 != toks.size()) || (!isFirst && i))
                    break;

                if (isFirst)
                    toks[i].erase(toks[i].find_first_not_of(DIGITS));
                else
                    toks[i].erase(0, toks[i].find_last_not_of(DIGITS)+1);
            }

            if (!i)
                timeStruct.m_hours = std::chrono::hours(StrToInt(toks[i]));
            else if (i == 1)
                timeStruct.m_minutes = std::chrono::minutes(StrToInt(toks[i]));
            else if (i == 2 && format & TD_HHMMSS)
                timeStruct.m_seconds = std::chrono::seconds(StrToInt(toks[i]));
            else if (i == 3 && format & TD_HHMMSS)
                timeStruct.m_millisecs = std::chrono::milliseconds(int(StrToInt(toks[i])*intPower(10.0, 3-toks[i].length()))); // scaling due to possible missing trailing zeros
        }
    }

    // Contains DD.MM.YY or DD/MM/YYYY
    if (format & TD_DDMM || format & TD_DDMMYY)
    {
        std::vector<std::string> toks = split(sString, format & TD_SEP_SLASH ? '/' : '.');
        date::year y(timeStruct.m_ymd.year());
        date::month m(0);
        date::day d(0);

        bool isFirst = sString.find(format & TD_SEP_SLASH ? '/' : '.') < 3 + pos;

        for (size_t i = 0; i < toks.size(); i++)
        {
            StripSpaces(toks[i]);

            // Remove leading or trailing non-date chars
            if (toks[i].find_first_not_of(DIGITS) != std::string::npos)
            {
                // Obscure logic needed because msecs are also
                // separated by a dot
                if ((isFirst && i < bool(format & TD_DDMMYY) + 1) || (!isFirst && i))
                    break;

                if (isFirst)
                    toks[i].erase(toks[i].find_first_not_of(DIGITS));
                else
                    toks[i].erase(0, toks[i].find_last_not_of(DIGITS)+1);
            }

            if (!i)
                d = date::day(StrToInt(toks[i]));
            else if (i == 1)
                m = date::month(StrToInt(toks[i]));
            else if (i == 2 && format & TD_DDMMYY)
            {
                if (toks[i].length() > 2)
                    y = date::year(StrToInt(toks[i]));
                else
                    y = date::year(StrToInt(toks[i]) + 2000);
            }
        }

        timeStruct.m_ymd = date::year_month_day(y,m,d);
    }

    // Contains YY-MM-DD or YY/MM/DD
    if (format & TD_YYMMDD)
    {
        std::vector<std::string> toks;
        bool isFirst;
        date::year y(0);
        date::month m(0);
        date::day d(0);

        if (format & TD_SEP_MINUS)
        {
            toks = split(sString, '-');
            isFirst = sString.find('-') < 5 + pos;
        }
        else
        {
            toks = split(sString, '/');
            isFirst = sString.find('/') < 5 + pos;
        }

        for (size_t i = 0; i < toks.size(); i++)
        {
            StripSpaces(toks[i]);

            // Remove leading or trailing non-date chars
            if (toks[i].find_first_not_of(DIGITS) != std::string::npos)
            {
                if ((isFirst && i+1 != toks.size()) || (!isFirst && i))
                    break;

                if (isFirst)
                    toks[i].erase(toks[i].find_first_not_of(DIGITS));
                else
                    toks[i].erase(0, toks[i].find_last_not_of(DIGITS)+1);
            }

            if (!i)
            {
                if (toks[i].length() > 2)
                    y = date::year(StrToInt(toks[i]));
                else
                    y = date::year(StrToInt(toks[i]) + 2000);
            }
            else if (i == 1)
                m = date::month(StrToInt(toks[i]));
            else if (i == 2)
                d = date::day(StrToInt(toks[i]));
        }

        timeStruct.m_ymd = date::year_month_day(y,m,d);
    }

    return getTimePointFromTimeStamp(timeStruct);
}


/////////////////////////////////////////////////
/// \brief Converts a version string into a
/// multi-digit integer
///
/// \param sVersion std::string
/// \return size_t
///
/////////////////////////////////////////////////
size_t versionToInt(std::string sVersion)
{
    size_t nVOffset = 0;

    if (sVersion.front() == 'v')
        nVOffset = 1;

    replaceAll(sVersion, ".", "");
    return StrToInt(sVersion.substr(nVOffset));
}


/////////////////////////////////////////////////
/// \brief Converts a multi-digit integer into a
/// verison string.
///
/// \param nVersionDigits size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string intToVersion(size_t nVersionDigits)
{
    std::string _sVer = toString(nVersionDigits);

    // Prepend zeroes, if the length is shorter than
    // three
    if (_sVer.length() < 3)
        _sVer.insert(0, 3-_sVer.length(), '0');

    // Convert the version string into the M.m.b format
    for (size_t n = 1; n < _sVer.length(); n++)
    {
        if (n % 2)
            _sVer.insert(n, 1, '.');
    }

    return _sVer;
}


/////////////////////////////////////////////////
/// \brief Converts a string literal to the
/// internal representation in tables and
/// clusters.
///
/// \param sStr std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string toInternalString(std::string sStr)
{
    if (sStr.front() == '"' && sStr.back() == '"')
        sStr = sStr.substr(1, sStr.length()-2);

    for (size_t i = 0; i < sStr.length(); i++)
    {
        if (sStr.compare(i, 2, "\\t") == 0
            && sStr.compare(i, 4, "\\tau") != 0
            && sStr.compare(i, 6, "\\theta") != 0
            && sStr.compare(i, 6, "\\times") != 0)
            sStr.replace(i, 2, "\t");

        if (sStr.compare(i, 2, "\\n") == 0
            && sStr.compare(i, 3, "\\nu") != 0
            && sStr.compare(i, 4, "\\neq") != 0)
            sStr.replace(i, 2, "\n");

        if (sStr.compare(i, 2, "\\\"") == 0)
            sStr.replace(i, 2, "\"");

        if (sStr.compare(i, 2, "\\ ") == 0)
            sStr.replace(i, 2, "\\");
    }

    return sStr;
}


/////////////////////////////////////////////////
/// \brief Converts an internal string to the
/// external representation in the terminal.
///
/// \param sStr std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string toExternalString(std::string sStr)
{
    replaceAll(sStr, "\"", "\\\"");
    return "\"" + sStr + "\"";
}


/////////////////////////////////////////////////
/// \brief Converts uppercase to lowercase
/// letters.
///
/// \param sUpperCase const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toLowerCase(const std::string& sUpperCase)
{
    std::string sLowerCase = sUpperCase;
    static Umlauts _umlauts;
    constexpr int charDiff = (int)'a' - (int)'A';

    for (size_t i = 0; i < sLowerCase.length(); i++)
    {
        // --> Laufe alle Zeichen im String ab und pruefe, ob ihr CHAR-Wert zwischen A und Z liegt
        if ((int)sLowerCase[i] >= (int)'A' && (int)sLowerCase[i] <= (int)'Z')
            sLowerCase[i] = (char)((int)sLowerCase[i] + charDiff);
        else
            sLowerCase[i] = _umlauts.toLower(sLowerCase[i]);
    }
    return sLowerCase;
}


/////////////////////////////////////////////////
/// \brief Converts lowercase letters to
/// uppercase ones.
///
/// \param sLowerCase const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toUpperCase(const std::string& sLowerCase)
{
    std::string sUpperCase = sLowerCase;
    static Umlauts _umlauts;
    const int EQUAL = 0;
    constexpr int charDiff = (int)'A' - (int)'a';

    for (size_t i = 0; i < sUpperCase.length(); i++)
    {
        // Handle escape characters like linebreaks or tabulator characters
        if ((!i || sUpperCase[i - 1] != '\\') && (sUpperCase.compare(i, 2, "\\n") == EQUAL || sUpperCase.compare(i, 2, "\\t") == EQUAL))
        {
            i++;
            continue;
        }
        else if (sUpperCase.compare(i, 2, "\\n") == EQUAL)
            sUpperCase.replace(i, 2, "N");
        else if (sUpperCase.compare(i, 2, "\\t") == EQUAL)
            sUpperCase.replace(i, 2, "T");
        else if ((int)sUpperCase[i] >= (int)'a' && (int)sLowerCase[i] <= (int)'z')
            sUpperCase[i] = (char)((int)sUpperCase[i] + charDiff);
        else
            sUpperCase[i] = _umlauts.toUpper(sUpperCase[i]);
    }

    return sUpperCase;
}


/////////////////////////////////////////////////
/// \brief Static function to determine, whether
/// the character at passed position fits the
/// pattern needed for a date.
///
/// \param sStr const std::string&
/// \param pos size_t
/// \return bool
///
/////////////////////////////////////////////////
static bool isDateTimePattern(const std::string& sStr, size_t pos)
{
    if (pos
        && isdigit(sStr[pos-1])
        && isdigit(sStr[pos+1])
        && ((pos+3 < sStr.length() && sStr[pos+3] == sStr[pos])
            || (pos+2 < sStr.length() && sStr[pos+2] == sStr[pos])
            || sStr[pos] == ':'
            || (sStr[pos] == '.' && pos > 6 && sStr[pos-2] == ':')))
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This function is used during the detectCommonType Function to
///        count detected Number Formats troughout one Column
///
/// \param numType int
/// \return void
///
/////////////////////////////////////////////////
static void voteNumType(int numType)  {
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

//TODO Is that a nice way? I dont think so at all
int last_num_format = 0;
int num_format_votes[] = {0,0,0,0,0,0};

/////////////////////////////////////////////////
/// \brief This function checks, whether a string
/// can be converted to the selected
/// ConvertibleType.
///
/// \param sStr const std::string&
/// \param type ConvertibleType
/// \return bool
///
/////////////////////////////////////////////////
bool isConvertible(const std::string& sStr, ConvertibleType type){
    if (type == CONVTYPE_VALUE)
    {
        // Apply the simplest heuristic: mostly every numerical valid character
        // and ignore characters, which cannot represent a value by themselves
        if (sStr.find_first_not_of(" 0123456789.,eianfEIANF+-*/\t") != std::string::npos
            || (sStr.find_first_not_of("+-* .,\teE") == std::string::npos && sStr.find("---") == std::string::npos))
            return false;

        // Eliminate invalid character positions
        if (tolower(sStr.front()) == 'e'
            || tolower(sStr.front()) == 'a'
            || tolower(sStr.front()) == 'f'
            || tolower(sStr.back()) == 'e')
            return false;

        // NEW
        bool inNum = false;
        int numStartIdx = 0;
        int lastSepIdx = -1;

        // TODO is a starting decimal seperator valid ? If so we know what Format hast do be
        int nFormat = NUM_NONE;
        if(sStr[0] == '.')
            nFormat |= NUM_DECIMAL_US;
        else if (sStr[0] == ',')
            nFormat |= NUM_DECIMAL_EU;

        if(nFormat != NUM_NONE){
            lastSepIdx = 0;
            inNum = true;
        } else if('0' <= sStr[0] && sStr[0] <= '9' )
            inNum = true;

        // Regression fix introduced because NA is accepted as NaN
        size_t i = 1;
        for (; i < sStr.length(); i++)
        {
            //if(sStr[i] < '0' || '9' < sStr[i]) { // No seperator and no Decimal -> outside of Number
            if(sStr[i] == ',' || sStr[i] == '.' || sStr[i] == ' ' || ('0' <= sStr[i] && sStr[i] <= '9' )) {
                if(!inNum){
                    inNum = true;
                    numStartIdx = i;
                }
                if(sStr[i] == ',' || sStr[i] == '.' || sStr[i] == ' ' ) {
                    if(lastSepIdx != -1) {
                        if(sStr[lastSepIdx] != sStr[i]) {
                            // Case 1: two seperators in one number
                            // TODO check for invalid change of Format inside one number like xxx.xxx,xxx.xxx
                            if(sStr[i] == ',') {
                                nFormat |= NUM_DECIMAL_EU;
                            } else if(sStr[i] == '.') {
                                nFormat |= NUM_DECIMAL_US;
                            } else {
                                // invalid ! cannot be Leerzeichen
                                nFormat |= NUM_INVALID;
                                //return false;
                            }
                            if(sStr[lastSepIdx] == ' ') {
                                nFormat |= NUM_K_SPACE;
                            } else if(sStr[lastSepIdx] == '.') {
                                nFormat |= NUM_K_EU;
                            } else if(sStr[lastSepIdx] == ',') {
                                nFormat |= NUM_K_US;
                            }
                        } else {
                            // Case 3: Two same seperators inbetween must be thousands seperator
                            if(i - lastSepIdx != 4) {
                                nFormat |= NUM_INVALID;
                            } else if(sStr[i] == ' ') {
                                nFormat |= NUM_K_SPACE;
                            } else if(sStr[i] == '.') {
                                nFormat |= NUM_K_EU;
                            } else if(sStr[i] == ',') {
                                nFormat |= NUM_K_US;
                            } else {
                                nFormat |= NUM_INVALID;
                                //return false;
                            }
                        }
                    } else {
                        if((i - numStartIdx) > 4) {
                            // Case 4 first seperator but left more than 3 Digits -> must be Decimal seperator
                            if(sStr[i] == ',') {
                                nFormat |= NUM_DECIMAL_EU;
                            } else if(sStr[i] == '.') {
                                nFormat |= NUM_DECIMAL_US;
                            } else {
                                nFormat |= NUM_INVALID;
                                //return false;
                            }
                        }
                    }
                    lastSepIdx = i;
                }
            } else {
                if(inNum){
                    if(lastSepIdx != -1 && (i - lastSepIdx) != 3){
                        // Case 2: != 3 digits after last seperator -> must be decimal seperator
                        if(sStr[lastSepIdx] == ',') { // TODO ADD check that type is not already US via &
                            nFormat |= NUM_DECIMAL_EU;
                        } else if(sStr[lastSepIdx] == '.') {
                            nFormat |= NUM_DECIMAL_US;
                        } else {  // ' ' cannot be a decimal seperator
                            nFormat |= NUM_INVALID;
                            //return false;
                        }
                    }

                    inNum = false;
                    numStartIdx = 0;
                    lastSepIdx = -1;
                }
                if (i-1 < sStr.length() && sStr[i] == '-' || sStr[i] == '+')
                {
                    if (tolower(sStr[i-1]) != 'e'
                        && (isdigit(sStr[i-1]) && sStr.find_first_of("iI", i+1) == std::string::npos)
                        && sStr[i-1] != ' ')
                        return false;
                }
            }
        }

        if(inNum){
            if(lastSepIdx != -1 && (i - lastSepIdx) != 4){
                // Case 2: != 3 digits after last seperator -> must be decimal seperator
                if(sStr[lastSepIdx] == ',') { // TODO ADD check that type is not already US via &
                    nFormat |= NUM_DECIMAL_EU;
                } else if(sStr[lastSepIdx] == '.') {
                    nFormat |= NUM_DECIMAL_US;
                } else {  // ' ' cannot be a decimal seperator
                    nFormat |= NUM_INVALID;
                    //return false;
                }
            }
        }

        //TODO Check last value for number format
        voteNumType(nFormat);
        // Try to detect dates
        return !isConvertible(sStr, CONVTYPE_DATE_TIME);
    }
    else if (type == CONVTYPE_LOGICAL)
    {
        // Apply the simplest heuristic: mostly every numerical valid character
        // and ignore characters, which cannot represent a value by themselves
        if (toLowerCase(sStr) == "true"
            || toLowerCase(sStr) == "false"
            || sStr == "0"
            || sStr == "1")
            return true;

        // Try to detect values
        return isConvertible(sStr, CONVTYPE_VALUE);
    }
    else if (type == CONVTYPE_DATE_TIME)
    {
        // Apply the simplest heuristic: only digits and separators
        // and ignore characters, which cannot represent a date by themselves
        if (sStr.find_first_not_of(" 0123456789,.:-/\tTZ") != std::string::npos
            || sStr.find_first_not_of(" \t,.:-/TZ") == std::string::npos)
            return false;

        // Try to detect dates
        size_t pos = sStr.find_first_not_of(" \t");

        if (sStr.length() >= pos+3 && isdigit(sStr[pos]))
        {
            for (size_t i = pos; i < sStr.length()-1; i++)
            {
                // Detects these candidates:
                // (YY)YY-MM-DD, DD.MM.YY(YY), (yy)yy-m-d, d.m.yy(yy),
                // d.m., dd.mm., (YY)YY/MM/DD, (YY)YY/M/D,
                // hh:mm:ss, h:mm:ss, h:mm, hh:mm
                // hh:mm:ss:iii, h:mm:ss:iii, hh:mm:ss.iii, h:mm:ss.iii
                // YYYY-MM-DDThh:mm:ss.iiiZ
                if (sStr[i] == '-' || sStr[i] == '.' || sStr[i] == '/' || sStr[i] == ':')
                {
                    if (isDateTimePattern(sStr, i))
                        return true;

                    return false;
                }
            }
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Returns a formatting flag combination,
/// if the string is a US-specific date pattern
/// or TD_NONE.
///
/// \param sStr const std::string&
/// \param i size_t
/// \return int
///
/////////////////////////////////////////////////
static int isDatePattern_US(const std::string& sStr, size_t i)
{
    // Surrounded by digits?
    if (!i || !isdigit(sStr[i-1]) || !isdigit(sStr[i+1]))
        return TD_NONE;

    // Detects these candidates:
    // YY-MM-DD, yy-m-d, YY/MM/DD, YY/M/D, DD/MM/YYYY, D/M/YY(YY)
    if ((i+3 < sStr.length() && sStr[i+3] == sStr[i])
        || (i+2 < sStr.length() && sStr[i+2] == sStr[i]))
    {
        // Special case for DD/MM/YYYY, D/M/YY(YY) (note that the year has more characters)
        if ((sStr[i+3] == '/' && i+6 < sStr.length() && isdigit(sStr[i+6]))
            || (sStr[i+2] == '/' && i+4 < sStr.length() && isdigit(sStr[i+4])))
            return TD_DDMMYY | TD_SEP_SLASH;

        return TD_YYMMDD | (sStr[i] == '-' ? TD_SEP_MINUS : TD_SEP_SLASH);
    }

    return TD_NONE;
}


/////////////////////////////////////////////////
/// \brief Returns a formatting flag combination,
/// if the string is a DE-specific date pattern
/// or TD_NONE.
///
/// \param sStr const std::string&
/// \param i size_t
/// \return int
///
/////////////////////////////////////////////////
static int isDatePattern_DE(const std::string& sStr, size_t i)
{
    // Surrounded by digits?
    if (!i || !isdigit(sStr[i-1]) || !isdigit(sStr[i+1]))
        return TD_NONE;

    // Detects these candidates:
    // DD.MM.YY, d.m.yy
    if ((i+4 < sStr.length() && sStr[i+3] == sStr[i] && isdigit(sStr[i+4]))
        || (i+3 < sStr.length() && sStr[i+2] == sStr[i] && isdigit(sStr[i+3])))
        return TD_DDMMYY | TD_SEP_DOT;
    // d.m., dd.mm.
    else if ((i+3 < sStr.length() && sStr[i+3] == sStr[i])
             || (i+2 < sStr.length() && sStr[i+2] == sStr[i]))
        return TD_DDMM | TD_SEP_DOT;

    return TD_NONE;
}


/////////////////////////////////////////////////
/// \brief Returns a formatting flag combination,
/// if the string is a time pattern or TD_NONE.
///
/// \param sStr const std::string&
/// \param i size_t
/// \return int
///
/////////////////////////////////////////////////
static int isTimePattern(const std::string& sStr, size_t i)
{
    // Surrounded by digits?
    if (!i || !isdigit(sStr[i-1]) || !isdigit(sStr[i+1]))
        return TD_NONE;

    // Detects these candidates:
    // hh:mm:ss, h:mm:ss, hh:mm:ss:iii, h:mm:ss:iii, hh:mm:ss.iii, h:mm:ss.iii
    if ((i+3 < sStr.length() && sStr[i+3] == sStr[i])
        || (i+2 < sStr.length() && sStr[i+2] == sStr[i]))
        return TD_HHMMSS | TD_SEP_COLON;

    // h:mm, hh:mm
    return TD_HHMM | TD_SEP_COLON;
}


/////////////////////////////////////////////////
/// \brief Detects the contained date-time format
/// and returns it as a bitflag composition.
///
/// \param sStr const std::string&
/// \return int
///
/////////////////////////////////////////////////
int detectTimeDateFormat(const std::string& sStr)
{
    if (sStr.find_first_not_of(" 0123456789,.:-/\tTZ") != std::string::npos || sStr.find_first_not_of(" \t") == std::string::npos)
        return TD_NONE;

    // Try to detect dates
    size_t pos = sStr.find_first_not_of(" \t");
    int format = TD_NONE;

    if (sStr.length() >= pos+3 && isdigit(sStr[pos]))
    {
        for (size_t i = pos; i < sStr.length()-1; i++)
        {
            // Detects these candidates:
            // (YY)YY-MM-DD, (yy)yy-m-d, (YY)YY/MM/DD, (YY)YY/M/D, DD/MM/YYYY, D/M/YYYY
            if (sStr[i] == '-' || sStr[i] == '/')
                format |= isDatePattern_US(sStr, i);
            // DD.MM.YY(YY), d.m.yy(yy), d.m., dd.mm.
            else if (sStr[i] == '.')
                format |= isDatePattern_DE(sStr, i);
            // hh:mm:ss, h:mm:ss, h:mm, hh:mm
            // hh:mm:ss:iii, h:mm:ss:iii, hh:mm:ss.iii, h:mm:ss.iii
            else if (sStr[i] == ':')
                format |= isTimePattern(sStr, i);
        }
    }

    // If we have the dashed american theme and T and Z,
    // then we have a full UTC time stamp
    if (format & (TD_YYMMDD | TD_HHMMSS | TD_SEP_COLON | TD_SEP_MINUS)
        && sStr.find('T') != std::string::npos
        && sStr.find('Z') != std::string::npos)
        format |= TD_UTC;

    return format;
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// usual wcstombs function, which can handle
/// wstrings.
///
/// \param wStr const std::wstring&
/// \return std::string
///
/////////////////////////////////////////////////
std::string wcstombs(const std::wstring& wStr)
{
    std::string sReturn;

    // provide a target character array
    char* cBuf = new char[wStr.length() * 2 + 1];

    // transform the wstring into the target array
    size_t nRet = wcstombs(cBuf, wStr.c_str(), wStr.length() * 2 + 1);

    // Write the zero character and copy the contents of the character
    // array to the target string
    if (nRet == wStr.length() * 2 + 1)
        cBuf[wStr.length() * 2] = '\0';
    if (nRet)
        sReturn = cBuf;

    // Clear the memory and return the string
    delete[] cBuf;
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief Converts an internal to an external
/// string. Does nothing currently.
///
/// \param sOutput std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string toSystemCodePage(std::string sOutput)
{
    return sOutput;
}


/////////////////////////////////////////////////
/// \brief Transforms the system code page to the
/// internal one.
///
/// \param sOutput std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string fromSystemCodePage(std::string sOutput)
{
    for (size_t i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == (char)142)
            sOutput[i] = (char)0xC4;
        else if (sOutput[i] == (char)132)
            sOutput[i] = (char)0xE4;
        else if (sOutput[i] == (char)153)
            sOutput[i] = (char)0xD6;
        else if (sOutput[i] == (char)148)
            sOutput[i] = (char)0xF6;
        else if (sOutput[i] == (char)154)
            sOutput[i] = (char)0xDC;
        else if (sOutput[i] == (char)129)
            sOutput[i] = (char)0xFC;
        else if (sOutput[i] == (char)225)
            sOutput[i] = (char)0xDF;
        else if (sOutput[i] == (char)248)
            sOutput[i] = (char)0xB0;
        else if (sOutput[i] == (char)174)
            sOutput[i] = (char)171;
        else if (sOutput[i] == (char)175)
            sOutput[i] = (char)187;
        else
            continue;
    }
    return sOutput;
}


/////////////////////////////////////////////////
/// \brief Transforms a UTF8 encoded string into
/// a Win CP1252 string in the internal code
/// page representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string utf8parser(const std::string& sString)
{
    // Static declaration of constant
	static const unsigned char NONANSIBITMASK = 128;
    std::string sWinCp = sString;

    for (size_t i = 0; i < sWinCp.length(); i++)
    {
        if (sWinCp[i] & NONANSIBITMASK)
        {
            if (sWinCp[i] == (char)0xC2)
            {
                // Regl. ANSI > 127
                sWinCp.erase(i, 1);
            }
            else if (sWinCp[i] == (char)0xE2 && sWinCp.length() > i+2)
            {
                // These are the about 20 characters, which have other
                // code points in Win-CP1252 and unicode
                // U+20.. and U+21..

                // Trademark symbol
                if (sWinCp[i+1] == (char)0x84 && (sWinCp[i+2] & ~(char)0xC0) == (char)0x22)
                {
                    sWinCp.erase(i, 2);
                    sWinCp[i] = (char)0x99;
                    continue;
                }

                // All other cases
                char sChar = sWinCp[i+2] & ~(char)0xC0;
                sChar += (sWinCp[i+1] & (char)0x3) << 6;
                sWinCp.erase(i, 2);

                // Switch as lookup table
                switch (sChar)
                {
                    case (char)0xAC:
                        sWinCp[i] = (char)0x80;
                        break;
                    case (char)0x1A:
                        sWinCp[i] = (char)0x82;
                        break;
                    case (char)0x1E:
                        sWinCp[i] = (char)0x84;
                        break;
                    case (char)0x26:
                        sWinCp[i] = (char)0x85;
                        break;
                    case (char)0x20:
                        sWinCp[i] = (char)0x86;
                        break;
                    case (char)0x21:
                        sWinCp[i] = (char)0x87;
                        break;
                    case (char)0x30:
                        sWinCp[i] = (char)0x89;
                        break;
                    case (char)0x39:
                        sWinCp[i] = (char)0x8B;
                        break;
                    case (char)0x18:
                        sWinCp[i] = (char)0x91;
                        break;
                    case (char)0x19:
                        sWinCp[i] = (char)0x92;
                        break;
                    case (char)0x1C:
                        sWinCp[i] = (char)0x93;
                        break;
                    case (char)0x1D:
                        sWinCp[i] = (char)0x94;
                        break;
                    case (char)0x22:
                        sWinCp[i] = (char)0x95;
                        break;
                    case (char)0x13:
                        sWinCp[i] = (char)0x96;
                        break;
                    case (char)0x14:
                        sWinCp[i] = (char)0x97;
                        break;
                    case (char)0x3A:
                        sWinCp[i] = (char)0x9B;
                        break;
                }
            }
            else if (sWinCp[i] >= (char)0xC4 && sWinCp[i] <= (char)0xCB)
            {
                // These are the about 20 characters, which have other
                // code points in Win-CP1252 and unicode
                // U+01.. and U+02..
                char sChar = sWinCp[i+1] & ~(char)0xC0;
                sChar += (sWinCp[i] & (char)0x3) << 6;
                sWinCp.erase(i, 1);

                // Switch as lookup table
                switch (sChar)
                {
                    case (char)0x92:
                        sWinCp[i] = (char)0x83;
                        break;
                    case (char)0xC6:
                        sWinCp[i] = (char)0x88;
                        break;
                    case (char)0x60:
                        sWinCp[i] = (char)0x8A;
                        break;
                    case (char)0x52:
                        sWinCp[i] = (char)0x8C;
                        break;
                    case (char)0x7D:
                        sWinCp[i] = (char)0x8E;
                        break;
                    case (char)0x7E:
                        sWinCp[i] = (char)0x9E;
                        break;
                    case (char)0x78:
                        sWinCp[i] = (char)0x9F;
                        break;
                    case (char)0xDC:
                        sWinCp[i] = (char)0x98;
                        break;
                    case (char)0x61:
                        sWinCp[i] = (char)0x9A;
                        break;
                    case (char)0x53:
                        sWinCp[i] = (char)0x9C;
                        break;
                }
            }
            else if (sWinCp[i] == (char)0xC3)
            {
                // Regl. ANSI > 127
                sWinCp.erase(i, 1);
                sWinCp[i] += 64;
            }
        }
    }

    return sWinCp;
}


/////////////////////////////////////////////////
/// \brief Transforms a UTF8 encoded string into
/// a Win CP1252 string in the internal code
/// page representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string utf8ToAnsi(const std::string& sString)
{
    return utf8parser(sString);
}


/////////////////////////////////////////////////
/// \brief Creates an up to three byte sequence
/// UTF8 character from a two byte unicode
/// character. The length of the returned
/// byte sequence is stored in nLength.
///
/// \param cHighByte unsigned char
/// \param cLowByte unsigned char
/// \param nLength size_t&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createUTF8FromUnicode(unsigned char cHighByte, unsigned char cLowByte, size_t& nLength)
{
    std::string sUTF8;

    // If the high byte is larger than or equal to 0x08, we will
    // return three bytes, otherwise only two bytes
    if (cHighByte >= 0x08)
    {
        nLength = 3;
        sUTF8 = "   ";
        sUTF8[0] = (char)0xE0 | ((cHighByte & (unsigned char)0xF0) >> 4);
        sUTF8[1] = (char)0x80 | ((cHighByte & (unsigned char)0x0F) << 2) | ((cLowByte & (unsigned char)0xC0) >> 6);
        sUTF8[2] = (char)0x80 | (cLowByte & ~(char)0xC0);
    }
    else
    {
        nLength = 2;
        sUTF8 = "  ";
        sUTF8[0] = (char)0xC0 | ((cHighByte & (unsigned char)0x07) << 2) | ((cLowByte & (unsigned char)0xC0) >> 6);
        sUTF8[1] = (char)0x80 | (cLowByte & ~(char)0xC0);
    }

    return sUTF8;
}


/////////////////////////////////////////////////
/// \brief Transforms a Win CP1253 encoded string
/// into a UTF8 string in the internal code page
/// representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string ansiToUtf8(const std::string& sString)
{
    // Ensure that element text is available
    if (!sString.length())
        return std::string();

    // convert current element into a regular string
    // object (much easier to handle)
    std::string sUtf8String = sString;

    // static declarations of constants
    static const unsigned char NONANSIBITMASK = 128;
    static const unsigned char TWOBYTEUTF8 = 0xA0;

    // Go through the complete xml text and replace the
    // characters with the UTF8 encoded unicode value. The
    // unicode value of Win-CP1252 and unicode is quite similar
    // except of a set of about 24 characters between NONANSIBITMASK
    // and TWOBYTEUTF8
    for (size_t i = 0; i < sUtf8String.length(); i++)
    {
        // Only do something, if the character is larger than NONANSIBITMASK
        if (sUtf8String[i] & NONANSIBITMASK)
        {
            if ((unsigned char)sUtf8String[i] >= TWOBYTEUTF8)
            {
                // Regular encoding
                char cp1252Char = sUtf8String[i];
                sUtf8String.replace(i, 1, "  ");
                sUtf8String[i] = (char)0xC0 | ((cp1252Char & (unsigned char)0xC0) >> 6);
                sUtf8String[i+1] = (char)0x80 | (cp1252Char & ~(char)0xC0);
                i++;
            }
            else
            {
                // Special code sequences. These have other code points in UTF8
                // and therefore we have to replace them manually
                char cp1252Char = sUtf8String[i];
                size_t nLength = 0;

                // Switch as lookup table
                switch (cp1252Char)
                {
                    case (char)0x80:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0xAC, nLength));
                        break;
                    case (char)0x82:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1A, nLength));
                        break;
                    case (char)0x83:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x92, nLength));
                        break;
                    case (char)0x84:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1E, nLength));
                        break;
                    case (char)0x85:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x26, nLength));
                        break;
                    case (char)0x86:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x20, nLength));
                        break;
                    case (char)0x87:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x21, nLength));
                        break;
                    case (char)0x88:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x02, 0xC6, nLength));
                        break;
                    case (char)0x89:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x30, nLength));
                        break;
                    case (char)0x8A:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x60, nLength));
                        break;
                    case (char)0x8B:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x39, nLength));
                        break;
                    case (char)0x8C:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x52, nLength));
                        break;
                    case (char)0x8E:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x7D, nLength));
                        break;
                    case (char)0x91:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x18, nLength));
                        break;
                    case (char)0x92:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x19, nLength));
                        break;
                    case (char)0x93:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1C, nLength));
                        break;
                    case (char)0x94:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1D, nLength));
                        break;
                    case (char)0x95:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x22, nLength));
                        break;
                    case (char)0x96:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x13, nLength));
                        break;
                    case (char)0x97:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x14, nLength));
                        break;
                    case (char)0x98:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x02, 0xDC, nLength));
                        break;
                    case (char)0x99:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x21, 0x22, nLength));
                        break;
                    case (char)0x9A:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x61, nLength));
                        break;
                    case (char)0x9B:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x3A, nLength));
                        break;
                    case (char)0x9C:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x53, nLength));
                        break;
                    case (char)0x9E:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x7E, nLength));
                        break;
                    case (char)0x9F:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x78, nLength));
                        break;
                }

                i += nLength-1;

            }
        }
    }

    return sUtf8String;
}


/////////////////////////////////////////////////
/// \brief This function simple returns the
/// current time as a default timestamp.
///
/// \param bGetStamp bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string getTimeStamp(bool bGetStamp)
{
    return toString(_time64(0), bGetStamp ? GET_AS_TIMESTAMP : GET_WITH_TEXT);
}


/////////////////////////////////////////////////
/// \brief Splits a vector at the selected
/// characters.
///
/// \param sStr const std::string&
/// \param cSplit char
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> split(const std::string& sStr, char cSplit)
{
    std::vector<std::string> vSplit;
    size_t lastPos = 0;

    for (size_t i = 1; i < sStr.length(); i++)
    {
        if (sStr[i] == cSplit)
        {
            vSplit.push_back(sStr.substr(lastPos, i - lastPos));
            lastPos = i+1;
        }
    }

    if (lastPos < sStr.length())
        vSplit.push_back(sStr.substr(lastPos));

    return vSplit;
}


/////////////////////////////////////////////////
/// \brief Shortens the passed string by
/// introducing a ellipsis in the middle of the
/// string, if the string is longer than the
/// maximal string length.
///
/// \param sLongString const std::string&
/// \param nMaxStringLength size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string ellipsize(const std::string& sLongString, size_t nMaxStringLength)
{
    if (sLongString.length() > nMaxStringLength)
        return sLongString.substr(0, nMaxStringLength/2-2) + "[...]" + sLongString.substr(sLongString.length()-nMaxStringLength+2);

    return sLongString;
}


/////////////////////////////////////////////////
/// \brief This function replaces all occurences
/// of the string sToRep in the string sToModify
/// with the new value sNewValue. The boundaries
/// limit the range of processing.
///
/// \param sToModify MutableStringView
/// \param sToRep const char*
/// \param sNewValue const char*
/// \param nStart size_t
/// \param nEnd size_t
/// \return void
///
/////////////////////////////////////////////////
void replaceAll(MutableStringView sToModify, const char* sToRep, const char* sNewValue, size_t nStart /*= 0*/, size_t nEnd /*= string::npos*/)
{
    size_t nRepLength = strlen(sToRep);
    size_t nNewLength = strlen(sNewValue);
    int nOffSet = nNewLength - nRepLength;
    // Ensure the values are correct
    if (!sToModify.length() || !nRepLength)
        return;

    // check the boundaries
    if ((size_t)nStart > sToModify.length())
        return;

    if (nEnd == std::string::npos)
        nEnd = sToModify.length();

    // Process the replacing
    for (size_t i = nStart; i <= nEnd-nRepLength; i++)
    {
        if (i >= sToModify.length())
            break;

        if (sToModify.match(sToRep, i))
        {
            sToModify.replace(i, nRepLength, sNewValue);
            nEnd += nOffSet;
            i += nNewLength - 1;
        }
    }
}

/////////////////////////////////////////////////
/// \brief This function replaces all occurences
/// of the string sToRep in the string sToModify
/// with the new value sNewValue. The boundaries
/// limit the range of processing. This function
/// is a (slower) overload for std::strings.
///
/// \param sToModify MutableStringView
/// \param sToRep StringView
/// \param sNewValue StringView
/// \param nStart size_t
/// \param nEnd size_t
/// \return void
///
/////////////////////////////////////////////////
void replaceAll(MutableStringView sToModify, StringView sToRep, StringView sNewValue, size_t nStart /*= 0*/, size_t nEnd /*= string::npos*/)
{
    size_t nRepLength = sToRep.length();
    size_t nNewLength = sNewValue.length();
    int nOffSet = nNewLength - nRepLength;
    // Ensure the values are correct
    if (!sToModify.length() || !nRepLength)
        return;

    // check the boundaries
    if ((size_t)nStart > sToModify.length())
        return;

    if (nEnd == std::string::npos)
        nEnd = sToModify.length();

    // Process the replacing
    for (size_t i = nStart; i <= nEnd-nRepLength; i++)
    {
        if (i >= sToModify.length())
            break;

        if (sToModify.match(sToRep, i))
        {
            sToModify.replace(i, nRepLength, sNewValue);
            nEnd += nOffSet;
            i += nNewLength - 1;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function is a simple wrapper for
/// replaceAll() and specialized to remove
/// control characters from strings.
///
/// \param sToModify std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string replaceControlCharacters(std::string sToModify)
{
    replaceAll(sToModify, "\n", "\\n");
    replaceAll(sToModify, "\t", "\\t");

    return sToModify;
}

/////////////////////////////////////////////////
/// \brief This function changes Number Format inside a String
///
/// \param sNum std::string&
/// \param numFormat NumberFormat
/// \return void
///
/////////////////////////////////////////////////
void strChangeNumberFormat(std::string &sNum, int numFormat){

    if(numFormat & NUM_K_EU)
        replaceAll(sNum, ".", "");

    if(numFormat & NUM_K_US)
        replaceAll(sNum, ",", "");

    if(numFormat & NUM_K_SPACE)
        replaceAll(sNum, " ", "");

    if(numFormat & NUM_DECIMAL_EU)
        replaceAll(sNum, ",", ".");
}


/////////////////////////////////////////////////
/// \brief This function replaces the Windows
/// style path sparators to UNIX style.
///
/// \param __sPath const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string replacePathSeparator(const std::string& __sPath)
{
    std::string sPath = __sPath;

    // Go through the whole string
    for (size_t i = 0; i < sPath.length(); i++)
    {
        // Replace the backslash
        if (sPath[i] == '\\')
            sPath[i] = '/';
    }

    return sPath;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the contents of the
/// strings are equal, if all surrounding
/// whitespaces will be stripped.
///
/// \param str1 StringView
/// \param str2 StringView
/// \return bool
///
/////////////////////////////////////////////////
bool isEqualStripped(StringView str1, StringView str2)
{
    str1.strip();
    str2.strip();

    return str1 == str2;
}


