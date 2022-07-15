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

#include <cstring>
#include <sstream>
#include <iomanip>

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

    if (timeStampFlags & GET_ONLY_TIME)
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
/// \brief This function converts a std::vector
/// into a std::string, where the components are
/// translated into "{x,y,z...}"
///
/// \param vVector const std::vector<int>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string toString(const std::vector<int>& vVector)
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


/////////////////////////////////////////////////
/// \brief This function converts a std::string
/// into a std::vector, where the string shall be
/// passed as "{x,y,z,...}"
///
/// \param sString std::string
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> toVector(std::string sString)
{
    std::vector<int> vVector;

    if (sString == "{}" || !sString.length())
        return vVector;

    if (sString.front() == '{' && sString.back() == '}')
    {
        sString.front() = ' ';
        sString.back() = ' ';
    }

    while (sString.length())
    {
        vVector.push_back(StrToInt(getNextArgument(sString, true)));
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
    static std::string sToErase = " AaEeIiOoUuƒ‰÷ˆ‹¸ﬂYy";
    for (unsigned int i = 0; i < sReturn.length(); i++)
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

    if (!isConvertible(sString, CONVTYPE_VALUE))
        return NAN;

    size_t imagPos = 0;

    // read 1st value
    re = std::stod(sString, &imagPos);

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (imagPos < sString.length() && sString[imagPos] == ' ')
        imagPos++;

    if (imagPos >= sString.length())
        return re; // End of Input

    // Is it the actual imaginary value?
    if (sString[imagPos] == 'i' || sString[imagPos] == 'I' || sString[imagPos] == '*')
        return std::complex<double>(0.0, re);

    size_t imagEnd;
    im = std::stod(sString.substr(imagPos), &imagEnd);

    imagEnd += imagPos;

    // check whether next char is 'i' and advance
    // over all whitespaces
    while (imagEnd < sString.length() &&sString[imagEnd] == ' ')
        imagEnd++;

    if (sString[imagEnd] == EOF
        || (sString[imagEnd] != 'i' && sString[imagEnd] != 'I' && sString[imagEnd] != '*'))
    { // ERROR or premature end of input
        return re;
    }

    return std::complex<double>(re, im);
    /*std::stringstream in(sString);

    // read 1st value
    if (!(in >> re))
    {
        // If this does not succeed, we read it as a
        // string and try to detect INF and NAN
        // constants it manually
        in.clear();
        in.seekg(0);
        std::string val;
        in >> val;
        val = toLowerCase(val);

        if (val == "inf")
            re = INFINITY;
        else if (val == "-inf")
            re = -INFINITY;
        else
            re = NAN;
    }

    char c;

    // check whether next char is 'i' and advance
    // over all whitespaces
    while ((c = in.peek()) == ' ')
        in.get(c);

    if (c == EOF)
        return re; // End of Input

    // Is it the actual imaginary value?
    if (c == 'i' || c == 'I' || c == '*')
        return std::complex<double>(0.0, re);

    if (!(in >> im))
    {
        // Anything left?
        if (in.eof())
            return re;

        in.clear();

        // If this does not succeed, we read it as a
        // string and try to detect INF and NAN
        // constants it manually
        std::string val;
        in >> val;
        val = toLowerCase(val);

        if (val == "inf")
            im = (c == '-' ? -1.0 : 1.0) * INFINITY;
        else
            im = NAN;
    }

    // check whether next char is 'i' and advance
    // over all whitespaces
    while ((c = in.peek()) == ' ')
        in.get(c);

    if (c == EOF || (c != 'i' && c != 'I' && c != '*'))
    { // ERROR or premature end of input
        return re;
    }

    return std::complex<double>(re, im);*/
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

    // Contains DD.MM.YY
    if (format & TD_DDMM || format & TD_DDMMYY)
    {
        std::vector<std::string> toks = split(sString, '.');
        date::year y(timeStruct.m_ymd.year());
        date::month m(0);
        date::day d(0);

        bool isFirst = sString.find('.') < 3 + pos;

        for (size_t i = 0; i < toks.size(); i++)
        {
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
    for (unsigned int i = 0; i < sLowerCase.length(); i++)
    {
        // --> Laufe alle Zeichen im String ab und pruefe, ob ihr CHAR-Wert zwischen A und Z liegt
        if ((int)sLowerCase[i] >= (int)'A' && (int)sLowerCase[i] <= (int)'Z')
        {
            // --> Falls ja, verschiebe den CHAR-Wert um die Differenz aus A und a <--
            sLowerCase[i] = (char)((int)sLowerCase[i] + ((int)'a' - (int)'A'));
        }
        if (sLowerCase[i] == 'ƒ')
            sLowerCase[i] = '‰';
        else if (sLowerCase[i] == '÷')
            sLowerCase[i] = 'ˆ';
        else if (sLowerCase[i] == '‹')
            sLowerCase[i] = '¸';
        else if (sLowerCase[i] == (char)142)
            sLowerCase[i] = (char)132;
        else if (sLowerCase[i] == (char)153)
            sLowerCase[i] = (char)148;
        else if (sLowerCase[i] == (char)154)
            sLowerCase[i] = (char)129;
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
    for (unsigned int i = 0; i < sUpperCase.length(); i++)
    {
        // Handle escape characters like linebreaks or tabulator characters
        if ((!i || sUpperCase[i - 1] != '\\') && (sUpperCase.substr(i, 2) == "\\n" || sUpperCase.substr(i, 2) == "\\t"))
        {
            i++;
            continue;
        }
        else if (sUpperCase.substr(i, 2) == "\\n")
        {
            sUpperCase.replace(i, 2, "N");
        }
        else if (sUpperCase.substr(i, 2) == "\\t")
        {
            sUpperCase.replace(i, 2, "T");
        }
        // --> Laufe alle Zeichen im String ab und pruefe, ob ihr CHAR-Wert zwischen a und z liegt
        if ((int)sUpperCase[i] >= (int)'a' && (int)sLowerCase[i] <= (int)'z')
        {
            // --> Falls ja, verschiebe den CHAR-Wert um die Differenz aus a und A <--
            sUpperCase[i] = (char)((int)sUpperCase[i] + ((int)'A' - (int)'a'));
        }
        if (sUpperCase[i] == '‰')
            sUpperCase[i] = 'ƒ';
        else if (sUpperCase[i] == 'ˆ')
            sUpperCase[i] = '÷';
        else if (sUpperCase[i] == '¸')
            sUpperCase[i] = '‹';
        else if (sUpperCase[i] == (char)132)
            sUpperCase[i] = (char)142;
        else if (sUpperCase[i] == (char)148)
            sUpperCase[i] = (char)153;
        else if (sUpperCase[i] == (char)129)
            sUpperCase[i] = (char)154;
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
/// \brief This function checks, whether a string
/// can be converted to the selected
/// ConvertibleType.
///
/// \param sStr const std::string&
/// \param type ConvertibleType
/// \return bool
///
/////////////////////////////////////////////////
bool isConvertible(const std::string& sStr, ConvertibleType type)
{
    if (type == CONVTYPE_VALUE)
    {
        // Apply the simplest heuristic: mostly every numerical valid character
        if (sStr.find_first_not_of(" 0123456789.,eianfEIANF+-*\t") != std::string::npos || sStr.find_first_not_of(" \t") == std::string::npos)
            return false;

        // Eliminate invalid character positions
        if (tolower(sStr.front()) == 'e'
            || tolower(sStr.front()) == 'a'
            || tolower(sStr.front()) == 'f'
            || tolower(sStr.back()) == 'e'
            || tolower(sStr.back()) == 'a')
            return false;

        // Try to detect dates
        return !isConvertible(sStr, CONVTYPE_DATE_TIME);
    }
    else if (type == CONVTYPE_DATE_TIME)
    {
        // Apply the simplest heuristic: only digits and separators
        if (sStr.find_first_not_of(" 0123456789,.:-/\tTZ") != std::string::npos || sStr.find_first_not_of(" \t") == std::string::npos)
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
    // YY-MM-DD, yy-m-d, YY/MM/DD, YY/M/D,
    if ((i+3 < sStr.length() && sStr[i+3] == sStr[i])
        || (i+2 < sStr.length() && sStr[i+2] == sStr[i]))
        return TD_YYMMDD | (sStr[i] == '-' ? TD_SEP_MINUS : TD_SEP_SLASH);

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
            // (YY)YY-MM-DD, (yy)yy-m-d, (YY)YY/MM/DD, (YY)YY/M/D
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
    unsigned int nRet = wcstombs(cBuf, wStr.c_str(), wStr.length() * 2 + 1);

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
    for (unsigned int i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == (char)142)
            sOutput[i] = 'ƒ';
        else if (sOutput[i] == (char)132)
            sOutput[i] = '‰';
        else if (sOutput[i] == (char)153)
            sOutput[i] = '÷';
        else if (sOutput[i] == (char)148)
            sOutput[i] = 'ˆ';
        else if (sOutput[i] == (char)154)
            sOutput[i] = '‹';
        else if (sOutput[i] == (char)129)
            sOutput[i] = '¸';
        else if (sOutput[i] == (char)225)
            sOutput[i] = 'ﬂ';
        else if (sOutput[i] == (char)248)
            sOutput[i] = '∞';
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
/// a standard ASCII string in the internal code
/// page representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string utf8parser(const std::string& sString)
{
    std::string sReturn = sString;
    if (sReturn.length() < 2)
        return sReturn;

    // Go through the complete string
    for (unsigned int i = 0; i < sReturn.length() - 1; i++)
    {
        // UTF-8 encoded characters are more than one byte in length
        if (sReturn[i] == (char)195)
        {
            if (sReturn[i + 1] == (char)132) //ƒ
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)142;
            }
            else if (sReturn[i + 1] == (char)164) //‰
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)132;
            }
            else if (sReturn[i + 1] == (char)150) //÷
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)153;
            }
            else if (sReturn[i + 1] == (char)182) //ˆ
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)148;
            }
            else if (sReturn[i + 1] == (char)156) //‹
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)154;
            }
            else if (sReturn[i + 1] == (char)188) //¸
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)129;
            }
            else if (sReturn[i + 1] == (char)159) //ﬂ
            {
                sReturn.erase(i, 1);
                sReturn[i] = (char)225;
            }
        }
        if (sReturn[i] == (char)194 && sReturn[i + 1] == (char)176)
        {
            sReturn.erase(i, 1);
            sReturn[i] = (char)248;
        }
    }
    return sReturn;
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
/// \param sToModify std::string&
/// \param sToRep const char*
/// \param sNewValue const char*
/// \param nStart size_t
/// \param nEnd size_t
/// \return void
///
/////////////////////////////////////////////////
void replaceAll(std::string& sToModify, const char* sToRep, const char* sNewValue, size_t nStart /*= 0*/, size_t nEnd /*= string::npos*/)
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

        if (!sToModify.compare(i, nRepLength, sToRep))
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
    for (unsigned int i = 0; i < sPath.length(); i++)
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


