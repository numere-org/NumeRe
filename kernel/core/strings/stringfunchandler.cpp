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

#include "stringfunchandler.hpp"
#include "../../kernel.hpp"
#define DEFAULT_NUM_ARG INT_MIN
// define the "End of transmission block" as string separator
#define NEWSTRING (char)23

string removeQuotationMarks(const string& sString);
string addQuotationMarks(const string& sString);

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This member function adds escape
    /// characters into the passed string and
    /// transforms the newstring character into a
    /// comma.
    ///
    /// \param sString const string&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringFuncHandler::addMaskedStrings(const string& sString)
    {
        if (sString.find('"') == string::npos && sString.find(NEWSTRING) == string::npos && sString.back() != '\\')
            return sString;
        string sRet = sString;

        // Go through the complete string without the first
        // and the last character
        for (size_t i = 1; i < sRet.length() - 1; i++)
        {
            // Escape backslashes
            if (sRet[i] == '\\' && sRet[i + 1] != '"' && sRet[i + 1] != ' ')
            {
                sRet.insert(i + 1, " ");
                i++;
            }

            // Escape quotation marks
            if (sRet[i] == '"' && sRet[i - 1] != '\\' && sRet[i + 1] != NEWSTRING && sRet.find('"', i + 1) != string::npos)
            {
                sRet.insert(i, "\\");
                i++;
            }

            // Replace the new string character with a comma
            if (sRet[i] == NEWSTRING)
            {
                sRet[i] = ',';
                if (sRet[i + 1] == '"')
                    i++;
            }

            // Escape tab and newlines
            if (sRet[i] == '\t')
                sRet.replace(i, 1, "\\t");
            if (sRet[i] == '\n')
                sRet.replace(i, 1, "\\n");
        }
        return sRet;
    }


    /////////////////////////////////////////////////
    /// \brief This member function evaluates the
    /// passed call sequence to the string function.
    ///
    /// \param sLine string&
    /// \param sFuncName const string&
    /// \param funcHandle StringFuncHandle
    /// \return void
    ///
    /// The arguments of the string function are
    /// evaluated depending on the defined signature of
    /// the called string function and passed to the
    /// linked implementation of the string function.
    /// The return value of the string function is
    /// transformed back into a usual string
    /// expression afterwards and stored in a string
    /// vector, which replaces the selected call to
    /// the string function.
    /////////////////////////////////////////////////
    void StringFuncHandler::evalFunction(string& sLine, const string& sFuncName, StringFuncHandle funcHandle)
    {
        size_t nStartPosition = 0;
        size_t nEndPosition = 0;

        // While the function signature can be found
        while ((nStartPosition = findNextFunction(sFuncName, sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            // Extract the argument of the current found function and process it
            string sFunctionArgument = getFunctionArgumentList(sFuncName, sLine, nStartPosition, nEndPosition);
            vector<string> vReturnValues;
            StringFuncArgs stringArgs;
            stringArgs.opt = &NumeReKernel::getInstance()->getSettings();
            bool bLogicalOnly = false;

            // Apply the parser as specified by the function signature. After that call the corresponding
            // string function with the returned arguments as many times as it's needed
            if (funcHandle.fType >= PARSER_INT && funcHandle.fType < PARSER_STRING)
            {
                n_vect nIntArg;
                size_t nMaxArgs = argumentParser(sFunctionArgument, nIntArg);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                if (funcHandle.bTakesMultiArguments)
                {
                    stringArgs.nMultiArg = nIntArg;
                    vReturnValues.push_back(funcHandle.fHandle(stringArgs));
                }
                else
                {
                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        stringArgs.nArg1 = nIntArg[i];
                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
            }
            else if (funcHandle.fType >= PARSER_STRING && funcHandle.fType < PARSER_STRING_INT_INT)
            {
                s_vect sStringArg;
                size_t nMaxArgs = 0;

                if (sFuncName == "to_string(" && !containsStrings(sFunctionArgument))
                {
                    sStringArg.push_back(sFunctionArgument);
                    nMaxArgs = 1;
                }
                else
                    nMaxArgs = argumentParser(sFunctionArgument, sStringArg, bLogicalOnly);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // These five multiargument functions are also defined for numerical values.
                // If the return value for the current functions arguments is an only logical
                // value, ignore the current function call
                if (bLogicalOnly
                    && (sFuncName == "min(" || sFuncName == "max(" || sFuncName == "cnt(" || sFuncName == "num(" || sFuncName == "sum("))
                {
                    nStartPosition++;
                    continue;
                }

                if (funcHandle.bTakesMultiArguments)
                {
                    stringArgs.sMultiArg = sStringArg;
                    vReturnValues.push_back(funcHandle.fHandle(stringArgs));
                }
                else
                {
                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        stringArgs.sArg1 = sStringArg[i];
                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
            }
            else if (funcHandle.fType >= PARSER_STRING_INT_INT && funcHandle.fType < PARSER_STRING_INT_INT_STRING)
            {
                s_vect sStringArg;
                n_vect nIntArg1, nIntArg2;
                size_t nMaxArgs = argumentParser(sFunctionArgument, sStringArg, nIntArg1, nIntArg2);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                for (size_t i = 0; i < nMaxArgs; i++)
                {
                    if (i < sStringArg.size())
                        stringArgs.sArg1 = sStringArg[i];
                    else if (sStringArg.size() == 1)
                        stringArgs.sArg1 = sStringArg[0];
                    else
                        stringArgs.sArg1 = "";

                    if (i < nIntArg1.size())
                        stringArgs.nArg1 = nIntArg1[i];
                    else if (nIntArg1.size() == 1)
                        stringArgs.nArg1 = nIntArg1[0];
                    else
                        stringArgs.nArg1 = DEFAULT_NUM_ARG;

                    if (i < nIntArg2.size())
                        stringArgs.nArg2 = nIntArg2[i];
                    else if (nIntArg2.size() == 1)
                        stringArgs.nArg2 = nIntArg2[0];
                    else
                        stringArgs.nArg2 = DEFAULT_NUM_ARG;

                    vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                }

            }
            else if (funcHandle.fType >= PARSER_STRING_INT_INT_STRING && funcHandle.fType < PARSER_STRING_STRING_INT_INT)
            {
                s_vect sStringArg1, sStringArg2;
                n_vect nIntArg1, nIntArg2;
                size_t nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, nIntArg1, nIntArg2, sStringArg2);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                for (size_t i = 0; i < nMaxArgs; i++)
                {
                    if (i < sStringArg1.size())
                        stringArgs.sArg1 = sStringArg1[i];
                    else if (sStringArg1.size() == 1)
                        stringArgs.sArg1 = sStringArg1[0];
                    else
                        stringArgs.sArg1 = "";

                    if (i < sStringArg2.size())
                        stringArgs.sArg2 = sStringArg2[i];
                    else if (sStringArg2.size() == 1)
                        stringArgs.sArg2 = sStringArg2[0];
                    else
                        stringArgs.sArg2 = "";

                    if (i < nIntArg1.size())
                        stringArgs.nArg1 = nIntArg1[i];
                    else if (nIntArg1.size() == 1)
                        stringArgs.nArg1 = nIntArg1[0];
                    else
                        stringArgs.nArg1 = DEFAULT_NUM_ARG;

                    if (i < nIntArg2.size())
                        stringArgs.nArg2 = nIntArg2[i];
                    else if (nIntArg2.size() == 1)
                        stringArgs.nArg2 = nIntArg2[0];
                    else
                        stringArgs.nArg2 = DEFAULT_NUM_ARG;

                    vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                }
            }
            else if (funcHandle.fType >= PARSER_STRING_STRING_INT_INT && funcHandle.fType < PARSER_STRING_STRING_STRING_INT_INT)
            {
                s_vect sStringArg1, sStringArg2;
                n_vect nIntArg1, nIntArg2;
                size_t nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, sStringArg2, nIntArg1, nIntArg2);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                if (funcHandle.bTakesMultiArguments)
                {
                    stringArgs.sMultiArg = sStringArg1;
                    nMaxArgs = max(max(sStringArg2.size(), nIntArg1.size()), nIntArg2.size());

                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
                else
                {
                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg1.size())
                            stringArgs.sArg1 = sStringArg1[i];
                        else if (sStringArg1.size() == 1)
                            stringArgs.sArg1 = sStringArg1[0];
                        else
                            stringArgs.sArg1 = "";

                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
            }
            else if (funcHandle.fType >= PARSER_STRING_STRING_STRING_INT_INT)
            {
                s_vect sStringArg1, sStringArg2, sStringArg3;
                n_vect nIntArg1, nIntArg2;
                size_t nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2);

                if (!nMaxArgs)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                if (funcHandle.bTakesMultiArguments)
                {
                    stringArgs.sMultiArg = sStringArg1;
                    nMaxArgs = max(max(max(sStringArg2.size(), sStringArg3.size()), nIntArg1.size()), nIntArg2.size());

                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < sStringArg3.size())
                            stringArgs.sArg3 = sStringArg3[i];
                        else if (sStringArg3.size() == 1)
                            stringArgs.sArg3 = sStringArg3[0];
                        else
                            stringArgs.sArg3 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
                else
                {
                    for (size_t i = 0; i < nMaxArgs; i++)
                    {
                        if (i < sStringArg1.size())
                            stringArgs.sArg1 = sStringArg1[i];
                        else if (sStringArg1.size() == 1)
                            stringArgs.sArg1 = sStringArg1[0];
                        else
                            stringArgs.sArg1 = "";

                        if (i < sStringArg2.size())
                            stringArgs.sArg2 = sStringArg2[i];
                        else if (sStringArg2.size() == 1)
                            stringArgs.sArg2 = sStringArg2[0];
                        else
                            stringArgs.sArg2 = "";

                        if (i < sStringArg3.size())
                            stringArgs.sArg3 = sStringArg3[i];
                        else if (sStringArg3.size() == 1)
                            stringArgs.sArg3 = sStringArg3[0];
                        else
                            stringArgs.sArg3 = "";

                        if (i < nIntArg1.size())
                            stringArgs.nArg1 = nIntArg1[i];
                        else if (nIntArg1.size() == 1)
                            stringArgs.nArg1 = nIntArg1[0];
                        else
                            stringArgs.nArg1 = DEFAULT_NUM_ARG;

                        if (i < nIntArg2.size())
                            stringArgs.nArg2 = nIntArg2[i];
                        else if (nIntArg2.size() == 1)
                            stringArgs.nArg2 = nIntArg2[0];
                        else
                            stringArgs.nArg2 = DEFAULT_NUM_ARG;

                        vReturnValues.push_back(addMaskedStrings(funcHandle.fHandle(stringArgs)));
                    }
                }
            }

            // copy the return values to the final variable
            string sFuncReturnValue = "";

            // Expand the string vector component, if needed
            expandStringVectorComponents(vReturnValues);

            // Create a string vector variable for the function output
            sFuncReturnValue = createStringVectorVar(vReturnValues);

            // replace the function with the return value
            sLine.replace(nStartPosition, nEndPosition + 1 - nStartPosition, sFuncReturnValue);

            nStartPosition++;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for numerical
    /// arguments.
    ///
    /// \param __sFuncArgument const string&
    /// \param nArg n_vect& a vector of numerical
    /// values as return value
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, n_vect& nArg)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        Datafile& _data = NumeReKernel::getInstance()->getData();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        string sFuncArgument = __sFuncArgument;
        value_type* v = 0;
        int nReturn = 0;

        // If the current function argument contains strings,
        // parse it correspondingly
        if (isStringExpression(sFuncArgument) || _data.containsClusters(sFuncArgument))
        {
            // Call the string parser core
            StringResult strRes = eval(sFuncArgument, "", true);

            // If already numerical value are available, use them directly
            if (strRes.vNumericalValues.size())
            {
                for (size_t i = 0; i < strRes.vNumericalValues.size(); i++)
                    nArg.push_back(intCast(strRes.vNumericalValues[i]));

                return nArg.size();
            }

            // Evaluate the returned strings numerically
            for (size_t i = 0; i < strRes.vResult.size(); i++)
            {
                _parser.SetExpr(strRes.vResult[i]);
                v = _parser.Eval(nReturn);

                for (int n = 0; n < nReturn; n++)
                    nArg.push_back(intCast(v[n]));
            }

            return nArg.size();
        }
        else if (_data.containsTablesOrClusters(sFuncArgument))
        {
            getDataElements(sFuncArgument, _parser, _data, _option, false);
        }

        // Set the expression and evaluate it
        _parser.SetExpr(sFuncArgument);
        v = _parser.Eval(nReturn);

        for (int i = 0; i < nReturn; i++)
        {
            nArg.push_back(intCast(v[i]));
        }

        return (size_t)nReturn;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for string
    /// arguments.
    ///
    /// \param __sFuncArgument const string&
    /// \param sArg s_vect& a vector of string values
    /// as return value
    /// \param bLogicalOnly bool&
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, s_vect& sArg, bool& bLogicalOnly)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        Datafile& _data = NumeReKernel::getInstance()->getData();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        string sFuncArgument = __sFuncArgument;

        // If the current function argument contains strings,
        // parse it correspondingly
        if (isStringExpression(sFuncArgument) || _data.containsClusters(sFuncArgument))
        {
            // Call the string parser core
            StringResult strRes = eval(sFuncArgument, "", true);

            // Use the returned values as function arguments
            for (size_t i = 0; i < strRes.vResult.size(); i++)
                sArg.push_back(removeQuotationMarks(strRes.vResult[i]));

            bLogicalOnly = strRes.bOnlyLogicals;
            return strRes.vResult.size();
        }
        else if (_data.containsTablesOrClusters(sFuncArgument))
        {
            getDataElements(sFuncArgument, _parser, _data, _option, false);
        }

        // Expand the passed argument, if needed and
        // distribute it to the components of the argument vector
        if (sFuncArgument.find('{') != string::npos || sFuncArgument.find(',') != string::npos)
        {
            convertVectorToExpression(sFuncArgument, _option);

            // As long as the function argument has a length,
            // get the next argument and store it in the vector
            while (sFuncArgument.length())
                sArg.push_back(removeQuotationMarks(getNextArgument(sFuncArgument, true)));
        }
        else
            sArg.push_back(removeQuotationMarks(sFuncArgument));

        // Declare argument as numerical only
        bLogicalOnly = true;

        return sArg.size();
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for one string and
    /// two (optional) numerical arguments.
    ///
    /// \param __sFuncArgument const string&
    /// \param sArg1 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2)
    {
        string sFuncArgument = __sFuncArgument;
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        string sString = getNextArgument(sFuncArgument, true);
        string sNumVal1 = getNextArgument(sFuncArgument, true);
        string sNumVal2 = getNextArgument(sFuncArgument, true);

        // Handle the arguments using the basic functions
        // and store the highets number of return values
        nMaxLength = argumentParser(sString, sArg1, bLogicalOnly);

        if (!nMaxLength)
            return 0;

        if (sNumVal1.length())
        {
            size_t nReturn = argumentParser(sNumVal1, nArg1);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal2.length())
        {
            size_t nReturn = argumentParser(sNumVal2, nArg2);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for two string
    /// and two numerical values, where the second
    /// string is the last argument. Every argument
    /// except of the first one is optional.
    ///
    /// \param __sFuncArgument const string&
    /// \param sArg1 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \param sArg2 s_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2, s_vect& sArg2)
    {
        string sFuncArgument = __sFuncArgument;
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        string sString1 = getNextArgument(sFuncArgument, true);
        string sNumVal1 = getNextArgument(sFuncArgument, true);
        string sNumVal2 = getNextArgument(sFuncArgument, true);
        string sString2 = getNextArgument(sFuncArgument, true);

        // Handle the arguments using the basic functions
        // and store the highets number of return values
        nMaxLength = argumentParser(sString1, sArg1, bLogicalOnly);

        if (!nMaxLength)
            return 0;

        if (sNumVal1.length())
        {
            size_t nReturn = argumentParser(sNumVal1, nArg1);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal2.length())
        {
            size_t nReturn = argumentParser(sNumVal2, nArg2);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sString2.length())
        {
            size_t nReturn = argumentParser(sString2, sArg2, bLogicalOnly);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for two string
    /// and two numerical values. Every argument
    /// except of the first one is optional.
    ///
    /// \param __sFuncArgument const string&
    /// \param sArg1 s_vect&
    /// \param sArg2 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, s_vect& sArg1, s_vect& sArg2, n_vect& nArg1, n_vect& nArg2)
    {
        string sFuncArgument = __sFuncArgument;
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        string sString1 = getNextArgument(sFuncArgument, true);
        string sString2 = getNextArgument(sFuncArgument, true);
        string sNumVal1 = getNextArgument(sFuncArgument, true);
        string sNumVal2 = getNextArgument(sFuncArgument, true);

        // Handle the arguments using the basic functions
        // and store the highets number of return values
        nMaxLength = argumentParser(sString1, sArg1, bLogicalOnly);

        if (!nMaxLength)
            return 0;

        if (sString2.length())
        {
            size_t nReturn = argumentParser(sString2, sArg2, bLogicalOnly);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal1.length())
        {
            size_t nReturn = argumentParser(sNumVal1, nArg1);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal2.length())
        {
            size_t nReturn = argumentParser(sNumVal2, nArg2);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for three string
    /// and two numerical values. Every argument
    /// except of the first one is optional.
    ///
    /// \param __sFuncArgument const string&
    /// \param sArg1 s_vect&
    /// \param sArg2 s_vect&
    /// \param sArg3 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(const string& __sFuncArgument, s_vect& sArg1, s_vect& sArg2, s_vect& sArg3, n_vect& nArg1, n_vect& nArg2)
    {
        string sFuncArgument = __sFuncArgument;
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        string sString1 = getNextArgument(sFuncArgument, true);
        string sString2 = getNextArgument(sFuncArgument, true);
        string sString3 = getNextArgument(sFuncArgument, true);
        string sNumVal1 = getNextArgument(sFuncArgument, true);
        string sNumVal2 = getNextArgument(sFuncArgument, true);

        // Handle the arguments using the basic functions
        // and store the highets number of return values
        nMaxLength = argumentParser(sString1, sArg1, bLogicalOnly);

        if (!nMaxLength)
            return 0;

        if (sString2.length())
        {
            size_t nReturn = argumentParser(sString2, sArg2, bLogicalOnly);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sString3.length())
        {
            size_t nReturn = argumentParser(sString3, sArg3, bLogicalOnly);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal1.length())
        {
            size_t nReturn = argumentParser(sNumVal1, nArg1);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }
        else
            return nMaxLength;

        if (sNumVal2.length())
        {
            size_t nReturn = argumentParser(sNumVal2, nArg2);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies special
    /// string functions in the expression, which
    /// cannot be implemented as standard string
    /// functions.
    ///
    /// \param sLine string
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringFuncHandler::applySpecialStringFuncs(string sLine)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        Datafile& _data = NumeReKernel::getInstance()->getData();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        size_t nStartPosition = 0;
        size_t nEndPosition;

        // str string_cast(EXPR)
        while ((nStartPosition = findNextFunction("string_cast(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sToString = getFunctionArgumentList("string_cast(", sLine, nStartPosition, nEndPosition);

            if (sToString.find('"') != string::npos || sToString.find('#') != string::npos)
            {
                StringResult strRes = eval(sToString, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                sToString = createStringVectorVar(strRes.vResult);
            }
            else
                sToString = "\"" + sToString + "\"";

            sLine = sLine.substr(0, nStartPosition) + sToString + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // cmd to_cmd(str)
        while ((nStartPosition = findNextFunction("to_cmd(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sCmdString = getFunctionArgumentList("to_cmd(", sLine, nStartPosition, nEndPosition);
            StripSpaces(sCmdString);

            if (isStringExpression(sCmdString))
            {
                StringResult strRes = eval(sCmdString, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only the first one
                sCmdString = strRes.vResult[0];
            }

            sLine = sLine.substr(0, nStartPosition) + removeQuotationMarks(sCmdString) + sLine.substr(nEndPosition + 1);
            nStartPosition++;
        }

        nStartPosition = 0;
        // val to_value(str)
        while ((nStartPosition = findNextFunction("to_value(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sToValue = getFunctionArgumentList("to_value(", sLine, nStartPosition, nEndPosition);
            StripSpaces(sToValue);

            if (isStringExpression(sToValue))
            {
                StringResult strRes = eval(sToValue, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                vector<string> vToValueResults;

                // Remove all quotation marks from the results TODO: Split the equations
                for (size_t i = 0; i < strRes.vResult.size(); i++)
                {
                    strRes.vResult[i] = removeQuotationMarks(strRes.vResult[i]);

                    while (strRes.vResult[i].length())
                    {
                        vToValueResults.push_back(getNextArgument(strRes.vResult[i], true));
                    }
                }

                // Create a new string vector variable
                sToValue = createStringVectorVar(vToValueResults);
            }

            sLine = sLine.substr(0, nStartPosition) + sToValue + sLine.substr(nEndPosition + 1);
            nStartPosition++;
        }

        nStartPosition = 0;
        // log is_string(EXPR)
        while ((nStartPosition = findNextFunction("is_string(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sArgument = getFunctionArgumentList("is_string(", sLine, nStartPosition, nEndPosition);

            if (isStringExpression(sArgument))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // {val} = getindices(str, [val])
        while ((nStartPosition = findNextFunction("getindices(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string _sObject = getFunctionArgumentList("getindices(", sLine, nStartPosition, nEndPosition);
            StringResult strRes = eval(_sObject, "");

            if (!strRes.vResult.size())
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // use only first one
            string sType = strRes.vResult[0];
            int nType = 0;
            _sObject = getNextArgument(sType, true);

            if (!sType.length())
            {
                sType = "0";
            }

            if (_sObject[0] == '"')
                _sObject.erase(0, 1);

            if (_sObject[_sObject.length() - 1] == '"')
                _sObject.erase(_sObject.length() - 1);

            StripSpaces(_sObject);

            if (isStringExpression(sType))
            {
                StringResult strRes = eval(sType, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                sType = strRes.vResult[0];
            }

            _parser.SetExpr(sType);
            nType = (int)_parser.Eval();

            if (nType < -1 || nType > 2)
                nType = 0;

            DataAccessParser _accessParser(_sObject);

            if (!_accessParser.getDataObject().length() || !isValidIndexSet(_accessParser.getIndices()))
            {
                sLine = sLine.substr(0, nStartPosition) + "nan" + sLine.substr(nEndPosition + 1);
                nStartPosition++;
                continue;
            }

            if (nType > -1)
            {
                if (nType == 2 && _accessParser.getIndices().row.isOpenEnd())
                {
                    _accessParser.getIndices().row.setRange(_accessParser.getIndices().row.front(), _accessParser.getIndices().row.front() + 1);
                }
                else if (nType == 1 && _accessParser.getIndices().col.isOpenEnd())
                {
                    _accessParser.getIndices().col.setRange(_accessParser.getIndices().col.front(), _accessParser.getIndices().col.front() + 1);
                }
                else if (!nType)
                {
                    if (_accessParser.isCluster())
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                            _accessParser.getIndices().row.setRange(0, _data.getCluster(_accessParser.getDataObject()).size()-1);

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.back() = VectorIndex::INVALID;
                    }
                    else if (_accessParser.getDataObject() == "string")
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                        {
                            if (_accessParser.getIndices().col.size() == 1)
                            {
                                if (_data.getStringElements(_accessParser.getIndices().col.front()))
                                    _accessParser.getIndices().row.setRange(0, _data.getStringElements(_accessParser.getIndices().col.front())-1);
                                else
                                    _accessParser.getIndices().row.setRange(0, 0);
                            }
                            else
                            {
                                if (_data.getStringElements())
                                    _accessParser.getIndices().row.setRange(0, _data.getStringElements()-1);
                                else
                                    _accessParser.getIndices().row.setRange(0, 0);
                            }
                        }

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.setRange(0, _data.getStringCols()-1);
                    }
                    else
                    {
                        if (_accessParser.getIndices().row.isOpenEnd())
                            _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                        if (_accessParser.getIndices().col.isOpenEnd())
                            _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);
                    }
                }
            }

            _accessParser.getIndices().row.linearize();
            _accessParser.getIndices().col.linearize();

            vector<double> vIndices;
            vIndices.push_back(_accessParser.getIndices().row.front() + 1);
            vIndices.push_back(_accessParser.getIndices().row.last() + 1);
            vIndices.push_back(_accessParser.getIndices().col.front() + 1);
            vIndices.push_back(_accessParser.getIndices().col.last() + 1);
            _parser.SetVectorVar("_~indices[" + replaceToVectorname(_sObject) + "]", vIndices);
            sLine = sLine.substr(0, nStartPosition) + "_~indices[" + replaceToVectorname(_sObject) + "]" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // log = is_data(EXPR)
        while ((nStartPosition = findNextFunction("is_data(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("is_data(", sLine, nStartPosition, nEndPosition);

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0];
            }

            if (sData[0] == '"')
                sData.erase(0, 1);

            if (sData[sData.length() - 1] == '"')
                sData.erase(sData.length() - 1);

            StripSpaces(sData);

            if (sData.substr(0, 5) == "data(" || _data.isTable(sData) || _data.isCluster(sData))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // log = is_table(EXPR)
        while ((nStartPosition = findNextFunction("is_table(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("is_table(", sLine, nStartPosition, nEndPosition);

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0];
            }

            if (sData[0] == '"')
                sData.erase(0, 1);

            if (sData[sData.length() - 1] == '"')
                sData.erase(sData.length() - 1);

            StripSpaces(sData);

            if (sData.substr(0, 5) == "data(" || _data.isTable(sData))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // log = is_cluster(EXPR)
        while ((nStartPosition = findNextFunction("is_cluster(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("is_cluster(", sLine, nStartPosition, nEndPosition);

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0];
            }

            if (sData[0] == '"')
                sData.erase(0, 1);

            if (sData[sData.length() - 1] == '"')
                sData.erase(sData.length() - 1);

            StripSpaces(sData);

            if (_data.isCluster(sData))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // {var} = findcolumn("data","header")
        while ((nStartPosition = findNextFunction("findcolumn(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("findcolumn(", sLine, nStartPosition, nEndPosition);
            string sHeadline;

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0];
                sHeadline = strRes.vResult[1];
            }

            if (!sHeadline.length())
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            if (sData[0] == '"')
                sData.erase(0, 1);

            if (sData[sData.length() - 1] == '"')
                sData.erase(sData.length() - 1);

            StripSpaces(sData);

            if (sHeadline.front() == '"')
                sHeadline.erase(0, 1);

            if (sHeadline.back() == '"')
                sHeadline.erase(sHeadline.length() - 1);

            StripSpaces(sHeadline);

            if (sData.substr(0, 5) == "data(" || _data.isTable(sData))
            {
                sData.erase(sData.find("("));
                string sResult;

                for (long long int i = 0; i < _data.getCols(sData, false); i++)
                {
                    if (_data.getHeadLineElement(i, sData) == sHeadline)
                    {
                        if (sResult.length())
                            sResult += ", ";

                        sResult += toString(i + 1);
                    }
                }

                if (!sResult.length())
                    sResult = "nan";

                if (sResult.find(',') != string::npos)
                    sResult = "{" + sResult + "}";

                sLine = sLine.substr(0, nStartPosition) + sResult + sLine.substr(nEndPosition + 1);
            }
            else
                sLine = sLine.substr(0, nStartPosition) + "nan" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // str = valtostr(EXPR, [str])
        while ((nStartPosition = findNextFunction("valtostr(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sToString = getFunctionArgumentList("valtostr(", sLine, nStartPosition, nEndPosition);
            string sExpr = getNextArgument(sToString, true);
            string sChar = "";
            unsigned int nCount = 0;

            if (sToString.length())
            {
                sChar = getNextArgument(sToString, true);

                if (isStringExpression(sChar))
                {
                    StringResult strRes = eval(sChar, "");

                    if (!strRes.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                    // use only first one
                    sChar = removeQuotationMarks(strRes.vResult[0]);
                }

                string sCnt = getNextArgument(sToString, true);

                if (sCnt.length())
                {
                    _parser.SetExpr(sCnt);
                    nCount = (unsigned int)fabs(_parser.Eval());
                }
            }

            if (!isStringExpression(sExpr))
            {
                // check for data sets in the evaluation of the `valtostr()` arguments
                if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
                    getDataElements(sExpr, _parser, _data, _option);

                int nResults = 0;
                value_type* v = 0;
                _parser.SetExpr(sExpr);
                v = _parser.Eval(nResults);
                vector<string> vToString;
                string sElement = "";

                for (int n = 0; n < nResults; n++)
                {
                    if (fabs(rint(v[n]) - v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                        sElement = toString((long long int)rint(v[n]));
                    else
                        sElement = toString(v[n], _option);

                    while (sElement.length() < nCount && sChar.length())
                        sElement.insert(0, sChar);

                    vToString.push_back("\"" + sElement + "\"");
                }

                sToString = createStringVectorVar(vToString);
            }
            else
            {
                StringResult strRes = eval(sExpr, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                for (size_t i = 0; i < strRes.vResult.size(); i++)
                {
                    while (strRes.vResult[i].length() < nCount && sChar.length())
                        strRes.vResult[i].insert(0, sChar);

                    // add quotation marks, if they are missing
                    strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                }

                sToString = createStringVectorVar(strRes.vResult);
            }

            sLine = sLine.substr(0, nStartPosition) + sToString + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function searches for an
    /// occurence of a known string function in the
    /// passed command line and passes the control to
    /// StringFuncHandler::evalFunction() member
    /// function for evaluation.
    ///
    /// \param sLine string
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringFuncHandler::applyStringFuncs(string sLine)
    {
        for (auto iter = m_mStringFuncs.begin(); iter != m_mStringFuncs.end(); ++iter)
        {
            // Found an occurence -> call the string function handler
            if (sLine.find(iter->first + "(") != string::npos)
            {
                evalFunction(sLine, iter->first + "(", iter->second);

                if (sLine.find('(') == string::npos)
                    break;
            }
        }

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to fill
    /// the internal map of declared string functions
    /// with the data of the passed one.
    ///
    /// \param mStringFuncs const map<string,StringFuncHandle>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringFuncHandler::declareStringFuncs(const map<string,StringFuncHandle>& mStringFuncs)
    {
        if (!m_mStringFuncs.size())
            m_mStringFuncs = mStringFuncs;
    }


    /////////////////////////////////////////////////
    /// \brief Finds the position of the next function
    /// occurence in the passed string including the
    /// position of the closing parenthesis.
    ///
    /// \param sFunc const string&
    /// \param sLine const string&
    /// \param nStartPos size_t
    /// \param nEndPosition size_t&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::findNextFunction(const string& sFunc, const string& sLine, size_t nStartPos, size_t& nEndPosition)
    {
        // Search for occurences of the passed function
        while ((nStartPos = sLine.find(sFunc, nStartPos)) != string::npos)
        {
            // Ignore false positives
            if ((nStartPos && !isDelimiter(sLine[nStartPos-1])) || isInQuotes(sLine, nStartPos, true))
            {
                nStartPos++;
                continue;
            }

            // Find the matching parenthesis
            if ((nEndPosition = getMatchingParenthesis(sLine.substr(nStartPos + sFunc.length() - 1))) == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nStartPos + sFunc.length() - 1);

            // Update the end position and return the
            // starting position
            nEndPosition += nStartPos + sFunc.length() - 1;
            return nStartPos;
        }

        // Return string::npos, if nothing was found
        return string::npos;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the contents of the argument
    /// parentheses of the function starting at
    /// nStartPosition.
    ///
    /// \param sFunc const string&
    /// \param sLine const string&
    /// \param nStartPosition size_t First character of the function
    /// \param nEndPosition size_t Position of the closing parenthesis
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringFuncHandler::getFunctionArgumentList(const string& sFunc, const string& sLine, size_t nStartPosition, size_t nEndPosition)
    {
        return sLine.substr(nStartPosition + sFunc.length(), nEndPosition - nStartPosition - sFunc.length());
    }

}

