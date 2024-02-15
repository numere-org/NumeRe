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
#include "../structures.hpp"

extern value_type vAns;

std::string removeQuotationMarks(const std::string& sString);
std::string addQuotationMarks(const std::string& sString);

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
    /// \param sLine std::string
    /// \param n_pos size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::getDataForString(std::string sLine, size_t n_pos)
    {
        // We disable the caching here, because it is not supported for
        // recursive evaluations
        _parser.DisableAccessCaching();

        // Get the contents of "string()", "data()" and the other caches
        size_t nEndPosition;

        if (sLine.find_first_of("({") == std::string::npos)
            return sLine;

        // {str} = string(...)
        while ((n_pos = findNextFunction("string(", sLine, n_pos, nEndPosition)) != std::string::npos)
        {
            // Check for arguments to parse and
            // get the indices afterwards
            Indices _idx = getIndices("string(" + parseStringsInIndices(getFunctionArgumentList("string(", sLine, n_pos, nEndPosition).to_string()) + ")", _parser, _data, _option, false);

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
                std::string sLeft = sLine.substr(0, n_pos);
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
                    std::string sString = "";
                    std::vector<std::string> vStrings;

                    for (size_t i = 0; i < _idx.row.size(); i++)
                    {
                        vStrings.push_back("\"" +  _data.readString((size_t)_idx.row[i], _idx.col.front()) + "\"");
                    }

                    sString = createStringVectorVar(vStrings);
                    sLine = sLine.substr(0, n_pos) + sString + sLine.substr(nEndPosition + 1);
                }
                else
                    sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nEndPosition + 1);
            }

            n_pos++;
        }

        if (sLine.find_first_of("({") == std::string::npos)
            return sLine;

        // Replace calls to any table
        for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
        {
            if (sLine.find('(') == std::string::npos)
                break;

            replaceDataOccurence(sLine, iter->first + "(");
        }

        if (sLine.find('{') == std::string::npos)
            return sLine;

        // Replace calls to any cluster
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
        {
            if (sLine.find('{') == std::string::npos)
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
    /// \param sIndexExpression std::string
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::parseStringsInIndices(std::string sIndexExpression)
    {
        // Ensure that this is an expression,
        // which actually contains strings
        if (!isStringExpression(sIndexExpression))
            return sIndexExpression;

        std::string sParsedIndices;

        // As long as the passed index expression
        // still has a length
        while (sIndexExpression.length())
        {
            // Get the next index pair and evaluate
            // its strings
            std::string sIndexPairs = getNextArgument(sIndexExpression, true);

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

            EndlessVector<std::string> idx = getAllIndices(sIndexPairs + " ");
            sIndexPairs.clear();

            for (size_t i = 0; i < idx.size(); i++)
            {
                if (isStringExpression(idx[i]))
                {
                    // Cannot handle things like A: (without terminating index)
                    StringResult strRes = eval(idx[i], "", false);

                    if (!strRes.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, idx[i], SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

                    // Check, if the return values are
                    // only strings
                    if (strRes.bOnlyLogicals && strRes.vResult.size() > 1)
                    {
                        std::vector<mu::value_type> vIndices;

                        // Convert the strings to doubles
                        for (size_t i = 0; i < strRes.vResult.size(); i++)
                            vIndices.push_back(StrToDb(strRes.vResult[i].to_string()));

                        // Create a temporary vector
                        idx[i] = _parser.CreateTempVectorVar(vIndices);
                    }
                    else if (strRes.vResult.size() > 1)
                        idx[i] = createTempStringVectorVar(strRes.vResult); // Only add string vector variables, if necessary
                    else
                        idx[i] = strRes.vResult.getRef(0);
                }

                sIndexPairs += idx[i];

                if (i+1 < idx.size())
                    sIndexPairs += ":";
            }

            if (sParsedIndices.length())
                sParsedIndices += ",";

            sParsedIndices += sIndexPairs;
        }

        return sParsedIndices;
    }


    /////////////////////////////////////////////////
    /// \brief Replaces all occurences of the passed
    /// data access occurence in the passed string.
    ///
    /// \param sLine std::string&
    /// \param sOccurence const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::replaceDataOccurence(std::string& sLine, const std::string& sOccurence)
    {
        size_t nStartPosition = 0;
        size_t nEndPosition;

        // Find the next occurence
        while ((nStartPosition = findNextFunction(sOccurence, sLine, nStartPosition, nEndPosition, true)) != std::string::npos)
        {
            size_t nStartPos = nStartPosition;
            size_t nEndPos = nEndPosition;
            bool isMultArgFunc = false;
            // Handle multi-argument functions
            if (parser_CheckMultArgFunc(sLine.substr(0, nStartPosition), sLine.substr(nEndPosition + 1)))
            {
                if (nStartPosition > 4 && sLine.substr(sLine.rfind('(', nStartPosition) - 4, 5) == "norm(")
                    nStartPos -= 5;
                else
                    nStartPos -= 4;

                isMultArgFunc = true;
                nEndPos++;
            }

            std::string sData = sLine.substr(nStartPos, nEndPos - nStartPos + 1);

            // Is the current data access a method?
            if (sData.find("().") != std::string::npos)
            {
                if (containsStringVectorVars(sData))
                {
                    bool temp;
                    std::vector<std::string> vRes = evaluateStringVectors(sData);
                    applyElementaryStringOperations(vRes, temp);
                    sData = vRes.front();
                }

                getDataElements(sData, _parser, _data, _option,
                                REPLACE_NAN | (nStartPos && (sLine[nStartPos-1] == '#' || sLine[nStartPos-1] == '~') ? INSERT_STRINGS : 0));
            }
            else
            {
                sData.replace(nStartPosition-nStartPos+sOccurence.length(),
                              nEndPosition-nStartPosition-sOccurence.length(),
                              parseStringsInIndices(getFunctionArgumentList(sOccurence, sLine, nStartPosition, nEndPosition).to_string()));

                // Get the data and parse string expressions
                replaceDataEntities(sData, sOccurence, _data, _parser, _option,
                                    REPLACE_NAN | (!isMultArgFunc && nStartPos && (sLine[nStartPos-1] == '#' || sLine[nStartPos-1] == '~') ? INSERT_STRINGS : 0));
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
    /// \param sLine const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::numToString(const std::string& sLine)
    {
        // Do nothing, if there's no #
        if (sLine.find('#') == std::string::npos)
            return sLine + " ";

        // Create a copy of the current line
        StringView sLineToParsed = sLine; // + " ";
        std::string sLineToParsedTemp;

        size_t nPos = 0;
        size_t n_pos = 0;

        // As long as there are further "#"
        while (sLineToParsed.find('#', nPos) != std::string::npos)
        {
            // Store its position
            nPos = sLineToParsed.find('#', nPos);

            // Ensure that it is not in a string
            if (!isInQuotes(sLineToParsed, nPos, true))
            {
                StringView sPrefix;
                sLineToParsedTemp += sLineToParsed.subview(0, nPos).to_string();
                sLineToParsed.trim_front(nPos+1);

                // If the variable starts with a tilde, it's possible that we
                // need to prepend zeros. Get the prefix here
                if (sLineToParsed[0] == '~')
                {
                    for (size_t i = 0; i < sLineToParsed.length(); i++)
                    {
                        if (sLineToParsed[i] != '~')
                        {
                            sPrefix = sLineToParsed.subview(0, i);
                            sLineToParsed.trim_front(i);
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
                    std::string sExpr = sLineToParsed.subview(1, getMatchingParenthesis(sLineToParsed) - 1).to_string();

                    // Does it contain strings?
                    if (isStringExpression(sExpr))
                    {
                        StringResult strRes = eval(sExpr, "");
                        if (!strRes.vResult.size())
                            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

                        // Examine, whether the return value is logical only
                        if (!strRes.bOnlyLogicals)
                        {
                            // This case contains strings. We will cast the whole
                            // result into a string
                            for (size_t i = 0; i < strRes.vResult.size(); i++)
                            {
                                strRes.vResult.convert_to_string(i, sPrefix.length());
                            }

                            // Create a vector variable from the return value
                            sExpr = createStringVectorVar(strRes.vResult);
                            sLineToParsedTemp += sExpr;

                            // Get the next part of the command line
                            if (getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)) < sLineToParsed.length())
                                sLineToParsed.trim_front(getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)));
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
                    std::string sExpr = sLineToParsed.subview(1, sLineToParsed.find('"', 1) - 1).to_string();

                    // Add the zeros, if needed
                    while (sExpr.length() < sPrefix.length() + 2)
                        sExpr.insert(0, 1, '0');

                    // Store the result and continue
                    sLineToParsedTemp += "\"" + sExpr + "\"";
                    if (sLineToParsed.find('"', 1) < sLineToParsed.length() - 1)
                        sLineToParsed.trim_front(sLineToParsed.find('"', 1) + 1);
                    else
                        sLineToParsed.clear();
                    nPos = 0;
                    continue;
                }
                else if (sLineToParsed[0] == '<')
                {
                    // Does the current object contain a path token?
                    if (sLineToParsed.starts_with("<>")
                        || sLineToParsed.starts_with("<this>")
                        || sLineToParsed.starts_with("<wp>")
                        || sLineToParsed.starts_with("<loadpath>")
                        || sLineToParsed.starts_with("<savepath>")
                        || sLineToParsed.starts_with("<plotpath>")
                        || sLineToParsed.starts_with("<procpath>")
                        || sLineToParsed.starts_with("<scriptpath>"))
                    {
                        // Replace the path tokens
                        if (sLineToParsed.starts_with("<>") || sLineToParsed.starts_with("<this>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getExePath()) + "\"";
                        else if (sLineToParsed.starts_with("<wp>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getWorkPath()) + "\"";
                        else if (sLineToParsed.starts_with("<loadpath>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getLoadPath()) + "\"";
                        else if (sLineToParsed.starts_with("<savepath>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getSavePath()) + "\"";
                        else if (sLineToParsed.starts_with("<plotpath>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getPlotPath()) + "\"";
                        else if (sLineToParsed.starts_with("<procpath>"))
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getProcPath()) + "\"";
                        else
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getScriptPath()) + "\"";

                        sLineToParsed.trim_front(sLineToParsed.find('>') + 1);
                    }
                    else if (sLineToParsed.find('>') != std::string::npos)
                    {
                        // If no path token, only use the part in between of the angle brackets
                        sLineToParsedTemp += "\"" + sLineToParsed.subview(1, sLineToParsed.find('>') - 1) + "\"";
                        sLineToParsed.trim_front(sLineToParsed.find('>') + 1);
                    }
                    else
                    {
                        // Throw an error
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INVALID_PATHIDENTIFIER"));
                    }
                    nPos = 0;
                    continue;
                }
                else if (containsStringVectorVars(sLineToParsed.subview(0, getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)))))
                {
                    // Here are string vector variables
                    std::string sExpr = sLineToParsed.subview(0, getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos))).to_string();

                    // Parse the current line
                    StringResult strRes = eval(sExpr, "");

                    // Ensure that the result exists
                    if (!strRes.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

                    // Add the needed quotation marks
                    for (size_t i = 0; i < strRes.vResult.size(); i++)
                    {
                        strRes.vResult.convert_to_string(i, sPrefix.length());
                    }

                    // Create a new string vector var, append it to the string and continue
                    sExpr = createStringVectorVar(strRes.vResult);
                    sLineToParsedTemp += sExpr;

                    if (getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)) < sLineToParsed.length())
                        sLineToParsed.trim_front(getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)));
                    else
                        sLineToParsed.clear();

                    continue;
                }
                else // Set the expression
                    _parser.SetExpr(sLineToParsed.subview(0, getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos))));

                int nResults = 0;
                mu::value_type* v = 0;
                std::vector<std::string> vResults;
                std::string sElement = "";

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
                if (getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)) < sLineToParsed.length())
                    sLineToParsed.trim_front(getPositionOfFirstDelimiter(sLineToParsed.subview(n_pos)));
                else
                    sLineToParsed.clear();

                nPos = 0;
            }
            else
                nPos++;
        }

        // Append the remaining parts of the current expression
        if (sLineToParsed.length() && sLineToParsedTemp.length())
            sLineToParsedTemp += sLineToParsed.to_string();

        // Handle remaining vector braces
        if (sLineToParsedTemp.find('{') != std::string::npos)
            convertVectorToExpression(sLineToParsedTemp, _option);

        // Determine the return value
        if (sLineToParsedTemp.length())
            return sLineToParsedTemp;

        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringParser::storeStringResults(). It will
    /// store the strings into the data tables.
    ///
    /// \param strRes StringResult&
    /// \param sObject std::string&
    /// \param nCurrentComponent size_t&
    /// \param nStrings size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::storeStringToDataObjects(StringResult& strRes, std::string& sObject, size_t& nCurrentComponent, size_t nStrings)
    {
        // Identify the correct table
        DataAccessParser _accessParser(sObject, true);
        //NumeReKernel::print(sObject);
        //NumeReKernel::print(_accessParser.getIndices().row.to_string());
        //NumeReKernel::print(_accessParser.getIndices().sCompiledAccessEquation);

        if (!_accessParser.getDataObject().length())
            return;

        std::string sTableName = _accessParser.getDataObject();
        Indices& _idx = _accessParser.getIndices();

        // Is the target a headline or is it a regular index?
        if (_idx.row.isString())
        {
            // If the first element is non-zero but the second is,
            // we use the number of elements as upper boundary
            _idx.col.setOpenEndIndex(_idx.col.front() + nStrings-nCurrentComponent-1);

            for (size_t i = 0; i < _idx.col.size(); i++)
            {
                if ((i + nCurrentComponent >= nStrings && nStrings > 1)
                    || _idx.col[i] == VectorIndex::INVALID)
                    break;

                _data.setHeadLineElement(_idx.col[i], sTableName,
                                         strRes.vResult[nStrings > 1 ? i+nCurrentComponent : 0].to_string());
            }
        }
        else if (_accessParser.isCluster())
        {
            // Write the return values to the cluster with
            // parsing them
            mu::value_type* v = nullptr;
            int nResults = 0;
            NumeRe::Cluster& cluster = _data.getCluster(sTableName);

            // Clusters are overwritten, if the last index
            // is not explictly set
            if ((_idx.row.isOpenEnd() || (_idx.row.isExpanded() && _idx.row.last() == (int)cluster.size()-1)) && _idx.row.front() == 0)
                cluster.clear();

            //_idx.row.setOpenEndIndex(_idx.row.front() + nStrings-nCurrentComponent-1);
            size_t last = _idx.row.isOpenEnd() ? nStrings-nCurrentComponent : _idx.row.size();

            for (size_t i = 0; i < last; i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID
                    || (nStrings > 1 && nCurrentComponent+i >= nStrings))
                    break;

                // Set expression and evaluate it (not efficient but currently necessary)
                if (strRes.vResult.is_string(nStrings > 1 ? i + nCurrentComponent : 0))
                    cluster.setString(_idx.row[i],
                                      strRes.vResult[nStrings > 1 ? i + nCurrentComponent : 0].to_string());
                else
                {
                    _parser.SetExpr(strRes.vResult[nStrings > 1 ? i + nCurrentComponent : 0]);
                    v = _parser.Eval(nResults);
                    strRes.vNumericalValues.insert(strRes.vNumericalValues.end(), v, v+nResults);

                    // Write the single values
                    for (int j = 0; j < nResults; j++)
                    {
                        if (_idx.row[j+i] == VectorIndex::INVALID || j+i >= _idx.row.size())
                            break;

                        cluster.setDouble(_idx.row[j+i], v[j]);
                    }

                    i += nResults-1;
                }
            }
        }
        else
        {
            // Write the return values to the data table with
            // parsing them
            mu::value_type* v = nullptr;
            int nResults = 0;

            bool rewriteColumn = _idx.row.front() == 0 && (_idx.row.isOpenEnd()
                                                           || (_idx.row.isExpanded()
                                                               && _idx.row.last() == _data.getColElements(_idx.col.subidx(0,1),sTableName)-1));

            //_idx.row.setOpenEndIndex(_idx.row.front() + nStrings-nCurrentComponent-1);
            //_idx.col.setOpenEndIndex(_idx.col.front() + nStrings-nCurrentComponent-1);

            size_t last = _idx.row.isOpenEnd() || _idx.col.isOpenEnd()
                ? nStrings-nCurrentComponent
                : std::max(_idx.row.size(), _idx.col.size());

            for (size_t i = 0; i < last; i++)
            {
                if (nStrings > 1 && nCurrentComponent+i >= nStrings)
                    break;

                // Set expression and evaluate it (not efficient but currently necessary)
                if (strRes.vResult.is_string(nStrings > 1 ? i + nCurrentComponent : 0))
                {
                    if (_idx.row.size() == 1 && _idx.col.size() >= 1)
                    {
                        if (_idx.col[i] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row.front(), _idx.col[i], sTableName,
                                           strRes.vResult[nStrings > 1 ? i + nCurrentComponent : 0].to_string());
                    }
                    else if (_idx.row.size() > 1 && _idx.col.size() == 1)
                    {
                        if (!i && rewriteColumn)
                            _data.overwriteColumn(_idx.col.front(), sTableName, TableColumn::TYPE_STRING);

                        if (_idx.row[i] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row[i], _idx.col.front(), sTableName,
                                           strRes.vResult[nStrings > 1 ? i + nCurrentComponent : 0].to_string());
                    }
                }
                else
                {
                    _parser.SetExpr(strRes.vResult[nStrings > 1 ? i + nCurrentComponent : 0]);
                    v = _parser.Eval(nResults);
                    strRes.vNumericalValues.insert(strRes.vNumericalValues.end(), v, v+nResults);

                    for (int j = 0; j < nResults; j++)
                    {
                        if (_idx.row.size() == 1 && _idx.col.size() >= 1)
                        {
                            if (_idx.col[i+j] == VectorIndex::INVALID || j+i >= _idx.col.size())
                                break;

                            _data.writeToTable(_idx.row.front(), _idx.col[i+j], sTableName, v[j]);
                        }
                        else if (_idx.row.size() > 1 && _idx.col.size() == 1)
                        {
                            if (!i
                                && rewriteColumn
                                && ((_data.getType(_idx.col.subidx(0, 1), sTableName) != TableColumn::TYPE_DATETIME
                                     && !TableColumn::isValueType(_data.getType(_idx.col.subidx(0, 1), sTableName)))
                                    || !mu::isreal(v, nResults)))
                                _data.overwriteColumn(_idx.col.front(), sTableName, TableColumn::TYPE_VALUE);

                            if (_idx.row[i] == VectorIndex::INVALID || j+i >= _idx.row.size())
                                break;

                            _data.writeToTable(_idx.row[i+j], _idx.col.front(), sTableName, v[j]);
                        }
                    }

                    i += nResults-1;
                }
            }
        }

        nCurrentComponent = nStrings;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a helper for
    /// StringParser::storeStringResults(). It will
    /// store the strings into the string object.
    ///
    /// \param vFinal const std::vector<std::string>&
    /// \param sObject std::string&
    /// \param nCurrentComponent size_t&
    /// \param nStrings size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::storeStringToStringObject(const std::vector<std::string>& vFinal, std::string& sObject, size_t& nCurrentComponent, size_t nStrings)
    {
        Indices _idx = getIndices(sObject, _parser, _data, _option, true);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front()+nStrings-nCurrentComponent-1);

        if (!_idx.col.isValid())
            _idx.col.front() = 0;

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (nCurrentComponent == nStrings)
                return;

            _data.writeString(removeQuotationMarks(vFinal[nCurrentComponent]), _idx.row[i], _idx.col.front());
            nCurrentComponent++;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function stores the
    /// processed and calculated string results in
    /// their desired targets.
    ///
    /// \param strRes StringResult&
    /// \param __sObject std::string
    /// \return int
    ///
    /////////////////////////////////////////////////
    int StringParser::storeStringResults(StringResult& strRes, std::string __sObject)
    {
        // Only do something, if the target object is not empty
        size_t nBracePos;

        if (!__sObject.length() || (nBracePos = __sObject.find_first_not_of(' ')) == std::string::npos)
            return 1;

        strRes.vNumericalValues.clear();

        // Handle remaining vector braces
        if (__sObject[nBracePos] == '{')
            convertVectorToExpression(__sObject, _option);

        std::string sObject;
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
            else if (sObject.find("string(") != std::string::npos)
            {
                // Store the strings into the string object
                storeStringToStringObject(strRes.vResult, sObject, nCurrentComponent, nStrings);
            }
            else if (containsStringVars(sObject))
            {
                // Store the strings in string variables
                StripSpaces(sObject);

                if (sObject.find(' ') != std::string::npos)
                    return 0;

                if (nCurrentComponent >= nStrings)
                    setStringValue(sObject, "");
                else
                    setStringValue(sObject, strRes.vResult[nCurrentComponent].to_string());

                // Do not increment singletons
                if (nStrings > 1)
                    nCurrentComponent++;
            }
            else
            {
                // Store the results as numerical values
                StripSpaces(sObject);

                if (sObject.find(' ') != std::string::npos)
                    return 0;

                // Search for the adress of the current variable
                if (getPointerToVariable(sObject, _parser))
                {
                    if (strRes.vNoStringVal.size() > nCurrentComponent && !strRes.vNoStringVal[nCurrentComponent])
                        return 0;
                }

                // If this is a numerical value
                if (strRes.vNoStringVal.size() > nCurrentComponent && strRes.vNoStringVal[nCurrentComponent])
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
                        NumeReKernel::getInstance()->getAns().setDoubleArray(nResults, v);
                        std::string sValues;
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

                    // Do not increment singletons
                    if (nStrings > 1)
                        nCurrentComponent++;
                }
                else if (strRes.vNoStringVal.size() <= nCurrentComponent && nStrings > 1)
                {
                    // Fallback: try to find the variable address
                    // although it doesn't seem to be a numerical value
                    if (!getPointerToVariable(sObject, _parser))
                    {
                        // If there's no numerical value, create
                        // a new string variable
                        setStringValue(sObject, "");
                        nCurrentComponent++;
                    }
                }
                else
                {
                    // Create a new string variable
                    setStringValue(sObject, strRes.vResult[nCurrentComponent].to_string());

                    // Do not increment singletons
                    if (nStrings > 1)
                        nCurrentComponent++;
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
    /// \param sLine std::string&
    /// \param parserFlags int
    /// \param bSilent bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::createStringOutput(StringResult& StrRes, std::string& sLine, int parserFlags, bool bSilent)
    {
        sLine.clear();
        NumeRe::Cluster& ans = NumeReKernel::getInstance()->getAns();
        ans.clear();

        s_vect& vFinal = StrRes.vResult;
        std::vector<bool>& vIsNoStringValue = StrRes.vNoStringVal;

        // Catch the cases, where the results are
        // numerical only here
        if (StrRes.bOnlyLogicals)
        {
            if (StrRes.vNumericalValues.size())
                sLine = NumeReKernel::getInstance()->getParser().CreateTempVectorVar(StrRes.vNumericalValues);
            else
            {
                for (size_t i = 0; i < vFinal.size(); i++)
                    sLine += vFinal.getRef(i) + ",";

                sLine.pop_back();
            }

            return sLine;
        }

        std::string sConsoleOut;
        //std::string sCurrentComponent;

        if (parserFlags & PEEK || !(NumeReKernel::bSupressAnswer || bSilent))
            sConsoleOut = createTerminalOutput(StrRes, parserFlags);

        // Every result in the current return values
        // is a single string result
        for (size_t j = 0; j < vFinal.size(); j++)
        {
            // In this case, no conversions are done
            // the results are simply put together and returned
            if (parserFlags & KEEP_MASKED_CONTROL_CHARS && parserFlags & KEEP_MASKED_QUOTES)
            {
                if (!(parserFlags & NO_QUOTES) && !vIsNoStringValue[j])
                    sLine += vFinal.getRef(j);
                else
                    sLine += vFinal[j].to_string();

                if (j < vFinal.size() - 1)
                    sLine += ",";

                continue;
            }

            if (!vIsNoStringValue[j])
            {
                ans.push_back(vFinal[j].to_string());

                if (parserFlags & NO_QUOTES)
                    sLine += vFinal[j].to_string();
                else
                    sLine += vFinal.getRef(j);
            }
            else
            {
                _parser.SetExpr(vFinal[j]);
                int nResults = 0;
                mu::value_type* v = _parser.Eval(nResults);
                std::string stres;

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
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::createTerminalOutput(StringResult& strRes, int parserFlags)
    {
        // Shall the output be printed?
        if (parserFlags & NO_QUOTES && parserFlags & PEEK)
        {
            std::string sConsoleOut = "|-> ";
            bool bLineBreaks = false;

            // Every result in the current return values
            // is a single string result
            for (size_t j = 0; j < strRes.vResult.size(); j++)
            {
                if (!strRes.vNoStringVal[j])
                {
                    if (strRes.vResult[j].find('\n') != std::string::npos)
                        bLineBreaks = true;

                    sConsoleOut = sConsoleOut + strRes.vResult[j];
                }
                else
                {
                    _parser.SetExpr(strRes.vResult[j]);
                    int nResults = 0;
                    mu::value_type* v = _parser.Eval(nResults);
                    std::string stres;

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
            std::vector<std::string> vStringResult;
            // Every result in the current return values
            // is a single string result
            for (size_t j = 0; j < strRes.vResult.size(); j++)
            {
                if (!strRes.vNoStringVal[j])
                {
                    if (parserFlags & NO_QUOTES)
                    {
                        vStringResult.push_back(strRes.vResult[j].to_string());
                        replaceAll(vStringResult.back(), "\"", "\\\"");
                    }
                    else
                    {
                        vStringResult.push_back(strRes.vResult.getRef(j));
                        replaceAll(vStringResult.back(), "\"", "\\\"", 1, vStringResult.back().length()-1);
                    }

                    replaceAll(vStringResult.back(), "\n", "\\n");
                    replaceAll(vStringResult.back(), "\t", "\\t");
                }
                else
                {
                    _parser.SetExpr(strRes.vResult[j]);
                    //vStringResult.pop_back();
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
    /// \param sLine std::string&
    /// \return int
    ///
    /////////////////////////////////////////////////
    int StringParser::decodeStringParams(std::string& sLine)
    {
        int parserFlags = NO_FLAG;
        std::vector<int> vPositions;

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
    /// \param sLine const std::string&
    /// \return bool
    ///
    /// This function is called mainly first in the
    /// recursion cycle to avoid not necessary
    /// calculations, because already evaluated string
    /// literals mainly do not need any processing.
    /////////////////////////////////////////////////
    bool StringParser::isSimpleString(const std::string& sLine)
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
    /// \param sLine const std::string&
    /// \param pos size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringParser::isToken(const char* sToken, const std::string& sLine, size_t pos)
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
    /// \param sString std::string
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringParser::maskControlCharacters(std::string sString)
    {
        replaceAll(sString, "\n", "\\n");
        replaceAll(sString, "\t", "\\t");
        return sString;
    }


    /////////////////////////////////////////////////
    /// \brief Static function to obtain the passed
    /// operator's precedence.
    ///
    /// \param oprt StringView
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    static size_t getOperatorPrecedence(StringView oprt)
    {
        if (oprt == "+")
            return 10;

        if (oprt == ":")
            return 7;

        if (oprt == "==" || oprt == "!=" || oprt == "<" || oprt == ">" || oprt == "<=" || oprt == ">=")
            return 5;

        if (oprt == "&&" || oprt == "||" || oprt == "|||")
            return 2;

        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Static function to finalize the
    /// if-else logic in this stack.
    ///
    /// \param rpnStack std::vector<StringStackItem>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    static void finalizeStack(std::vector<StringStackItem>& rpnStack)
    {
        std::stack<size_t> stIf;
        std::stack<size_t> stElse;
        int idx = 0;
        bool lastIsIf = false;

        for (size_t j = 0; j < rpnStack.size(); j++)
        {
            if (rpnStack[j].m_data == "?")
            {
                stIf.push(j);
                lastIsIf = true;
            }

            // Only consider ":" as part of a ? : , if
            // there's already a ? in the stack
            if (rpnStack[j].m_data == ":" && stIf.size())
            {
                if (!lastIsIf && stElse.size())
                {
                    // Finalize this ? : expression
                    idx = stElse.top();
                    stElse.pop();
                    rpnStack[idx].m_val = j;
                }

                // Finalize the if part of the expression
                stElse.push(j);
                idx = stIf.top();
                stIf.pop();
                rpnStack[idx].m_val = j;
                lastIsIf = false;
            }
        }

        // Finalize all remaining ? : expressions
        while (stElse.size())
        {
            idx = stElse.top();
            stElse.pop();
            rpnStack[idx].m_val = rpnStack.size();
        }
    }


    /////////////////////////////////////////////////
    /// \brief Static function to convert the tokens
    /// to a RPN stack.
    ///
    /// \param vTokens const std::vector<StringView>&
    /// \return std::vector<StringStackItem>
    ///
    /////////////////////////////////////////////////
    static std::vector<StringStackItem> convertToStack(const std::vector<StringView>& vTokens)
    {
        std::vector<StringStackItem> rpnStack;
        std::stack<StringView> operators;
        int hasIf = 0;

        // Now convert the tokens to the RPN stack
        for (size_t i = 0; i < vTokens.size(); i++)
        {
            // Odd numbers are operators
            if (i % 2)
            {
                // This case is special, because this is already
                // in RPN
                if (vTokens[i] == "?" || (vTokens[i] == ":" && hasIf > 0))
                {
                    // Clear all operators first
                    while (operators.size())
                    {
                        rpnStack.push_back(StringStackItem(operators.top(), 0));
                        operators.pop();
                    }

                    rpnStack.push_back(StringStackItem(vTokens[i], 0));
                    hasIf += (vTokens[i] == "?") - (vTokens[i] == ":");
                    continue;
                }

                // This case is special, because it creates an additional
                // return value stack
                if (vTokens[i] == "," && !hasIf)
                {
                    // Clear all operators first
                    while (operators.size())
                    {
                        rpnStack.push_back(StringStackItem(operators.top(), 0));
                        operators.pop();
                    }

                    rpnStack.push_back(StringStackItem(vTokens[i], 0));
                    continue;
                }

                // Operators with larger or equal precedence
                // have to be evaluated first. Remove them from
                // the operator stack
                while (operators.size()
                       && getOperatorPrecedence(operators.top()) >= getOperatorPrecedence(vTokens[i]))
                {
                    rpnStack.push_back(StringStackItem(operators.top(), 0));
                    operators.pop();
                }

                // Add the new operator to the operator stack
                operators.push(vTokens[i]);
            }
            else
                rpnStack.push_back(StringStackItem(vTokens[i]));
        }

        while (operators.size())
        {
            rpnStack.push_back(StringStackItem(operators.top(), 0));
            operators.pop();
        }

        // Finalize the if-else logic
        finalizeStack(rpnStack);

        // Dump the stack
        //std::string sDump;
        //
        //for (size_t i = 0; i < rpnStack.size(); i++)
        //{
        //    sDump += "[" + rpnStack[i].m_data.to_string() + "]";
        //}
        //
        //g_logger.info("RPNSTACK = " + sDump);

        // Nothing more to do. Return the stack
        return rpnStack;
    }


    /////////////////////////////////////////////////
    /// \brief This member function creates a stack
    /// from the passed expression, which may then be
    /// evaluated more easily.
    ///
    /// \param sExpr StringView
    /// \return std::vector<StringStackItem>
    ///
    /////////////////////////////////////////////////
    std::vector<StringStackItem> StringParser::createStack(StringView sExpr) const
    {
        std::vector<StringView> vTokens;

        const std::string BINARYOPS = "+?:<>!=&|,";
        size_t nQuotes = 0;
        int hasIf = 0;

        // As long as the expression has a length
        while (sExpr.length())
        {
            // Remove all surrounding whitespace
            sExpr.strip();

            // Go through the expression
            for (size_t i = 0; i < sExpr.length(); i++)
            {
                // Count quotes
                if (sExpr[i] == '"' && (!i || sExpr[i-1] != '\\'))
                {
                    nQuotes++;

                    if (!(nQuotes % 2))
                    {
                        // The next non-whitespace character should be
                        // a valid operator or the end of the line
                        size_t nextChar = sExpr.find_first_not_of(' ', i+1);

                        if (nextChar != std::string::npos && BINARYOPS.find(sExpr[nextChar]) == std::string::npos)
                            throw SyntaxError(SyntaxError::STRING_ERROR, sExpr.get_viewed_string(),
                                              nextChar+sExpr.get_offset(), _lang.get("ERR_NR_3603_MISSING_OPERATOR"));
                    }
                    else if (i) // If the quotation mark is not the first element, s.th. is wrong
                        throw SyntaxError(SyntaxError::STRING_ERROR, sExpr.get_viewed_string(),
                                          i+sExpr.get_offset(), _lang.get("ERR_NR_3603_MISSING_OPERATOR"));
                }

                if (nQuotes % 2)
                    continue;

                // Jump over possible parentheses
                if (sExpr[i] == '(' || sExpr[i] == '[' || sExpr[i] == '{')
                {
                    i += getMatchingParenthesis(sExpr.subview(i));

                    // The next non-whitespace character should be
                    // a valid operator or the end of the line
                    size_t nextChar = sExpr.find_first_not_of(' ', i+1);

                    if (nextChar != std::string::npos && (isalnum(sExpr[nextChar]) || sExpr[nextChar] == '"'))
                        throw SyntaxError(SyntaxError::STRING_ERROR, sExpr.get_viewed_string(),
                                          nextChar+sExpr.get_offset(), _lang.get("ERR_NR_3603_MISSING_OPERATOR"));
                }

                // Detect operators
                if (sExpr.subview(i, 3) == "|||")
                {
                    // Prepare the token
                    StringView sToken = sExpr.subview(0, i);
                    sToken.strip();

                    // Store token and operator
                    vTokens.push_back(sToken);
                    vTokens.push_back(sExpr.subview(i, 3));

                    // Remove the beginning and restart the loop
                    sExpr.trim_front(i+3);
                    break;
                }
                else if (sExpr.subview(i, 2) == "=="
                    || sExpr.subview(i, 2) == "!="
                    || sExpr.subview(i, 2) == "<="
                    || sExpr.subview(i, 2) == ">="
                    || sExpr.subview(i, 2) == "&&"
                    || sExpr.subview(i, 2) == "||")
                {
                    // Prepare the token
                    StringView sToken = sExpr.subview(0, i);
                    sToken.strip();

                    // Store token and operator
                    vTokens.push_back(sToken);
                    vTokens.push_back(sExpr.subview(i, 2));

                    // Remove the beginning and restart the loop
                    sExpr.trim_front(i+2);
                    break;
                }
                else if (sExpr[i] == '+'
                         || sExpr[i] == '?'
                         || sExpr[i] == ':'
                         || sExpr[i] == ','
                         || sExpr[i] == '<'
                         || sExpr[i] == '>')
                {
                    if (sExpr[i] == ':' && hasIf <= 0)
                    {
                        if (i+1 == sExpr.length())
                            throw SyntaxError(SyntaxError::STRING_ERROR, sExpr.get_viewed_string(),
                                              i+sExpr.get_offset(), _lang.get("ERR_NR_3603_MISSING_OPERAND"));

                        continue;
                    }
                    else if (sExpr[i] == ',' && hasIf > 0)
                    {
                        if (i+1 == sExpr.length())
                            throw SyntaxError(SyntaxError::STRING_ERROR, sExpr.get_viewed_string(),
                                              i+sExpr.get_offset(), _lang.get("ERR_NR_3603_MISSING_OPERAND"));

                        continue;
                    }
                    else if (sExpr[i] == ':')
                        hasIf--;
                    else if (sExpr[i] == '?')
                        hasIf++;

                    // Prepare the token
                    StringView sToken = sExpr.subview(0, i);
                    sToken.strip();

                    // Store token and operator
                    vTokens.push_back(sToken);
                    vTokens.push_back(sExpr.subview(i, 1));

                    // Remove the beginning and restart the loop
                    sExpr.trim_front(i+1);
                    break;
                }

                // Is this the last token?
                if (i+1 == sExpr.length())
                {
                    vTokens.push_back(sExpr);

                    // Now convert the tokens to the RPN stack
                    // and return
                    return convertToStack(vTokens);
                }
            }
        }

        // Return the stack (should actually never hit this line)
        g_logger.warning("Returned from StringParser::createStack() from an unexpected statement.");
        return std::vector<StringStackItem>();
    }


    /////////////////////////////////////////////////
    /// \brief Evaluate the created RPN stack between
    /// the selected start and end points.
    ///
    /// \param rpnStack const std::vector<StringStackItem>&
    /// \param from size_t
    /// \param to size_t
    /// \return StringVector
    ///
    /////////////////////////////////////////////////
    StringVector StringParser::evaluateStack(const std::vector<StringStackItem>& rpnStack, size_t from, size_t to)
    {
        std::stack<StringVector> valueStack;
        size_t nReturnValues = 1;

        for (size_t i = from; i < to; i++)
        {
            // This is a value/variable
            if (rpnStack[i].m_val < 0)
            {
                std::string sValue = rpnStack[i].m_data.to_string();

                if (sValue.front() == '"') // This is needed because the math parser does not handle string literals
                    valueStack.push(StringVector::convert_literal(sValue));
                else if (isStringVectorVar(sValue))
                    valueStack.push(getStringVectorVar(sValue));
                else if (containsStringVectorVars(sValue) || _parser.ContainsVectorVars(sValue, false))
                    valueStack.push(evaluateStringVectors(sValue));
                else
                    valueStack.push(StringVector::convert_literal(sValue));
            }
            else
            {
                if (rpnStack[i].m_data == "+")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() += op;
                }
                else if (rpnStack[i].m_data == ",")
                {
                    nReturnValues++;
                    continue;
                }
                else if (rpnStack[i].m_data == "==")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() == op;
                }
                else if (rpnStack[i].m_data == "!=")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() != op;
                }
                else if (rpnStack[i].m_data == ">=")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() >= op;
                }
                else if (rpnStack[i].m_data == "<=")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() <= op;
                }
                else if (rpnStack[i].m_data == ">")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() > op;
                }
                else if (rpnStack[i].m_data == "<")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top() < op;
                }
                else if (rpnStack[i].m_data == "&&")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top().and_f(op);
                }
                else if (rpnStack[i].m_data == "||")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top().or_f(op);
                }
                else if (rpnStack[i].m_data == "|||")
                {
                    StringVector op = valueStack.top();
                    valueStack.pop();
                    valueStack.top() = valueStack.top().xor_f(op);
                }
                else if (rpnStack[i].m_data == "?")
                {
                    // Get the absolute jump points
                    size_t nIfEnd = rpnStack[i].m_val;
                    size_t nElseEnd = rpnStack[nIfEnd].m_val;

                    // Evaluate the results recursively (lazy is not possible within this logic)
                    valueStack.top().evalIfElse(valueStack.top(),
                                                evaluateStack(rpnStack, i+1, nIfEnd),
                                                evaluateStack(rpnStack, nIfEnd+1, nElseEnd));
                    // Jump this evaluation to the end of the else
                    // statement (most probably also the end of the
                    // end of the current stack)
                    i = nElseEnd;
                }
            }

            //std::string stackTop;
            //
            //for (size_t i = 0; i < valueStack.top().size(); i++)
            //    stackTop += valueStack.top().getRef(i).substr(0, 100) + ",";
            //
            //stackTop.pop_back();
            //g_logger.info("STACK.TOP() = " + stackTop);
            //g_logger.info("STACK.TOP() = " + valueStack.top().getRef(0).substr(0, 100));
        }

        // return the value on top of the stack
        if (nReturnValues > valueStack.size())
            throw std::out_of_range("Stack size (" + toString(valueStack.size()) + ") is smaller than the requested number of return values (" + toString(nReturnValues) + ").");

        StringVector vRet = valueStack.top();
        valueStack.pop();
        nReturnValues--;

        while (nReturnValues)
        {
            vRet.insert(vRet.begin(), valueStack.top().begin(), valueStack.top().end());
            valueStack.pop();
            nReturnValues--;
        }

        return vRet;
    }


    /////////////////////////////////////////////////
    /// \brief Create a stack from the expression and
    /// evaluate it.
    ///
    /// \param sExpr StringView
    /// \return StringResult
    ///
    /////////////////////////////////////////////////
    StringResult StringParser::createAndEvaluateStack(StringView sExpr)
    {
        std::vector<StringStackItem> rpnStack = createStack(sExpr);
        StringResult res;

        res.vResult = evaluateStack(rpnStack, 0, rpnStack.size());
        res.vNoStringVal.resize(res.vResult.size());
        res.bOnlyLogicals = true;

        for (size_t i = 0; i < res.vResult.size(); i++)
        {
            res.vNoStringVal[i] = !res.vResult.is_string(i);

            if (!res.vNoStringVal[i])
                res.bOnlyLogicals = false;
        }

        return res;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies some
    /// elementary string operations like concatenation
    /// to the string expression.
    ///
    /// \param vFinal std::vector<std::string>&
    /// \param bReturningLogicals bool&
    /// \return std::vector<bool>
    ///
    /////////////////////////////////////////////////
    std::vector<bool> StringParser::applyElementaryStringOperations(vector<std::string>& vFinal, bool& bReturningLogicals)
    {
        std::vector<bool> vIsNoStringValue(vFinal.size(), false);

        // Examine the whole passed vector
        for (size_t n = 0; n < vFinal.size(); n++)
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

            // Concatenate the std::strings
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
    /// \param sLine std::string&
    /// \param sCache std::string
    /// \param bParseNumericals bool
    /// \return StringResult
    ///
    /////////////////////////////////////////////////
    StringResult StringParser::eval(std::string& sLine, std::string sCache, bool bParseNumericals)
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
                strRes.vResult.push_generic(getNextArgument(sLine, true));
                strRes.vNoStringVal.push_back(false);
            }

            return strRes;
        }

        StringExpression strExpr(sLine, sCache);

        // Recurse for multiple store targets
        // Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.
        if (strExpr.nEqPos && strExpr.sLine.find(',') != std::string::npos)
        {
            // Get the left part of the assignment
            std::string sStringObject = strExpr.sLine.substr(0, strExpr.nEqPos);

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
                if (nPos == std::string::npos)
                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, getMatchingParenthesis(strExpr.sLine), _lang.get("ERR_NR_3603_MISSING_ASSIGNMENT"));

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
                std::string sRecursion = "";
                std::vector<std::string> vResult;

                // While the current line is not empty
                while (strExpr.sLine.length())
                {
                    // Cut of the first argument
                    sRecursion = getNextArgument(strExpr.sLine, true);

                    // Apply the string parser on the current argument and store its
                    // results in a common vector
                    StringResult _res = eval(sRecursion, "");

                    for (size_t n = 0; n < _res.vResult.size(); n++)
                        vResult.push_back(_res.vResult.getRef(n));
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
        if (containsStringVars(StringView(strExpr.sLine, n_pos)))
            getStringValuesAsInternalVar(strExpr.sLine, n_pos);

        // Does the current line contain candidates
        // for string functions?
        if (strExpr.sLine.find('(') != std::string::npos)
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
            if (strExpr.sLine.find_first_not_of("+-*/:?!.,;%&<>=^ ") != std::string::npos && bParseNumericals)
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
                NumeReKernel::getInstance()->getAns().setDoubleArray(nResults, v);
                //eq_pos = strExpr.sLine.find('=');
                //strExpr.findAssignmentOperator();

                // Remove the left part of the assignment, because it's already assigned
                //if (eq_pos != std::string::npos
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
                strRes.vResult.push_generic(getNextArgument(strExpr.sLine, true));
                strRes.vNoStringVal.push_back(false);
            }

            return strRes;
        }

        // If there are any opening parentheses, it is possible that we need
        // to pre-evaluate its contents
        if (strExpr.sLine.find('(') != std::string::npos)
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
                            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, j, _lang.get("ERR_NR_3603_UNEXPECTED_LITERAL"));

                        if ((strExpr.sLine[j] == '(' || strExpr.sLine[j] == '{') && getMatchingParenthesis(StringView(strExpr.sLine, j)) != std::string::npos)
                            j += getMatchingParenthesis(StringView(sLine, j));

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
                    if (getMatchingParenthesis(StringView(strExpr.sLine, i)) == std::string::npos)
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, strExpr.sLine, i);

                    size_t nPos = getMatchingParenthesis(StringView(strExpr.sLine, i)) + i;

                    // Ignore any calls to "string()"
                    if (i < 6 || strExpr.sLine.substr(i - 6, 6) != "string")
                    {
                        // The contents of the parenthesis
                        std::string sString = strExpr.sLine.substr(i + 1, nPos - i - 1);

                        // Pre-evaluate the contents of the parenthesis
                        if (i > 0 && !StringView(strExpr.sLine).is_delimited_sequence(i, nPos-i)) // this is probably a numerical function. Keep the parentheses
                        {
                            // Do only something, if the contents are containings strings
                            if (isStringExpression(sString))
                            {
                                StringResult _res = eval(sString, "");
                                std::string strvar = createStringVectorVar(_res.vResult);

                                if (!strvar.length())
                                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

                                strExpr.sLine = strExpr.sLine.substr(0, i + 1) + strvar + strExpr.sLine.substr(nPos);
                            }
                        }
                        else // replace the whole parenthesis
                        {
                            // Do only something, if the contents are containing strings
                            if (isStringExpression(sString))
                            {
                                StringResult _res = eval(sString, "");
                                std::string strvar = createStringVectorVar(_res.vResult);

                                if (!strvar.length())
                                    throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

                                strExpr.sLine = strExpr.sLine.substr(0, i) + strvar + strExpr.sLine.substr(nPos + 1);
                            }
                        }
                    }
                }
            }
        }

        // Are there any vector braces left?
        if (strExpr.sLine.find('{') != std::string::npos)
        {
            size_t nQuotes = 0;

            // Examine each vector brace
            for (size_t i = 0; i < strExpr.sLine.length(); i++)
            {
                if (strExpr.sLine[i] == '"' && (!i || strExpr.sLine[i-1] != '\\'))
                    nQuotes++;

                if (!(nQuotes % 2) && strExpr.sLine[i] == '{')
                {
                    size_t nmatching = getMatchingParenthesis(StringView(strExpr.sLine, i));
                    std::string sVectorTemp = strExpr.sLine.substr(i+1, nmatching-1);

                    if (!sVectorTemp.length()) // Empty brace == nan
                    {
                        strExpr.sLine.replace(i, nmatching+1, "nan");
                        continue;
                    }

                    // Does the vector brace contain colons? Then it
                    // might be a numerical vector expansion if it
                    // is not a string expression.
                    // CAREFUL: Colons are also part of file paths!
                    if (sVectorTemp.find(':') != std::string::npos)
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
                            std::vector<mu::value_type> vRes(res, res + nRes);
                            strExpr.sLine.replace(i, nmatching+1, _parser.CreateTempVectorVar(vRes));

                            continue;
                        }
                        else if (sVectorTemp.find('"') == std::string::npos && !containsStringVectorVars(sVectorTemp))
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
                        std::vector<mu::value_type> vRes(res, res + nRes);
                        strExpr.sLine.replace(i, nmatching+1, _parser.CreateTempVectorVar(vRes));

                        continue;
                    }

                    StringResult tempres = eval(sVectorTemp, "");

                    if (!tempres.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INTERNAL"));

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
                        std::vector<mu::value_type> vRes(res, res + nRes);
                        sVectorTemp = _parser.CreateTempVectorVar(vRes);
                    }
                    else // Should in fact never reach this point
                        continue;

                    strExpr.sLine.replace(i, nmatching+1, sVectorTemp);
                }
            }
        }

        // Strip all whitespaces and ensure, that there's something left
        StripSpaces(strExpr.sLine);

        if (!strExpr.sLine.length())
            return StringResult("");

        // If the current line doesn't contain any further std::string literals or return values
        // return it
        if (strExpr.sLine.find('"') == std::string::npos
            && strExpr.sLine.find('#') == std::string::npos
            && !containsStringVectorVars(strExpr.sLine)
            && !bObjectContainsTablesOrClusters)
        {
            // Ensure that this is no false positive
            if (strExpr.sLine.find("string(") != std::string::npos || containsStringVars(strExpr.sLine))
                throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INCOMPLETE"));

            // return the current line
            return StringResult(strExpr.sLine);
        }

        // Apply the "#" parser to the string
        strExpr.sLine = numToString(strExpr.sLine);

        // Evaluate the remaining expression
        strRes = createAndEvaluateStack(strExpr.sLine);

        // Ensure that there is at least one result
        if (!strRes.vResult.size())
            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_STACK"));

        // store the string results in the variables or inb "string()" respectively
        if (!storeStringResults(strRes, strExpr.sAssignee))
            throw SyntaxError(SyntaxError::STRING_ERROR, strExpr.sLine, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_ASSIGNMENT"));

        // Return the evaluated string command line
        return strRes;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function evaluates
    /// the passed string expression and formats the
    /// results for the console.
    ///
    /// \param sLine std::string&
    /// \param sCache std::string&
    /// \param bSilent bool
    /// \param bCheckAssertions bool
    /// \param bClearVars bool
    /// \return StringParser::StringParserRetVal
    ///
    /////////////////////////////////////////////////
    StringParser::StringParserRetVal StringParser::evalAndFormat(std::string& sLine, std::string& sCache, bool bSilent, bool bCheckAssertions, bool bClearVars)
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
        if (bClearVars)
            removeStringVectorVars();

        // Check the results, if the assertion handler is active
        // and the results are not logical only
        if (bCheckAssertions)
            _assertionHandler.checkAssertion(StrRes);

        // The result of the string parser core has to be parsed, so that
        // it is readable in the terminal. This is done here in this
        // function
        std::string sConsoleOut = createStringOutput(StrRes, sLine, parserFlags, bSilent);

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
    /// \param sExpression StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringParser::isStringExpression(StringView sExpression)
    {
        size_t nEqPos = findAssignmentOperator(sExpression);

        // hack for performance Re:#178
        if (nEqPos != std::string::npos)
        {
            StringView sAssignee = sExpression.subview(0, nEqPos);
            sAssignee.strip();

            if (!NumeReKernel::getInstance()->getMemoryManager().isTable(sAssignee)
                || getMatchingParenthesis(sAssignee)+1 != sAssignee.length())
                nEqPos = 0;
        }
        else
            nEqPos = 0;

        if (sExpression.find_first_of("\"#", nEqPos) != std::string::npos
			|| sExpression.find("string(", nEqPos) != std::string::npos
			|| sExpression.find("string_cast(", nEqPos) != std::string::npos
			|| sExpression.find("char(", nEqPos) != std::string::npos
			|| sExpression.find("getlasterror(", nEqPos) != std::string::npos
			|| sExpression.find("getuilang(", nEqPos) != std::string::npos
			|| sExpression.find("getversioninfo(", nEqPos) != std::string::npos
			|| sExpression.find("valtostr(", nEqPos) != std::string::npos
			|| sExpression.find("weekday(", nEqPos) != std::string::npos
            || sExpression.find("to_tex(", nEqPos) != std::string::npos)
            return true;

		if (containsStringVars(sExpression.subview(nEqPos)) || containsStringVectorVars(sExpression.subview(nEqPos)))
            return true;

        if (sExpression.find('{', nEqPos) == std::string::npos)
            return false;

        const std::map<std::string,NumeRe::Cluster>& mClusterMap = _data.getClusterMap();

        for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
        {
            if (iter->second.isMixed() || iter->second.isString())
            {
                size_t pos = sExpression.find(iter->first + "{", nEqPos);

                if (pos != std::string::npos && (!pos || isDelimiter(sExpression[pos-1])))
                    return true;
            }
        }

        return false;
    }
}

