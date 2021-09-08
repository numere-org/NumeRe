/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "stringlogicparser.hpp"
#include "../../kernel.hpp"

string removeMaskedStrings(const string& sString);

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This member function will evaluate the
    /// ternary operator for strings.
    ///
    /// \param sLine string
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringLogicParser::evalStringTernary(string sLine)
    {
        bool bReturningLogicals = false;
        size_t nPos = 0;

        // As long as there's a question mark in the current string
        while (sLine.find('?', nPos) != string::npos)
        {
            nPos = sLine.find('?', nPos);

            // Ensure, that we're not in a string literal
            if (!isInQuotes(sLine, nPos))
            {
                // Get the current ternary as a vector
                vector<string> vTernary = getStringTernaryExpression(sLine, nPos);
                nPos = 0;

                // Evaluate logical string expressions
                vTernary[0] = evalStringLogic(vTernary[0], bReturningLogicals);
                bool result = false;

                // Evaluate the condition of the ternary logically
                if (vTernary[0].find('"') != string::npos)
                {
                    StripSpaces(vTernary[0]);
                    result = (bool)(vTernary[0].length() - 2);
                }
                else
                {
                    NumeReKernel::getInstance()->getParser().SetExpr(vTernary[0]);
                    result = NumeReKernel::getInstance()->getParser().Eval() != 0.0;
                }

                // return the evaluated part of the string
                if (result)
                    return sLine + evalStringLogic(vTernary[1], bReturningLogicals);
                else
                    return sLine + evalStringLogic(vTernary[2], bReturningLogicals);
            }
            else
                nPos++;
        }

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringLogicParser::detectStringLogicals().
    ///
    /// \param sString const string&
    /// \param nPos size_t
    /// \return size_t
    ///
    /// This function will identify path tokens, which
    /// will start at the passed position.
    /////////////////////////////////////////////////
    size_t StringLogicParser::detectPathTokens(const string& sString, size_t nPos)
    {
        if (sString.substr(nPos, 2) == "<>")
            return 1u;

        if (sString.substr(nPos, 4) == "<wp>")
            return 3u;

        if (sString.substr(nPos, 6) == "<this>")
            return 5u;

        if (sString.substr(nPos, 10) == "<loadpath>")
            return 9u;

        if (sString.substr(nPos, 10) == "<savepath>")
            return 9u;

        if (sString.substr(nPos, 10) == "<procpath>")
            return 9u;

        if (sString.substr(nPos, 10) == "<plotpath>")
            return 9u;

        if (sString.substr(nPos, 12) == "<scriptpath>")
            return 11u;

        return 0u;
    }


    /////////////////////////////////////////////////
    /// \brief Removes masked strings, strips spaces
    /// and removes surrounding quotation marks and
    /// concatenates the values, if necessary.
    ///
    /// \param _sLine const std::string&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringLogicParser::prepareComparisonValues(const std::string& _sLine)
    {
        bool bReturningLogicals = false;

        string sLine = removeMaskedStrings(evalStringLogic(_sLine, bReturningLogicals));
        StripSpaces(sLine);

        if (sLine.front() == '"' && sLine.back() == '"')
        {
            concatenateStrings(sLine);
            sLine = sLine.substr(1, sLine.length() - 2);
        }

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringLogicParser::evalStringLogic().
    ///
    /// \param sLine string&
    /// \param nPos size_t&
    /// \return vector<string>
    ///
    /// It will return the expression parts of the
    /// current ternary at \c nPos in the components
    /// of the returned vector. \c nPos ist the
    /// position of the question mark.
    /////////////////////////////////////////////////
    vector<string> StringLogicParser::getStringTernaryExpression(string& sLine, size_t& nPos)
    {
        vector<string> vTernary;
        size_t nTernaryStart = 0;
        size_t nColonPosition = 0;

        string sTernary = sLine.substr(nTernaryStart);
        sLine.erase(nTernaryStart);

        size_t quotes = 0;
        int nQuestionMarks = 0;

        // Search for the operators of the ternary
        // Jump over additional quotation marks
        for (size_t i = nPos; i < sTernary.length(); i++)
        {
            // Jump ver parentheses
            if (!(quotes % 2) && (sTernary[i] == '(' || sTernary[i] == '[' || sTernary[i] == '{'))
                i += getMatchingParenthesis(sTernary.substr(i));

            // Increment the question mark counter
            if (!(quotes % 2) && sTernary[i] == '?')
                nQuestionMarks++;

            // Increment the quotation mark counter
            if (sTernary[i] == '"' && sTernary[i - 1] != '\\')
                quotes++;

            // If there's a colon, decrement the quotation mark
            // counter
            if (!(quotes % 2) && sTernary[i] == ':')
            {
                nQuestionMarks--;
                if (!nQuestionMarks)
                {
                    // this is the correct colon
                    nColonPosition = i;
                    break;
                }
            }
        }

        // This is obviously not a real ternary
        if (!nColonPosition)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, nPos, sTernary);

        // Distribute the expression parts of the ternary
        // across the vector components
        vTernary.push_back(sTernary.substr(0, nPos));
        vTernary.push_back(sTernary.substr(nPos + 1, nColonPosition - 1 - nPos));
        vTernary.push_back(sTernary.substr(nColonPosition + 1));

        nPos = nTernaryStart;
        return vTernary;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is may detect
    /// logical expressions in the passed string
    /// expression.
    ///
    /// \param sString const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringLogicParser::detectStringLogicals(const string& sString)
    {
        if (!sString.length())
            return false;

        int nQuotes = 0;

        for (size_t i = 0; i < sString.length(); i++)
        {
            // Count quotation marks
            if (sString[i] == '"' && (!i || sString[i - 1] != '\\'))
            {
                nQuotes++;
                continue;
            }

            // Detect path tokens
            if (sString[i] == '<')
            {
                // Search for path tokens
                size_t nAdv = detectPathTokens(sString, i);

                if (nAdv)
                {
                    i += nAdv;
                    continue;
                }
            }

            // Logicals are only possible outside of string literals
            if (!(nQuotes % 2))
            {
                if (sString.substr(i, 2) == "&&"
                        || sString.substr(i, 2) == "||"
                        || sString.substr(i, 2) == "<="
                        || sString.substr(i, 2) == ">="
                        || sString.substr(i, 2) == "!="
                        || sString.substr(i, 2) == "=="
                        || sString[i] == '?'
                        || sString[i] == '<'
                        || sString[i] == '>')
                    return true;
            }
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will evaluate
    /// logical string expressions in the passed
    /// command line.
    ///
    /// \param sLine string
    /// \param bReturningLogicals bool&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringLogicParser::evalStringLogic(string sLine, bool& bReturningLogicals)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();

        // Exclude border cases
        if (!sLine.length())
            return "false";

        if (sLine.find('"') == string::npos)
        {
            bReturningLogicals = true;
            return sLine;
        }

        sLine += " ";

        // Evaluate ternaries first (will start a recursion)
        sLine = evalStringTernary(sLine);

        unsigned int nPos;

        // Handle parenthesed expression parts
        // (will start a recursion)
        if (sLine.find('(') != string::npos)
        {
            nPos = 0;

            while (sLine.find('(', nPos) != string::npos)
            {
                nPos = sLine.find('(', nPos) + 1;

                if (!isInQuotes(sLine, nPos - 1))
                {
                    sLine = sLine.substr(0, nPos - 1)
                            + evalStringLogic(sLine.substr(nPos, getMatchingParenthesis(sLine.substr(nPos - 1)) - 1), bReturningLogicals)
                            + sLine.substr(getMatchingParenthesis(sLine.substr(nPos - 1)) + nPos);
                    nPos = 0;
                }
            }
        }

        // Handle the logical and
        if (sLine.find("&&") != string::npos)
        {
            nPos = 0;

            while (sLine.find("&&", nPos) != string::npos)
            {
                nPos = sLine.find("&&", nPos) + 2;

                if (!isInQuotes(sLine, nPos - 2))
                {
                    string sLeft = prepareComparisonValues(sLine.substr(0, nPos-2));
                    string sRight = prepareComparisonValues(sLine.substr(nPos));

                    bReturningLogicals = true;

                    if (sLeft == "true" && sRight == "true")
                        return "true";
                    else if (sLeft == "false" || sRight == "false")
                        return "false";
                    else
                    {
                        _parser.SetExpr(sLeft + " && " + sRight);
                        return toString(_parser.Eval() != 0.0);
                    }
                }
            }
        }

        // Handle the exclusive or
        if (sLine.find("|||") != string::npos)
        {
            nPos = 0;

            while (sLine.find("|||", nPos) != string::npos)
            {
                nPos = sLine.find("|||", nPos) + 3;

                if (!isInQuotes(sLine, nPos - 3))
                {
                    string sLeft = prepareComparisonValues(sLine.substr(0, nPos-3));
                    string sRight = prepareComparisonValues(sLine.substr(nPos));

                    bReturningLogicals = true;

                    if ((sLeft == "true" && sRight == "false") || (sLeft == "false" && sRight == "true"))
                        return "true";
                    else if (sLeft == sRight)
                        return "false";
                    else
                    {
                        _parser.SetExpr(sLeft + " ||| " + sRight);
                        return toString(_parser.Eval() != 0.0);
                    }
                }
            }
        }

        // Handle the logical or
        if (sLine.find("||") != string::npos)
        {
            nPos = 0;

            while (sLine.find("||", nPos) != string::npos)
            {
                nPos = sLine.find("||", nPos) + 2;

                if (!isInQuotes(sLine, nPos - 2))
                {
                    string sLeft = prepareComparisonValues(sLine.substr(0, nPos-2));
                    string sRight = prepareComparisonValues(sLine.substr(nPos));

                    bReturningLogicals = true;

                    if (sLeft == "true" || sRight == "true")
                        return "true";
                    else if (sLeft == "false" && sRight == "false")
                        return "false";
                    else
                    {
                        _parser.SetExpr(sLeft + " || " + sRight);
                        return toString(_parser.Eval() != 0.0);
                    }
                }
            }
        }

        int nQuotes = 0;

        // Handle logical comparisons
        for (size_t i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '"' && (!i || sLine[i - 1] != '\\'))
                nQuotes++;

            if (!(nQuotes % 2))
            {
                if (sLine.substr(i, 2) == "==")
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) == prepareComparisonValues(sLine.substr(i + 2)));
                }
                else if (sLine.substr(i, 2) == "!=")
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) != prepareComparisonValues(sLine.substr(i + 2)));
                }
                else if (sLine.substr(i, 2) == "<=")
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) <= prepareComparisonValues(sLine.substr(i + 2)));
                }
                else if (sLine.substr(i, 2) == ">=")
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) >= prepareComparisonValues(sLine.substr(i + 2)));
                }
                else if (sLine[i] == '<')
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) < prepareComparisonValues(sLine.substr(i + 1)));
                }
                else if (sLine[i] == '>')
                {
                    bReturningLogicals = true;
                    return toString(prepareComparisonValues(sLine.substr(0, i)) > prepareComparisonValues(sLine.substr(i + 1)));
                }

            }
        }

        StripSpaces(sLine);
        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function performs the
    /// actual string concatenation of the passed
    /// string expression.
    ///
    /// \param sExpr string&
    /// \return void
    ///
    /// This member is called by
    /// StringParser::applyElementaryStringOperations()
    /// for the concatenation.
    /////////////////////////////////////////////////
    void StringLogicParser::concatenateStrings(string& sExpr)
    {
        size_t nQuotes = 0;

        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (sExpr[i] == '"' && (!i || sExpr[i-1] != '\\'))
                nQuotes++;

            // Search for the concatenation operator (aka "+")
            if (!(nQuotes % 2) && sExpr[i] == '+')
            {
                string sLeft = sExpr.substr(0, i);
                string sRight = sExpr.substr(i+1);

                StripSpaces(sLeft);
                StripSpaces(sRight);

                // Determine the correct concatenation process
                if (sLeft == "\"\"" && sRight != "\"\"")
                {
                    sExpr = " " + sRight;
                    i = 0;
                }
                else if (sLeft != "\"\"" && sRight == "\"\"")
                {
                    sExpr = sLeft;
                    break;
                }
                else if (sLeft.back() == '"' && sRight.front() == '"')
                {
                    sExpr = sLeft.substr(0, sLeft.length()-1) + sRight.substr(1);

                    // We removed some characters
                    i = sLeft.length()-2;

                    // We're now part of a string
                    nQuotes++;
                }

                // Everything not catched here is a strange mixture
            }
            else if (!(nQuotes % 2) && isalnum(sExpr[i])) // Unexpected variables or literals
                throw SyntaxError(SyntaxError::STRING_ERROR, sExpr, i, sExpr);
        }

        StripSpaces(sExpr);
    }
}

