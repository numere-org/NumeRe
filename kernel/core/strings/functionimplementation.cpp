/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#include "functionimplementation.hpp"
#include "../utils/tools.hpp"
#include "../utils/filecheck.hpp"
#include "../../../common/compareFiles.hpp"
#ifndef PARSERSTANDALONE
#include "../../kernel.hpp"
#endif
#include <boost/tokenizer.hpp>
#include <regex>
#include <sstream>
#include <libsha.hpp>

/////////////////////////////////////////////////
/// \brief Simple helper to create a LaTeX
/// exponent from a string.
///
/// \param sExp const std::string&
/// \param negative bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createLaTeXExponent(const std::string& sExp, bool negative)
{
    return "\\cdot10^{" + (negative ? "-"+sExp : sExp) + "}";
}


/////////////////////////////////////////////////
/// \brief This function converts a number
/// into a tex string.
///
/// \param number const mu::Value&
/// \param precision size_t
/// \return string
///
/////////////////////////////////////////////////
static std::string formatNumberToTex(const mu::Value& number, size_t precision = 0)
{
#ifndef PARSERSTANDALONE
    // Use the default precision if precision is default value
    if (precision == 0)
        precision = NumeReKernel::getInstance()->getSettings().getPrecision();
#endif
    std::string sNumber = toString(number.getNum().asCF64(), precision);

    // Handle floating point numbers with
    // exponents correctly
    while (sNumber.find('e') != std::string::npos)
    {
        // Find first exponent start and value
        size_t firstExp = sNumber.find('e');
        size_t expBegin = sNumber.find_first_not_of('0', firstExp + 2);
        size_t expEnd = sNumber.find_first_not_of("0123456789", expBegin);

        // Get the modified string where the first exponent is replaced by the tex string format
        sNumber.replace(firstExp, expEnd-firstExp,
                        createLaTeXExponent(sNumber.substr(expBegin, expEnd-expBegin), sNumber[firstExp+1] == '-'));
    }

    // Consider some special values
    replaceAll(sNumber, "inf", "\\infty");
    replaceAll(sNumber, "-inf", "-\\infty");
    replaceAll(sNumber, "nan", "---");

    // Return the formatted string in math mode
    return "$" + sNumber + "$";
}


mu::Array strfnc_to_tex(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;

    if (!a2.isDefault())
    {
        for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
        {
            ret.push_back(formatNumberToTex(a1.get(i), a2.get(i).getNum().asI64()));
        }
    }
    else
    {
        for (size_t i = 0; i < a1.size(); i++)
        {
            ret.push_back(formatNumberToTex(a1.get(i), 0));
        }
    }

    return ret;
}


mu::Array strfnc_to_uppercase(const mu::Array& a)
{
    return mu::apply(toUpperCase, a);
}


mu::Array strfnc_to_lowercase(const mu::Array& a)
{
    return mu::apply(toLowerCase, a);
}


mu::Array strfnc_utf8ToAnsi(const mu::Array& a)
{
    return mu::apply(utf8ToAnsi, a);
}


mu::Array strfnc_ansiToUtf8(const mu::Array& a)
{
    return mu::apply(ansiToUtf8, a);
}


mu::Array strfnc_getenvvar(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
            ret.push_back(mu::Value(""));

        char* sVarValue = getenv(a[i].getStr().c_str());

        if (!sVarValue)
            ret.push_back(mu::Value(""));
        else
            ret.push_back(std::string(sVarValue));
    }

    return ret;
}


mu::Array strfnc_getFileParts(const mu::Array& a)
{
    mu::Array ret;
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
            ret.push_back("");

        std::vector<std::string> vFileParts = _fSys.getFileParts(a[i].getStr());
        ret.insert(ret.end(), vFileParts.begin(), vFileParts.end());
    }

    return ret;
}


mu::Array strfnc_getFileDiffs(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
    {
        if (!a1.get(i).getStr().length() || !a2.get(i).getStr().length())
            ret.push_back(mu::Value(""));

        std::string sDiffs = compareFiles(_fSys.ValidFileName(a1.get(i).getStr(), "", false, false),
                                          _fSys.ValidFileName(a2.get(i).getStr(), "", false, false));
        replaceAll(sDiffs, "\r\n", "\n");
        std::vector<std::string> vSplitted = split(sDiffs, '\n');

        ret.insert(ret.end(), vSplitted.begin(), vSplitted.end());
    }
#endif // PARSERSTANDALONE
    return ret;
}



mu::Array strfnc_getfilelist(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
    {
        std::vector<std::string> vFileList = NumeReKernel::getInstance()->getFileSystem().getFileList(a1.get(i).getStr(), a2.isDefault() ? 0 : a2.get(i).getNum().asI64());

        if (!vFileList.size())
            ret.push_back("");
        else
            ret.insert(ret.end(), vFileList.begin(), vFileList.end());
    }

    if (!ret.size())
        ret.push_back("");
#endif
    return ret;
}


mu::Array strfnc_getfolderlist(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
    {
        std::vector<std::string> vFolderList = NumeReKernel::getInstance()->getFileSystem().getFolderList(a1.get(i).getStr(), a2.isDefault() ? 0 : a2.get(i).getNum().asI64());

        if (!vFolderList.size())
            ret.push_back("");
        else
            ret.insert(ret.end(), vFolderList.begin(), vFolderList.end());
    }

    if (!ret.size())
        ret.push_back("");
#endif
    return ret;
}


mu::Array strfnc_strlen(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        ret.push_back(a[i].getStr().length());
    }

    return ret;
}


mu::Array strfnc_firstch(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
            ret.push_back("");
        else
            ret.push_back(std::string(1, a[i].getStr().front()));
    }

    return ret;
}


mu::Array strfnc_lastch(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
            ret.push_back("");
        else
            ret.push_back(std::string(1, a[i].getStr().back()));
    }

    return ret;
}


mu::Array strfnc_getmatchingparens(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < a.size(); i++)
    {
        ret.push_back(getMatchingParenthesis(a[i].getStr())+1);
    }
#endif
    return ret;
}


mu::Array strfnc_ascii(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            ret.push_back((uint8_t)a[i].getStr()[j]);
        }
    }

    return ret;
}


