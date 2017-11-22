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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <windows.h>
#include <conio.h>
#include <cstdlib>
#include <fstream>
#include <ctime>
#include <vector>

#include "structures.hpp"
#include "error.hpp"
#include "settings.hpp"

extern const string sVersion;
extern int nLINE_LENGTH;
using namespace std;

/** \brief Macht aus einem Int ein String (mit Praezision)
 *
 * \param nNumber int
 * \param _option const Settings&
 * \return string
 *
 */
string toString(int nNumber, const Settings& _option);

/** \brief Macht aus einem Double ein String (Wobei die Praezision beachtet wird)
 *
 * \param dNumber double
 * \param _option const Settings&
 * \return string
 *
 */
string toString(double dNumber, const Settings& _option);

/** \brief Macht aus einem Double einen String mit beliebiger Preazision
 *
 * \param dNumber double
 * \param nPrecision int
 * \return string
 *
 */
string toString(double dNumber, int nPrecision);

/** \brief Transforms an integer into a string
 *
 * \param int
 * \return string
 *
 */
string toString(int);

/** \brief Transforms a time_t into a string (formatted as time)
 *
 * \param tTime time_t
 * \param onlyTime bool
 * \return string
 *
 */
string toString(time_t tTime, bool bOnlyTime = false);

/** \brief Macht aus einem Int ein String (ohne Praezision)
 *
 * \param nNumber long longint
 * \return string
 *
 */
string toString(long long int nNumber);

/** \brief Macht aus einem Double einen String mit 20 Ziffern
 *
 * \param dNumber double
 * \return string
 *
 */
string toCmdString(double dNumber);

/** \brief Macht aus einem bool einen String mit "true" oder "false"
 *
 * \param bBoolean bool
 * \return string
 *
 */
string toString(bool bBoolean);

/** \brief Removes vowels and umlauts from strings
 *
 * \param sText const string&
 * \return string
 *
 */
string condenseText(const string& sText);
string wcstombs(const wstring& wStr);
void StripSpaces(string&);
int matchParams(const string& sCmd, const string& sParam, const char cFollowing = ' ');
bool getStringArgument(const string& sCmd, string& sArgument);
bool getIntArgument(const string& sCmd, int& nArgument);
int StrToInt(const string&);
double StrToDb(const string&);
unsigned int getMatchingParenthesis(const string&);
string toLowerCase(const string& sUpperCase);
string toUpperCase(const string& sLowerCase);
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis = false);
string replaceToTeX(const string& sString, bool replaceForTeXFile = false);
Match findCommand(const string& sCmd, string sCommand = "");
string extractCommandString(const string& sCmd, const Match& _mMatch);
void removeArgSep(string&);
void openExternally(const string&, const string&, const string&);
void moveFile(const string&, const string&);
void writeTeXMain(const string&);
string removeControlSymbols(const string&);
string addControlSymbols(const string&);
string getArgAtPos(const string& sCmd, unsigned int nPos);
bool isInQuotes(const string& sExpr, unsigned int nPos, bool bIgnoreVarParser = false);
bool isToStringArg(const string& sExpr, unsigned int nPos);
bool isDelimiter(char cChar);
bool addLegends(string&);
bool checkDelimiter(const string& sToken, bool stringdelim = false);
string LineBreak(string sOutput, const Settings& _option, bool bAllowDashBreaks = true, int nFirstIndent = 4, int nIndent = 4);
bool ResizeConsole(const Settings& _option);
bool ColorTheme(const Settings& _option);
double Linearize(double x_0, double y_0, double x_1, double y_1);
string toSystemCodePage(string sOutput);
string fromSystemCodePage(string sOutput);
void make_hline(int nLength = -1);
void make_progressBar(int nStep, int nFirstStep = 1, int nFinalStep = 100, const string& sType = "std");
bool containsStrings(const string& sLine);
bool fileExists(const string& sFilename);
void reduceLogFilesize(const string& sFileName);
string replaceToVectorname(const string& sExpression);
string utf8parser(const string& sString);
void eraseToken(string& sExpr, const string& sToken, bool bTokenHasValue = false);
vector<string> getDBFileContent(const string& sFilename, Settings& _option);
vector<vector<string> > getDataBase(const string& sDatabaseFileName, Settings& _option);
string generateCacheName(const string& sFilename, Settings& _option);
string getFileInfo(const string& sFilename);

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

/** \brief Calculates the power of a number using an integer as exponent
 *
 * \param dNumber double
 * \param nExponent int
 * \return double
 *
 */
double intPower(double dNumber, int nExponent);

string getNextArgument(string& sArgList, bool bCut = true);
string getLastArgument(string& sArgList, bool bCut = true);

/** \brief Transforms Windows-style filepaths to Unix-Style
 *
 * \param __sPath const string&
 * \return string
 *
 */
string replacePathSeparator(const string& __sPath);

/** \brief Checks, whether the "to_cmd()" function was used
 *
 * \param sCmd const string&
 * \param nPos unsigned int
 * \return bool
 *
 */
bool isToCmd(const string& sCmd, unsigned int nPos);

void printLogo();
string getTimeStamp(bool bGetStamp = true);
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

bool containsDataObject(const string& sExpr);
void evalRecursiveExpressions(string& sExpr);

int compareDouble(const void* p1, const void* p2);
size_t qSortDouble(double* dArray, size_t nlength);


#endif

