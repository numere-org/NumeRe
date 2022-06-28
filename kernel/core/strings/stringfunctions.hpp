/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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


#include "stringdatastructures.hpp"
#include "../../kernel.hpp"
#include "../utils/filecheck.hpp"
#include <boost/tokenizer.hpp>
#include <regex>
#include <sstream>

#define DEFAULT_NUM_ARG INT_MIN
// define the "End of transmission block" as string separator
#define NEWSTRING (char)23

using namespace std;

/////////////////////////////////////////////////
/// \brief This function removes the escape
/// characters from the passed string.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
string removeMaskedStrings(const std::string& sString)
{
    std::string sRet = sString;

    // Go through the string and remove all relevant escape characters
    // Omit the characters, which are identifying LaTeX command sequences
    for (size_t i = 0; i < sRet.length(); i++)
    {
        if (sRet.compare(i, 2, "\\\"") == 0)
            sRet.erase(i, 1);

        //if (sRet.compare(i, 2, "\\t") == 0
        //    && sRet.compare(i, 4, "\\tau") != 0
        //    && sRet.compare(i, 6, "\\theta") != 0
        //    && sRet.compare(i, 6, "\\times") != 0)
        //    sRet.replace(i, 2, "\t");
        //
        //if (sRet.compare(i, 2, "\\n") == 0
        //    && sRet.compare(i, 3, "\\nu") != 0)
        //    sRet.replace(i, 2, "\n");

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
string removeQuotationMarks(const std::string& sString)
{
    if (sString.find('"') == std::string::npos || sString.front() != '"' || sString.back() != '"')
        return sString;

    return sString.substr(1, sString.length() - 2);
}


/////////////////////////////////////////////////
/// \brief This function simply adds the
/// surrounding quotation marks.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
string addQuotationMarks(const std::string& sString)
{
    if (sString.front() == '"' && sString.back() == '"')
        return sString;
    else
        return "\"" + sString + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_string()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_to_string(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.view().length())
        return "\"\"";

    if (funcArgs.sArg1.is_string())
        return funcArgs.sArg1.getRef();

    // Is not a string
    return "\"" + funcArgs.sArg1.getRef() + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_uppercase()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_to_uppercase(StringFuncArgs& funcArgs)
{
    return "\"" + toUpperCase(funcArgs.sArg1.view().to_string()) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_lowercase()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_to_lowercase(StringFuncArgs& funcArgs)
{
    return "\"" + toLowerCase(funcArgs.sArg1.view().to_string()) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getenvvar()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getenvvar(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.view().length())
        return "\"\"";

    char* sVarValue = getenv(funcArgs.sArg1.view().to_string().c_str());

    if (!sVarValue)
        return "\"\"";
    else
        return "\"" + std::string(sVarValue) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfileparts()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getFileParts(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.view().length())
        return "\"\"";

    std::vector<std::string> vFileParts = funcArgs.opt->getFileParts(funcArgs.sArg1.view().to_string());

    std::string sReturnValue;

    for (size_t i = 0; i < vFileParts.size(); i++)
        sReturnValue += "\"" + vFileParts[i] + "\"" + NEWSTRING;

    sReturnValue.pop_back();
    return sReturnValue;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfilelist()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getfilelist(StringFuncArgs& funcArgs)
{
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    std::vector<std::string> vFileList = getFileList(funcArgs.sArg1.view().to_string(), *(funcArgs.opt), funcArgs.nArg1);
    std::string sFileList = "";
    for (unsigned int i = 0; i < vFileList.size(); i++)
    {
        sFileList += "\"" + vFileList[i] + "\"";
        if (i < vFileList.size() - 1)
            sFileList += NEWSTRING;
    }
    if (!sFileList.length())
        return "\"\"";
    else
        return sFileList;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfolderlist()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getfolderlist(StringFuncArgs& funcArgs)
{
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    std::vector<std::string> vFolderList = getFolderList(funcArgs.sArg1.view().to_string(), *(funcArgs.opt), funcArgs.nArg1);
    std::string sFolderList = "";
    for (unsigned int i = 0; i < vFolderList.size(); i++)
    {
        sFolderList += "\"" + vFolderList[i] + "\"";
        if (i < vFolderList.size() - 1)
            sFolderList += NEWSTRING;
    }
    if (!sFolderList.length())
        return "\"\"";
    else
        return sFolderList;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strlen()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strlen(StringFuncArgs& funcArgs)
{
    return toString((int)funcArgs.sArg1.view().length());
}


/////////////////////////////////////////////////
/// \brief Implementation of the getmatchinparens()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getmatchingparens(StringFuncArgs& funcArgs)
{
    return toString((int)getMatchingParenthesis(funcArgs.sArg1.view()) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the ascii()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_ascii(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    for (unsigned int i = 0; i < sView.length(); i++)
    {
        sCodes += toString((int)sView[i]);
        if (i + 1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_blank()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isblank(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isblank(sView[i])
            && _umlauts.lower.find(sView[i]) == std::string::npos
            && _umlauts.upper.find(sView[i]) == std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alnum()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isalnum(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isalnum(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alpha()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isalpha(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isalpha(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_cntrl()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_iscntrl(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (iscntrl(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_digit()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isdigit(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isdigit(sView[i]))
        {
            sCodes += "1";
        }
        else
        {
            sCodes += "0";
        }
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_dir()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isdir(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();

    if (is_dir(sView.to_string()))
        return "1";

    return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_file()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isfile(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    if (is_file(sView.to_string()))
        return "1";

    return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_graph()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isgraph(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isgraph(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_lower()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_islower(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    // Get an Umlauts structure instance and store it statically
    // (this variable will only be instantiated once and kept in
    // memory afterwards, which is more efficient)
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        // If the current character is found by "islower()" or is
        // part of the "lower" field of the "Umlauts" structure,
        // then it is a lowercase letter. In all other cases, it
        // is not
        if (islower(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_print()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isprint(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isprint(sView[i])
            || _umlauts.lower.find(sView[i]) != std::string::npos
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_punct()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_ispunct(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();

    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (ispunct(sView[i])
            && _umlauts.lower.find(sView[i]) == std::string::npos
            && _umlauts.upper.find(sView[i]) == std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_space()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isspace(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();

    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isspace(sView[i])
            && _umlauts.lower.find(sView[i]) == std::string::npos
            && _umlauts.upper.find(sView[i]) == std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_upper()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isupper(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();

    static Umlauts _umlauts;

    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isupper(sView[i])
            || _umlauts.upper.find(sView[i]) != std::string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_xdigit()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_isxdigit(StringFuncArgs& funcArgs)
{
    std::string sCodes = "";
    StringView sView = funcArgs.sArg1.view();
    for (unsigned int i = 0; i < sView.length(); i++)
    {
        if (isxdigit(sView[i]))
        {
            sCodes += "1";
        }
        else
        {
            sCodes += "0";
        }
        if (i+1 < sView.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_char()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_to_char(StringFuncArgs& funcArgs)
{
    std::string sToChar = "";
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        sToChar += (char)(funcArgs.nMultiArg[i]);
    }
    return "\"" + sToChar + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the and()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_and(StringFuncArgs& funcArgs)
{
    if (!funcArgs.nMultiArg.size())
        return "false";
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        if (!funcArgs.nMultiArg[i])
            return "false";
    }
    return "true";
}


/////////////////////////////////////////////////
/// \brief Implementation of the or()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_or(StringFuncArgs& funcArgs)
{
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        if (funcArgs.nMultiArg[i])
            return "true";
    }
    return "false";
}


/////////////////////////////////////////////////
/// \brief Implementation of the xor()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_xor(StringFuncArgs& funcArgs)
{
    bool isTrue = false;
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        if (funcArgs.nMultiArg[i])
        {
            if (!isTrue)
                isTrue = true;
            else
                return "false";
        }
    }
    if (isTrue)
        return "true";
    return "false";
}


/////////////////////////////////////////////////
/// \brief Implementation of the findfile()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_findfile(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();
    FileSystem _fSys;
    _fSys.setTokens(funcArgs.opt->getTokenPaths());

    if (sView2.length())
        _fSys.setPath(sView2.to_string(), false, funcArgs.opt->getExePath());
    else
        _fSys.setPath(funcArgs.opt->getExePath(), false, funcArgs.opt->getExePath());

    StringView sExtension = std::string(".dat");

    if (sView1.rfind('.') != std::string::npos)
    {
        sExtension = sView1.subview(sView1.rfind('.'));

        if (sExtension.find('*') != std::string::npos || sExtension.find('?') != std::string::npos)
            sExtension = std::string(".dat");
        else
            _fSys.declareFileType(sExtension.to_string());
    }

    std::string sFile = _fSys.ValidFileName(sView1.to_string(), sExtension.to_string());

    if (fileExists(sFile))
        return "true";

    return "false";
}


/////////////////////////////////////////////////
/// \brief Implementation of the split()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_split(StringFuncArgs& funcArgs)
{
    std::string sSplittedString = "";
    std::string sSep = funcArgs.sArg2.view().to_string();

    if (!sSep.length())
        return "\"\"";

    boost::char_separator<char> cSep(sSep.c_str());

    std::string sToSeparate = funcArgs.sArg1.view().to_string();
    boost::tokenizer<boost::char_separator<char> > tok(sToSeparate, cSep);

    for (boost::tokenizer<boost::char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
    {
        if (sSplittedString.length())
            sSplittedString += NEWSTRING;
        sSplittedString += "\"" + std::string(*iter) + "\"";
    }

    return /*addMaskedStrings*/ (sSplittedString);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_time()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_to_time(StringFuncArgs& funcArgs)
{
    std::string sTime = funcArgs.sArg2.view().to_string() + " ";
    std::string sPattern = funcArgs.sArg1.view().to_string() + " ";

    if (sTime.length() != sPattern.length())
        return "nan";

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

    return toCmdString(to_double(getTimePointFromTimeStamp(timeStruct)));
}


/////////////////////////////////////////////////
/// \brief Implementation of the strfnd()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strfnd(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    return toString((int)sView.find(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strfndall()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strfndall(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG || funcArgs.nArg2 <= 0 || sView.length() < (size_t)funcArgs.nArg2)
        funcArgs.nArg2 = sView.length();

    std::string positions;
    size_t pos_start = funcArgs.nArg1 - 1;
    size_t pos_last = funcArgs.nArg2 - sStr.length();

    while (pos_start <= pos_last)
    {
        pos_start = sView.find(sStr, pos_start);

        if (pos_start <= pos_last)
        {
            pos_start++;

            if (positions.length())
                positions += ",";

            positions += toString(pos_start);
        }
        else
        {
            if (positions.length())
                return positions;

            return "0";
        }
    }

    if (!positions.length())
        return "0";

    return positions;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strmatchall()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strmatchall(StringFuncArgs& funcArgs)
{
    StringView sStr = funcArgs.sArg1.view();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG || funcArgs.nArg2 <= 0 || sView.length() < (size_t)funcArgs.nArg2)
        funcArgs.nArg2 = sView.length();

    std::string positions;
    size_t pos_start = funcArgs.nArg1 - 1;
    size_t pos_last = funcArgs.nArg2 - 1;

    for (size_t i = 0; i < sStr.length(); i++)
    {
        size_t match = sView.find(sStr[i], pos_start);

        if (positions.length())
            positions += ",";

        if (match <= pos_last)
            positions += toString(match+1);
        else
            positions += "0";
    }

    if (!positions.length())
        return "0";

    return positions;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strmatch(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    return toString((int)sView.find_first_of(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_match()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_str_not_match(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    return toString((int)sView.find_first_not_of(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrfnd()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strrfnd(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = sView.length() + 1;

    return toString((int)sView.rfind(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strrmatch(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = sView.length() + 1;

    return toString((int)sView.find_last_of(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_rmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_str_not_rmatch(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();
    StringView sView = funcArgs.sArg2.view();

    if (!sView.length())
        return "0";

    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || sView.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = sView.length() + 1;

    return toString((int)sView.find_last_not_of(sStr, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the findparam()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_findparam(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();
    StringView sView3 = funcArgs.sArg3.view();

    if (!sView2.length())
        return "0";

    size_t nMatch;

    if (sView3.length())
        nMatch = findParameter(sView2.to_string(), sView1.to_string(), sView3[0]);
    else
        nMatch = findParameter(sView2.to_string(), sView1.to_string());

    if (nMatch != std::string::npos)
        return toString((int)(nMatch + 1));
    else
        return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the substr()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_substr(StringFuncArgs& funcArgs)
{
    StringView sView = funcArgs.sArg1.view();

    if (!sView.length())
        return "\"\"";

    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sView.length())
        return "\"\"";

    if (funcArgs.nArg2 < 0)
        funcArgs.nArg2 = -1;

    return "\"" + sView.subview(funcArgs.nArg1 - 1, funcArgs.nArg2).to_string() + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the repeat()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_repeat(StringFuncArgs& funcArgs)
{
    std::string sReturn;
    std::string sStr = funcArgs.sArg1.view().to_string();

    for (int i = 0; i < funcArgs.nArg1; i++)
        sReturn += sStr;

    return "\"" + sReturn + "\"";
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


/////////////////////////////////////////////////
/// \brief Implementation of the timeformat()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_timeformat(StringFuncArgs& funcArgs)
{
    std::string sFormattedTime = funcArgs.sArg1.view().to_string() + " "; // contains pattern
    sys_time_point nTime = to_timePoint(funcArgs.dArg1.real());
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
    sFormattedTime.pop_back();
    return "\"" + sFormattedTime + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the char()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_char(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();

    if (funcArgs.nArg1 <= 1)
        return "\"" + sStr.substr(0, 1) + "\"";

    if ((size_t)funcArgs.nArg1 >= sStr.length())
        return "\"" + sStr.substr(sStr.length() - 1) + "\"";

    return "\"" + sStr.substr(funcArgs.nArg1 - 1, 1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getopt()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getopt(StringFuncArgs& funcArgs)
{
    std::string sStr = funcArgs.sArg1.view().to_string();

    if (funcArgs.nArg1 <= 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sStr.length())
        return "\"\"";

    return "\"" + getArgAtPos(sStr, funcArgs.nArg1 - 1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the replace()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_replace(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();

    if (!sView1.length())
        return "\"\"";

    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sView1.length())
        funcArgs.nArg1 = sView1.length();

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;

    return "\"" + sView1.to_string().replace(funcArgs.nArg1 - 1, funcArgs.nArg2, sView2.to_string()) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the textparse()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_textparse(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();

    // Exclude border cases
    if (!sView1.length())
        return "\"\"";

    if (!sView2.length())
        return "\"" + sView1.to_string() + "\"";

    // Ensure that the indices are valid
    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sView1.length())
        return "\"\"";

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;

    // Examples for text, which shall be parsed
    // 2018-09-21: Message VAL=12452
    // %s: %s VAL=%f
    // {sDate, sMessage, fValue} = textparse("2018-09-21: Message VAL=12452", "%s: %s VAL=%f");

    std::string sParsedStrings;
    size_t lastPosition = funcArgs.nArg1 - 1;

    // If the search string starts with whitespaces and the
    // pattern doesn't start with a percentage sign, search
    // for the first non-whitespace character
    if (sView2.front() != '%' && sView1.front() == ' ')
    {
        lastPosition = sView1.find_first_not_of(' ');
    }

    // Go through the pattern
    for (size_t i = 0; i < sView2.length(); i++)
    {
        // Ensure that the last position is considered
        if (lastPosition > (size_t)funcArgs.nArg2)
            break;

        // Find the identifiers
        if (sView2.subview(i, 2) == "%s" || sView2.subview(i, 2) == "%f" || sView2.subview(i, 2) == "%a")
        {
            // Find the following identifier
            size_t pos = std::string::npos;

            for (size_t j = i+2; j < sView2.length(); j++)
            {
                if (sView2.subview(j, 2) == "%s" || sView2.subview(j, 2) == "%f" || sView2.subview(j, 2) == "%a")
                {
                    pos = j;
                    break;
                }
            }

            // Define the search pattern to find the
            // separator at the end of the current
            // token
            std::string sSearchPattern = sView2.subview(i+2, pos - i - 2).to_string();
            if (!sSearchPattern.length())
                pos = std::string::npos;
            else
                pos = sView1.find(sSearchPattern, lastPosition);

            // Ensure that the found position is inside
            // the right border
            if (pos > (size_t)funcArgs.nArg2 && (size_t)funcArgs.nArg2 < sView1.length())
                break;

            // Append a newstring character, if needed
            if (sParsedStrings.length() && sView2.subview(i, 2) != "%a")
                sParsedStrings += NEWSTRING;

            // Append the found token
            if (sView2.subview(i, 2) == "%s")
                sParsedStrings += "\"" + sView1.subview(lastPosition, pos - lastPosition).to_string() + "\"";
            else if (sView2.subview(i, 2) == "%f")
                sParsedStrings += sView1.subview(lastPosition, pos - lastPosition).to_string();

            // Store the position of the separator
            lastPosition = pos;
            i++;
        }
        else
            lastPosition++;
    }

    return sParsedStrings;
}


/////////////////////////////////////////////////
/// \brief Implementation of the locate()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_locate(StringFuncArgs& funcArgs)
{
    std::string sIds;

    // Remove the masked strings
    StringView sView = funcArgs.sArg2.view();

    // Set the default tolerance mode, if necessary
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    // Examine the whole string array
    for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
    {
        StringView arg = funcArgs.sMultiArg[i];

        // Apply the chosen matching method
        if (funcArgs.nArg1 == 1)
        {
            // Remove surrounding whitespaces and compare
            arg.strip();

            if (arg == sView)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 2)
        {
            // Take the first non-whitespace characters
            if (arg.find_first_not_of(' ') != std::string::npos
                && arg.subview(arg.find_first_not_of(' '), sView.length()) == sView)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 3)
        {
            // Take the last non-whitespace characters
            if (arg.find_last_not_of(' ') != std::string::npos
                && arg.find_last_not_of(' ')+1 >= sView.length()
                && arg.subview(arg.find_last_not_of(' ')-sView.length()+1, sView.length()) == sView)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 4)
        {
            // Search anywhere in the string
            if (arg.find(sView.to_string()) != std::string::npos)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 5)
        {
            // Search any of the characters in the string
            if (arg.find_first_of(sView.to_string()) != std::string::npos)
                sIds += toString(i+1) + ",";
        }
        else
        {
            // Simply compare
            if (arg == sView)
                sIds += toString(i+1) + ",";
        }
    }

    // Pop the trailing comma, if the string has a length.
    // Otherwise set the ID to 0 - nothing found
    if (sIds.length())
        sIds.pop_back();
    else
        sIds = "0";

    return sIds;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strunique()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strunique(StringFuncArgs& funcArgs)
{
    // Set the default tolerance mode, if necessary
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = funcArgs.sMultiArg.size() > 1 ? 0 : 1;

    std::string sUniqueStrings;

    // Separate unique strings from unique chars
    if (funcArgs.nArg1 == 0)
    {
        // Create a copy of all values (we do
        // not need to remove the quotation marks here)
        s_vect vFuncArgs = funcArgs.sMultiArg;

        // Sort and isolate the unique values
        std::sort(vFuncArgs.begin(), vFuncArgs.end());
        auto iter = std::unique(vFuncArgs.begin(), vFuncArgs.end());

        // Copy together
        for (auto it = vFuncArgs.begin(); it != iter; ++it)
        {
            if (sUniqueStrings.length())
                sUniqueStrings += NEWSTRING;

            sUniqueStrings += *it; // Should already contain the surrounding quotation marks
        }
    }
    else
    {
        // Examine each value independently
        for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
        {
            // Get a quotation mark free copy
            std::string sArg = funcArgs.sMultiArg[i].to_string();

            // Sort and isolate the unique chars
            std::sort(sArg.begin(), sArg.end());
            auto iter = std::unique(sArg.begin(), sArg.end());

            // Append a comma
            if (sUniqueStrings.length())
                sUniqueStrings += NEWSTRING;

            // Append the string with unique characters
            sUniqueStrings += "\"";
            sUniqueStrings.append(sArg.begin(), iter);
            sUniqueStrings += "\"";
        }
    }

    return sUniqueStrings;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getkeyval()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getkeyval(StringFuncArgs& funcArgs)
{
    std::string sValues;

    StringView sView2 = funcArgs.sArg2.view();

    // Set the default tolerance mode, if necessary
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    // Ensure that the length of the array is
    // even
    if (funcArgs.sMultiArg.size() % 2)
        funcArgs.sMultiArg.pop_back();

    // Examine the whole string array
    for (size_t i = 0; i < funcArgs.sMultiArg.size(); i+=2)
    {
        // Remove the masked strings
        StringView arg = funcArgs.sMultiArg[i];

        // Remove surrounding whitespaces and compare
        arg.strip();

        if (arg == sView2)
        {
            if (funcArgs.sMultiArg.is_string(i+1))
                sValues += "\"" + funcArgs.sMultiArg[i+1].to_string() + "\"" + NEWSTRING;
            else
                sValues += funcArgs.sMultiArg[i+1].to_string() + NEWSTRING;
        }
    }

    // Pop the trailing comma, if the string has a length.
    // Otherwise set values to the default values and probably
    // issue a warning
    if (sValues.length())
        sValues.pop_back();
    else
    {
        if (funcArgs.nArg1)
            NumeReKernel::issueWarning(_lang.get("PARSERFUNCS_LISTFUNC_GETKEYVAL_WARNING", "\"" + sView2.to_string() + "\""));

        if (funcArgs.sArg3.is_string())
            sValues = funcArgs.sArg3.getRef();
        else
            sValues = funcArgs.sArg3.getRef();
    }

    return sValues;
}


/////////////////////////////////////////////////
/// \brief Implementation of the findtoken()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_findtoken(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    std::string sStr2 = funcArgs.sArg2.view().to_string();
    std::string sStr3 = funcArgs.sArg3.view().to_string();

    // Define default arguments
    if (!sStr3.length())
        sStr3 = " \t";

    size_t nMatch = 0;

    // search the first match of the token, which is surrounded by the
    // defined separator characters
    while ((nMatch = sView1.find(sStr2, nMatch)) != std::string::npos)
    {
        if ((!nMatch || sStr3.find(sView1[nMatch-1]) != std::string::npos)
            && (nMatch + sStr2.length() >= sView1.length() || sStr3.find(sView1[nMatch+sStr2.length()]) != std::string::npos))
        {
            return toString(nMatch + 1);
        }
        nMatch++;
    }
    return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the replaceall()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_replaceall(StringFuncArgs& funcArgs)
{
    std::string sStr1 = funcArgs.sArg1.view().to_string();
    std::string sStr2 = funcArgs.sArg2.view().to_string();
    std::string sStr3 = funcArgs.sArg3.view().to_string();

    if (!sStr1.length())
        return "\"\"";

    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sStr1.length())
        funcArgs.nArg1 = sStr1.length();

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = sStr1.length() + 1;

    if (!sStr2.length())
        return "\"" + sStr1 + "\"";

    replaceAll(sStr1, sStr2.c_str(), sStr3.c_str(), funcArgs.nArg1-1, funcArgs.nArg2-1);

    return "\"" + sStr1 + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the strip()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_strip(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();
    StringView sView3 = funcArgs.sArg3.view();

    if (!sView1.length())
        return "\"\"";

    while (sView2.length()
           && sView1.length() >= sView2.length()
           && sView1.subview(0, sView2.length()) == sView2)
    {
        sView1.trim_front(sView2.length());

        if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
            break;
    }

    while (sView3.length()
           && sView1.length() >= sView3.length()
           && sView1.subview(sView1.length() - sView3.length()) == sView3)
    {
        sView1.trim_back(sView3.length());

        if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
            break;
    }

    return "\"" + sView1.to_string() + "\"";

}


/////////////////////////////////////////////////
/// \brief Implementation of the regex()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_regex(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();

    if (!sView1.length())
        return "0,0";

    // Ensure that the indices are valid
    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;

    if ((size_t)funcArgs.nArg1 > sView2.length())
        return "0,0";

    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;

    try
    {
        std::smatch match;
        std::regex expr(sView1.to_string());
        std::string sStr = sView2.subview(funcArgs.nArg1-1, funcArgs.nArg2).to_string();

        if (std::regex_search(sStr, match, expr))
        {
            return toString(match.position(0) + funcArgs.nArg1) + "," + toString(match.length(0));
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

        throw SyntaxError(SyntaxError::INVALID_REGEX, sView1.to_string(), SyntaxError::invalid_position, message);

    }

    return "0,0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the cnt()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_cnt(StringFuncArgs& funcArgs)
{
    if (funcArgs.sMultiArg.size())
    {
        return toString((int)funcArgs.sMultiArg.size());
    }
    else if (funcArgs.nMultiArg.size())
        return toString((int)funcArgs.nMultiArg.size());
    return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the num()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_num(StringFuncArgs& funcArgs)
{
    if (funcArgs.sMultiArg.size())
    {
        int nRet = 0;
        for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
        {
            if (funcArgs.sMultiArg[i].length())
                nRet++;
        }
        return toString(nRet);
    }
    else if (funcArgs.nMultiArg.size())
    {
        return toString((int)funcArgs.nMultiArg.size());
    }
    return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the min()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_min(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sMultiArg.size())
        return "\"\"";

    StringView sMin = funcArgs.sMultiArg[0];

    for (size_t i = 1; i < funcArgs.sMultiArg.size(); i++)
    {
        if (sMin > funcArgs.sMultiArg[i])
            sMin = funcArgs.sMultiArg[i];
    }

    return "\"" + sMin.to_string() + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the max()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_max(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sMultiArg.size())
        return "\"\"";

    StringView sMax = funcArgs.sMultiArg[0];

    for (size_t i = 1; i < funcArgs.sMultiArg.size(); i++)
    {
        if (sMax < funcArgs.sMultiArg[i])
            sMax = funcArgs.sMultiArg[i];
    }

    return "\"" + sMax.to_string() + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the sum()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_sum(StringFuncArgs& funcArgs)
{
    if (funcArgs.sMultiArg.size())
    {
        std::string sRet = "";

        for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
            sRet += funcArgs.sMultiArg[i].to_string();

        return "\"" + sRet + "\"";
    }
    else if (funcArgs.nMultiArg.size())
    {
        int nRet = 0;

        for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
            nRet += funcArgs.nMultiArg[i];

        return toString(nRet);
    }

    return "\"\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the dectobase()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_dectobase(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    std::stringstream stream;

    if (sView1 == "hex")
        stream.setf(std::ios::hex, std::ios::basefield);
    else if (sView1 == "oct")
        stream.setf(std::ios::oct, std::ios::basefield);
    else if (sView1 == "bin")
    {
        int i = 0;
        std::string ret;

        while ((1 << i) <= funcArgs.nArg1)
        {
            if (funcArgs.nArg1 & (1 << i))
                ret.insert(0, "1");
            else
                ret.insert(0, "0");

            i++;
        }

        if (!ret.length())
            ret = "0";

        return "\"" + ret + "\"";
    }

    stream.setf(std::ios::showbase);
    stream << funcArgs.nArg1;
    std::string ret;
    stream >> ret;

    return "\"" + ret + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the basetodec()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_basetodec(StringFuncArgs& funcArgs)
{
    StringView sView1 = funcArgs.sArg1.view();
    StringView sView2 = funcArgs.sArg2.view();
    std::stringstream stream;

    if (sView1 == "hex")
        stream.setf(std::ios::hex, std::ios::basefield);
    else if (sView1 == "oct")
        stream.setf(std::ios::oct, std::ios::basefield);
    else if (sView1 == "bin")
    {
        int ret = 0;

        for (int i = sView2.length() - 1; i >= 0; i--)
        {
            if (sView2[i] == '1')
                ret += intPower(2, sView2.length()-1 - i);
        }

        return toString(ret);
    }

    stream << sView2.to_string();
    long long int ret;
    stream >> ret;

    return toString(ret);
}


/////////////////////////////////////////////////
/// \brief Implementation of the justify
/// function. Each string in a std::vector of strings
/// is filled with whitespaces until they are
/// all the same length.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_justify(StringFuncArgs& funcArgs)
{
    std::string result;

    // Set the default justification mode
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = -1;

    // Find the string of max length
    size_t maxLength = 0;

    // Examine the whole string array
    for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
    {
        // Remove the masked strings
        StringView sStr = funcArgs.sMultiArg[i];

        // Remove surrounding whitespaces
        sStr.strip();

        if (sStr.length() > maxLength)
            maxLength = sStr.length();
    }

    // Fill all string with as many whitespaces as necessary
    for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
    {
        StringView view = funcArgs.sMultiArg[i];
        view.strip();

        std::string sStr = view.to_string();

        if (funcArgs.nArg1 == 1)
            sStr.insert(0, maxLength - sStr.size(), ' ');
        else if (funcArgs.nArg1 == -1)
            sStr.append(maxLength - sStr.size(), ' ');
        else if (funcArgs.nArg1 == 0)
        {
            size_t leftSpace = (maxLength - sStr.size()) / 2;
            size_t rightSpace = maxLength - leftSpace - sStr.size();
            sStr.insert(0, leftSpace, ' ');
            sStr.append(rightSpace, ' ');
        }

        // Append a comma
        if (result.length())
            result += NEWSTRING;

        // Append the string with the justified result
        result += "\"" + sStr + "\"";
    }

    return result;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getlasterror()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getlasterror(StringFuncArgs& funcArgs)
{
    return "\"" + errorTypeToString(getLastErrorType()) + "\"" + NEWSTRING + "\"" +getLastErrorMessage() + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getversioninfo()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strfnc_getversioninfo(StringFuncArgs& funcArgs)
{
    static std::string sBUILDDATE = std::string(AutoVersion::YEAR) + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE;
    static std::string sINTVERSION = toString((int)AutoVersion::MAJOR) + "."
        + toString((int)AutoVersion::MINOR) + "."
        + toString((int)AutoVersion::BUILD) + "."
        + toString((int)(std::stod(AutoVersion::UBUNTU_VERSION_STYLE)*100));
    static std::string sINSTNAME = toString((int)AutoVersion::MAJOR) + toString((int)AutoVersion::MINOR) + toString((int)AutoVersion::BUILD)
        + (std::string(AutoVersion::STATUS_SHORT).find("rc") != std::string::npos ? AutoVersion::STATUS_SHORT : "");

    return std::string("\"Version\"") + NEWSTRING + "\"" + sVersion + "\"" + NEWSTRING
        + "\"BuildDate\"" + NEWSTRING + "\"" + sBUILDDATE + "\"" + NEWSTRING
        + "\"FullVersion\"" + NEWSTRING + "\"" + sINTVERSION + "\"" + NEWSTRING
        + "\"FileVersion\"" + NEWSTRING + "\"" + sINSTNAME + "\"";
}


/////////////////////////////////////////////////
/// \brief This static function is used to construct
/// the string map.
///
/// \return std::map<std::string, StringFuncHandle>
///
/// This function is only called once during the
/// construction of the string parser class
/// instance.
/////////////////////////////////////////////////
static std::map<std::string, StringFuncHandle> getStringFuncHandles()
{
    std::map<std::string, StringFuncHandle> mHandleTable;

    mHandleTable["and"]                 = StringFuncHandle(VAL, strfnc_and, true);
    mHandleTable["ascii"]               = StringFuncHandle(STR, strfnc_ascii, false);
    mHandleTable["basetodec"]           = StringFuncHandle(STR_STR, strfnc_basetodec, false);
    mHandleTable["char"]                = StringFuncHandle(STR_VAL, strfnc_char, false);
    mHandleTable["cnt"]                 = StringFuncHandle(STR, strfnc_cnt, true);
    mHandleTable["dectobase"]           = StringFuncHandle(STR_VAL, strfnc_dectobase, false);
    mHandleTable["findfile"]            = StringFuncHandle(STR_STROPT, strfnc_findfile, false);
    mHandleTable["findparam"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findparam, false);
    mHandleTable["findtoken"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findtoken, false);
    mHandleTable["getenvvar"]           = StringFuncHandle(STR, strfnc_getenvvar, false);
    mHandleTable["getfilelist"]         = StringFuncHandle(STR_VALOPT, strfnc_getfilelist, false);
    mHandleTable["getfileparts"]        = StringFuncHandle(STR, strfnc_getFileParts, false);
    mHandleTable["getfolderlist"]       = StringFuncHandle(STR_VALOPT, strfnc_getfolderlist, false);
    mHandleTable["getkeyval"]           = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_getkeyval, true);
    mHandleTable["getlasterror"]        = StringFuncHandle(NOARGS, strfnc_getlasterror, false);
    mHandleTable["getversioninfo"]      = StringFuncHandle(NOARGS, strfnc_getversioninfo, false);
    mHandleTable["getmatchingparens"]   = StringFuncHandle(STR, strfnc_getmatchingparens, false);
    mHandleTable["getopt"]              = StringFuncHandle(STR_VAL, strfnc_getopt, false);
    mHandleTable["is_alnum"]            = StringFuncHandle(STR, strfnc_isalnum, false);
    mHandleTable["is_alpha"]            = StringFuncHandle(STR, strfnc_isalpha, false);
    mHandleTable["is_blank"]            = StringFuncHandle(STR, strfnc_isblank, false);
    mHandleTable["is_cntrl"]            = StringFuncHandle(STR, strfnc_iscntrl, false);
    mHandleTable["is_digit"]            = StringFuncHandle(STR, strfnc_isdigit, false);
    mHandleTable["is_dir"]              = StringFuncHandle(STR, strfnc_isdir, false);
    mHandleTable["is_file"]             = StringFuncHandle(STR, strfnc_isfile, false);
    mHandleTable["is_graph"]            = StringFuncHandle(STR, strfnc_isgraph, false);
    mHandleTable["is_lower"]            = StringFuncHandle(STR, strfnc_islower, false);
    mHandleTable["is_print"]            = StringFuncHandle(STR, strfnc_isprint, false);
    mHandleTable["is_punct"]            = StringFuncHandle(STR, strfnc_ispunct, false);
    mHandleTable["is_space"]            = StringFuncHandle(STR, strfnc_isspace, false);
    mHandleTable["is_upper"]            = StringFuncHandle(STR, strfnc_isupper, false);
    mHandleTable["is_xdigit"]           = StringFuncHandle(STR, strfnc_isxdigit, false);
    mHandleTable["justify"]             = StringFuncHandle(STR_VAL, strfnc_justify, true);
    mHandleTable["locate"]              = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_locate, true);
    mHandleTable["max"]                 = StringFuncHandle(STR, strfnc_max, true);
    mHandleTable["min"]                 = StringFuncHandle(STR, strfnc_min, true);
    mHandleTable["num"]                 = StringFuncHandle(STR, strfnc_num, true);
    mHandleTable["or"]                  = StringFuncHandle(VAL, strfnc_or, true);
    mHandleTable["regex"]               = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_regex, false);
    mHandleTable["repeat"]              = StringFuncHandle(STR_VAL, strfnc_repeat, false);
    mHandleTable["replace"]             = StringFuncHandle(STR_VAL_VALOPT_STROPT, strfnc_replace, false);
    mHandleTable["replaceall"]          = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_replaceall, false);
    mHandleTable["split"]               = StringFuncHandle(STR_STR, strfnc_split, false);
    mHandleTable["str_not_match"]       = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_match, false);
    mHandleTable["str_not_rmatch"]      = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_rmatch, false);
    mHandleTable["strip"]               = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_strip, false);
    mHandleTable["strfnd"]              = StringFuncHandle(STR_STR_VALOPT, strfnc_strfnd, false);
    mHandleTable["strfndall"]           = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_strfndall, false);
    mHandleTable["strlen"]              = StringFuncHandle(STR, strfnc_strlen, false);
    mHandleTable["strmatch"]            = StringFuncHandle(STR_STR_VALOPT, strfnc_strmatch, false);
    mHandleTable["strmatchall"]         = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_strmatchall, false);
    mHandleTable["strrfnd"]             = StringFuncHandle(STR_STR_VALOPT, strfnc_strrfnd, false);
    mHandleTable["strrmatch"]           = StringFuncHandle(STR_STR_VALOPT, strfnc_strrmatch, false);
    mHandleTable["strunique"]           = StringFuncHandle(STR_VALOPT, strfnc_strunique, true);
    mHandleTable["sum"]                 = StringFuncHandle(STR, strfnc_sum, true);
    mHandleTable["substr"]              = StringFuncHandle(STR_VAL_VALOPT, strfnc_substr, false);
    mHandleTable["textparse"]           = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_textparse, false);
    mHandleTable["timeformat"]          = StringFuncHandle(STR_DBL, strfnc_timeformat, false);
    mHandleTable["to_char"]             = StringFuncHandle(VAL, strfnc_to_char, true);
    mHandleTable["to_lowercase"]        = StringFuncHandle(STR, strfnc_to_lowercase, false);
    mHandleTable["to_string"]           = StringFuncHandle(STR, strfnc_to_string, false);
    mHandleTable["to_time"]             = StringFuncHandle(STR_STR, strfnc_to_time, false);
    mHandleTable["to_uppercase"]        = StringFuncHandle(STR, strfnc_to_uppercase, false);
    mHandleTable["xor"]                 = StringFuncHandle(VAL, strfnc_xor, true);

    return mHandleTable;
}