mu::Array strfnc_isblank(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isblank(a[i].getStr()[j])
                && _umlauts.lower.find(a[i].getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a[i].getStr()[j]) == std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isalnum(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isalnum(a[i].getStr()[j])
                || _umlauts.lower.find(a[i].getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isalpha(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isalpha(a[i].getStr()[j])
                || _umlauts.lower.find(a[i].getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_iscntrl(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (iscntrl(a[i].getStr()[j])
                && _umlauts.lower.find(a[i].getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a[i].getStr()[j]) == std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isdigit(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isdigit(a[i].getStr()[j]))
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isgraph(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isgraph(a[i].getStr()[j])
                || _umlauts.lower.find(a[i].getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_islower(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (islower(a[i].getStr()[j])
                || _umlauts.lower.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isprint(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isprint(a[i].getStr()[j])
                || _umlauts.lower.find(a[i].getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_ispunct(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (ispunct(a[i].getStr()[j])
                && _umlauts.lower.find(a[i].getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a[i].getStr()[j]) == std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isspace(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isspace(a[i].getStr()[j])
                && _umlauts.lower.find(a[i].getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a[i].getStr()[j]) == std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isupper(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isupper(a[i].getStr()[j])
                || _umlauts.upper.find(a[i].getStr()[j]) != std::string::npos)
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isxdigit(const mu::Array& a)
{
    mu::Array ret;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i].getStr().length())
        {
            ret.push_back(false);
            continue;
        }

        for (size_t j = 0; j < a[i].getStr().length(); j++)
        {
            if (isxdigit(a[i].getStr()[j]))
                ret.push_back(true);
            else
                ret.push_back(false);
        }
    }

    return ret;
}


mu::Array strfnc_isdir(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < a.size(); i++)
    {
        ret.push_back(is_dir(a[i].getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_isfile(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < a.size(); i++)
    {
        ret.push_back(is_file(a[i].getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_to_char(const mu::Array* arrs, int n)
{
    std::string sToChar;

    if (n == 1)
    {
        for (size_t i = 0; i < arrs[0].size(); i++)
        {
            if (arrs[0][i].isValid())
                sToChar += char(arrs[0][i].getNum().asI64());
        }
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (arrs[i].front().isValid())
                sToChar += char(arrs[i].front().getNum().asI64());
        }
    }

    return mu::Value(sToChar);
}


mu::Array strfnc_findfile(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    FileSystem _fSys;
    _fSys.initializeFromKernel();
    static std::string sExePath = NumeReKernel::getInstance()->getSettings().getExePath();

    for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
    {
        if (!a2.isDefault())
            _fSys.setPath(a2.get(i).getStr(), false, sExePath);
        else
            _fSys.setPath(sExePath, false, sExePath);

        std::string sExtension = ".dat";

        if (a1.get(i).getStr().rfind('.') != std::string::npos)
        {
            sExtension = a1.get(i).getStr().substr(a1.get(i).getStr().rfind('.'));

            if (sExtension.find('*') != std::string::npos || sExtension.find('?') != std::string::npos)
                sExtension = ".dat";
            else
                _fSys.declareFileType(sExtension);
        }

        std::string sFile = _fSys.ValidFileName(a1.get(i).getStr(), sExtension);

        ret.push_back(fileExists(sFile));
    }
#endif
    return ret;
}


mu::Array strfnc_split(const mu::Array& a1, const mu::Array& a2, const mu::Array& a3)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
    {
        if (!a2.get(i).getStr().length())
        {
            ret.push_back("");
            continue;
        }

        boost::char_separator<char> cSep(a2.get(i).getStr().c_str(), nullptr,
                                         (!a3.isDefault() && (bool)a3.get(i)) ? boost::keep_empty_tokens : boost::drop_empty_tokens);
        boost::tokenizer<boost::char_separator<char>> tok(a1.get(i).getStr(), cSep);

        for (boost::tokenizer<boost::char_separator<char>>::iterator iter = tok.begin(); iter != tok.end(); ++iter)
        {
            ret.push_back(std::string(*iter));
        }
    }

    if (!ret.size())
        ret.push_back("");

    return ret;
}


mu::Array strfnc_to_time(const mu::Array& a1, const mu::Array& a2)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
    {
        std::string sTime = a2.get(i).getStr() + " ";

        if (!a1.get(i).getStr().length() && isConvertible(sTime, CONVTYPE_DATE_TIME))
        {
            ret.push_back(to_double(StrToTime(sTime)));
            continue;
        }

        std::string sPattern = a1.get(i).getStr() + " ";

        if (sTime.length() != sPattern.length())
        {
            ret.push_back(NAN);
            continue;
        }

        time_stamp timeStruct = getTimeStampFromTimePoint(sys_time_now());
        time_zone tz = getCurrentTimeZone();
        timeStruct.m_hours = std::chrono::hours::zero();
        timeStruct.m_minutes = std::chrono::minutes::zero();
        timeStruct.m_seconds = std::chrono::seconds::zero();
        timeStruct.m_millisecs = std::chrono::milliseconds::zero();
        timeStruct.m_microsecs = std::chrono::microseconds::zero();

        char cCurrentChar = sPattern.front();
        std::string sCurrentElement;
        date::year y{1970u};// timeStruct.m_ymd.year();

        if (sPattern.find_first_of("MD") != std::string::npos)
            y = timeStruct.m_ymd.year();

        date::month m{1u};// timeStruct.m_ymd.month();
        date::day d{1u};//timeStruct.m_ymd.day();

        for (size_t i = 0; i < sPattern.length(); i++)
        {
            if (sPattern[i] != cCurrentChar)
            {
                switch (cCurrentChar)
                {
                    case 'y':
                    case 'Y': // year is either four or two chars long. The structure expects the time to start at the year 1900
                        if (sCurrentElement.length() > 2)
                            y = date::year(StrToInt(sCurrentElement));
                        else
                            y = date::year(StrToInt(sCurrentElement) + 2000);
                        break;
                    case 'M':
                        m = date::month(StrToInt(sCurrentElement));
                        break;
                    case 'D':
                        d = date::day(StrToInt(sCurrentElement));
                        break;
                    case 'H':
                        timeStruct.m_hours = std::chrono::hours(StrToInt(sCurrentElement));
                        break;
                    case 'h':
                        timeStruct.m_hours = std::chrono::hours(StrToInt(sCurrentElement) + (tz.Bias + tz.DayLightBias).count() / 60);
                        break;
                    case 'm':
                        timeStruct.m_minutes = std::chrono::minutes(StrToInt(sCurrentElement));
                        break;
                    case 's':
                        timeStruct.m_seconds = std::chrono::seconds(StrToInt(sCurrentElement));
                        break;
                    case 'i':
                        sCurrentElement.append(3-sCurrentElement.size(), '0');
                        timeStruct.m_millisecs = std::chrono::milliseconds(StrToInt(sCurrentElement));
                        break;
                    case 'u':
                        sCurrentElement.append(3-sCurrentElement.size(), '0');
                        timeStruct.m_microsecs = std::chrono::microseconds(StrToInt(sCurrentElement));
                        break;
                }

                cCurrentChar = sPattern[i];
                sCurrentElement.clear();
            }

            sCurrentElement += sTime[i];
        }

        timeStruct.m_ymd = date::year_month_day(y,m,d);

        ret.push_back(to_double(getTimePointFromTimeStamp(timeStruct)));
    }

    return ret;
}


mu::Array strfnc_strfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({what.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = 1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().find(what.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_strrfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({what.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = where.get(i).getStr().length()+1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().rfind(what.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_strmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({chars.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = 1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().find_first_of(chars.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_strrmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({chars.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = where.get(i).getStr().length()+1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().find_last_of(chars.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_str_not_match(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({chars.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = 1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().find_first_not_of(chars.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_str_not_rmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({chars.size(), where.size(), from.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos = where.get(i).getStr().length()+1;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos = from.get(i).getNum().asI64();

        ret.push_back(where.get(i).getStr().find_last_not_of(chars.get(i).getStr(), pos-1)+1);
    }

    return ret;
}


mu::Array strfnc_strfndall(const mu::Array& what, const mu::Array& where, const mu::Array& from, const mu::Array& to)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({what.size(), where.size(), from.size(), to.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos_start = 0;
        size_t pos_last;
        bool found = false;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos_start = from.get(i).getNum().asI64()-1;

        if (!to.isDefault()
            && to.get(i).getNum().asI64() > 0
            && to.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos_last = to.get(i).getNum().asI64() - what.get(i).getStr().length();
        else
            pos_last = where.get(i).getStr().length() - what.get(i).getStr().length();

        while (pos_start <= pos_last)
        {
            pos_start = where.get(i).getStr().find(what.get(i).getStr(), pos_start);

            if (pos_start <= pos_last)
            {
                found = true;
                pos_start++;
                ret.push_back(pos_start);
            }
        }

        if (!found)
            ret.push_back(0);
    }

    return ret;
}


mu::Array strfnc_strmatchall(const mu::Array& chars, const mu::Array& where, const mu::Array& from, const mu::Array& to)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({chars.size(), where.size(), from.size(), to.size()}); i++)
    {
        if (!where.get(i).getStr().length())
        {
            ret.push_back(0u);
            continue;
        }

        size_t pos_start = 0;
        size_t pos_last;

        if (!from.isDefault()
            && from.get(i).getNum().asI64() > 0
            && from.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos_start = from.get(i).getNum().asI64()-1;

        if (!to.isDefault()
            && to.get(i).getNum().asI64() > 0
            && to.get(i).getNum().asI64() <= (int64_t)where.get(i).getStr().length())
            pos_last = to.get(i).getNum().asI64()-1;
        else
            pos_last = where.get(i).getStr().length()-1;

        for (size_t j = 0; j < chars.get(i).getStr().length(); j++)
        {
            size_t match = where.get(i).getStr().find(chars.get(i).getStr()[j], pos_start);

            if (match <= pos_last)
                ret.push_back(match+1);
            else
                ret.push_back(0);
        }
    }

    return ret;
}


mu::Array strfnc_findparam(const mu::Array& par, const mu::Array& line, const mu::Array& following)
{
    mu::Array ret;

#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max({par.size(), line.size(), following.size()}); i++)
    {
        if (!line.get(i).getStr().length())
        {
            ret.push_back(0);
            continue;
        }

        size_t nMatch;

        if (!following.isDefault())
            nMatch = findParameter(line.get(i).getStr(), par.get(i).getStr(), following.get(i).getStr().front());
        else
            nMatch = findParameter(line.get(i).getStr(), par.get(i).getStr());

        if (nMatch)
            ret.push_back(nMatch); // findParameter returns already pos+1
        else
            ret.push_back(0);
    }
#endif
    return ret;
}


mu::Array strfnc_substr(const mu::Array& sStr, const mu::Array& pos, const mu::Array& len)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({sStr.size(), pos.size(), len.size()}); i++)
    {
        if (!sStr.get(i).getStr().length() || (size_t)pos.get(i).getNum().asI64() > sStr.get(i).getStr().length())
        {
            ret.push_back("");
            continue;
        }

        if (!len.isDefault())
            ret.push_back(sStr.get(i).getStr().substr(std::max(0LL, pos.get(i).getNum().asI64()-1), len.get(i).getNum().asI64()));
        else
            ret.push_back(sStr.get(i).getStr().substr(std::max(0LL, pos.get(i).getNum().asI64()-1)));
    }

    return ret;
}


mu::Array strfnc_repeat(const mu::Array& sStr, const mu::Array& rep)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(sStr.size(), rep.size()); i++)
    {
        ret.push_back(strRepeat(sStr.get(i).getStr(), rep.get(i).getNum().asI64()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Creates a padding string full of \c 0.
///
/// \param nTime int
/// \param nLength size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string padWithZeros(int nTime, size_t nLength)
{
    std::string sPadded = toString(nTime);
    sPadded.insert(0, nLength - sPadded.length(), '0');
    return sPadded;
}


mu::Array strfnc_timeformat(const mu::Array& fmt, const mu::Array& time)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(fmt.size(), time.size()); i++)
    {
        if (!fmt.get(i).getStr().length())
        {
            ret.push_back(toString(to_timePoint(time.get(i).getNum().asF64()), 0));
            continue;
        }

        std::string sFormattedTime = fmt.get(i).getStr() + " "; // contains pattern
        sys_time_point nTime = to_timePoint(time.get(i).getNum().asF64());
        time_stamp timeStruct = getTimeStampFromTimePoint(nTime);
        time_zone tz = getCurrentTimeZone();

        char cCurrentChar = sFormattedTime.front();
        size_t currentElementStart = 0;

        for (size_t i = 0; i < sFormattedTime.length(); i++)
        {
            if (cCurrentChar != sFormattedTime[i])
            {
                switch (cCurrentChar)
                {
                    case 'Y':
                    case 'y':
                        if (i - currentElementStart > 2)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(int(timeStruct.m_ymd.year()), i - currentElementStart));
                        else
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(int(timeStruct.m_ymd.year()) - 100 * (int(timeStruct.m_ymd.year()) / 100), i - currentElementStart));
                        break;
                    case 'M':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(unsigned(timeStruct.m_ymd.month()), i - currentElementStart));
                        break;
                    case 'D':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(unsigned(timeStruct.m_ymd.day()), i - currentElementStart));
                        break;
                    case 'd':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros((date::sys_days(timeStruct.m_ymd) - date::sys_days(timeStruct.m_ymd.year()/1u/1u)).count()+1, i - currentElementStart));
                        break;
                    case 'H':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count(), i - currentElementStart));
                        break;
                    case 'h':
                        if (timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60 < 0)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() + 24 - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        else if (timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60  >= 24)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() - 24 - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        else
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        break;
                    case 'm':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_minutes.count(), i - currentElementStart));
                        break;
                    case 's':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_seconds.count(), i - currentElementStart));
                        break;
                    case 'i':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_millisecs.count(), i - currentElementStart));
                        break;
                    case 'u':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_microsecs.count(), i - currentElementStart));
                        break;
                }

                currentElementStart = i;
                cCurrentChar = sFormattedTime[i];
            }
        }

        ret.push_back(sFormattedTime.substr(0, sFormattedTime.length()-1));
    }

    return ret;
}


mu::Array strfnc_weekday(const mu::Array& daynum, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(daynum.size(), opts.size()); i++)
    {
        sys_time_point nTime = to_timePoint(daynum.get(i).getNum().asF64());

        size_t day = getWeekDay(nTime);

        if (opts.isDefault()|| !opts.get(i))
        {
            ret.push_back(day);
            continue;
        }

        static std::vector<std::string> weekdays = _lang.getList("COMMON_WEEKDAY_*");

        if (weekdays.size() >= 7)
            ret.push_back(weekdays[day-1]);
        else
            ret.push_back("UNDEFINED");
    }

    return ret;
}


mu::Array strfnc_char(const mu::Array& sStr, const mu::Array& pos)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(sStr.size(), pos.size()); i++)
    {
        const std::string& s = sStr.get(i).getStr();
        int64_t p = pos.get(i).getNum().asI64();

        if (p <= 1)
            ret.push_back(s.substr(0, 1));
        else if (p >= (int64_t)s.length())
            ret.push_back(s.substr(s.length()-1, 1));
        else
            ret.push_back(s.substr(p-1, 1));
    }

    return ret;
}


mu::Array strfnc_getopt(const mu::Array& sStr, const mu::Array& pos)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max(sStr.size(), pos.size()); i++)
    {
        const std::string& s = sStr.get(i).getStr();
        int64_t p = pos.get(i).getNum().asI64();

        if (p < 1)
            p = 1;

        if (p > s.length())
            ret.push_back("");
        else
            ret.push_back(getArgAtPos(s, p-1));
    }
#endif
    return ret;
}


mu::Array strfnc_replace(const mu::Array& where, const mu::Array& from, const mu::Array& len, const mu::Array& rep)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({where.size(), from.size(), len.size(), rep.size()}); i++)
    {
        std::string s = where.get(i).getStr();
        const std::string& r = rep.get(i).getStr();

        int64_t p = std::min((int64_t)s.length(), std::max(1LL, from.get(i).getNum().asI64()));
        int64_t l = len.get(i).getNum().asI64();

        if (!s.length())
            ret.push_back("");
        else
            ret.push_back(s.replace(p-1, l, r));
    }

    return ret;
}


enum NumberBase
{
    LOG,
    BIN,
    OCT,
    HEX
};


/////////////////////////////////////////////////
/// \brief Static helper function for converting
/// number bases into the decimal base.
///
/// \param value StringView
/// \param base ValueBase
/// \return int64_t
///
/////////////////////////////////////////////////
static int64_t convertBaseToDecimal(StringView value, NumberBase base)
{
    std::stringstream stream;
    int64_t ret = 0;

    if (base == HEX)
        stream.setf(std::ios::hex, std::ios::basefield);
    else if (base == OCT)
        stream.setf(std::ios::oct, std::ios::basefield);
    else if (base == BIN)
    {
        for (int i = value.length() - 1; i >= 0; i--)
        {
            if (value[i] == '1')
                ret += intPower(2, value.length()-1 - i);
        }

        return ret;
    }
    else if (base == LOG)
    {
        if (toLowerCase(value.to_string()) == "true" || value == "1")
            return 1LL;

        return 0LL;
    }

    stream << value.to_string();
    stream >> ret;

    return ret;
}


/////////////////////////////////////////////////
/// \brief Simple helper to parse the exponents
/// in LaTeX format.
///
/// \param sExpr std::string&
/// \return double
///
/////////////////////////////////////////////////
static double extractLaTeXExponent(std::string& sExpr)
{
    double val;

    if (sExpr.find('{') != std::string::npos)
    {
        // a^{xyz}
        val = std::stod(sExpr.substr(sExpr.find('{')+1, sExpr.find('}')));
        sExpr.erase(0, sExpr.find('}')+1);
    }
    else
    {
        // a^b
        val = std::stod(sExpr.substr(sExpr.find_first_not_of("^ "), 1));
        sExpr.erase(0, sExpr.find_first_not_of("^ ")+1);
    }

    return val;
}


mu::Array strfnc_textparse(const mu::Array& sStr, const mu::Array& pattern, const mu::Array& p1, const mu::Array& p2)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({sStr.size(), pattern.size(), p1.size(), p2.size()}); i++)
    {
        StringView sSearchString = sStr.get(i).getStr();
        StringView sPattern = pattern.get(i).getStr();

        int64_t pos1 = 1;
        int64_t pos2 = sSearchString.length();

        if (!p1.isDefault())
            pos1 = std::max(pos1, p1.get(i).getNum().asI64());

        if (!p2.isDefault())
            pos2 = std::min(pos2, std::max(1LL, p2.get(i).getNum().asI64()));

        // Exclude border cases
        if (!sSearchString.length() || pos1 > (int64_t)sSearchString.length())
        {
            ret.push_back("");
            continue;
        }
        else if (!sPattern.length())
        {
            ret.push_back(sSearchString.to_string());
            continue;
        }

        // Examples for text, which shall be parsed
        // 2018-09-21: Message VAL=12452
        // %s: %s VAL=%f
        // {sDate, sMessage, fValue} = textparse("2018-09-21: Message VAL=12452", "%s: %s VAL=%f");

        size_t lastPosition = pos1 - 1;
        static const std::string sIDENTIFIERCHARS = "sfaLlthbo";

        std::vector<StringView> vPatterns;
        size_t offset = ret.size();

        // Tokenize the search pattern
        while (sPattern.length())
        {
            // Handle single char pattern stubs first
            if (sPattern.length() == 1)
            {
                vPatterns.push_back(sPattern);
                break;
            }

            for (size_t i = 0; i < sPattern.length()-1; i++)
            {
                if (sPattern[i] == '%' && sIDENTIFIERCHARS.find(sPattern[i+1]) != std::string::npos)
                {
                    vPatterns.push_back(sPattern.subview(0, i));
                    vPatterns.push_back(sPattern.subview(i, 2));

                    if (vPatterns.back() == "%s")
                        ret.push_back("");
                    else if (vPatterns.back() != "%a")
                        ret.push_back(std::complex<double>(NAN));

                    sPattern.trim_front(i+2);
                    break;
                }

                if (i+2 == sPattern.length())
                {
                    vPatterns.push_back(sPattern);
                    sPattern.clear();
                    break;
                }
            }
        }

        // If the search string starts with whitespaces and the
        // pattern doesn't start with a percentage sign, search
        // for the first non-whitespace character
        if (!vPatterns.front().length() && sSearchString.front() == ' ')
        {
            lastPosition = sSearchString.find_first_not_of(' ');

            if (lastPosition == std::string::npos)
                continue;
        }
        else if (vPatterns.front().length())
        {
            lastPosition = sSearchString.find(vPatterns.front().to_string());

            if (lastPosition == std::string::npos)
                continue;

            lastPosition += vPatterns.front().length();
        }

        size_t nth_token = 0;

        // Patterns and identifiers are alternating, starting with
        // identifiers
        for (size_t n = 1; n < vPatterns.size(); n+=2)
        {
            size_t pos = sSearchString.length() + lastPosition;

            //if ((int64_t)pos >= pos2)
            //    break;

            StringView nextPattern;

            if (vPatterns.size() > n+1)
                nextPattern = vPatterns[n+1];

            if (nextPattern.length())
                pos = sSearchString.find(nextPattern.to_string(), lastPosition);

            if (pos == std::string::npos)
                break;

            // Append the found token
            if (vPatterns[n] == "%s")
                ret[offset+nth_token] = sSearchString.subview(lastPosition, pos - lastPosition).to_string();
            else if (vPatterns[n] == "%h")
                ret[offset+nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), HEX));
            else if (vPatterns[n] == "%o")
                ret[offset+nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), OCT));
            else if (vPatterns[n] == "%b")
                ret[offset+nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), BIN));
            else if (vPatterns[n] == "%l")
                ret[offset+nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), LOG));
            else if (vPatterns[n] == "%t")
                ret[offset+nth_token] = to_double(StrToTime(sSearchString.subview(lastPosition, pos - lastPosition).to_string()));
            else if (vPatterns[n] == "%f")
            {
                std::string sFloatingPoint = sSearchString.subview(lastPosition, pos - lastPosition).to_string();

                if (sFloatingPoint.find('.') == std::string::npos)
                    replaceAll(sFloatingPoint, ",", ".");

                ret[offset+nth_token] = isConvertible(sFloatingPoint, CONVTYPE_VALUE) ? StrToCmplx(sFloatingPoint) : NAN;
            }
            else if (vPatterns[n] == "%L")
            {
                StringView sLaTeXView = sSearchString.subview(lastPosition, pos - lastPosition);
                sLaTeXView.strip();

                if (sLaTeXView.front() == '$' && sLaTeXView.back() == '$')
                {
                    sLaTeXView.trim_front(1);
                    sLaTeXView.trim_back(1);
                    sLaTeXView.strip();
                }

                std::string sLaTeXFormatted = sLaTeXView.to_string();

                replaceAll(sLaTeXFormatted, "{,}", ".");
                replaceAll(sLaTeXFormatted, "\\times", "*");
                replaceAll(sLaTeXFormatted, "\\cdot", "*");
                replaceAll(sLaTeXFormatted, "2\\pi", "6.283185");
                replaceAll(sLaTeXFormatted, "\\pi", "3.1415926");
                replaceAll(sLaTeXFormatted, "\\infty", "inf");
                replaceAll(sLaTeXFormatted, "---", "nan");
                replaceAll(sLaTeXFormatted, "\\,", " ");
                // 1.0*10^{-5} 1.0*10^2 1.0*10^3 2.5^{0.5}

                if (sLaTeXFormatted.find('^') != std::string::npos)
                {
                    std::complex<double> vVal = 0;
                    std::complex<double> vValFinal = 0;

                    size_t nOpPos = std::string::npos;

                    // Go through all exponents and parse them into complex double format
                    while (sLaTeXFormatted.length())
                    {
                        nOpPos = sLaTeXFormatted.find_first_of("*^");
                        vVal = StrToCmplx(sLaTeXFormatted.substr(0, nOpPos));

                        if (sLaTeXFormatted[nOpPos] == '*')
                        {
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            nOpPos = sLaTeXFormatted.find('^');
                            std::complex<double> vBase = StrToCmplx(sLaTeXFormatted.substr(0, nOpPos));
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            vVal *= std::pow(vBase, extractLaTeXExponent(sLaTeXFormatted));
                        }
                        else
                        {
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            vVal = std::pow(vVal, extractLaTeXExponent(sLaTeXFormatted));
                        }

                        if (sLaTeXFormatted.find_first_not_of(" *") != std::string::npos
                            && tolower(sLaTeXFormatted[sLaTeXFormatted.find_first_not_of(" *")]) == 'i')
                        {
                            vVal = std::complex<double>(vVal.imag(), vVal.real());
                            sLaTeXFormatted.erase(0, sLaTeXFormatted.find_first_of("iI")+1);
                        }

                        vValFinal += vVal;

                        if (sLaTeXFormatted.find_first_not_of(' ') == std::string::npos)
                            break;
                    }

                    ret[offset+nth_token] = vValFinal;
                }
                else // This can handle simple multiplications
                    ret[offset+nth_token] = StrToCmplx(sLaTeXFormatted);
            }

            if (vPatterns[n] != "%a")
                nth_token++;

            // Store the position of the separator
            lastPosition = pos + nextPattern.length();
        }

    }

    return ret;
}


