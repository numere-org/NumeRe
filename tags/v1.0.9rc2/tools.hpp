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

//#include <cstring>
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

#include "error.hpp"
#include "settings.hpp"
#include "datafile.hpp"

extern const string sVersion;
extern int nLINE_LENGTH;
using namespace std;

struct Match
{
    string sString;
    unsigned int nPos;
};

struct Line
{
    string sDesc;
    string sStyle;
    double dPos;
};

struct Axis
{
    string sLabel;
    string sStyle;
    double dMin;
    double dMax;
};

struct Returnvalue
{
    vector<double> vNumVal;
    vector<string> vStringVal;
};

// --> Macht aus einem Int ein String (mit Praezision) <--
inline string toString (int nNumber, const Settings& _option)
{
    return toString((double)nNumber, _option);  // Unnoetig das nochmal zu schreiben. Rufen wir die andere Funktion mit einer expliziten Konvertierung auf
}

// --> Macht aus einem Double ein String (Wobei die Praezision beachtet wird) <--
inline string toString (double dNumber, const Settings& _option)
{
    ostringstream Temp;                        // Wir brauchen einen ostringstream, umd double in string zu konvertieren
    Temp.precision(_option.getPrecision());
    Temp << dNumber;
    return Temp.str();                         // Auf den eigentlichen string wird dann mit der Methode ostringstream::str() zugegriffen
}

// --> Macht aus einem Double einen String mit beliebiger Preazision <--
inline string toString (double dNumber, int nPrecision)
{
    ostringstream Temp;
    Temp.precision(nPrecision);
    Temp << dNumber;
    return Temp.str();
}

string toString(int);
string toString(time_t tTime, bool bOnlyTime = false);

// --> Macht aus einem Int ein String (ohne Praezision) <--
inline string toString (long long int nNumber)
{
    ostringstream Temp;
    Temp << nNumber;
    return Temp.str();
}

// --> Macht aus einem Double einen String mit 20 Ziffern <--
inline string toCmdString (double dNumber)
{
    ostringstream Temp;
    Temp.precision(12);
    Temp << dNumber;
    return Temp.str();
}

// --> Macht aus einem bool einen String mit "true" oder "false" <--
inline string toString(bool bBoolean)
{
    if (bBoolean)
        return "true";
    else
        return "false";
}

inline string condenseText(const string& sText)
{
    string sReturn = sText;
    string sToErase = " AaEeIiOoUuÄäÖöÜüß";
    for (unsigned int i = 0; i < sReturn.length(); i++)
    {
        if (sToErase.find(sReturn[i]) != string::npos
            || sReturn[i] == 142
            || sReturn[i] == 132
            || sReturn[i] == 153
            || sReturn[i] == 148
            || sReturn[i] == 154
            || sReturn[i] == 129
            || sReturn[i] == 225)
        {
            sReturn.erase(i,1);
            i--;
        }
    }
    return sReturn;
}

void StripSpaces(string&);
void SetConsTitle(const Datafile&, const Settings&, string sScript = "");
int matchParams(const string& sCmd, const string& sParam, const char cFollowing = ' ');
bool getStringArgument(const string& sCmd, string& sArgument);
bool getIntArgument(const string& sCmd, int& nArgument);
int StrToInt(const string&);
double StrToDb(const string&);
unsigned int getMatchingParenthesis(const string&);
string toLowerCase(const string& sUpperCase);
string toUpperCase(const string& sLowerCase);
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis = false);
string replaceToTeX(const string&);
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
bool checkDelimiter(const string&);
string LineBreak(string sOutput, const Settings& _option, bool bAllowDashBreaks = true, int nFirstIndent = 0, int nIndent = 4);
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

inline bool validateParenthesisNumber(const string& sCmd)
{
    int nParCount = 0;
    int nVectCount = 0;
    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == '(' && !isInQuotes(sCmd, i, true))
            nParCount++;
        if (sCmd[i] == ')' && !isInQuotes(sCmd, i, true))
            nParCount--;
        if (sCmd[i] == '{' && !isInQuotes(sCmd, i, true))
            nVectCount++;
        if (sCmd[i] == '}' && !isInQuotes(sCmd, i, true))
            nVectCount--;
    }
    return !((bool)nParCount || (bool)nVectCount);
}

inline void addArgumentQuotes(string& sToAdd, const string& sParam)
{
    if (matchParams(sToAdd, sParam, '='))
    {
        //cerr << sToAdd << endl;
        int nPos = matchParams(sToAdd, sParam, '=') + sParam.length();
        //cerr << nPos << endl;
        while (sToAdd[nPos] == ' ')
            nPos++;
        //cerr << nPos << endl;
        if (!containsStrings(sToAdd.substr(nPos, sToAdd.find(' ', nPos)-nPos)))  //sToAdd[nPos] != '"' && sToAdd[nPos] != '#' && sToAdd.substr(nPos, 10) != "to_string(" && sToAdd.substr(nPos, 7) != "string(")
        {
            sToAdd = sToAdd.substr(0,nPos)
                + "\"" + getArgAtPos(sToAdd, nPos) + "\""
                + sToAdd.substr(sToAdd.find(' ', sToAdd.find(getArgAtPos(sToAdd, nPos))+getArgAtPos(sToAdd, nPos).length()));
            //cerr << sToAdd << endl;
        }
        else
            return;
    }
    else
        return;
    return;
}

inline double intPower(double dNumber, int nExponent)
{
    long double dResult = 1.0L;
    if (!nExponent)
        return 1.0;

    for (int i = abs(nExponent); i > 0; i--)
    {
        dResult *= (long double)dNumber;
    }

    if (nExponent > 0)
        return dResult;
    else
        return 1.0/dResult;
}
string getNextArgument(string& sArgList, bool bCut = true);

inline string replacePathSeparator(const string& __sPath)
{
    if (__sPath.find('\\') == string::npos)
        return __sPath;

    string sPath = __sPath;
    for (unsigned int i = 0; i < sPath.length(); i++)
    {
        if (sPath[i] == '\\')
            sPath[i] = '/';
    }
    return sPath;
}

inline bool isToCmd(const string& sCmd, unsigned int nPos)
{
    if (nPos < 6 || nPos >= sCmd.length())
        return false;
    if (sCmd.find("to_cmd(") == string::npos || sCmd.find("to_cmd(") > nPos)
        return false;
    for (unsigned int i = nPos-6; i >= 0; i--)
    {
        if (sCmd.substr(i,7) == "to_cmd(" && !isInQuotes(sCmd, i))
        {
            if (getMatchingParenthesis(sCmd.substr(i+6)) > nPos-i-6 && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos)
                return true;
        }
        if (!i)
            break;
    }
    return false;
}

inline void nextLoadMessage(unsigned int nSleep)
{
    Sleep(nSleep);
    cerr << "\r                                                                              \r";
    return;
}
void printLogo();
string getTimeStamp(bool bGetStamp = true);
inline unsigned int countEscapeSymbols(const string& sLine)
{
    unsigned int nCount = 0;
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine.substr(i,2) == "\\$")
            nCount++;
    }
    return nCount;
}
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


#endif

