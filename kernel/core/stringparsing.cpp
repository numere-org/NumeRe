
#include "parser_functions.hpp"
#include "../kernel.hpp"

extern value_type vAns;
extern Integration_Vars parser_iVars;
//extern bool bSupressAnswer;
extern mglGraph _fontData;
extern Plugin _plugin;

#define DEFAULT_NUM_ARG INT_MIN
#define NEWSTRING (char)23
// define the "End of transmission block" as string separator

typedef std::vector<std::string> s_vect;
typedef std::vector<int> n_vect;

// Define the string function arguments in a structure
struct StringFuncArgs
{
    string sArg1, sArg2, sArg3;
    s_vect sMultiArg;
    int nArg1, nArg2;
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
    StringFuncHandle() {bTakesMultiArguments = false;}
    StringFuncHandle(FunctionSignatureType _fType, StringFunc _fHandle, bool _bTakesMultiArguments) : fType(_fType), fHandle(_fHandle), bTakesMultiArguments(_bTakesMultiArguments) {}

    FunctionSignatureType fType;
    StringFunc fHandle;
    bool bTakesMultiArguments;
};

// Defines the return value of the string parser core
struct StringResult
{
    StringResult() {bOnlyLogicals = false;}
    StringResult(vector<string>& _vResult, vector<bool>& _vNoStringVal, bool _bOnlyLogicals) : vResult(_vResult), vNoStringVal(_vNoStringVal), bOnlyLogicals(_bOnlyLogicals) {}
    StringResult(const string& sRet, bool _bOnlyLogicals = false)
        {
            StringResult();
            vResult.push_back(sRet);
            vNoStringVal.push_back(sRet.find('"') == string::npos);
            bOnlyLogicals = _bOnlyLogicals;
        }

    vector<string> vResult;
    vector<bool> vNoStringVal;
    bool bOnlyLogicals;
};


// Function handler:
// ======================
string parser_ApplySpecialStringFuncs(string sLine, Datafile&, Parser&, const Settings&, map<string, vector<string> >&);
string parser_ApplyStringFuncs(string sLine, Datafile&, Parser&, const Settings&, map<string, vector<string> >&);
void parser_StringFuncHandler(string& sLine, const string& sFuncName, Datafile&, Parser&, const Settings&, map<string, vector<string> >&, StringFuncHandle);
map<string, StringFuncHandle> parser_getStringFuncHandles();
string removeMaskedStrings(const string& sString);
string addMaskedStrings(const string& sString);
string listToVector(const string& sString);
string removeQuotationMarks(const string& sString);
string addQuotationMarks(const string& sString);

StringResult parser_StringParserCore(string& sLine, string sCache, Datafile& _data, Parser& _parser, const Settings& _option, map<string,vector<string> > mStringVectorVars, bool bParseNumericals = true);

string parser_GetDataForString(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, size_t n_pos);
string parser_NumToString(const string& sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars);
int parser_StoreStringResults(const vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string sObject, Datafile& _data, Parser& _parser, const Settings& _option);
string parser_CreateStringOutput(vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string& sLine, bool bNoQuotes, bool bReturningLogicals, bool bKeepMaskedQuotes);
vector<bool> parser_ApplyElementaryStringOperations(vector<string>& vFinal, Parser& _parser, const Settings& _option, bool& bReturningLogicals);
string parser_CreateStringVectorVar(const vector<string>& vStringVector, map<string, vector<string> >& mStringVectorVars);
bool parser_containsStringVectorVars(const string& sLine, const map<string, vector<string> >& mStringVectorVars);
vector<string> parser_EvaluateStringVectors(string sLine, const map<string, vector<string> >& mStringVectorVars);
void parser_ExpandStringVectorComponents(vector<string>& vStringVector);

// String functions:
// ======================
// string STRINGFUNC(ARGS)
//
// Parser functions:
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, n_vect&);
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&);
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, n_vect&, n_vect&);
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, n_vect&, n_vect&, s_vect&);
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, s_vect&, n_vect&, n_vect&);
size_t parser_StringFuncArgParser(Datafile&, Parser&, const Settings&, const string&, map<string, vector<string> >&, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&);
//


// Function signatures
// ====================
// bool PARSER(const string&, string&)
// STR__STR
// str = to_string(EXPR)
string strfnc_to_string(StringFuncArgs& funcArgs)
{
    if (funcArgs.sArg1.find('"') == string::npos)
        return "\"" + funcArgs.sArg1 + "\"";
    return funcArgs.sArg1;
}

// str = string_cast(EXPR)
//string strfnc_string_cast(StringFuncArgs& funcArgs);

// STR__STR
// str = to_uppercase(str)
string strfnc_to_uppercase(StringFuncArgs& funcArgs)
{
    return "\"" + toUpperCase(funcArgs.sArg1) + "\"";
}

// str = to_lowercase(str)
string strfnc_to_lowercase(StringFuncArgs& funcArgs)
{
    return "\"" + toLowerCase(funcArgs.sArg1) + "\"";
}

// cmd = to_cmd(str)
// string strfnc_to_cmd(StringFuncArgs& funcArgs);

// {str} = getfilelist(str, [val])
string strfnc_getfilelist(StringFuncArgs& funcArgs)
{
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    vector<string> vFileList = getFileList(removeMaskedStrings(funcArgs.sArg1), *(funcArgs.opt), funcArgs.nArg1);
    string sFileList = "";
    for (unsigned int i = 0; i < vFileList.size(); i++)
    {
        sFileList += "\"" + vFileList[i] + "\"";
        if (i < vFileList.size()-1)
            sFileList += NEWSTRING;
    }
    if (!sFileList.length())
        return "\"\"";
    else
        return sFileList;
}
// {str} = getfolderlist(str, [val])
string strfnc_getfolderlist(StringFuncArgs& funcArgs)
{
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG)
        funcArgs.nArg1 = 0;

    vector<string> vFolderList = getFolderList(removeMaskedStrings(funcArgs.sArg1), *(funcArgs.opt), funcArgs.nArg1);
    string sFolderList = "";
    for (unsigned int i = 0; i < vFolderList.size(); i++)
    {
        sFolderList += "\"" + vFolderList[i] + "\"";
        if (i < vFolderList.size()-1)
            sFolderList += NEWSTRING;
    }
    if (!sFolderList.length())
        return "\"\"";
    else
        return sFolderList;
}

// VAL__STR
// val = to_value(str)
string strfnc_to_value(StringFuncArgs& funcArgs)
{
    return funcArgs.sArg1;
}

// val = strlen(str)
string strfnc_strlen(StringFuncArgs& funcArgs)
{
    return toString((int)removeMaskedStrings(funcArgs.sArg1).length());
}

// val = getmatchingparens(str)
string strfnc_getmatchingparens(StringFuncArgs& funcArgs)
{
    return toString((int)getMatchingParenthesis(removeMaskedStrings(funcArgs.sArg1))+1);
}

// {val} = ascii(str)
string strfnc_ascii(StringFuncArgs& funcArgs)
{
    string sCodes = "";
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    for (unsigned int i = 0; i < funcArgs.sArg1.length(); i++)
    {
        sCodes += toString((int)funcArgs.sArg1[i]);
        if (i+1 < funcArgs.sArg1.length())
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
string strfnc_to_char(StringFuncArgs& funcArgs)
{
    string sToChar = "";
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        sToChar += (char)(funcArgs.nMultiArg[i]);
    }
    return "\"" + sToChar + "\"";
}

// val = and(num)
string strfnc_and(StringFuncArgs& funcArgs)
{
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        if (!funcArgs.nMultiArg[i])
            return "false";
    }
    return "true";
}

// val = or(num)
string strfnc_or(StringFuncArgs& funcArgs)
{
    string sToChar = "";
    for (size_t i = 0; i < funcArgs.nMultiArg.size(); i++)
    {
        if (funcArgs.nMultiArg[i])
            return "true";
    }
    return "false";
}

// ----------------------------
// bool PARSER(const string&, string&, string&)
// LOG__STR_STROPT
// log = findfile(str, [str])
string strfnc_findfile(StringFuncArgs& funcArgs)
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
string strfnc_split(StringFuncArgs& funcArgs)
{
    string sSplittedString = "";
    if (!funcArgs.sArg2.length())
        return "\"\"";
    boost::char_separator<char> cSep(funcArgs.sArg2.substr(0,1).c_str());
    tokenizer<char_separator<char> > tok(funcArgs.sArg1, cSep);
    for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
    {
        if (sSplittedString.length())
            sSplittedString += NEWSTRING;
        sSplittedString += "\"" + string(*iter) + "\"";
    }
    return sSplittedString;
}

// STR__STR_STROPT
// str = valtostr(EXPR, [str], [val])
// string strfnc_valtostr(StringFuncArgs& funcArgs)