mu::Array strfnc_locate(const mu::Array& arr, const mu::Array& tofind, const mu::Array& tol)
{
    if (!arr.size())
        return mu::Value(false);

    mu::Array ret;

    for (size_t j = 0; j < std::max(tofind.size(), tol.size()); j++)
    {
        int64_t t = 0;
        size_t offset = ret.size();
        StringView sView = tofind.get(j).getStr();

        if (!tol.isDefault())
            t = tol.get(j).getNum().asI64();

        // Examine the whole string array
        for (size_t i = 0; i < arr.size(); i++)
        {
            StringView arg = arr[i].getStr();

            // Apply the chosen matching method
            if (t == 1)
            {
                // Remove surrounding whitespaces and compare
                arg.strip();

                if (arg == sView)
                    ret.push_back(i+1);
            }
            else if (t == 2)
            {
                // Take the first non-whitespace characters
                if (arg.find_first_not_of(' ') != std::string::npos
                    && arg.subview(arg.find_first_not_of(' '), sView.length()) == sView)
                    ret.push_back(i+1);
            }
            else if (t == 3)
            {
                // Take the last non-whitespace characters
                if (arg.find_last_not_of(' ') != std::string::npos
                    && arg.find_last_not_of(' ')+1 >= sView.length()
                    && arg.subview(arg.find_last_not_of(' ')-sView.length()+1, sView.length()) == sView)
                    ret.push_back(i+1);
            }
            else if (t == 4)
            {
                // Search anywhere in the string
                if (arg.find(sView.to_string()) != std::string::npos)
                    ret.push_back(i+1);
            }
            else if (t == 5)
            {
                // Search any of the characters in the string
                if (arg.find_first_of(sView.to_string()) != std::string::npos)
                    ret.push_back(i+1);
            }
            else
            {
                // Simply compare
                if (arg == sView)
                    ret.push_back(i+1);
            }
        }

        if (ret.size() == offset)
            ret.push_back(false);
    }

    return ret;
}


