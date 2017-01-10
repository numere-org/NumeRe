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


#include "syntax.hpp"
#include "core/tools.hpp"


NumeReSyntax::NumeReSyntax()
{
    vCommands.push_back("NO_SYNTAX_ELEMENTS");
    vFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vOptions.push_back("NO_SYNTAX_ELEMENTS");
    vConstants.push_back("NO_SYNTAX_ELEMENTS");
    vSpecialValues.push_back("NO_SYNTAX_ELEMENTS");
    vOperators.push_back("NO_SYNTAX_ELEMENTS");
    sSingleOperators = "+-*/?=()[]{},;#^!&|<>%:";
}

NumeReSyntax::NumeReSyntax(const string& sPath)
{
    NumeReSyntax();
    loadSyntax(sPath);
}

void NumeReSyntax::loadSyntax(const string& sPath)
{
    ifstream file_in;
    string sLine;

    file_in.open(sPath + "lang\\syntaxelements.nlng");
    if (!file_in.good())
    {
        return;
    }
    while (!file_in.eof())
    {
        getline(file_in, sLine);
        if (!sLine.length())
            continue;
        if (sLine.front() == '#')
            continue;
        if (sLine.find("NSCR_COMMANDS") != string::npos)
            vCommands = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.find("NSCR_FUNCTIONS") != string::npos)
            vFunctions = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.find("NSCR_OPTIONS") != string::npos)
            vOptions = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.find("NSCR_CONSTANTS") != string::npos)
            vConstants = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.find("NSCR_SPECIALVALS") != string::npos)
            vSpecialValues = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.find("NSCR_OPERATORS") != string::npos)
            vOperators = splitString(sLine.substr(sLine.find('=')+1));
    }
}

string NumeReSyntax::constructString(const vector<string>& vVector) const
{
    string sReturn = "";

    for (size_t i = 0; i < vVector.size(); i++)
    {
        sReturn += vVector[i] + " ";
    }
    return sReturn;
}

vector<string> NumeReSyntax::splitString(string sString)
{
    vector<string> vReturn;
    while (sString.length())
    {
        while (sString.front() == ' ')
            sString.erase(0,1);
        if (sString.find(' ') != string::npos)
        {
            vReturn.push_back(sString.substr(0,sString.find(' ')));
            sString.erase(0,sString.find(' ')+1);

            if (!vReturn.back().length())
                vReturn.pop_back();
        }
        else
        {
            vReturn.push_back(sString);
            break;
        }
    }
    return vReturn;
}

bool NumeReSyntax::matchItem(const vector<string>& vVector, const string& sString)
{
    for (size_t i = 0; i < vVector.size(); i++)
    {
        if (vVector[i] == sString)
            return true;
    }
    return false;
}

