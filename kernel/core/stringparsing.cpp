

#include "maths/parser_functions.hpp"
#include "../kernel.hpp"
#include <ctime>
#include <cstdlib>
#include <windows.h>
#include <algorithm>

extern value_type vAns;
extern Integration_Vars parser_iVars;
//extern bool bSupressAnswer;
extern mglGraph _fontData;
extern Plugin _plugin;

#define DEFAULT_NUM_ARG INT_MIN
#define NEWSTRING (char)23
// define the "End of transmission block" as string separator

typedef std::vector<std::string> s_vect;
typedef std::vector<long long int> n_vect;

// Define the string function arguments in a structure
struct StringFuncArgs
{
	string sArg1, sArg2, sArg3;
	s_vect sMultiArg;
	long long int nArg1, nArg2;
	n_vect nMultiArg;
	const Settings* opt;
};

typedef std::string (*StringFunc)(StringFuncArgs&);

// Define the signatures as enumeration
enum FunctionSignatureType
{
	PARSER_INT,
	VAL,

	PARSER_STRING,
	STR,

	PARSER_STRING_INT_INT,
	STR_VAL,
	STR_VALOPT,
	STR_VAL_VALOPT,

	PARSER_STRING_INT_INT_STRING,
	STR_VAL_VALOPT_STROPT,

	PARSER_STRING_STRING_INT_INT,
	STR_STR,
	STR_STROPT,
	STR_STROPT_VALOPT,
	STR_STR_VALOPT,
	STR_STR_VALOPT_VALOPT,

	PARSER_STRING_STRING_STRING_INT_INT,
	STR_STR_STROPT,
	STR_STR_STR_VALOPT_VALOPT
};

// Define the properties, which shall identify the functions semantically
struct StringFuncHandle
{
	StringFuncHandle()
	{
		bTakesMultiArguments = false;
	}
	StringFuncHandle(FunctionSignatureType _fType, StringFunc _fHandle, bool _bTakesMultiArguments) : fType(_fType), fHandle(_fHandle), bTakesMultiArguments(_bTakesMultiArguments) {}

	FunctionSignatureType fType;
	StringFunc fHandle;
	bool bTakesMultiArguments;
};

// Defines the return value of the string parser core
struct StringResult
{
	StringResult()
	{
		bOnlyLogicals = false;
	}
	StringResult(vector<string>& _vResult, vector<bool>& _vNoStringVal, bool _bOnlyLogicals) : vResult(_vResult), vNoStringVal(_vNoStringVal), bOnlyLogicals(_bOnlyLogicals) {}
	StringResult(const string& sRet, bool _bOnlyLogicals = false) : StringResult()
	{
		vResult.push_back(sRet);
		vNoStringVal.push_back(sRet.find('"') == string::npos);
		bOnlyLogicals = _bOnlyLogicals;
	}
	StringResult(const string& sRet, double* vals, int nvals) : StringResult()
	{
	    bOnlyLogicals = true;
	    vResult.push_back(sRet);
        vNoStringVal.resize(nvals, true);

	    for (int i = 0; i < nvals; i++)
        {
            vNumericalValues.push_back(vals[i]);
        }
	}

	vector<string> vResult;
	vector<bool> vNoStringVal;
	vector<double> vNumericalValues;
	bool bOnlyLogicals;
};

// Defines the possible flags for the string parser
enum StringParserFlags
{
	NO_FLAG = 0,
	NO_QUOTES = 1,
	PEEK = 2,
	KEEP_MASKED_QUOTES = 4,
	KEEP_MASKED_CONTROL_CHARS = 8
};


// Function handler:
// ======================
static string parser_ApplySpecialStringFuncs(string sLine, Datafile&, Parser&, const Settings&, map<string, vector<string> >&);
static string parser_ApplyStringFuncs(string sLine, Datafile&, Parser&, const Settings&, map<string, vector<string> >&);
static void parser_StringFuncHandler(string& sLine, const string& sFuncName, Datafile&, Parser&, const Settings&, map<string, vector<string> >&, StringFuncHandle);
static map<string, StringFuncHandle> parser_getStringFuncHandles();
static string removeMaskedStrings(const string& sString);
static string addMaskedStrings(const string& sString);
static string removeQuotationMarks(const string& sString);
static string addQuotationMarks(const string& sString);
static bool parser_detectStringLogicals(const string& sString);
static size_t parser_detectPathTokens(const string& sString, size_t nPos);

static StringResult parser_StringParserCore(string& sLine, string sCache, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> > mStringVectorVars, bool bParseNumericals = true);

static string parser_GetDataForString(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, size_t n_pos);
static string parser_NumToString(const string& sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars);
static int parser_StoreStringResults(vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string sObject, Datafile& _data, Parser& _parser, const Settings& _option);
static string parser_CreateStringOutput(Parser& _parser, StringResult& strRes, string& sLine, int parserFlags);
static vector<bool> parser_ApplyElementaryStringOperations(vector<string>& vFinal, Parser& _parser, const Settings& _option, bool& bReturningLogicals);
static string parser_CreateStringVectorVar(const vector<string>& vStringVector, map<string, vector<string> >& mStringVectorVars);
static bool parser_containsStringVectorVars(const string& sLine, const map<string, vector<string> >& mStringVectorVars);
static vector<string> parser_EvaluateStringVectors(string sLine, const map<string, vector<string> >& mStringVectorVars, Parser& _parser);
static void parser_ExpandStringVectorComponents(vector<string>& vStringVector);
static string parser_evalStringLogic(string sLine, Parser& _parser, bool& bReturningLogicals);
static string parser_evalStringTernary(string sLine, Parser& _parser);
static vector<string> parser_getStringTernaryExpression(string& sLine, size_t& nPos);
static string parser_concatenateStrings(const string& sExpr);

// String functions:
// ======================
// string STRINGFUNC(ARGS)
//
// Parser functions:
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, n_vect&);
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, bool& bLogicalOnly);
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, n_vect&, n_vect&);
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, n_vect&, n_vect&, s_vect&);
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, s_vect&, n_vect&, n_vect&);
static size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&);
//


// Function signatures
// ====================
// bool PARSER(const string&, string&)
// STR__STR
// str = to_string(EXPR)
static string strfnc_to_string(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg1.length())
        return "\"\"";

	if (funcArgs.sArg1.find_first_not_of(" ") != string::npos && funcArgs.sArg1[funcArgs.sArg1.find_first_not_of(" ")] == '"')
		return funcArgs.sArg1; // Already is a string

    // Is not a string
	return "\"" + funcArgs.sArg1 + "\"";
}

// str = string_cast(EXPR)
//string strfnc_string_cast(StringFuncArgs& funcArgs);

// STR__STR
// str = to_uppercase(str)
static string strfnc_to_uppercase(StringFuncArgs& funcArgs)
{
	return "\"" + toUpperCase(funcArgs.sArg1) + "\"";
}

// str = to_lowercase(str)
static string strfnc_to_lowercase(StringFuncArgs& funcArgs)
{
	return "\"" + toLowerCase(funcArgs.sArg1) + "\"";
}

// str = getenvvar(str)
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

// {str} = getfileparts(str)
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

// cmd = to_cmd(str)
// string strfnc_to_cmd(StringFuncArgs& funcArgs);

// {str} = getfilelist(str, [val])
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
// {str} = getfolderlist(str, [val])
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

// VAL__STR
// val = strlen(str)
static string strfnc_strlen(StringFuncArgs& funcArgs)
{
	return toString((int)removeMaskedStrings(funcArgs.sArg1).length());
}

// val = getmatchingparens(str)
static string strfnc_getmatchingparens(StringFuncArgs& funcArgs)
{
	return toString((int)getMatchingParenthesis(removeMaskedStrings(funcArgs.sArg1)) + 1);
}

// {val} = ascii(str)
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

// LOG__STR
// log = is_string(EXPR)
// string strfnc_is_string(StringFuncArgs& funcArgs)

// log = is_data(EXPR)
// string strfnc_is_data(StringFuncArgs& funcArgs);

// ----------------------------
// bool PARSER(const string&, int&)
// STR__VAL
// str = to_char(num)
static string strfnc_to_char(StringFuncArgs& funcArgs)
{
	string sToChar = "";
	for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
	{
		sToChar += (char)(funcArgs.nMultiArg[i]);
	}
	return "\"" + sToChar + "\"";
}

// val = and(num)
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

// val = or(num)
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

// val = xor(num)
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

// ----------------------------
// bool PARSER(const string&, string&, string&)
// LOG__STR_STROPT
// log = findfile(str, [str])
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

// STR__STR_STR
// {str} = split(str, str)
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

// val = to_time(str, str)
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

// STR__STR_STROPT
// str = valtostr(EXPR, [str], [val])
// string strfnc_valtostr(StringFuncArgs& funcArgs)

// ----------------------------
// bool PARSER(const string&, string&, string&, int&)
// VAL__STR_STR_VALOPT
// val = strfnd(str, str, [val])
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

// val = strmatch(str, str, [val])
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

// val = str_not_match(str, str, [val])
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

// val = strrfnd(str, str, [val])
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

// val = strrmatch(str, str, [val])
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

// val = str_not_rmatch(str, str, [val])
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

// ----------------------------
// bool PARSER(const string&, string&, string&, string&)
// VAL__STR_STR_STROPT
// val = findparam(str, str, [str])
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
		nMatch = matchParams(funcArgs.sArg2, funcArgs.sArg1, funcArgs.sArg3[0]);
	}
	else
	{
		nMatch = matchParams(funcArgs.sArg2, funcArgs.sArg1);
	}
	if (nMatch != string::npos)
		return toString((int)(nMatch + 1));
	else
		return "0";
}

// ----------------------------
// bool PARSER(const string&, string&, int&, int&)
// STR__STR_VAL_VALOPT
// str = substr(str, val, [val])
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

// ----------------------------
// bool PARSER(const string&, string&, int&)
// STR__STR_VAL
// str = repeat(str, val)
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

static string padWithZeros(int nTime, size_t nLength)
{
	string sPadded = toString(nTime);
	sPadded.insert(0, nLength - sPadded.length(), '0');
	return sPadded;
}

// str = timeformat(str, val)
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

// str = char(str, val)
static string strfnc_char(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	if (funcArgs.nArg1 <= 1)
		return "\"" + funcArgs.sArg1.substr(0, 1) + "\"";
	if ((size_t)funcArgs.nArg1 >= funcArgs.sArg1.length())
		return "\"" + funcArgs.sArg1.substr(funcArgs.sArg1.length() - 1) + "\"";
	return "\"" + funcArgs.sArg1.substr(funcArgs.nArg1 - 1, 1) + "\"";
}

// str = getopt(str, val)
static string strfnc_getopt(StringFuncArgs& funcArgs)
{
	funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
	if (funcArgs.nArg1 <= 1)
		funcArgs.nArg1 = 1;
	if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
		return "\"\"";

	return "\"" + getArgAtPos(funcArgs.sArg1, funcArgs.nArg1 - 1) + "\"";
}

// VAL__STR_VALOPT
// {val} = getindices(str, [val])
// string strfnc_getindices(StringFuncArgs& funcArgs);

// ----------------------------
// bool PARSER(const string&, string&, string&, int&, int&)
// STR__STR_STR_VALOPT_VALOPT
// str = replace(str, str, [val], [val])
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

// {val} = textparse(str, str, [pos], [pos])
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

// {val} = locate({str}, str, [tol])
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

// STR__STR_STR_STROPT
// val = findtoken(str, str, [str])
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

// ----------------------------
// bool PARSER(const string&, string&, string&, string&, int&, int&)
// STR__STR_STR_STR_VALOPT_VALOPT
// str = replaceall(str, str, str, [val], [val])
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

    replaceAll(funcArgs.sArg1, funcArgs.sArg2, funcArgs.sArg3, funcArgs.nArg1-1, funcArgs.nArg2-1);

	/*for (int i = funcArgs.nArg1 - 1; i < funcArgs.nArg2 - 1; i++)
	{
		if ((size_t)i == funcArgs.sArg1.length())
			break;
		if (funcArgs.sArg1.substr(i, funcArgs.sArg2.length()) == funcArgs.sArg2)
		{
			funcArgs.sArg1.replace(i, funcArgs.sArg2.length(), funcArgs.sArg3);
			funcArgs.nArg2 += funcArgs.sArg3.length() - funcArgs.sArg2.length() + 1;
			i += funcArgs.sArg3.length() - 1;
		}
	}*/

	return "\"" + funcArgs.sArg1 + "\"";
}

// ----------------------------
// bool PARSER(const string&, string&)
// VAL__STR
// val = cnt({str})
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

// val = num({str})
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

// STR__STR
// str = min({str})
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

// str = max({str})
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

// str = sum({str})
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
//
//

// Static map containing parameters with their corresponding flags
static map<string, int> getStringParamFlags()
{
    map<string, int> stringParams;

    stringParams["noquotes"] = NO_QUOTES;
    stringParams["nq"] = NO_QUOTES;
    stringParams["peek"] = PEEK;
    stringParams["print"] = NO_QUOTES | PEEK;
    stringParams["komq"] = KEEP_MASKED_QUOTES;
    stringParams["kmq"] = KEEP_MASKED_CONTROL_CHARS | KEEP_MASKED_QUOTES;

    return stringParams;
}

// static function for decoding the string paramters
// and removing the parameter list from
static int decodeStringParams(string& sLine)
{
    static map<string, int> stringParams = getStringParamFlags();

    int parserFlags = NO_FLAG;
    vector<int> vPositions;

    // Search for every parameter and add the
    // corresponding flag, if the parameter was
    // detected
    for (auto iter = stringParams.begin(); iter != stringParams.end(); ++iter)
    {
        vPositions.push_back(matchParams(sLine, iter->first));
        if (vPositions.back())
            parserFlags |= iter->second;
    }

    // Sort the vector (only about 6 elements,
    // should be quite fast) to obtain the
    // smallest, non-zero object quite fast
    sort(vPositions.begin(), vPositions.end());

    // Find the start of the parameter list and
    // erase them from the expression
    for (size_t i = 0; i < vPositions.size(); i++)
    {
        if (vPositions[i])
        {
            sLine.erase(sLine.rfind('-', vPositions[i]));
            break;
        }
    }

    // Return the calculated flags
    return parserFlags;
}