mu::Array strfnc_strunique(const mu::Array& arr, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t j = 0; j < std::max((size_t)1, opts.size()); j++)
    {
        int64_t o = arr.size() > 1 ? 0 : 1;

        if (!opts.isDefault())
            o = opts.get(j).getNum().asI64();

        // Separate unique strings from unique chars
        if (o == 0)
        {
            // Create a copy of all values
            std::vector<std::string> vFuncArgs;

            for (size_t i = 0; i < arr.size(); i++)
            {
                vFuncArgs.push_back(arr[i].getStr());
            }

            // Sort and isolate the unique values
            std::sort(vFuncArgs.begin(), vFuncArgs.end());
            auto iter = std::unique(vFuncArgs.begin(), vFuncArgs.end());

            // Copy together
            for (auto it = vFuncArgs.begin(); it != iter; ++it)
            {
                ret.push_back(*it);
            }
        }
        else
        {
            // Examine each value independently
            for (size_t i = 0; i < arr.size(); i++)
            {
                // Get a quotation mark free copy
                std::string sArg = arr[i].getStr();

                // Sort and isolate the unique chars
                std::sort(sArg.begin(), sArg.end());
                auto iter = std::unique(sArg.begin(), sArg.end());

                // Append the string with unique characters
                ret.push_back(std::string(sArg.begin(), iter));
            }
        }
    }

    return ret;
}