// ----------------------------
// bool PARSER(const string&, string&, string&, int&)
// VAL__STR_STR_VALOPT
// val = strfnd(str, str, [val])
string strfnc_strfnd(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.find(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}
// val = strmatch(str, str, [val])
string strfnc_strmatch(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.find_first_of(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}

// val = str_not_match(str, str, [val])
string strfnc_str_not_match(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = 1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.find_first_not_of(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}

// val = strrfnd(str, str, [val])
string strfnc_strrfnd(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = funcArgs.sArg2.length()+1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.rfind(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}

// val = strrmatch(str, str, [val])
string strfnc_strrmatch(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = funcArgs.sArg2.length()+1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.find_last_of(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}

// val = str_not_rmatch(str, str, [val])
string strfnc_str_not_rmatch(StringFuncArgs& funcArgs)
{
    if (!funcArgs.sArg2.length())
        return "0";
    if (funcArgs.nArg1 == DEFAULT_NUM_ARG || funcArgs.nArg1 < 0 || funcArgs.sArg2.length() < (size_t)funcArgs.nArg1)
        funcArgs.nArg1 = funcArgs.sArg2.length()+1;

    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.sArg2.front() == '"')
        funcArgs.sArg2 = funcArgs.sArg2.substr(funcArgs.sArg2.find('"')+1, funcArgs.sArg2.rfind('"')-funcArgs.sArg2.find('"')-1);
    return toString((int)funcArgs.sArg2.find_last_not_of(funcArgs.sArg1, funcArgs.nArg1-1)+1);
}

// ----------------------------
// bool PARSER(const string&, string&, string&, string&)
// VAL__STR_STR_STROPT
// val = findparam(str, str, [str])
string strfnc_findparam(StringFuncArgs& funcArgs)
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
        return toString((int)(nMatch+1));
    else
        return "0";
}

// ----------------------------
// bool PARSER(const string&, string&, int&, int&)
// STR__STR_VAL_VALOPT
// str = substr(str, val, [val])
string strfnc_substr(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;
    if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
        funcArgs.nArg1 = funcArgs.sArg1.length();
    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;
    return "\"" + funcArgs.sArg1.substr(funcArgs.nArg1-1, funcArgs.nArg2) + "\"";
}

// ----------------------------
// bool PARSER(const string&, string&, int&)
// STR__STR_VAL
// str = repeat(str, val)
string strfnc_repeat(StringFuncArgs& funcArgs)
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

// str = char(str, val)
string strfnc_char(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    if (funcArgs.nArg1 <= 1)
        return "\"" + funcArgs.sArg1.substr(0,1) + "\"";
    if ((size_t)funcArgs.nArg1 >= funcArgs.sArg1.length())
        return "\"" + funcArgs.sArg1.substr(funcArgs.sArg1.length()-1) + "\"";
    return "\"" + funcArgs.sArg1.substr(funcArgs.nArg1-1, 1) + "\"";
}

// str = getopt(str, val)
string strfnc_getopt(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    if (funcArgs.nArg1 <= 1)
        funcArgs.nArg1 = 1;
    if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
        return "\"\"";

    return "\"" + getArgAtPos(funcArgs.sArg1, funcArgs.nArg1-1) + "\"";
}

// VAL__STR_VALOPT
// {val} = getindices(str, [val])
// string strfnc_getindices(StringFuncArgs& funcArgs);

// ----------------------------
// bool PARSER(const string&, string&, string&, int&, int&)
// STR__STR_STR_VALOPT_VALOPT
// str = replace(str, str, [val], [val])
string strfnc_replace(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);

    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;
    if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
        funcArgs.nArg1 = funcArgs.sArg1.length();
    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = -1;
    return "\"" + funcArgs.sArg1.replace(funcArgs.nArg1-1, funcArgs.nArg2, funcArgs.sArg2) + "\"";
}

// ----------------------------
// bool PARSER(const string&, string&, string&, string&, int&, int&)
// STR__STR_STR_STR_VALOPT_VALOPT
// str = replaceall(str, str, str, [val], [val])
string strfnc_replaceall(StringFuncArgs& funcArgs)
{
    funcArgs.sArg1 = removeMaskedStrings(funcArgs.sArg1);
    funcArgs.sArg2 = removeMaskedStrings(funcArgs.sArg2);
    funcArgs.sArg3 = removeMaskedStrings(funcArgs.sArg3);

    if (funcArgs.nArg1 < 1)
        funcArgs.nArg1 = 1;
    if ((size_t)funcArgs.nArg1 > funcArgs.sArg1.length())
        funcArgs.nArg1 = funcArgs.sArg1.length();
    if (funcArgs.nArg2 == DEFAULT_NUM_ARG)
        funcArgs.nArg2 = funcArgs.sArg1.length()+1;
    if (!funcArgs.sArg2.length())
        return "\"" + funcArgs.sArg1 + "\"";

    for (int i = funcArgs.nArg1-1; i < funcArgs.nArg2-1; i++)
    {
        if ((size_t)i == funcArgs.sArg1.length())
            break;
        if (funcArgs.sArg1.substr(i, funcArgs.sArg2.length()) == funcArgs.sArg2)
        {
            funcArgs.sArg1.replace(i, funcArgs.sArg2.length(), funcArgs.sArg3);
            funcArgs.nArg2 += funcArgs.sArg3.length()-funcArgs.sArg2.length()+1;
            i += funcArgs.sArg3.length()-1;
        }
    }

    return "\"" + funcArgs.sArg1 + "\"";
}

// ----------------------------
// bool PARSER(const string&, string&)
// VAL__STR
// val = cnt({str})
string strfnc_cnt(StringFuncArgs& funcArgs)
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
string strfnc_num(StringFuncArgs& funcArgs)
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
string strfnc_min(StringFuncArgs& funcArgs)
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
string strfnc_max(StringFuncArgs& funcArgs)
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
string strfnc_sum(StringFuncArgs& funcArgs)
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



// --> Verarbeitet String-Ausdruecke <--
int parser_StringParser(string& sLine, string& sCache, Datafile& _data, Parser& _parser, const Settings& _option, bool bSilent)
{
    bool bNoQuotes = false;
    bool bPeek = false;
    bool bKeepMaskedQuotes = false;

    sLine = " " + sLine + " ";

    // Process the parameters
    if (matchParams(sLine, "noquotes")
        || matchParams(sLine, "nq")
        || matchParams(sLine, "peek")
        || matchParams(sLine, "print")
        || matchParams(sLine, "kmq"))
    {
        int nPos = (int)sLine.length();
        if (matchParams(sLine, "noquotes") < nPos && matchParams(sLine, "noquotes"))
        {
            nPos = matchParams(sLine, "noquotes");
            bNoQuotes = true;
        }
        if (matchParams(sLine, "nq") < nPos && matchParams(sLine, "nq"))
        {
            nPos = matchParams(sLine, "nq");
            bNoQuotes = true;
        }
        if (matchParams(sLine, "peek") < nPos && matchParams(sLine, "peek"))
        {
            nPos = matchParams(sLine, "peek");
            bPeek = true;
        }
        if (matchParams(sLine, "kmq") < nPos && matchParams(sLine, "kmq"))
        {
            nPos = matchParams(sLine, "kmq");
            bKeepMaskedQuotes = true;
        }
        if (matchParams(sLine, "print")  < nPos && matchParams(sLine, "print"))
        {
            nPos = matchParams(sLine, "print");
            bNoQuotes = true;
            bPeek = true;
        }
        nPos = sLine.rfind('-', nPos);
        sLine = sLine.substr(0, nPos);
    }

//-------------------------
// new: string core functionality
    map<string, vector<string> > mStringVectorVars;
    StringResult StrRes = parser_StringParserCore(sLine, sCache, _data, _parser, _option, mStringVectorVars);
//-------------------------

    string sConsoleOut = parser_CreateStringOutput(StrRes.vResult, StrRes.vNoStringVal, sLine, bNoQuotes, StrRes.bOnlyLogicals, bKeepMaskedQuotes);


    if (NumeReKernel::bSupressAnswer)
        bSilent = true;

    if (bPeek)
        NumeReKernel::printPreFmt("\r                                                       \r");
    if ((!bSilent || bPeek) && !StrRes.bOnlyLogicals)
        NumeReKernel::printPreFmt(LineBreak(sConsoleOut, _option, false, 0) + "\n");

    if (StrRes.bOnlyLogicals)
        return -1;
    else
        return 1;
}


StringResult parser_StringParserCore(string& sLine, string sCache, Datafile& _data, Parser& _parser, const Settings& _option, map<string,vector<string> > mStringVectorVars, bool bParseNumericals)
{
    StringResult strRes;

    string sObject;

    // Identify target vectors and parse this as a list
    size_t eq_pos = sLine.find('=');
    if (sLine.find('{') != string::npos
        && eq_pos != string::npos
        && sLine.find('{') < eq_pos
        && sLine.find('<') != eq_pos-1
        && sLine.find('>') != eq_pos-1
        && sLine.find('!') != eq_pos-1
        && sLine[eq_pos+1] != '=')
    {
        while (isInQuotes(sLine, eq_pos) && sLine.find('=', eq_pos+1) != string::npos)
            eq_pos = sLine.find('=', eq_pos+1);
        if (!isInQuotes(sLine, eq_pos))
        {
            string sLeftSide = sLine.substr(0,eq_pos+1);
            sLine.erase(0,eq_pos+1);

            StringResult _res = parser_StringParserCore(sLine, "", _data, _parser, _option, mStringVectorVars);
            string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

            if (!strvar.length())
                throw STRING_ERROR;
            sLine = sLeftSide + strvar;

//            if (!containsStrings(sLine) && !_data.containsStringVars(sLine))
//                return StringResult(sLine);

        }
    }
    else if (sLine.find('{') != string::npos
        && sLine.find("string(") != string::npos
        && eq_pos != string::npos
        && sLine.find("string(") < sLine.find('{')
        && eq_pos > sLine.find("string(")
        && sLine.find('=') < sLine.find('{')
        && sLine[eq_pos-1] != '<'
        && sLine[eq_pos-1] != '>'
        && sLine[eq_pos-1] != '!'
        && sLine[eq_pos+1] != '='
        )
    {
        for (unsigned int i = 0; i < sLine.find('='); i++)
        {
            if (sLine[i] != ' ')
            {
                if (sLine.substr(i, 7) == "string(")
                {
                    i += getMatchingParenthesis(sLine.substr(i+6))+6;
                    if (sLine.find_first_not_of(' ', i) == sLine.find('='))
                    {
                        string sLeftSide = sLine.substr(0,sLine.find('=')+1);
                        sLine.erase(0,sLine.find('=')+1);

                        StringResult _res = parser_StringParserCore(sLine, "", _data, _parser, _option, mStringVectorVars);

                        string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

                        if (!strvar.length())
                            throw STRING_ERROR;
                        sLine = sLeftSide + strvar;

            //            if (!containsStrings(sLine) && !_data.containsStringVars(sLine))
            //                return StringResult(sLine);
                    }
                    break;
                }
                else
                {
                    parser_VectorToExpr(sLine, _option);
                    break;
                }
            }
        }
    }
    /*else if (sLine.find('{') != string::npos)
    {
        parser_VectorToExpr(sLine, _option);
    }*/

    // Recurse for multiple store targets
    // Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.

    if (sLine.find(',') != string::npos && sLine.find('=') != string::npos && !isInQuotes(sLine, sLine.find('=')))
    {
        string sStringObject = sLine.substr(0, sLine.find('='));
        if (sStringObject.substr(sStringObject.find_first_not_of(' '), 7) == "string("
            || sStringObject.substr(sStringObject.find_first_not_of(' '), 5) == "data("
            || _data.containsCacheElements(sStringObject))
        {
            unsigned int nPos = getMatchingParenthesis(sLine);
            nPos = sLine.find('=', nPos);
            if (nPos == string::npos)
                throw STRING_ERROR;
            if (sLine[nPos+1] == '=')
                nPos++;
            sStringObject = sLine.substr(0, nPos + 1);
            sLine.erase(0,nPos+1);
        }
        else
            sStringObject.clear();
        StripSpaces(sLine);

        if (sLine != getNextArgument(sLine, false))
        {
            string sRecursion = "";
            string sParsed = "";
            vector<string> vResult;
            while (sLine.length())
            {
                sRecursion = getNextArgument(sLine, true);
                if (sLine.length() || sRecursion.find('=') != string::npos)
                {
                    StringResult _res = parser_StringParserCore(sRecursion, "", _data, _parser, _option, mStringVectorVars);
                    for (size_t n = 0; n < _res.vResult.size(); n++)
                        vResult.push_back(_res.vResult[n]);
                }
                else if (sRecursion.find('=') == string::npos)
                    vResult.push_back(sRecursion);
            }
            sLine = parser_CreateStringVectorVar(vResult, mStringVectorVars);
        }
        if (sStringObject.length())
            sLine = sStringObject + sLine;
    }

    // Get the string variables
    unsigned int n_pos = 0;
    eq_pos = sLine.find('=');
    if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && eq_pos != sLine.find("==")
        && eq_pos != sLine.find("!=")+1
        && eq_pos != sLine.find("<=")+1
        && eq_pos != sLine.find(">=")+1)
        n_pos = eq_pos + 1;

    if (_data.containsStringVars(sLine.substr(n_pos)))
        _data.getStringValues(sLine, n_pos);

    // NEW: String func handler
    sLine = parser_ApplyStringFuncs(sLine, _data, _parser, _option, mStringVectorVars);

    sLine = parser_ApplySpecialStringFuncs(sLine, _data, _parser, _option, mStringVectorVars);

    eq_pos = sLine.find('=');
    // Extract target object
    if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && eq_pos != sLine.find("==")
        && eq_pos != sLine.find("!=")+1
        && eq_pos != sLine.find("<=")+1
        && eq_pos != sLine.find(">=")+1)
    {
        if (sLine.substr(0, eq_pos).find("data(") != string::npos || _data.containsCacheElements(sLine.substr(0, eq_pos)))
        {
            sObject = sLine.substr(0, eq_pos);
            sLine.erase(0, eq_pos+1);
        }
    }

    // Get the contents of "string()", "data()" and the other caches
    sLine = parser_GetDataForString(sLine, _data, _parser, _option, mStringVectorVars, n_pos);

    if (!containsStrings(sLine) && !_data.containsStringVars(sLine) && !parser_containsStringVectorVars(sLine, mStringVectorVars))
    {
        if (sObject.length() && !sCache.length())
            sCache = sObject;
        if (sLine.find("string(") != string::npos || _data.containsStringVars(sLine))
            throw STRING_ERROR;

        // make sure that there are parseable characters
        if (sLine.find_first_not_of("+-*/:?!.,;%&<>=^ ") != string::npos && bParseNumericals)
        {
            int nResults = 0;
            value_type* v = 0;
            _parser.SetExpr(sLine);
            v = _parser.Eval(nResults);
            vAns = v[0];
            eq_pos = sLine.find('=');
            if (eq_pos != string::npos
                && eq_pos
                && eq_pos < sLine.length()+1
                && sLine[eq_pos+1] != '='
                && sLine[eq_pos-1] != '!'
                && sLine[eq_pos-1] != '<'
                && sLine[eq_pos-1] != '>')
                sLine.erase(0,eq_pos+1);
            StripSpaces(sLine);
        }
        return StringResult(sLine, true);
    }

    n_pos = 0;
    if (sLine.find('(') != string::npos)
    {
        size_t nQuotes = 0;
        for (size_t i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '"')
            {
                if (i && sLine[i-1] == '\\')
                    continue;
                nQuotes++;
            }
            // Consider the var parsing feature
            if (sLine[i] == '#' && !(nQuotes % 2))
            {
                for (size_t j = i; j < sLine.length(); j++)
                {
                    if (sLine[j] == '"')
                        throw STRING_ERROR;
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
            if (sLine[i] == '(' && !(nQuotes % 2))
            {
                if (getMatchingParenthesis(sLine.substr(i)) == string::npos)
                    throw UNMATCHED_PARENTHESIS;

                size_t nPos = getMatchingParenthesis(sLine.substr(i)) + i;
                if (i < 6 || (i >= 6 && sLine.substr(i-6,6) != "string"))
                {
                    string sString = sLine.substr(i+1, nPos-i-1);

                    if (i > 0 && !checkDelimiter(sLine.substr(i-1, nPos-i+2))) // this is probably a numerical function. Keep the parentheses
                    {
                        if (containsStrings(sString) || _data.containsStringVars(sString))
                        {
                            StringResult _res = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars);
                            string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

                            if (!strvar.length())
                                throw STRING_ERROR;
                            sLine = sLine.substr(0,i+1) + strvar + sLine.substr(nPos);
                        }
                    }
                    else // replace the whole parenthesis
                    {
                        if (containsStrings(sString) || _data.containsStringVars(sString))
                        {
                            StringResult _res = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars);
                            string strvar = parser_CreateStringVectorVar(_res.vResult, mStringVectorVars);

                            if (!strvar.length())
                                throw STRING_ERROR;
                            sLine = sLine.substr(0,i) + strvar + sLine.substr(nPos+1);
                        }
                    }
                }
            }
        }
    }

    if (sLine.find('{') != string::npos)
    {
        n_pos = 0;
        if (sLine.find('=') != string::npos
            && sLine.find('=') != sLine.find("==")
            && sLine.find('=') != sLine.find("!=")+1
            && sLine.find('=') != sLine.find("<=")+1
            && sLine.find('=') != sLine.find(">=")+1
            && !sObject.length()
            && !isInQuotes(sLine, sLine.find('=')))
            n_pos = sLine.find('=') + 1;
        string sVectortemp = sLine.substr(0,n_pos);
        sLine.erase(0,n_pos);
        parser_VectorToExpr(sLine, _option);
        sLine = sVectortemp + sLine;
    }

    StripSpaces(sLine);
    if (!sLine.length())
        return StringResult("");

    if (sLine.find('"') == string::npos && sLine.find('#') == string::npos && !parser_containsStringVectorVars(sLine, mStringVectorVars))
    {
        if (sObject.length() && !sCache.length())
            sCache = sObject;
        if (sLine.find("string(") != string::npos || _data.containsStringVars(sLine))
            throw STRING_ERROR;
        return StringResult(sLine);
    }

    // Extract the object, if not already done
    eq_pos = sLine.find('=');
    if (!sObject.length()
        && eq_pos != string::npos
        && !isInQuotes(sLine, eq_pos)
        && eq_pos != sLine.find("==")
        && eq_pos != sLine.find("!=")+1
        && eq_pos != sLine.find("<=")+1
        && eq_pos != sLine.find(">=")+1)
    {
        sObject = sLine.substr(0, eq_pos);
        sLine = sLine.substr(eq_pos+1);
    }

    // Apply the "#" parser to the string
    sLine = parser_NumToString(sLine, _data, _parser, _option, mStringVectorVars);

    // Split the list to a vector
    strRes.vResult = parser_EvaluateStringVectors(sLine, mStringVectorVars);

    if (!strRes.vResult.size())
        throw STRING_ERROR;

    // Apply some elementary operations such as concatenation and logical operations
    strRes.vNoStringVal = parser_ApplyElementaryStringOperations(strRes.vResult, _parser, _option, strRes.bOnlyLogicals);

        // store the string results in the variables or inb "string()" respectively
    if (!parser_StoreStringResults(strRes.vResult, strRes.vNoStringVal, sObject, _data, _parser, _option))
        throw STRING_ERROR;

    return strRes;
}