// This function is the interface to the current file.
// It wraps the string parser core, which will do the main tasks
int parser_StringParser(string& sLine, string& sCache, Datafile& _data, Parser& _parser, const Settings& _option, bool bSilent)
{
	sLine = " " + sLine + " ";

	// Process the parameters and store their
	// results in the parser flags
	int parserFlags = decodeStringParams(sLine);

    // Create a map for the string vector vars, which may
    // occur in the current expression. This map will be used
    // in all recursions
	map<string, vector<string> > mStringVectorVars;

	// Perform the actual work in the string parser core function
	// This function will parse the string, call the corresponding
	// static functions and perform all required logical operations
	StringResult StrRes = parser_StringParserCore(sLine, sCache, _data, _parser, _option, mStringVectorVars);

	// The result of the string parser core has to be parsed, so that
	// it is readable in the terminal. This is done here in this
	// function
	string sConsoleOut = parser_CreateStringOutput(_parser, StrRes, sLine, parserFlags);

	// The output is probably not desired
	if (NumeReKernel::bSupressAnswer)
		bSilent = true;

    // Print the output
	if ((!bSilent || parserFlags & PEEK) && !StrRes.bOnlyLogicals)
    {
        // The Line break function is not needed any more
        // because the terminal is capable of breaking the
        // lines by itself
        if (parserFlags & PEEK)
            NumeReKernel::printPreFmt("\r" + sConsoleOut + "\n");
		else
            NumeReKernel::printPreFmt(sConsoleOut + "\n");
    }

    // return the corresponding value. The string parser may
    // evaluate the command line to only contain logical or
    // numerical expressions
	if (StrRes.bOnlyLogicals)
		return -1;
	else
		return 1;
}

// This static function determines, whether the equal sign at eq_pos is
// an assignment operator and no boolean expression
static bool parser_isAssignmentOperator(const string& sLine, size_t eq_pos)
{
    if (!eq_pos || eq_pos >= sLine.length())
        return false;
    return sLine[eq_pos - 1] != '!' && sLine[eq_pos - 1] != '<' && sLine[eq_pos - 1] != '>' && sLine[eq_pos + 1] != '=';
}

// This static function determines, whether the passed string is
// simple, i.e. it is a string literal without any operation
static bool parser_isSimpleString(const string& sLine)
{
    size_t nQuotes = 0;

    // Got through the string
    for (size_t i = 0; i < sLine.size(); i++)
    {
        if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
        {
            // Count quotation marks
            nQuotes++;
            continue;
        }

        // Not in quotes and not a space?
        if (!(nQuotes % 2) && !isspace(sLine[i]))
            return false;
    }

    return true;
}

// This static function contains the core string functionality
static StringResult parser_StringParserCore(string& sLine, string sCache, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> > mStringVectorVars, bool bParseNumericals)
{
	StringResult strRes;

	string sObject;
	bool bObjectContainsTablesOrClusters = false;

	// If the current line is a simple string,
	// Strip the surrounding spaces and return directly
	// This saves between 30-70% of the evaluation time
	if (parser_isSimpleString(sLine))
    {
        StripSpaces(sLine);

        strRes.vResult.push_back(sLine);
        strRes.vNoStringVal.push_back(false);

        return strRes;
    }

	// Identify target vectors and parse this as a list
	size_t eq_pos = sLine.find('=');
	size_t brace_pos = sLine.find('{');
	size_t string_pos = sLine.find("string(");

	if (brace_pos != string::npos
        && eq_pos != string::npos
        && brace_pos < eq_pos
        && parser_isAssignmentOperator(sLine, eq_pos))
	{
	    // find the actual equal sign in the current string expression
		while (isInQuotes(sLine, eq_pos) && sLine.find('=', eq_pos + 1) != string::npos)
			eq_pos = sLine.find('=', eq_pos + 1);

        // Did we find an actual equal sign?
		if (!isInQuotes(sLine, eq_pos) && parser_isAssignmentOperator(sLine, eq_pos))
		{
		    // Store the left side and erase it from the original string
			string sLeftSide = sLine.substr(0, eq_pos + 1);
			sLine.erase(0, eq_pos + 1);

			// Apply the string parser core to the right side of the assignment
			// Create a string vector variable from the result of the string parser
			StringResult _res = parser_StringParserCore(sLine, "", _data, _parser, _option, mStringVectorVars);
			string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

			if (!strvar.length())
				throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // Combine the left side and the string vector variable to
            // a new assignment
			sLine = sLeftSide + strvar;
		}
	}
	else if (brace_pos != string::npos
        && string_pos != string::npos
        && eq_pos != string::npos
        && string_pos < brace_pos
        && eq_pos > string_pos
        && eq_pos < brace_pos
        && parser_isAssignmentOperator(sLine, eq_pos))
	{
	    // Examine the left side of the assignment
		for (unsigned int i = 0; i < sLine.find('='); i++)
		{
		    // Search for a character, which isn't a whitespace
			if (sLine[i] != ' ')
			{
			    // If the current character is the beginning of "string()"
				if (sLine.substr(i, 7) == "string(")
				{
				    // Jump over the argument parenthesis
					i += getMatchingParenthesis(sLine.substr(i + 6)) + 6;

					// Find the next non-whitespace character and check, whether it is an equal sign
					if (sLine.find_first_not_of(' ', i) == sLine.find('='))
					{
					    // Store the left side and erase it from the original string
						string sLeftSide = sLine.substr(0, sLine.find('=') + 1);
						sLine.erase(0, sLine.find('=') + 1);

						// Apply the string parser core to the right side of the assignment
                        // Create a string vector variable from the result of the string parser
						StringResult _res = parser_StringParserCore(sLine, "", _data, _parser, _option, mStringVectorVars);
						string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

						if (!strvar.length())
							throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                        // Combine the left side and the string vector variable to
                        // a new assignment
						sLine = sLeftSide + strvar;
					}
					break;
				}
				else
				{
				    // Otherwise parse as a vector expression
					parser_VectorToExpr(sLine, _option);
					break;
				}
			}
		}
	}

	eq_pos = sLine.find('=');

	// Recurse for multiple store targets
	// Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.
	if (sLine.find(',') != string::npos && eq_pos != string::npos && !isInQuotes(sLine, eq_pos))
	{
	    // Get the left part of the assignment
		string sStringObject = sLine.substr(0, eq_pos);

		// Ensure that the left part contains a data or a string object,
		// otherwise it is cleared
		if (sStringObject.substr(sStringObject.find_first_not_of(' '), 7) == "string("
				|| sStringObject.substr(sStringObject.find_first_not_of(' '), 5) == "data("
				|| _data.containsTablesOrClusters(sStringObject))
		{
		    // Find the equal sign after the closing parenthesis
			size_t nPos = getMatchingParenthesis(sLine);
			nPos = sLine.find('=', nPos);

			// Ensure that it exists
			if (nPos == string::npos)
				throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // Get the data object including the assignment operator and
            // cut this part from the original line
			if (sLine[nPos + 1] == '=')
				nPos++;
			sStringObject = sLine.substr(0, nPos + 1);
			sLine.erase(0, nPos + 1);
		}
		else
			sStringObject.clear();

        // Remove whitespaces
		StripSpaces(sLine);

		// If the current line contains more than one string expressions
		if (sLine != getNextArgument(sLine, false))
		{
			string sRecursion = "";
			vector<string> vResult;

			// While the current line is not empty
			while (sLine.length())
			{
			    // Cut of the first argument
				sRecursion = getNextArgument(sLine, true);

				// Apply the string parser on the current argument and store its
				// results in a common vector
                StringResult _res = parser_StringParserCore(sRecursion, "", _data, _parser, _option, mStringVectorVars);

                for (size_t n = 0; n < _res.vResult.size(); n++)
                    vResult.push_back(_res.vResult[n]);
            }

			// Create a new string vector variable from the results
			sLine = parser_CreateStringVectorVar(vResult, mStringVectorVars);
		}

		// Prepend the stored string object to the current input line
		if (sStringObject.length())
			sLine = sStringObject + sLine;
	}

	// Find the start of the asignee
	size_t n_pos = 0;
	eq_pos = sLine.find('=');

	if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && parser_isAssignmentOperator(sLine, eq_pos))
		n_pos = eq_pos + 1;

	// Get the string variables
	if (_data.containsStringVars(sLine.substr(n_pos)))
		_data.getStringValues(sLine, n_pos);

    // Does the current line contain candidates
    // for string functions?
    if (sLine.find('(') != string::npos)
    {
        // Apply the standard string functions
        sLine = parser_ApplyStringFuncs(sLine, _data, _parser, _option, mStringVectorVars);

        // Apply the special string functions
        sLine = parser_ApplySpecialStringFuncs(sLine, _data, _parser, _option, mStringVectorVars);

        // Get the position of the equal sign after the
        // application of all string functions
        eq_pos = sLine.find('=');
    }

	// Extract target object
	if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && parser_isAssignmentOperator(sLine, eq_pos))
	{
		if (sLine.substr(0, eq_pos).find("data(") != string::npos || _data.containsTablesOrClusters(sLine.substr(0, eq_pos)))
		{
			sObject = sLine.substr(0, eq_pos);
			sLine.erase(0, eq_pos + 1);
		}
	}
    else if (!sObject.length() && sCache.length())
    {
        sObject = sCache;
    }

    if (_data.containsTablesOrClusters(sObject))
        bObjectContainsTablesOrClusters = true;

	// Get the contents of "string()", "data()" and the other caches
	sLine = parser_GetDataForString(sLine, _data, _parser, _option, mStringVectorVars, n_pos);

	// If the line now doesn't contain any strings, return with the onlyLogicals flag set
	if (!containsStrings(sLine) && !_data.containsStringVars(sLine) && !parser_containsStringVectorVars(sLine, mStringVectorVars) && !bObjectContainsTablesOrClusters)
	{
		// make sure that there are parseable characters between the operators
		if (sLine.find_first_not_of("+-*/:?!.,;%&<>=^ ") != string::npos && bParseNumericals)
		{
			int nResults = 0;
			value_type* v = 0;

			// Parse the epression
			_parser.SetExpr(sLine);
			v = _parser.Eval(nResults);
			vAns = v[0];
			eq_pos = sLine.find('=');

			// Remove the left part of the assignment, because it's already assigned
			if (eq_pos != string::npos
                && eq_pos
                && eq_pos < sLine.length() + 1
                && parser_isAssignmentOperator(sLine, eq_pos))
				sLine.erase(0, eq_pos + 1);

			StripSpaces(sLine);

			// Return with the numbers already contained
			return StringResult(sLine, v, nResults);
		}

		// return with the onlyLogicals flag
		return StringResult(sLine, true);
	}

	n_pos = 0;

	// If there are any opening parentheses, it is possible that we need
	// to pre-evaluate its contents
	if (sLine.find('(') != string::npos)
	{
		size_t nQuotes = 0;

		// Examine the whole command line
		for (size_t i = 0; i < sLine.length(); i++)
		{
		    // Jump over string literals
			if (sLine[i] == '"')
			{
				if (i && sLine[i - 1] == '\\')
					continue;
				nQuotes++;
			}

			// Consider the var parsing feature
			if (sLine[i] == '#' && !(nQuotes % 2))
			{
			    // Examine, whether the var parsing feature was used
			    // correctly and search for the end of its current application
				for (size_t j = i; j < sLine.length(); j++)
				{
					if (sLine[j] == '"')
						throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

					if ((sLine[j] == '(' || sLine[j] == '{') && getMatchingParenthesis(sLine.substr(j)) != string::npos)
						j += getMatchingParenthesis(sLine.substr(j));

					if (sLine[j] == ' ' || sLine[j] == '+')
					{
						i = j;
						break;
					}

					if (j + 1 == sLine.length())
					{
						i = j;
					}
				}
			}

			// Examine the current parenthesis: is it able to pre-evaluate the
			// current parenthesis?
			if (sLine[i] == '(' && !(nQuotes % 2))
			{
			    // Ensure that its counterpart exists
				if (getMatchingParenthesis(sLine.substr(i)) == string::npos)
					throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, i);

				size_t nPos = getMatchingParenthesis(sLine.substr(i)) + i;

				// Ignore any calls to "string()"
				if (i < 6 || (i >= 6 && sLine.substr(i - 6, 6) != "string"))
				{
				    // The contents of the parenthesis
					string sString = sLine.substr(i + 1, nPos - i - 1);

					// Pre-evaluate the contents of the parenthesis
					if (i > 0 && !checkDelimiter(sLine.substr(i - 1, nPos - i + 2))) // this is probably a numerical function. Keep the parentheses
					{
					    // Do only something, if the contents are containings strings
						if (containsStrings(sString) || _data.containsStringVars(sString) || parser_containsStringVectorVars(sString, mStringVectorVars))
						{
							StringResult _res = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars);
							string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

							if (!strvar.length())
								throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

							sLine = sLine.substr(0, i + 1) + strvar + sLine.substr(nPos);
						}
					}
					else // replace the whole parenthesis
					{
					    // Do only something, if the contents are containing strings
						if (containsStrings(sString) || _data.containsStringVars(sString) || parser_containsStringVectorVars(sString, mStringVectorVars))
						{
							StringResult _res = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars);
							string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

							if (!strvar.length())
								throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

							sLine = sLine.substr(0, i) + strvar + sLine.substr(nPos + 1);
						}
					}
				}
			}
		}
	}

	// Are there any vector braces left? We will only parse them, if they
    // contain a string part or a string parser
	if (sLine.find('{') != string::npos
        && (sLine.find('"') != string::npos || sLine.find('#') != string::npos))
	{
		n_pos = 0;
		eq_pos = sLine.find('=');

		if (eq_pos != string::npos
            && parser_isAssignmentOperator(sLine, eq_pos)
            && !sObject.length()
            && !isInQuotes(sLine, eq_pos))
			n_pos = eq_pos + 1;

        size_t nQuotes = 0;

		for (size_t i = n_pos; i < sLine.length(); i++)
        {
            if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
                nQuotes++;

            if (!(nQuotes % 2) && sLine[i] == '{')
            {
                size_t nmatching = getMatchingParenthesis(sLine.substr(i));
                string sVectorTemp = sLine.substr(i+1, nmatching-1);
                StringResult tempres = parser_StringParserCore(sVectorTemp, "", _data, _parser, _option, mStringVectorVars);

                if (!tempres.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Create a string vector variable from the returned vector
                // if it is a string. Otherwise simple use the first return
                // value, which already contains a numerical vector
                if (!tempres.bOnlyLogicals)
                    sVectorTemp = parser_CreateStringVectorVar(tempres.vResult, mStringVectorVars);
                else
                    sVectorTemp = tempres.vResult.front();

                sLine.replace(i, nmatching+1, sVectorTemp);
            }
        }
	}

	// Strip all whitespaces and ensure, that there's something left
	StripSpaces(sLine);

	if (!sLine.length())
		return StringResult("");

    // If the current line doesn't contain any further string literals or return values
    // return it
	if (sLine.find('"') == string::npos && sLine.find('#') == string::npos && !parser_containsStringVectorVars(sLine, mStringVectorVars) && !bObjectContainsTablesOrClusters)
	{
        // Ensure that this is no false positive
		if (sLine.find("string(") != string::npos || _data.containsStringVars(sLine))
			throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

        // return the current line
		return StringResult(sLine);
	}

	// Extract the object, if not already done
	eq_pos = sLine.find('=');

	if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && parser_isAssignmentOperator(sLine, eq_pos))
	{
		sObject = sLine.substr(0, eq_pos);
		sLine = sLine.substr(eq_pos + 1);
	}

	// Apply the "#" parser to the string
	sLine = parser_NumToString(sLine, _data, _parser, _option, mStringVectorVars);

	// Split the list to a vector
	strRes.vResult = parser_EvaluateStringVectors(sLine, mStringVectorVars, _parser);

	// Ensure that there is at least one result
	if (!strRes.vResult.size())
		throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

	// Apply some elementary operations such as concatenation and logical operations
	strRes.vNoStringVal = parser_ApplyElementaryStringOperations(strRes.vResult, _parser, _option, strRes.bOnlyLogicals);

	// store the string results in the variables or inb "string()" respectively
	if (!parser_StoreStringResults(strRes.vResult, strRes.vNoStringVal, sObject, _data, _parser, _option))
		throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

    // Return the evaluated string command line
	return strRes;
}