mu::Array strfnc_strjoin(const mu::Array& arr, const mu::Array& sep, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t j = 0; j < std::max({(size_t)1, sep.size(), opts.size()}); j++)
    {
        std::string sJoined;
        std::string sSeparator;
        int64_t keepEmpty = 0;

        if (!sep.isDefault())
            sSeparator = sep.get(j).getStr();

        if (!opts.isDefault())
            keepEmpty = opts.get(j).getNum().asI64();

        for (size_t i = 0; i < arr.size(); i++)
        {
            // Only insert the separator if either the string as well
            // as the following one have a length or if also empty strings
            // shall be kept and this is the second string to be added
            if ((sJoined.length() && arr[i].getStr().length())
                || (i && keepEmpty))
                sJoined += sSeparator;

            sJoined += arr[i].getStr();
        }

        ret.push_back(sJoined);
    }

    return ret;
}


mu::Array strfnc_getkeyval(const mu::Array& kvlist, const mu::Array& key, const mu::Array& defs, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t j = 0; j < std::max({key.size(), defs.size(), opts.size()}); j++)
    {
        StringView sKey = key.get(j).getStr();
        int64_t o = 0;
        bool found = false;

        if (!opts.isDefault())
            o = opts.get(j).getNum().asI64();

        // Examine the whole string array
        for (size_t i = 0; i < kvlist.size(); i+=2)
        {
            // Remove the masked strings
            StringView arg = kvlist[i].getStr();

            // Remove surrounding whitespaces and compare
            arg.strip();

            if (arg == sKey)
            {
                ret.push_back(kvlist[i+1]);
                found = true;
            }
        }

        // set values to the default values and probably
        // issue a warning, if no values were found
        if (!found)
        {
#ifndef PARSERSTANDALONE
            if (o)
                NumeReKernel::issueWarning(_lang.get("PARSERFUNCS_LISTFUNC_GETKEYVAL_WARNING", "\"" + sKey.to_string() + "\""));
#endif
            if (!defs.isDefault())
                ret.push_back(defs.get(j));
            else
                ret.push_back(mu::Value(""));
        }
    }

    return ret;
}


