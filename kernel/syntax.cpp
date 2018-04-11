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
#include "core/utils/tools.hpp"


NumeReSyntax::NumeReSyntax()
{
    vNSCRCommands.push_back("NO_SYNTAX_ELEMENTS");
    vNPRCCommands.push_back("NO_SYNTAX_ELEMENTS");
    vFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vMethods.push_back("NO_SYNTAX_ELEMENTS");
    vOptions.push_back("NO_SYNTAX_ELEMENTS");
    vConstants.push_back("NO_SYNTAX_ELEMENTS");
    vSpecialValues.push_back("NO_SYNTAX_ELEMENTS");
    vOperators.push_back("NO_SYNTAX_ELEMENTS");
    vMatlabKeyWords.push_back("NO_SYNTAX_ELEMENTS");
    vMatlabFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vCppKeyWords.push_back("NO_SYNTAX_ELEMENTS");
    vCppFunctions.push_back("NO_SYNTAX_ELEMENTS");
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
        if (sLine.substr(0, 13) == "NSCR_COMMANDS")
            vNSCRCommands = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 13) == "NPRC_COMMANDS")
            vNPRCCommands = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 9) == "FUNCTIONS")
            vFunctions = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 7) == "METHODS")
            vMethods = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 7) == "OPTIONS")
            vOptions = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 9) == "CONSTANTS")
            vConstants = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 11) == "SPECIALVALS")
            vSpecialValues = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 9) == "OPERATORS")
            vOperators = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 14) == "MATLABKEYWORDS")
            vMatlabKeyWords = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 15) == "MATLABFUNCTIONS")
            vMatlabFunctions = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 11) == "CPPKEYWORDS")
            vCppKeyWords = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 12) == "CPPFUNCTIONS")
            vCppFunctions = splitString(sLine.substr(sLine.find('=')+1));
    }
}

void NumeReSyntax::addPlugins(const vector<string>& vPlugins)
{
    if (!vPlugins.size())
        return;
    vNSCRCommands.insert(vNSCRCommands.end(), vPlugins.begin(), vPlugins.end());
}