// This static function is used to create a string vector var
static string parser_CreateStringVectorVar(const vector<string>& vStringVector, map<string, vector<string> >& mStringVectorVars)
{
	// Return, if empty (something else went wrong)
	if (!vStringVector.size())
		return "";

	string strVectName = "_~STRVECT[" + toString((int)mStringVectorVars.size()) + "]";

	// Does it already exist?
	if (mStringVectorVars.find(strVectName) != mStringVectorVars.end())
		throw SyntaxError(SyntaxError::STRING_ERROR, strVectName, SyntaxError::invalid_position);

	// save the vector
	mStringVectorVars[strVectName] = vStringVector;

	return strVectName;
}

// This static function determines, whether there are string vector
// vars in the passed command line
static bool parser_containsStringVectorVars(const string& sLine, const map<string, vector<string> >& mStringVectorVars)
{
    // If the command line or the string vector vars are empty return false
	if (!sLine.length() || !mStringVectorVars.size())
		return false;

    // Try to find at least one of the known string vector vars
	for (auto iter = mStringVectorVars.begin(); iter != mStringVectorVars.end(); ++iter)
	{
	    // There's one: return true
		if (sLine.find(iter->first) != string::npos)
			return true;
	}

	// Nothing found
	return false;
}

// This static function is used to determine, whether a string vector
// component is numerical parsable
static bool parser_isNumericCandidate(const string& sComponent)
{
    if (sComponent.front() != '"' && sComponent.find_first_of("+-*/^!&|<>=% ?:,") != string::npos)
        return true;
    return false;
}

// This static function evaluates the passed string vector
// and returns it's evaluated components in separate vector
// components
static vector<string> parser_EvaluateStringVectors(string sLine, const map<string, vector<string> >& mStringVectorVars, Parser& _parser)
{
	vector<string> vRes;
    const map<string, vector<double> >& mNumVectorVars = _parser.GetVectors();

	// As long as the current vector is not empty
	while (sLine.length())
	{
	    // Get-cut the next argument
		string sCurrentComponent = getNextArgument(sLine, true);

		// If the current component does not contain any further
		// vectors, push it back.
		// Otherwise expand the contained vectors
		if (!parser_containsStringVectorVars(sCurrentComponent, mStringVectorVars))
			vRes.push_back(sCurrentComponent);
		else
		{
		    // Expand the contained vectors
			size_t nCurrentComponent = 0;

			// This while loop is terminated with a break command
			while (true)
			{
				string currentline = sCurrentComponent;
				bool bHasComponents = false;

				// Replace all found vectors with their nCurrentComponent-th component
				for (auto iter = mStringVectorVars.begin(); iter != mStringVectorVars.end(); ++iter)
				{
					size_t nMatch = 0;

					// Vector found: replace its occurence with its nCurrentComponent-th value
					while ((nMatch = currentline.find(iter->first)) != string::npos)
					{
					    // Handle the size of the current vector correspondingly
						if ((iter->second).size() > nCurrentComponent)
						{
							bHasComponents = true;
							if (parser_isNumericCandidate((iter->second)[nCurrentComponent]))
                                currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second)[nCurrentComponent] + ")");
                            else
                                currentline.replace(nMatch, (iter->first).length(), (iter->second)[nCurrentComponent]);
						}
						else if ((iter->second).size() == 1)
						{
						    if (parser_isNumericCandidate((iter->second)[0]))
                                currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second)[0] + ")");
                            else
                                currentline.replace(nMatch, (iter->first).length(), (iter->second)[0]);
						}
						else
							currentline.replace(nMatch, (iter->first).length(), "\"\"");
					}
				}

				// Replace all found numerical vectors with their nCurrentComponent-th component
				for (auto iter = mNumVectorVars.begin(); iter != mNumVectorVars.end(); ++iter)
				{
					size_t nMatch = 0;

					// Vector found: replace its occurence with its nCurrentComponent-th value
					while ((nMatch = currentline.find(iter->first, nMatch)) != string::npos)
					{
					    // Do not replace vector variables, which are part
					    // of a string
					    if (isInQuotes(currentline, nMatch))
					    {
					        nMatch++;
                            continue;
					    }

					    // Handle the size of the current vector correspondingly
						if ((iter->second).size() > nCurrentComponent)
						{
							bHasComponents = true;
							currentline.replace(nMatch, (iter->first).length(), toCmdString((iter->second)[nCurrentComponent]));
						}
						else if ((iter->second).size() == 1)
						{
							currentline.replace(nMatch, (iter->first).length(), toCmdString((iter->second)[0]));
						}
						else
							currentline.replace(nMatch, (iter->first).length(), "nan");
					}
				}

				// Break the loop, if there are no further vector components
				if (!bHasComponents)
					break;

                // Push back the current line and increment the component
				vRes.push_back(currentline);
				nCurrentComponent++;
			}
		}
	}

	return vRes;
}

// This static function expands the internal multi-expressions
// into vector components by enlarging the dimension of the vector
static void parser_ExpandStringVectorComponents(vector<string>& vStringVector)
{
    // Examine each vector component
	for (size_t i = 0; i < vStringVector.size(); i++)
	{
	    // If the next argument is not equal to the current component itself
		if (getNextArgument(vStringVector[i], false) != vStringVector[i])
		{
		    // Store the first part of the multi-expression in the current component
			string sComponent = vStringVector[i];
			vStringVector[i] = getNextArgument(sComponent, true);
			size_t nComponent = 1;

			// As long as the component is not empty
			while (sComponent.length())
			{
			    // Get the next argument and insert it after the
			    // previous argument into the string vector
				vStringVector.insert(vStringVector.begin() + i + nComponent, getNextArgument(sComponent, true));
				nComponent++;
			}

			// Jump over the already expanded components
			i += nComponent;
		}
	}
}

