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


// Default constructor
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
    vTeXKeyWords.push_back("NO_SYNTAX_ELEMENTS");

    // The operator characters are always the same
    sSingleOperators = "+-*/?=()[]{},;#^!&|<>%:";
}

// Specialized constructor. Will also load the syntax definitions
NumeReSyntax::NumeReSyntax(const string& sPath) : NumeReSyntax()
{
    loadSyntax(sPath);
}

// Member function for loading the syntax element definitions
void NumeReSyntax::loadSyntax(const string& sPath)
{
    ifstream file_in;
    string sLine;

    // Open the file and check, whether the file stream is valid
    file_in.open(sPath + "lang\\syntaxelements.nlng");
    if (!file_in.good())
    {
        return;
    }

    // Read the overall file
    while (!file_in.eof())
    {
        // Get the current line
        getline(file_in, sLine);

        // Ignore it, if it's empty or if it starts with a comment sign
        if (!sLine.length())
            continue;
        if (sLine.front() == '#')
            continue;

        // Depending on the identifier at the beginning of the line, the contents
        // are used for different syntax elements
        // The member function "splitString" splits the line into a vector
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
        else if (sLine.substr(0, 11) == "TEXKEYWORDS")
            vTeXKeyWords = splitString(sLine.substr(sLine.find('=')+1));
    }
}

// Add the plugin definitions to the command strings
void NumeReSyntax::addPlugins(const vector<string>& vPlugins)
{
    if (!vPlugins.size())
        return;
    vNSCRCommands.insert(vNSCRCommands.end(), vPlugins.begin(), vPlugins.end());
}

