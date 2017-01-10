/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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




#ifndef SYNTAX_HPP
#define SYNTAX_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>

using namespace std;

class NumeReSyntax
{
    private:
        vector<string> vCommands;
        vector<string> vOptions;
        vector<string> vFunctions;
        vector<string> vConstants;
        vector<string> vSpecialValues;
        vector<string> vOperators;
        string sSingleOperators;
        map<string, int> mAutoCompList;

        string constructString(const vector<string>& vVector) const;
        vector<string> splitString(string sString);
        bool matchItem(const vector<string>& vVector, const string& sString);
    public:
        enum SyntaxColors
        {
            SYNTAX_COMMAND = 1,
            SYNTAX_OPTION,
            SYNTAX_FUNCTION,
            SYNTAX_CONSTANT,
            SYNTAX_SPECIALVAL,
            SYNTAX_STRING,
            SYNTAX_STD,
            SYNTAX_OPERATOR,
            SYNTAX_PROCEDURE,
            SYNTAX_NUMBER
        };
        NumeReSyntax();
        NumeReSyntax(const string& sPath);

        void loadSyntax(const string& sPath);
        string getCommands() const
            {return constructString(vCommands);}
        string getOptions() const
            {return constructString(vOptions);}
        string getFunctions() const
            {return constructString(vFunctions);}
        string getConstants() const
            {return constructString(vConstants);}
        string getSpecial() const
            {return constructString(vSpecialValues);}
        string getOperators() const
            {return constructString(vOperators);}
        string getAutoCompList(string sFirstChars);

        string highlightLine(const string& sCommandLine);

};
#endif // SYNTAX_HPP

