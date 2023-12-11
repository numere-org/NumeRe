/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include <cstdlib>
#include <vector>
#include <random>
#include <gsl/gsl_rng.h>

#include "../structures.hpp"
#include "../ui/error.hpp"
#include "../settings.hpp"
#include "stringtools.hpp"

extern const std::string sVersion;

/////////////////////////////////////////////////
/// \brief Casts doubles to integers and avoids
/// rounding errors.
///
/// \param number double
/// \return INT
///
/////////////////////////////////////////////////
template <class INT>
INT genericIntCast(double number)
{
    // if quite close, use rint
    if (std::abs(number - rint(number)) < 1e-7)
        return std::rint(number);

    // otherwise truncate
    return static_cast<INT>(number);
}

/////////////////////////////////////////////////
/// \brief Casts the real part of the complex
/// number to an integer and avoids rounding
/// errors.
///
/// \param number const std::complex<double>&
/// \return INT
///
/////////////////////////////////////////////////
template <class INT>
INT genericIntCast(const std::complex<double>& number)
{
    return genericIntCast<INT>(number.real());
}

inline int64_t intCast(double number)
{
    return genericIntCast<int64_t>(number);
}

inline int64_t intCast(const std::complex<double>& number)
{
    return genericIntCast<int64_t>(number.real());
}

bool isInt(const std::complex<double>& number);

std::mt19937& getRandGenInstance();
const gsl_rng* getGslRandGenInstance();
int findParameter(const std::string& sCmd, const std::string& sParam, const char cFollowing = ' ');
bool getStringArgument(const std::string& sCmd, std::string& sArgument);
std::string extractStringToken(const std::string& sCmd, size_t nPos);
size_t getMatchingParenthesis(const StringView&);
bool isMultiValue(const std::string& sExpr, bool bIgnoreClosingParenthesis = false);
std::string replaceToTeX(const std::string& sString, bool replaceForTeXFile = false);
Match findCommand(StringView sCmd, const std::string& sCommand = "");
std::string extractCommandString(const std::string& sCmd, const Match& _mMatch);
void openExternally(const std::string&);
void moveFile(const std::string&, const std::string&);
void copyFile(const std::string&, const std::string&);
void writeTeXMain(const std::string&);

enum ArgExtraction
{
    ARGEXTRACT_NONE = 0x0,
    ARGEXTRACT_STRIPPED = 0x1,
    ARGEXTRACT_PARSED = 0x2,
    ARGEXTRACT_ASINT = 0x4,
    ARGEXTRACT_ASSTRING = 0x8
};

std::string getArgAtPos(const std::string& sCmd, size_t nPos, int extraction = ARGEXTRACT_STRIPPED);
bool isInQuotes(StringView sExpr, size_t nPos, bool bIgnoreVarParser = false);
bool isToStringArg(const std::string& sExpr, size_t nPos);
bool isDelimiter(char cChar);
bool addLegends(std::string&);
bool checkDelimiter(StringView sToken, bool stringdelim = false);
std::vector<std::string> splitIntoLines(std::string sOutput, size_t lineWidth, bool bAllowDashBreaks = true, int nFirstIndent = 4, int nIndent = 4);
std::string outputString(std::vector<std::string> stringInLines, int nFirstIndent, int nIndent);
std::string LineBreak(std::string sOutput, const Settings& _option, bool bAllowDashBreaks = true, int nFirstIndent = 4, int nIndent = 4);
double Linearize(double x_0, double y_0, double x_1, double y_1);
void make_hline(int nLength = -1);
void make_progressBar(int nStep, int nFirstStep = 1, int nFinalStep = 100, const std::string& sType = "std");
bool containsStrings(const std::string& sLine);
bool fileExists(const std::string& sFilename);
void reduceLogFilesize(const std::string& sFileName);
std::string replaceToVectorname(const std::string& sExpression);
void eraseToken(std::string& sExpr, const std::string& sToken, bool bTokenHasValue = false);
std::string getFileInfo(const std::string& sFilename);
std::string decodeNameSpace(std::string sCommandLine, const std::string& sThisNameSpace);

// This inline function checks, if the passed value is finite and
// not a NAN value
inline bool isValidValue(double d)
{
    return !std::isnan(d) && !std::isinf(d);
}

/** \brief Checks, whether the number of parentheses is an even number
 *
 * \param sCmd StringView
 * \return bool
 *
 * Doesn't check, whether the parentheses are ordered reasonable
 *
 */
bool validateParenthesisNumber(StringView sCmd);

void addArgumentQuotes(std::string& sToAdd, const std::string& sParam);
void addArgumentQuotes(std::string& sToAdd, size_t pos);

/** \brief Calculates the power of a number using an integer as exponent
 *
 * \param dNumber double
 * \param nExponent int
 * \return double
 *
 */
double intPower(double dNumber, int nExponent);
std::complex<double> intPower(const std::complex<double>& dNumber, int nExponent);

std::string getNextArgument(std::string& sArgList, bool bCut = true);
std::string getNextIndex(std::string& sArgList, bool bCut = true);
std::string getNextSemiColonSeparatedToken(std::string& sArgList, bool bCut = true);
StringView getNextViewedArgument(StringView& sView);
StringView getNextViewedIndex(StringView& sView);
EndlessVector<StringView> getAllArguments(StringView sArgList);
EndlessVector<std::string> getAllArguments(std::string sArgList);
EndlessVector<std::string> getAllIndices(std::string sArgList);
EndlessVector<std::string> getAllSemiColonSeparatedTokens(std::string sArgList);


/** \brief Checks, whether the "to_cmd()" function was used
 *
 * \param sCmd StringView
 * \param nPos size_t
 * \return bool
 *
 */
bool isToCmd(StringView sCmd, size_t nPos);

size_t countEscapeSymbols(const std::string& sLine);
std::vector<std::string> getFileList(const std::string& sDirectory, int nFlags = 0);
std::vector<std::string> getFolderList(const std::string& sDirectory, int nFlags = 0);

namespace little_endian_io
{
	template <typename Word>
	inline std::ostream& write_word( std::ostream& outs, Word value, unsigned size = sizeof( Word ) )
	{
		for (; size; --size, value >>= 8)
			outs.put( static_cast <char> (value & 0xFF) );
		return outs;
	}

	template <typename Word>
	inline std::istream& read_word( std::istream& ins, Word& value, unsigned size = sizeof( Word ) )
	{
		for (unsigned n = 0, value = 0; n < size; ++n)
			value |= ins.get() << (8 * n);
		return ins;
	}
}

namespace big_endian_io
{
	template <typename Word>
	inline std::ostream& write_word( std::ostream& outs, Word value, unsigned size = sizeof( Word ) )
	{
		while (size)
			outs.put( static_cast <char> ( (value >> (8 * --size)) & 0xFF ));
		return outs;
	}

	template <typename Word>
	inline std::istream& read_word( std::istream& ins, Word& value, unsigned size = sizeof( Word ) )
	{
		for (value = 0; size; --size)
			value = (value << 8) | ins.get();
		return ins;
	}
}

void evalRecursiveExpressions(std::string& sExpr);

size_t qSortDouble(double* dArray, size_t nlength);
void replaceStringMethod(MutableStringView sLine, size_t nPos, size_t nLength, const std::string& sReplacement);
std::string shortenFileName(const std::string& sFullFileName);
std::string removeDefaultPath(const std::string& sFullPath);
std::string incrementVersion(std::string _sVer);

inline bool isDimensionVar(const std::string& sVarName)
{
    return sVarName == "nrows" || sVarName == "ncols" || sVarName == "nlen" || sVarName == "nlines";
}


#endif