string parser_CreateStringVectorVar(const vector<string>& vStringVector, map<string, vector<string> >& mStringVectorVars)
{
    // Return, if empty (something else went wrong)
    if (!vStringVector.size())
        return "";

    string strVectName = "_~STRVECT[" + toString((int)mStringVectorVars.size()) + "]";

    // Does it already exist?
    if (mStringVectorVars.find(strVectName) != mStringVectorVars.end())
        throw STRING_ERROR;

    // save the vector
    mStringVectorVars[strVectName] = vStringVector;

    return strVectName;
}

bool parser_containsStringVectorVars(const string& sLine, const map<string, vector<string> >& mStringVectorVars)
{
    if (!sLine.length() || !mStringVectorVars.size())
        return false;
    for (auto iter = mStringVectorVars.begin(); iter != mStringVectorVars.end(); ++iter)
    {
        if (sLine.find(iter->first) != string::npos)
            return true;
    }
    return false;
}

vector<string> parser_EvaluateStringVectors(string sLine, const map<string, vector<string> >& mStringVectorVars)
{
    vector<string> vRes;

    while (sLine.length())
    {
        string sCurrentComponent = getNextArgument(sLine, true);

        if (!parser_containsStringVectorVars(sCurrentComponent, mStringVectorVars))
            vRes.push_back(sCurrentComponent);
        else
        {
            size_t nCurrentComponent = 0;
            while (true)
            {
                string currentline = sCurrentComponent;
                bool bHasComponents = false;
                for (auto iter = mStringVectorVars.begin(); iter != mStringVectorVars.end(); ++iter)
                {
                    size_t nMatch = 0;
                    while ((nMatch = currentline.find(iter->first)) != string::npos)
                    {
                        if ((iter->second).size() > nCurrentComponent)
                        {
                            bHasComponents = true;
                            currentline.replace(nMatch, (iter->first).length(), (iter->second)[nCurrentComponent]);
                        }
                        else if ((iter->second).size() == 1)
                        {
                            currentline.replace(nMatch, (iter->first).length(), (iter->second)[0]);
                        }
                        else
                            currentline.replace(nMatch, (iter->first).length(), "\"\"");
                    }
                }
                if (!bHasComponents)
                    break;
                vRes.push_back(currentline);
                nCurrentComponent++;
            }
        }
    }

    return vRes;
}

