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
#include <boost/nowide/fstream.hpp>

/////////////////////////////////////////////////
/// \brief A simple structure to define a syntax
/// block for folding, etc.
/////////////////////////////////////////////////
struct SyntaxBlockDefinition
{
    std::string startWord;
    std::string endWord;
    std::string middleWord1;
    std::string middleWord2;
};



/////////////////////////////////////////////////
/// \brief This class contains all needed
/// keywords to highlight their occurences
/// correspondingly. It will pass them to the
/// NumeReEditor instances. For the terminal,
/// this class acts as lexer. It will also
/// provide autocompletion lists to be used by
/// both, the NumeReEditor instances and the
/// terminal.
/////////////////////////////////////////////////
class NumeReSyntax
{
    private:
        std::vector<std::string> vNSCRCommands;
        std::vector<std::string> vNPRCCommands;
        std::vector<std::string> vOptions;
        std::vector<std::string> vFunctions;
        std::vector<std::string> vMethods;
        std::vector<std::string> vMethodsArgs;
        std::vector<std::string> vConstants;
        std::vector<std::string> vSpecialValues;
        std::vector<std::string> vOperators;
        std::vector<std::string> vDocKeyWords;
        std::vector<SyntaxBlockDefinition> vBlockDefs;
        std::vector<std::string> vMatlabKeyWords;
        std::vector<std::string> vMatlabFunctions;
        std::vector<std::string> vCppKeyWords;
        std::vector<std::string> vCppFunctions;
        std::vector<std::string> vTeXKeyWords;

        std::vector<std::string> vProcedureTree;

        std::string sSingleOperators;
        std::map<std::string, std::pair<std::string, int>> mAutoCompList;
        std::map<std::string, int> mAutoCompListMATLAB;
        std::map<std::string, int> mAutoCompListCPP;
        std::map<std::string, int> mAutoCompListTeX;

        std::string sPath;

        std::string constructString(const std::vector<std::string>& vVector) const;
        std::vector<std::string> splitString(std::string sString);
        std::vector<SyntaxBlockDefinition> splitDefs(std::string sDefString);
        bool matchItem(const std::vector<std::string>& vVector, const std::string& sString);
        void prepareAutoCompMaps();
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
            SYNTAX_NUMBER,
            SYNTAX_NPRC_COMMAND,
            SYNTAX_METHODS,
            SYNTAX_COMMENT,
            SYNTAX_CLUSTER,
            SYNTAX_TABLE,
            SYNTAX_CATEGORY
        };

        NumeReSyntax();
        NumeReSyntax(const std::string& _sPath);
        NumeReSyntax(const std::string& _sPath, const std::vector<std::string>& vPlugins);

        void loadSyntax(const std::string& _sPath = "");
        void addPlugins(const std::vector<std::string>& vPlugins);
        void setProcedureTree(const std::vector<std::string>& vTree);

        static std::string mergeAutoCompleteLists(std::string sPreDefList, std::string sScopedList);

        std::string getCommands() const
            {return constructString(vNSCRCommands);}
        std::string getNPRCCommands() const
            {return constructString(vNPRCCommands);}
        std::string getOptions() const
            {return constructString(vOptions);}
        std::string getFunctions() const
            {return constructString(vFunctions);}
        std::string getMethods() const
            {return constructString(vMethods) + " " + constructString(vMethodsArgs);}
        std::string getNoArgMethods() const
            {return constructString(vMethods);}
        std::string getConstants() const
            {return constructString(vConstants);}
        std::string getSpecial() const
            {return constructString(vSpecialValues);}
        std::string getOperators() const
            {return constructString(vOperators);}
        std::string getDocKeyWords() const
            {return constructString(vDocKeyWords);}
        std::string getBlockDefs() const;
        std::string getMatlab() const
            {return constructString(vMatlabKeyWords);}
        std::string getMatlabFunctions() const
            {return constructString(vMatlabFunctions);}
        std::string getCpp() const
            {return constructString(vCppKeyWords);}
        std::string getCppFunctions() const
            {return constructString(vCppFunctions);}
        std::string getAutoCompList(std::string sFirstChars, bool useSmartSense, const std::string& varType = "", bool isVect = false) const;
        std::string getAutoCompListMATLAB(std::string sFirstChars) const;
        std::string getAutoCompListCPP(std::string sFirstChars) const;
        std::string getAutoCompListTeX(std::string sFirstChars) const;
        std::string getProcAutoCompList(std::string sFirstChars, std::string sBaseNameSpace, std::string sSelectedNameSpace) const;
        std::string getNameSpaceAutoCompList(std::string sFirstChars) const;

        std::string highlightLine(const std::string& sCommandLine);
        std::string highlightError(const std::string& sCommandLine);
        std::string highlightWarning(const std::string& sCommandLine);

        std::vector<SyntaxBlockDefinition> getFullBlockDefs() const
            {return vBlockDefs;}

};

#endif // SYNTAX_HPP