// This static function is used to apply some special string functions
// to the command line, which cannot be abstrahized like the others
static string parser_ApplySpecialStringFuncs(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
	size_t n_pos = 0;
	size_t nParensMatch;

	// str string_cast(EXPR)
	while (sLine.find("string_cast(", n_pos) != string::npos)
	{
		n_pos = sLine.find("string_cast(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 11;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
		nPos += nParensMatch;
		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sToString = sLine.substr(n_pos + 12, nPos - n_pos - 12);
			if (sToString.find('"') != string::npos || sToString.find('#') != string::npos)
			{
				StringResult strRes = parser_StringParserCore(sToString, "", _data, _parser, _option, mStringVectorVars);
				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				sToString = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
			}
			else
				sToString = "\"" + sToString + "\"";
			sLine = sLine.substr(0, n_pos) + sToString + sLine.substr(nPos + 1);
		}
		n_pos++;
	}

	n_pos = 0;
	// cmd to_cmd(str)
	while (sLine.find("to_cmd(", n_pos) != string::npos)
	{
		n_pos = sLine.find("to_cmd(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nParPos = getMatchingParenthesis(sLine.substr(n_pos));

		if (nParPos == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, n_pos);

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
		    n_pos += 6;
		    nParPos -= 6;
			string sCmdString = sLine.substr(n_pos + 1, nParPos - 1);
			StripSpaces(sCmdString);

			if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString) || parser_containsStringVectorVars(sCmdString, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sCmdString, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				// use only the first one
				sCmdString = strRes.vResult[0];
			}

			sLine = sLine.substr(0, n_pos - 6) + removeQuotationMarks(sCmdString) + sLine.substr(n_pos + nParPos + 1);
			n_pos -= 5;
		}
	}

	n_pos = 0;
	// val to_value(str)
	while (sLine.find("to_value(", n_pos) != string::npos)
	{
		n_pos = sLine.find("to_value(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nParPos = getMatchingParenthesis(sLine.substr(n_pos));

		if (nParPos == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, n_pos);

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
		    n_pos += 8;
		    nParPos -= 8;
			string sToValue = sLine.substr(n_pos + 1, nParPos - 1);
			StripSpaces(sToValue);

			if (containsStrings(sToValue) || _data.containsStringVars(sToValue) || parser_containsStringVectorVars(sToValue, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sToValue, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				// use only the first one
				sToValue = strRes.vResult[0];
			}

			sLine = sLine.substr(0, n_pos - 8) + removeQuotationMarks(sToValue) + sLine.substr(n_pos + nParPos + 1);
			n_pos -= 7;
		}
	}

	n_pos = 0;
	// log is_string(EXPR)
	while (sLine.find("is_string(", n_pos) != string::npos)
	{
		n_pos = sLine.find("is_string(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 9;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if ((!n_pos || isDelimiter(sLine[n_pos-1]))
				&& (containsStrings(sLine.substr(n_pos + 10, nPos - n_pos - 10))
					|| _data.containsStringVars(sLine.substr(n_pos + 10, nPos - n_pos - 10))))
			sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos + 1);
		else if (isInQuotes(sLine, nPos)
				 || isInQuotes(sLine, n_pos))
			n_pos++;
		else
			sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos + 1);
	}

	n_pos = 0;
	// {val} = getindices(str, [val])
	while (sLine.find("getindices(", n_pos) != string::npos)
	{
		n_pos = sLine.find("getindices(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 10;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string _sObject = sLine.substr(n_pos + 11, nPos - n_pos - 11);
			StringResult strRes = parser_StringParserCore(_sObject, "", _data, _parser, _option, mStringVectorVars);

			if (!strRes.vResult.size())
				throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

			// use only first one
			string sType = strRes.vResult[0];
			int nType = 0;
			_sObject = getNextArgument(sType, true);

			if (!sType.length())
			{
				sType = "0";
			}

			if (_sObject[0] == '"')
				_sObject.erase(0, 1);

			if (_sObject[_sObject.length() - 1] == '"')
				_sObject.erase(_sObject.length() - 1);

			StripSpaces(_sObject);

			if (containsStrings(sType) || parser_containsStringVectorVars(sType, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sType, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				sType = strRes.vResult[0];
			}

			_parser.SetExpr(sType);
			nType = (int)_parser.Eval();

			if (nType < -1 || nType > 2)
				nType = 0;

            DataAccessParser _accessParser(_sObject);

            if (!_accessParser.getDataObject().length() || !isValidIndexSet(_accessParser.getIndices()))
            {
                sLine = sLine.substr(0, n_pos) + "nan" + sLine.substr(nPos + 1);
				n_pos++;
				continue;
            }

			if (nType > -1)
			{
				if (nType == 2 && _accessParser.getIndices().row.isOpenEnd())
				{
					_accessParser.getIndices().row.setRange(_accessParser.getIndices().row.front(), _accessParser.getIndices().row.front() + 1);
				}
				else if (nType == 1 && _accessParser.getIndices().col.isOpenEnd())
				{
					_accessParser.getIndices().col.setRange(_accessParser.getIndices().col.front(), _accessParser.getIndices().col.front() + 1);
				}
				else if (!nType)
                {
                    if (_accessParser.isCluster())
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                            _accessParser.getIndices().row.setRange(0, _data.getCluster(_accessParser.getDataObject()).size()-1);

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.back() = VectorIndex::INVALID;
                    }
                    else if (_accessParser.getDataObject() == "string")
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                        {
                            if (_accessParser.getIndices().col.size() == 1)
                            {
                                if (_data.getStringElements(_accessParser.getIndices().col.front()))
                                    _accessParser.getIndices().row.setRange(0, _data.getStringElements(_accessParser.getIndices().col.front())-1);
                                else
                                    _accessParser.getIndices().row.setRange(0, 0);
                            }
                            else
                            {
                                if (_data.getStringElements())
                                    _accessParser.getIndices().row.setRange(0, _data.getStringElements()-1);
                                else
                                    _accessParser.getIndices().row.setRange(0, 0);
                            }
                        }

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.setRange(0, _data.getStringCols()-1);
                    }
                    else
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                            _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);
                    }
                }
			}

			_accessParser.getIndices().row.linearize();
			_accessParser.getIndices().col.linearize();

			vector<double> vIndices;
			vIndices.push_back(_accessParser.getIndices().row.front() + 1);
			vIndices.push_back(_accessParser.getIndices().row.last() + 1);
			vIndices.push_back(_accessParser.getIndices().col.front() + 1);
			vIndices.push_back(_accessParser.getIndices().col.last() + 1);
			_parser.SetVectorVar("_~indices[" + replaceToVectorname(_sObject) + "]", vIndices);
			sLine = sLine.substr(0, n_pos) + "_~indices[" + replaceToVectorname(_sObject) + "]" + sLine.substr(nPos + 1);
		}

		n_pos++;
	}

	n_pos = 0;
	// log = is_data(EXPR)
	while (sLine.find("is_data(", n_pos) != string::npos)
	{
		n_pos = sLine.find("is_data(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 7;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sData = sLine.substr(n_pos + 8, nPos - n_pos - 8);

			if (containsStrings(sData) || parser_containsStringVectorVars(sData, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				// use only first one
				sData = strRes.vResult[0];
			}

			if (sData[0] == '"')
				sData.erase(0, 1);

			if (sData[sData.length() - 1] == '"')
				sData.erase(sData.length() - 1);

			StripSpaces(sData);

			if (sData.substr(0, 5) == "data(" || _data.isTable(sData) || _data.isCluster(sData))
				sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos + 1);
			else
				sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos + 1);
		}

		n_pos++;
	}

	n_pos = 0;
	// log = is_table(EXPR)
	while (sLine.find("is_table(", n_pos) != string::npos)
	{
		n_pos = sLine.find("is_table(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 8;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sData = sLine.substr(n_pos + 9, nPos - n_pos - 9);

			if (containsStrings(sData) || parser_containsStringVectorVars(sData, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				// use only first one
				sData = strRes.vResult[0];
			}

			if (sData[0] == '"')
				sData.erase(0, 1);

			if (sData[sData.length() - 1] == '"')
				sData.erase(sData.length() - 1);

			StripSpaces(sData);

			if (sData.substr(0, 5) == "data(" || _data.isTable(sData))
				sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos + 1);
			else
				sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos + 1);
		}

		n_pos++;
	}

	n_pos = 0;
	// log = is_cluster(EXPR)
	while (sLine.find("is_cluster(", n_pos) != string::npos)
	{
		n_pos = sLine.find("is_cluster(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 10;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sData = sLine.substr(n_pos + 11, nPos - n_pos - 11);

			if (containsStrings(sData) || parser_containsStringVectorVars(sData, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				// use only first one
				sData = strRes.vResult[0];
			}

			if (sData[0] == '"')
				sData.erase(0, 1);

			if (sData[sData.length() - 1] == '"')
				sData.erase(sData.length() - 1);

			StripSpaces(sData);

			if (_data.isCluster(sData))
				sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos + 1);
			else
				sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos + 1);
		}

		n_pos++;
	}

	n_pos = 0;
	// {var} = findcolumn("data","header")
	while (sLine.find("findcolumn(", n_pos) != string::npos)
	{
		n_pos = sLine.find("findcolumn(", n_pos);
		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}
		unsigned int nPos = n_pos + 10;
		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
		nPos += nParensMatch;
		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sData = sLine.substr(n_pos + 11, nPos - n_pos - 11);
			string sHeadline;
			if (containsStrings(sData) || parser_containsStringVectorVars(sData, mStringVectorVars))
			{
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);
				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				// use only first one
				sData = strRes.vResult[0];
				sHeadline = strRes.vResult[1];
			}
			if (!sHeadline.length())
				throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
			if (sData[0] == '"')
				sData.erase(0, 1);
			if (sData[sData.length() - 1] == '"')
				sData.erase(sData.length() - 1);
			StripSpaces(sData);
			if (sHeadline.front() == '"')
				sHeadline.erase(0, 1);
			if (sHeadline.back() == '"')
				sHeadline.erase(sHeadline.length() - 1);
			StripSpaces(sHeadline);
			if (sData.substr(0, 5) == "data(" || _data.isTable(sData))
			{
				sData.erase(sData.find("("));
				string sResult;
				for (long long int i = 0; i < _data.getCols(sData, false); i++)
				{
					if (_data.getHeadLineElement(i, sData) == sHeadline)
					{
						if (sResult.length())
							sResult += ", ";
						sResult += toString(i + 1);
					}
				}
				if (!sResult.length())
					sResult = "nan";
				if (sResult.find(',') != string::npos)
					sResult = "{" + sResult + "}";
				sLine = sLine.substr(0, n_pos) + sResult + sLine.substr(nPos + 1);
			}
			else
				sLine = sLine.substr(0, n_pos) + "nan" + sLine.substr(nPos + 1);
		}
		n_pos++;
	}

	n_pos = 0;
	// str = valtostr(EXPR, [str])
	while (sLine.find("valtostr(", n_pos) != string::npos)
	{
		n_pos = sLine.find("valtostr(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 8;

		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sToString = sLine.substr(n_pos + 9, nPos - n_pos - 9);
			string sExpr = getNextArgument(sToString, true);
			string sChar = "";
			unsigned int nCount = 0;

			if (sToString.length())
			{
				sChar = getNextArgument(sToString, true);

				if (containsStrings(sChar) || _data.containsStringVars(sChar))
				{
					StringResult strRes = parser_StringParserCore(sChar, "", _data, _parser, _option, mStringVectorVars);

					if (!strRes.vResult.size())
						throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

					// use only first one
					sChar = removeQuotationMarks(strRes.vResult[0]);
				}

				string sCnt = getNextArgument(sToString, true);

				if (sCnt.length())
				{
					_parser.SetExpr(sCnt);
					nCount = (unsigned int)fabs(_parser.Eval());
				}
			}

			if (!containsStrings(sExpr) && !_data.containsStringVars(sExpr) && !parser_containsStringVectorVars(sExpr, mStringVectorVars))
			{
                // check for data sets in the evaluation of the `valtostr()` arguments
                if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
                    getDataElements(sExpr, _parser, _data, _option);

				int nResults = 0;
				value_type* v = 0;
				_parser.SetExpr(sExpr);
				v = _parser.Eval(nResults);
				vector<string> vToString;
                string sElement = "";

				for (int n = 0; n < nResults; n++)
				{
					if (fabs(rint(v[n]) - v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
						sElement = toString((long long int)rint(v[n]));
					else
						sElement = toString(v[n], _option);

					while (sElement.length() < nCount && sChar.length())
						sElement.insert(0, sChar);

                    vToString.push_back("\"" + sElement + "\"");
				}

				sToString = parser_CreateStringVectorVar(vToString, mStringVectorVars);
			}
			else
			{
				StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

				for (size_t i = 0; i < strRes.vResult.size(); i++)
				{
					while (strRes.vResult[i].length() < nCount && sChar.length())
						strRes.vResult[i].insert(0, sChar);

					// add quotation marks, if they are missing
					strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
				}

				sToString = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
			}

			sLine = sLine.substr(0, n_pos) + sToString + sLine.substr(nPos + 1);
		}
		n_pos++;
	}
	return sLine;
}

// This static function searches for an occurence of a known string
// function in the passed command line and passes the control to the
// string function handler
static string parser_ApplyStringFuncs(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
    // Store the string function map statically
	static map<string, StringFuncHandle> mFuncHandleTable = parser_getStringFuncHandles();

	for (auto iter = mFuncHandleTable.begin(); iter != mFuncHandleTable.end(); ++iter)
	{
	    // Found an occurence -> call the string function handler
		if (sLine.find(iter->first + "(") != string::npos)
			parser_StringFuncHandler(sLine, iter->first, _data, _parser, _option, mStringVectorVars, iter->second);
	}

	return sLine;
}

// This static function is used to construct the string function map
// it is only called once
static map<string, StringFuncHandle> parser_getStringFuncHandles()
{
	map<string, StringFuncHandle> mHandleTable;

	mHandleTable["strlen"]              = StringFuncHandle(STR, strfnc_strlen, false);
	mHandleTable["ascii"]               = StringFuncHandle(STR, strfnc_ascii, false);
	mHandleTable["getfileparts"]        = StringFuncHandle(STR, strfnc_getFileParts, false);
	mHandleTable["to_string"]           = StringFuncHandle(STR, strfnc_to_string, false);
	mHandleTable["to_uppercase"]        = StringFuncHandle(STR, strfnc_to_uppercase, false);
	mHandleTable["to_lowercase"]        = StringFuncHandle(STR, strfnc_to_lowercase, false);
	mHandleTable["getenvvar"]           = StringFuncHandle(STR, strfnc_getenvvar, false);
	mHandleTable["getmatchingparens"]   = StringFuncHandle(STR, strfnc_getmatchingparens, false);
	mHandleTable["findfile"]            = StringFuncHandle(STR_STROPT, strfnc_findfile, false);
	mHandleTable["split"]               = StringFuncHandle(STR_STR, strfnc_split, false);
	mHandleTable["to_time"]             = StringFuncHandle(STR_STR, strfnc_to_time, false);
	mHandleTable["strfnd"]              = StringFuncHandle(STR_STR_VALOPT, strfnc_strfnd, false);
	mHandleTable["strmatch"]            = StringFuncHandle(STR_STR_VALOPT, strfnc_strmatch, false);
	mHandleTable["str_not_match"]       = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_match, false);
	mHandleTable["strrfnd"]             = StringFuncHandle(STR_STR_VALOPT, strfnc_strrfnd, false);
	mHandleTable["strrmatch"]           = StringFuncHandle(STR_STR_VALOPT, strfnc_strrmatch, false);
	mHandleTable["str_not_rmatch"]      = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_rmatch, false);
	mHandleTable["findparam"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findparam, false);
	mHandleTable["substr"]              = StringFuncHandle(STR_VAL_VALOPT, strfnc_substr, false);
	mHandleTable["repeat"]              = StringFuncHandle(STR_VAL, strfnc_repeat, false);
	mHandleTable["timeformat"]          = StringFuncHandle(STR_VAL, strfnc_timeformat, false);
	mHandleTable["char"]                = StringFuncHandle(STR_VAL, strfnc_char, false);
	mHandleTable["getopt"]              = StringFuncHandle(STR_VAL, strfnc_getopt, false);
	mHandleTable["replace"]             = StringFuncHandle(STR_VAL_VALOPT_STROPT, strfnc_replace, false); // fehler
	mHandleTable["textparse"]           = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_textparse, false);
	mHandleTable["locate"]              = StringFuncHandle(STR_STR_VALOPT_VALOPT, strfnc_locate, true);
	mHandleTable["findtoken"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findtoken, false);
	mHandleTable["replaceall"]          = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_replaceall, false);
	mHandleTable["getfilelist"]         = StringFuncHandle(STR_VALOPT, strfnc_getfilelist, false);
	mHandleTable["getfolderlist"]       = StringFuncHandle(STR_VALOPT, strfnc_getfolderlist, false);
	mHandleTable["to_char"]             = StringFuncHandle(VAL, strfnc_to_char, true);
	mHandleTable["and"]                 = StringFuncHandle(VAL, strfnc_and, true);
	mHandleTable["or"]                  = StringFuncHandle(VAL, strfnc_or, true);
	mHandleTable["xor"]                 = StringFuncHandle(VAL, strfnc_xor, true);
	mHandleTable["num"]                 = StringFuncHandle(STR, strfnc_num, true);
	mHandleTable["cnt"]                 = StringFuncHandle(STR, strfnc_cnt, true);
	mHandleTable["min"]                 = StringFuncHandle(STR, strfnc_min, true);
	mHandleTable["max"]                 = StringFuncHandle(STR, strfnc_max, true);
	mHandleTable["sum"]                 = StringFuncHandle(STR, strfnc_sum, true);

	return mHandleTable;
}

// This static function removes the escape characters from the passed string
static string removeMaskedStrings(const string& sString)
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

// This static function adds escape characters into the passed string
static string addMaskedStrings(const string& sString)
{
	if (sString.find('"') == string::npos && sString.find(NEWSTRING) == string::npos && sString.back() != '\\')
		return sString;
	string sRet = sString;

	// Go through the complete string without the first
	// and the last character
	for (size_t i = 1; i < sRet.length() - 1; i++)
	{
	    // Escape backslashes
		if (sRet[i] == '\\' && sRet[i + 1] != '"' && sRet[i + 1] != ' ')
		{
			sRet.insert(i + 1, " ");
			i++;
		}

		// Escape quotation marks
		if (sRet[i] == '"' && sRet[i - 1] != '\\' && sRet[i + 1] != NEWSTRING && sRet.find('"', i + 1) != string::npos)
		{
			sRet.insert(i, "\\");
			i++;
		}

		// Replace the new string character with a comma
		if (sRet[i] == NEWSTRING)
		{
			sRet[i] = ',';
			if (sRet[i + 1] == '"')
				i++;
		}

		// Escape tab and newlines
		if (sRet[i] == '\t')
			sRet.replace(i, 1, "\\t");
		if (sRet[i] == '\n')
			sRet.replace(i, 1, "\\n");
	}
	return sRet;
}

// This static function simply removes the surrounding quotation marks
static string removeQuotationMarks(const string& sString)
{
	if (sString.find('"') == string::npos || sString.front() != '"' || sString.back() != '"')
		return sString;
	return sString.substr(1, sString.length() - 2);
}

// This static function simply adds the surrounding quotation marks
static string addQuotationMarks(const string& sString)
{
	if (sString.front() == '"' && sString.back() == '"')
		return sString;
	else
		return "\"" + sString + "\"";
}

// This static function masks the line break and the tabulator
// control characters for storing the results
static string maskControlCharacters(string sString)
{
    replaceAll(sString, "\n", "\\n");
    replaceAll(sString, "\t", "\\t");
    return sString;
}

