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
#define DEFAULT_NUM_ARG INT_MIN
// define the "End of transmission block" as string separator
#define NEWSTRING (char)23


/////////////////////////////////////////////////
/// \brief This function removes the escape
/// characters from the passed string.
///
/// \param sString const string&
/// \return string
///
/////////////////////////////////////////////////
string removeMaskedStrings(const string& sString)
{
	if (sString.find("\\\"") == string::npos && sString.find("\\t") == string::npos && sString.find("\\n") == string::npos && sString.find("\\ ") == string::npos)
		return sString;

	string sRet = sString;

	// Go through the string and remove all relevant escape characters
	// Omit the characters, which are identifying LaTeX command sequences
	for (size_t i = 0; i < sRet.length(); i++)
	{
		if (sRet.substr(i, 2) == "\\\"")
			sRet.erase(i, 1);
		if (sRet.substr(i, 2) == "\\t" && sRet.substr(i, 4) != "\\tau" && sRet.substr(i, 6) != "\\theta")
			sRet.replace(i, 2, "\t");
		if (sRet.substr(i, 2) == "\\n" && sRet.substr(i, 3) != "\\nu")
			sRet.replace(i, 2, "\n");
		if (sRet.substr(i, 2) == "\\ ")
			sRet.erase(i + 1, 1);
	}

	return sRet;
}


/////////////////////////////////////////////////
/// \brief This function simply removes the
/// surrounding quotation marks.
///
/// \param sString const string&
/// \return string
///
/////////////////////////////////////////////////
string removeQuotationMarks(const string& sString)
{
	if (sString.find('"') == string::npos || sString.front() != '"' || sString.back() != '"')
		return sString;
	return sString.substr(1, sString.length() - 2);
}