void parser_ExpandStringVectorComponents(vector<string>& vStringVector)
{
    for (size_t i = 0; i < vStringVector.size(); i++)
    {
        if (getNextArgument(vStringVector[i], false) != vStringVector[i])
        {
            string sComponent = vStringVector[i];
            vStringVector[i] = getNextArgument(sComponent, true);
            size_t nComponent = 1;
            while (sComponent.length())
            {
                vStringVector.insert(vStringVector.begin()+i+nComponent, getNextArgument(sComponent, true));
                nComponent++;
            }
            i += nComponent;
        }
    }
}

string parser_ApplySpecialStringFuncs(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
    unsigned int n_pos = 0;

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
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 13))))
        {
            string sToString = sLine.substr(n_pos+12, nPos-n_pos-12);
            if (sToString.find('"') != string::npos || sToString.find('#') != string::npos)
            {
                StringResult strRes = parser_StringParserCore(sToString, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
//                for (size_t n = 0; n < strRes.vResult.size(); n++)
//                {
//                    for (unsigned int i = 0; i < strRes.vResult[n].length(); i++)
//                    {
//                        if (i && strRes.vResult[n][i] == '"' && strRes.vResult[n][i-1] != '\\')
//                            strRes.vResult[n].insert(i,1,'\\');
//                    }
//                    //strRes.vResult[n] = "\"" + strRes.vResult[n] + "\"";
//                }
                sToString = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
            }
            else
                sToString = "\"" + sToString + "\"";
            sLine = sLine.substr(0,n_pos) + sToString + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    // cmd to_cmd(str)
    while (sLine.find("to_cmd(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_cmd(", n_pos) + 6;
        if (isInQuotes(sLine, n_pos, true))
            continue;
        unsigned int nParPos = getMatchingParenthesis(sLine.substr(n_pos));
        if (nParPos == string::npos)
            throw UNMATCHED_PARENTHESIS;
        if (!isInQuotes(sLine, nParPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sCmdString = sLine.substr(n_pos+1, nParPos-1);
            StripSpaces(sCmdString);
            if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString) || parser_containsStringVectorVars(sCmdString, mStringVectorVars))
            {
                StringResult strRes = parser_StringParserCore(sCmdString, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                // use only the first one
                sCmdString = strRes.vResult[0];
            }
            sLine = sLine.substr(0, n_pos-6) + sCmdString + sLine.substr(n_pos + nParPos+1);
            n_pos -= 5;
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
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true)
            && !isInQuotes(sLine, n_pos, true)
            && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 11)))
            && (containsStrings(sLine.substr(n_pos+10, nPos - n_pos-10))
                || _data.containsStringVars(sLine.substr(n_pos+10, nPos - n_pos-10))))
            sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos+1);
        else if (isInQuotes(sLine, nPos)
            || isInQuotes(sLine, n_pos))
            n_pos++;
        else
            sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos+1);
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
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 12))))
        {
            string _sObject = sLine.substr(n_pos+11, nPos-n_pos-11);
            StringResult strRes = parser_StringParserCore(_sObject, "", _data, _parser, _option, mStringVectorVars);
            if (!strRes.vResult.size())
                throw STRING_ERROR;
            // use only first one
            string sType = strRes.vResult[0];
            int nType = 0;
            _sObject = getNextArgument(sType, true);
            if (!sType.length())
            {
                sType = "0";
            }
            if (_sObject[0] == '"')
                _sObject.erase(0,1);
            if (_sObject[_sObject.length()-1] == '"')
                _sObject.erase(_sObject.length()-1);
            StripSpaces(_sObject);
            if (containsStrings(sType) || parser_containsStringVectorVars(sType, mStringVectorVars))
            {
                StringResult strRes = parser_StringParserCore(sType, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                sType = strRes.vResult[0];
            }
            _parser.SetExpr(sType);
            nType = (int)_parser.Eval();
            if (nType < -1 || nType > 2)
                nType = 0;
            if (_sObject.find("string(") != string::npos)
            {
                vector<double> vIndices;
                if (_sObject.find(':', _sObject.find("string(")+7) != string::npos)
                {
                    string s[3];
                    s[0] = _sObject.substr(_sObject.find("string(")+6);
                    parser_SplitArgs(s[0], s[1], ':', _option);
                    StripSpaces(s[0]);
                    StripSpaces(s[1]);
                    if (s[1].find(',') != string::npos)
                        parser_SplitArgs(s[1],s[2],',',_option, true);
                    if (!s[0].length())
                        vIndices.push_back(1.0);
                    else
                    {
                        _parser.SetExpr(s[0]);
                        vIndices.push_back(_parser.Eval());
                    }
                    vIndices.push_back(-1.0);
                    if (!s[2].length())
                        vIndices.push_back(1.0);
                    else
                    {
                        _parser.SetExpr(s[2]);
                        vIndices.push_back(_parser.Eval());
                    }
                    if (!s[1].length() && nType == -1)
                        vIndices[1] = -1.0;
                    else if (!s[1].length() && nType > 0)
                        vIndices[1] = vIndices.front()+1;
                    else if (!s[1].length())
                    {
                        vIndices[1] = _data.getStringElements((unsigned int)vIndices.back()-1);
                        if (!vIndices[1])
                            vIndices[1] = vIndices[0];
                    }
                    else
                    {
                        _parser.SetExpr(s[1]);
                        vIndices[1] = _parser.Eval();
                    }
                }
                else
                {
                    string s1 = _sObject.substr(_sObject.find("string(")+7, getMatchingParenthesis(_sObject.substr(_sObject.find("string(")+6))-1), sCol = "";
                    if (s1.find(',') != string::npos)
                        parser_SplitArgs(s1, sCol, ',', _option, true);

                    vIndices.push_back(1.0);
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        vIndices.push_back(_parser.Eval());
                    }
                    else
                        vIndices.push_back(1.0);
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        vIndices[0] = _parser.Eval();
                    }
                    else
                        vIndices[0] = _data.getStringElements((unsigned int)vIndices.back()-1);
                }
                _parser.SetVectorVar("indices["+ replaceToVectorname(_sObject) +"]", vIndices);
                sLine = sLine.substr(0,n_pos) + "indices["+ replaceToVectorname(_sObject) +"]" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            if (_sObject.find("data(") == string::npos && !_data.containsCacheElements(_sObject))
            {
                sLine = sLine.substr(0,n_pos) + "nan" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            Indices _mIndex = parser_getIndices(_sObject, _parser, _data, _option);
            if (_mIndex.nI[0] == -1 || _mIndex.nJ[0] == -1)
            {
                sLine = sLine.substr(0,n_pos) + "nan" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            if (nType > -1)
            {
                if (_mIndex.nI[1] == -1)
                    _mIndex.nI[1] = _mIndex.nI[0];
                if (_mIndex.nJ[1] == -1)
                    _mIndex.nJ[1] = _mIndex.nJ[0];
                if (_mIndex.nI[1] == -2)
                {
                    if (nType == 2)
                        _mIndex.nI[1] = _mIndex.nI[0]+1;
                }
                if (_mIndex.nJ[1] == -2)
                {
                    if (nType == 1)
                        _mIndex.nJ[1] = _mIndex.nJ[0]+1;
                }
                if (_sObject.find("data(") != string::npos)
                {
                    if (_mIndex.nI[1] == -2)
                        _mIndex.nI[1] = _data.getLines("data", false)-1;

                    if (_mIndex.nJ[1] == -2)
                        _mIndex.nJ[1] = _data.getCols("data")-1;
                }
                else
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (_sObject.find(iter->first+"(") != string::npos
                            && (!_sObject.find(iter->first+"(")
                                || (_sObject.find(iter->first+"(") && checkDelimiter(_sObject.substr(_sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                        {
                            if (_mIndex.nI[1] == -2)
                                _mIndex.nI[1] = _data.getLines(iter->first, false)-1;
                            if (_mIndex.nJ[1] == -2)
                                _mIndex.nJ[1] = _data.getCols(iter->first)-1;
                        }
                    }
                }
            }
            vector<double> vIndices;
            vIndices.push_back(_mIndex.nI[0]+1);
            vIndices.push_back(_mIndex.nI[1]+1);
            vIndices.push_back(_mIndex.nJ[0]+1);
            vIndices.push_back(_mIndex.nJ[1]+1);
            _parser.SetVectorVar("indices["+replaceToVectorname(_sObject)+ "]", vIndices);
            sLine = sLine.substr(0,n_pos) + "indices["+ replaceToVectorname(_sObject) +"]" + sLine.substr(nPos+1);
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
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 9))))
        {
            string sData = sLine.substr(n_pos+8,nPos-n_pos-8);
            if (containsStrings(sData) || parser_containsStringVectorVars(sData, mStringVectorVars))
            {
                StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                // use only first one
                sData = strRes.vResult[0];
            }
            if (sData[0] == '"')
                sData.erase(0,1);
            if (sData[sData.length()-1] == '"')
                sData.erase(sData.length()-1);
            StripSpaces(sData);
            if (sData.substr(0,5) == "data(" || _data.isCacheElement(sData))
                sLine = sLine.substr(0,n_pos) + "true" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + "false" + sLine.substr(nPos+1);
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
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 10))))
        {
            string sToString = sLine.substr(n_pos+9, nPos-n_pos-9);
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
                        throw STRING_ERROR;
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
                int nResults = 0;
                value_type* v = 0;
                _parser.SetExpr(sExpr);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                    sToString = "{";
                else
                    sToString.clear();
                string sElement = "";
                for (int n = 0; n < nResults; n++)
                {
                    if (fabs(rint(v[n])-v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                        sElement = toString((long long int)rint(v[n]));
                    else
                        sElement = toString(v[n], _option);
                    while (sElement.length() < nCount && sChar.length())
                        sElement.insert(0,sChar);
                    sToString += sElement;
                    if (nResults > 1)
                        sToString += ",";
                }
                if (nResults > 1)
                {
                    sToString.back() = '}';
                }
                sToString = "\"" + sToString + "\"";
            }
            else
            {
                StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                for (size_t i = 0; i < strRes.vResult.size(); i++)
                {
                    while (strRes.vResult[i].length() < nCount && sChar.length())
                        strRes.vResult[i].insert(0,sChar);
                    // add quotation marks, if they are missing
                    strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                }
                sToString = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
            }

            sLine = sLine.substr(0,n_pos) + sToString + sLine.substr(nPos+1);
        }
        n_pos++;
    }
    return sLine;
}