mu::Array strfnc_findtoken(const mu::Array& sStr, const mu::Array& tok, const mu::Array& sep)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({sStr.size(), tok.size(), sep.size()}); i++)
    {
        StringView sView1 = sStr.get(i).getStr();
        const std::string& t = tok.get(i).getStr();
        std::string s = " \t";

        // Define default arguments
        if (!sep.isDefault())
            s = sep.get(i).getStr();

        size_t nMatch = 0;

        // search the first match of the token, which is surrounded by the
        // defined separator characters
        while ((nMatch = sView1.find(t, nMatch)) != std::string::npos)
        {
            if ((!nMatch || s.find(sView1[nMatch-1]) != std::string::npos)
                && (nMatch + t.length() >= sView1.length() || s.find(sView1[nMatch+t.length()]) != std::string::npos))
            {
                ret.push_back(nMatch+1);
                break;
            }

            nMatch++;
        }
    }

    if (!ret.size())
        return mu::Value(false);

    return ret;
}


mu::Array strfnc_replaceall(const mu::Array& sStr, const mu::Array& fnd, const mu::Array& rep, const mu::Array& pos1, const mu::Array& pos2)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({sStr.size(), fnd.size(), rep.size(), pos1.size(), pos2.size()}); i++)
    {
        std::string s = sStr.get(i).getStr();
        StringView f = fnd.get(i).getStr();
        StringView r = rep.get(i).getStr();

        if (!s.length() || !f.length())
        {
            ret.push_back(s);
            continue;
        }

        int64_t p1 = 1;
        int64_t p2 = s.length()+1;

        if (!pos1.isDefault())
            p1 = std::max(p1, std::min((int64_t)s.length(), pos1.get(i).getNum().asI64()));

        if (!pos2.isDefault())
            p2 = std::max(p1, std::min((int64_t)s.length(), pos2.get(i).getNum().asI64()));

        // Using the slower version to enable replacement of null characters
        replaceAll(s, f, r, p1-1, p2-1);

        ret.push_back(s);
    }

    return ret;
}


mu::Array strfnc_strip(const mu::Array& sStr, const mu::Array& frnt, const mu::Array& bck, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({sStr.size(), frnt.size(), bck.size(), opts.size()}); i++)
    {
        StringView s = sStr.get(i).getStr();
        StringView f = frnt.get(i).getStr();
        StringView b = bck.get(i).getStr();

        int64_t stripAll = 0;

        if (!opts.isDefault())
            stripAll = opts.get(i).getNum().asI64();

        if (!s.length())
        {
            ret.push_back("");
            continue;
        }

        while (f.length()
               && s.length() >= f.length()
               && s.subview(0, f.length()) == f)
        {
            s.trim_front(f.length());

            if (!stripAll)
                break;
        }

        while (b.length()
               && s.length() >= b.length()
               && s.subview(s.length() - b.length()) == b)
        {
            s.trim_back(b.length());

            if (!stripAll)
                break;
        }

        ret.push_back(s.to_string());

    }

    return ret;
}


