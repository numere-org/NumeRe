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

#include <cstring>
#include <sstream>

// Forward declaration
std::string getNextArgument(std::string& sArgList, bool bCut);

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
    ostringstream Temp;
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
    ostringstream Temp;
    Temp.precision(std::rint(nPrecision / (dNumber.real() != 0.0 && dNumber.imag() != 0.0 && !isnan(dNumber.imag()) ? 2 : 1)));

    if (dNumber.real() || !dNumber.imag())
        Temp << dNumber.real();

    if (dNumber.imag())
    {
        if ((dNumber.imag() > 0.0 || isnan(dNumber.imag())) && dNumber.real() != 0.0)
            Temp << "+";

        Temp << dNumber.imag() << (isnan(dNumber.imag()) || isinf(dNumber.imag()) ? " i" : "i");
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
    ostringstream timeStream;

    if (!(timeStampFlags & GET_ONLY_TIME))
    {
        timeStream << 1900 + ltm->tm_year << "-"; //YYYY-

        if (1 + ltm->tm_mon < 10)		// 0, falls Monat kleiner als 10
            timeStream << "0";

        timeStream << 1 + ltm->tm_mon << "-"; // MM-

        if (ltm->tm_mday < 10)		// 0, falls Tag kleiner als 10
            timeStream << "0";

        timeStream << ltm->tm_mday; 	// DD

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

    if (ltm->tm_hour < 10)
        timeStream << "0";

    if (timeStampFlags & GET_ONLY_TIME)
        timeStream << ltm->tm_hour - 1; 	// hh
    else
        timeStream << ltm->tm_hour; 	// hh

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";		// ':' im regulaeren Datum

    if (ltm->tm_min < 10)
        timeStream << "0";

    timeStream << ltm->tm_min;	// mm

    if (!(timeStampFlags & GET_AS_TIMESTAMP))
        timeStream << ":";

    if (ltm->tm_sec < 10)
        timeStream << "0";

    timeStream << ltm->tm_sec;	// ss

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
    ostringstream Temp;
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
    ostringstream Temp;
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
    ostringstream Temp;
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
    string sString;

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
    vector<int> vVector;

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
    string sReturn = sText;
    string sToErase = " AaEeIiOoUuƒ‰÷ˆ‹¸ﬂYy";
    for (unsigned int i = 0; i < sReturn.length(); i++)
    {
        if (sToErase.find(sReturn[i]) != string::npos
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

    if (sString.find_first_not_of("0123456789.+-*eEiI ") != std::string::npos)
        return NAN;

    std::stringstream in(sString);

    // read 1st value
    if (!(in >> re))
        return NAN; // ERROR!

    // check whether next char is 'i'
    char c = in.peek();

    if (c == EOF)
        return re; // End of Input

    // Is it the actual imaginary value?
    if (c == 'i' || c == 'I' || c == '*')
        return std::complex<double>(0.0, re);

    if (!(in >> im))
        return re; // ERROR or end of input

    c = in.peek();

    if (c == EOF || (c != 'i' && c != 'I' && c != '*'))
    { // ERROR or premature end of input
        return re;
    }

    return std::complex<double>(re, im);
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
    // Ensure the values are correct
    if (!sToModify.length() || !nRepLength)
        return;

    // check the boundaries
    if ((size_t)nStart > sToModify.length())
        return;

    if (nEnd == std::string::npos)
        nEnd = sToModify.length();

    // Process the replacing
    for (size_t i = nStart; i < nEnd; i++)
    {
        if (i == sToModify.length())
            break;
        if (sToModify.substr(i, nRepLength) == sToRep)
        {
            sToModify.replace(i, nRepLength, sNewValue);
            nEnd += nNewLength - nRepLength + 1;
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
    string sPath = __sPath;

    // Go through the whole string
    for (unsigned int i = 0; i < sPath.length(); i++)
    {
        // Replace the backslash
        if (sPath[i] == '\\')
            sPath[i] = '/';
    }
    return sPath;
}

