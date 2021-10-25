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

#include "../structures.hpp"
#include "../ui/error.hpp"
#include "../settings.hpp"
#include "stringtools.hpp"

extern const string sVersion;
using namespace std;

long long int intCast(double number);
long long int intCast(const std::complex<double>& number);

int findParameter(const string& sCmd, const string& sParam, const char cFollowing = ' ');
bool getStringArgument(const string& sCmd, string& sArgument);
string extractStringToken(const string& sCmd, size_t nPos);
unsigned int getMatchingParenthesis(const StringView&);
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis = false);
string replaceToTeX(const string& sString, bool replaceForTeXFile = false);
Match findCommand(const string& sCmd, string sCommand = "");
string extractCommandString(const string& sCmd, const Match& _mMatch);
void removeArgSep(string&);
void openExternally(const string&);
void moveFile(const string&, const string&);
void copyFile(const string&, const string&);
void writeTeXMain(const string&);
string addControlSymbols(const string&);

enum ArgExtraction
{
    ARGEXTRACT_NONE = 0x0,
    ARGEXTRACT_STRIPPED = 0x1,
    ARGEXTRACT_PARSED = 0x2,
    ARGEXTRACT_ASINT = 0x4,
    ARGEXTRACT_ASSTRING = 0x8
};

string getArgAtPos(const string& sCmd, unsigned int nPos, int extraction = ARGEXTRACT_STRIPPED);
bool isInQuotes(StringView sExpr, unsigned int nPos, bool bIgnoreVarParser = false);
bool isToStringArg(const string& sExpr, unsigned int nPos);
bool isDelimiter(char cChar);
bool addLegends(string&);
bool checkDelimiter(const string& sToken, bool stringdelim = false);
std::vector<std::string> splitIntoLines(std::string sOutput, size_t lineWidth, bool bAllowDashBreaks = true, int nFirstIndent = 4, int nIndent = 4);
std::string outputString(std::vector<std::string> stringInLines, int nFirstIndent, int nIndent);
string LineBreak(string sOutput, const Settings& _option, bool bAllowDashBreaks = true, int nFirstIndent = 4, int nIndent = 4);
double Linearize(double x_0, double y_0, double x_1, double y_1);
void make_hline(int nLength = -1);
void make_progressBar(int nStep, int nFirstStep = 1, int nFinalStep = 100, const string& sType = "std");
bool containsStrings(const string& sLine);
bool fileExists(const string& sFilename);
void reduceLogFilesize(const string& sFileName);
string replaceToVectorname(const string& sExpression);
void eraseToken(string& sExpr, const string& sToken, bool bTokenHasValue = false);
string generateCacheName(const string& sFilename, Settings& _option) __attribute__ ((deprecated));
string getFileInfo(const string& sFilename);
string decodeNameSpace(string sCommandLine, const string& sThisNameSpace);

// This inline function checks, if the passed value is finite and
// not a NAN value
inline bool isValidValue(double d)
{
    return !isnan(d) && !isinf(d);
}

/** \brief Checks, whether the number of parentheses is an even number
 *
 * \param sCmd const string&
 * \return bool
 *
 * Doesn't check, whether the parentheses are ordered reasonable
 *
 */
bool validateParenthesisNumber(const string& sCmd);

void addArgumentQuotes(string& sToAdd, const string& sParam);
void addArgumentQuotes(string& sToAdd, size_t pos);

/** \brief Calculates the power of a number using an integer as exponent
 *
 * \param dNumber double
 * \param nExponent int
 * \return double
 *
 */
double intPower(double dNumber, int nExponent);
std::complex<double> intPower(const std::complex<double>& dNumber, int nExponent);

string getNextArgument(string& sArgList, bool bCut = true);
string getNextIndex(string& sArgList, bool bCut = true);
string getNextSemiColonSeparatedToken(string& sArgList, bool bCut = true);
StringView getNextViewedArgument(StringView& sView);
StringView getNextViewedIndex(StringView& sView);
EndlessVector<StringView> getAllArguments(StringView sArgList);
EndlessVector<string> getAllArguments(string sArgList);
EndlessVector<string> getAllIndices(string sArgList);
EndlessVector<string> getAllSemiColonSeparatedTokens(string sArgList);


/** \brief Checks, whether the "to_cmd()" function was used
 *
 * \param sCmd const string&
 * \param nPos unsigned int
 * \return bool
 *
 */
bool isToCmd(const string& sCmd, unsigned int nPos);

unsigned int countEscapeSymbols(const string& sLine);
vector<string> getFileList(const string& sDirectory, const Settings& _option, int nFlags = 0);
vector<string> getFolderList(const string& sDirectory, const Settings& _option, int nFlags = 0);
string getClipboardText();

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

void evalRecursiveExpressions(string& sExpr);

size_t qSortDouble(double* dArray, size_t nlength);
void replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sReplacement);
std::string shortenFileName(const std::string& sFullFileName);
std::string incrementVersion(std::string _sVer);

#endif