string parser_ApplyStringFuncs(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
    static map<string, StringFuncHandle> mFuncHandleTable = parser_getStringFuncHandles();

    for (auto iter = mFuncHandleTable.begin(); iter != mFuncHandleTable.end(); ++iter)
    {
        if (sLine.find(iter->first + "(") != string::npos)
            parser_StringFuncHandler(sLine, iter->first, _data, _parser, _option, mStringVectorVars, iter->second);
    }
    return sLine;
}

map<string, StringFuncHandle> parser_getStringFuncHandles()
{
    map<string, StringFuncHandle> mHandleTable;

    mHandleTable["strlen"]              = StringFuncHandle(STR, strfnc_strlen, false);
    mHandleTable["ascii"]               = StringFuncHandle(STR, strfnc_ascii, false);
    mHandleTable["to_string"]           = StringFuncHandle(STR, strfnc_to_string, false);
    mHandleTable["to_value"]            = StringFuncHandle(STR, strfnc_to_value, false);
    mHandleTable["to_uppercase"]        = StringFuncHandle(STR, strfnc_to_uppercase, false);
    mHandleTable["to_lowercase"]        = StringFuncHandle(STR, strfnc_to_lowercase, false);
    //mHandleTable["is_string"]           = StringFuncHandle(STR, strfnc_is_string, false); //fehler
    mHandleTable["getmatchingparens"]   = StringFuncHandle(STR, strfnc_getmatchingparens, false);
    mHandleTable["findfile"]            = StringFuncHandle(STR_STROPT, strfnc_findfile, false);
    mHandleTable["split"]               = StringFuncHandle(STR_STR, strfnc_split, false);
    mHandleTable["strfnd"]              = StringFuncHandle(STR_STR_VALOPT, strfnc_strfnd, false);
    mHandleTable["strmatch"]            = StringFuncHandle(STR_STR_VALOPT, strfnc_strmatch, false);
    mHandleTable["str_not_match"]       = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_match, false);
    mHandleTable["strrfnd"]             = StringFuncHandle(STR_STR_VALOPT, strfnc_strrfnd, false);
    mHandleTable["strrmatch"]           = StringFuncHandle(STR_STR_VALOPT, strfnc_strrmatch, false);
    mHandleTable["str_not_rmatch"]      = StringFuncHandle(STR_STR_VALOPT, strfnc_str_not_rmatch, false);
    mHandleTable["findparam"]           = StringFuncHandle(STR_STR_STROPT, strfnc_findparam, false);
    mHandleTable["substr"]              = StringFuncHandle(STR_VAL_VALOPT, strfnc_substr, false);
    mHandleTable["repeat"]              = StringFuncHandle(STR_VAL, strfnc_repeat, false);
    mHandleTable["char"]                = StringFuncHandle(STR_VAL, strfnc_char, false);
    mHandleTable["getopt"]              = StringFuncHandle(STR_VAL, strfnc_getopt, false);
    mHandleTable["replace"]             = StringFuncHandle(STR_VAL_VALOPT_STROPT, strfnc_replace, false); // fehler
    mHandleTable["replaceall"]          = StringFuncHandle(STR_STR_STR_VALOPT_VALOPT, strfnc_replaceall, false);
    mHandleTable["getfilelist"]         = StringFuncHandle(STR_VALOPT, strfnc_getfilelist, false);
    mHandleTable["getfolderlist"]       = StringFuncHandle(STR_VALOPT, strfnc_getfolderlist, false);
    mHandleTable["to_char"]             = StringFuncHandle(VAL, strfnc_to_char, true);
    mHandleTable["and"]                 = StringFuncHandle(VAL, strfnc_and, true);
    mHandleTable["or"]                  = StringFuncHandle(VAL, strfnc_or, true);
    mHandleTable["num"]                 = StringFuncHandle(STR, strfnc_num, true);
    mHandleTable["cnt"]                 = StringFuncHandle(STR, strfnc_cnt, true);
    mHandleTable["min"]                 = StringFuncHandle(STR, strfnc_min, true);
    mHandleTable["max"]                 = StringFuncHandle(STR, strfnc_max, true);
    mHandleTable["sum"]                 = StringFuncHandle(STR, strfnc_sum, true);
    //mHandleTable["valtostr"]            = StringFuncHandle(STR_STROPT_VALOPT, strfnc_valtostr, true);

    return mHandleTable;
}

string removeMaskedStrings(const string& sString)
{
    if (sString.find("\\\"") == string::npos && sString.find("\\t") == string::npos && sString.find("\\n") == string::npos)
        return sString;
    string sRet = sString;

    for (size_t i = 0; i < sRet.length(); i++)
    {
        if (sRet.substr(i,2) == "\\\"")
            sRet.erase(i,1);
        if (sRet.substr(i,2) == "\\t" && sRet.substr(i,4) != "\\tau" && sRet.substr(i,6) != "\\theta")
            sRet.replace(i,2,"\t");
        if (sRet.substr(i,2) == "\\n" && sRet.substr(i,3) != "\\nu")
            sRet.replace(i,2,"\n");
    }

    return sRet;
}

string addMaskedStrings(const string& sString)
{
    if (sString.find('"') == string::npos && sString.find(NEWSTRING) == string::npos)
        return sString;
    string sRet = sString;
    for (size_t i = 1; i < sRet.length()-1; i++)
    {
        if (sRet[i] == '"' && sRet[i-1] != '\\' && sRet[i+1] != NEWSTRING && sRet.find('"', i+1) != string::npos)
        {
            sRet.insert(i, "\\");
            i++;
        }
        if (sRet[i] == NEWSTRING)
        {
            sRet[i] = ',';
            if (sRet[i+1] == '"')
                i++;
        }
        if (sRet[i] == '\t')
            sRet.replace(i,1,"\\t");
        if (sRet[i] == '\n')
            sRet.replace(i,1,"\\n");
    }
    return sRet;
}

string listToVector(const string& sString)
{
    if (sString.find('"') == string::npos && (sString.find(',') == string::npos || sString.find('{') != string::npos))
        return sString;
    else if (sString.find('"') == string::npos)
        return "{" + sString + "}";
    else if (sString.find('"') != string::npos && sString.find(',') != string::npos && sString.find('{') == string::npos)
    {
        int nQuotes = 0;
        for (size_t i = 0; i < sString.length(); i++)
        {
            if (sString[i] == '"' && (!i || sString[i-1] != '\\'))
                nQuotes++;
            if (sString[i] == ',' && !(nQuotes % 2))
            {
                return "{" + sString + "}";
            }
        }
    }
    return sString;
}

string removeQuotationMarks(const string& sString)
{
    if (sString.find('"') == string::npos)
        return sString;
    return sString.substr(1,sString.length()-2);
}

string addQuotationMarks(const string& sString)
{
    if (sString.front() == '"' && sString.back() == '"')
        return sString;
    else
        return "\"" + sString + "\"";
}

