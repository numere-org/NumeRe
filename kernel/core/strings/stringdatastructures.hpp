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

#include "../ParserLib/muParserDef.h"
#include "../utils/stringtools.hpp"
#include "../structures.hpp"

class Settings;



/////////////////////////////////////////////////
/// \brief Simply container to provide the data
/// for a StringView instance usable by all the
/// string functions.
/////////////////////////////////////////////////
class StringArg
{
    private:
        std::string m_data;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor
        /////////////////////////////////////////////////
        StringArg()
        { }

        /////////////////////////////////////////////////
        /// \brief Construct a StringArg instance from a
        /// std::string instance.
        ///
        /// \param sStr const std::string
        ///
        /////////////////////////////////////////////////
        StringArg(const std::string& sStr) : m_data(sStr)
        { }

        /////////////////////////////////////////////////
        /// \brief Copy a StringArg instance.
        ///
        /// \param sStr const StringArg&
        ///
        /////////////////////////////////////////////////
        StringArg(const StringArg& sStr) : m_data(sStr.m_data)
        { }

        /////////////////////////////////////////////////
        /// \brief StringArg assignment operator
        /// overload.
        ///
        /// \param sStr const StringArg&
        /// \return StringArg&
        ///
        /////////////////////////////////////////////////
        StringArg& operator=(const StringArg& sStr)
        {
            m_data = sStr.m_data;
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief StringArg assignment operator overload
        /// for a std::string instance.
        ///
        /// \param sStr const std::string&
        /// \return StringArg&
        ///
        /////////////////////////////////////////////////
        StringArg& operator=(const std::string& sStr)
        {
            m_data = sStr;
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Determine, whether the contained
        /// string represents a string literal.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool is_string() const
        {
            return m_data.length() && m_data.front() == '"';
        }

        /////////////////////////////////////////////////
        /// \brief Get a view to the contained string.
        ///
        /// \return StringView
        ///
        /////////////////////////////////////////////////
        StringView view() const
        {
            if (is_string())
                return StringView(m_data, 1, m_data.length()-2);

            return StringView(m_data);
        }

        /////////////////////////////////////////////////
        /// \brief Get a reference to the contained
        /// string.
        ///
        /// \return std::string&
        ///
        /////////////////////////////////////////////////
        std::string& getRef()
        {
            return m_data;
        }
};


/////////////////////////////////////////////////
/// \brief This class is an extension to the
/// std::vector<std::string> to provide the
/// vector-like functionalities as requested by
/// the string parser. It also provides the
/// functionality to distinguish between a string
/// and a numerical value.
/////////////////////////////////////////////////
class StringVector : public std::vector<std::string>
{
    private:
        const std::string m_sDUMMY;

        static StringView makePureString(StringView sStr);
        static std::string makeLocalString(const std::string& sStr);
        StringView getVectorized(size_t i) const;
        mu::value_type getNumericalVectorized(size_t i) const;
        bool getBooleanVectorized(size_t i) const;
        void assign(const StringVector& sVect);
        void assign(const std::vector<bool>& vect);
        StringVector operator+(const std::string& sLiteral) const;
        StringVector& operator+=(const std::string& sLiteral);

    public:
        StringVector();
        StringVector(const std::string& sStr);
        StringVector(const char* sStr);
        StringVector(bool val);
        StringVector(const std::vector<std::string>& vect);
        StringVector(const StringVector& vect);
        StringVector(const std::vector<bool>& vect);
        StringVector(size_t n, const std::string& sStr = std::string());
        StringVector(StringVector&& vect) = default;
        static StringVector empty_string();
        static StringVector convert_internal(const std::string& sInternal);
        static StringVector convert_literal(const std::string& sLiteral);
        StringVector& operator=(const StringVector& sVect);
        StringVector& operator=(StringVector&& sVect) = default;
        StringVector& operator=(const std::vector<bool>& vect);
        void push_back(const std::string& sStr);
        void push_back(const char* sStr);
        void push_back(const mu::value_type& vVal);
        void push_back(size_t nVal);
        void push_back(int nVal);
        void push_back(long long int nVal);
        void push_back(bool nVal);
        void push_generic(const std::string& sStr);
        bool is_string(size_t i) const;
        void convert_to_string(size_t i);
        StringView operator[](size_t i) const;
        StringArg getArg(size_t i) const;
        std::string getMasked(size_t i) const;
        std::string& getRef(size_t i);
        std::vector<bool> operator==(const StringVector& sVect) const;
        std::vector<bool> operator!=(const StringVector& sVect) const;
        std::vector<bool> operator<(const StringVector& sVect) const;
        std::vector<bool> operator<=(const StringVector& sVect) const;
        std::vector<bool> operator>(const StringVector& sVect) const;
        std::vector<bool> operator>=(const StringVector& sVect) const;
        std::vector<bool> and_f(const StringVector& sVect) const;
        std::vector<bool> or_f(const StringVector& sVect) const;
        std::vector<bool> xor_f(const StringVector& sVect) const;
        StringVector operator+(const StringVector& sVect) const;
        StringVector& operator+=(const StringVector& sVect);
        StringVector& evalIfElse(const StringVector& sLogicals, const StringVector& sIfBranch, const StringVector& sElseBranch);
};


/////////////////////////////////////////////////
/// \brief A simple container for a single item
/// in the string expression RPN stack.
/////////////////////////////////////////////////
struct StringStackItem
{
    StringView m_data;
    int m_val;

    StringStackItem(StringView sView, int val = -1) : m_data(sView), m_val(val) {}
};


/////////////////////////////////////////////////
/// \brief Simple abbreviation
/////////////////////////////////////////////////
typedef StringVector s_vect;

/////////////////////////////////////////////////
/// \brief Simple abbreviation
/////////////////////////////////////////////////
typedef std::vector<long long int> n_vect;

/////////////////////////////////////////////////
/// \brief Simple abbreviation
/////////////////////////////////////////////////
typedef std::vector<mu::value_type> d_vect;


/////////////////////////////////////////////////
/// \brief This structure combines all string
/// function's arguments into a single structure
/// to align all string function's signatures.
/////////////////////////////////////////////////
struct StringFuncArgs
{
	StringArg sArg1, sArg2, sArg3;
	s_vect sMultiArg;
	long long int nArg1, nArg2;
	mu::value_type dArg1;
	n_vect nMultiArg;
	const Settings* opt;

	StringFuncArgs() : nArg1(INT_MIN), nArg2(INT_MIN), dArg1(0.0), opt(nullptr) {}
};


/////////////////////////////////////////////////
/// \brief Defines the pointer to an arbitrary
/// string function as StringFunc.
/////////////////////////////////////////////////
typedef StringVector (*StringFunc)(StringFuncArgs&);


/////////////////////////////////////////////////
/// \brief This enumeration contains all possible
/// string function signature types (as they are
/// called from the user, not internally).
/////////////////////////////////////////////////
enum FunctionSignatureType
{
    NOARGS,

	PARSER_INT,
	VAL,

	PARSER_STRING,
	STR,

	PARSER_STRING_DOUBLE,
	STR_DBL,

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
		fType = PARSER_INT;
		fHandle = nullptr;
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

	StringResult(s_vect& _vResult, std::vector<bool>& _vNoStringVal, bool _bOnlyLogicals) : vResult(_vResult), vNoStringVal(_vNoStringVal), bOnlyLogicals(_bOnlyLogicals) {}

	StringResult(const std::string& sRet, bool _bOnlyLogicals = false) : StringResult()
	{
		vResult.push_generic(sRet);
		vNoStringVal.push_back(sRet.find('"') == std::string::npos);
		bOnlyLogicals = _bOnlyLogicals;
	}

	StringResult(const std::string& sRet, mu::value_type* vals, int nvals) : StringResult()
	{
	    bOnlyLogicals = true;
	    vResult.push_generic(sRet);
        vNoStringVal.resize(nvals, true);
        vNumericalValues.assign(vals, vals+nvals);
	}

	s_vect vResult;
	std::vector<bool> vNoStringVal;
	std::vector<mu::value_type> vNumericalValues;
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
    std::string lower;
    std::string upper;

    // Constructor fills the fields with the corresponding
    // character codes (eg \x94 is a Hex value for (char)148)
    Umlauts() : lower("äüöß\x84\x94\x81"), upper("ÄÖÜ\x8E\x99\x9A") {}
};

#endif // STRINGDATASTRUCTURES_HPP