void NumeReSyntax::setProcedureTree(const vector<string>& vTree)
{
    vProcedureTree = vTree;
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        while (vProcedureTree[i].find('/') != string::npos)
            vProcedureTree[i][vProcedureTree[i].find('/')] = '~';
        while (vProcedureTree[i].find('\\') != string::npos)
            vProcedureTree[i][vProcedureTree[i].find('\\')] = '~';
        if (vProcedureTree[i].find(".nprc") != string::npos)
            vProcedureTree[i].erase(vProcedureTree[i].rfind(".nprc"));
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

    if (sCommandLine.front() == (char)15)
        return highlightError(sCommandLine);

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
            if (c == c_normal && sCommandLine[k] == '"' && (!k || sCommandLine[k-1] != '\\'))
            {
                c = c_string;
                colors[k] = c;
                continue;
            }

            colors[k] = c;

            if (c == c_string && sCommandLine[k] == '"' && k > sCommandLine.find('"') && sCommandLine[k-1] != '\\')
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
            if (matchItem(vNSCRCommands, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_COMMAND);
            }
            if (matchItem(vNPRCCommands, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_PROCEDURE);
            }
            else if (i+nLen < sCommandLine.length()
                && sCommandLine.find('.', i) < i+nLen
                && matchItem(vMethods, sCommandLine.substr(sCommandLine.find('.', i)+1, i+nLen-sCommandLine.find('.', i)-1)))
            {
                size_t new_len = i+nLen-sCommandLine.find('.', i)-1;
                colors.replace(sCommandLine.find('.', i)+1, new_len, new_len, '0'+SYNTAX_METHODS);
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


string NumeReSyntax::highlightError(const string& sCommandLine)
{
    string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_OPERATOR);
    return colors;
}


string NumeReSyntax::getAutoCompList(string sFirstChars, string sType)
{
    string sAutoCompList;
    if (!mAutoCompList.size())
    {
        for (size_t i = 0; i < vNSCRCommands.size(); i++)
            mAutoCompList[toLowerCase(vNSCRCommands[i])+" |"+vNSCRCommands[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vNPRCCommands.size(); i++)
            mAutoCompList[toLowerCase(vNPRCCommands[i])+" |"+vNPRCCommands[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vFunctions.size(); i++)
            mAutoCompList[toLowerCase(vFunctions[i])+" |"+vFunctions[i]+"("] = SYNTAX_FUNCTION;
        for (size_t i = 0; i < vMethods.size(); i++)
            mAutoCompList[toLowerCase(vMethods[i])+" |"+vMethods[i]+"("] = SYNTAX_METHODS; // should be SYNTAX_METHODS, however icon is needed first
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
            if (sType == "NSCR" && iter->second == SYNTAX_NPRC_COMMAND)
                continue;
            if (sFirstChars == (iter->first).substr(0,sFirstChars.length()))
            {
                if (iter->second == SYNTAX_NPRC_COMMAND)
                    sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(SYNTAX_COMMAND)) + " ";
                else
                    sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
            }
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}

string NumeReSyntax::getAutoCompListMATLAB(string sFirstChars)
{
    string sAutoCompList;
    if (!mAutoCompListMATLAB.size())
    {
        for (size_t i = 0; i < vMatlabKeyWords.size(); i++)
            mAutoCompListMATLAB[toLowerCase(vMatlabKeyWords[i])+" |"+vMatlabKeyWords[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vMatlabFunctions.size(); i++)
            mAutoCompListMATLAB[toLowerCase(vMatlabFunctions[i])+" |"+vMatlabFunctions[i]+"("] = SYNTAX_FUNCTION;
    }
    sFirstChars = toLowerCase(sFirstChars);
    for (auto iter = mAutoCompListMATLAB.begin(); iter != mAutoCompListMATLAB.end(); ++iter)
    {
        if ((iter->first).front() == sFirstChars.front())
        {
            if (sFirstChars == (iter->first).substr(0,sFirstChars.length()))
            {
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
            }
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}

string NumeReSyntax::getAutoCompListCPP(string sFirstChars)
{
    string sAutoCompList;
    if (!mAutoCompListCPP.size())
    {
        for (size_t i = 0; i < vCppKeyWords.size(); i++)
            mAutoCompListCPP[toLowerCase(vCppKeyWords[i])+" |"+vCppKeyWords[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vCppFunctions.size(); i++)
            mAutoCompListCPP[toLowerCase(vCppFunctions[i])+" |"+vCppFunctions[i]+"("] = SYNTAX_FUNCTION;
    }
    sFirstChars = toLowerCase(sFirstChars);
    for (auto iter = mAutoCompListCPP.begin(); iter != mAutoCompListCPP.end(); ++iter)
    {
        if ((iter->first).front() == sFirstChars.front())
        {
            if (sFirstChars == (iter->first).substr(0,sFirstChars.length()))
            {
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
            }
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}


string NumeReSyntax::getProcAutoCompList(string sFirstChars, string sBaseNameSpace, string sSelectedNameSpace)
{
    if (!vProcedureTree.size())
        return "";

    string sProcName;
    static string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};
    if (sSelectedNameSpace.length())
    {
        if (sSelectedNameSpace.back() != '~')
            sProcName = sSelectedNameSpace + "~" + sFirstChars;
        else
            sProcName = sSelectedNameSpace + sFirstChars;
    }
    else if (sBaseNameSpace.length())
    {
        if (sBaseNameSpace.back() != '~')
            sProcName = sBaseNameSpace + "~" + sFirstChars;
        else
            sProcName = sBaseNameSpace + sFirstChars;
    }
    else
        sProcName = sFirstChars;
    string sAutoCompList = " ";
    if (!sSelectedNameSpace.length())
    {
        for (size_t i = 0; i < 3; i++)
        {
            if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
                sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
        }
    }

    string sToken;
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        if (vProcedureTree[i].substr(0, sProcName.length()) == sProcName)
        {
            if (sSelectedNameSpace.length())
            {
                sToken = vProcedureTree[i].substr(sSelectedNameSpace.length());
                if (sToken.find('~', sFirstChars.length()) != string::npos)
                {
                    sToken.erase(sToken.find('~', sFirstChars.length())+1);
                    sToken += "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
                }
                else
                    sToken += "(?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            }
            else if (vProcedureTree[i].find('~', sProcName.length()) != string::npos)
                sToken = vProcedureTree[i].substr(0, vProcedureTree[i].find('~', sFirstChars.length())+1) + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            else
                sToken = vProcedureTree[i] + "(?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            if (!sSelectedNameSpace.length())
            {
                if (sToken.substr(0, sBaseNameSpace.length()) == sBaseNameSpace)
                    sToken.erase(0, sBaseNameSpace.length());
                if (sToken.front() == '~')
                    sToken.erase(0,1);
            }
            if (sAutoCompList.find(" " + sToken) == string::npos)
                sAutoCompList += sToken;
        }
    }
    return sAutoCompList.substr(1);
}


string NumeReSyntax::getNameSpaceAutoCompList(string sFirstChars)
{
    if (!vProcedureTree.size())
        return "";

    string sProcName;
    static string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};
    sProcName = sFirstChars;
    string sAutoCompList = " ";

    for (size_t i = 0; i < 3; i++)
    {
        if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
            sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
    }


    string sToken;
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        if (vProcedureTree[i].substr(0, sProcName.length()) == sProcName)
        {
            if (vProcedureTree[i].find('~', sProcName.length()) != string::npos)
            {
                sToken = vProcedureTree[i].substr(0, vProcedureTree[i].find('~', sProcName.length())+1) + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            }
            if (sAutoCompList.find(" " + sToken) == string::npos)
                sAutoCompList += sToken;
        }
    }
    return sAutoCompList.substr(1);
}