void parser_StringFuncHandler(string& sLine, const string& sFuncName, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, StringFuncHandle funcHandle)
{
    size_t n_pos = 0;

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

        unsigned int nPos = n_pos + sFuncName.length();

        // If no matching parenthesis is found, throw an error
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;

        nPos += getMatchingParenthesis(sLine.substr(nPos));

        // Extract the argument of the current found function and process it
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, sFuncName.length()+2))))
        {
            string sFunctionArgument = sLine.substr(n_pos+sFuncName.length()+1, nPos-n_pos-sFuncName.length()-1);
            vector<string> vReturnValues;
            StringFuncArgs stringArgs;
            stringArgs.opt = &_option;

            // Apply the parser as specified by the function signature. After that call the corresponding
            // string function with the returned arguments as many times as it's needed
            if (funcHandle.fType >= PARSER_INT && funcHandle.fType < PARSER_STRING)
            {
                n_vect nIntArg;
                size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, nIntArg);
                if (!nMaxArgs)
                {
                    throw STRING_ERROR;
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
                size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg);
                if (!nMaxArgs)
                {
                    throw STRING_ERROR;
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
                    throw STRING_ERROR;
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
                    throw STRING_ERROR;
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
                    throw STRING_ERROR;
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
            else if (funcHandle.fType >= PARSER_STRING_STRING_STRING_INT_INT)
            {
                s_vect sStringArg1, sStringArg2, sStringArg3;
                n_vect nIntArg1, nIntArg2;
                size_t nMaxArgs = parser_StringFuncArgParser(_data, _parser, _option, sFunctionArgument, mStringVectorVars, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2);
                if (!nMaxArgs)
                {
                    throw STRING_ERROR;
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

            parser_ExpandStringVectorComponents(vReturnValues);

            sFuncReturnValue = parser_CreateStringVectorVar(vReturnValues, mStringVectorVars);

            // replace the function with the return value
            sLine.replace(n_pos, nPos+1-n_pos, sFuncReturnValue);
        }
        n_pos++;
    }
}

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, n_vect& nArg)
{
    string sFuncArgument = __sFuncArgument;
    value_type* v = 0;
    int nReturn = 0;
    if (containsStrings(sFuncArgument) || _data.containsStringVars(sFuncArgument) || parser_containsStringVectorVars(sFuncArgument, mStringVectorVars))
    {
        StringResult strRes = parser_StringParserCore(sFuncArgument, "", _data, _parser, _option, mStringVectorVars);

        for (size_t i = 0; i < strRes.vResult.size(); i++)
        {
            _parser.SetExpr(strRes.vResult[i]);
            v = _parser.Eval(nReturn);
            for (int n = 0; n < nReturn; n++)
                nArg.push_back((int)v[n]);
            //nArg.push_back(StrToInt(strRes.vResult[i]));
        }
        return strRes.vResult.size();
    }
    // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
    if (sFuncArgument.find('{') != string::npos)
    {
        parser_VectorToExpr(sFuncArgument, _option);
    }
    _parser.SetExpr(sFuncArgument);
    v = _parser.Eval(nReturn);
    for (int i = 0; i < nReturn; i++)
    {
        nArg.push_back((int)v[i]);
    }
    return (size_t)nReturn;
}

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg)
{
    string sFuncArgument = __sFuncArgument;
    if (containsStrings(sFuncArgument) || _data.containsStringVars(sFuncArgument) || parser_containsStringVectorVars(sFuncArgument, mStringVectorVars))
    {
        StringResult strRes = parser_StringParserCore(sFuncArgument, "", _data, _parser, _option, mStringVectorVars);
        for (size_t i = 0; i < strRes.vResult.size(); i++)
            sArg.push_back(removeQuotationMarks(strRes.vResult[i]));
        return strRes.vResult.size();
    }
    if (sFuncArgument.find('{') != string::npos || sFuncArgument.find(',') != string::npos)
    {
        parser_VectorToExpr(sFuncArgument, _option);
        while (sFuncArgument.length())
            sArg.push_back(removeQuotationMarks(getNextArgument(sFuncArgument, true)));
    }
    else
        sArg.push_back(removeQuotationMarks(sFuncArgument));
    return sArg.size();
}

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2)
{
    string sFuncArgument = __sFuncArgument;
    size_t nMaxLength = 0;

    string sString = getNextArgument(sFuncArgument, true);
    string sNumVal1 = "";
    string sNumVal2 = "";

    if (sFuncArgument.length())
        sNumVal1 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal2 = getNextArgument(sFuncArgument, true);

    nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString, mStringVectorVars, sArg1);
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

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2, s_vect& sArg2)
{
    string sFuncArgument = __sFuncArgument;
    size_t nMaxLength = 0;

    string sString1 = getNextArgument(sFuncArgument, true);
    string sString2 = "";
    string sNumVal1 = "";
    string sNumVal2 = "";

    if (sFuncArgument.length())
        sNumVal1 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal2 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sString2 = getNextArgument(sFuncArgument, true);

    nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1);
    if (!nMaxLength)
        return 0;
    if (sString2.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
    if (sNumVal1.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
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

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, s_vect& sArg2, n_vect& nArg1, n_vect& nArg2)
{
    string sFuncArgument = __sFuncArgument;
    size_t nMaxLength = 0;

    string sString1 = getNextArgument(sFuncArgument, true);
    string sString2 = "";
    string sNumVal1 = "";
    string sNumVal2 = "";

    if (sFuncArgument.length())
        sString2 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal1 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal2 = getNextArgument(sFuncArgument, true);

    nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1);
    if (!nMaxLength)
        return 0;
    if (sString2.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
    if (sNumVal1.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
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

size_t parser_StringFuncArgParser(Datafile& _data, Parser& _parser, const Settings& _option, const string& __sFuncArgument, map<string, vector<string> >& mStringVectorVars, s_vect& sArg1, s_vect& sArg2, s_vect& sArg3, n_vect& nArg1, n_vect& nArg2)
{
    string sFuncArgument = __sFuncArgument;
    size_t nMaxLength = 0;

    string sString1 = getNextArgument(sFuncArgument, true);
    string sString2 = "";
    string sString3 = "";
    string sNumVal1 = "";
    string sNumVal2 = "";

    if (sFuncArgument.length())
        sString2 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sString3 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal1 = getNextArgument(sFuncArgument, true);
    if (sFuncArgument.length())
        sNumVal2 = getNextArgument(sFuncArgument, true);

    nMaxLength = parser_StringFuncArgParser(_data, _parser, _option, sString1, mStringVectorVars, sArg1);
    if (!nMaxLength)
        return 0;
    if (sString2.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString2, mStringVectorVars, sArg2);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
    if (sString3.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sString3, mStringVectorVars, sArg3);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
    if (sNumVal1.length())
    {
        size_t nReturn = parser_StringFuncArgParser(_data, _parser, _option, sNumVal1, mStringVectorVars, nArg1);
        if (!nReturn)
            return 0;
        if (nMaxLength < nReturn)
            nMaxLength = nReturn;
    }
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

string parser_GetDataForString(string sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars, size_t n_pos)
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
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sString = "";
            if (nPos-n_pos-7 > 0)
            {
                sString = sLine.substr(n_pos+7, nPos-n_pos-7);
                StripSpaces(sString);
            }
            if (sString.length())
            {
                StringResult strRes = parser_StringParserCore(sString, "", _data, _parser, _option, mStringVectorVars, false);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                sString = strRes.vResult[0];
                if (sString.find(':') != string::npos)
                {
                    int i1 = 0, i2 = 0, nCol = 0;
                    string s1 = "(" + sString + ")", s2 = "", sCol = "";
                    parser_SplitArgs(s1, s2, ':', _option);
                    if (s2.find(',') != string::npos)
                        parser_SplitArgs(s2, sCol, ',', _option, true);
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        i1 = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        nCol = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(s2))
                    {
                        _parser.SetExpr(s2);
                        i2 = (unsigned int)_parser.Eval();
                    }
                    else
                        i2 = _data.getStringElements(nCol);
                    parser_CheckIndices(i1, i2);
                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                    {
                        string sLeft = sLine.substr(0, n_pos);
                        StripSpaces(sLeft);
                        if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + toString((int)(i2-i1)) + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.maxString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.minString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.sumString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    }
                    else
                    {
                        if (_data.getStringElements(nCol))
                        {
                            sString = "";
                            vector<string> vStrings;
                            for (int i = i1; i < i2; i++)
                            {
                                vStrings.push_back("\"" +  _data.readString((unsigned int)i, nCol) + "\"");
                            }
                            sString = parser_CreateStringVectorVar(vStrings, mStringVectorVars);
                            sLine = sLine.substr(0, n_pos) + sString + sLine.substr(nPos+1);
                        }
                        else
                            sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nPos+1);
                    }
                }
                else
                {
                    unsigned int nIndex = string::npos, nCol = 0;
                    string s1 = "(" + sString + ")", sCol = "";
                    if (s1.find(',') != string::npos)
                        parser_SplitArgs(s1, sCol, ',', _option);
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        nCol = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        nIndex = (unsigned int)_parser.Eval()-1;
                    }


                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                    {
                        string sLeft = sLine.substr(0, n_pos);

                        StripSpaces(sLeft);
                        if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "1" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    }
                    else
                    {
                        sLine = sLine.substr(0,n_pos) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(nPos+1);
                    }
                }
            }
            else
            {
                //cerr << sLine.substr(0,n_pos) << "***" << sLine.substr(nPos+1) << endl;
                if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                {
                    string sLeft = sLine.substr(0, n_pos);
                    StripSpaces(sLeft);
                    if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "1" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                }
                else
                    sLine = sLine.substr(0,n_pos) + "\"" + _data.readString() + "\"" + sLine.substr(nPos+1);
            }
        }
        n_pos++;
        //cerr << sLine << endl;
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
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 6))))
        {
            if (parser_CheckMultArgFunc(sLine.substr(0,n_pos),sLine.substr(nPos+1)))
            {
                if (n_pos > 4 && sLine.substr(sLine.rfind('(',n_pos)-4,5) == "norm(")
                    n_pos -= 5;
                else
                    n_pos -= 4;
                nPos++;
            }
            string sData = sLine.substr(n_pos, nPos-n_pos+1);
            //cerr << sData << endl;
            parser_GetDataElement(sData, _parser, _data, _option);
            StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);
            if (!strRes.vResult.size())
                throw STRING_ERROR;
            sData = strRes.vResult[0];
            StripSpaces(sData);
            sLine = sLine.substr(0,n_pos) + sData + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (n_pos < sLine.length() && _data.containsCacheElements(sLine.substr(n_pos)))
    {
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sLine.find(iter->first+"(", n_pos) != string::npos)
            {
                n_pos = sLine.find(iter->first+"(", n_pos);
                if (isInQuotes(sLine, n_pos, true))
                {
                    n_pos++;
                    continue;
                }
                unsigned int nPos = n_pos + (iter->first).length();
                if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
                    throw UNMATCHED_PARENTHESIS;
                nPos += getMatchingParenthesis(sLine.substr(nPos));
                if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, (iter->first).length()+2))))
                {
                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos),sLine.substr(nPos+1)))
                    {
                        if (n_pos > 4 && sLine.substr(sLine.rfind('(',n_pos)-4,5) == "norm(")
                            n_pos -= 5;
                        else
                            n_pos -= 4;
                        nPos++;
                    }
                    string sData = sLine.substr(n_pos, nPos-n_pos+1);
                    parser_GetDataElement(sData, _parser, _data, _option);
                    StringResult strRes = parser_StringParserCore(sData, "", _data, _parser, _option, mStringVectorVars);
                    if (!strRes.vResult.size())
                        throw STRING_ERROR;
                    sData = strRes.vResult[0];
                    StripSpaces(sData);
                    sLine = sLine.substr(0,n_pos) + sData + sLine.substr(nPos+1);
                }
            }
        }
        n_pos++;
    }
    return sLine;
}