// Set the procedure tree (used for autocompleting)
void NumeReSyntax::setProcedureTree(const vector<string>& vTree)
{
    // Copy the tree
    vProcedureTree = vTree;

    // Replace the path separators with namespace characters
    // Also remove the file extensions
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

// This member function concatenates the passed vector elements to a whitespace-separated single string
string NumeReSyntax::constructString(const vector<string>& vVector) const
{
    string sReturn = "";

    // Concatenate all elements in vVector
    for (size_t i = 0; i < vVector.size(); i++)
    {
        sReturn += vVector[i] + " ";
    }
    return sReturn;
}

// This function splits the passed string up into single string tokens
vector<string> NumeReSyntax::splitString(string sString)
{
    vector<string> vReturn;

    // As long as the string has a length
    while (sString.length())
    {
        // Remove leading whitespaces
        while (sString.front() == ' ')
            sString.erase(0,1);

        // Separate at the whitespaces
        if (sString.find(' ') != string::npos)
        {
            // Push the current token
            vReturn.push_back(sString.substr(0,sString.find(' ')));

            // Remove the current token from the string
            sString.erase(0,sString.find(' ')+1);

            // ensure that the current token has a length
            if (!vReturn.back().length())
                vReturn.pop_back();
        }
        else
        {
            // If no whitespace was found, simply push the complete string
            vReturn.push_back(sString);
            break;
        }
    }
    return vReturn;
}

// This function searches for a match of the passed string in th passed vector
bool NumeReSyntax::matchItem(const vector<string>& vVector, const string& sString)
{
    for (size_t i = 0; i < vVector.size(); i++)
    {
        // Match found -> return true
        if (vVector[i] == sString)
            return true;
    }

    // Nothing found
    return false;
}

// This function applies the highlighting colors to the command line (only used in the console)
string NumeReSyntax::highlightLine(const string& sCommandLine)
{
    // Ensure that a command line with a length is available
    if (!sCommandLine.length())
        return "";

    // Search for the error marker.
    // If found jump to the error highlighting function and return
    if (sCommandLine.front() == (char)15)
        return highlightError(sCommandLine);

    // Search for the warning operator.
    if (sCommandLine.substr(0, 3) == "|!>")
        return highlightWarning(sCommandLine);

    // Create a color string, which will only contain default colors
    string colors;

    // Fill the color string with the default colors
    colors.assign(sCommandLine.length(),'0'+SYNTAX_STD);
    char c;

    // Avoid some special cases
    // These shall not be highlighted, because they are messages from the kernel
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

    // Search for strings
    if (sCommandLine.find('"') != string::npos)
    {
        char c_string = '0'+SYNTAX_STRING;
        char c_normal = '0'+SYNTAX_STD;
        c = c_string;

        // Simply alternate the colors
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

    // Search for procedures
    if (sCommandLine.find('$') != string::npos)
    {
        int c_proc = '0'+SYNTAX_PROCEDURE;
        int c_normal = '0'+SYNTAX_STD;
        c = c_proc;

        // Apply the color from the dollar sign just up to the next whitespace or an opening parenthesis
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

    // Apply the syntax elements
    for (unsigned int i = 0; i < sCommandLine.length(); i++)
    {
        // find the first relevant character
        if (!i && sCommandLine.substr(0,3) == "|<-")
            i += 3;
        else if (!i && (sCommandLine.substr(0,5) == "|-\?\?>" || sCommandLine.substr(0,4) == "|FOR" || sCommandLine.substr(0,5) == "|ELSE" || sCommandLine.substr(0,5) == "|ELIF" || sCommandLine.substr(0,4) == "|WHL" || sCommandLine.substr(0,3) == "|IF" || sCommandLine.substr(0,5) == "|PROC" || sCommandLine.substr(0,5) == "|COMP"))
            i += sCommandLine.find('>')+1;
        else if (!i && sCommandLine.substr(0,4) == "||<-")
            i += 4;
        else if (!i && sCommandLine.substr(0,3) == "|| ")
            i += 3;

        // Ignore whitespaces
        if (sCommandLine[i] == ' ')
            continue;

        // Highlight numbers
        if (sCommandLine[i] >= '0' && sCommandLine[i] <= '9')
        {
            unsigned int nLen = 0;

            // Until the curent number ends
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

            // replace the characters
            colors.replace(i, nLen, nLen, '0'+SYNTAX_NUMBER);
            i += nLen;
        }

        // Only enter this block, if it is only default-highlighted
        if (colors[i] == '0'+SYNTAX_STD)
        {
            unsigned int nLen = 0;

            // Find the end of the current syntax element
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && sCommandLine[i+nLen] != ' '
                && sCommandLine[i+nLen] != '\''
                && sSingleOperators.find(sCommandLine[i+nLen]) == string::npos)
                nLen++;

            // color operators
            if (sSingleOperators.find(sCommandLine[i+nLen]) != string::npos)
            {
                colors[i+nLen] = '0'+SYNTAX_OPERATOR;
            }

            // Color the actual syntax elements
            if (matchItem(vNSCRCommands, sCommandLine.substr(i,nLen)))
            {
                // Commands
                colors.replace(i, nLen, nLen, '0'+SYNTAX_COMMAND);
            }
            if (matchItem(vNPRCCommands, sCommandLine.substr(i,nLen)))
            {
                // Commands for NPRC in NSCR (highlighted differently)
                colors.replace(i, nLen, nLen, '0'+SYNTAX_PROCEDURE);
            }
            else if (i+nLen < sCommandLine.length()
                && sCommandLine.find('.', i) < i+nLen)
            {
                // Methods on string variables or on data sets
                size_t nPos = sCommandLine.find('.', i)+1;
                for (size_t n = nPos; n < i+nLen; n++)
                {
                    if (sCommandLine[n] == '.')
                    {
                        if (matchItem(vMethods, sCommandLine.substr(nPos, n-nPos)))
                            colors.replace(nPos, n-nPos, n-nPos, '0'+SYNTAX_METHODS);
                        nPos = n+1;
                    }
                    if (n+1 == i+nLen)
                    {
                        if (matchItem(vMethods, sCommandLine.substr(nPos, n-nPos+1)))
                            colors.replace(nPos, n-nPos+1, n-nPos+1, '0'+SYNTAX_METHODS);
                        nPos = n+1;
                    }
                }
            }
            else if (i+nLen < sCommandLine.length()
                && sCommandLine[i+nLen] == '('
                && matchItem(vFunctions, sCommandLine.substr(i,nLen)))
            {
                // Functions
                colors.replace(i, nLen, nLen, '0'+SYNTAX_FUNCTION);
            }
            else if (matchItem(vOptions, sCommandLine.substr(i,nLen)))
            {
                // Command line options
                colors.replace(i, nLen, nLen, '0'+SYNTAX_OPTION);
            }
            else if (matchItem(vConstants, sCommandLine.substr(i,nLen)))
            {
                // Constants
                colors.replace(i, nLen, nLen, '0'+SYNTAX_CONSTANT);
            }
            else if (matchItem(vSpecialValues, sCommandLine.substr(i,nLen)))
            {
                // Special variables
                colors.replace(i, nLen, nLen, '0'+SYNTAX_SPECIALVAL);
            }
            i += nLen;
        }
    }

    // Return the color string
    return colors;
}

// Highlight an error message. We simply use the color of the operators (red as default)
string NumeReSyntax::highlightError(const string& sCommandLine)
{
    string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_OPERATOR);
    return colors;
}

// Highlight a warning message. We simply use the color of numbers (orange as default)
string NumeReSyntax::highlightWarning(const string& sCommandLine)
{
    string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_NUMBER);
    return colors;
}

