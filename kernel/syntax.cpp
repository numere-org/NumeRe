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


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
NumeReSyntax::NumeReSyntax() : sPath("")
{
    vNSCRCommands.push_back("NO_SYNTAX_ELEMENTS");
    vNPRCCommands.push_back("NO_SYNTAX_ELEMENTS");
    vFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vMethods.push_back("NO_SYNTAX_ELEMENTS");
    vMethodsArgs.push_back("NO_SYNTAX_ELEMENTS");
    vOptions.push_back("NO_SYNTAX_ELEMENTS");
    vConstants.push_back("NO_SYNTAX_ELEMENTS");
    vSpecialValues.push_back("NO_SYNTAX_ELEMENTS");
    vOperators.push_back("NO_SYNTAX_ELEMENTS");
    vDocKeyWords.push_back("NO_SYNTAX_ELEMENTS");
    vMatlabKeyWords.push_back("NO_SYNTAX_ELEMENTS");
    vMatlabFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vCppKeyWords.push_back("NO_SYNTAX_ELEMENTS");
    vCppFunctions.push_back("NO_SYNTAX_ELEMENTS");
    vTeXKeyWords.push_back("NO_SYNTAX_ELEMENTS");

    // The operator characters are always the same
    sSingleOperators = "+-*/?=()[]{},;#^!&|<>%:";
}


/////////////////////////////////////////////////
/// \brief Specialized constructor. Delegates to
/// the default constructor and will also load
/// the syntax definitions.
///
/// \param _sPath const std::string&
///
/////////////////////////////////////////////////
NumeReSyntax::NumeReSyntax(const std::string& _sPath) : NumeReSyntax()
{
    loadSyntax(_sPath);
}


/////////////////////////////////////////////////
/// \brief Specialized constructor. Delegates to
/// the default constructor and will also load
/// the syntax definitions. Afterwards, the
/// plugin commands are added to the list of
/// known commands.
///
/// \param _sPath const std::string&
/// \param vPlugins const std::vector<std::string>&
///
/////////////////////////////////////////////////
NumeReSyntax::NumeReSyntax(const std::string& _sPath, const std::vector<std::string>& vPlugins) : NumeReSyntax(_sPath)
{
    vNSCRCommands.insert(vNSCRCommands.end(), vPlugins.begin(), vPlugins.end());
}


