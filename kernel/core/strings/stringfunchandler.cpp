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
#include "../structures.hpp"
#define DEFAULT_NUM_ARG INT_MIN
// define the "End of transmission block" as string separator
#define NEWSTRING (char)23

using namespace std;

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
            //if (sRet[i] == '\t')
            //    sRet.replace(i, 1, "\\t");
            //if (sRet[i] == '\n')
            //    sRet.replace(i, 1, "\\n");
        }
        return sRet;
    }


    /////////////////////////////////////////////////
    /// \brief This member function evaluates the
    /// passed call sequence to the string function.
    ///
    /// \param sLine std::string&
    /// \param sFuncName const std::string&
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
    void StringFuncHandler::evalFunction(std::string& sLine, const std::string& sFuncName, StringFuncHandle funcHandle)
    {
        size_t nStartPosition = 0;
        size_t nEndPosition = 0;

        // While the function signature can be found
        while ((nStartPosition = findNextFunction(sFuncName, sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            // Extract the argument of the current found function and process it
            StringView sFunctionArgument = getFunctionArgumentList(sFuncName, sLine, nStartPosition, nEndPosition);
            std::vector<s_vect> vReturnValues;
            StringFuncArgs stringArgs;
            stringArgs.opt = &NumeReKernel::getInstance()->getSettings();
            bool bLogicalOnly = false;

            // Create function argument vector variables
            s_vect sStringArg1, sStringArg2, sStringArg3;
            n_vect nIntArg1, nIntArg2;
            d_vect dValArg;
            size_t nMaxArgs = 0;

            // Apply the parser as specified by the function signature. After that call the corresponding
            // string function with the returned arguments as many times as it's needed
            if (funcHandle.fType >= PARSER_INT && funcHandle.fType < PARSER_DOUBLE)
                nMaxArgs = argumentParser(sFunctionArgument, nIntArg1);
            else if (funcHandle.fType >= PARSER_DOUBLE && funcHandle.fType < PARSER_STRING)
                nMaxArgs = argumentParser(sFunctionArgument, dValArg, nIntArg1);
            else if (funcHandle.fType >= PARSER_STRING && funcHandle.fType < PARSER_STRING_DOUBLE)
            {
                if (sFuncName == "to_string(" && !isStringExpression(sFunctionArgument.to_string()))
                {
                    sStringArg1.push_generic(sFunctionArgument.to_string());
                    nMaxArgs = 1;
                }
                else
                    nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, bLogicalOnly);

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
            }
            else if (funcHandle.fType >= PARSER_STRING_DOUBLE && funcHandle.fType < PARSER_STRING_INT_INT)
                nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, dValArg);
            else if (funcHandle.fType >= PARSER_STRING_INT_INT && funcHandle.fType < PARSER_STRING_INT_INT_STRING)
                nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, nIntArg1, nIntArg2);
            else if (funcHandle.fType >= PARSER_STRING_INT_INT_STRING && funcHandle.fType < PARSER_STRING_STRING_INT_INT)
                nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, nIntArg1, nIntArg2, sStringArg2);
            else if (funcHandle.fType >= PARSER_STRING_STRING_INT_INT && funcHandle.fType < PARSER_STRING_STRING_STRING_INT_INT)
                nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, sStringArg2, nIntArg1, nIntArg2);
            else if (funcHandle.fType >= PARSER_STRING_STRING_STRING_INT_INT)
                nMaxArgs = argumentParser(sFunctionArgument, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2);

            // Ensure that at least a single argument is available
            if (!nMaxArgs && funcHandle.fType != NOARGS)
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // Evaluate the function calls
            if (funcHandle.bTakesMultiArguments)
            {
                if (funcHandle.fType >= PARSER_INT && funcHandle.fType < PARSER_STRING)
                {
                    stringArgs.nMultiArg = nIntArg1;
                    vReturnValues.push_back(funcHandle.fHandle(stringArgs));
                }
                else
                {
                    nMaxArgs = max(max(max(max(sStringArg2.size(), sStringArg3.size()), nIntArg1.size()), nIntArg2.size()), 1u);

                    if (nMaxArgs < 500)
                        vReturnValues = callMultiFunction(funcHandle, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2, dValArg, nMaxArgs);
                    else
                        vReturnValues = callMultiFunctionParallel(funcHandle, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2, dValArg, nMaxArgs);
                }
            }
            else
            {
                if (nMaxArgs < 500)
                    vReturnValues = callFunction(funcHandle, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2, dValArg, nMaxArgs);
                else
                    vReturnValues = callFunctionParallel(funcHandle, sStringArg1, sStringArg2, sStringArg3, nIntArg1, nIntArg2, dValArg, nMaxArgs);
            }

            // Create a string vector variable for the function output
            std::string sFuncReturnValue = createStringVectorVar(expandStringVectorComponents(vReturnValues));

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
    /// \param __sFuncArgument StringView
    /// \param nArg n_vect& a vector of numerical
    /// values as return value
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, n_vect& nArg)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        std::string sFuncArgument = __sFuncArgument.to_string();
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
    /// function argument parser for numerical
    /// arguments.
    ///
    /// \param __sFuncArgument StringView
    /// \param dArg d_vect& a vector of numerical
    /// values as return value
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, d_vect& dArg)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        std::string sFuncArgument = __sFuncArgument.to_string();
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
                dArg = strRes.vNumericalValues;

                return dArg.size();
            }

            // Evaluate the returned strings numerically
            for (size_t i = 0; i < strRes.vResult.size(); i++)
            {
                _parser.SetExpr(strRes.vResult[i]);
                v = _parser.Eval(nReturn);
                dArg.insert(dArg.end(), v, v+nReturn);
            }

            return dArg.size();
        }
        else if (_data.containsTablesOrClusters(sFuncArgument))
        {
            getDataElements(sFuncArgument, _parser, _data, _option, false);
        }

        // Set the expression and evaluate it
        _parser.SetExpr(sFuncArgument);
        v = _parser.Eval(nReturn);
        dArg.insert(dArg.end(), v, v+nReturn);

        return (size_t)nReturn;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for double and
    /// numeric values.
    ///
    /// \param __sFuncArgument StringView
    /// \param dArg d_vect& a vector of numerical
    /// values as return value
    /// \param nArg n_vect& a vector of numerical
    /// values as return value
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, d_vect& dArg, n_vect& nArg)
    {
        size_t nMaxLength = 0;

        // Get the single arguments
        StringView sDouble = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal = getNextViewedArgument(__sFuncArgument);

        // Handle the arguments using the basic functions
        // and store the highest number of return values
        nMaxLength = argumentParser(sDouble, dArg);

        if (!nMaxLength)
            return 0;

        if (sNumVal.length())
        {
            size_t nReturn = argumentParser(sNumVal, nArg);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for string
    /// arguments.
    ///
    /// \param __sFuncArgument StringView
    /// \param sArg s_vect& a vector of string values
    /// as return value
    /// \param bLogicalOnly bool&
    /// \return size_t
    ///
    /// It is one of the two basic argument parser
    /// functions, which are called by all others
    /// depending on the signatures of their functions.
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg, bool& bLogicalOnly)
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        std::string sFuncArgument = __sFuncArgument.to_string();

        // If the current function argument contains strings,
        // parse it correspondingly
        if (isStringExpression(sFuncArgument) || _data.containsClusters(sFuncArgument))
        {
            // Call the string parser core
            StringResult strRes = eval(sFuncArgument, "", true);

            // Use the returned values as function arguments
            sArg = strRes.vResult;

            bLogicalOnly = strRes.bOnlyLogicals;
            return strRes.vResult.size();
        }
        else if (_data.containsTablesOrClusters(sFuncArgument))
        {
            getDataElements(sFuncArgument, _parser, _data, _option, INSERT_STRINGS);

            if (isStringExpression(sFuncArgument))
            {
                // Call the string parser core
                StringResult strRes = eval(sFuncArgument, "", true);

                // Use the returned values as function arguments
                sArg = strRes.vResult;

                bLogicalOnly = strRes.bOnlyLogicals;
                return strRes.vResult.size();
            }
        }

        // Expand the passed argument, if needed and
        // distribute it to the components of the argument vector
        if (sFuncArgument.find('{') != string::npos || sFuncArgument.find(',') != string::npos)
        {
            convertVectorToExpression(sFuncArgument, _option);

            // As long as the function argument has a length,
            // get the next argument and store it in the vector
            while (sFuncArgument.length())
                sArg.push_generic(removeQuotationMarks(getNextArgument(sFuncArgument, true)));
        }
        else
            sArg.push_generic(removeQuotationMarks(sFuncArgument));

        // Declare argument as numerical only
        bLogicalOnly = true;

        return sArg.size();
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for one string and
    /// one (optional) numerical argument.
    ///
    /// \param __sFuncArgument StringView
    /// \param sArg1 s_vect&
    /// \param dArg1 d_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg1, d_vect& dArg1)
    {
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        StringView sString = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal = getNextViewedArgument(__sFuncArgument);

        // Handle the arguments using the basic functions
        // and store the highets number of return values
        nMaxLength = argumentParser(sString, sArg1, bLogicalOnly);

        if (!nMaxLength)
            return 0;

        if (sNumVal.length())
        {
            size_t nReturn = argumentParser(sNumVal, dArg1);

            if (!nReturn)
                return 0;

            if (nMaxLength < nReturn)
                nMaxLength = nReturn;
        }

        return nMaxLength;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the string
    /// function argument parser for one string and
    /// two (optional) numerical arguments.
    ///
    /// \param __sFuncArgument StringView
    /// \param sArg1 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2)
    {
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        StringView sString = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal1 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal2 = getNextViewedArgument(__sFuncArgument);

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
    /// \param __sFuncArgument StringView
    /// \param sArg1 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \param sArg2 s_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg1, n_vect& nArg1, n_vect& nArg2, s_vect& sArg2)
    {
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        StringView sString1 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal1 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal2 = getNextViewedArgument(__sFuncArgument);
        StringView sString2 = getNextViewedArgument(__sFuncArgument);

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
    /// \param __sFuncArgument StringView
    /// \param sArg1 s_vect&
    /// \param sArg2 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg1, s_vect& sArg2, n_vect& nArg1, n_vect& nArg2)
    {
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        StringView sString1 = getNextViewedArgument(__sFuncArgument);
        StringView sString2 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal1 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal2 = getNextViewedArgument(__sFuncArgument);

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
    /// \param __sFuncArgument StringView
    /// \param sArg1 s_vect&
    /// \param sArg2 s_vect&
    /// \param sArg3 s_vect&
    /// \param nArg1 n_vect&
    /// \param nArg2 n_vect&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::argumentParser(StringView __sFuncArgument, s_vect& sArg1, s_vect& sArg2, s_vect& sArg3, n_vect& nArg1, n_vect& nArg2)
    {
        size_t nMaxLength = 0;
        bool bLogicalOnly = false;

        // Get the single arguments
        StringView sString1 = getNextViewedArgument(__sFuncArgument);
        StringView sString2 = getNextViewedArgument(__sFuncArgument);
        StringView sString3 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal1 = getNextViewedArgument(__sFuncArgument);
        StringView sNumVal2 = getNextViewedArgument(__sFuncArgument);

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
    /// \brief Calls the selected string function
    /// using the passed string function arguments
    /// sequentially.
    ///
    /// \param funcHandle StringFuncHandle
    /// \param sStringArg1 s_vect&
    /// \param sStringArg2 s_vect&
    /// \param sStringArg3 s_vect&
    /// \param nIntArg1 n_vect&
    /// \param nIntArg2 n_vect&
    /// \param dValArg d_vect&
    /// \param nMaxArgs size_t
    /// \return std::vector<s_vect>
    ///
    /////////////////////////////////////////////////
    std::vector<s_vect> StringFuncHandler::callFunction(StringFuncHandle funcHandle, s_vect& sStringArg1, s_vect& sStringArg2, s_vect& sStringArg3, n_vect& nIntArg1, n_vect& nIntArg2, d_vect& dValArg, size_t nMaxArgs)
    {
        StringFuncArgs stringArgs;
        stringArgs.opt = &NumeReKernel::getInstance()->getSettings();

        // Shortcut for empty function arguments
        if (!nMaxArgs)
            return std::vector<s_vect>(1, funcHandle.fHandle(stringArgs));

        std::vector<s_vect> vReturnValues(nMaxArgs);

        for (size_t i = 0; i < nMaxArgs; i++)
        {
            stringArgs.sArg1 = sStringArg1.getArg(i);
            stringArgs.sArg2 = sStringArg2.getArg(i);
            stringArgs.sArg3 = sStringArg3.getArg(i);

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

            if (i < dValArg.size())
                stringArgs.dArg1 = dValArg[i];
            else if (dValArg.size() == 1)
                stringArgs.dArg1 = dValArg[0];
            else
                stringArgs.dArg1 = 0.0;

            vReturnValues[i] = funcHandle.fHandle(stringArgs);
        }

        return vReturnValues;
    }


    /////////////////////////////////////////////////
    /// \brief Calls the selected string function
    /// using the passed string function arguments
    /// parallel using OpenMP.
    ///
    /// \param funcHandle StringFuncHandle
    /// \param sStringArg1 s_vect&
    /// \param sStringArg2 s_vect&
    /// \param sStringArg3 s_vect&
    /// \param nIntArg1 n_vect&
    /// \param nIntArg2 n_vect&
    /// \param dValArg d_vect&
    /// \param nMaxArgs size_t
    /// \return std::vector<s_vect>
    ///
    /////////////////////////////////////////////////
    std::vector<s_vect> StringFuncHandler::callFunctionParallel(StringFuncHandle funcHandle, s_vect& sStringArg1, s_vect& sStringArg2, s_vect& sStringArg3, n_vect& nIntArg1, n_vect& nIntArg2, d_vect& dValArg, size_t nMaxArgs)
    {
        std::vector<s_vect> vReturnValues(nMaxArgs);

        StringFuncArgs stringArgs;
        stringArgs.opt = &NumeReKernel::getInstance()->getSettings();

        #pragma omp parallel for firstprivate(stringArgs)
        for (size_t i = 0; i < nMaxArgs; i++)
        {
            stringArgs.sArg1 = sStringArg1.getArg(i);
            stringArgs.sArg2 = sStringArg2.getArg(i);
            stringArgs.sArg3 = sStringArg3.getArg(i);

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

            if (i < dValArg.size())
                stringArgs.dArg1 = dValArg[i];
            else if (dValArg.size() == 1)
                stringArgs.dArg1 = dValArg[0];
            else
                stringArgs.dArg1 = 0.0;

            vReturnValues[i] = funcHandle.fHandle(stringArgs);
        }

        return vReturnValues;
    }


    /////////////////////////////////////////////////
    /// \brief Calls the selected string function
    /// accepting multiple values as first argument
    /// using the passed string function arguments
    /// sequentially.
    ///
    /// \param funcHandle StringFuncHandle
    /// \param sStringArg1 s_vect&
    /// \param sStringArg2 s_vect&
    /// \param sStringArg3 s_vect&
    /// \param nIntArg1 n_vect&
    /// \param nIntArg2 n_vect&
    /// \param dValArg d_vect&
    /// \param nMaxArgs size_t
    /// \return std::vector<s_vect>
    ///
    /////////////////////////////////////////////////
    std::vector<s_vect> StringFuncHandler::callMultiFunction(StringFuncHandle funcHandle, s_vect& sStringArg1, s_vect& sStringArg2, s_vect& sStringArg3, n_vect& nIntArg1, n_vect& nIntArg2, d_vect& dValArg, size_t nMaxArgs)
    {
        std::vector<s_vect> vReturnValues(nMaxArgs);

        StringFuncArgs stringArgs;
        stringArgs.opt = &NumeReKernel::getInstance()->getSettings();
        stringArgs.sMultiArg = sStringArg1;

        for (size_t i = 0; i < nMaxArgs; i++)
        {
            stringArgs.sArg2 = sStringArg2.getArg(i);
            stringArgs.sArg3 = sStringArg3.getArg(i);

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

            if (i < dValArg.size())
                stringArgs.dArg1 = dValArg[i];
            else if (dValArg.size() == 1)
                stringArgs.dArg1 = dValArg[0];
            else
                stringArgs.dArg1 = 0.0;

            vReturnValues[i] = funcHandle.fHandle(stringArgs);
        }

        return vReturnValues;
    }


    /////////////////////////////////////////////////
    /// \brief Calls the selected string function
    /// accepting multiple values as first argument
    /// using the passed string function arguments
    /// parallel using OpenMP.
    ///
    /// \param funcHandle StringFuncHandle
    /// \param sStringArg1 s_vect&
    /// \param sStringArg2 s_vect&
    /// \param sStringArg3 s_vect&
    /// \param nIntArg1 n_vect&
    /// \param nIntArg2 n_vect&
    /// \param dValArg d_vect&
    /// \param nMaxArgs size_t
    /// \return std::vector<s_vect>
    ///
    /////////////////////////////////////////////////
    std::vector<s_vect> StringFuncHandler::callMultiFunctionParallel(StringFuncHandle funcHandle, s_vect& sStringArg1, s_vect& sStringArg2, s_vect& sStringArg3, n_vect& nIntArg1, n_vect& nIntArg2, d_vect& dValArg, size_t nMaxArgs)
    {
        std::vector<s_vect> vReturnValues(nMaxArgs);

        StringFuncArgs stringArgs;
        stringArgs.opt = &NumeReKernel::getInstance()->getSettings();
        stringArgs.sMultiArg = sStringArg1;

        #pragma omp parallel for firstprivate(stringArgs)
        for (size_t i = 0; i < nMaxArgs; i++)
        {
            stringArgs.sArg2 = sStringArg2.getArg(i);
            stringArgs.sArg3 = sStringArg3.getArg(i);

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

            if (i < dValArg.size())
                stringArgs.dArg1 = dValArg[i];
            else if (dValArg.size() == 1)
                stringArgs.dArg1 = dValArg[0];
            else
                stringArgs.dArg1 = 0.0;

            vReturnValues[i] = funcHandle.fHandle(stringArgs);
        }

        return vReturnValues;
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
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        Settings& _option = NumeReKernel::getInstance()->getSettings();
        size_t nStartPosition = 0;
        size_t nEndPosition;

        // str string_cast(EXPR)
        while ((nStartPosition = findNextFunction("string_cast(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sToString = getFunctionArgumentList("string_cast(", sLine, nStartPosition, nEndPosition).to_string();

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
            string sCmdString = getFunctionArgumentList("to_cmd(", sLine, nStartPosition, nEndPosition).to_string();
            StripSpaces(sCmdString);

            if (isStringExpression(sCmdString))
            {
                StringResult strRes = eval(sCmdString, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only the first one
                sCmdString = strRes.vResult.front();
            }

            // Because the result might be a constructed table, we
            // disable the access caching for this expression
            _parser.DisableAccessCaching();

            sLine = sLine.substr(0, nStartPosition) + removeQuotationMarks(sCmdString) + sLine.substr(nEndPosition + 1);
            nStartPosition++;
        }

        nStartPosition = 0;
        // val to_value(str)
        while ((nStartPosition = findNextFunction("to_value(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sToValue = getFunctionArgumentList("to_value(", sLine, nStartPosition, nEndPosition).to_string();
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
                    std::string sComponent = strRes.vResult[i].to_string();

                    while (sComponent.length())
                    {
                        vToValueResults.push_back(getNextArgument(sComponent, true));
                    }
                }

                // Create a new string vector variable
                sToValue = createStringVectorVar(vToValueResults);
            }

            // Because the result might be a constructed table, we
            // disable the access caching for this expression
            _parser.DisableAccessCaching();

            sLine = sLine.substr(0, nStartPosition) + sToValue + sLine.substr(nEndPosition + 1);
            nStartPosition++;
        }

        nStartPosition = 0;
        // log is_string(EXPR)
        while ((nStartPosition = findNextFunction("is_string(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sArgument = getFunctionArgumentList("is_string(", sLine, nStartPosition, nEndPosition).to_string();

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
            string _sObject = getFunctionArgumentList("getindices(", sLine, nStartPosition, nEndPosition).to_string();
            StringResult strRes = eval(_sObject, "");

            if (!strRes.vResult.size())
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // use only first one
            string sType = strRes.vResult[0].to_string();
            int nType = 0;
            _sObject = getNextArgument(sType, true);

            if (!sType.length())
            {
                sType = "0";
            }

            StripSpaces(_sObject);

            if (isStringExpression(sType))
            {
                StringResult res = eval(sType, "");

                if (!res.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                sType = res.vResult[0].to_string();
            }

            _parser.SetExpr(sType);
            nType = intCast(_parser.Eval());

            if (nType < -1 || nType > 2)
                nType = 0;

            // Because the object might be a constructed table, we
            // disable the access caching for this expression
            _parser.DisableAccessCaching();

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

            vector<mu::value_type> vIndices;
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
            string sData = getFunctionArgumentList("is_data(", sLine, nStartPosition, nEndPosition).to_string();

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0].to_string();
            }

            StripSpaces(sData);

            if (_data.isTable(sData) || _data.isCluster(sData))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // log = is_table(EXPR)
        while ((nStartPosition = findNextFunction("is_table(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("is_table(", sLine, nStartPosition, nEndPosition).to_string();

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0].to_string();
            }

            StripSpaces(sData);

            if (_data.isTable(sData))
                sLine = sLine.substr(0, nStartPosition) + "true" + sLine.substr(nEndPosition + 1);
            else
                sLine = sLine.substr(0, nStartPosition) + "false" + sLine.substr(nEndPosition + 1);

            nStartPosition++;
        }

        nStartPosition = 0;
        // log = is_cluster(EXPR)
        while ((nStartPosition = findNextFunction("is_cluster(", sLine, nStartPosition, nEndPosition)) != string::npos)
        {
            string sData = getFunctionArgumentList("is_cluster(", sLine, nStartPosition, nEndPosition).to_string();

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0].to_string();
            }

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
            string sData = getFunctionArgumentList("findcolumn(", sLine, nStartPosition, nEndPosition).to_string();
            string sHeadline;

            if (isStringExpression(sData))
            {
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // use only first one
                sData = strRes.vResult[0].to_string();
                sHeadline = strRes.vResult[1].to_string();
            }

            if (!sHeadline.length())
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            StripSpaces(sData);
            StripSpaces(sHeadline);

            if (_data.isTable(sData))
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
            string sToString = getFunctionArgumentList("valtostr(", sLine, nStartPosition, nEndPosition).to_string();
            string sExpr = getNextArgument(sToString, true);
            string sChar = "";
            std::vector<mu::value_type> vCounts;

            if (sToString.length())
            {
                sChar = getNextArgument(sToString, true);

                if (isStringExpression(sChar))
                {
                    StringResult strRes = eval(sChar, "");

                    if (!strRes.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                    // use only first one
                    sChar = strRes.vResult[0].to_string();
                }

                string sCnt = getNextArgument(sToString, true);

                if (sCnt.length())
                {
                    if (_data.containsTablesOrClusters(sCnt))
                        getDataElements(sCnt, _parser, _data, _option);

                    _parser.SetExpr(sCnt);
                    int nCount;
                    value_type* v;
                    v = _parser.Eval(nCount);
                    vCounts.assign(v, v+nCount);
                }
            }

            // check for data sets in the evaluation of the `valtostr()` arguments
            if (!isStringExpression(sExpr) && _data.containsTablesOrClusters(sExpr))
                getDataElements(sExpr, _parser, _data, _option);

            if (!isStringExpression(sExpr))
            {
                int nResults = 0;
                value_type* v = 0;
                _parser.SetExpr(sExpr);
                v = _parser.Eval(nResults);
                vector<string> vToString;
                string sElement = "";
                size_t nLen = 0;

                for (int n = 0; n < nResults; n++)
                {
                    sElement = printValue(v[n]);

                    if (n < (int)vCounts.size())
                        nLen = intCast(fabs(vCounts[n]));

                    while (sElement.length() < nLen && sChar.length())
                        sElement.insert(0, sChar);

                    vToString.push_back("\"" + sElement + "\"");
                }

                sToString = createStringVectorVar(vToString);
            }
            else
            {
                StringResult strRes = eval(sExpr, "");
                size_t nLen = 0;

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                for (size_t i = 0; i < strRes.vResult.size(); i++)
                {
                    if (i < vCounts.size())
                        nLen = intCast(fabs(vCounts[i]));

                    while (strRes.vResult[i].length() < nLen && sChar.length())
                        strRes.vResult.getRef(i).insert(strRes.vResult.is_string(i) ? 1 : 0, sChar);

                    // add quotation marks, if they are missing
                    strRes.vResult.convert_to_string(i);
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
    /// \param sLine StringView
    /// \param nStartPos size_t
    /// \param nEndPosition size_t&
    /// \param searchForMethods bool
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StringFuncHandler::findNextFunction(const string& sFunc, StringView sLine, size_t nStartPos, size_t& nEndPosition, bool searchForMethods)
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
            if ((nEndPosition = getMatchingParenthesis(sLine.subview(nStartPos + sFunc.length() - 1))) == std::string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine.to_string(), nStartPos + sFunc.length() - 1);

            // Update the end position and return the
            // starting position
            nEndPosition += nStartPos + sFunc.length() - 1;

            // Catch method calls
            if (searchForMethods && nEndPosition+1 < sLine.length() && sLine[nEndPosition+1] == '.')
            {
                for (size_t i = nEndPosition+1; i < sLine.length(); i++)
                {
                    if (sLine[i] == '(' || sLine[i] == '{')
                        i += getMatchingParenthesis(sLine.subview(i));
                    else if (sLine[i] == ')' || sLine[i] == '}')
                    {
                        // This block will only get activated, if we find an unmatched
                        // closing paranthesis, which can happen with methods, that
                        // don't need parentheses
                        nEndPosition = i-1;
                        break;
                    }

                    if (isDelimiter(sLine[i]) || i+1 == sLine.length())
                    {
                        nEndPosition = i;
                        break;
                    }
                }
            }

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
    /// \param sFunc const std::string&
    /// \param sLine StringView
    /// \param nStartPosition size_t First character of the function
    /// \param nEndPosition size_t Position of the closing parenthesis
    /// \return string
    ///
    /////////////////////////////////////////////////
    StringView StringFuncHandler::getFunctionArgumentList(const std::string& sFunc, StringView sLine, size_t nStartPosition, size_t nEndPosition)
    {
        return sLine.subview(nStartPosition + sFunc.length(),
                             nEndPosition - nStartPosition - sFunc.length());
    }


    /////////////////////////////////////////////////
    /// \brief Prints a value to a string respecting
    /// possible integer optimizations.
    ///
    /// \param value const mu::value_type&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringFuncHandler::printValue(const mu::value_type& value)
    {
        // Is one of the components zero, then try to find an
        // integer optimisation
        if (value.imag() == 0.0)
        {
            if (fabs(rint(value.real()) - value.real()) < 1e-14 && fabs(value.real()) >= 1.0)
                return toString(intCast(value.real()));
        }
        else if (value.real() == 0.0)
        {
            if (fabs(rint(value.imag()) - value.imag()) < 1e-14 && fabs(value.imag()) >= 1.0)
                return toString(intCast(value.imag())) + "i";
        }

        // Otherwise do not optimize due to the fact that the
        // precision will get halved in this case
        return toString(value, NumeReKernel::getInstance()->getSettings().getPrecision());
    }

}