mu::Array strfnc_regex(const mu::Array& rgx, const mu::Array& sStr, const mu::Array& pos, const mu::Array& len)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max({rgx.size(), sStr.size(), pos.size(), len.size()}); i++)
    {
        StringView r = rgx.get(i).getStr();
        StringView s = sStr.get(i).getStr();

        int64_t p = 1;
        int64_t l = s.length();

        if (!r.length())
        {
            ret.push_back(0);
            ret.push_back(0);
            continue;
        }

        if (!pos.isDefault())
            p = std::max(p, std::min(l, pos.get(i).getNum().asI64()));

        if (!len.isDefault())
            p = std::min(l, pos.get(i).getNum().asI64());

        try
        {
            std::smatch match;
            std::regex expr(r.to_string());
            std::string sStr = s.subview(p-1, l).to_string();

            if (std::regex_search(sStr, match, expr))
            {
                ret.push_back(match.position(0) + (size_t)p);
                ret.push_back(match.length(0));
            }
            else
            {
                ret.push_back(0);
                ret.push_back(0);
            }
        }
        catch (std::regex_error& e)
        {
            std::string message;

            switch (e.code())
            {
                case std::regex_constants::error_collate:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_COLLATE");
                    break;
                case std::regex_constants::error_ctype:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_CTYPE");
                    break;
                case std::regex_constants::error_escape:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_ESCAPE");
                    break;
                case std::regex_constants::error_backref:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BACKREF");
                    break;
                case std::regex_constants::error_brack:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BRACK");
                    break;
                case std::regex_constants::error_paren:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_PAREN");
                    break;
                case std::regex_constants::error_brace:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BRACE");
                    break;
                case std::regex_constants::error_badbrace:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BADBRACE");
                    break;
                case std::regex_constants::error_range:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_RANGE");
                    break;
                case std::regex_constants::error_space:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_SPACE");
                    break;
                case std::regex_constants::error_badrepeat:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BADREPEAT");
                    break;
                case std::regex_constants::error_complexity:
                    message =_lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_COMPLEXITY");
                    break;
                case std::regex_constants::error_stack:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_STACK");
                    break;
            }

            throw SyntaxError(SyntaxError::INVALID_REGEX, r.to_string(), SyntaxError::invalid_position, message);
        }
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_basetodec(const mu::Array& base, const mu::Array& val)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(base.size(), val.size()); i++)
    {
        StringView b = base.get(i).getStr();
        StringView v = val.get(i).getStr();

        if (b == "hex")
            ret.push_back(convertBaseToDecimal(v, HEX));
        else if (b == "oct")
            ret.push_back(convertBaseToDecimal(v, OCT));
        else if (b == "bin")
            ret.push_back(convertBaseToDecimal(v, BIN));
        else
            ret.push_back(v.to_string());
    }

    return ret;
}


mu::Array strfnc_dectobase(const mu::Array& base, const mu::Array& val)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(base.size(), val.size()); i++)
    {
        StringView b = base.get(i).getStr();
        int64_t v = val.get(i).getNum().asI64();
        std::stringstream stream;

        if (b == "hex")
            stream.setf(std::ios::hex, std::ios::basefield);
        else if (b == "oct")
            stream.setf(std::ios::oct, std::ios::basefield);
        else if (b == "bin")
        {
            int i = 0;
            std::string bin;

            while ((1 << i) <= v)
            {
                if (v & (1 << i))
                    bin.insert(0, "1");
                else
                    bin.insert(0, "0");

                i++;
            }

            if (!bin.length())
                bin = "0";

            ret.push_back(bin);
            continue;
        }

        stream.setf(std::ios::showbase);
        stream << v;
        std::string conv;
        stream >> conv;

        ret.push_back(conv);
    }

    return ret;
}


mu::Array strfnc_justify(const mu::Array& arr, const mu::Array& align)
{
    mu::Array ret;

    for (size_t j = 0; j < std::max((size_t)1, align.size()); j++)
    {
        int64_t a = -1;

        if (!align.isDefault())
            a = align.get(j).getNum().asI64();

        // Find the string of max length
        size_t maxLength = 0;

        // Examine the whole string array
        for (size_t i = 0; i < arr.size(); i++)
        {
            // Remove the masked strings
            StringView sStr = arr[i].getStr();

            // Remove surrounding whitespaces
            sStr.strip();

            if (sStr.length() > maxLength)
                maxLength = sStr.length();
        }

        // Fill all string with as many whitespaces as necessary
        for (size_t i = 0; i < arr.size(); i++)
        {
            StringView view = arr[i].getStr();
            view.strip();

            std::string sStr = view.to_string();

            if (a == 1)
                sStr.insert(0, maxLength - sStr.size(), ' ');
            else if (a == -1)
                sStr.append(maxLength - sStr.size(), ' ');
            else if (a == 0)
            {
                size_t leftSpace = (maxLength - sStr.size()) / 2;
                size_t rightSpace = maxLength - leftSpace - sStr.size();
                sStr.insert(0, leftSpace, ' ');
                sStr.append(rightSpace, ' ');
            }

            // Append the string with the justified result
            ret.push_back(sStr);
        }
    }

    return ret;
}


mu::Array strfnc_getlasterror()
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.push_back(errorTypeToString(getLastErrorType()));
    ret.push_back(getLastErrorMessage());
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_getversioninfo()
{
#ifndef PARSERSTANDALONE
    static std::string sBUILDDATE = std::string(AutoVersion::YEAR) + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE;
    static std::string sINTVERSION = toString((int)AutoVersion::MAJOR) + "."
        + toString((int)AutoVersion::MINOR) + "."
        + toString((int)AutoVersion::BUILD) + "."
        + toString((int)(std::stod(AutoVersion::UBUNTU_VERSION_STYLE)*100))
#ifdef __GNUWIN64__
        + "-x64"
#endif
        ;
    static std::string sINSTNAME = toString((int)AutoVersion::MAJOR) + toString((int)AutoVersion::MINOR) + toString((int)AutoVersion::BUILD)
        + (std::string(AutoVersion::STATUS_SHORT).find("rc") != std::string::npos ? AutoVersion::STATUS_SHORT : "")
#ifdef __GNUWIN64__
        + "_x64"
#endif
        ;
#endif // PARSERSTANDALONE

    mu::Array sVersionInfo;
#ifndef PARSERSTANDALONE
    sVersionInfo.push_back("Version");
    sVersionInfo.push_back(sVersion);
    sVersionInfo.push_back("BuildDate");
    sVersionInfo.push_back(sBUILDDATE);
    sVersionInfo.push_back("FullVersion");
    sVersionInfo.push_back(sINTVERSION);
    sVersionInfo.push_back("FileVersion");
    sVersionInfo.push_back(sINSTNAME);
    sVersionInfo.push_back("Architecture");
#ifdef __GNUWIN64__
    sVersionInfo.push_back("64 bit");
#else
    sVersionInfo.push_back("32 bit");
#endif
#endif // PARSERSTANDALONE
    return sVersionInfo;
}


mu::Array strfnc_getuilang()
{
    return mu::Value(_lang.get("LANGUAGE"));
}