/////////////////////////////////////////////////
/// \brief This function simply adds the
/// surrounding quotation marks.
///
/// \param sString const string&
/// \return string
///
/////////////////////////////////////////////////
string addQuotationMarks(const string& sString)
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_to_string(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.length())
        return "\"\"";

	if (funcArgs.sArg1.find_first_not_of(" ") != string::npos && funcArgs.sArg1[funcArgs.sArg1.find_first_not_of(" ")] == '"')
		return funcArgs.sArg1; // Already is a string

    // Is not a string
	return "\"" + funcArgs.sArg1 + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_uppercase()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_to_uppercase(StringFuncArgs& funcArgs)
{
	return "\"" + toUpperCase(funcArgs.sArg1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_lowercase()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_to_lowercase(StringFuncArgs& funcArgs)
{
	return "\"" + toLowerCase(funcArgs.sArg1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getenvvar()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getenvvar(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.length())
        return "\"\"";

    char* sVarValue = getenv(funcArgs.sArg1.c_str());

    if (!sVarValue)
        return "\"\"";
    else
        return "\"" + string(sVarValue) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfileparts()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getFileParts(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.length())
        return "\"\"";

    vector<string> vFileParts = funcArgs.opt->getFileParts(funcArgs.sArg1);

    string sReturnValue;

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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getfilelist(StringFuncArgs& funcArgs)
{
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
		funcArgs.nArg1 = 0;

	vector<string> vFileList = getFileList(removeMaskedStrings(funcArgs.sArg1), *(funcArgs.opt), funcArgs.nArg1);
	string sFileList = "";
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getfolderlist(StringFuncArgs& funcArgs)
{
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
		funcArgs.nArg1 = 0;

	vector<string> vFolderList = getFolderList(removeMaskedStrings(funcArgs.sArg1), *(funcArgs.opt), funcArgs.nArg1);
	string sFolderList = "";
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_strlen(StringFuncArgs& funcArgs)
{
	return toString((int)removeMaskedStrings(funcArgs.sArg1).length());
}


/////////////////////////////////////////////////
/// \brief Implementation of the getmatchinparens()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getmatchingparens(StringFuncArgs& funcArgs)
{
	return toString((int)getMatchingParenthesis(removeMaskedStrings(funcArgs.sArg1)) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the ascii()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_ascii(StringFuncArgs& funcArgs)
{
	string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
	{
		sCodes += toString((int)funcArgs.sArg1[i]);
		if (i + 1 < funcArgs.sArg1.length())
			sCodes += ",";
	}
	return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_blank()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isblank(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	static Umlauts _umlauts;

	for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isblank(funcArgs.sArg1[i]) && _umlauts.lower.find(funcArgs.sArg1[i]) == string::npos && _umlauts.upper.find(funcArgs.sArg1[i]) == string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alnum()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isalnum(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isalnum(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alpha()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isalpha(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isalpha(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_cntrl()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_iscntrl(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (iscntrl(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_digit()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isdigit(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isdigit(funcArgs.sArg1[i]))
        {
            sCodes += "1";
        }
        else
        {
            sCodes += "0";
        }
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_graph()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isgraph(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isgraph(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_lower()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_islower(StringFuncArgs& funcArgs)
{
    string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	// Get an Umlauts structure instance and store it statically
	// (this variable will only be instantiated once and kept in
    // memory afterwards, which is more efficient)
	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        // If the current character is found by "islower()" or is
        // part of the "lower" field of the "Umlauts" structure,
        // then it is a lowercase letter. In all other cases, it
        // is not
        if (islower(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";

        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_print()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isprint(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isprint(funcArgs.sArg1[i]) || _umlauts.lower.find(funcArgs.sArg1[i]) != string::npos || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_punct()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_ispunct(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);

	static Umlauts _umlauts;

	for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (ispunct(funcArgs.sArg1[i]) && _umlauts.lower.find(funcArgs.sArg1[i]) == string::npos && _umlauts.upper.find(funcArgs.sArg1[i]) == string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_space()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isspace(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);

	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isspace(funcArgs.sArg1[i]) && _umlauts.lower.find(funcArgs.sArg1[i]) == string::npos && _umlauts.upper.find(funcArgs.sArg1[i]) == string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_upper()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isupper(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);

	static Umlauts _umlauts;

    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isupper(funcArgs.sArg1[i]) || _umlauts.upper.find(funcArgs.sArg1[i]) != string::npos)
            sCodes += "1";
        else
            sCodes += "0";
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_xdigit()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_isxdigit(StringFuncArgs& funcArgs)
{
   string sCodes = "";
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        if (isxdigit(funcArgs.sArg1[i]))
        {
            sCodes += "1";
        }
        else
        {
            sCodes += "0";
        }
        if (i+1 < funcArgs.sArg1.length())
            sCodes += ",";
    }
    return sCodes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_char()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_to_char(StringFuncArgs& funcArgs)
{
	string sToChar = "";
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_and(StringFuncArgs& funcArgs)
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_or(StringFuncArgs& funcArgs)
{
	string sToChar = "";
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_xor(StringFuncArgs& funcArgs)
{
	string sToChar = "";
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_findfile(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	FileSystem _fSys;
	_fSys.setTokens(funcArgs.opt->getTokenPaths());
	if (funcArgs.sArg2.length())
		_fSys.setPath(removeMaskedStrings(funcArgs.sArg2), false, funcArgs.opt->getExePath());
	else
		_fSys.setPath(funcArgs.opt->getExePath(), false, funcArgs.opt->getExePath());
	string sExtension = ".dat";
	if (funcArgs.sArg1.rfind('.') != string::npos)
	{
		sExtension = funcArgs.sArg1.substr(funcArgs.sArg1.rfind('.'));
		if (sExtension.find('*') != string::npos || sExtension.find('?') != string::npos)
			sExtension = ".dat";
		else
			_fSys.declareFileType(sExtension);
	}
	funcArgs.sArg1 = _fSys.ValidFileName(funcArgs.sArg1, sExtension);
	if (fileExists(funcArgs.sArg1))
		return "true";
	return "false";
}


/////////////////////////////////////////////////
/// \brief Implementation of the split()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_split(StringFuncArgs& funcArgs)
{
	string sSplittedString = "";
	if (!funcArgs.sArg2.length())
		return "\"\"";
	string sSep = removeMaskedStrings(funcArgs.sArg2);
	sSep.erase(1);
	boost::char_separator<char> cSep(sSep.c_str());
	string sToSeparate = removeMaskedStrings(funcArgs.sArg1);
	tokenizer<char_separator<char> > tok(sToSeparate, cSep);
	for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
	{
		if (sSplittedString.length())
			sSplittedString += NEWSTRING;
		sSplittedString += "\"" + string(*iter) + "\"";
	}
	return /*addMaskedStrings*/ (sSplittedString);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_time()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_to_time(StringFuncArgs& funcArgs)
{
	string sTime = funcArgs.sArg2 + " ";
	string sPattern = funcArgs.sArg1 + " ";
	if (sTime.length() != sPattern.length())
		return "nan";
	time_t timeVal = time(nullptr);
	tm* timeStruct = localtime(&timeVal);
	TIME_ZONE_INFORMATION timezone;
	GetTimeZoneInformation(&timezone);
	char cCurrentChar = sPattern.front();
	string sCurrentElement;
	for (size_t i = 0; i < sPattern.length(); i++)
	{
		if (sPattern[i] != cCurrentChar)
		{
			switch (cCurrentChar)
			{
				case 'y':
				case 'Y': // year is either four or two chars long. The structure expects the time to start at the year 1900
					if (sCurrentElement.length() > 2)
						timeStruct->tm_year = StrToInt(sCurrentElement) - 1900;
					else
						timeStruct->tm_year = StrToInt(sCurrentElement) + (100 * (timeStruct->tm_year / 100));
					break;
				case 'M':
					timeStruct->tm_mon = StrToInt(sCurrentElement) - 1;
					break;
				case 'D':
					timeStruct->tm_mday = StrToInt(sCurrentElement);
					break;
				case 'H':
					timeStruct->tm_hour = StrToInt(sCurrentElement);
					break;
				case 'h':
					timeStruct->tm_hour = StrToInt(sCurrentElement) - timezone.Bias / 60;
					if (timeStruct->tm_hour < 0)
						timeStruct->tm_hour += 24;
					else if (timeStruct->tm_hour >= 24)
						timeStruct->tm_hour -= 24;
					break;
				case 'm':
					timeStruct->tm_min = StrToInt(sCurrentElement);
					break;
				case 's':
					timeStruct->tm_sec = StrToInt(sCurrentElement);
					break;
			}
			cCurrentChar = sPattern[i];
			sCurrentElement.clear();
		}
		sCurrentElement += sTime[i];
	}

	timeVal = mktime(timeStruct);
	return toString((size_t)timeVal);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strfnd()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_strfnd(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.find(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_strmatch(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.find_first_of(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_match()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_str_not_match(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.find_first_not_of(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrfnd()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_strrfnd(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = funcArgs.sArg2.length() + 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.rfind(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_strrmatch(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = funcArgs.sArg2.length() + 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.find_last_of(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_rmatch()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_str_not_rmatch(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 <= 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
		funcArgs.nArg1 = funcArgs.sArg2.length() + 1;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (funcArgs.sArg2.front() == '"')
		funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"') + 1, funcArgs.sArg2.rfind('"') - funcArgs.sArg2.find('"') - 1);
	return toString((int)funcArgs.sArg2.find_last_not_of(funcArgs.sArg1, funcArgs.nArg1 - 1) + 1);
}


/////////////////////////////////////////////////
/// \brief Implementation of the findparam()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_findparam(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sArg2.length())
		return "0";
	size_t nMatch;

	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);
	funcArgs.sArg3 = removeMaskedStrings(funcArgs.sArg3);

	if (funcArgs.sArg3.length())
	{
		nMatch = findParameter(funcArgs.sArg2, funcArgs.sArg1, funcArgs.sArg3[0]);
	}
	else
	{
		nMatch = findParameter(funcArgs.sArg2, funcArgs.sArg1);
	}
	if (nMatch != string::npos)
		return toString((int)(nMatch + 1));
	else
		return "0";
}


/////////////////////////////////////////////////
/// \brief Implementation of the substr()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_substr(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	if (!funcArgs.sArg1.length())
		return "\"\"";
	if (funcArgs.nArg1 < 1)
		funcArgs.nArg1 = 1;
	if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
		funcArgs.nArg1 = funcArgs.sArg1.length();
	if (funcArgs.nArg2 < 0)
		funcArgs.nArg2 = -1;
    string sRet = funcArgs.sArg1.substr(funcArgs.nArg1 - 1, funcArgs.nArg2);
    if (sRet.length() && sRet.back() == '\\')
        sRet += " ";
	return "\"" + sRet + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the repeat()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_repeat(StringFuncArgs& funcArgs)
{
	if (funcArgs.nArg1 <= 1)
		return "\"" + funcArgs.sArg1 + "\"";
	else
	{
		string sReturn;
		for (int i = 0; i < funcArgs.nArg1; i++)
			sReturn += funcArgs.sArg1;
		return "\"" + sReturn + "\"";
	}
}


/////////////////////////////////////////////////
/// \brief Creates a padding string full of \c 0.
///
/// \param nTime int
/// \param nLength size_t
/// \return string
///
/////////////////////////////////////////////////
static string padWithZeros(int nTime, size_t nLength)
{
	string sPadded = toString(nTime);
	sPadded.insert(0, nLength - sPadded.length(), '0');
	return sPadded;
}


/////////////////////////////////////////////////
/// \brief Implementation of the timeformat()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_timeformat(StringFuncArgs& funcArgs)
{
	string sFormattedTime = funcArgs.sArg1 + " "; // contains pattern
	__time64_t nTime = llabs(funcArgs.nArg1);
	tm* timeStruct = _localtime64(&nTime);
	TIME_ZONE_INFORMATION timezone;
	GetTimeZoneInformation(&timezone);
	char cCurrentChar = sFormattedTime.front();
	size_t currentElementStart = 0;

	for (size_t i = 0; i < sFormattedTime.length(); i++)
	{
		if (cCurrentChar != sFormattedTime[i])
		{
			switch (cCurrentChar)
			{
				case 'Y':
				case 'y': // year is either four or two chars long. The structure expects the time to start at the year 1900
					if (i - currentElementStart > 2)
						sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_year + 1900, i - currentElementStart));
					else
						sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_year - (100 * (timeStruct->tm_year / 100)), i - currentElementStart));
					break;
				case 'M':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_mon + 1, i - currentElementStart));
					break;
				case 'D':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_mday, i - currentElementStart));
					break;
				case 'd':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_yday, i - currentElementStart));
					break;
				case 'H':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_hour, i - currentElementStart));
					break;
				case 'h':
					if (timeStruct->tm_hour + timezone.Bias / 60 < 0)
						sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_hour + 24 + timezone.Bias / 60, i - currentElementStart));
					else if (timeStruct->tm_hour + timezone.Bias / 60 >= 24)
						sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_hour - 24 + timezone.Bias / 60, i - currentElementStart));
					else
						sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_hour + timezone.Bias / 60, i - currentElementStart));
					break;
				case 'm':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_min, i - currentElementStart));
					break;
				case 's':
					sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct->tm_sec, i - currentElementStart));
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_char(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	if (funcArgs.nArg1 <= 1)
		return "\"" + funcArgs.sArg1.substr(0, 1) + "\"";
	if ((size_t)funcArgs.nArg1 >= funcArgs.sArg1.length())
		return "\"" + funcArgs.sArg1.substr(funcArgs.sArg1.length() - 1) + "\"";
	return "\"" + funcArgs.sArg1.substr(funcArgs.nArg1 - 1, 1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the getopt()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getopt(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	if (funcArgs.nArg1 <= 1)
		funcArgs.nArg1 = 1;
	if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
		return "\"\"";

	return "\"" + getArgAtPos(funcArgs.sArg1, funcArgs.nArg1 - 1) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the replace()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_replace(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

	if (!funcArgs.sArg1.length())
		return "\"\"";

	if (funcArgs.nArg1 < 1)
		funcArgs.nArg1 = 1;
	if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
		funcArgs.nArg1 = funcArgs.sArg1.length();
	if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
		funcArgs.nArg2 = -1;
	return "\"" + funcArgs.sArg1.replace(funcArgs.nArg1 - 1, funcArgs.nArg2, funcArgs.sArg2) + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the textparse()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_textparse(StringFuncArgs& funcArgs)
{
    // Remove the masked strings
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    // Exclude border cases
    if (!funcArgs.sArg1.length())
        return "\"\"";
    if (!funcArgs.sArg2.length())
        return "\"" + funcArgs.sArg1 + "\"";

    // Ensure that the indices are valid
    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;
    if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
        return "\"\"";
    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;

    // Examples for text, which shall be parsed
    // 2018-09-21: Message VAL=12452
    // %s: %s VAL=%f
    // {sDate, sMessage, fValue} = textparse("2018-09-21: Message VAL=12452", "%s: %s VAL=%f");

    string sParsedStrings;
    size_t lastPosition = funcArgs.nArg1 - 1;

    // If the search string starts with whitespaces and the
    // pattern doesn't start with a percentage sign, search
    // for the first non-whitespace character
    if (funcArgs.sArg2.front() != '%' && funcArgs.sArg1.front() == ' ')
    {
        lastPosition = funcArgs.sArg1.find_first_not_of(' ');
    }

    // Go through the pattern
    for (size_t i = 0; i < funcArgs.sArg2.length(); i++)
    {
        // Ensure that the last position is considered
        if (lastPosition > (size_t)funcArgs.nArg2)
            break;

        // Find the identifiers
        if (funcArgs.sArg2.substr(i, 2) == "%s" || funcArgs.sArg2.substr(i, 2) == "%f" || funcArgs.sArg2.substr(i, 2) == "%a")
        {
            // Find the following identifier
            size_t pos = string::npos;
            for (size_t j = i+2; j < funcArgs.sArg2.length(); j++)
            {
                if (funcArgs.sArg2.substr(j, 2) == "%s" || funcArgs.sArg2.substr(j, 2) == "%f" || funcArgs.sArg2.substr(j, 2) == "%a")
                {
                    pos = j;
                    break;
                }
            }

            // Define the search pattern to find the
            // separator at the end of the current
            // token
            string sSearchPattern = funcArgs.sArg2.substr(i+2, pos - i - 2);
            if (!sSearchPattern.length())
                pos = string::npos;
            else
                pos = funcArgs.sArg1.find(sSearchPattern, lastPosition);

            // Ensure that the found position is inside
            // the right border
            if (pos > (size_t)funcArgs.nArg2 && (size_t)funcArgs.nArg2 < funcArgs.sArg1.length())
                break;

            // Append a newstring character, if needed
            if (sParsedStrings.length() && funcArgs.sArg2.substr(i, 2) != "%a")
                sParsedStrings += NEWSTRING;

            // Append the found token
            if (funcArgs.sArg2.substr(i, 2) == "%s")
                sParsedStrings += "\"" + funcArgs.sArg1.substr(lastPosition, pos - lastPosition) + "\"";
            else if (funcArgs.sArg2.substr(i, 2) == "%f")
                sParsedStrings += funcArgs.sArg1.substr(lastPosition, pos - lastPosition);

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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_locate(StringFuncArgs& funcArgs)
{
    string sIds;

    // Remove the masked strings
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    // Set the default tolerance mode, if necessary
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    // Examine the whole string array
    for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
    {
        // Remove the masked strings
        funcArgs.sMultiArg[i] = removeMaskedStrings(funcArgs.sMultiArg[i]);

        // Apply the chosen matching method
        if (funcArgs.nArg1 == 1)
        {
            // Remove surrounding whitespaces and compare
            StripSpaces(funcArgs.sMultiArg[i]);

            if (funcArgs.sMultiArg[i] == funcArgs.sArg2)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 2)
        {
            // Take the first non-whitespace characters
            if (funcArgs.sMultiArg[i].find_first_not_of(' ') != string::npos
                && funcArgs.sMultiArg[i].substr(funcArgs.sMultiArg[i].find_first_not_of(' '), funcArgs.sArg2.length()) == funcArgs.sArg2)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 3)
        {
            // Take the last non-whitespace characters
            if (funcArgs.sMultiArg[i].find_last_not_of(' ') != string::npos
                && funcArgs.sMultiArg[i].find_last_not_of(' ')+1 >= funcArgs.sArg2.length()
                && funcArgs.sMultiArg[i].substr(funcArgs.sMultiArg[i].find_last_not_of(' ')-funcArgs.sArg2.length()+1, funcArgs.sArg2.length()) == funcArgs.sArg2)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 4)
        {
            // Search anywhere in the string
            if (funcArgs.sMultiArg[i].find(funcArgs.sArg2) != string::npos)
                sIds += toString(i+1) + ",";
        }
        else if (funcArgs.nArg1 == 5)
        {
            // Search any of the characters in the string
            if (funcArgs.sMultiArg[i].find_first_of(funcArgs.sArg2) != string::npos)
                sIds += toString(i+1) + ",";
        }
        else
        {
            // Simply compare
            if (funcArgs.sMultiArg[i] == funcArgs.sArg2)
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
/// \brief Implementation of the getkeyval()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_getkeyval(StringFuncArgs& funcArgs)
{
    string sValues;

    // Remove the masked strings
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);
    funcArgs.sArg3 = removeMaskedStrings(funcArgs.sArg3);

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
        funcArgs.sMultiArg[i] = removeMaskedStrings(funcArgs.sMultiArg[i]);

        // Remove surrounding whitespaces and compare
        StripSpaces(funcArgs.sMultiArg[i]);

        if (funcArgs.sMultiArg[i] == funcArgs.sArg2)
            sValues += funcArgs.sMultiArg[i+1] + ",";
    }

    // Pop the trailing comma, if the string has a length.
    // Otherwise set values to the default values and probably
    // issue a warning
    if (sValues.length())
        sValues.pop_back();
    else
    {
        if (funcArgs.nArg1)
            NumeReKernel::issueWarning(_lang.get("PARSERFUNCS_LISTFUNC_GETKEYVAL_WARNING", "\"" + funcArgs.sArg2 + "\""));

        if (funcArgs.sArg3.find_first_not_of("0123456789.eE+-(){},") != string::npos && funcArgs.sArg3.front() != '"')
            sValues = "\"" + funcArgs.sArg3 + "\"";
        else
            sValues = funcArgs.sArg3;
    }

    return sValues;
}


/////////////////////////////////////////////////
/// \brief Implementation of the findtoken()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_findtoken(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);
    funcArgs.sArg3 = removeMaskedStrings(funcArgs.sArg3);

    // Define default arguments
    if (!funcArgs.sArg3.length())
        funcArgs.sArg3 = " \t";

    size_t nMatch = 0;

    // search the first match of the token, which is surrounded by the
    // defined separator characters
    while ((nMatch = funcArgs.sArg1.find(funcArgs.sArg2, nMatch)) != string::npos)
    {
        if ((!nMatch || funcArgs.sArg3.find(funcArgs.sArg1[nMatch-1]) != string::npos)
            && (nMatch + funcArgs.sArg2.length() >= funcArgs.sArg1.length() || funcArgs.sArg3.find(funcArgs.sArg1[nMatch+funcArgs.sArg2.length()]) != string::npos))
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_replaceall(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);
	funcArgs.sArg3 = removeMaskedStrings(funcArgs.sArg3);

	if (!funcArgs.sArg1.length())
		return "\"\"";

	if (funcArgs.nArg1 < 1)
		funcArgs.nArg1 = 1;
	if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
		funcArgs.nArg1 = funcArgs.sArg1.length();
	if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
		funcArgs.nArg2 = funcArgs.sArg1.length() + 1;
	if (!funcArgs.sArg2.length())
		return "\"" + funcArgs.sArg1 + "\"";

    replaceAll(funcArgs.sArg1, funcArgs.sArg2.c_str(), funcArgs.sArg3.c_str(), funcArgs.nArg1-1, funcArgs.nArg2-1);

	return "\"" + funcArgs.sArg1 + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the cnt()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_cnt(StringFuncArgs& funcArgs)
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_num(StringFuncArgs& funcArgs)
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
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_min(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sMultiArg.size())
		return "\"\"";
	string sMin = funcArgs.sMultiArg[0];
	for (size_t i = 1; i < funcArgs.sMultiArg.size(); i++)
	{
		if (sMin > funcArgs.sMultiArg[i])
			sMin = funcArgs.sMultiArg[i];
	}
	return "\"" + sMin + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the max()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_max(StringFuncArgs& funcArgs)
{
	if (!funcArgs.sMultiArg.size())
		return "\"\"";
	string sMax = funcArgs.sMultiArg[0];
	for (size_t i = 1; i < funcArgs.sMultiArg.size(); i++)
	{
		if (sMax < funcArgs.sMultiArg[i])
			sMax = funcArgs.sMultiArg[i];
	}
	return "\"" + sMax + "\"";
}


/////////////////////////////////////////////////
/// \brief Implementation of the sum()
/// function.
///
/// \param funcArgs StringFuncArgs&
/// \return string
///
/////////////////////////////////////////////////
static string strfnc_sum(StringFuncArgs& funcArgs)
{
	if (funcArgs.sMultiArg.size())
	{
		string sRet = "";
		for (size_t i = 0; i < funcArgs.sMultiArg.size(); i++)
			sRet += funcArgs.sMultiArg[i];
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
/// \brief This static function is used to construct
/// the string map.
///
/// \return map<string, StringFuncHandle>
///
/// This function is only called once during the
/// construction of the string parser class
/// instance.
/////////////////////////////////////////////////
static map<string, StringFuncHandle> getStringFuncHandles()
{
	map<string, StringFuncHandle> mHandleTable;

	mHandleTable["and"]                 = StringFuncHandle(VAL, strfnc_and, true);
	mHandleTable["ascii"]               = StringFuncHandle(STR, strfnc_ascii, false);
	mHandleTable["char"]                = StringFuncHandle(STR_VAL, strfnc_char, false);
	mHandleTable["cnt"]                 = StringFuncHandle(STR, strfnc_cnt, true);
	mHandleTable["findfile"]            = StringFuncHandle(STR_STROPT, strfnc_findfile, false);
	mHandleTable["findparam"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findparam, false);
	mHandleTable["findtoken"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findtoken, false);
	mHandleTable["getenvvar"]           = StringFuncHandle(STR, strfnc_getenvvar, false);
	mHandleTable["getfilelist"]         = StringFuncHandle(STR_VALOPT, strfnc_getfilelist, false);
	mHandleTable["getfileparts"]        = StringFuncHandle(STR, strfnc_getFileParts, false);
	mHandleTable["getfolderlist"]       = StringFuncHandle(STR_VALOPT, strfnc_getfolderlist, false);
	mHandleTable["getkeyval"]           = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_getkeyval, true);
	mHandleTable["getmatchingparens"]   = StringFuncHandle(STR, strfnc_getmatchingparens, false);
	mHandleTable["getopt"]              = StringFuncHandle(STR_VAL, strfnc_getopt, false);
	mHandleTable["is_alnum"]            = StringFuncHandle(STR, strfnc_isalnum, false);
    mHandleTable["is_alpha"]            = StringFuncHandle(STR, strfnc_isalpha, false);
	mHandleTable["is_blank"]            = StringFuncHandle(STR, strfnc_isblank, false);
    mHandleTable["is_cntrl"]            = StringFuncHandle(STR, strfnc_iscntrl, false);
    mHandleTable["is_digit"]            = StringFuncHandle(STR, strfnc_isdigit, false);
    mHandleTable["is_graph"]            = StringFuncHandle(STR, strfnc_isgraph, false);
    mHandleTable["is_lower"]            = StringFuncHandle(STR, strfnc_islower, false);
    mHandleTable["is_print"]            = StringFuncHandle(STR, strfnc_isprint, false);
    mHandleTable["is_punct"]            = StringFuncHandle(STR, strfnc_ispunct, false);
    mHandleTable["is_space"]            = StringFuncHandle(STR, strfnc_isspace, false);
    mHandleTable["is_upper"]            = StringFuncHandle(STR, strfnc_isupper, false);
    mHandleTable["is_xdigit"]           = StringFuncHandle(STR, strfnc_isxdigit, false);
	mHandleTable["locate"]              = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_locate, true);
	mHandleTable["max"]                 = StringFuncHandle(STR, strfnc_max, true);
	mHandleTable["min"]                 = StringFuncHandle(STR, strfnc_min, true);
	mHandleTable["num"]                 = StringFuncHandle(STR, strfnc_num, true);
	mHandleTable["or"]                  = StringFuncHandle(VAL, strfnc_or, true);
	mHandleTable["repeat"]              = StringFuncHandle(STR_VAL, strfnc_repeat, false);
	mHandleTable["replace"]             = StringFuncHandle(STR_VAL_VALOPT_STROPT, strfnc_replace, false);
	mHandleTable["replaceall"]          = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_replaceall, false);
	mHandleTable["split"]               = StringFuncHandle(STR_STR, strfnc_split, false);
	mHandleTable["str_not_match"]       = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_match, false);
	mHandleTable["str_not_rmatch"]      = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_rmatch, false);
	mHandleTable["strfnd"]              = StringFuncHandle(STR_STR_VALOPT, strfnc_strfnd, false);
	mHandleTable["strlen"]              = StringFuncHandle(STR, strfnc_strlen, false);
	mHandleTable["strmatch"]            = StringFuncHandle(STR_STR_VALOPT, strfnc_strmatch, false);
	mHandleTable["strrfnd"]             = StringFuncHandle(STR_STR_VALOPT, strfnc_strrfnd, false);
	mHandleTable["strrmatch"]           = StringFuncHandle(STR_STR_VALOPT, strfnc_strrmatch, false);
	mHandleTable["sum"]                 = StringFuncHandle(STR, strfnc_sum, true);
	mHandleTable["substr"]              = StringFuncHandle(STR_VAL_VALOPT, strfnc_substr, false);
	mHandleTable["textparse"]           = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_textparse, false);
	mHandleTable["timeformat"]          = StringFuncHandle(STR_VAL, strfnc_timeformat, false);
	mHandleTable["to_char"]             = StringFuncHandle(VAL, strfnc_to_char, true);
	mHandleTable["to_lowercase"]        = StringFuncHandle(STR, strfnc_to_lowercase, false);
	mHandleTable["to_string"]           = StringFuncHandle(STR, strfnc_to_string, false);
	mHandleTable["to_time"]             = StringFuncHandle(STR_STR, strfnc_to_time, false);
	mHandleTable["to_uppercase"]        = StringFuncHandle(STR, strfnc_to_uppercase, false);
	mHandleTable["xor"]                 = StringFuncHandle(VAL, strfnc_xor, true);

	return mHandleTable;
}