// This function returns the autocompletion list for the editor
string NumeReSyntax::getAutoCompList(string sFirstChars, string sType)
{
    string sAutoCompList;

    // Only of the autocompletion map is not filled
    if (!mAutoCompList.size())
    {
        // Insert every element in the map
        for (size_t i = 0; i < vNSCRCommands.size(); i++)
            mAutoCompList[toLowerCase(vNSCRCommands[i])+" |"+vNSCRCommands[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vNPRCCommands.size(); i++)
            mAutoCompList[toLowerCase(vNPRCCommands[i])+" |"+vNPRCCommands[i]] = SYNTAX_COMMAND;
        for (size_t i = 0; i < vFunctions.size(); i++)
            mAutoCompList[toLowerCase(vFunctions[i])+" |"+vFunctions[i]+"("] = SYNTAX_FUNCTION;
        for (size_t i = 0; i < vMethods.size(); i++)
            mAutoCompList[toLowerCase(vMethods[i])+" |"+vMethods[i]+"("] = SYNTAX_METHODS;
        for (size_t i = 0; i < vOptions.size(); i++)
            mAutoCompList[toLowerCase(vOptions[i])+" |"+vOptions[i]] = SYNTAX_OPTION;
        for (size_t i = 0; i < vConstants.size(); i++)
            mAutoCompList[toLowerCase(vConstants[i])+" |"+vConstants[i]] = SYNTAX_CONSTANT;
        for (size_t i = 0; i < vSpecialValues.size(); i++)
            mAutoCompList[toLowerCase(vSpecialValues[i])+" |"+vSpecialValues[i]] = SYNTAX_SPECIALVAL;
        for (size_t i = 0; i < vOperators.size(); i++)
            mAutoCompList[toLowerCase(vOperators[i])+" |"+vOperators[i]] = SYNTAX_OPERATOR;
    }

    // Transform the passed first characters to lower case to avoid case sensitivity
    sFirstChars = toLowerCase(sFirstChars);

    // Try to find the correspondig elements in the map
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

    // return the created list
    return sAutoCompList;
}

// The same as above but specialized for MATLAB commands
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

// The same as above but specialized for C++ Commands
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

// The same as above but specialized for LaTeX Commands
string NumeReSyntax::getAutoCompListTeX(string sFirstChars)
{
    string sAutoCompList;
    if (!mAutoCompListTeX.size())
    {
        for (size_t i = 0; i < vTeXKeyWords.size(); i++)
            mAutoCompListTeX[toLowerCase(vTeXKeyWords[i])+" |"+vTeXKeyWords[i]] = SYNTAX_COMMAND;
    }
    sFirstChars = toLowerCase(sFirstChars);
    for (auto iter = mAutoCompListTeX.begin(); iter != mAutoCompListTeX.end(); ++iter)
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

// This function will return the autocompletion list for the procedures based upon the provided procedure tree
string NumeReSyntax::getProcAutoCompList(string sFirstChars, string sBaseNameSpace, string sSelectedNameSpace)
{
    if (!vProcedureTree.size())
        return "";

    string sProcName;
    static string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};

    // Try to detect the actual name of the procedure including its namespace
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

    // If no namespace was pre-selected, provide the standard namespaces
    if (!sSelectedNameSpace.length())
    {
        for (size_t i = 0; i < 3; i++)
        {
            if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
                sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
        }
    }

    string sToken;

    // Go through the complete procedure tree
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        // If the procedure in the tree corresponds to the passed procedure fragment
        if (vProcedureTree[i].substr(0, sProcName.length()) == sProcName)
        {
            // Only add the namespace if it's needed
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

            // Remove the base namespace, if it is available in the current token
            if (!sSelectedNameSpace.length())
            {
                if (sToken.substr(0, sBaseNameSpace.length()) == sBaseNameSpace)
                    sToken.erase(0, sBaseNameSpace.length());
                if (sToken.front() == '~')
                    sToken.erase(0,1);
            }

            // Only add the current element to the autocompletion list, if it is not already available
            if (sAutoCompList.find(" " + sToken) == string::npos)
                sAutoCompList += sToken;
        }
    }

    // Return the autocompletion list (the first character is always a whitespace character)
    return sAutoCompList.substr(1);
}

// This function returns the autocompletion list for the namespaces
string NumeReSyntax::getNameSpaceAutoCompList(string sFirstChars)
{
    if (!vProcedureTree.size())
        return "";

    string sProcName;
    static string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};
    sProcName = sFirstChars;
    string sAutoCompList = " ";

    // Provide the standard namespaces first
    for (size_t i = 0; i < 3; i++)
    {
        if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
            sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
    }

    // Append all available namespaces
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

    // Return the autocompletion list (the first character is always a whitespace character)
    return sAutoCompList.substr(1);
}