string parser_NumToString(const string& sLine, Datafile& _data, Parser& _parser, const Settings& _option, map<string, vector<string> >& mStringVectorVars)
{
    if (sLine.find('#') == string::npos)
        return sLine + " ";
    string sLineToParsed = sLine + " ";
    string sLineToParsedTemp;

    unsigned int nPos = 0;
    unsigned int n_pos = 0;
    while (sLineToParsed.find('#', nPos) != string::npos)
    {
        nPos = sLineToParsed.find('#', nPos);
        if (!isInQuotes(sLineToParsed, nPos, true))
        {
            string sPrefix = "";
            sLineToParsedTemp += sLineToParsed.substr(0,nPos);
            sLineToParsed = sLineToParsed.substr(nPos+1);
            if (sLineToParsed[0] == '~')
            {
                for (unsigned int i = 0; i < sLineToParsed.length(); i++)
                {
                    if (sLineToParsed[i] != '~')
                    {
                        sPrefix = sLineToParsed.substr(0,i);
                        sLineToParsed = sLineToParsed.substr(i);
                        break;
                    }
                }
            }
            if (sLineToParsed[0] == '-' || sLineToParsed[0] == '+')
                n_pos = 1;
            else
                n_pos = 0;
            if (sLineToParsed[0] == '(' || sLineToParsed[0] == '{')
            {
                string sExpr = sLineToParsed.substr(1,getMatchingParenthesis(sLineToParsed)-1);
                if (containsStrings(sExpr) || parser_containsStringVectorVars(sExpr, mStringVectorVars))
                {
                    StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);
                    if (!strRes.vResult.size())
                        throw STRING_ERROR;

                    for (size_t i = 0; i < strRes.vResult.size(); i++)
                    {
                        strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                    }
                    sExpr = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
                    /*string sElement = "";
                    string sBlock = "";
                    while (getNextArgument(sExpr, false).length())
                    {
                        sElement = getNextArgument(sExpr, true);
                        while (sElement.length() < sPrefix.length()+1)
                            sElement.insert(0,1,'0');
                        sBlock += "\"" + sElement + "\"";
                        if (getNextArgument(sExpr, false).length())
                        {
                            sBlock += ",";
                            if (sBlock.front() != '{')
                                sBlock.insert(0,1,'{');
                        }
                    }
                    if (sBlock.front() == '{')
                        sBlock += "}";
                    sLineToParsedTemp += sBlock;*/
                    sLineToParsedTemp += sExpr;
                    if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                        sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
                    else
                        sLineToParsed.clear();

                    nPos = 0;
                    continue;
                }
                _parser.SetExpr(sExpr);
            }
            else if (sLineToParsed[0] == '"')
            {
                string sExpr = sLineToParsed.substr(1, sLineToParsed.find('"', 1)-1);
                while (sExpr.length() < sPrefix.length()+2)
                    sExpr.insert(0,1,'0');
                sLineToParsedTemp += "\"" + sExpr + "\"";
                if (sLineToParsed.find('"', 1) < sLineToParsed.length()-1)
                    sLineToParsed = sLineToParsed.substr(sLineToParsed.find('"', 1)+1);
                else
                    sLineToParsed.clear();
                continue;
            }
            else if (sLineToParsed[0] == '<')
            {
                if (sLineToParsed.find("<>") == 0
                    || sLineToParsed.find("<this>") == 0
                    || sLineToParsed.find("<wp>") == 0
                    || sLineToParsed.find("<loadpath>") == 0
                    || sLineToParsed.find("<savepath>") == 0
                    || sLineToParsed.find("<plotpath>") == 0
                    || sLineToParsed.find("<procpath>") == 0
                    || sLineToParsed.find("<scriptpath>") == 0)
                {
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
                    sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>')+1);
                }
                else if (sLineToParsed.find('>') != string::npos)
                {
                    sLineToParsedTemp += "\"" + sLineToParsed.substr(1, sLineToParsed.find('>')-1) + "\"";
                    sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>')+1);
                }
                else
                {
                    throw STRING_ERROR;
                }
                continue;
            }
            else if (parser_containsStringVectorVars(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos))), mStringVectorVars))
            {
                string sExpr = sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
                StringResult strRes = parser_StringParserCore(sExpr, "", _data, _parser, _option, mStringVectorVars);
                if (!strRes.vResult.size())
                    throw STRING_ERROR;
                for (size_t i = 0; i < strRes.vResult.size(); i++)
                {
                    strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                }
                sExpr = parser_CreateStringVectorVar(strRes.vResult, mStringVectorVars);
                sLineToParsedTemp += sExpr;
                if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                    sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
                else
                    sLineToParsed.clear();

                continue;
            }
            else
                _parser.SetExpr(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos))));

            int nResults = 0;
            value_type* v = 0;
            vector<string> vResults;
            string sElement = "";
            v = _parser.Eval(nResults);

            for (int n = 0; n < nResults; n++)
            {
                if (fabs(rint(v[n])-v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                    sElement = toString((long long int)rint(v[n]));
                else
                    sElement = toString(v[n], _option);
                while (sElement.length() < sPrefix.length()+1)
                    sElement.insert(0, 1, '0');
                vResults.push_back(addQuotationMarks(sElement));
            }

            sElement = parser_CreateStringVectorVar(vResults, mStringVectorVars);

            sLineToParsedTemp += sElement;

            if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
            else
                sLineToParsed.clear();

            nPos = 0;
        }
        else
            nPos++;
    }

    if (sLineToParsed.length() && sLineToParsedTemp.length())
        sLineToParsedTemp += sLineToParsed;

    if (sLineToParsedTemp.find('{') != string::npos)
    {
        parser_VectorToExpr(sLineToParsedTemp, _option);
    }
    if (sLineToParsedTemp.length())
        sLineToParsed = sLineToParsedTemp;
    else
        sLineToParsed = sLine;
    return sLineToParsed;
}