string NumeReSyntax::highlightLine(const string& sCommandLine)
{
    string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_STD);
    char c;

    if (sCommandLine.substr(0,3) != "|<-"
        && sCommandLine.substr(0,5) != "|-\?\?>"
        && (sCommandLine.substr(0,2) != "||" || sCommandLine.substr(0,4) == "||->")
        && sCommandLine.substr(0,4) != "|FOR"
        && sCommandLine.substr(0,5) != "|ELSE"
        && sCommandLine.substr(0,5) != "|ELIF"
        && sCommandLine.substr(0,5) != "|PROC"
        && sCommandLine.substr(0,5) != "|COMP"
        && sCommandLine.substr(0,3) != "|IF"
        && sCommandLine.substr(0,4) != "|WHL")
        return colors;

    if (sCommandLine.find('"') != string::npos)
    {
        char c_string = '0'+SYNTAX_STRING;
        char c_normal = '0'+SYNTAX_STD;
        c = c_string;
        for (size_t k = sCommandLine.find('"'); k < sCommandLine.length(); k++)
        {
            if (c == c_normal && sCommandLine[k] == '"')
            {
                c = c_string;
                colors[k] = c;
                continue;
            }

            colors[k] = c;

            if (c == c_string && sCommandLine[k] == '"' && k > sCommandLine.find('"'))
            {
                c = c_normal;
            }
        }
    }
    if (sCommandLine.find('$') != string::npos)
    {
        int c_proc = '0'+SYNTAX_PROCEDURE;
        int c_normal = '0'+SYNTAX_STD;
        c = c_proc;
        for (size_t k = sCommandLine.find('$'); k < sCommandLine.length(); k++)
        {
            if (sCommandLine[k] == '(' || sCommandLine[k] == ' ')
            {
                c = c_normal;
            }
            else if (sCommandLine[k] == '$')
            {
                c = c_proc;
            }
            if (colors[k] == '0'+SYNTAX_STD)
                colors[k] = c;
            else
                c = c_normal;
        }
    }

    for (unsigned int i = 0; i < sCommandLine.length(); i++)
    {
        if (!i && sCommandLine.substr(0,3) == "|<-")
            i += 3;
        else if (!i && (sCommandLine.substr(0,5) == "|-\?\?>" || sCommandLine.substr(0,4) == "|FOR" || sCommandLine.substr(0,5) == "|ELSE" || sCommandLine.substr(0,5) == "|ELIF" || sCommandLine.substr(0,4) == "|WHL" || sCommandLine.substr(0,3) == "|IF" || sCommandLine.substr(0,5) == "|PROC" || sCommandLine.substr(0,5) == "|COMP"))
            i += sCommandLine.find('>')+1;
        else if (!i && sCommandLine.substr(0,4) == "||<-")
            i += 4;
        else if (!i && sCommandLine.substr(0,3) == "|| ")
            i += 3;
        if (sCommandLine[i] == ' ')
            continue;
        if (sCommandLine[i] >= '0' && sCommandLine[i] <= '9')
        {
            unsigned int nLen = 0;
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && ((sCommandLine[i+nLen] >= '0' && sCommandLine[i+nLen] <= '9')
                    || sCommandLine[i+nLen] == '.'
                    || sCommandLine[i+nLen] == 'e'
                    || sCommandLine[i+nLen] == 'E'
                    || ((sCommandLine[i+nLen] == '-' || sCommandLine[i+nLen] == '+')
                        && (sCommandLine[i+nLen-1] == 'e' || sCommandLine[i+nLen-1] == 'E'))
                    )
                )
                nLen++;
            colors.replace(i, nLen, nLen, '0'+SYNTAX_NUMBER);
            i += nLen;
        }
        if (colors[i] == '0'+SYNTAX_STD)
        {
            unsigned int nLen = 0;
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && sCommandLine[i+nLen] != ' '
                && sCommandLine[i+nLen] != '\''
                && sSingleOperators.find(sCommandLine[i+nLen]) == string::npos)
                nLen++;
            if (sSingleOperators.find(sCommandLine[i+nLen]) != string::npos)
            {
                colors[i+nLen] = '0'+SYNTAX_OPERATOR;
            }
            if (matchItem(vCommands, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_COMMAND);
            }
            else if (i+nLen < sCommandLine.length()
                && sCommandLine[i+nLen] == '('
                && matchItem(vFunctions, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_FUNCTION);
            }
            else if (matchItem(vOptions, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_OPTION);
            }
            else if (matchItem(vConstants, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_CONSTANT);
            }
            else if (matchItem(vSpecialValues, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_SPECIALVAL);
            }
            i += nLen;
        }
    }

    return colors;
}

string NumeReSyntax::getAutoCompList(string sFirstChars)
{
    string sAutoCompList;
    if (!mAutoCompList.size())
    {
        for (size_t i = 0; i < vCommands.size(); i++)
            mAutoCompList[toLowerCase(vCommands[i])+" |"+vCommands[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vFunctions.size(); i++)
            mAutoCompList[toLowerCase(vFunctions[i])+" |"+vFunctions[i]+"("] = SYNTAX_FUNCTION;
        for (size_t i = 0; i < vOptions.size(); i++)
            mAutoCompList[toLowerCase(vOptions[i])+" |"+vOptions[i]] = SYNTAX_OPTION;
        for (size_t i = 0; i < vConstants.size(); i++)
            mAutoCompList[toLowerCase(vConstants[i])+" |"+vConstants[i]] = SYNTAX_CONSTANT;
        for (size_t i = 0; i < vSpecialValues.size(); i++)
            mAutoCompList[toLowerCase(vSpecialValues[i])+" |"+vSpecialValues[i]] = SYNTAX_SPECIALVAL;
        for (size_t i = 0; i < vOperators.size(); i++)
            mAutoCompList[toLowerCase(vOperators[i])+" |"+vOperators[i]] = SYNTAX_OPERATOR;
    }
    sFirstChars = toLowerCase(sFirstChars);
    for (auto iter = mAutoCompList.begin(); iter != mAutoCompList.end(); ++iter)
    {
        if ((iter->first).front() == sFirstChars.front())
        {
            if (sFirstChars == (iter->first).substr(0,sFirstChars.length()))
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}