// This static function handles all regular string functions
static void parser_StringFuncHandler(string& sLine, const string& sFuncName, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, StringFuncHandle funcHandle)
{
	size_t n_pos = 0;
	size_t nParensMatch = 0;

	// While the function signature can be found
	while (sLine.find(sFuncName + "(", n_pos) != string::npos)
	{
		n_pos = sLine.find(sFuncName + "(", n_pos);

		// Ignore false positives
		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		size_t nPos = n_pos + sFuncName.length();

		// If no matching parenthesis is found, throw an error
		if ((nParensMatch = getMatchingParenthesis(sLine.substr(nPos))) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += nParensMatch;

		// Extract the argument of the current found function and process it
		if (!n_pos || isDelimiter(sLine[n_pos-1]))
		{
			string sFunctionArgument = sLine.substr(n_pos + sFuncName.length() + 1, nPos - n_pos - sFuncName.length() - 1);
			vector<string> vReturnValues;
			StringFuncArgs stringArgs;
			stringArgs.opt = &_option;
			bool bLogicalOnly = false;

			// Apply the parser as specified by the function signature. After that call the corresponding
			// string function with the returned arguments as many times as it's needed
			if (funcHandle.fType >= PARSER_INT && funcHandle.fType < PARSER_STRING)
			{
				n_vect nIntArg;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, nIntArg);
				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}
				if (funcHandle.bTakesMultiArguments)
				{
					stringArgs.nMultiArg = nIntArg;
					vReturnValues.push_back(funcHandle.fHandle(stringArgs));
				}
				else
				{
					for (size_t i = 0; i < nMaxArgs; i++)
					{
						stringArgs.nArg1 = nIntArg[i];
						vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
					}
				}
			}
			else if (funcHandle.fType >= PARSER_STRING && funcHandle.fType < PARSER_STRING_INT_INT)
			{
				s_vect sStringArg;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg, bLogicalOnly);
				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}

				// These five multiargument functions are also defined for numerical values.
				// If the return value for the current functions arguments is an only logical
				// value, ignore the current function call
				if (bLogicalOnly
                    && (sFuncName == "min" || sFuncName == "max" || sFuncName == "cnt" || sFuncName == "num" || sFuncName == "sum"))
                {
                    n_pos++;
                    continue;
                }

				if (funcHandle.bTakesMultiArguments)
				{
					stringArgs.sMultiArg = sStringArg;
					vReturnValues.push_back(funcHandle.fHandle(stringArgs));
				}
				else
				{
					for (size_t i = 0; i < nMaxArgs; i++)
					{
						stringArgs.sArg1 = sStringArg[i];
						vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
					}
				}
			}
			else if (funcHandle.fType >= PARSER_STRING_INT_INT && funcHandle.fType < PARSER_STRING_INT_INT_STRING)
			{
				s_vect sStringArg;
				n_vect nIntArg1, nIntArg2;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg, nIntArg1, nIntArg2);
				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}
				for (size_t i = 0; i < nMaxArgs; i++)
				{
					if (i < sStringArg.size())
						stringArgs.sArg1 = sStringArg[i];
					else if (sStringArg.size() == 1)
						stringArgs.sArg1 = sStringArg[0];
					else
						stringArgs.sArg1 = "";
					if (i < nIntArg1.size())
						stringArgs.nArg1 = nIntArg1[i];
					else if (nIntArg1.size() == 1)
						stringArgs.nArg1 = nIntArg1[0];
					else
						stringArgs.nArg1 = DEFAULT_NUM_ARG;
					if (i < nIntArg2.size())
						stringArgs.nArg2 = nIntArg2[i];
					else if (nIntArg2.size() == 1)
						stringArgs.nArg2 = nIntArg2[0];
					else
						stringArgs.nArg2 = DEFAULT_NUM_ARG;

					vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
				}

			}
			else if (funcHandle.fType >= PARSER_STRING_INT_INT_STRING && funcHandle.fType < PARSER_STRING_STRING_INT_INT)
			{
				s_vect sStringArg1, sStringArg2;
				n_vect nIntArg1, nIntArg2;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg1, nIntArg1, nIntArg2, sStringArg2);
				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}
				for (size_t i = 0; i < nMaxArgs; i++)
				{
					if (i < sStringArg1.size())
						stringArgs.sArg1 = sStringArg1[i];
					else if (sStringArg1.size() == 1)
						stringArgs.sArg1 = sStringArg1[0];
					else
						stringArgs.sArg1 = "";
					if (i < sStringArg2.size())
						stringArgs.sArg2 = sStringArg2[i];
					else if (sStringArg2.size() == 1)
						stringArgs.sArg2 = sStringArg2[0];
					else
						stringArgs.sArg2 = "";
					if (i < nIntArg1.size())
						stringArgs.nArg1 = nIntArg1[i];
					else if (nIntArg1.size() == 1)
						stringArgs.nArg1 = nIntArg1[0];
					else
						stringArgs.nArg1 = DEFAULT_NUM_ARG;
					if (i < nIntArg2.size())
						stringArgs.nArg2 = nIntArg2[i];
					else if (nIntArg2.size() == 1)
						stringArgs.nArg2 = nIntArg2[0];
					else
						stringArgs.nArg2 = DEFAULT_NUM_ARG;

					vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
				}
			}
			else if (funcHandle.fType >= PARSER_STRING_STRING_INT_INT && funcHandle.fType < PARSER_STRING_STRING_STRING_INT_INT)
			{
				s_vect sStringArg1, sStringArg2;
				n_vect nIntArg1, nIntArg2;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg1, sStringArg2, nIntArg1, nIntArg2);

				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}

				if (funcHandle.bTakesMultiArguments)
				{
					stringArgs.sMultiArg = sStringArg1;
					nMaxArgs = max(max(sStringArg2.size(), nIntArg1.size()), nIntArg2.size());

					for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
				}
				else
				{
                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg1.size())
                            stringArgs.sArg1 = sStringArg1[i];
                        else if (sStringArg1.size() == 1)
                            stringArgs.sArg1 = sStringArg1[0];
                        else
                            stringArgs.sArg1 = "";

                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
				}
			}
			else if (funcHandle.fType >= PARSER_STRING_STRING_STRING_INT_INT)
			{
				s_vect sStringArg1, sStringArg2, sStringArg3;
				n_vect nIntArg1, nIntArg2;
				size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2);
				if (!nMaxArgs)
				{
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}
				for (size_t i = 0; i < nMaxArgs; i++)
				{
					if (i < sStringArg1.size())
						stringArgs.sArg1 = sStringArg1[i];
					else if (sStringArg1.size() == 1)
						stringArgs.sArg1 = sStringArg1[0];
					else
						stringArgs.sArg1 = "";
					if (i < sStringArg2.size())
						stringArgs.sArg2 = sStringArg2[i];
					else if (sStringArg2.size() == 1)
						stringArgs.sArg2 = sStringArg2[0];
					else
						stringArgs.sArg2 = "";
					if (i < sStringArg3.size())
						stringArgs.sArg3 = sStringArg3[i];
					else if (sStringArg3.size() == 1)
						stringArgs.sArg3 = sStringArg3[0];
					else
						stringArgs.sArg3 = "";
					if (i < nIntArg1.size())
						stringArgs.nArg1 = nIntArg1[i];
					else if (nIntArg1.size() == 1)
						stringArgs.nArg1 = nIntArg1[0];
					else
						stringArgs.nArg1 = DEFAULT_NUM_ARG;
					if (i < nIntArg2.size())
						stringArgs.nArg2 = nIntArg2[i];
					else if (nIntArg2.size() == 1)
						stringArgs.nArg2 = nIntArg2[0];
					else
						stringArgs.nArg2 = DEFAULT_NUM_ARG;

					vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
				}
			}

			// copy the return values to the final variable
			string sFuncReturnValue = "";

            // Expand the string vector component, if needed
			parser_ExpandStringVectorComponents(vReturnValues);

			// Create a string vector variable for the function output
			sFuncReturnValue = parser_CreateStringVectorVar(vReturnValues, mStringVectorVars);

			// replace the function with the return value
			sLine.replace(n_pos, nPos + 1 - n_pos, sFuncReturnValue);
		}

		n_pos++;
	}
}

// This static function is one of the string function argument
// parser. It is one of the two basic ones, which will be called
// by all others
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, n_vect& nArg)
{
	string sFuncArgument = __sFuncArgument;
	value_type* v = 0;
	int nReturn = 0;

	// If the current function argument contains strings,
	// parse it correspondingly
	if (containsStrings(sFuncArgument) || _data.containsStringVars(sFuncArgument) || parser_containsStringVectorVars(sFuncArgument, mStringVectorVars) || _data.containsClusters(sFuncArgument))
	{
	    // Call the string parser core
		StringResult strRes = parser_StringParserCore(sFuncArgument, "", _data, _parser, _option, mStringVectorVars);

		// If already numerical value are available, use them directly
		if (strRes.vNumericalValues.size())
        {
            for (size_t i = 0; i < strRes.vNumericalValues.size(); i++)
                nArg.push_back(intCast(strRes.vNumericalValues[i]));

            return nArg.size();
        }

		// Evaluate the returned strings numerically
		for (size_t i = 0; i < strRes.vResult.size(); i++)
		{
			_parser.SetExpr(strRes.vResult[i]);
			v = _parser.Eval(nReturn);

			for (int n = 0; n < nReturn; n++)
				nArg.push_back(intCast(v[n]));
		}

		return nArg.size();
	}
	else if (_data.containsTablesOrClusters(sFuncArgument))
    {
        getDataElements(sFuncArgument, _parser, _data, _option, false);
    }

	// Set the expression and evaluate it
	_parser.SetExpr(sFuncArgument);
	v = _parser.Eval(nReturn);

	for (int i = 0; i < nReturn; i++)
	{
		nArg.push_back(intCast(v[i]));
	}

	return (size_t)nReturn;
}

// This static function is one of the string function argument
// parser. It is one of the two basic ones, which will be called
// by all others
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg, bool& bLogicalOnly)
{
	string sFuncArgument = __sFuncArgument;

	// If the current function argument contains strings,
	// parse it correspondingly
	if (containsStrings(sFuncArgument) || _data.containsStringVars(sFuncArgument) || parser_containsStringVectorVars(sFuncArgument, mStringVectorVars) || _data.containsClusters(sFuncArgument))
	{
	    // Call the string parser core
		StringResult strRes = parser_StringParserCore(sFuncArgument, "", _data, _parser, _option, mStringVectorVars);

		// Use the returned values as function arguments
		for (size_t i = 0; i < strRes.vResult.size(); i++)
			sArg.push_back(removeQuotationMarks(strRes.vResult[i]));

        bLogicalOnly = strRes.bOnlyLogicals;
		return strRes.vResult.size();
	}
	else if (_data.containsTablesOrClusters(sFuncArgument))
    {
        getDataElements(sFuncArgument, _parser, _data, _option, false);
    }

	// Expand the passed argument, if needed and
	// distribute it to the components of the argument vector
	if (sFuncArgument.find('{') != string::npos || sFuncArgument.find(',') != string::npos)
	{
		parser_VectorToExpr(sFuncArgument, _option);

		// As long as the function argument has a length,
		// get the next argument and store it in the vector
		while (sFuncArgument.length())
			sArg.push_back(removeQuotationMarks(getNextArgument(sFuncArgument, true)));
	}
	else
		sArg.push_back(removeQuotationMarks(sFuncArgument));

    // Declare argument as numerical only
    bLogicalOnly = true;

	return sArg.size();
}

// This static function is one of the string function argument
// parser. It handles one string and two numerical arguments
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2)
{
	string sFuncArgument = __sFuncArgument;
	size_t nMaxLength = 0;
	bool bLogicalOnly = false;

	// Get the single arguments
	string sString = getNextArgument(sFuncArgument, true);
	string sNumVal1 = getNextArgument(sFuncArgument, true);
	string sNumVal2 = getNextArgument(sFuncArgument, true);

	// Handle the arguments using the basic functions
	// and store the highets number of return values
	nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString, mStringVectorVars, sArg1, bLogicalOnly);

	if (!nMaxLength)
		return 0;

	if (sNumVal1.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal2, mStringVectorVars, nArg2);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}

	return nMaxLength;
}

// This static function is one of the string function argument
// parser. It handles two string and two numerical arguments, where
// the second string is the last argument
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2, s_vect& sArg2)
{
	string sFuncArgument = __sFuncArgument;
	size_t nMaxLength = 0;
	bool bLogicalOnly = false;

	// Get the single arguments
	string sString1 = getNextArgument(sFuncArgument, true);
	string sNumVal1 = getNextArgument(sFuncArgument, true);
	string sNumVal2 = getNextArgument(sFuncArgument, true);
	string sString2 = getNextArgument(sFuncArgument, true);

	// Handle the arguments using the basic functions
	// and store the highets number of return values
	nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1, bLogicalOnly);

	if (!nMaxLength)
		return 0;

	if (sNumVal1.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal2, mStringVectorVars, nArg2);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sString2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2, bLogicalOnly);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}

	return nMaxLength;
}

// This static function is one of the string function argument
// parser. It handles two string and two numerical arguments
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, s_vect& sArg2, n_vect& nArg1, n_vect& nArg2)
{
	string sFuncArgument = __sFuncArgument;
	size_t nMaxLength = 0;
	bool bLogicalOnly = false;

	// Get the single arguments
	string sString1 = getNextArgument(sFuncArgument, true);
	string sString2 = getNextArgument(sFuncArgument, true);
	string sNumVal1 = getNextArgument(sFuncArgument, true);
	string sNumVal2 = getNextArgument(sFuncArgument, true);

	// Handle the arguments using the basic functions
	// and store the highets number of return values
	nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1, bLogicalOnly);

	if (!nMaxLength)
		return 0;

	if (sString2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2, bLogicalOnly);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal1.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal2, mStringVectorVars, nArg2);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}

	return nMaxLength;
}

