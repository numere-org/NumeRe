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
        StringArg()
        { }

        StringArg(const std::string& sStr) : m_data(sStr)
        { }

        StringArg(const StringArg& sStr) : m_data(sStr.m_data)
        { }

        StringArg& operator=(const StringArg& sStr)
        {
            m_data = sStr.m_data;
            return *this;
        }

        StringArg& operator=(const std::string& sStr)
        {
            m_data = sStr;
            return *this;
        }

        bool is_string() const
        {
            return m_data.length() && m_data.front() == '"';
        }

        StringView view() const
        {
            if (is_string())
                return StringView(m_data, 1, m_data.length()-2);

            return StringView(m_data);
        }

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

        static StringView makePureString(StringView sStr)
        {
            if (sStr.length() && sStr.front() == '"')
                return sStr.subview(1, sStr.length()-2);

            return sStr;
        }

        static std::string makeLocalString(const std::string& sStr)
        {
            if (sStr.front() == '"')
                return "\"" + toInternalString(sStr) + "\"";

            return sStr;
        }

        StringView getVectorized(size_t i) const
        {
            if (size() == 1)
                return makePureString(front());
            else if (i < size())
                return makePureString(at(i));

            return StringView(m_sDUMMY);
        }

        void assign(const StringVector& sVect)
        {
            std::vector<std::string>::assign(sVect.begin(), sVect.end());
        }

        void assign(const std::vector<bool>& vect)
        {
            resize(vect.size());

            for (size_t i = 0; i < vect.size(); i++)
            {
                std::vector<std::string>::operator[](i) = vect[i] ? "true" : "false";
            }
        }

    public:
        StringVector() : std::vector<std::string>()
        { }

        StringVector(const std::vector<std::string>& vect) : std::vector<std::string>(vect)
        { }

        StringVector(const std::string& sLiteral) : std::vector<std::string>(1, makeLocalString(sLiteral))
        { }

        StringVector(const StringVector& vect) : std::vector<std::string>()
        {
            assign(vect);
        }

        StringVector(const std::vector<bool>& vect) : std::vector<std::string>()
        {
            assign(vect);
        }

        StringVector(size_t n, const std::string& sStr = std::string()) : std::vector<std::string>(n, sStr)
        { }

        StringVector& operator=(const StringVector& sVect)
        {
            assign(sVect);
            return *this;
        }

        StringVector& operator=(const std::vector<bool>& vect)
        {
            assign(vect);
            return *this;
        }

        void push_back(const std::string& sLiteral)
        {
            std::vector<std::string>::push_back(makeLocalString(sLiteral));
        }

        bool is_string(size_t i) const
        {
            if (i < size())
                return at(i).front() == '"';
            else if (size() == 1)
                return front().front() == '"';

            return false;
        }

        void convert_to_string(size_t i)
        {
            if (i < size() && !is_string(i))
                std::vector<std::string>::operator[](i) = "\"" + std::vector<std::string>::operator[](i) + "\"";
        }

        StringView operator[](size_t i) const
        {
            if (i < size())
                return makePureString(at(i));

            return StringView(m_sDUMMY);
        }

        StringArg getArg(size_t i) const
        {
            if (i < size())
                return StringArg(at(i));
            else if (size() == 1)
                return StringArg(at(0));

            return StringArg();
        }

        std::string getMasked(size_t i) const
        {
            if (i < size() && is_string(i))
                return toExternalString(at(i));
            else if (i < size())
                return at(i);

            return m_sDUMMY;
        }

        std::string& getRef(size_t i)
        {
            if (i < size())
                return std::vector<std::string>::operator[](i);

            throw std::out_of_range("Requested element " + toString(i) + ", which is greater than this->size().");
        }

        std::vector<bool> operator==(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) == sVect.getVectorized(i);
            }

            return vRet;
        }

        std::vector<bool> operator!=(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) != sVect.getVectorized(i);
            }

            return vRet;
        }

        std::vector<bool> operator<(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) < sVect.getVectorized(i);
            }

            return vRet;
        }

        std::vector<bool> operator<=(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) <= sVect.getVectorized(i);
            }

            return vRet;
        }

        std::vector<bool> operator>(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) > sVect.getVectorized(i);
            }

            return vRet;
        }

        std::vector<bool> operator>=(const StringVector& sVect) const
        {
            std::vector<bool> vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                vRet[i] = getVectorized(i) >= sVect.getVectorized(i);
            }

            return vRet;
        }

        StringVector operator+(const StringVector& sVect) const
        {
            StringVector vRet(std::max(size(), sVect.size()));

            for (size_t i = 0; i < vRet.size(); i++)
            {
                if (is_string(i) && sVect.is_string(i))
                    vRet[i] = "\"" + (getVectorized(i) + sVect.getVectorized(i)) + "\"";
                else
                    vRet[i] = getVectorized(i) + std::string(" + ") + sVect.getVectorized(i);
            }

            return vRet;
        }

        StringVector& operator+=(const StringVector& sVect)
        {
            assign(operator+(sVect));
            return *this;
        }

        StringVector operator+(const std::string& sLiteral) const
        {
            return operator+(StringVector(sLiteral));
        }

        StringVector& operator+=(const std::string& sLiteral)
        {
            assign(operator+(StringVector(sLiteral)));
            return *this;
        }
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
typedef std::string (*StringFunc)(StringFuncArgs&);


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
		vResult.push_back(sRet);
		vNoStringVal.push_back(sRet.find('"') == std::string::npos);
		bOnlyLogicals = _bOnlyLogicals;
	}

	StringResult(const std::string& sRet, mu::value_type* vals, int nvals) : StringResult()
	{
	    bOnlyLogicals = true;
	    vResult.push_back(sRet);
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