/////////////////////////////////////////////////
/// \brief Member function for loading the syntax
/// element definitions.
///
/// \param _sPath const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReSyntax::loadSyntax(const std::string& _sPath)
{
    if (_sPath.length() && !sPath.length())
        sPath = _sPath;

    std::ifstream file_in;
    std::string sLine;

    // Open the file and check, whether the file stream is valid
    file_in.open(sPath + "lang\\syntaxelements.nlng");

    if (!file_in.good())
        return;

    // Read the overall file
    while (!file_in.eof())
    {
        // Get the current line
        std::getline(file_in, sLine);

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
        else if (sLine.substr(0, 11) == "METHODSARGS")
            vMethodsArgs = splitString(sLine.substr(sLine.find('=')+1));
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
        else if (sLine.substr(0, 11) == "DOCKEYWORDS")
            vDocKeyWords = splitString(sLine.substr(sLine.find('=')+1));
        else if (sLine.substr(0, 9) == "BLOCKDEFS")
            vBlockDefs = splitDefs(sLine.substr(sLine.find('=')+1));
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


/////////////////////////////////////////////////
/// \brief Add the plugin definitions to the
/// command strings. Will reload the standard
/// settings in advance to restore the default
/// state.
///
/// \param vPlugins const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void NumeReSyntax::addPlugins(const std::vector<std::string>& vPlugins)
{
    mAutoCompList.clear();

    loadSyntax();
    vNSCRCommands.insert(vNSCRCommands.end(), vPlugins.begin(), vPlugins.end());
}


/////////////////////////////////////////////////
/// \brief Set the procedure tree (used for
/// autocompleting).
///
/// \param vTree const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void NumeReSyntax::setProcedureTree(const std::vector<std::string>& vTree)
{
    // Copy the tree
    vProcedureTree = vTree;

    // Replace the path separators with namespace characters
    // Also remove the file extensions
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        while (vProcedureTree[i].find('/') != std::string::npos)
            vProcedureTree[i][vProcedureTree[i].find('/')] = '~';

        while (vProcedureTree[i].find('\\') != std::string::npos)
            vProcedureTree[i][vProcedureTree[i].find('\\')] = '~';

        if (vProcedureTree[i].find(".nprc") != std::string::npos)
            vProcedureTree[i].erase(vProcedureTree[i].rfind(".nprc"));
    }
}


/////////////////////////////////////////////////
/// \brief Returns all block definitions as a
/// folding string for the lexers.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getBlockDefs() const
{
    std::string sReturn;
    std::string endWords;

    for (const SyntaxBlockDefinition& def : vBlockDefs)
    {
        sReturn += def.startWord + " ";
        endWords += def.endWord + " ";
    }

    return sReturn + ";" + endWords;
}


/////////////////////////////////////////////////
/// \brief This member function concatenates the
/// passed vector elements to a whitespace-
/// separated single string.
///
/// \param vVector const std::vector<std::string>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::constructString(const std::vector<std::string>& vVector) const
{
    std::string sReturn = "";

    // Concatenate all elements in vVector
    for (size_t i = 0; i < vVector.size(); i++)
    {
        sReturn += vVector[i].substr(0, vVector[i].find('.')) + " ";
    }

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function splits the passed string
/// up into single string tokens.
///
/// \param sString std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReSyntax::splitString(std::string sString)
{
    std::vector<std::string> vReturn;

    // As long as the string has a length
    while (sString.length())
    {
        // Remove leading whitespaces
        while (sString.front() == ' ')
            sString.erase(0,1);

        // Separate at the whitespaces
        if (sString.find(' ') != std::string::npos)
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


/////////////////////////////////////////////////
/// \brief Converts a block def string into
/// actual syntax block definitions.
///
/// \param sDefString std::string
/// \return std::vector<SyntaxBlockDefinition>
///
/////////////////////////////////////////////////
std::vector<SyntaxBlockDefinition> NumeReSyntax::splitDefs(std::string sDefString)
{
    std::vector<SyntaxBlockDefinition> vDefs;

    EndlessVector<std::string> defs = getAllSemiColonSeparatedTokens(sDefString);

    for (size_t i = 0; i < defs.size(); i++)
    {
        std::vector<std::string> words = splitString(defs[i]);

        // Block is at least two words
        if (words.size() < 2)
            continue;

        SyntaxBlockDefinition definition;
        definition.startWord = words.front();
        definition.endWord = words.back();

        if (words.size() > 2)
            definition.middleWord1 = words[1];

        if (words.size() > 3)
            definition.middleWord2 = words[2];

        vDefs.push_back(definition);
    }

    return vDefs;
}


/////////////////////////////////////////////////
/// \brief This function searches for a match of
/// the passed string in the passed vector.
///
/// \param vVector const std::vector<std::string>&
/// \param sString const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReSyntax::matchItem(const std::vector<std::string>& vVector, const std::string& sString)
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


/////////////////////////////////////////////////
/// \brief This function applies the highlighting
/// colors to the command line (only used in the
/// terminal).
///
/// \param sCommandLine const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::highlightLine(const std::string& sCommandLine)
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
    std::string colors;

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
        && sCommandLine.substr(0,4) != "|TRY"
        && sCommandLine.substr(0,5) != "|CTCH"
        && sCommandLine.substr(0,4) != "|WHL")
        return colors;

    // Search for strings
    if (sCommandLine.find('"') != std::string::npos)
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
    if (sCommandLine.find('$') != std::string::npos)
    {
        int c_proc = '0'+SYNTAX_PROCEDURE;
        int c_normal = '0'+SYNTAX_STD;
        c = c_proc;

        // Apply the color from the dollar sign just up to the next whitespace or an opening parenthesis
        for (size_t k = sCommandLine.find('$'); k < sCommandLine.length(); k++)
        {
            if (sCommandLine[k] == '(' || sCommandLine[k] == ' ')
                c = c_normal;
            else if (sCommandLine[k] == '$')
                c = c_proc;

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
        else if (!i && (sCommandLine.substr(0,5) == "|-\?\?>"
                        || sCommandLine.substr(0,4) == "|FOR"
                        || sCommandLine.substr(0,5) == "|ELSE"
                        || sCommandLine.substr(0,5) == "|ELIF"
                        || sCommandLine.substr(0,4) == "|WHL"
                        || sCommandLine.substr(0,3) == "|IF"
                        || sCommandLine.substr(0,4) == "|TRY"
                        || sCommandLine.substr(0,5) == "|CTCH"
                        || sCommandLine.substr(0,5) == "|PROC"
                        || sCommandLine.substr(0,5) == "|COMP"))
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
                    || sCommandLine[i+nLen] == 'i'
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
            // Highlight line comments
            if (sCommandLine.substr(i, 2) == "##")
            {
                size_t len = sCommandLine.find("\\n", i+2);

                if (len == std::string::npos)
                {
                    len = colors.length() - i;
                    colors.replace(i, len, len, '0'+SYNTAX_COMMENT);
                    break;
                }
                else
                {
                    len -= i;
                    colors.replace(i, len, len, '0'+SYNTAX_COMMENT);
                    i = len;
                }

            }

            // Highlight block comments
            if (sCommandLine.substr(i, 2) == "#*")
            {
                size_t len = sCommandLine.find("*#", i+2);

                if (len == std::string::npos)
                {
                    len = colors.length() - i;
                    colors.replace(i, len, len, '0'+SYNTAX_COMMENT);
                    break;
                }
                else
                {
                    len -= i;
                    colors.replace(i, len+2, len+2, '0'+SYNTAX_COMMENT);
                    i = len+2;
                }

            }

            unsigned int nLen = 0;

            // Find the end of the current syntax element
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && sCommandLine[i+nLen] != ' '
                && sCommandLine[i+nLen] != '\''
                && sSingleOperators.find(sCommandLine[i+nLen]) == std::string::npos)
                nLen++;

            // color operators
            if (sSingleOperators.find(sCommandLine[i+nLen]) != std::string::npos)
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
                colors.replace(i, nLen, nLen, '0'+SYNTAX_COMMAND); // Changed for debug viewer
            }
            else if (i+nLen <= sCommandLine.length()
                && sCommandLine.find('.', i) < i+nLen)
            {
                // Methods on string variables or on data sets
                size_t nPos = sCommandLine.find('.', i)+1;

                for (size_t n = nPos; n < i+nLen; n++)
                {
                    if (sCommandLine[n] == '.')
                    {
                        if (matchItem(vMethods, sCommandLine.substr(nPos, n-nPos))
                            || matchItem(vMethodsArgs, sCommandLine.substr(nPos, n-nPos)))
                            colors.replace(nPos, n-nPos, n-nPos, '0'+SYNTAX_METHODS);
                        nPos = n+1;
                    }

                    if (n+1 == i+nLen)
                    {
                        if (matchItem(vMethods, sCommandLine.substr(nPos, n-nPos+1))
                            || matchItem(vMethodsArgs, sCommandLine.substr(nPos, n-nPos+1)))
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


/////////////////////////////////////////////////
/// \brief Highlight an error message. We simply
/// use the color of the operators (which is red
/// as default).
///
/// \param sCommandLine const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::highlightError(const std::string& sCommandLine)
{
    std::string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_OPERATOR);
    return colors;
}


/////////////////////////////////////////////////
/// \brief Highlight a warning message. We simply
/// use the color of numbers (which is orange as
/// default).
///
/// \param sCommandLine const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::highlightWarning(const std::string& sCommandLine)
{
    std::string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_NUMBER);
    return colors;
}


/////////////////////////////////////////////////
/// \brief This function returns the
/// autocompletion list for the editor.
///
/// \param sFirstChars std::string
/// \param useSmartSense bool
/// \param varType NumeReSyntax::SyntaxColors
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getAutoCompList(std::string sFirstChars, bool useSmartSense, NumeReSyntax::SyntaxColors varType)
{
    std::string sAutoCompList;

    // Only of the autocompletion map is not filled
    if (!mAutoCompList.size())
    {
        // Insert every element in the map
        for (size_t i = 0; i < vNSCRCommands.size(); i++)
            mAutoCompList[toLowerCase(vNSCRCommands[i])] = std::make_pair(vNSCRCommands[i] + "?" + toString((int)SYNTAX_COMMAND),
                                                                          SYNTAX_COMMAND);

        for (size_t i = 0; i < vNPRCCommands.size(); i++)
            mAutoCompList[toLowerCase(vNPRCCommands[i])] = std::make_pair(vNPRCCommands[i] + "?" + toString((int)SYNTAX_COMMAND),
                                                                          SYNTAX_COMMAND);

        for (size_t i = 0; i < vFunctions.size(); i++)
            mAutoCompList[toLowerCase(vFunctions[i])+"("] = std::make_pair(vFunctions[i]+"(?" + toString((int)SYNTAX_FUNCTION),
                                                                           SYNTAX_FUNCTION);

        for (size_t i = 0; i < vMethods.size(); i++)
            mAutoCompList[toLowerCase(vMethods[i])] = std::make_pair(vMethods[i].substr(0, vMethods[i].find('.'))+"?"+toString((int)SYNTAX_METHODS),
                                                                         SYNTAX_METHODS); // Methods shall not override functions

        for (size_t i = 0; i < vMethodsArgs.size(); i++)
            mAutoCompList[toLowerCase(vMethodsArgs[i])] = std::make_pair(vMethodsArgs[i].substr(0, vMethodsArgs[i].find('.'))+"(?"+toString((int)SYNTAX_METHODS),
                                                                              SYNTAX_METHODS); // Methods shall not override functions

        for (size_t i = 0; i < vOptions.size(); i++)
            mAutoCompList[toLowerCase(vOptions[i])] = std::make_pair(vOptions[i] + "?" + toString((int)SYNTAX_OPTION),
                                                                     SYNTAX_OPTION);

        for (size_t i = 0; i < vConstants.size(); i++)
            mAutoCompList[toLowerCase(vConstants[i])] = std::make_pair(vConstants[i] + "?" + toString((int)SYNTAX_CONSTANT),
                                                                       SYNTAX_CONSTANT);

        for (size_t i = 0; i < vSpecialValues.size(); i++)
        {
            if (vSpecialValues[i] == "ans")
                mAutoCompList["ans"] = std::make_pair("ans{?" + toString((int)SYNTAX_SPECIALVAL), SYNTAX_SPECIALVAL);
            else if (vSpecialValues[i] == "table" || vSpecialValues[i] == "data" || vSpecialValues[i] == "string")
                mAutoCompList[toLowerCase(vSpecialValues[i])] = std::make_pair(vSpecialValues[i] + "(?" + toString((int)SYNTAX_SPECIALVAL),
                                                                               SYNTAX_SPECIALVAL);
            else
                mAutoCompList[toLowerCase(vSpecialValues[i])] = std::make_pair(vSpecialValues[i] + "?" + toString((int)SYNTAX_SPECIALVAL),
                                                                               SYNTAX_SPECIALVAL);
        }

        for (size_t i = 0; i < vOperators.size(); i++)
            mAutoCompList[toLowerCase(vOperators[i])] = std::make_pair(vOperators[i] + "?" + toString((int)SYNTAX_OPERATOR),
                                                                       SYNTAX_OPERATOR);
    }

    // Transform the passed first characters to lower case to avoid case sensitivity
    sFirstChars = toLowerCase(sFirstChars);

    bool selectMethods = useSmartSense && sFirstChars.front() == '.';

    if (selectMethods)
        sFirstChars.erase(0, 1);

    bool selectAllMethods = selectMethods && !sFirstChars.length();

    // Try to find the correspondig elements in the map
    for (auto iter = mAutoCompList.begin(); iter != mAutoCompList.end(); ++iter)
    {
        if (selectAllMethods || (iter->first).front() == sFirstChars.front())
        {
            if (useSmartSense)
            {
                if (selectMethods && (iter->second.second != SYNTAX_METHODS
                                      || iter->first.find(varType == SYNTAX_TABLE ? ".tab" : ".str") == std::string::npos))
                    continue;
                else if (!selectMethods && iter->second.second == SYNTAX_METHODS)
                    continue;
            }

            if (sFirstChars == (iter->first).substr(0, sFirstChars.length()) || selectAllMethods)
                sAutoCompList += iter->second.first + " ";
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    // return the created list
    return sAutoCompList;
}


/////////////////////////////////////////////////
/// \brief The same as above but specialized for
/// MATLAB commands.
///
/// \param sFirstChars std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getAutoCompListMATLAB(std::string sFirstChars)
{
    std::string sAutoCompList;

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
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}


/////////////////////////////////////////////////
/// \brief The same as above but specialized for
/// C++ Commands.
///
/// \param sFirstChars std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getAutoCompListCPP(std::string sFirstChars)
{
    std::string sAutoCompList;

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
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}


/////////////////////////////////////////////////
/// \brief The same as above but specialized for
/// LaTeX Commands.
///
/// \param sFirstChars std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getAutoCompListTeX(std::string sFirstChars)
{
    std::string sAutoCompList;

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
                sAutoCompList += (iter->first).substr((iter->first).find('|')+1) + "?" + toString((int)(iter->second)) + " ";
        }
        else if ((iter->first).front() > sFirstChars.front())
            break;
    }

    return sAutoCompList;
}


/////////////////////////////////////////////////
/// \brief This function will return the
/// autocompletion list for the procedures based
/// upon the provided procedure tree.
///
/// \param sFirstChars std::string
/// \param sBaseNameSpace std::string
/// \param sSelectedNameSpace std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getProcAutoCompList(std::string sFirstChars, std::string sBaseNameSpace, std::string sSelectedNameSpace)
{
    if (!vProcedureTree.size())
        return "";

    std::string sProcName;
    static std::string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};

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

    sFirstChars = toLowerCase(sFirstChars);
    sProcName = toLowerCase(sProcName);

    std::string sAutoCompList = " ";

    // If no namespace was pre-selected, provide the standard namespaces
    if (!sSelectedNameSpace.length())
    {
        for (size_t i = 0; i < 3; i++)
        {
            if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
                sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
        }
    }

    std::string sToken;

    // Go through the complete procedure tree
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        // If the procedure in the tree corresponds to the passed procedure fragment
        if (toLowerCase(vProcedureTree[i].substr(0, sProcName.length())) == sProcName)
        {
            // Only add the namespace if it's needed
            if (sSelectedNameSpace.length())
            {
                sToken = vProcedureTree[i].substr(sSelectedNameSpace.length());

                if (sToken.find('~', sFirstChars.length()) != std::string::npos)
                {
                    sToken.erase(sToken.find('~', sFirstChars.length())+1);
                    sToken += "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
                }
                else
                    sToken += "(?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            }
            else if (vProcedureTree[i].find('~', sProcName.length()) != std::string::npos)
                sToken = vProcedureTree[i].substr(0, vProcedureTree[i].find('~', sFirstChars.length())+1) + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
            else
                sToken = vProcedureTree[i] + "(?" + toString((int)(SYNTAX_PROCEDURE)) + " ";

            // Remove the base namespace, if it is available in the current token
            if (!sSelectedNameSpace.length())
            {
                if (toLowerCase(sToken.substr(0, sBaseNameSpace.length())) == toLowerCase(sBaseNameSpace))
                    sToken.erase(0, sBaseNameSpace.length());

                if (sToken.front() == '~')
                    sToken.erase(0,1);
            }

            // Only add the current element to the autocompletion list, if it is not already available
            if (sAutoCompList.find(" " + sToken) == std::string::npos)
                sAutoCompList += sToken;
        }
    }

    // Return the autocompletion list (the first character is always a whitespace character)
    return sAutoCompList.substr(1);
}


/////////////////////////////////////////////////
/// \brief This function returns the
/// autocompletion list for the namespaces.
///
/// \param sFirstChars std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReSyntax::getNameSpaceAutoCompList(std::string sFirstChars)
{
    if (!vProcedureTree.size())
        return "";

    std::string sProcName;
    static std::string sStandardNamespaces[] = {"main~", "this~", "thisfile~"};
    sProcName = sFirstChars;
    std::string sAutoCompList = " ";

    // Provide the standard namespaces first
    for (size_t i = 0; i < 3; i++)
    {
        if (sStandardNamespaces[i].substr(0, sFirstChars.length()) == sFirstChars)
            sAutoCompList += sStandardNamespaces[i] + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";
    }

    // Append all available namespaces
    std::string sToken;

    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        if (vProcedureTree[i].substr(0, sProcName.length()) == sProcName)
        {
            if (vProcedureTree[i].find('~', sProcName.length()) != std::string::npos)
                sToken = vProcedureTree[i].substr(0, vProcedureTree[i].find('~', sProcName.length())+1) + "?" + toString((int)(SYNTAX_PROCEDURE)) + " ";

            if (sAutoCompList.find(" " + sToken) == std::string::npos)
                sAutoCompList += sToken;
        }
    }

    // Return the autocompletion list (the first character is always a whitespace character)
    return sAutoCompList.substr(1);
}