// This static function is one of the string function argument
// parser. It handles three string and two numerical arguments
static size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, s_vect& sArg2, s_vect& sArg3, n_vect& nArg1, n_vect& nArg2)
{
	string sFuncArgument = __sFuncArgument;
	size_t nMaxLength = 0;
	bool bLogicalOnly = false;

	// Get the single arguments
	string sString1 = getNextArgument(sFuncArgument, true);
	string sString2 = getNextArgument(sFuncArgument, true);
	string sString3 = getNextArgument(sFuncArgument, true);
	string sNumVal1 = getNextArgument(sFuncArgument, true);
	string sNumVal2 = getNextArgument(sFuncArgument, true);

	// Handle the arguments using the basic functions
	// and store the highets number of return values
	nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1, bLogicalOnly);

	if (!nMaxLength)
		return 0;

	if (sString2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2, bLogicalOnly);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sString3.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString3, mStringVectorVars, sArg3, bLogicalOnly);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal1.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}
	else
        return nMaxLength;

	if (sNumVal2.length())
	{
		size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal2, mStringVectorVars, nArg2);

		if (!nReturn)
			return 0;

		if (nMaxLength < nReturn)
			nMaxLength = nReturn;
	}

	return nMaxLength;
}

// This static function returns the content of the data objects and
// expands them as a list
static string parser_GetDataForString(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, size_t n_pos)
{
	// Get the contents of "string()", "data()" and the other caches
	string sDummy;

	// {str} = string(...)
	while (sLine.find("string(", n_pos) != string::npos)
	{
		n_pos = sLine.find("string(", n_pos);

		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}

		unsigned int nPos = n_pos + 6;

		if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

		nPos += getMatchingParenthesis(sLine.substr(nPos));

		if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, 8))))
		{
			string sString = "";

			if (nPos - n_pos - 7 > 0)
			{
				sString = sLine.substr(n_pos + 7, nPos - n_pos - 7);
				StripSpaces(sString);
			}

            StringResult strRes = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars, false);

            if (!strRes.vResult.size())
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            sString = strRes.vResult[0];

            Indices _idx = parser_getIndices("string(" + sString + ")", _parser, _data, _option);

            if (_idx.col.isOpenEnd())
                _idx.col.back() = VectorIndex::INVALID;

            if (_idx.row.isOpenEnd())
                _idx.row.setRange(0, _data.getStringElements()-1);

            if (!_idx.col.isValid())
                _idx.col.front() = 0;

            if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
            {
                string sLeft = sLine.substr(0, n_pos);
                StripSpaces(sLeft);

                if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "num(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + toString(_idx.row.size()) + sLine.substr(sLine.find(')', nPos + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "max(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.maxString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "min(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.minString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "sum(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.sumString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
            }
            else
            {
                if (_data.getStringElements(_idx.col.front()))
                {
                    sString = "";
                    vector<string> vStrings;

                    for (size_t i = 0; i < _idx.row.size(); i++)
                    {
                        vStrings.push_back("\"" +  _data.readString((unsigned int)_idx.row[i], _idx.col.front()) + "\"");
                    }

                    sString = parser_CreateStringVectorVar(vStrings, mStringVectorVars);
                    sLine = sLine.substr(0, n_pos) + sString + sLine.substr(nPos + 1);
                }
                else
                    sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nPos + 1);
            }
		}

		n_pos++;
	}

	n_pos = 0;
	// val = data(...)
	while (sLine.find("data(", n_pos) != string::npos)
	{
		n_pos = sLine.find("data(", n_pos);
		if (isInQuotes(sLine, n_pos, true))
		{
			n_pos++;
			continue;
		}
		unsigned int nPos = n_pos + 4;
		if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
			throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
		nPos += getMatchingParenthesis(sLine.substr(nPos));
		if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, 6))))
		{
			if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
			{
				if (n_pos > 4 && sLine.substr(sLine.rfind('(', n_pos) - 4, 5) == "norm(")
					n_pos -= 5;
				else
					n_pos -= 4;
				nPos++;
			}
			string sData = sLine.substr(n_pos, nPos - n_pos + 1);
			// Get the data and parse string expressions
			getDataElements(sData, _parser, _data, _option);
			StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

			if (!strRes.vResult.size())
				throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // Create a string vector variable from the returned vector
            // if it is a string. Otherwise simple use the first return
            // value, which already contains a numerical vector
            if (!strRes.bOnlyLogicals)
                sData = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
            else
                sData = strRes.vResult.front();

			sLine = sLine.substr(0, n_pos) + sData + sLine.substr(nPos + 1);
		}
		n_pos++;
	}

	for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
	{
		n_pos = 0;

		while (sLine.find(iter->first + "(", n_pos) != string::npos)
		{
			n_pos = sLine.find(iter->first + "(", n_pos);

			if (isInQuotes(sLine, n_pos, true))
			{
				n_pos++;
				continue;
			}

			unsigned int nPos = n_pos + (iter->first).length();

			if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
				throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

			nPos += getMatchingParenthesis(sLine.substr(nPos));

			if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, (iter->first).length() + 2))))
			{
				if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
				{
					if (n_pos > 4 && sLine.substr(sLine.rfind('(', n_pos) - 4, 5) == "norm(")
						n_pos -= 5;
					else
						n_pos -= 4;

					nPos++;
				}

				string sData = sLine.substr(n_pos, nPos - n_pos + 1);
				// Get the data and parse string expressions
				getDataElements(sData, _parser, _data, _option);
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Create a string vector variable from the returned vector
                // if it is a string. Otherwise simple use the first return
                // value, which already contains a numerical vector
                if (!strRes.bOnlyLogicals)
                    sData = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
                else
                    sData = strRes.vResult.front();

				sLine = sLine.substr(0, n_pos) + sData + sLine.substr(nPos + 1);
			}
		}
	}

	for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
	{
		n_pos = 0;

		while (sLine.find(iter->first + "{", n_pos) != string::npos)
		{
			n_pos = sLine.find(iter->first + "{", n_pos);

			if (isInQuotes(sLine, n_pos, true))
			{
				n_pos++;
				continue;
			}

			unsigned int nPos = n_pos + (iter->first).length();

			if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
				throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

			nPos += getMatchingParenthesis(sLine.substr(nPos));

			if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, (iter->first).length() + 2))))
			{
				if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
				{
					if (n_pos > 4 && sLine.substr(sLine.rfind('(', n_pos) - 4, 5) == "norm(")
						n_pos -= 5;
					else
						n_pos -= 4;

					nPos++;
				}

				string sData = sLine.substr(n_pos, nPos - n_pos + 1);
				// Get the data and parse string expressions
				getDataElements(sData, _parser, _data, _option);
				StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);

				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Create a string vector variable from the returned vector
                // if it is a string. Otherwise simple use the first return
                // value, which already contains a numerical vector
                if (!strRes.bOnlyLogicals)
                    sData = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
                else
                    sData = strRes.vResult.front();

				sLine = sLine.substr(0, n_pos) + sData + sLine.substr(nPos + 1);
			}
		}
	}

	return sLine;
}

// This static function handles the value to string parser ("#")
static string parser_NumToString(const string& sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
    // Do nothing, if there's no #
	if (sLine.find('#') == string::npos)
		return sLine + " ";

    // Create a copy of the current line
	string sLineToParsed = sLine + " ";
	string sLineToParsedTemp;

	unsigned int nPos = 0;
	unsigned int n_pos = 0;

	// As long as there are further "#"
	while (sLineToParsed.find('#', nPos) != string::npos)
	{
	    // Store its position
		nPos = sLineToParsed.find('#', nPos);

		// Ensure that it is not in a string
		if (!isInQuotes(sLineToParsed, nPos, true))
		{
			string sPrefix = "";
			sLineToParsedTemp += sLineToParsed.substr(0, nPos);
			sLineToParsed = sLineToParsed.substr(nPos + 1);

			// If the variable starts with a tilde, it's possible that we
			// need to prepend zeros. Get the prefix here
			if (sLineToParsed[0] == '~')
			{
				for (unsigned int i = 0; i < sLineToParsed.length(); i++)
				{
					if (sLineToParsed[i] != '~')
					{
						sPrefix = sLineToParsed.substr(0, i);
						sLineToParsed = sLineToParsed.substr(i);
						break;
					}
				}
			}

			// Jump over simple signs
			if (sLineToParsed[0] == '-' || sLineToParsed[0] == '+')
				n_pos = 1;
			else
				n_pos = 0;

            // Handle enclosing characters: parentheses, quotation marks
            // angle brackets, ...
			if (sLineToParsed[0] == '(' || sLineToParsed[0] == '{')
			{
			    // Get the contents of the current parenthesis
				string sExpr = sLineToParsed.substr(1, getMatchingParenthesis(sLineToParsed) - 1);

				// Does it contain strings?
				if (containsStrings(sExpr) || parser_containsStringVectorVars(sExpr, mStringVectorVars))
				{
					StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);
					if (!strRes.vResult.size())
						throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                    // Examine, whether the return value is logical only
                    if (!strRes.bOnlyLogicals)
                    {
                        // This case contains strings. We will cast the whole
                        // result into a string
                        for (size_t i = 0; i < strRes.vResult.size(); i++)
                        {
                            strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                        }

                        // Create a vector variable from the return value
                        sExpr = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
                        sLineToParsedTemp += sExpr;

                        // Get the next part of the command line
                        if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                            sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
                        else
                            sLineToParsed.clear();

                        nPos = 0;
                        continue;
                    }

                    // This code section is reached, if the return value
                    // is logical only. We will construct a multi-value
                    // expression and evaluate it numerically
                    sExpr.clear();

                    // Construct the expression and remove the trailing comma
                    for (size_t i = 0; i < strRes.vResult.size(); i++)
                    {
                        sExpr += strRes.vResult[i] + ",";
                    }
                    sExpr.pop_back();

				}

				// Set the expression
				_parser.SetExpr(sExpr);
			}
			else if (sLineToParsed[0] == '"')
			{
			    // Get the contents of the current string
				string sExpr = sLineToParsed.substr(1, sLineToParsed.find('"', 1) - 1);

				// Add the zeros, if needed
				while (sExpr.length() < sPrefix.length() + 2)
					sExpr.insert(0, 1, '0');

                // Store the result and continue
				sLineToParsedTemp += "\"" + sExpr + "\"";
				if (sLineToParsed.find('"', 1) < sLineToParsed.length() - 1)
					sLineToParsed = sLineToParsed.substr(sLineToParsed.find('"', 1) + 1);
				else
					sLineToParsed.clear();
				nPos = 0;
				continue;
			}
			else if (sLineToParsed[0] == '<')
			{
			    // Does the current object contain a path token?
				if (sLineToParsed.find("<>") == 0
						|| sLineToParsed.find("<this>") == 0
						|| sLineToParsed.find("<wp>") == 0
						|| sLineToParsed.find("<loadpath>") == 0
						|| sLineToParsed.find("<savepath>") == 0
						|| sLineToParsed.find("<plotpath>") == 0
						|| sLineToParsed.find("<procpath>") == 0
						|| sLineToParsed.find("<scriptpath>") == 0)
				{
				    // Replace the path tokens
					if (sLineToParsed.find("<>") == 0 || sLineToParsed.find("<this>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getExePath()) + "\"";
					else if (sLineToParsed.find("<wp>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getWorkPath()) + "\"";
					else if (sLineToParsed.find("<loadpath>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getLoadPath()) + "\"";
					else if (sLineToParsed.find("<savepath>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getSavePath()) + "\"";
					else if (sLineToParsed.find("<plotpath>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getPlotOutputPath()) + "\"";
					else if (sLineToParsed.find("<procpath>") == 0)
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getProcsPath()) + "\"";
					else
						sLineToParsedTemp += "\"" + replacePathSeparator(_option.getScriptPath()) + "\"";
					sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>') + 1);
				}
				else if (sLineToParsed.find('>') != string::npos)
				{
				    // If no path token, only use the part in between of the angle brackets
					sLineToParsedTemp += "\"" + sLineToParsed.substr(1, sLineToParsed.find('>') - 1) + "\"";
					sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>') + 1);
				}
				else
				{
				    // Throw an error
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
				}
				nPos = 0;
				continue;
			}
			else if (parser_containsStringVectorVars(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos))), mStringVectorVars))
			{
			    // Here are string vector variables
				string sExpr = sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos)));

				// Parse the current line
				StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);

				// Ensure that the result exists
				if (!strRes.vResult.size())
					throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Add the needed quotation marks
				for (size_t i = 0; i < strRes.vResult.size(); i++)
				{
					strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
				}

				// Create a new string vector var, append it to the string and continue
				sExpr = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
				sLineToParsedTemp += sExpr;
				if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
					sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
				else
					sLineToParsed.clear();

				continue;
			}
			else // Set the expression
				_parser.SetExpr(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos))));

			int nResults = 0;
			value_type* v = 0;
			vector<string> vResults;
			string sElement = "";

			// Evaluate the parsed expression
			v = _parser.Eval(nResults);

			// Convert all results into strings
			for (int n = 0; n < nResults; n++)
			{
				if (fabs(rint(v[n]) - v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
					sElement = toString((long long int)rint(v[n]));
				else
					sElement = toString(v[n], _option);
				while (sElement.length() < sPrefix.length() + 1)
					sElement.insert(0, 1, '0');
				vResults.push_back(addQuotationMarks(sElement));
			}

			// Create a string vector var from the results and append it to
			// the current line
			sElement = parser_CreateStringVectorVar(vResults, mStringVectorVars);
			sLineToParsedTemp += sElement;

			// Search for the next delimiter
			if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
				sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
			else
				sLineToParsed.clear();

			nPos = 0;
		}
		else
			nPos++;
	}

	// Append the remaining parts of the current expression
	if (sLineToParsed.length() && sLineToParsedTemp.length())
		sLineToParsedTemp += sLineToParsed;

    // Handle remaining vector braces
	if (sLineToParsedTemp.find('{') != string::npos)
	{
		parser_VectorToExpr(sLineToParsedTemp, _option);
	}

	// Determine the return value
	if (sLineToParsedTemp.length())
		sLineToParsed = sLineToParsedTemp;
	else
		sLineToParsed = sLine;

	return sLineToParsed;
}

