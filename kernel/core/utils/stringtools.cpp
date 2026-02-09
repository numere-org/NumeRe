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
#include "../../../externals/base-n/include/basen.hpp"
#include "../../../common/markup.hpp"

#include <fast_float/fast_float.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <boost/tokenizer.hpp>
#include <boost/locale.hpp>

// Forward declarations
std::string getNextArgument(std::string& sArgList, bool bCut);
double intPower(double, int64_t);
bool isInt(const std::complex<double>& number);


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
    std::ostringstream Temp;
    Temp << nNumber;
    return Temp.str();
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
    return toString(getTimePointFromTimeStamp(getTimeStampFromTime_t(tTime)), timeStampFlags);
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

    if (timeStampFlags & GET_SHORTEST)
    {
        //if (to_double(tp) < 24*3600)
        //    timeStampFlags |= GET_ONLY_TIME;

        if (!(timeStampFlags & GET_ONLY_TIME)
            && !ltm.m_hours.count()
            && !ltm.m_minutes.count()
            && !ltm.m_seconds.count()
            && !ltm.m_millisecs.count()
            && !ltm.m_microsecs.count())
            timeStampFlags |= GET_ONLY_DATE;
    }

    if (!(timeStampFlags & GET_ONLY_TIME))
    {
        timeStream << ltm.m_ymd.year() << "-"; //YYYY-
        timeStream << std::setfill('0') << std::setw(2) << unsigned(ltm.m_ymd.month()) << "-"; // MM-
        timeStream << ltm.m_ymd.day(); 	// DD

        if (!(timeStampFlags & GET_ONLY_DATE))
        {
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
    }

    if (!(timeStampFlags & GET_ONLY_DATE))
    {
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
/// \brief Converts a double into a formatted
/// duration string.
///
/// \param dDuration double
/// \return std::string
///
/////////////////////////////////////////////////
std::string formatDuration(double dDuration)
{
    if (std::isnan(dDuration))
        return "---";

    auto intCast = [](double dur){ return std::abs(dur - rint(dur)) < 1e-7 ? (int)std::rint(dur) : static_cast<int>(dur);};
    std::string sDuration;
    bool isNegative = dDuration < 0.0;

    constexpr double YEAR = 365.25*24*3600;
    constexpr double WEEK = 7.0*24*3600;
    constexpr double DAY = 24.0*3600;

    dDuration = std::abs(dDuration);

    int nYears = std::floor(dDuration / YEAR);
    dDuration -= nYears * YEAR;

    int nWeeks = std::floor(dDuration / WEEK);
    dDuration -= nWeeks * WEEK;

    int nDays = std::floor(dDuration / DAY);
    dDuration -= nDays * DAY;

    int nHours = std::floor(dDuration / 3600.0);
    dDuration -= nHours * 3600.0;

    int nMinutes = std::floor(dDuration / 60.0);
    dDuration -= nMinutes * 60.0;

    int nSeconds = (int)dDuration;
    dDuration -= nSeconds;

    // There might be a rollover in the decimals, if we
    // cast them to an integer
    if (intCast(dDuration*1000) == 1000)
    {
        nSeconds++;
        dDuration = 0;

        if (nSeconds == 60)
        {
            nMinutes++;
            nSeconds = 0;
        }

        if (nMinutes == 60)
        {
            nHours++;
            nMinutes = 0;
        }

        if (nHours == 24)
        {
            nDays++;
            nHours = 0;
        }

        if (nDays == 7)
        {
            nWeeks++;
            nDays = 0;
        }
    }

    if (isNegative)
        sDuration += "-";

    if (nYears)
        sDuration += toString(nYears) + "y ";

    if (nWeeks)
        sDuration += toString(nWeeks) + "wk ";

    if (nDays)
        sDuration += toString(nDays) + "d ";

    sDuration += toString(nHours) + ":" + strfill(toString(nMinutes), 2, '0') + ":" + strfill(toString(nSeconds), 2, '0');

    if (dDuration > 0.0)
    {
        dDuration *= 1000;
        int msec = intCast(dDuration);
        dDuration -= msec;

        sDuration += "." + strfill(toString(msec), 3, '0');

        dDuration *= 1000;
        int musec = intCast(dDuration);

        if (musec)
            sDuration += strfill(toString(musec), 3, '0');
    }

    return sDuration;
}


/////////////////////////////////////////////////
/// \brief Formats a time-point according RFC
/// 5322.
///
/// \param tp sys_time_point
/// \return std::string
///
/////////////////////////////////////////////////
std::string formatRfc5322(sys_time_point tp)
{
    time_stamp ltm = getTimeStampFromTimePoint(tp);
    time_zone tz = getCurrentTimeZone();

    bool neg = (tz.Bias + tz.DayLightBias).count() > 0; // bias is oriented the other way around

    int biasHours = std::abs((tz.Bias + tz.DayLightBias).count() / 60);
    int biasMinutes = std::abs((tz.Bias + tz.DayLightBias).count()) - 60 * biasHours;

    static const std::string MONTHNAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    std::ostringstream timeStream;

    // Mon, 29 Nov 2010 21:54:29 +1100
    timeStream << std::setfill('0') << std::setw(2) << unsigned(ltm.m_ymd.day()) << " ";
    timeStream << MONTHNAMES[(unsigned)ltm.m_ymd.month()-1] << " ";
    timeStream << ltm.m_ymd.year() << " ";
    timeStream << std::setfill('0') << std::setw(2) << ltm.m_hours.count() << ":"
        << std::setfill('0') << std::setw(2) << ltm.m_minutes.count() << ":"
        << std::setfill('0') << std::setw(2) << ltm.m_seconds.count();

    if (biasHours || biasMinutes)
        timeStream << (neg ? " -" : " +") << std::setfill('0') << std::setw(2) << biasHours << std::setfill('0') << std::setw(2) << biasMinutes;

    return timeStream.str();
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
    if (countUnicodePoints(sText) <= nMaxChars)
        return sText;

    return sText.substr(0, findClosestCharStart(sText, nMaxChars-3)) + "...";
}


/////////////////////////////////////////////////
/// \brief This function fills the passed string
/// up to the width nWidth with the characters
/// cFill. The string will be aligned
/// right. If the bool option limit is true,
/// the string will be shortened to the required
/// size and '...' will be added.
///
/// \param sString const std::string&
/// \param nWidth size_t
/// \param cFill char
/// \param limit bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string strfill(const std::string& sString, size_t nWidth, char cFill, bool limit)
{
    if (!nWidth)
        return "";

    std::string sReturn = sString;
    size_t length = countUnicodePoints(sString);

    // Fill the string
    if (length < nWidth)
        sReturn.insert(0, nWidth-length, cFill);

    // Limit the output size if required
    if (limit && length > nWidth)
        sReturn = sString.substr(0, findClosestCharStart(sString, nWidth - 3)) + "...";

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function fills the passed string
/// up to the width nWidth with the characters
/// cFill. The string will be aligned left.
///
/// \param sString const std::string&
/// \param nWidth size_t
/// \param cFill char
/// \return std::string
///
/////////////////////////////////////////////////
std::string strlfill(const std::string& sString, size_t nWidth, char cFill)
{
    if (!nWidth)
        return "";

    std::string sReturn = sString;
    size_t length = sString.length();

    if (length < nWidth)
        sReturn.append(nWidth-length, cFill);

    return sReturn;
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
    const char* DIGITS_MINUS = "-0123456789";

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
            isFirst = sString.find('-') == sString.find_first_not_of(DIGITS, pos);
        }
        else
        {
            toks = split(sString, '/');
            isFirst = sString.find('/') == sString.find_first_not_of(DIGITS_MINUS, pos);
        }

        for (size_t i = 0; i < toks.size(); i++)
        {
            StripSpaces(toks[i]);

            // Remove leading or trailing non-date chars
            if (toks[i].find_first_not_of(DIGITS_MINUS) != std::string::npos)
            {
                if ((isFirst && i+1 != toks.size()) || (!isFirst && i))
                    break;

                if (isFirst)
                    toks[i].erase(toks[i].find_first_not_of(DIGITS_MINUS));
                else
                    toks[i].erase(0, toks[i].find_last_not_of(DIGITS_MINUS)+1);
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
/// multi-digit double
///
/// \param sVersion std::string
/// \return double
///
/////////////////////////////////////////////////
double versionToFloat(std::string sVersion)
{
    size_t nVOffset = 0;

    if (sVersion.front() == 'v')
        nVOffset = 1;

    size_t dots = 0;

    // Determine, whether we have a trailing variant
    for (char c : sVersion)
    {
        if (c == '.')
            dots++;

        if (dots > 2)
            break;
    }

    if (dots > 2)
        replaceAll(sVersion, ".", "", 0, sVersion.rfind('.'));
    else
        replaceAll(sVersion, ".", "");

    return StrToDb(sVersion.substr(nVOffset));
}


/////////////////////////////////////////////////
/// \brief Converts a multi-digit double into a
/// version string.
///
/// \param fVersionDigits double
/// \return std::string
///
/////////////////////////////////////////////////
std::string floatToVersion(double fVersionDigits)
{
    // Just use the int part first
    std::string _sVer = toString(int(fVersionDigits));

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

    // if we have a variant, append it
    if (!isInt(fVersionDigits))
    {
        _sVer += "." + toString(int((fVersionDigits - int(fVersionDigits))*1000));
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

        if (sStr.compare(i, 2, "\\r") == 0
            && sStr.compare(i, 4, "\\rho") != 0)
            sStr.replace(i, 2, "\r");

        if (sStr.compare(i, 2, "\\\"") == 0)
            sStr.replace(i, 2, "\"");

        if (sStr.compare(i, 2, "\\\\") == 0)
            sStr.replace(i, 2, "\\");
        else if (sStr.compare(i, 2, "\\ ") == 0)
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
    boost::locale::generator gen;
#warning TODO (numere#1#01/21/26): What to do with the locales?
    std::locale loc = gen("en_US.UTF-8");

    return boost::locale::to_lower(sUpperCase, loc);
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
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");

#warning TODO (numere#1#01/21/26): Boost::locale needs to be rebuild with ICU support
    return boost::locale::to_upper(sLowerCase, loc);
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
    if (!pos || !(isdigit(sStr[pos-1]) && isdigit(sStr[pos+1])))
        return false;

    // Scan for the following non-digit. This must be either the
    // same separator (occuing in pairs) or a group separator
    // between date and time
    for (size_t i = pos+1; i < sStr.length(); i++)
    {
        if (!isdigit(sStr[i]))
            return i <= pos+3 && (sStr[i] == sStr[pos] || sStr[i] == ' ' || sStr[i] == ',' || sStr[i] == '\t' || sStr[i] == 'T');
    }

    // digit colon digit is unique enough
    if (sStr[pos] == ':')
        return true;

    return false;
}

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
bool isConvertible(const std::string& sStr, ConvertibleType type, NumberFormatsVoter* voter)
{
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

        // Regression fix introduced because NA is accepted as NaNhow to
        for (size_t i = 0; i < sStr.length(); i++)
        {
            if (voter && (sStr[i] == ',' || sStr[i] == '.' || sStr[i] == ' ' || isdigit(sStr[i])))
            {
                if (!inNum && sStr[i] != '0')
                {
                    inNum = true;
                    voter->startParseNumber(i);
                }

                if (sStr[i] == ',' || sStr[i] == '.' || sStr[i] == ' ' )
                    voter->addSeperator(sStr[i], i);
            }
            else
            {
                if (voter && inNum)
                {
                    voter->endParseAndVote(i-1);  // last one was one before
                    inNum = false;
                }

                if (i > 0 && i-1 < sStr.length() && (sStr[i] == '-' || sStr[i] == '+'))
                {
                    if (tolower(sStr[i-1]) != 'e'
                        && (isdigit(sStr[i-1]) && sStr.find_first_of("iI", i+1) == std::string::npos)
                        && sStr[i-1] != ' ')
                        return false;
                }
            }
        }

        if (voter && inNum)
            voter->endParseAndVote(sStr.length()-1);

        if (!sStr.length() || toLowerCase(sStr) == "nan" || sStr == "---")
            return true;

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
        // NaN is also allowed
        if (!sStr.length() || toLowerCase(sStr) == "nan" || sStr == "---")
            return true;

        if (sStr.find_first_not_of(" 0123456789,.:-/\tTZ") != std::string::npos
            || sStr.find_first_not_of(" \t,.:-/TZ") == std::string::npos)
            return false;

        // Try to detect dates
        size_t pos = sStr.find_first_not_of(" \t");

        if (sStr.length() >= pos+3
            && (isdigit(sStr[pos]) || (sStr[pos] == '-' && isdigit(sStr[pos+1])))) // might be negative year
        {
            if (sStr[pos] == '-')
                pos++;

            for (size_t i = pos; i < sStr.length()-1; i++)
            {
                // A date-time value may not contain single values
                // separated by a simple whitespace
                if (sStr[i] == ' ' || sStr[i] == '\t')
                    return false;

                // Detects these candidates:
                // (YY)YY-MM-DD, DD.MM.YY(YY), (yy)yy-m-d, d.m.yy(yy),
                // d.m., dd.mm., (YY)YY/MM/DD, (YY)YY/M/D,
                // hh:mm:ss, h:mm:ss, h:mm, hh:mm
                // hh:mm:ss:iii, h:mm:ss:iii, hh:mm:ss.iii, h:mm:ss.iii
                // YYYY-MM-DDThh:mm:ss.iiiZ
                if (sStr[i] == '-' || sStr[i] == '.' || sStr[i] == '/' || sStr[i] == ':')
                    return isDateTimePattern(sStr, i);
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

    // May not be preceded by a colon: hh:yy-mm-dd or by a previously
    // examined date format
    int p = i-1;

    while (p >= 0)
    {
        if (!isdigit(sStr[p]))
        {
            if (sStr[p] == ':' || sStr[p] == '/')
                return TD_NONE;

            // Ensure that a minus sign is not used as a negative
            // year
            if (sStr[p] == '-' && p > 0 && isdigit(sStr[p-1]))
                return TD_NONE;

            break;
        }

        p--;
    }

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

    // May not be preceded by a colon: hh:d.m.yy or by a previously
    // examined date format
    int p = i-1;

    while (p >= 0)
    {
        if (!isdigit(sStr[p]))
        {
            if (sStr[p] == ':' || sStr[p] == '.')
                return TD_NONE;

            break;
        }

        p--;
    }

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

    // Invalidate such combinations: hh:mm.ss or h:mm.ss
    if ((i+3 < sStr.length() && sStr[i+3] == '.')
        || (i+2 < sStr.length() && sStr[i+2] == '.'))
        return TD_NONE;

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

    if (sStr.length() >= pos+3 && (isdigit(sStr[pos]) || sStr[pos] == '-')) // consider negative years
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
/// \brief Convert markup'ed text into an HTML
/// representation-.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string markupToHtml(const std::string& sString)
{
    std::vector<Markup::Token<std::string>> markupTokens = Markup::decode(sString);
    std::string cellValue;

    for (size_t i = 0; i < markupTokens.size(); i++)
    {
        if (i && markupTokens[i-1].line != markupTokens[i].line)
        {
            // Close the tags in the correct order
            if (!(markupTokens[i].inLine & Markup::EMPH) && i && markupTokens[i-1].inLine & Markup::EMPH)
                cellValue += "</span>";

            if (!(markupTokens[i].inLine & Markup::BRCKT) && i && markupTokens[i-1].inLine & Markup::BRCKT)
                cellValue += "</span>";

            if (!(markupTokens[i].inLine & Markup::CODE) && i && markupTokens[i-1].inLine & Markup::CODE)
                cellValue += "</code>";

            if (!(markupTokens[i].inLine & Markup::BOLD) && i && markupTokens[i-1].inLine & Markup::BOLD)
                cellValue += "</b>";

            if (!(markupTokens[i].inLine & Markup::ITALICS) && i && markupTokens[i-1].inLine & Markup::ITALICS)
                cellValue += "</i>";

            if (markupTokens[i-1].line == Markup::H1)
                cellValue += "</h1>";
            else if (markupTokens[i-1].line == Markup::H2)
                cellValue += "</h2>";
            else if (markupTokens[i-1].line == Markup::H3)
                cellValue += "</h3>";
            else if (markupTokens[i-1].line == Markup::UL)
                cellValue += "</li></ul>";
        }

        if (!i || markupTokens[i-1].line != markupTokens[i].line)
        {
            if (markupTokens[i].line == Markup::H1)
                cellValue += "<h1>";
            if (markupTokens[i].line == Markup::H2)
                cellValue += "<h2>";
            if (markupTokens[i].line == Markup::H3)
                cellValue += "<h3>";
            if (markupTokens[i].line == Markup::UL)
                cellValue += "<ul><li>";
        }

        if (markupTokens[i].inLine & Markup::ITALICS && (!i || !(markupTokens[i-1].inLine & Markup::ITALICS)))
            cellValue += "<i>";

        if (markupTokens[i].inLine & Markup::BOLD && (!i || !(markupTokens[i-1].inLine & Markup::BOLD)))
            cellValue += "<b>";

        if (markupTokens[i].inLine & Markup::CODE && (!i || !(markupTokens[i-1].inLine & Markup::CODE)))
            cellValue += "<code style=\"color:#00008B; background-color: #DDDDDD;\">";

        if (markupTokens[i].inLine & Markup::BRCKT && (!i || !(markupTokens[i-1].inLine & Markup::BRCKT)))
            cellValue += "<span style=\"color:#0000FF;\">";

        if (markupTokens[i].inLine & Markup::EMPH && (!i || !(markupTokens[i-1].inLine & Markup::EMPH)))
            cellValue += "<span style=\"background-color: #FFFFA4;\">";

        // Close the tags in the correct order
        if (!(markupTokens[i].inLine & Markup::EMPH) && i && markupTokens[i-1].inLine & Markup::EMPH)
            cellValue += "</span>";

        if (!(markupTokens[i].inLine & Markup::BRCKT) && i && markupTokens[i-1].inLine & Markup::BRCKT)
            cellValue += "</span>";

        if (!(markupTokens[i].inLine & Markup::CODE) && i && markupTokens[i-1].inLine & Markup::CODE)
            cellValue += "</code>";

        if (!(markupTokens[i].inLine & Markup::BOLD) && i && markupTokens[i-1].inLine & Markup::BOLD)
            cellValue += "</b>";

        if (!(markupTokens[i].inLine & Markup::ITALICS) && i && markupTokens[i-1].inLine & Markup::ITALICS)
            cellValue += "</i>";

        cellValue += markupTokens[i].text;
    }

    // Close the tags in the correct order
    if (markupTokens.back().inLine & Markup::EMPH)
        cellValue += "</span>";

    if (markupTokens.back().inLine & Markup::BRCKT)
        cellValue += "</span>";

    if (markupTokens.back().inLine & Markup::CODE)
        cellValue += "</code>";

    if (markupTokens.back().inLine & Markup::BOLD)
        cellValue += "</b>";

    if (markupTokens.back().inLine & Markup::ITALICS)
        cellValue += "</i>";

    // Close remaining open sections
    if (markupTokens.back().line == Markup::H1)
        cellValue += "</h1>";
    else if (markupTokens.back().line == Markup::H2)
        cellValue += "</h2>";
    else if (markupTokens.back().line == Markup::H3)
        cellValue += "</h3>";
    else if (markupTokens.back().line == Markup::UL)
        cellValue += "</li></ul>";

    replaceAll(cellValue, "</ul><ul>", "");
    replaceAll(cellValue, "\n<ul>", "<ul>");
    replaceAll(cellValue, "\n</li>", "</li>");
    replaceAll(cellValue, "\n", "<br/>");

    return cellValue;
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
        return sLongString.substr(0, nMaxStringLength/2-2) + "[...]" + sLongString.substr(sLongString.length()-nMaxStringLength/2+2);

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
/// \brief Repeat a string nCount times.
///
/// \param sStr const std::string&
/// \param nCount int
/// \return std::string
///
/////////////////////////////////////////////////
std::string strRepeat(const std::string& sStr, int nCount)
{
    if (nCount <= 0)
        return "";

    std::string ret;

    while (nCount)
    {
        ret += sStr;
        nCount--;
    }

    return ret;
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
    replaceAll(sToModify, "\\", "\\\\");
    replaceAll(sToModify, "\n", "\\n");
    replaceAll(sToModify, "\t", "\\t");
    replaceAll(sToModify, "\r", "\\r");

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


/////////////////////////////////////////////////
/// \brief This function removes the escape
/// characters from the passed string.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string removeMaskedStrings(const std::string& sString)
{
    std::string sRet = sString;

    // Go through the string and remove all relevant escape characters
    // Omit the characters, which are identifying LaTeX command sequences
    for (size_t i = 0; i < sRet.length(); i++)
    {
        if (sRet.compare(i, 2, "\\\"") == 0)
            sRet.erase(i, 1);

        if (sRet.compare(i, 2, "\\t") == 0
            && sRet.compare(i, 4, "\\tau") != 0
            && sRet.compare(i, 6, "\\theta") != 0
            && sRet.compare(i, 6, "\\times") != 0)
            sRet.replace(i, 2, "\t");

        if (sRet.compare(i, 2, "\\n") == 0
            && sRet.compare(i, 3, "\\nu") != 0)
            sRet.replace(i, 2, "\n");

        if (sRet.compare(i, 2, "\\ ") == 0)
            sRet.erase(i + 1, 1);
    }

    return sRet;
}


/////////////////////////////////////////////////
/// \brief This function simply removes the
/// surrounding quotation marks.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string removeQuotationMarks(const std::string& sString)
{
    if (sString.find('"') == std::string::npos || sString.front() != '"' || sString.back() != '"')
        return sString;

    return sString.substr(1, sString.length() - 2);
}


/////////////////////////////////////////////////
/// \brief Encode a string or some file's
/// contents into a Base-n-encoded string.
///
/// \param sToEncode const std::string&
/// \param isFile bool
/// \param n int
/// \return std::string
///
/////////////////////////////////////////////////
std::string encode_base_n(const std::string& sToEncode, bool isFile, int n)
{
    std::string sEncoded;

    if (isFile)
    {
        boost::nowide::ifstream file(sToEncode);

#ifndef PARSERSTANDALONE
        if (!file.good())
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sToEncode, sToEncode);
#endif

        // istreambuf_iterators do not skip whitespaces
        std::istreambuf_iterator<char> eos;
        std::istreambuf_iterator<char> f(file);

        switch (n)
        {
            case 16:
                bn::encode_b16(f, eos, std::back_inserter(sEncoded));
                break;
            case 32:
                bn::encode_b32(f, eos, std::back_inserter(sEncoded));
                break;
            case 64:
                bn::encode_b64(f, eos, std::back_inserter(sEncoded));
                break;
            default:
                throw std::runtime_error("Encoding with this n is no supported.");
        }
    }
    else
    {
        switch (n)
        {
            case 16:
                bn::encode_b16(sToEncode.begin(), sToEncode.end(), std::back_inserter(sEncoded));
                break;
            case 32:
                bn::encode_b32(sToEncode.begin(), sToEncode.end(), std::back_inserter(sEncoded));
                break;
            case 64:
                bn::encode_b64(sToEncode.begin(), sToEncode.end(), std::back_inserter(sEncoded));
                break;
            default:
                throw std::runtime_error("Encoding with this n is no supported.");
        }
    }

    // Append padding
    if (sEncoded.length() % 4)
        sEncoded.append(4 - sEncoded.length() % 4, '=');

    return sEncoded;
}


/////////////////////////////////////////////////
/// \brief Decode an Base-n-encoded string.
///
/// \param sToDecode const std::string&
/// \param n int
/// \return std::string
///
/////////////////////////////////////////////////
std::string decode_base_n(const std::string& sToDecode, int n)
{
    std::string sDecoded;

    switch (n)
    {
        case 16:
            bn::decode_b16(sToDecode.begin(), sToDecode.end(), std::back_inserter(sDecoded));
            break;
        case 32:
            bn::decode_b32(sToDecode.begin(), sToDecode.end(), std::back_inserter(sDecoded));
            break;
        case 64:
            bn::decode_b64(sToDecode.begin(), sToDecode.end(), std::back_inserter(sDecoded));
            break;
        default:
            throw std::runtime_error("Decoding with this n is no supported.");
    }

    return sDecoded;
}


/////////////////////////////////////////////////
/// \brief Implementation for STR.chr()
///
/// \param sString const std::string&
/// \param nthChar int64_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string chr_impl(const std::string& sString, int64_t nthChar)
{
    nthChar = std::max(nthChar, 0LL);

    size_t p = 0;
    size_t pOld = 0;

    // Find the correct character
    while (nthChar && p < sString.length())
    {
        pOld = p;
        p += getUtf8ByteLen(sString[p]);
        nthChar--;
    }

    // Ensure that we have a valid character position
    if (nthChar || p >= sString.length())
        p = pOld;

    // Extract the correct string length
    return sString.substr(p, getUtf8ByteLen(sString[p]));
}


/////////////////////////////////////////////////
/// \brief Implementation for substr()
///
/// \param sString const std::string&
/// \param p size_t
/// \param len size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string substr_impl(const std::string& sString, size_t p, size_t len)
{
    if (!sString.length() || p >= sString.length())
        return "";

    return sString.substr(std::max(0ULL, p), len);
}


/////////////////////////////////////////////////
/// \brief Implementation for split()
///
/// \param sString const std::string&
/// \param c const std::string&
/// \param keepEmpty bool
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> split_impl(const std::string& sString, const std::string& c, bool keepEmpty)
{
    std::vector<std::string> ret;

    if (!sString.length() || !c.length())
    {
        ret.emplace_back(sString);
        return ret;
    }

    std::vector<std::string> vUtf8Chars = splitUtf8Chars(c);
    size_t p0 = 0;

    for (size_t i = 0; i < sString.length(); i++)
    {
        size_t len = 0;

        if (getUtf8ByteLen(sString[i]) > 0ull
            && (len = matchesAny(sString, vUtf8Chars, i)))
        {
            if (p0 < i || keepEmpty)
                ret.emplace_back(sString.substr(p0, i-p0));

            p0 = i+len;
            i += len-1;
        }
    }

    if (p0 < sString.length() || keepEmpty)
        ret.emplace_back(sString.substr(p0));

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation for strfnd()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t strfnd_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = 0;

    return sString.find(sFind, p)+1;
}


/////////////////////////////////////////////////
/// \brief Implementation for strrfnd()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t strrfnd_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = sString.length();

    return sString.rfind(sFind, p)+1;
}


/////////////////////////////////////////////////
/// \brief Implementation for strmatch()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t strmatch_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = 0;

    std::vector<std::string> vUtf8Chars = splitUtf8Chars(sFind);

    for (; p < sString.length(); p++)
    {
        if (getUtf8ByteLen(sString[p]) > 0ull
            && matchesAny(sString, vUtf8Chars, p))
            return p+1;
    }

    return 0ull;
}


/////////////////////////////////////////////////
/// \brief Implementation for strrmatch()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t strrmatch_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = sString.length();

    std::vector<std::string> vUtf8Chars = splitUtf8Chars(sFind);

    for (int i = p-1; p >= 0; p--)
    {
        if (getUtf8ByteLen(sString[i]) > 0ull
            && matchesAny(sString, vUtf8Chars, i))
            return i+1ull;
    }

    return 0ull;
}


/////////////////////////////////////////////////
/// \brief Implementation for str_not_match()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t str_not_match_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = 0;

    std::vector<std::string> vUtf8Chars = splitUtf8Chars(sFind);

    for (; p < sString.length(); p++)
    {
        if (getUtf8ByteLen(sString[p]) > 0ull
            && !matchesAny(sString, vUtf8Chars, p))
            return p+1;
    }

    return 0ull;
}


/////////////////////////////////////////////////
/// \brief Implementation for str_not_rmatch()
///
/// \param sString const std::string&
/// \param sFind const std::string&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t str_not_rmatch_impl(const std::string& sString, const std::string& sFind, size_t p)
{
    if (!sString.length())
        return 0u;

    if (p >= sString.length())
        p = sString.length();

    std::vector<std::string> vUtf8Chars = splitUtf8Chars(sFind);

    for (int i = p-1; p >= 0; p--)
    {
        if (getUtf8ByteLen(sString[i]) > 0ull
            && !matchesAny(sString, vUtf8Chars, i))
            return i+1ull;
    }

    return 0ull;
}


/////////////////////////////////////////////////
/// \brief Implementation for STR.normalize()
///
/// \param sString const std::string&
/// \param sMethod const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string normalize_unicode_impl(const std::string& sString, const std::string& sMethod)
{
    if (!sString.length())
        return "";

    boost::locale::generator gen;
    boost::locale::norm_type type = boost::locale::norm_nfc;

    if (sMethod == "nfd")
        type = boost::locale::norm_nfd;
    else if (sMethod == "nfkd")
        type = boost::locale::norm_nfkd;
    else if (sMethod == "nfkc")
        type = boost::locale::norm_nfkc;

    return boost::locale::normalize(sString, type, gen("en_US.UTF-8"));
}


/////////////////////////////////////////////////
/// \brief Implementation for STR.collate()
///
/// \param sString1 const std::string&
/// \param sString2 const std::string&
/// \param level size_t
/// \return int
///
/////////////////////////////////////////////////
int collate_impl(const std::string& sString1, const std::string& sString2, size_t level)
{
    boost::locale::generator gen;
    boost::locale::collate_level colLevel = boost::locale::collate_level::primary;

    if (level == 2)
        colLevel = boost::locale::collate_level::secondary;
    else if (level == 3)
        colLevel = boost::locale::collate_level::tertiary;
    else if (level == 4)
        colLevel = boost::locale::collate_level::quaternary;
    else if (level == 5)
        colLevel = boost::locale::collate_level::identical;

    return std::use_facet<boost::locale::collator<char>>(gen("en_US.UTF-8")).compare(colLevel, sString1, sString2);
}


/////////////////////////////////////////////////
/// \brief Implementation for STR.segments. Not
/// yet supported by the boost library.
///
/// \param sString const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> segments_impl(const std::string& sString)
{
    if (!sString.length())
        return std::vector<std::string>(1, "");

    boost::locale::generator gen;

    // Create mapping of text for token iterator using the global locale.
    boost::locale::boundary::ssegment_index segIndex(boost::locale::boundary::word, sString.begin(), sString.end(), gen("en_US.UTF-8"));

    // Return all "words" -- chunks of word boundary
    return std::vector<std::string>(segIndex.begin(), segIndex.end());
}


/////////////////////////////////////////////////
/// \brief Return true, if the selected character
/// is an unescaped quotation mark.
///
/// \param sString const std::string&
/// \param pos size_t
/// \return bool
///
/////////////////////////////////////////////////
bool isQuotationMark(const std::string& sString, size_t pos)
{
    return sString[pos] == '"' && (!pos || sString[pos-1] != '\\' || (pos > 1 && sString[pos-2] == '\\'));
}


/////////////////////////////////////////////////
/// \brief Return true, if the selected character
/// is an unescaped quotation mark.
///
/// \param sString StringView
/// \param pos size_t
/// \return bool
/// \overload
/////////////////////////////////////////////////
bool isQuotationMark(StringView sString, size_t pos)
{
    return sString[pos] == '"' && (!pos || sString[pos-1] != '\\' || (pos > 1 && sString[pos-2] == '\\'));
}


