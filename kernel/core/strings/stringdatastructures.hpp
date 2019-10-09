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

#ifndef STRINGDATASTRUCTURES_HPP
#define STRINGDATASTRUCTURES_HPP

#include <string>
#include <vector>

using namespace std;
class Settings;

/////////////////////////////////////////////////
/// \brief Simple abbreviation
/////////////////////////////////////////////////
typedef std::vector<std::string> s_vect;

/////////////////////////////////////////////////
/// \brief Simple abbreviation
/////////////////////////////////////////////////
typedef std::vector<long long int> n_vect;


/////////////////////////////////////////////////
/// \brief This structure combines all string
/// function's arguments into a single structure
/// to align all string function's signatures.
/////////////////////////////////////////////////
struct StringFuncArgs
{
	string sArg1, sArg2, sArg3;
	s_vect sMultiArg;
	long long int nArg1, nArg2;
	n_vect nMultiArg;
	const Settings* opt;
};


/////////////////////////////////////////////////
/// \brief Defines the pointer to an arbitrary
/// string function as StringFunc.
/////////////////////////////////////////////////
typedef std::string (*StringFunc)(StringFuncArgs&);


/////////////////////////////////////////////////
/// \brief This enumeration contains all possible
/// string function signature types (as they are
/// called from the user, not internally).
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This structure defines the internal
/// string function signature. It contains the
/// pointer to the actual function, its signature
/// type and whether the function is capable of
/// handling multiple arguments.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This structure contains all possible
/// return values of the central string parser in
/// single combined structure.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Defines the possible flags, which the
/// user might pass to the string parser.
/////////////////////////////////////////////////
enum StringParserFlags
{
	NO_FLAG = 0,
	NO_QUOTES = 1,
	PEEK = 2,
	KEEP_MASKED_QUOTES = 4,
	KEEP_MASKED_CONTROL_CHARS = 8
};


/////////////////////////////////////////////////
/// \brief Structure containing the german umlauts. The
/// lower field will contain lower case umlauts,
/// upper field contains the upper case umlauts.
/////////////////////////////////////////////////
struct Umlauts
{
    string lower;
    string upper;

    // Constructor fills the fields with the corresponding
    // character codes (eg \x94 is a Hex value for (char)148)
    Umlauts() : lower("äüöß\x84\x94\x81"), upper("ÄÖÜ\x8E\x99\x9A") {}
};

#endif // STRINGDATASTRUCTURES_HPP