// This static function applies some elementary string operations
// like concatenation
static vector<bool> parser_ApplyElementaryStringOperations(vector<string>& vFinal, Parser& _parser, const Settings& _option, bool& bReturningLogicals)
{
	vector<bool> vIsNoStringValue;

	// Examine the whole passed vector
	for (unsigned int n = 0; n < vFinal.size(); n++)
	{
	    // Strip whitespaces and ensure that the
	    // current component is not empy
		StripSpaces(vFinal[n]);
		if (!vFinal[n].length())
			continue;

        // If there are logical expressions in the current
        // component, handle them here
		if (parser_detectStringLogicals(vFinal[n]))
		{
			vFinal[n] = parser_evalStringLogic(vFinal[n], _parser, bReturningLogicals);
			StripSpaces(vFinal[n]);
		}

		// Concatenate the strings
		if (vFinal[n].front() == '"' || vFinal[n].back() == '"')
		{
			vFinal[n] = parser_concatenateStrings(vFinal[n]);
		}

		// Determine, whether the current component is a string
		// or a numerical expression
		if (vFinal[n].front() != '"' && vFinal[n].back() != '"')
		{
		    // Try to evaluate the numerical expression, parse it as
		    // string and store it correspondingly
		    try
		    {
//                _parser.SetExpr(vFinal[n]);
//                if (vFinal[n].find('{') == string::npos)
//                    vFinal[n] = toCmdString(_parser.Eval());
                vIsNoStringValue.push_back(true);
            }
            catch (...)
            {
                // The parser was not able to parse the expression,
                // we keep it as a string
                vIsNoStringValue.push_back(false);
            }
		}
		else
			vIsNoStringValue.push_back(false);
	}

	// check, whether there's a string left
	bReturningLogicals = true;
	for (size_t i = 0; i < vIsNoStringValue.size(); i++)
	{
		if (!vIsNoStringValue[i])
		{
			bReturningLogicals = false;
			break;
		}
	}
	return vIsNoStringValue;
}

// This static function is a helper for parser_StoreStringResults()
// It will store the strings into the data tables
static void parser_StoreStringToDataObjects(const vector<string>& vFinal, string& sObject, size_t& nCurrentComponent, size_t nStrings, Parser& _parser, Datafile& _data, const Settings& _option)
{
    // Identify the correct table
    DataAccessParser _accessParser(sObject);

    if (!_accessParser.getDataObject().length())
        return;

    string sTableName = _accessParser.getDataObject();
    Indices& _idx = _accessParser.getIndices();

    // Is the target a headline or is it a regular index?
    if (_idx.row.isString())
    {
        // Write the string to the correct data table
        if (_idx.col.isOpenEnd() && sTableName == "data")
            _idx.col.setRange(0, _data.getCols("data")-1);

        // If the first element is non-zero but the second is,
        // we use the number of elements as upper boundary
        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _idx.col.front() + nStrings - nCurrentComponent - 1);

        for (int n = nCurrentComponent; n < (int)nStrings; n++)
        {
            if (!vFinal[n].length()
                || (_idx.col[n] == VectorIndex::INVALID)
                || (sTableName == "data" && _idx.col[n] >= _data.getCols("data")))
                break;

            _data.setHeadLineElement(_idx.col[n], sTableName, removeQuotationMarks(maskControlCharacters(vFinal[n])));
        }

        nCurrentComponent = nStrings;
    }
    else if (_accessParser.isCluster())
    {
        // Write the return values to the cluster with
        // parsing them
        value_type* v = nullptr;
        int nResults = 0;
        int nthComponent = 0;
        NumeRe::Cluster& cluster = _data.getCluster(sTableName);

        // Clusters are overwritten, if the last index
        // is not explictly set
        if (_idx.row.back() == -2 && _idx.row.front() == 0)
            cluster.clear();

        for (size_t i = nCurrentComponent; i < vFinal.size(); i++)
        {
            // Set expression and evaluate it (not efficient but currently necessary)
            if (vFinal[i].front() == '"')
            {
                // Special case: only one single value
                if (_idx.row.size() == 1 && _idx.col.size() == 1)
                {
                    cluster.setString(_idx.row.front(), vFinal[i]);
                    break;
                }

                if (_idx.row[nthComponent] == VectorIndex::INVALID)
                    break;

                cluster.setString(_idx.row[nthComponent], vFinal[i]);
                nthComponent++;
            }
            else
            {
                _parser.SetExpr(vFinal[i]);
                v = _parser.Eval(nResults);

                // Special case: only one single value
                if (_idx.row.size() == 1 && _idx.col.size() == 1)
                {
                    cluster.setDouble(_idx.row.front(), v[0]);

                    break;
                }

                // Write the single values
                for (int j = 0; j < nResults; j++)
                {
                    if (_idx.row[nthComponent] == VectorIndex::INVALID)
                        break;

                    cluster.setDouble(_idx.row[nthComponent], v[j]);

                    nthComponent++;
                }
            }
        }

        nCurrentComponent = nStrings;
    }
    else
    {
        // Find the correct table
        if (sTableName == "data")
            throw SyntaxError(SyntaxError::READ_ONLY_DATA, sObject, SyntaxError::invalid_position);

        // Write the return values to the data table with
        // parsing them
        value_type* v = nullptr;
        int nResults = 0;
        int nthComponent = 0;

        for (size_t i = nCurrentComponent; i < vFinal.size(); i++)
        {
            // Set expression and evaluate it (not efficient but currently necessary)
            _parser.SetExpr(vFinal[i]);
            v = _parser.Eval(nResults);

            // Special case: only one single value
            if (_idx.row.size() == 1 && _idx.col.size() == 1)
            {
                _data.writeToTable(_idx.row.front(), _idx.col.front(), sTableName, v[0]);

                break;
            }

            // Write the single values
            for (int j = 0; j < nResults; j++)
            {
                if (_idx.row.size() == 1 && _idx.col.size() > 1)
                {
                    if (_idx.col[nthComponent] == VectorIndex::INVALID)
                        break;

                    _data.writeToTable(_idx.row.front(), _idx.col[nthComponent], sTableName, v[j]);
                }
                else if (_idx.row.size() > 1 && _idx.col.size() == 1)
                {
                    if (_idx.row[nthComponent] == VectorIndex::INVALID)
                        break;

                    _data.writeToTable(_idx.row[nthComponent], _idx.col.front(), sTableName, v[j]);
                }

                nthComponent++;
            }
        }

        nCurrentComponent = nStrings;
    }

    _data.setCacheStatus(false);
}

// This static function is a helper for parser_StoreStringResults()
// It will store the strings into the string object
static void parser_StoreStringToStringObject(const vector<string>& vFinal, string& sObject, size_t& nCurrentComponent, size_t nStrings, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _idx = parser_getIndices(sObject, _parser, _data, _option);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _idx.row.front()+nStrings-nCurrentComponent-1);

    if (!_idx.col.isValid())
        _idx.col.front() = 0;

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        if (nCurrentComponent == nStrings)
            return;

        _data.writeString(removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])), _idx.row[i], _idx.col.front());
        nCurrentComponent++;
    }
}

// This static function stores the calculated string results
// in their desired targets.
static int parser_StoreStringResults(vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string __sObject, Datafile& _data, Parser& _parser, const Settings& _option)
{
    // Only do something, if the target object is not empty
	if (!__sObject.length())
        return 1;

    // Handle remaining vector braces
    if (__sObject.find('{') != string::npos)
        parser_VectorToExpr(__sObject, _option);

    string sObject;
    size_t nStrings = vFinal.size();
    size_t nCurrentComponent = 0;

    // As long as the target object is not empty
    while (__sObject.length())
    {
        // Get-cut the next target
        sObject = getNextArgument(__sObject, true);

        // Examine the current target
        if (sObject.find("data(") != string::npos || _data.containsTablesOrClusters(sObject))
        {
            // Store the strings into the data object
            parser_StoreStringToDataObjects(vFinal, sObject, nCurrentComponent, nStrings, _parser, _data, _option);
        }
        else if (sObject.find("string(") != string::npos)
        {
            // Store the strings into the string object
            parser_StoreStringToStringObject(vFinal, sObject, nCurrentComponent, nStrings, _parser, _data, _option);
        }
        else if (_data.containsStringVars(sObject))
        {
            // Store the strings in string variables
            StripSpaces(sObject);
            if (sObject.find(' ') != string::npos)
                return 0;
            try
            {
                if (nCurrentComponent >= nStrings)
                    _data.setStringValue(sObject, "");
                else
                    _data.setStringValue(sObject, removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])));
                nCurrentComponent++;
            }
            catch (...)
            {
                throw;
            }
        }
        else
        {
            // Store the results as numerical values
            StripSpaces(sObject);
            if (sObject.find(' ') != string::npos)
            {
                return 0;
            }

            // Search for the adress of the current variable
            if (parser_GetVarAdress(sObject, _parser))
            {
                if (vIsNoStringValue.size() > nCurrentComponent && !vIsNoStringValue[nCurrentComponent])
                {
                    return 0;
                }
            }

            // If this is a numerical value
            if (vIsNoStringValue.size() > nCurrentComponent && vIsNoStringValue[nCurrentComponent])
            {
                try
                {
                    // Parse and store it
                    int nResults = 0;
                    value_type* v = 0;
                    _parser.SetExpr(sObject + " = " + vFinal[nCurrentComponent]);
                    v = _parser.Eval(nResults);

                    // Replace the evaluated expression with its result,
                    // which will be shown in the terminal. The actual precision
                    // remains untouched!
                    if (nResults)
                    {
                        vAns = v[0];
                        string sValues;

                        // Transform the results into a string
                        for (int n = 0; n < nResults; n++)
                        {
                            sValues += toString(v[n], _option) + ",";
                        }
                        sValues.pop_back();

                        // replace the current expression
                        vFinal[nCurrentComponent] = sValues;
                    }

                    nCurrentComponent++;
                }
                catch (...)
                {
                    throw;
                }
            }
            else if (vIsNoStringValue.size() <= nCurrentComponent)
            {
                // Fallback: try to find the variable address
                // although it doesn't seem to be a numerical value
                if (!parser_GetVarAdress(sObject, _parser))
                {
                    try
                    {
                        // If there's no numerical value, create
                        // a new string variable
                        _data.setStringValue(sObject, "");
                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
            }
            else
            {
                try
                {
                    // Create a new string variable
                    _data.setStringValue(sObject, removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])));
                    nCurrentComponent++;
                }
                catch (...)
                {
                    throw;
                }
            }
        }
    }

	return 1;
}

// This static function converts the string parser results
// into an output string for the console
static string parser_CreateStringOutput(Parser& _parser, StringResult& StrRes, string& sLine, int parserFlags)
{
	sLine.clear();

	vector<string>& vFinal = StrRes.vResult;
	vector<bool>& vIsNoStringValue = StrRes.vNoStringVal;

	// remove the quotation marks
	for (size_t i = 0; i < vFinal.size(); i++)
		vFinal[i] = removeQuotationMarks(vFinal[i]);

	string sConsoleOut = "|-> ";
	static const string sConsoleIndent = "|   ";
	bool bLineBreaks = false;

	// Every result in the current return values
	// is a single string result
	for (size_t j = 0; j < vFinal.size(); j++)
	{
	    // In this case, no conversions are done
	    // the results are simply put together and returned
		if (parserFlags & KEEP_MASKED_CONTROL_CHARS && parserFlags & KEEP_MASKED_QUOTES)
		{
			if (!(parserFlags & NO_QUOTES) && !vIsNoStringValue[j])
				sLine += "\"" + vFinal[j] + "\"";
			else
            {
                sLine += vFinal[j];
            }

			if (j < vFinal.size() - 1)
				sLine += ",";
			continue;
		}

		// Start the current string value with a quotation mark
		// if it is not a special case
		if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !(parserFlags & NO_QUOTES) && !StrRes.bOnlyLogicals && !vIsNoStringValue[j])
		{
			sConsoleOut += "\"";
			sLine += "\"";
		}

		if (!vIsNoStringValue[j])
        {
            // Go through the current string value
            for (size_t k = 0; k < vFinal[j].length(); k++)
            {
                // If there are escaped control characters,
                // Replace them with their actual value here
                if (k + 1 < vFinal[j].length()
                        && vFinal[j][k] == '\\'
                        && (vFinal[j][k + 1] == 'n' || vFinal[j][k + 1] == 't' || vFinal[j][k + 1] == '"' || vFinal[j][k + 1] == ' ')
                        && !(vFinal[j].substr(k + 1, 3) == "tau"
                             && ((checkDelimiter(vFinal[j].substr(k, 5)) && vFinal[j].length() >= k + 5) || (vFinal[j].length() == k + 4)))
                        && !(vFinal[j].substr(k + 1, 5) == "theta"
                             && ((checkDelimiter(vFinal[j].substr(k, 7)) && vFinal[j].length() >= k + 7) || (vFinal[j].length() == k + 6)))
                        && !(vFinal[j].substr(k + 1, 2) == "nu"
                             && ((checkDelimiter(vFinal[j].substr(k, 4)) && vFinal[j].length() >= k + 4) || (vFinal[j].length() == k + 3)))
                        && !(vFinal[j].substr(k + 1, 3) == "neq"
                             && ((checkDelimiter(vFinal[j].substr(k, 5)) && vFinal[j].length() >= k + 5) || (vFinal[j].length() == k + 4)))
                   )
                {
                    //\not\neq\ni
                    if (vFinal[j][k + 1] == 'n') // Line break
                    {
                        sConsoleOut += "\n";
                        sLine += "\n";
                        bLineBreaks = true;
                    }
                    else if (vFinal[j][k + 1] == 't') // tabulator
                    {
                        sConsoleOut += "\t";
                        sLine += "\t";
                    }
                    else if (vFinal[j][k + 1] == '"') // quotation mark
                    {
                        sConsoleOut += "\"";
                        if (!(parserFlags & KEEP_MASKED_QUOTES))
                            sLine += "\"";
                        else
                            sLine += "\\\"";
                    }
                    else if (vFinal[j][k + 1] == ' ') // backslash itself
                    {
                        sConsoleOut += "\\";
                        if (!(parserFlags & KEEP_MASKED_CONTROL_CHARS))
                            sLine += "\\";
                        else
                            sLine += "\\ ";
                    }
                    k += 1;
                }
                else
                {
                    // Otherwise simply append the current character
                    if (vFinal[j][k] == '\n')
                        bLineBreaks = true;
                    sConsoleOut += vFinal[j][k];
                    sLine += vFinal[j][k];
                }
            }
        }
        else
        {
            _parser.SetExpr(vFinal[j]);
            int nResults = 0;
            value_type* v = _parser.Eval(nResults);
            string stres;

            for (int k = 0; k < nResults-1; k++)
            {
                stres = toCmdString(v[k]);
                sLine += stres + ", ";
                sConsoleOut += stres + ", ";
            }

            stres = toCmdString(v[nResults-1]);
            sLine += stres;
            sConsoleOut += stres;
        }
		// End the current string value with a quotation mark
		// if it is not a special case
		if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !(parserFlags & NO_QUOTES) && !StrRes.bOnlyLogicals && !vIsNoStringValue[j])
		{
			sConsoleOut += "\"";
			sLine += "\"";
		}

		// Break the loop, if this was the last string value
		if (j + 1 == vFinal.size())
			break;

        // Add a comma, if neither the next nor the current string
        // is a special case
		if (vFinal[j] != "\\n" && vFinal[j + 1] != "\\n" && vFinal[j] != "\\t" && vFinal[j + 1] != "\\t")
		{
		    // If the last character was a line break
			if (sLine.find_last_not_of("\" ") != string::npos && sLine[sLine.find_last_not_of("\" ")] == '\n')
			{
				sLine += ", ";
				if (sConsoleOut.back() == '"')
				{
					sConsoleOut[sConsoleOut.rfind('\n')] = '"';
					sConsoleOut.back() = '\n';
					bLineBreaks = true;
				}
				continue;
			}

			// Append the commas
			sConsoleOut += ", ";
			sLine += ", ";
		}
	}

	// Replace all line break characters with the console
	// indentation, if that is needed
	if (bLineBreaks)
        replaceAll(sConsoleOut, "\n", "\n" + sConsoleIndent);

	return sConsoleOut;
}