mu::Array strfnc_getfileinfo(const mu::Array& file)
{
    mu::Array sFileInfo;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < file.size(); i++)
    {
        FileInfo fInfo = NumeReKernel::getInstance()->getFileSystem().getFileInfo(file[i].getStr());

        sFileInfo.push_back("Drive");
        sFileInfo.push_back(fInfo.drive);
        sFileInfo.push_back("Path");
        sFileInfo.push_back(fInfo.path);
        sFileInfo.push_back("Name");
        sFileInfo.push_back(fInfo.name);
        sFileInfo.push_back("FileExt");
        sFileInfo.push_back(fInfo.ext);
        sFileInfo.push_back("Size");
        sFileInfo.push_back(fInfo.filesize);

        std::string sAttr = fInfo.fileAttributes & FileInfo::ATTR_READONLY ? "readonly," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_HIDDEN ? "hidden," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_SYSTEM ? "systemfile," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_DIRECTORY ? "directory," : "";
        //sAttr += fInfo.fileAttributes & FileInfo::ATTR_ARCHIVE ? "archive," : "";
        //sAttr += fInfo.fileAttributes & FileInfo::ATTR_DEVICE ? "device," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_TEMPORARY ? "temp," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_COMPRESSED ? "compressed," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_OFFLINE ? "offline," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_ENCRYPTED ? "encrypted," : "";

        if (sAttr.length())
            sAttr.pop_back();
        else
            sAttr = "none";

        sFileInfo.push_back("Attributes");
        sFileInfo.push_back(sAttr);
        sFileInfo.push_back("CreationTime");
        sFileInfo.push_back(fInfo.creationTime);
        sFileInfo.push_back("ModificationTime");
        sFileInfo.push_back(fInfo.modificationTime);
    }
#endif // PARSERSTANDALONE
    return sFileInfo;
}


mu::Array strfnc_sha256(const mu::Array& sStr, const mu::Array& opts)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(sStr.size(), opts.size()); i++)
    {
        if (opts.isDefault() || opts.get(i).getNum().asI64() == 0)
            ret.push_back(sha256(sStr.get(i).getStr()));
        else
        {
#ifndef PARSERSTANDALONE
            std::string sFileName = NumeReKernel::getInstance()->getFileSystem().ValidFileName(sStr.get(i).getStr(),
                                                                                               ".dat", false, true);

            // Ensure that the file actually exist
            if (fileExists(sFileName))
            {
                std::fstream file(sFileName, std::ios_base::in | std::ios_base::binary);
                ret.push_back(sha256(file));
            }
#endif // PARSERSTANDALONE
        }
    }

    return ret;
}


mu::Array strfnc_startswith(const mu::Array& sStr, const mu::Array& with)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(sStr.size(), with.size()); i++)
    {
        if (!with.get(i).getStr().length())
            ret.push_back(false);
        else
            ret.push_back(sStr.get(i).getStr().starts_with(with.get(i).getStr()));
    }

    return ret;
}


mu::Array strfnc_endswith(const mu::Array& sStr, const mu::Array& with)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max(sStr.size(), with.size()); i++)
    {
        if (!with.get(i).getStr().length())
            ret.push_back(false);
        else
            ret.push_back(sStr.get(i).getStr().ends_with(with.get(i).getStr()));
    }

    return ret;
}


mu::Array strfnc_to_value(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    mu::Parser p = NumeReKernel::getInstance()->getParser(); // Get a copy

    for (size_t i = 0; i < sStr.size(); i++)
    {
        if (sStr[i].isString())
        {
            p.SetExpr(sStr[i].getStr());
            int res;
            mu::Array* vals = p.Eval(res);

            for (int n = 0; n < res; n++)
            {
                ret.insert(ret.end(), vals[n].begin(), vals[n].end());
            }
        }
        else
            ret.push_back(sStr[i]);
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_to_string(const mu::Array& vals)
{
    mu::Array ret;

    for (const auto& v : vals)
    {
        ret.push_back(v.printVal());
    }

    return ret;
}


mu::Array strfnc_getindices(const mu::Array& tab, const mu::Array& opts)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < std::max(tab.size(), opts.size()); i++)
    {
        int64_t nType = 0;

        if (!opts.isDefault())
            nType = opts.get(i).getNum().asI64();

        // Because the object might be a constructed table, we
        // disable the access caching for this expression
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        DataAccessParser _accessParser(tab.get(i).getStr(), false, true);

        if (!_accessParser.getDataObject().length() || !isValidIndexSet(_accessParser.getIndices()))
        {
            ret.push_back(NAN);
            continue;
        }

        if (nType > -1)
        {
            if (nType == 2 && _accessParser.getIndices().row.isOpenEnd())
                _accessParser.getIndices().row.setRange(_accessParser.getIndices().row.front(),
                                                        _accessParser.getIndices().row.front() + 1);
            else if (nType == 1 && _accessParser.getIndices().col.isOpenEnd())
                _accessParser.getIndices().col.setRange(_accessParser.getIndices().col.front(),
                                                        _accessParser.getIndices().col.front() + 1);

            if (_accessParser.isCluster())
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getCluster(_accessParser.getDataObject()).size()-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.back() = VectorIndex::INVALID;
            }
            else
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);
            }
        }

        _accessParser.getIndices().row.linearize();
        _accessParser.getIndices().col.linearize();

        ret.push_back(_accessParser.getIndices().row.front() + 1);
        ret.push_back(_accessParser.getIndices().row.last() + 1);
        ret.push_back(_accessParser.getIndices().col.front() + 1);
        ret.push_back(_accessParser.getIndices().col.last() + 1);
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_is_data(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.push_back(_data.isTable(sStr[i].getStr()) || _data.isCluster(sStr[i].getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_is_table(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.push_back(_data.isTable(sStr[i].getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_is_cluster(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.push_back(_data.isCluster(sStr[i].getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_findcolumn(const mu::Array& tab, const mu::Array& col)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < std::max(tab.size(), col.size()); i++)
    {
        if (!_data.isTable(tab.get(i).getStr()))
        {
            ret.push_back(NAN);
            continue;
        }

        std::vector<std::complex<double>> cols = _data.findCols(tab.get(i).getStr().substr(0, tab.get(i).getStr().find('(')),
                                                                {col.get(i).getStr()}, false, false);

        if (cols.size())
            ret.insert(ret.end(), cols.begin(), cols.end());
        else
            ret.push_back(NAN);
    }
#endif // PARSERSTANDALONE
    return ret;
}


mu::Array strfnc_valtostr(const mu::Array& vals, const mu::Array& cfill, const mu::Array& len)
{
    mu::Array ret;

    for (size_t i = 0; i < std::max({vals.size(), cfill.size(), len.size()}); i++)
    {
#ifndef PARSERSTANDALONE
        std::string v = vals.get(i).printVal(NumeReKernel::getInstance()->getSettings().getPrecision());
#else
        std::string v = vals.get(i).printVal();
#endif // PARSERSTANDALONE

        if (!len.isDefault()
            && !cfill.isDefault()
            && cfill.get(i).getStr().length()
            && (int64_t)v.length() < len.get(i).getNum().asI64())
        {
            int64_t l = len.get(i).getNum().asI64();
            const std::string& sChar = cfill.get(i).getStr();

            while ((int64_t)v.length() < l)
            {
                v.insert(0, 1, sChar.front());
            }
        }

        ret.push_back(v);
    }

    return ret;
}


mu::Array strfnc_gettypeof(const mu::Array& vals)
{
    return mu::Value(vals.getCommonTypeAsString());
}













