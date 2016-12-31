#ifndef SYNTAX_HPP
#define SYNTAX_HPP

#include <string>
#include <vector>

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

        string highlightLine(const string& sCommandLine);

};
#endif // SYNTAX_HPP