// This static function is used to detect logical expressions
static bool parser_detectStringLogicals(const string& sString)
{
	if (!sString.length())
		return false;

	int nQuotes = 0;
	for (size_t i = 0; i < sString.length(); i++)
	{
	    // Count quotation marks
		if (sString[i] == '"' && (!i || sString[i - 1] != '\\'))
		{
			nQuotes++;
			continue;
		}

		// Detect path tokens
		if (sString[i] == '<')
		{
		    // Search for path tokens
			size_t nAdv = parser_detectPathTokens(sString, i);
			if (nAdv)
			{
				i += nAdv;
				continue;
			}
		}

		// Logicals are only possible outside of string literals
		if (!(nQuotes % 2))
		{
			if (sString.substr(i, 2) == "&&"
					|| sString.substr(i, 2) == "||"
					|| sString.substr(i, 2) == "<="
					|| sString.substr(i, 2) == ">="
					|| sString.substr(i, 2) == "!="
					|| sString.substr(i, 2) == "=="
					|| sString[i] == '?'
					|| sString[i] == '<'
					|| sString[i] == '>')
				return true;
		}
	}
	return false;
}

// This static function is a helper for parser_detectStringLogicals,
// it will identify path tokens, which will start at the passed
// position
static size_t parser_detectPathTokens(const string& sString, size_t nPos)
{
	if (sString.substr(nPos, 2) == "<>")
		return 1u;
	if (sString.substr(nPos, 4) == "<wp>")
		return 3u;
	if (sString.substr(nPos, 6) == "<this>")
		return 5u;
	if (sString.substr(nPos, 10) == "<loadpath>")
		return 9u;
	if (sString.substr(nPos, 10) == "<savepath>")
		return 9u;
	if (sString.substr(nPos, 10) == "<procpath>")
		return 9u;
	if (sString.substr(nPos, 10) == "<plotpath>")
		return 9u;
	if (sString.substr(nPos, 12) == "<scriptpath>")
		return 11u;
	return 0u;
}

// This static function will evaluate logical string expressions in
// the passed command line
static string parser_evalStringLogic(string sLine, Parser& _parser, bool& bReturningLogicals)
{
    // Exclude border cases
	if (!sLine.length())
		return "false";
	if (sLine.find('"') == string::npos)
	{
		bReturningLogicals = true;
		return sLine;
	}

	sLine += " ";

	// Evaluate ternaries first (will start a recursion)
	sLine = parser_evalStringTernary(sLine, _parser);

	unsigned int nPos = 0;

	// Handle parenthesed expression parts
	// (will start a recursion)
	if (sLine.find('(') != string::npos)
	{
		nPos = 0;
		while (sLine.find('(', nPos) != string::npos)
		{
			nPos = sLine.find('(', nPos) + 1;
			if (!isInQuotes(sLine, nPos - 1))
			{
				sLine = sLine.substr(0, nPos - 1) + parser_evalStringLogic(sLine.substr(nPos, getMatchingParenthesis(sLine.substr(nPos - 1)) - 1), _parser, bReturningLogicals) + sLine.substr(getMatchingParenthesis(sLine.substr(nPos - 1)) + nPos);
				nPos = 0;
			}
		}
	}

	// Handle the logical and
	if (sLine.find("&&") != string::npos)
	{
		nPos = 0;
		while (sLine.find("&&", nPos) != string::npos)
		{
			nPos = sLine.find("&&", nPos) + 2;
			if (!isInQuotes(sLine, nPos - 2))
			{
				string sLeft = removeMaskedStrings(parser_evalStringLogic(sLine.substr(0, nPos - 2), _parser, bReturningLogicals));
				string sRight = removeMaskedStrings(parser_evalStringLogic(sLine.substr(nPos), _parser, bReturningLogicals));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft == "true" && sRight == "true")
					return "true";
				else if (sLeft == "false" || sRight == "false")
					return "false";
				else
				{
					_parser.SetExpr(sLeft + " && " + sRight);
					return toString((bool)_parser.Eval());
				}
			}
		}
	}

	// Handle the exclusive or
	if (sLine.find("|||") != string::npos)
	{
		nPos = 0;
		while (sLine.find("|||", nPos) != string::npos)
		{
			nPos = sLine.find("|||", nPos) + 3;
			if (!isInQuotes(sLine, nPos - 3))
			{
				string sLeft = removeMaskedStrings(parser_evalStringLogic(sLine.substr(0, nPos - 3), _parser, bReturningLogicals));
				string sRight = removeMaskedStrings(parser_evalStringLogic(sLine.substr(nPos), _parser, bReturningLogicals));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if ((sLeft == "true" && sRight == "false") || (sLeft == "false" && sRight == "true"))
					return "true";
				else if (sLeft == sRight)
					return "false";
				else
				{
					_parser.SetExpr(sLeft + " ||| " + sRight);
					return toString((bool)_parser.Eval());
				}
			}
		}
	}

	// Handle the logical or
	if (sLine.find("||") != string::npos)
	{
		nPos = 0;
		while (sLine.find("||", nPos) != string::npos)
		{
			nPos = sLine.find("||", nPos) + 2;
			if (!isInQuotes(sLine, nPos - 2))
			{
				string sLeft = removeMaskedStrings(parser_evalStringLogic(sLine.substr(0, nPos - 2), _parser, bReturningLogicals));
				string sRight = removeMaskedStrings(parser_evalStringLogic(sLine.substr(nPos), _parser, bReturningLogicals));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;

				if (sLeft == "true" || sRight == "true")
					return "true";
				else if (sLeft == "false" && sRight == "false")
					return "false";
				else
				{
					_parser.SetExpr(sLeft + " || " + sRight);
					return toString((bool)_parser.Eval());
				}
			}
		}
	}

	int nQuotes = 0;

	// Handle logical comparisons
	for (size_t i = 0; i < sLine.length(); i++)
	{
		if (sLine[i] == '"' && (!i || sLine[i - 1] != '\\'))
			nQuotes++;
		if (!(nQuotes % 2))
		{
			if (sLine.substr(i, 2) == "==")
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 2));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft == sRight)
					return "true";
				else
					return "false";
			}
			else if (sLine.substr(i, 2) == "!=")
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 2));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft != sRight)
					return "true";
				else
					return "false";
			}
			else if (sLine.substr(i, 2) == "<=")
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 2));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft <= sRight)
					return "true";
				else
					return "false";
			}
			else if (sLine.substr(i, 2) == ">=")
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 2));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft >= sRight)
					return "true";
				else
					return "false";
			}
			else if (sLine[i] == '<')
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 1));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft < sRight)
					return "true";
				else
					return "false";
			}
			else if (sLine[i] == '>')
			{
				string sLeft = removeMaskedStrings(sLine.substr(0, i));
				string sRight = removeMaskedStrings(sLine.substr(i + 1));
				StripSpaces(sLeft);
				StripSpaces(sRight);
				if (sLeft[0] == '"' && sLeft[sLeft.length() - 1] == '"')
					sLeft = sLeft.substr(1, sLeft.length() - 2);
				if (sRight[0] == '"' && sRight[sRight.length() - 1] == '"')
					sRight = sRight.substr(1, sRight.length() - 2);
				bReturningLogicals = true;
				if (sLeft > sRight)
					return "true";
				else
					return "false";
			}

		}
	}
	StripSpaces(sLine);
	return sLine;
}

// This static function will evaluate the ternary
// for strings
static string parser_evalStringTernary(string sLine, Parser& _parser)
{
	bool bReturningLogicals = false;
	size_t nPos = 0;

    // As long as there's a question mark in the current string
    while (sLine.find('?', nPos) != string::npos)
    {
        nPos = sLine.find('?', nPos);

        // Ensure, that we're not in a string literal
        if (!isInQuotes(sLine, nPos))
        {
            // Get the current ternary as a vector
            vector<string> vTernary = parser_getStringTernaryExpression(sLine, nPos);
            nPos = 0;

            // Evaluate logical string expressions
            vTernary[0] = parser_evalStringLogic(vTernary[0], _parser, bReturningLogicals);
            bool result = false;

            // Evaluate the condition of the ternary logically
            if (vTernary[0].find('"') != string::npos)
            {
                StripSpaces(vTernary[0]);
                result = (bool)(vTernary[0].length() - 2);
            }
            else
            {
                _parser.SetExpr(vTernary[0]);
                result = (bool)_parser.Eval();
            }

            // return the evaluated part of the string
            if (result)
                return sLine + parser_evalStringLogic(vTernary[1], _parser, bReturningLogicals);
            else
                return sLine + parser_evalStringLogic(vTernary[2], _parser, bReturningLogicals);
        }
        else
            nPos++;
    }

	return sLine;
}

// This static function is a helper for parser_evalStringTernary
// it will return the expressions of the current ternary in the
// different vector components
static vector<string> parser_getStringTernaryExpression(string& sLine, size_t& nPos) // nPos ist the position of the question mark
{
	vector<string> vTernary;
	size_t nTernaryStart = 0;
	size_t nColonPosition = 0;

	string sTernary = sLine.substr(nTernaryStart);
	sLine.erase(nTernaryStart);

	size_t quotes = 0;
	int nQuestionMarks = 0;

	// Search for the operators of the ternary
	// Jump over additional quotation marks
	for (size_t i = nPos; i < sTernary.length(); i++)
	{
	    // Jump ver parentheses
		if (!(quotes % 2) && (sTernary[i] == '(' || sTernary[i] == '[' || sTernary[i] == '{'))
			i += getMatchingParenthesis(sTernary.substr(i));

		// Increment the question mark counter
		if (!(quotes % 2) && sTernary[i] == '?')
			nQuestionMarks++;

        // Increment the quotation mark counter
		if (sTernary[i] == '"' && sTernary[i - 1] != '\\')
			quotes++;

        // If there's a colon, decrement the quotation mark
        // counter
		if (!(quotes % 2) && sTernary[i] == ':')
		{
			nQuestionMarks--;
			if (!nQuestionMarks)
			{
			    // this is the correct colon
				nColonPosition = i;
				break;
			}
		}
	}

	// This is obviously not a real ternary
	if (!nColonPosition)
		throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, nPos);

    // Distribute the expression parts of the ternary
    // across the vector components
	vTernary.push_back(sTernary.substr(0, nPos));
	vTernary.push_back(sTernary.substr(nPos + 1, nColonPosition - 1 - nPos));
	vTernary.push_back(sTernary.substr(nColonPosition + 1));

	nPos = nTernaryStart;
	return vTernary;
}

static string parser_concatenateStrings(const string& sExpr)
{
    string sConcatenated = sExpr;
    size_t nQuotes = 0;

    for (unsigned int i = 0; i < sConcatenated.length(); i++)
    {
        if (sConcatenated[i] == '"' && (!i || sConcatenated[i-1] != '\\'))
            nQuotes++;

        // Search for the concatenation operator (aka "+")
        if (!(nQuotes % 2) && sConcatenated[i] == '+')
        {
            string sLeft = sConcatenated.substr(0, i);
            string sRight = sConcatenated.substr(i+1);

            StripSpaces(sLeft);
            StripSpaces(sRight);

            // Determine the correct concatenation process
            if (sLeft == "\"\"" && sRight != "\"\"")
            {
                sConcatenated = " " + sRight;
                i = 0;
            }
            else if (sLeft != "\"\"" && sRight == "\"\"")
            {
                sConcatenated = sLeft;
                break;
            }
            else if (sLeft.back() == '"' && sRight.front() == '"')
            {
                sConcatenated = sLeft.substr(0, sLeft.length()-1) + sRight.substr(1);

                // We removed some characters
                i = sLeft.length()-2;

                // We're now part of a string
                nQuotes++;
            }

            // Everything not catched here is a strange mixture
        }
    }

    StripSpaces(sConcatenated);

    return sConcatenated;
}