vector<bool> parser_ApplyElementaryStringOperations(vector<string>& vFinal, Parser& _parser, const Settings& _option, bool& bReturningLogicals)
{
    vector<bool> vIsNoStringValue;
    for (unsigned int n = 0; n < vFinal.size(); n++)
    {
        //cerr << vFinal[n] << endl;
        StripSpaces(vFinal[n]);
        if (!vFinal[n].length())
            continue;
        // Strings verknüpfen
        if (vFinal[n].front() == '"' && vFinal[n].back() == '"')
        {
            for (unsigned int j = 0; j < vFinal[n].length(); j++)
            {
                if (vFinal[n][j] == '+' && !isInQuotes(vFinal[n], j))
                {
                    unsigned int k = j;
                    j = vFinal[n].rfind('"', j);
                    vFinal[n] = vFinal[n].substr(0,vFinal[n].rfind('"', k)) + vFinal[n].substr(vFinal[n].find('"', k)+1);
                }
            }
        }
        if (vFinal[n].find("&&") != string::npos
            || vFinal[n].find("||") != string::npos
            || vFinal[n].find("<=") != string::npos
            || vFinal[n].find(">=") != string::npos
            || vFinal[n].find("==") != string::npos
            || vFinal[n].find("!=") != string::npos
            || (vFinal[n].find('<') != string::npos
                && vFinal[n].find("<>") != vFinal[n].find('<')
                && vFinal[n].find("<this>") != vFinal[n].find('<')
                && vFinal[n].find("<wp>") != vFinal[n].find('<')
                && vFinal[n].find("<loadpath>") != vFinal[n].find('<')
                && vFinal[n].find("<savepath>") != vFinal[n].find('<')
                && vFinal[n].find("<plotpath>") != vFinal[n].find('<')
                && vFinal[n].find("<procpath>") != vFinal[n].find('<')
                && vFinal[n].find("<scriptpath>") != vFinal[n].find('<'))
            || (vFinal[n].find('>') != string::npos
                && vFinal[n].find("<>") != vFinal[n].find('>')-1
                && vFinal[n].find("<this>") != vFinal[n].find('>')-5
                && vFinal[n].find("<wp>") != vFinal[n].find('>')-5
                && vFinal[n].find("<loadpath>") != vFinal[n].find('>')-9
                && vFinal[n].find("<savepath>") != vFinal[n].find('>')-9
                && vFinal[n].find("<plotpath>") != vFinal[n].find('>')-9
                && vFinal[n].find("<procpath>") != vFinal[n].find('>')-9
                && vFinal[n].find("<scriptpath>") != vFinal[n].find('>')-11)
            )
        {
            vFinal[n] = addMaskedStrings(parser_evalStringLogic(removeMaskedStrings(vFinal[n]), bReturningLogicals));
            StripSpaces(vFinal[n]);
        }
        if (vFinal[n].front() != '"' && vFinal[n].back() != '"')
        {
            _parser.SetExpr(vFinal[n]);
            vFinal[n] = toString(_parser.Eval(), _option);
            vIsNoStringValue.push_back(true);
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

int parser_StoreStringResults(const vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string __sObject, Datafile& _data, Parser& _parser, const Settings& _option)
{
    if (__sObject.length())
    {
        if (__sObject.find('{') != string::npos)
            parser_VectorToExpr(__sObject, _option);
        string sObject;
        size_t nStrings = vFinal.size();
        size_t nCurrentComponent = 0;

        while (__sObject.length())
        {
            sObject = getNextArgument(__sObject, true);
            if (sObject.find("data(") != string::npos || _data.containsCacheElements(sObject))
            {
                string si = "";
                string sj = "";
                int nIndex[2] = {0,0};
                if (sObject.find("data(") != string::npos)
                    si = sObject.substr(sObject.find("data(")+4);
                else
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sObject.find(iter->first+"(") != string::npos
                            && (!sObject.find(iter->first+"(")
                                || (sObject.find(iter->first+"(") && checkDelimiter(sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                        {
                            si = sObject.substr(sObject.find(iter->first+"(")+(iter->first).length());
                            break;
                        }
                    }
                    _data.setCacheStatus(true);
                }
                parser_SplitArgs(si, sj, ',', _option);

                if (si.find('#') != string::npos)
                {
                    if (sj.find(':') != string::npos)
                    {
                        si = sj.substr(0,sj.find(':'));
                        sj = sj.substr(sj.find(':')+1);
                    }
                    else
                        si = sj;
                    if (parser_ExprNotEmpty(si))
                    {
                        _parser.SetExpr(si);
                        nIndex[0] = (int)_parser.Eval();
                        nIndex[0]--;
                    }
                    if (parser_ExprNotEmpty(sj))
                    {
                        _parser.SetExpr(sj);
                        nIndex[1] = (int)_parser.Eval();
                    }
                    if (sObject.find("data(") != string::npos)
                    {
                        if (!nIndex[1])
                            nIndex[1] = _data.getCols("data");
                        parser_CheckIndices(nIndex[0], nIndex[1]);
                        for (int n = nCurrentComponent; n < (int)nStrings; n++)
                        {
                            if (!vFinal[n].length() || n+nIndex[0] == nIndex[1]+1 || n+nIndex[0] >= _data.getCols("data"))
                                break;
                            _data.setHeadLineElement(n+nIndex[0], "data", removeControlSymbols(removeQuotationMarks(vFinal[n])));
                        }
                        nCurrentComponent = nStrings;
                    }
                    else
                    {
                        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                        {
                            if (sObject.find(iter->first+"(") != string::npos
                                && (!sObject.find(iter->first+"(")
                                    || (sObject.find(iter->first+"(") && checkDelimiter(sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                            {
                                if (nIndex[1])
                                    parser_CheckIndices(nIndex[0], nIndex[1]);

                                for (int n = nCurrentComponent; n < (int)nStrings; n++)
                                {
                                    if (!vFinal[n].length() || (nIndex[1] && n+nIndex[0] == nIndex[1]+1))
                                        break;
                                    _data.setHeadLineElement(n+nIndex[0], iter->first, removeControlSymbols(removeQuotationMarks(vFinal[n])));
                                }
                                nCurrentComponent = nStrings;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    Indices _idx = parser_getIndices(sObject, _parser, _data, _option);

                    string sTable;
                    if (sObject.find("data(") != string::npos)
                       throw READ_ONLY_DATA;
                    else
                    {
                        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                        {
                            if (sObject.find(iter->first+"(") != string::npos
                                && (!sObject.find(iter->first+"(")
                                    || (sObject.find(iter->first+"(") && checkDelimiter(sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                                sTable = iter->first;
                        }
                    }

                    if (_idx.nI[1] == -1)
                        _idx.nI[1] = _idx.nI[0];
                    if (_idx.nJ[1] == -1)
                        _idx.nJ[1] = _idx.nJ[0];

                    for (size_t i = nCurrentComponent; i < vFinal.size(); i++)
                    {
                        if (_idx.nI[0] == _idx.nI[1] && _idx.nJ[0] != _idx.nJ[1])
                        {
                            if (_idx.nJ[0] + i > _idx.nJ[1] && _idx.nJ[1] != -2)
                                break;
                            _data.writeToCache(_idx.nI[0], _idx.nJ[0]+1, sTable, StrToDb(vFinal[i]));
                        }
                        else if (_idx.nI[0] != _idx.nI[1] && _idx.nJ[0] == _idx.nJ[1])
                        {
                            if (_idx.nI[0] + i > _idx.nI[1] && _idx.nI[1] != -2)
                                break;
                            _data.writeToCache(_idx.nI[0]+i, _idx.nJ[0], sTable, StrToDb(vFinal[i]));
                        }
                        else if (_idx.nI[0] == _idx.nI[1] && _idx.nJ[0] == _idx.nJ[1])
                        {
                            _data.writeToCache(_idx.nI[0], _idx.nJ[0], sTable, StrToDb(vFinal[i]));
                            break;
                        }

                    }
                    nCurrentComponent = nStrings;
                    // throw CANNOT_CONTAIN_STRINGS;
                }
                _data.setCacheStatus(false);
            }
            else if (sObject.find("string(") != string::npos)
            {
                string si = "";
                string sj = "";
                string sCol = "";
                int nIndex[3] = {-2,-2,0};
                si = sObject.substr(sObject.find("string(")+6);
                si = si.substr(1,getMatchingParenthesis(si)-1);
                StripSpaces(si);

                if (si.length())
                {
                    if (si.find(':') != string::npos)
                    {
                        try
                        {
                            si = "(" + si + ")";
                            parser_SplitArgs(si, sj, ':', _option);
                            if (sj.find(',') != string::npos)
                            {
                                parser_SplitArgs(sj, sCol, ',', _option, true);
                            }
                            if (!parser_ExprNotEmpty(si))
                            {
                                si = "1";
                            }
                        }
                        catch (...)
                        {
                            throw;
                        }
                    }
                    else if (si.find(',') != string::npos)
                    {
                        try
                        {
                            parser_SplitArgs(si,sCol, ',', _option, true);
                            sj = si;
                        }
                        catch (...)
                        {
                            throw;
                        }
                    }
                    else
                        sj = si;

                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        nIndex[2] = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(si))
                    {
                        _parser.SetExpr(si);
                        nIndex[0] = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(sj))
                    {
                        _parser.SetExpr(sj);
                        nIndex[1] = (unsigned int)_parser.Eval()-1;
                    }
                    if (nIndex[0] < 0 && nIndex[1] < 0)
                        nIndex[0] = _data.getStringElements(nIndex[2]);
                    else if (nIndex[0] < 0)
                        nIndex[0] = 0;

                    if (nIndex[1] >= 0)
                        parser_CheckIndices(nIndex[0], nIndex[1]);

                    for (int n = nCurrentComponent; n < (int)nStrings; n++)
                    {
                        if (n+nIndex[0] == nIndex[1]+1)
                            break;
                        _data.writeString(removeQuotationMarks(vFinal[n]), n+nIndex[0], nIndex[2]);
                    }
                    nCurrentComponent = nStrings;
                }
                else
                {
                    for (int n = nCurrentComponent; n < (int)nStrings; n++)
                    {
                        _data.writeString(removeQuotationMarks(vFinal[n]));
                    }
                    nCurrentComponent = nStrings;
                }
            }
            else if (_data.containsStringVars(sObject))
            {
                StripSpaces(sObject);
                if (sObject.find(' ') != string::npos)
                    return 0;
                try
                {
                    _data.setStringValue(sObject, removeQuotationMarks(vFinal[nCurrentComponent]));
                    nCurrentComponent++;
                }
                catch (...)
                {
                    throw;
                }
            }
            else
            {
                StripSpaces(sObject);
                if (sObject.find(' ') != string::npos)
                {
                    return 0;
                }
                if (parser_GetVarAdress(sObject, _parser))
                {
                    if (!vIsNoStringValue[nCurrentComponent])
                    {
                        return 0;
                    }
                }
                if (vIsNoStringValue[nCurrentComponent])
                {
                    try
                    {
                        int nResults = 0;
                        value_type* v = 0;
                        _parser.SetExpr(sObject + " = " + vFinal[nCurrentComponent]);
                        v = _parser.Eval(nResults);
                        vAns = v[0];
                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
                else
                {
                    try
                    {
                        _data.setStringValue(sObject, removeQuotationMarks(vFinal[nCurrentComponent]));
                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
            }
        }
    }
    return 1;
}

string parser_CreateStringOutput(vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string& sLine, bool bNoQuotes, bool bReturningLogicals, bool bKeepMaskedQuotes)
{
    sLine.clear();

    // remove the quotation marks
    for (size_t i = 0; i < vFinal.size(); i++)
        vFinal[i] = removeQuotationMarks(vFinal[i]);

    string sConsoleOut = "|-> ";
    for (unsigned int j = 0; j < vFinal.size(); j++)
    {
        if (bKeepMaskedQuotes)
        {
            if (!bNoQuotes && !vIsNoStringValue[j])
                sLine += "\"" + vFinal[j] + "\"";
            else
                sLine += vFinal[j];
            if (j < vFinal.size() - 1)
                sLine += ",";
            continue;
        }
        if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !bNoQuotes && !bReturningLogicals && !vIsNoStringValue[j])
        {
            sConsoleOut += "\"";
            sLine += "\"";
        }
        for (unsigned int k = 0; k < vFinal[j].length(); k++)
        {
            if (k+1 < vFinal[j].length()
                && vFinal[j][k] == '\\'
                && (vFinal[j][k+1] == 'n' || vFinal[j][k+1] == 't' || vFinal[j][k+1] == '"')
                && !(vFinal[j].substr(k+1,3) == "tau"
                    && ((checkDelimiter(vFinal[j].substr(k,5)) && vFinal[j].length() >= k+5) || (vFinal[j].length() == k+4)))
                && !(vFinal[j].substr(k+1,5) == "theta"
                    && ((checkDelimiter(vFinal[j].substr(k,7)) && vFinal[j].length() >= k+7) || (vFinal[j].length() == k+6)))
                && !(vFinal[j].substr(k+1,2) == "nu"
                    && ((checkDelimiter(vFinal[j].substr(k,4)) && vFinal[j].length() >= k+4) || (vFinal[j].length() == k+3)))
                && !(vFinal[j].substr(k+1,3) == "neq"
                    && ((checkDelimiter(vFinal[j].substr(k,5)) && vFinal[j].length() >= k+5) || (vFinal[j].length() == k+4)))
            )
            {//\not\neq\ni
                if (vFinal[j][k+1] == 'n')
                {
                    sConsoleOut += "$";
                    sLine += "\n";
                }
                else if (vFinal[j][k+1] == 't')
                {
                    sConsoleOut += "\t";
                    sLine += "\t";
                }
                else if (vFinal[j][k+1] == '"')
                {
                    sConsoleOut += "\"";
                    if (!bKeepMaskedQuotes)
                        sLine += "\"";
                    else
                        sLine += "\\\"";
                }
                k += 1;
            }
            else
            {
                sConsoleOut += vFinal[j][k];
                sLine += vFinal[j][k];
            }
        }
        if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !bNoQuotes && !bReturningLogicals && !vIsNoStringValue[j])
        {
            sConsoleOut += "\"";
            sLine += "\"";
        }
        if (j == vFinal.size() - 1)
            break;

        if (vFinal[j] != "\\n" && vFinal[j+1] != "\\n" && vFinal[j] != "\\t" && vFinal[j] != "\\t")
        {
            if (sLine.find_last_not_of("\" ") != string::npos && sLine[sLine.find_last_not_of("\" ")] == '\n')
            {
                sLine += ", ";
                if (sConsoleOut.back() == '"')
                {
                    sConsoleOut[sConsoleOut.rfind('$')] = '"';
                    sConsoleOut.back() = '$';
                }
                continue;
            }
            sConsoleOut += ", ";
            sLine += ", ";
        }
    }
    return sConsoleOut;
}












int parser_countStrings(const string& sCmd)
{
    if (!sCmd.length())
        return 0;
    if (sCmd.find(',') == string::npos)
        return 1;
    int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    return nCount;
}

int parser_numStrings(const string& sCmd)
{
    if (!sCmd.length() || sCmd == "\"\"")
        return 0;
    if (sCmd != "\"\"" && sCmd.find(',') == string::npos)
        return 1;
    int nCount = 0;
    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if ((sCmd[i] == ',' && !isInQuotes(sCmd, i)) || i+1 == sCmd.length())
        {
            for (int j = i; j >= 0; j--)
            {
                if (j > 0 && sCmd[j] == '"' && sCmd[j-1] == '"')
                    break;
                else if (j > 0 && sCmd[j] == '"' && sCmd[j-1] != '"')
                {
                    nCount++;
                    break;
                }
            }
        }
    }
    return nCount;
}

string parser_getMinString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sMin = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    sMin = sStrings[0];
    for (unsigned int i = 1; i < nCount; i++)
    {
        if (sMin > sStrings[i])
            sMin = sStrings[i];
    }
    sMin = "\"" + sMin + "\"";
    delete[] sStrings;
    return sMin;
}

string parser_getMaxString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sMax = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    sMax = sStrings[0];
    for (unsigned int i = 1; i < nCount; i++)
    {
        if (sMax < sStrings[i])
            sMax = sStrings[i];
    }
    sMax = "\"" + sMax + "\"";
    delete[] sStrings;
    return sMax;
}

string parser_getSumString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sSum = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    for (unsigned int i = 0; i < nCount; i++)
    {
        sSum += sStrings[i];
    }
    sSum = "\"" + sSum + "\"";
    delete[] sStrings;
    return sSum;
}


