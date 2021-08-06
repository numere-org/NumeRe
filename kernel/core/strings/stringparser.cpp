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

#include "stringparser.hpp"
#include "stringfunctions.hpp"
#include "stringexpression.hpp"
#include <algorithm>

#include "../utils/timer.hpp"

extern value_type vAns;

string removeQuotationMarks(const string& sString);
string addQuotationMarks(const string& sString);

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief String parser constructor.
    ///
    /// \param parser mu::Parser&
    /// \param data Datafile&
    /// \param option Settings&
    ///
    /////////////////////////////////////////////////
    StringParser::StringParser(mu::Parser& parser, MemoryManager& data, Settings& option) : StringLogicParser(), StringFuncHandler(), _parser(parser), _data(data), _option(option)
    {
        declareStringFuncs(getStringFuncHandles());

        m_mStringParams["noquotes"] = NO_QUOTES;
        m_mStringParams["nq"] = NO_QUOTES;
        m_mStringParams["peek"] = PEEK;
        m_mStringParams["print"] = NO_QUOTES | PEEK;
        m_mStringParams["komq"] = KEEP_MASKED_QUOTES;
        m_mStringParams["kmq"] = KEEP_MASKED_CONTROL_CHARS | KEEP_MASKED_QUOTES;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the
    /// contents of the data objects and expands them
    /// as a list.
    ///
    /// \param sLine string
    /// \param n_pos size_t
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::getDataForString(string sLine, size_t n_pos)
    {
        // Get the contents of "string()", "data()" and the other caches
        size_t nEndPosition;

        if (sLine.find_first_of("({") == string::npos)
            return sLine;

        // {str} = string(...)
        while ((n_pos = findNextFunction("string(", sLine, n_pos, nEndPosition)) != string::npos)
        {
            // Check for arguments to parse and
            // get the indices afterwards
            Indices _idx = getIndices("string(" + parseStringsInIndices(getFunctionArgumentList("string(", sLine, n_pos, nEndPosition)) + ")", _parser, _data, _option);

            // Pre-process the indices
            if (_idx.col.isOpenEnd())
                _idx.col.back() = VectorIndex::INVALID;

            if (_idx.row.isOpenEnd())
                _idx.row.setRange(0, _data.getStringElements(_idx.col.front())-1);

            if (!_idx.col.isValid())
                _idx.col.front() = 0;

            // Handle multi-argument functions or return
            // the stored strings directly
            if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nEndPosition + 1)))
            {
                string sLeft = sLine.substr(0, n_pos);
                StripSpaces(sLeft);

                if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "num(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + toString(_idx.row.size()) + sLine.substr(sLine.find(')', nEndPosition + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "max(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.maxString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nEndPosition + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "min(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.minString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nEndPosition + 1) + 1);
                else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "sum(")
                    sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.sumString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nEndPosition + 1) + 1);
            }
            else
            {
                // Create the string vector
                if (_data.getStringElements(_idx.col.front()))
                {
                    string sString = "";
                    vector<string> vStrings;

                    for (size_t i = 0; i < _idx.row.size(); i++)
                    {
                        vStrings.push_back("\"" +  _data.readString((unsigned int)_idx.row[i], _idx.col.front()) + "\"");
                    }

                    sString = createStringVectorVar(vStrings);
                    sLine = sLine.substr(0, n_pos) + sString + sLine.substr(nEndPosition + 1);
                }
                else
                    sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nEndPosition + 1);
            }

            n_pos++;
        }

        if (sLine.find_first_of("({") == string::npos)
            return sLine;

        //getDataElements(sLine, NumeReKernel::getInstance()->getParser(), NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());
        //return sLine;

        // Replace calls to any table
        for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
        {
            if (sLine.find('(') == string::npos)
                break;

            replaceDataOccurence(sLine, iter->first + "(");
        }

        if (sLine.find('{') == string::npos)
            return sLine;

        // Replace calls to any cluster
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
        {
            if (sLine.find('{') == string::npos)
                break;

            replaceDataOccurence(sLine, iter->first + "{");
        }

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief Parses the string expressions in index
    /// expressions so that they may used as numerical
    /// index expressions.
    ///
    /// \param sIndexExpression string
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::parseStringsInIndices(string sIndexExpression)
    {
        // Ensure that this is an expression,
        // which actually contains strings
        if (!isStringExpression(sIndexExpression))
            return sIndexExpression;

        string sParsedIndices;

        // As long as the passed index expression
        // still has a length
        while (sIndexExpression.length())
        {
            // Get the next index pair and evaluate
            // its strings
            string sIndexPairs = getNextArgument(sIndexExpression, true);

            // If the current set of indices is
            // only the value parser, then ignore
            // that it is a string functionality,
            // because the user tries to access
            // the headlines of a table
            if (sIndexPairs == "#")
            {
                if (sParsedIndices.length())
                    sParsedIndices += ",#";
                else
                    sParsedIndices = "#";

                continue;
            }

            StringResult strRes = eval(sIndexPairs, "", false);

            if (!strRes.vResult.size())
                throw SyntaxError(SyntaxError::STRING_ERROR, sIndexPairs, SyntaxError::invalid_position);

            // Check, if the return values are
            // only strings
            if (strRes.bOnlyLogicals && strRes.vResult.size() > 1)
            {
                vector<mu::value_type> vIndices;

                // Convert the strings to doubles
                for (size_t i = 0; i < strRes.vResult.size(); i++)
                    vIndices.push_back(StrToDb(strRes.vResult[i]));

                // Create a temporary vector
                if (sParsedIndices.length())
                    sParsedIndices += ", " + _parser.CreateTempVectorVar(vIndices);
                else
                    sParsedIndices = _parser.CreateTempVectorVar(vIndices);
            }
            else
            {
                if (sParsedIndices.length())
                    sParsedIndices += ", " + strRes.vResult[0];
                else
                    sParsedIndices = strRes.vResult[0];
            }
        }

        return sParsedIndices;
    }


    /////////////////////////////////////////////////
    /// \brief Replaces all occurences of the passed
    /// data access occurence in the passed string.
    ///
    /// \param sLine string&
    /// \param sOccurence const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::replaceDataOccurence(string& sLine, const string& sOccurence)
    {
        size_t nStartPosition = 0;
        size_t nEndPosition;

        // Find the next occurence
        while ((nStartPosition = findNextFunction(sOccurence, sLine, nStartPosition, nEndPosition, true)) != string::npos)
        {
            size_t nStartPos = nStartPosition;
            size_t nEndPos = nEndPosition;
            // Handle multi-argument functions
            if (parser_CheckMultArgFunc(sLine.substr(0, nStartPosition), sLine.substr(nEndPosition + 1)))
            {
                if (nStartPosition > 4 && sLine.substr(sLine.rfind('(', nStartPosition) - 4, 5) == "norm(")
                    nStartPos -= 5;
                else
                    nStartPos -= 4;

                nEndPos++;
            }

            string sData = sLine.substr(nStartPos, nEndPos - nStartPos + 1);

            // Is the current data access a method?
            if (sData.find("().") != std::string::npos)
            {
                if (containsStringVectorVars(sData))
                {
                    bool temp;
                    std::vector<string> vRes = evaluateStringVectors(sData);
                    applyElementaryStringOperations(vRes, temp);
                    sData = vRes.front();
                }

                getDataElements(sData, _parser, _data, _option, true);
            }
            else
            {
                sData.replace(nStartPosition-nStartPos+sOccurence.length(), nEndPosition-nStartPosition-sOccurence.length(), parseStringsInIndices(getFunctionArgumentList(sOccurence, sLine, nStartPosition, nEndPosition)));

                // Get the data and parse string expressions
                replaceDataEntities(sData, sOccurence, _data, _parser, _option, true);
            }

            // Strip the spaces, which have been added during the
            // calls to the data entities
            StripSpaces(sData);

            // NOTE: Explicit parsing of the result is not necessary any more,
            // because replaceDataEntities will either return vectors or
            // plain strings or numerical values!
            //
            //StringResult strRes = eval(sData, "");
            //if (!strRes.vResult.size())
            //    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
            //
            //// Create a string vector variable from the returned vector
            //// if it is a string. Otherwise simple use the first return
            //// value, which already contains a numerical vector
            //if (!strRes.bOnlyLogicals)
            //    sData = createStringVectorVar(strRes.vResult);
            //else
            //    sData = strRes.vResult.front();

            sLine = sLine.substr(0, nStartPos) + sData + sLine.substr(nEndPos + 1);

            nStartPosition = nStartPos+1;
        }

    }


    /////////////////////////////////////////////////
    /// \brief This member function implements the
    /// so-called value-to-string parser (called via
    /// \c #VAR in code).
    ///
    /// \param sLine const string&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::numToString(const string& sLine)
    {
        // Do nothing, if there's no #
        if (sLine.find('#') == string::npos)
            return sLine + " ";

        // Create a copy of the current line
        string sLineToParsed = sLine + " ";
        string sLineToParsedTemp;

        unsigned int nPos = 0;
        unsigned int n_pos = 0;

        // As long as there are further "#"
        while (sLineToParsed.find('#', nPos) != string::npos)
        {
            // Store its position
            nPos = sLineToParsed.find('#', nPos);

            // Ensure that it is not in a string
            if (!isInQuotes(sLineToParsed, nPos, true))
            {
                string sPrefix = "";
                sLineToParsedTemp += sLineToParsed.substr(0, nPos);
                sLineToParsed = sLineToParsed.substr(nPos + 1);

                // If the variable starts with a tilde, it's possible that we
                // need to prepend zeros. Get the prefix here
                if (sLineToParsed[0] == '~')
                {
                    for (unsigned int i = 0; i < sLineToParsed.length(); i++)
                    {
                        if (sLineToParsed[i] != '~')
                        {
                            sPrefix = sLineToParsed.substr(0, i);
                            sLineToParsed = sLineToParsed.substr(i);
                            break;
                        }
                    }
                }

                // Jump over simple signs
                if (sLineToParsed[0] == '-' || sLineToParsed[0] == '+')
                    n_pos = 1;
                else
                    n_pos = 0;

                // Handle enclosing characters: parentheses, quotation marks
                // angle brackets, ...
                if (sLineToParsed[0] == '(' || sLineToParsed[0] == '{')
                {
                    // Get the contents of the current parenthesis
                    string sExpr = sLineToParsed.substr(1, getMatchingParenthesis(sLineToParsed) - 1);

                    // Does it contain strings?
                    if (isStringExpression(sExpr))
                    {
                        StringResult strRes = eval(sExpr, "");
                        if (!strRes.vResult.size())
                            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                        // Examine, whether the return value is logical only
                        if (!strRes.bOnlyLogicals)
                        {
                            // This case contains strings. We will cast the whole
                            // result into a string
                            for (size_t i = 0; i < strRes.vResult.size(); i++)
                            {
                                strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                            }

                            // Create a vector variable from the return value
                            sExpr = createStringVectorVar(strRes.vResult);
                            sLineToParsedTemp += sExpr;

                            // Get the next part of the command line
                            if (getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                                sLineToParsed = sLineToParsed.substr(getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)));
                            else
                                sLineToParsed.clear();

                            nPos = 0;
                            continue;
                        }

                        // This code section is reached, if the return value
                        // is logical only. We will construct a multi-value
                        // expression and evaluate it numerically
                        sExpr.clear();

                        // Construct the expression and remove the trailing comma
                        for (size_t i = 0; i < strRes.vResult.size(); i++)
                        {
                            sExpr += strRes.vResult[i] + ",";
                        }
                        sExpr.pop_back();

                    }

                    // Set the expression
                    _parser.SetExpr(sExpr);
                }
                else if (sLineToParsed[0] == '"')
                {
                    // Get the contents of the current string
                    string sExpr = sLineToParsed.substr(1, sLineToParsed.find('"', 1) - 1);

                    // Add the zeros, if needed
                    while (sExpr.length() < sPrefix.length() + 2)
                        sExpr.insert(0, 1, '0');

                    // Store the result and continue
                    sLineToParsedTemp += "\"" + sExpr + "\"";
                    if (sLineToParsed.find('"', 1) < sLineToParsed.length() - 1)
                        sLineToParsed = sLineToParsed.substr(sLineToParsed.find('"', 1) + 1);
                    else
                        sLineToParsed.clear();
                    nPos = 0;
                    continue;
                }
                else if (sLineToParsed[0] == '<')
                {
                    // Does the current object contain a path token?
                    if (sLineToParsed.find("<>") == 0
                            || sLineToParsed.find("<this>") == 0
                            || sLineToParsed.find("<wp>") == 0
                            || sLineToParsed.find("<loadpath>") == 0
                            || sLineToParsed.find("<savepath>") == 0
                            || sLineToParsed.find("<plotpath>") == 0
                            || sLineToParsed.find("<procpath>") == 0
                            || sLineToParsed.find("<scriptpath>") == 0)
                    {
                        // Replace the path tokens
                        if (sLineToParsed.find("<>") == 0 || sLineToParsed.find("<this>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getExePath()) + "\"";
                        else if (sLineToParsed.find("<wp>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getWorkPath()) + "\"";
                        else if (sLineToParsed.find("<loadpath>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getLoadPath()) + "\"";
                        else if (sLineToParsed.find("<savepath>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getSavePath()) + "\"";
                        else if (sLineToParsed.find("<plotpath>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getPlotPath()) + "\"";
                        else if (sLineToParsed.find("<procpath>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getProcPath()) + "\"";
                        else
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getScriptPath()) + "\"";
                        sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>') + 1);
                    }
                    else if (sLineToParsed.find('>') != string::npos)
                    {
                        // If no path token, only use the part in between of the angle brackets
                        sLineToParsedTemp += "\"" + sLineToParsed.substr(1, sLineToParsed.find('>') - 1) + "\"";
                        sLineToParsed = sLineToParsed.substr(sLineToParsed.find('>') + 1);
                    }
                    else
                    {
                        // Throw an error
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
                    }
                    nPos = 0;
                    continue;
                }
                else if (containsStringVectorVars(sLineToParsed.substr(0, getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)))))
                {
                    // Here are string vector variables
                    string sExpr = sLineToParsed.substr(0, getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)));

                    // Parse the current line
                    StringResult strRes = eval(sExpr, "");

                    // Ensure that the result exists
                    if (!strRes.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                    // Add the needed quotation marks
                    for (size_t i = 0; i < strRes.vResult.size(); i++)
                    {
                        strRes.vResult[i] = addQuotationMarks(strRes.vResult[i]);
                    }

                    // Create a new string vector var, append it to the string and continue
                    sExpr = createStringVectorVar(strRes.vResult);
                    sLineToParsedTemp += sExpr;
                    if (getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                        sLineToParsed = sLineToParsed.substr(getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)));
                    else
                        sLineToParsed.clear();

                    continue;
                }
                else // Set the expression
                    _parser.SetExpr(sLineToParsed.substr(0, getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos))));

                int nResults = 0;
                mu::value_type* v = 0;
                vector<string> vResults;
                string sElement = "";

                // Evaluate the parsed expression
                v = _parser.Eval(nResults);

                // Convert all results into strings
                for (int n = 0; n < nResults; n++)
                {
                    sElement = printValue(v[n]);

                    while (sElement.length() < sPrefix.length() + 1)
                        sElement.insert(0, 1, '0');

                    vResults.push_back(addQuotationMarks(sElement));
                }

                // Create a string vector var from the results and append it to
                // the current line
                sElement = createStringVectorVar(vResults);
                sLineToParsedTemp += sElement;

                // Search for the next delimiter
                if (getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                    sLineToParsed = sLineToParsed.substr(getPositionOfFirstDelimiter(sLineToParsed.substr(n_pos)));
                else
                    sLineToParsed.clear();

                nPos = 0;
            }
            else
                nPos++;
        }

        // Append the remaining parts of the current expression
        if (sLineToParsed.length() && sLineToParsedTemp.length())
            sLineToParsedTemp += sLineToParsed;

        // Handle remaining vector braces
        if (sLineToParsedTemp.find('{') != string::npos)
        {
            convertVectorToExpression(sLineToParsedTemp, _option);
        }

        // Determine the return value
        if (sLineToParsedTemp.length())
            sLineToParsed = sLineToParsedTemp;
        else
            sLineToParsed = sLine;

        return sLineToParsed;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringParser::storeStringResults(). It will
    /// store the strings into the data tables.
    ///
    /// \param strRes StringResult&
    /// \param sObject string&
    /// \param nCurrentComponent size_t&
    /// \param nStrings size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::storeStringToDataObjects(StringResult& strRes, string& sObject, size_t& nCurrentComponent, size_t nStrings)
    {
        // Identify the correct table
        DataAccessParser _accessParser(sObject);

        if (!_accessParser.getDataObject().length())
            return;

        string sTableName = _accessParser.getDataObject();
        Indices& _idx = _accessParser.getIndices();

        // Is the target a headline or is it a regular index?
        if (_idx.row.isString())
        {
            // If the first element is non-zero but the second is,
            // we use the number of elements as upper boundary
            if (_idx.col.isOpenEnd())
                _idx.col.setRange(0, _idx.col.front() + nStrings - nCurrentComponent - 1);

            for (int n = nCurrentComponent; n < (int)nStrings; n++)
            {
                if (!strRes.vResult[n].length()
                    || (_idx.col[n] == VectorIndex::INVALID))
                    break;

                _data.setHeadLineElement(_idx.col[n], sTableName, removeQuotationMarks(maskControlCharacters(strRes.vResult[n])));
            }

            nCurrentComponent = nStrings;
        }
        else if (_accessParser.isCluster())
        {
            // Write the return values to the cluster with
            // parsing them
            mu::value_type* v = nullptr;
            int nResults = 0;
            int nthComponent = 0;
            NumeRe::Cluster& cluster = _data.getCluster(sTableName);

            // Clusters are overwritten, if the last index
            // is not explictly set
            if (_idx.row.back() == -2 && _idx.row.front() == 0)
                cluster.clear();

            for (size_t i = nCurrentComponent; i < strRes.vResult.size(); i++)
            {
                // Set expression and evaluate it (not efficient but currently necessary)
                if (strRes.vResult[i].front() == '"')
                {
                    // Special case: only one single value
                    if (_idx.row.size() == 1 && _idx.col.size() == 1)
                    {
                        cluster.setString(_idx.row.front(), strRes.vResult[i]);
                        break;
                    }

                    if (_idx.row[nthComponent] == VectorIndex::INVALID)
                        break;

                    cluster.setString(_idx.row[nthComponent], strRes.vResult[i]);
                    nthComponent++;
                }
                else
                {
                    _parser.SetExpr(strRes.vResult[i]);
                    v = _parser.Eval(nResults);
                    strRes.vNumericalValues.insert(strRes.vNumericalValues.end(), v, v+nResults);

                    // Special case: only one single value
                    if (_idx.row.size() == 1 && _idx.col.size() == 1)
                    {
                        cluster.setDouble(_idx.row.front(), v[0]);

                        break;
                    }

                    // Write the single values
                    for (int j = 0; j < nResults; j++)
                    {
                        if (_idx.row[nthComponent] == VectorIndex::INVALID)
                            break;

                        cluster.setDouble(_idx.row[nthComponent], v[j]);

                        nthComponent++;
                    }
                }
            }

            nCurrentComponent = nStrings;
        }
        else
        {
            // Write the return values to the data table with
            // parsing them
            mu::value_type* v = nullptr;
            int nResults = 0;
            int nthComponent = 0;
            string sExpr;
            size_t nOffset = 0;
            const size_t nMAXSETLENGTH = 5000;
            size_t nVectorOffset = strRes.vNumericalValues.size();

            while (nCurrentComponent + nOffset < strRes.vResult.size())
            {
                sExpr.clear();

                for (size_t i = nCurrentComponent + nOffset; i < min(nCurrentComponent+nOffset+nMAXSETLENGTH, strRes.vResult.size()); i++)
                {
                    sExpr += strRes.vResult[i] + ",";
                }

                // Set expression and evaluate it (not efficient but currently necessary)
                _parser.SetExpr(sExpr.substr(0, sExpr.length()-1));
                v = _parser.Eval(nResults);
                strRes.vNumericalValues.insert(strRes.vNumericalValues.end(), v, v+nResults);
                nOffset += nMAXSETLENGTH;
            }

            // Special case: only one single value
            if (_idx.row.size() == 1 && _idx.col.size() == 1)
            {
                _data.writeToTable(_idx.row.front(), _idx.col.front(), sTableName, strRes.vNumericalValues[nVectorOffset]);
            }
            else
            {
                // Write the single values
                for (size_t j = nVectorOffset; j < strRes.vNumericalValues.size(); j++)
                {
                    if (_idx.row.size() == 1 && _idx.col.size() > 1)
                    {
                        if (_idx.col[nthComponent] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row.front(), _idx.col[nthComponent], sTableName, strRes.vNumericalValues[j]);
                    }
                    else if (_idx.row.size() > 1 && _idx.col.size() == 1)
                    {
                        if (_idx.row[nthComponent] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row[nthComponent], _idx.col.front(), sTableName, strRes.vNumericalValues[j]);
                    }

                    nthComponent++;
                }
            }


            nCurrentComponent = nStrings;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringParser::storeStringResults(). It will
    /// store the strings into the string object.
    ///
    /// \param vFinal const vector<string>&
    /// \param sObject string&
    /// \param nCurrentComponent size_t&
    /// \param nStrings size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::storeStringToStringObject(const vector<string>& vFinal, string& sObject, size_t& nCurrentComponent, size_t nStrings)
    {
        Indices _idx = getIndices(sObject, _parser, _data, _option);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front()+nStrings-nCurrentComponent-1);

        if (!_idx.col.isValid())
            _idx.col.front() = 0;

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (nCurrentComponent == nStrings)
                return;

            _data.writeString(removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])), _idx.row[i], _idx.col.front());
            nCurrentComponent++;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function stores the
    /// processed and calculated string results in
    /// their desired targets.
    ///
    /// \param strRes StringResult&
    /// \param __sObject string
    /// \return int
    ///
    /////////////////////////////////////////////////
    int StringParser::storeStringResults(StringResult& strRes, string __sObject)
    {
        // Only do something, if the target object is not empty
        if (!__sObject.length())
            return 1;

        strRes.vNumericalValues.clear();

        // Handle remaining vector braces
        if (__sObject.find('{') != string::npos)
            convertVectorToExpression(__sObject, _option);

        string sObject;
        size_t nStrings = strRes.vResult.size();
        size_t nCurrentComponent = 0;

        // As long as the target object is not empty
        while (__sObject.length())
        {
            // Get-cut the next target
            sObject = getNextArgument(__sObject, true);

            // Examine the current target
            if (_data.containsTablesOrClusters(sObject))
            {
                // Store the strings into the data object
                storeStringToDataObjects(strRes, sObject, nCurrentComponent, nStrings);
            }
            else if (sObject.find("string(") != string::npos)
            {
                // Store the strings into the string object
                storeStringToStringObject(strRes.vResult, sObject, nCurrentComponent, nStrings);
            }
            else if (containsStringVars(sObject))
            {
                // Store the strings in string variables
                StripSpaces(sObject);
                if (sObject.find(' ') != string::npos)
                    return 0;
                try
                {
                    if (nCurrentComponent >= nStrings)
                        setStringValue(sObject, "");
                    else
                        setStringValue(sObject, removeQuotationMarks(maskControlCharacters(strRes.vResult[nCurrentComponent])));
                    nCurrentComponent++;
                }
                catch (...)
                {
                    throw;
                }
            }
            else
            {
                // Store the results as numerical values
                StripSpaces(sObject);
                if (sObject.find(' ') != string::npos)
                {
                    return 0;
                }

                // Search for the adress of the current variable
                if (getPointerToVariable(sObject, _parser))
                {
                    if (strRes.vNoStringVal.size() > nCurrentComponent && !strRes.vNoStringVal[nCurrentComponent])
                    {
                        return 0;
                    }
                }

                // If this is a numerical value
                if (strRes.vNoStringVal.size() > nCurrentComponent && strRes.vNoStringVal[nCurrentComponent])
                {
                    try
                    {
                        // Parse and store it
                        int nResults = 0;
                        mu::value_type* v = 0;
                        _parser.SetExpr(sObject + " = " + strRes.vResult[nCurrentComponent]);
                        v = _parser.Eval(nResults);

                        // Replace the evaluated expression with its result,
                        // which will be shown in the terminal. The actual precision
                        // remains untouched!
                        if (nResults)
                        {
                            vAns = v[0];
                            NumeReKernel::getInstance()->getAns().clear();
                            NumeReKernel::getInstance()->getAns().setDoubleArray(nResults, v);
                            string sValues;
                            strRes.vNumericalValues.insert(strRes.vNumericalValues.end(), v, v+nResults);

                            // Transform the results into a string
                            for (int n = 0; n < nResults; n++)
                            {
                                sValues += toString(v[n], _option.getPrecision()) + ",";
                            }
                            sValues.pop_back();

                            // replace the current expression
                            strRes.vResult[nCurrentComponent] = sValues;
                        }

                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
                else if (strRes.vNoStringVal.size() <= nCurrentComponent)
                {
                    // Fallback: try to find the variable address
                    // although it doesn't seem to be a numerical value
                    if (!getPointerToVariable(sObject, _parser))
                    {
                        try
                        {
                            // If there's no numerical value, create
                            // a new string variable
                            setStringValue(sObject, "");
                            nCurrentComponent++;
                        }
                        catch (...)
                        {
                            throw;
                        }
                    }
                }
                else
                {
                    try
                    {
                        // Create a new string variable
                        setStringValue(sObject, removeQuotationMarks(maskControlCharacters(strRes.vResult[nCurrentComponent])));
                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
            }
        }

        return 1;
    }


    /////////////////////////////////////////////////
    /// \brief This member function converts the
    /// processed string parser results into an output
    /// string, which is formatted for the console.
    ///
    /// \param StrRes StringResult&
    /// \param sLine string&
    /// \param parserFlags int
    /// \param bSilent bool
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::createStringOutput(StringResult& StrRes, string& sLine, int parserFlags, bool bSilent)
    {
        sLine.clear();
        NumeRe::Cluster& ans = NumeReKernel::getInstance()->getAns();
        ans.clear();

        vector<string>& vFinal = StrRes.vResult;
        vector<bool>& vIsNoStringValue = StrRes.vNoStringVal;

        // Catch the cases, where the results are
        // numerical only here
        if (StrRes.bOnlyLogicals)
        {
            if (StrRes.vNumericalValues.size())
                sLine = NumeReKernel::getInstance()->getParser().CreateTempVectorVar(StrRes.vNumericalValues);
            else
            {
                for (size_t i = 0; i < vFinal.size(); i++)
                    sLine += vFinal[i] + ",";

                sLine.pop_back();

            }

            return sLine;
        }

        string sConsoleOut;
        string sCurrentComponent;

        if (parserFlags & PEEK || !(NumeReKernel::bSupressAnswer || bSilent))
            sConsoleOut = createTerminalOutput(StrRes, parserFlags);

        // Every result in the current return values
        // is a single string result
        for (size_t j = 0; j < vFinal.size(); j++)
        {
            vFinal[j] = removeQuotationMarks(vFinal[j]);

            // In this case, no conversions are done
            // the results are simply put together and returned
            if (parserFlags & KEEP_MASKED_CONTROL_CHARS && parserFlags & KEEP_MASKED_QUOTES)
            {
                if (!(parserFlags & NO_QUOTES) && !vIsNoStringValue[j])
                    sLine += "\"" + vFinal[j] + "\"";
                else
                    sLine += vFinal[j];

                if (j < vFinal.size() - 1)
                    sLine += ",";

                continue;
            }

            if (!vIsNoStringValue[j])
            {
                sCurrentComponent.clear();
                // Start the current string value with a quotation mark
                // if it is not a special case
                if (!(parserFlags & NO_QUOTES))
                    sCurrentComponent += "\"";

                // Go through the current string value
                for (size_t k = 0; k < vFinal[j].length(); k++)
                {
                    // If there are escaped control characters,
                    // Replace them with their actual value here
                    if (k + 1 < vFinal[j].length() && vFinal[j][k] == '\\')
                    {
                        //\not\neq\ni
                        if (vFinal[j][k + 1] == 'n' && !isToken("nu", vFinal[j], k+1) && !isToken("neq", vFinal[j], k+1)) // Line break
                        {
                            sCurrentComponent += "\n";
                            k++;
                        }
                        else if (vFinal[j][k + 1] == 't' && !isToken("tau", vFinal[j], k+1) && !isToken("theta", vFinal[j], k+1) && !isToken("times", vFinal[j], k+1)) // tabulator
                        {
                            sCurrentComponent += "\t";
                            k++;
                        }
                        else if (vFinal[j][k + 1] == '"') // quotation mark
                        {
                            if (!(parserFlags & KEEP_MASKED_QUOTES))
                                sCurrentComponent += "\"";
                            else
                                sCurrentComponent += "\\\"";

                            k++;
                        }
                        else if (vFinal[j][k + 1] == ' ') // backslash itself
                        {
                            if (!(parserFlags & KEEP_MASKED_CONTROL_CHARS))
                                sCurrentComponent += "\\";
                            else
                                sCurrentComponent += "\\ ";

                            k++;
                        }
                        else
                        {
                            sCurrentComponent += "\\";
                        }
                    }
                    else
                    {
                        // Otherwise simply append the current character
                        sCurrentComponent += vFinal[j][k];
                    }
                }

                // End the current string value with a quotation mark
                // if it is not a special case
                if (!(parserFlags & NO_QUOTES) && !vIsNoStringValue[j])
                    sCurrentComponent += "\"";

                ans.push_back(sCurrentComponent);
                sLine += sCurrentComponent;
            }
            else
            {
                _parser.SetExpr(vFinal[j]);
                int nResults = 0;
                mu::value_type* v = _parser.Eval(nResults);
                string stres;

                for (int k = 0; k < nResults-1; k++)
                {
                    ans.push_back(v[k]);
                    stres = toCmdString(v[k]);
                    sLine += stres + ", ";
                }

                ans.push_back(v[nResults-1]);
                stres = toCmdString(v[nResults-1]);
                sLine += stres;
            }

            // Break the loop, if this was the last string value
            if (j + 1 == vFinal.size())
                break;

            // Add a comma, if neither the next nor the current string
            // is a special case
            sLine += ", ";
        }

        return sConsoleOut;
    }


    /////////////////////////////////////////////////
    /// \brief This private member function creates
    /// the output specialized for the terminal
    ///
    /// \param strRes StringResult&
    /// \param parserFlags int
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::createTerminalOutput(StringResult& strRes, int parserFlags)
    {
        // Shall the output be printed?
        if (parserFlags & NO_QUOTES && parserFlags & PEEK)
        {
            string sConsoleOut = "|-> ";
            bool bLineBreaks = false;

            // Every result in the current return values
            // is a single string result
            for (size_t j = 0; j < strRes.vResult.size(); j++)
            {
                strRes.vResult[j] = removeQuotationMarks(strRes.vResult[j]);

                if (!strRes.vNoStringVal[j])
                {
                    // Go through the current string value
                    for (size_t k = 0; k < strRes.vResult[j].length(); k++)
                    {
                        // If there are escaped control characters,
                        // Replace them with their actual value here
                        if (k + 1 < strRes.vResult[j].length() && strRes.vResult[j][k] == '\\')
                        {
                            //\not\neq\ni
                            if (strRes.vResult[j][k + 1] == 'n' && !isToken("nu", strRes.vResult[j], k+1) && !isToken("neq", strRes.vResult[j], k+1)) // Line break
                            {
                                sConsoleOut += "\n";
                                bLineBreaks = true;
                                k++;
                            }
                            else if (strRes.vResult[j][k + 1] == 't' && !isToken("tau", strRes.vResult[j], k+1) && !isToken("theta", strRes.vResult[j], k+1) && !isToken("times", strRes.vResult[j], k+1)) // tabulator
                            {
                                sConsoleOut += "\t";
                                k++;
                            }
                            else if (strRes.vResult[j][k + 1] == '"') // quotation mark
                            {
                                sConsoleOut += "\"";
                                k++;
                            }
                            else if (strRes.vResult[j][k + 1] == ' ') // backslash itself
                            {
                                sConsoleOut += "\\";
                                k++;
                            }
                            else
                            {
                                sConsoleOut += "\\";
                            }
                        }
                        else
                        {
                            // Otherwise simply append the current character
                            if (strRes.vResult[j][k] == '\n')
                                bLineBreaks = true;

                            sConsoleOut += strRes.vResult[j][k];
                        }
                    }
                }
                else
                {
                    _parser.SetExpr(strRes.vResult[j]);
                    int nResults = 0;
                    mu::value_type* v = _parser.Eval(nResults);
                    string stres;

                    for (int k = 0; k < nResults-1; k++)
                    {
                        stres = toString(v[k], _option.getPrecision());
                        sConsoleOut += stres + ", ";
                    }

                    stres = toString(v[nResults-1], _option.getPrecision());
                    sConsoleOut += stres;
                }

                // Break the loop, if this was the last string value
                if (j + 1 == strRes.vResult.size())
                    break;

                // Add a comma, if neither the next nor the current string
                // is a special case
                sConsoleOut += ", ";
            }

            // Replace all line break characters with the console
            // indentation, if that is needed
            if (bLineBreaks)
                replaceAll(sConsoleOut, "\n", "\n|   ");

            return sConsoleOut;

        }
        else
        {
            vector<string> vStringResult;
            // Every result in the current return values
            // is a single string result
            for (size_t j = 0; j < strRes.vResult.size(); j++)
            {
                if (parserFlags & NO_QUOTES)
                    vStringResult.push_back(removeQuotationMarks(strRes.vResult[j]));
                else
                    vStringResult.push_back(strRes.vResult[j]);

                if (!strRes.vNoStringVal[j])
                {
                    // Go through the current string value
                    for (size_t k = 0; k < vStringResult[j].length(); k++)
                    {
                        // If there are escaped control characters,
                        // Replace them with their actual value here
                        if (k + 1 < vStringResult[j].length() && vStringResult[j][k] == '\\')
                        {
                            if (vStringResult[j][k + 1] == '"') // quotation mark
                                vStringResult[j].erase(k, 1);
                            else if (vStringResult[j][k + 1] == ' ') // backslash itself
                                vStringResult[j].erase(k+1, 1);
                        }
                    }
                }
                else
                {
                    _parser.SetExpr(vStringResult[j]);
                    vStringResult.pop_back();
                    int nResults = 0;
                    mu::value_type* v = _parser.Eval(nResults);

                    for (int k = 0; k < nResults-1; k++)
                    {
                        vStringResult.push_back(toString(v[k], _option.getPrecision()));
                    }

                    vStringResult.push_back(toString(v[nResults-1], _option.getPrecision()));

                }

            }

            return "|-> " + NumeReKernel::formatResultOutput(vStringResult);
        }

    }


    /////////////////////////////////////////////////
    /// \brief This member function finds and decodes
    /// all passed string expression parameters and
    /// removes them from the expression.
    ///
    /// \param sLine string&
    /// \return int
    ///
    /////////////////////////////////////////////////
    int StringParser::decodeStringParams(string& sLine)
    {
        int parserFlags = NO_FLAG;
        vector<int> vPositions;

        // Search for every parameter and add the
        // corresponding flag, if the parameter was
        // detected
        for (auto iter = m_mStringParams.begin(); iter != m_mStringParams.end(); ++iter)
        {
            vPositions.push_back(findParameter(sLine, iter->first));

            if (vPositions.back())
                parserFlags |= iter->second;
        }

        // Return, if no flags were attached
        // to the current expression
        if (!vPositions.size())
            return parserFlags;

        // Sort the vector (only about 6 elements,
        // should be quite fast) to obtain the
        // smallest, non-zero object quite fast
        sort(vPositions.begin(), vPositions.end());

        // Find the start of the parameter list and
        // erase them from the expression
        for (size_t i = 0; i < vPositions.size(); i++)
        {
            if (vPositions[i])
            {
                sLine.erase(sLine.rfind('-', vPositions[i]));
                break;
            }
        }

        // Return the calculated flags
        return parserFlags;
    }


    /////////////////////////////////////////////////
    /// \brief This member function determines, whether
    /// the passed string is simple, i.e. it is a
    /// string literal without any operation.
    ///
    /// \param sLine const string&
    /// \return bool
    ///
    /// This function is called mainly first in the
    /// recursion cycle to avoid not necessary
    /// calculations, because already evaluated string
    /// literals mainly do not need any processing.
    /////////////////////////////////////////////////
    bool StringParser::isSimpleString(const string& sLine)
    {
        size_t nQuotes = 0;

        // Got through the string
        for (size_t i = 0; i < sLine.size(); i++)
        {
            if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
            {
                // Count quotation marks
                nQuotes++;
                continue;
            }

            // Not in quotes and not a space?
            if (!(nQuotes % 2) && !isspace(sLine[i]) && sLine[i] != ',')
                return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Determines, whether the passed token
    /// can be found at the passed position.
    ///
    /// \param sToken const char*
    /// \param sLine const string&
    /// \param pos size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringParser::isToken(const char* sToken, const string& sLine, size_t pos)
    {
        size_t nLength = strlen(sToken);

        if (sLine.length() < pos + nLength)
            return false;

        if (sLine.substr(pos, nLength) == sToken && (sLine.length() == pos+nLength || isDelimiter(sLine[pos+nLength])))
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function masks the line
    /// break and the tabulator control characters for
    /// storing the processed results.
    ///
    /// \param sString string
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::maskControlCharacters(string sString)
    {
        replaceAll(sString, "\n", "\\n");
        replaceAll(sString, "\t", "\\t");
        return sString;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies some
    /// elementary string operations like concatenation
    /// to the string expression.
    ///
    /// \param vFinal vector<string>&
    /// \param bReturningLogicals bool&
    /// \return vector<bool>
    ///
    /////////////////////////////////////////////////
    vector<bool> StringParser::applyElementaryStringOperations(vector<string>& vFinal, bool& bReturningLogicals)
    {
        vector<bool> vIsNoStringValue(vFinal.size(), false);

        // Examine the whole passed vector
        for (unsigned int n = 0; n < vFinal.size(); n++)
        {
            // Strip whitespaces and ensure that the
            // current component is not empy
            StripSpaces(vFinal[n]);

            if (!vFinal[n].length())
                continue;

            // If there are logical expressions in the current
            // component, handle them here
            if (detectStringLogicals(vFinal[n]))
            {
                vFinal[n] = evalStringLogic(vFinal[n], bReturningLogicals);
                StripSpaces(vFinal[n]);
            }

            // Concatenate the strings
            if (vFinal[n].front() == '"' || vFinal[n].back() == '"')
                concatenateStrings(vFinal[n]);

            // Determine, whether the current component is a string
            // or a numerical expression
            if (vFinal[n].front() != '"' && vFinal[n].back() != '"')
                vIsNoStringValue[n] = true;
        }

        // check, whether there's a string left
        bReturningLogicals = true;

        for (size_t i = 0; i < vIsNoStringValue.size(); i++)
        {
            if (!vIsNoStringValue[i])
            {
                bReturningLogicals = false;
                break;
            }
        }

        return vIsNoStringValue;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function provides
    /// the string parser core functionality and is
    /// the function, which is called recursively.
    ///
    /// \param sLine string&
    /// \param sCache string
    /// \param bParseNumericals bool
    /// \return StringResult
    ///
    /////////////////////////////////////////////////
    StringResult StringParser::eval(string& sLine, string sCache, bool bParseNumericals)
    {
        StringResult strRes;

        bool bObjectContainsTablesOrClusters = false;

        // If the current line is a simple string,
        // Strip the surrounding spaces and return directly
        // This saves between 30-70% of the evaluation time
        if (!sCache.length() && isSimpleString(sLine))
        {
            StripSpaces(sLine);

            while (sLine.length())
            {
                strRes.vResult.push_back(getNextArgument(sLine, true));
                strRes.vNoStringVal.push_back(false);
            }

            return strRes;
        }

        StringExpression strExpr(sLine, sCache);

        // Recurse for multiple store targets
        // Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.
        if (strExpr.nEqPos && strExpr.sLine.find(',') != string::npos)
        {
            // Get the left part of the assignment
            string sStringObject = strExpr.sLine.substr(0, strExpr.nEqPos);

            // Ensure that the left part contains a data or a string object,
            // otherwise it is cleared
            if (sStringObject.substr(sStringObject.find_first_not_of(' '), 7) == "string("
                    || _data.containsTablesOrClusters(sStringObject)
                    || sStringObject[sStringObject.find_first_not_of(' ')] == '{')
            {
                // Find the equal sign after the closing parenthesis
                size_t nPos = getMatchingParenthesis(strExpr.sLine);
                nPos = strExpr.sLine.find('=', nPos);

                // Ensure that it exists
                if (nPos == string::npos)
                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

                // Get the data object including the assignment operator and
                // cut this part from the original line
                if (strExpr.sLine[nPos + 1] == '=')
                    nPos++;

                sStringObject = strExpr.sLine.substr(0, nPos + 1);
                strExpr.sLine.erase(0, nPos + 1);
            }
            else
                sStringObject.clear();

            // Remove whitespaces
            StripSpaces(strExpr.sLine);

            // If the current line contains more than one string expressions
            if (strExpr.sLine != getNextArgument(strExpr.sLine, false))
            {
                string sRecursion = "";
                vector<string> vResult;

                // While the current line is not empty
                while (strExpr.sLine.length())
                {
                    // Cut of the first argument
                    sRecursion = getNextArgument(strExpr.sLine, true);

                    // Apply the string parser on the current argument and store its
                    // results in a common vector
                    StringResult _res = eval(sRecursion, "");

                    for (size_t n = 0; n < _res.vResult.size(); n++)
                        vResult.push_back(_res.vResult[n]);
                }

                // Create a new string vector variable from the results
                strExpr.sLine = createStringVectorVar(vResult);
            }

            // Prepend the stored string object to the current input line
            if (sStringObject.length())
                strExpr.sLine = sStringObject + strExpr.sLine;

            strExpr.findAssignmentOperator();
        }

        // Find the start of the asignee
        size_t n_pos = 0;

        if (strExpr.nEqPos)
            n_pos = strExpr.nEqPos+1;

        // Get the string variables
        if (containsStringVars(strExpr.sLine.substr(n_pos)))
            getStringValues(strExpr.sLine, n_pos);

        // Does the current line contain candidates
        // for string functions?
        if (strExpr.sLine.find('(') != string::npos)
        {
            //Timer timer("String function application");
            // Apply the standard string functions
            strExpr.sLine = applyStringFuncs(strExpr.sLine);

            // Apply the special string functions
            strExpr.sLine = applySpecialStringFuncs(strExpr.sLine);

            // Get the position of the equal sign after the
            // application of all string functions
            strExpr.findAssignmentOperator();
        }

        strExpr.split();

        if (strExpr.sAssignee.length() && _data.containsTablesOrClusters(strExpr.sAssignee))
            bObjectContainsTablesOrClusters = true;

        // Get the contents of "string()", "data()" and the other caches
        strExpr.sLine = getDataForString(strExpr.sLine, 0);

        // If the line now doesn't contain any strings, return with the onlyLogicals flag set
        if (!isStringExpression(strExpr.sLine) && !bObjectContainsTablesOrClusters)
        {
            // make sure that there are parseable characters between the operators
            if (strExpr.sLine.find_first_not_of("+-*/:?!.,;%&<>=^ ") != string::npos && bParseNumericals)
            {
                int nResults = 0;
                mu::value_type* v = 0;

                // Parse the epression
                if (strExpr.sAssignee.length())
                    _parser.SetExpr(strExpr.sAssignee + "=" + strExpr.sLine);
                else
                    _parser.SetExpr(strExpr.sLine);

                v = _parser.Eval(nResults);
                vAns = v[0];
                NumeReKernel::getInstance()->getAns().clear();
                NumeReKernel::getInstance()->getAns().setDoubleArray(nResults, v);
                //eq_pos = strExpr.sLine.find('=');
                //strExpr.findAssignmentOperator();

                // Remove the left part of the assignment, because it's already assigned
                //if (eq_pos != string::npos
                //    && eq_pos
                //    && eq_pos < sLine.length() + 1
                //    && isAssignmentOperator(sLine, eq_pos))
                //    sLine.erase(0, eq_pos + 1);

                StripSpaces(strExpr.sLine);

                // Return with the numbers already contained
                return StringResult(strExpr.sLine, v, nResults);
            }

            // return with the onlyLogicals flag
            return StringResult(strExpr.sLine, true);
        }
        else if (!bObjectContainsTablesOrClusters && !strExpr.sAssignee.length() && isSimpleString(strExpr.sLine))
        {
            // After the data objects are resolved,
            // it's quite possible that there are only
            // simple strings left in this recursion
            StripSpaces(strExpr.sLine);

            while (strExpr.sLine.length())
            {
                strRes.vResult.push_back(getNextArgument(strExpr.sLine, true));
                strRes.vNoStringVal.push_back(false);
            }

            return strRes;
        }

        // If there are any opening parentheses, it is possible that we need
        // to pre-evaluate its contents
        if (strExpr.sLine.find('(') != string::npos)
        {
            size_t nQuotes = 0;

            // Examine the whole command line
            for (size_t i = 0; i < strExpr.sLine.length(); i++)
            {
                // Jump over string literals
                if (strExpr.sLine[i] == '"')
                {
                    if (i && strExpr.sLine[i - 1] == '\\')
                        continue;
                    nQuotes++;
                }

                // Consider the var parsing feature
                if (strExpr.sLine[i] == '#' && !(nQuotes % 2))
                {
                    // Examine, whether the var parsing feature was used
                    // correctly and search for the end of its current application
                    for (size_t j = i; j < strExpr.sLine.length(); j++)
                    {
                        if (strExpr.sLine[j] == '"')
                            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

                        if ((strExpr.sLine[j] == '(' || strExpr.sLine[j] == '{') && getMatchingParenthesis(strExpr.sLine.substr(j)) != string::npos)
                            j += getMatchingParenthesis(sLine.substr(j));

                        if (strExpr.sLine[j] == ' ' || strExpr.sLine[j] == '+')
                        {
                            i = j;
                            break;
                        }

                        if (j + 1 == strExpr.sLine.length())
                        {
                            i = j;
                        }
                    }
                }

                // Examine the current parenthesis: is it able to pre-evaluate the
                // current parenthesis?
                if (strExpr.sLine[i] == '(' && !(nQuotes % 2))
                {
                    // Ensure that its counterpart exists
                    if (getMatchingParenthesis(strExpr.sLine.substr(i)) == string::npos)
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, strExpr.sLine, i);

                    size_t nPos = getMatchingParenthesis(strExpr.sLine.substr(i)) + i;

                    // Ignore any calls to "string()"
                    if (i < 6 || strExpr.sLine.substr(i - 6, 6) != "string")
                    {
                        // The contents of the parenthesis
                        string sString = strExpr.sLine.substr(i + 1, nPos - i - 1);

                        // Pre-evaluate the contents of the parenthesis
                        if (i > 0 && !checkDelimiter(strExpr.sLine.substr(i - 1, nPos - i + 2))) // this is probably a numerical function. Keep the parentheses
                        {
                            // Do only something, if the contents are containings strings
                            if (isStringExpression(sString))
                            {
                                StringResult _res = eval(sString, "");
                                string strvar = createStringVectorVar(_res.vResult);

                                if (!strvar.length())
                                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

                                strExpr.sLine = strExpr.sLine.substr(0, i + 1) + strvar + strExpr.sLine.substr(nPos);
                            }
                        }
                        else // replace the whole parenthesis
                        {
                            // Do only something, if the contents are containing strings
                            if (isStringExpression(sString))
                            {
                                StringResult _res = eval(sString, "");
                                string strvar = createStringVectorVar(_res.vResult);

                                if (!strvar.length())
                                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

                                strExpr.sLine = strExpr.sLine.substr(0, i) + strvar + strExpr.sLine.substr(nPos + 1);
                            }
                        }
                    }
                }
            }
        }

        // Are there any vector braces left?
        if (strExpr.sLine.find('{') != string::npos)
        {
            size_t nQuotes = 0;

            // Examine each vector brace
            for (size_t i = 0; i < strExpr.sLine.length(); i++)
            {
                if (strExpr.sLine[i] == '"' && (!i || strExpr.sLine[i-1] != '\\'))
                    nQuotes++;

                if (!(nQuotes % 2) && strExpr.sLine[i] == '{')
                {
                    size_t nmatching = getMatchingParenthesis(strExpr.sLine.substr(i));
                    string sVectorTemp = strExpr.sLine.substr(i+1, nmatching-1);

                    // Does the vector brace contain colons? Then it
                    // might be a numerical vector expansion if it
                    // is not a string expression.
                    // CAREFUL: Colons are also part of file paths!
                    if (sVectorTemp.find(':') != string::npos)
                    {
                        if (!isStringExpression(sVectorTemp))
                        {
                            // Try to evaluate the vector as
                            // vector expansion
                            _parser.SetExpr("{" + sVectorTemp + "}");
                            int nRes = 0;
                            mu::value_type* res = _parser.Eval(nRes);

                            // Store the result in a vector and
                            // create a temporary vector variable
                            vector<mu::value_type> vRes(res, res + nRes);
                            strExpr.sLine.replace(i, nmatching+1, _parser.CreateTempVectorVar(vRes));

                            continue;
                        }
                        else if (sVectorTemp.find('"') == string::npos && !containsStringVectorVars(sVectorTemp))
                            continue;
                    }

                    if (!isStringExpression(sVectorTemp))
                    {
                        // Try to evaluate the vector as
                        // vector expansion
                        _parser.SetExpr(sVectorTemp);
                        int nRes = 0;
                        mu::value_type* res = _parser.Eval(nRes);

                        // Store the result in a vector and
                        // create a temporary vector variable
                        vector<mu::value_type> vRes(res, res + nRes);
                        strExpr.sLine.replace(i, nmatching+1, _parser.CreateTempVectorVar(vRes));

                        continue;
                    }

                    StringResult tempres = eval(sVectorTemp, "");

                    if (!tempres.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

                    // Create a string vector variable from the returned vector
                    // if it is a string. Otherwise we create a new temporary
                    // vector from the parser using the numerical results
                    if (!tempres.bOnlyLogicals)
                        sVectorTemp = createStringVectorVar(tempres.vResult);
                    else if (tempres.vNumericalValues.size())
                        sVectorTemp = _parser.CreateTempVectorVar(tempres.vNumericalValues);
                    else if (tempres.vResult.size() == 1 && tempres.vResult.front().find_first_of(":,") != std::string::npos)
                    {
                        _parser.SetExpr("{" + tempres.vResult.front() + "}");
                        int nRes = 0;
                        mu::value_type* res = _parser.Eval(nRes);

                        // Store the result in a vector and
                        // create a temporary vector variable
                        vector<mu::value_type> vRes(res, res + nRes);
                        sVectorTemp = _parser.CreateTempVectorVar(vRes);
                    }
                    // TODO: What happens in the remaining case of multiple
                    // elements in tempres.vResult?

                    strExpr.sLine.replace(i, nmatching+1, sVectorTemp);
                }
            }
        }

        // Strip all whitespaces and ensure, that there's something left
        StripSpaces(strExpr.sLine);

        if (!strExpr.sLine.length())
            return StringResult("");

        // If the current line doesn't contain any further string literals or return values
        // return it
        if (strExpr.sLine.find('"') == string::npos && strExpr.sLine.find('#') == string::npos && !containsStringVectorVars(strExpr.sLine) && !bObjectContainsTablesOrClusters)
        {
            // Ensure that this is no false positive
            if (strExpr.sLine.find("string(") != string::npos || containsStringVars(strExpr.sLine))
                throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

            // return the current line
            return StringResult(strExpr.sLine);
        }

        // Apply the "#" parser to the string
        strExpr.sLine = numToString(strExpr.sLine);

        // Split the list to a vector
        strRes.vResult = evaluateStringVectors(strExpr.sLine);

        // Ensure that there is at least one result
        if (!strRes.vResult.size())
            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

        // Apply some elementary operations such as concatenation and logical operations
        strRes.vNoStringVal = applyElementaryStringOperations(strRes.vResult, strRes.bOnlyLogicals);

        // store the string results in the variables or inb "string()" respectively
        if (!storeStringResults(strRes, strExpr.sAssignee))
            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position);

        // Return the evaluated string command line
        return strRes;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function evaluates
    /// the passed string expression and formats the
    /// results for the console.
    ///
    /// \param sLine string&
    /// \param sCache string&
    /// \param bSilent bool
    /// \return StringParser::StringParserRetVal
    ///
    /////////////////////////////////////////////////
    StringParser::StringParserRetVal StringParser::evalAndFormat(string& sLine, string& sCache, bool bSilent)
    {
        //Timer timer(sLine);
        sLine = " " + sLine + " ";

        // Process the parameters and store their
        // results in the parser flags
        int parserFlags = decodeStringParams(sLine);

        // Perform the actual work in the string parser core function
        // This function will parse the string, call the corresponding
        // static functions and perform all required logical operations
        StringResult StrRes = eval(sLine, sCache);

        // Clear the internal string vector variables, because they're
        // all evaluated and processed now
        removeStringVectorVars();

        // Check the results, if the assertion handler is active
        // and the results are not logical only
        _assertionHandler.checkAssertion(StrRes);

        // The result of the string parser core has to be parsed, so that
        // it is readable in the terminal. This is done here in this
        // function
        string sConsoleOut = createStringOutput(StrRes, sLine, parserFlags, bSilent);

        // The output is probably not desired
        if (NumeReKernel::bSupressAnswer)
            bSilent = true;

        // Print the output
        if ((!bSilent || parserFlags & PEEK) && !StrRes.bOnlyLogicals)
        {
            // The Line break function is not needed any more
            // because the terminal is capable of breaking the
            // lines by itself
            if (parserFlags & PEEK)
                NumeReKernel::printPreFmt("\r" + sConsoleOut + "\n");
            else
                NumeReKernel::printPreFmt(sConsoleOut + "\n");
        }

        // return the corresponding value. The string parser may
        // evaluate the command line to only contain logical or
        // numerical expressions
        if (StrRes.bOnlyLogicals)
            return STRING_NUMERICAL;

        return STRING_SUCCESS;
    }


    /////////////////////////////////////////////////
    /// \brief Returns true, if the passed expression
    /// is an expression containing strings, string
    /// variables or string vector variables.
    ///
    /// \param sExpression const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringParser::isStringExpression(const string& sExpression)
    {
        if (sExpression.find_first_of("\"#") != string::npos
			|| sExpression.find("string(") != string::npos
			|| sExpression.find("string_cast(") != string::npos
			|| sExpression.find("char(") != string::npos
			|| sExpression.find("valtostr(") != string::npos)
            return true;

		if (containsStringVars(sExpression) || containsStringVectorVars(sExpression))
            return true;

        if (sExpression.find('{') == string::npos)
            return false;

        const map<string,NumeRe::Cluster>& mClusterMap = _data.getClusterMap();

        for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
        {
            if (iter->second.isMixed() || iter->second.isString())
            {
                size_t pos = sExpression.find(iter->first + "{");

                if (pos != string::npos && (!pos || isDelimiter(sExpression[pos-1])))
                    return true;
            }
        }

        return false;
    }
}

