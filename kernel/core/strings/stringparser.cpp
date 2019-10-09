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
#include <algorithm>

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
    StringParser::StringParser(mu::Parser& parser, Datafile& data, Settings& option) : StringLogicParser(), StringFuncHandler(), _parser(parser), _data(data), _option(option)
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
        string sDummy;

        // {str} = string(...)
        while (sLine.find("string(", n_pos) != string::npos)
        {
            n_pos = sLine.find("string(", n_pos);

            if (isInQuotes(sLine, n_pos, true))
            {
                n_pos++;
                continue;
            }

            unsigned int nPos = n_pos + 6;

            if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

            nPos += getMatchingParenthesis(sLine.substr(nPos));

            if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, 8))))
            {
                string sString = "";

                if (nPos - n_pos - 7 > 0)
                {
                    sString = sLine.substr(n_pos + 7, nPos - n_pos - 7);
                    StripSpaces(sString);
                }

                StringResult strRes = eval(sString, "", false);

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                sString = strRes.vResult[0];

                Indices _idx = parser_getIndices("string(" + sString + ")", _parser, _data, _option);

                if (_idx.col.isOpenEnd())
                    _idx.col.back() = VectorIndex::INVALID;

                if (_idx.row.isOpenEnd())
                    _idx.row.setRange(0, _data.getStringElements()-1);

                if (!_idx.col.isValid())
                    _idx.col.front() = 0;

                if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
                {
                    string sLeft = sLine.substr(0, n_pos);
                    StripSpaces(sLeft);

                    if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "num(")
                        sLine = sLeft.substr(0, sLeft.length() - 4) + toString(_idx.row.size()) + sLine.substr(sLine.find(')', nPos + 1) + 1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "max(")
                        sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.maxString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "min(")
                        sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.minString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length() - 4) == "sum(")
                        sLine = sLeft.substr(0, sLeft.length() - 4) + "\"" + _data.sumString(_idx.row, _idx.col) + "\"" + sLine.substr(sLine.find(')', nPos + 1) + 1);
                }
                else
                {
                    if (_data.getStringElements(_idx.col.front()))
                    {
                        sString = "";
                        vector<string> vStrings;

                        for (size_t i = 0; i < _idx.row.size(); i++)
                        {
                            vStrings.push_back("\"" +  _data.readString((unsigned int)_idx.row[i], _idx.col.front()) + "\"");
                        }

                        sString = createStringVectorVar(vStrings);
                        sLine = sLine.substr(0, n_pos) + sString + sLine.substr(nPos + 1);
                    }
                    else
                        sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nPos + 1);
                }
            }

            n_pos++;
        }

        // Replace calls to "data()"
        replaceDataOccurence(sLine, "data(");

        // Replace calls to any table
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            replaceDataOccurence(sLine, iter->first + "(");

        // Replace calls to any cluster
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
            replaceDataOccurence(sLine, iter->first + "{");

        return sLine;
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
        size_t n_pos = 0;

        while (sLine.find(sOccurence, n_pos) != string::npos)
        {
            n_pos = sLine.find(sOccurence, n_pos);

            if (isInQuotes(sLine, n_pos, true))
            {
                n_pos++;
                continue;
            }

            size_t nPos = n_pos + sOccurence.length()-1;

            if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

            nPos += getMatchingParenthesis(sLine.substr(nPos));

            if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos - 1, sOccurence.length() + 1))))
            {
                if (parser_CheckMultArgFunc(sLine.substr(0, n_pos), sLine.substr(nPos + 1)))
                {
                    if (n_pos > 4 && sLine.substr(sLine.rfind('(', n_pos) - 4, 5) == "norm(")
                        n_pos -= 5;
                    else
                        n_pos -= 4;

                    nPos++;
                }

                string sData = sLine.substr(n_pos, nPos - n_pos + 1);
                // Get the data and parse string expressions
                getDataElements(sData, _parser, _data, _option);
                StringResult strRes = eval(sData, "");

                if (!strRes.vResult.size())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Create a string vector variable from the returned vector
                // if it is a string. Otherwise simple use the first return
                // value, which already contains a numerical vector
                if (!strRes.bOnlyLogicals)
                    sData = createStringVectorVar(strRes.vResult);
                else
                    sData = strRes.vResult.front();

                sLine = sLine.substr(0, n_pos) + sData + sLine.substr(nPos + 1);
            }
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
                            if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                                sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
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
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getPlotOutputPath()) + "\"";
                        else if (sLineToParsed.find("<procpath>") == 0)
                            sLineToParsedTemp += "\"" + replacePathSeparator(_option.getProcsPath()) + "\"";
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
                else if (containsStringVectorVars(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos)))))
                {
                    // Here are string vector variables
                    string sExpr = sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos)));

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
                    if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                        sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
                    else
                        sLineToParsed.clear();

                    continue;
                }
                else // Set the expression
                    _parser.SetExpr(sLineToParsed.substr(0, parser_getDelimiterPos(sLineToParsed.substr(n_pos))));

                int nResults = 0;
                value_type* v = 0;
                vector<string> vResults;
                string sElement = "";

                // Evaluate the parsed expression
                v = _parser.Eval(nResults);

                // Convert all results into strings
                for (int n = 0; n < nResults; n++)
                {
                    if (fabs(rint(v[n]) - v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                        sElement = toString((long long int)rint(v[n]));
                    else
                        sElement = toString(v[n], _option);
                    while (sElement.length() < sPrefix.length() + 1)
                        sElement.insert(0, 1, '0');
                    vResults.push_back(addQuotationMarks(sElement));
                }

                // Create a string vector var from the results and append it to
                // the current line
                sElement = createStringVectorVar(vResults);
                sLineToParsedTemp += sElement;

                // Search for the next delimiter
                if (parser_getDelimiterPos(sLineToParsed.substr(n_pos)) < sLineToParsed.length())
                    sLineToParsed = sLineToParsed.substr(parser_getDelimiterPos(sLineToParsed.substr(n_pos)));
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
            parser_VectorToExpr(sLineToParsedTemp, _option);
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
    /// \param vFinal const vector<string>&
    /// \param sObject string&
    /// \param nCurrentComponent size_t&
    /// \param nStrings size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringParser::storeStringToDataObjects(const vector<string>& vFinal, string& sObject, size_t& nCurrentComponent, size_t nStrings)
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
            // Write the string to the correct data table
            if (_idx.col.isOpenEnd() && sTableName == "data")
                _idx.col.setRange(0, _data.getCols("data")-1);

            // If the first element is non-zero but the second is,
            // we use the number of elements as upper boundary
            if (_idx.col.isOpenEnd())
                _idx.col.setRange(0, _idx.col.front() + nStrings - nCurrentComponent - 1);

            for (int n = nCurrentComponent; n < (int)nStrings; n++)
            {
                if (!vFinal[n].length()
                    || (_idx.col[n] == VectorIndex::INVALID)
                    || (sTableName == "data" && _idx.col[n] >= _data.getCols("data")))
                    break;

                _data.setHeadLineElement(_idx.col[n], sTableName, removeQuotationMarks(maskControlCharacters(vFinal[n])));
            }

            nCurrentComponent = nStrings;
        }
        else if (_accessParser.isCluster())
        {
            // Write the return values to the cluster with
            // parsing them
            value_type* v = nullptr;
            int nResults = 0;
            int nthComponent = 0;
            NumeRe::Cluster& cluster = _data.getCluster(sTableName);

            // Clusters are overwritten, if the last index
            // is not explictly set
            if (_idx.row.back() == -2 && _idx.row.front() == 0)
                cluster.clear();

            for (size_t i = nCurrentComponent; i < vFinal.size(); i++)
            {
                // Set expression and evaluate it (not efficient but currently necessary)
                if (vFinal[i].front() == '"')
                {
                    // Special case: only one single value
                    if (_idx.row.size() == 1 && _idx.col.size() == 1)
                    {
                        cluster.setString(_idx.row.front(), vFinal[i]);
                        break;
                    }

                    if (_idx.row[nthComponent] == VectorIndex::INVALID)
                        break;

                    cluster.setString(_idx.row[nthComponent], vFinal[i]);
                    nthComponent++;
                }
                else
                {
                    _parser.SetExpr(vFinal[i]);
                    v = _parser.Eval(nResults);

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
            // Find the correct table
            if (sTableName == "data")
                throw SyntaxError(SyntaxError::READ_ONLY_DATA, sObject, SyntaxError::invalid_position);

            // Write the return values to the data table with
            // parsing them
            value_type* v = nullptr;
            int nResults = 0;
            int nthComponent = 0;

            for (size_t i = nCurrentComponent; i < vFinal.size(); i++)
            {
                // Set expression and evaluate it (not efficient but currently necessary)
                _parser.SetExpr(vFinal[i]);
                v = _parser.Eval(nResults);

                // Special case: only one single value
                if (_idx.row.size() == 1 && _idx.col.size() == 1)
                {
                    _data.writeToTable(_idx.row.front(), _idx.col.front(), sTableName, v[0]);

                    break;
                }

                // Write the single values
                for (int j = 0; j < nResults; j++)
                {
                    if (_idx.row.size() == 1 && _idx.col.size() > 1)
                    {
                        if (_idx.col[nthComponent] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row.front(), _idx.col[nthComponent], sTableName, v[j]);
                    }
                    else if (_idx.row.size() > 1 && _idx.col.size() == 1)
                    {
                        if (_idx.row[nthComponent] == VectorIndex::INVALID)
                            break;

                        _data.writeToTable(_idx.row[nthComponent], _idx.col.front(), sTableName, v[j]);
                    }

                    nthComponent++;
                }
            }

            nCurrentComponent = nStrings;
        }

        _data.setCacheStatus(false);
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
        Indices _idx = parser_getIndices(sObject, _parser, _data, _option);

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
    /// \param vFinal vector<string>&
    /// \param vIsNoStringValue const vector<bool>&
    /// \param __sObject string
    /// \return int
    ///
    /////////////////////////////////////////////////
    int StringParser::storeStringResults(vector<string>& vFinal, const vector<bool>& vIsNoStringValue, string __sObject)
    {
        // Only do something, if the target object is not empty
        if (!__sObject.length())
            return 1;

        // Handle remaining vector braces
        if (__sObject.find('{') != string::npos)
            parser_VectorToExpr(__sObject, _option);

        string sObject;
        size_t nStrings = vFinal.size();
        size_t nCurrentComponent = 0;

        // As long as the target object is not empty
        while (__sObject.length())
        {
            // Get-cut the next target
            sObject = getNextArgument(__sObject, true);

            // Examine the current target
            if (sObject.find("data(") != string::npos || _data.containsTablesOrClusters(sObject))
            {
                // Store the strings into the data object
                storeStringToDataObjects(vFinal, sObject, nCurrentComponent, nStrings);
            }
            else if (sObject.find("string(") != string::npos)
            {
                // Store the strings into the string object
                storeStringToStringObject(vFinal, sObject, nCurrentComponent, nStrings);
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
                        setStringValue(sObject, removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])));
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
                if (parser_GetVarAdress(sObject, _parser))
                {
                    if (vIsNoStringValue.size() > nCurrentComponent && !vIsNoStringValue[nCurrentComponent])
                    {
                        return 0;
                    }
                }

                // If this is a numerical value
                if (vIsNoStringValue.size() > nCurrentComponent && vIsNoStringValue[nCurrentComponent])
                {
                    try
                    {
                        // Parse and store it
                        int nResults = 0;
                        value_type* v = 0;
                        _parser.SetExpr(sObject + " = " + vFinal[nCurrentComponent]);
                        v = _parser.Eval(nResults);

                        // Replace the evaluated expression with its result,
                        // which will be shown in the terminal. The actual precision
                        // remains untouched!
                        if (nResults)
                        {
                            vAns = v[0];
                            string sValues;

                            // Transform the results into a string
                            for (int n = 0; n < nResults; n++)
                            {
                                sValues += toString(v[n], _option) + ",";
                            }
                            sValues.pop_back();

                            // replace the current expression
                            vFinal[nCurrentComponent] = sValues;
                        }

                        nCurrentComponent++;
                    }
                    catch (...)
                    {
                        throw;
                    }
                }
                else if (vIsNoStringValue.size() <= nCurrentComponent)
                {
                    // Fallback: try to find the variable address
                    // although it doesn't seem to be a numerical value
                    if (!parser_GetVarAdress(sObject, _parser))
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
                        setStringValue(sObject, removeQuotationMarks(maskControlCharacters(vFinal[nCurrentComponent])));
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
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringParser::createStringOutput(StringResult& StrRes, string& sLine, int parserFlags)
    {
        sLine.clear();

        vector<string>& vFinal = StrRes.vResult;
        vector<bool>& vIsNoStringValue = StrRes.vNoStringVal;

        // remove the quotation marks
        for (size_t i = 0; i < vFinal.size(); i++)
            vFinal[i] = removeQuotationMarks(vFinal[i]);

        string sConsoleOut = "|-> ";
        static const string sConsoleIndent = "|   ";
        bool bLineBreaks = false;

        // Every result in the current return values
        // is a single string result
        for (size_t j = 0; j < vFinal.size(); j++)
        {
            // In this case, no conversions are done
            // the results are simply put together and returned
            if (parserFlags & KEEP_MASKED_CONTROL_CHARS && parserFlags & KEEP_MASKED_QUOTES)
            {
                if (!(parserFlags & NO_QUOTES) && !vIsNoStringValue[j])
                    sLine += "\"" + vFinal[j] + "\"";
                else
                {
                    sLine += vFinal[j];
                }

                if (j < vFinal.size() - 1)
                    sLine += ",";
                continue;
            }

            // Start the current string value with a quotation mark
            // if it is not a special case
            if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !(parserFlags & NO_QUOTES) && !StrRes.bOnlyLogicals && !vIsNoStringValue[j])
            {
                sConsoleOut += "\"";
                sLine += "\"";
            }

            if (!vIsNoStringValue[j])
            {
                // Go through the current string value
                for (size_t k = 0; k < vFinal[j].length(); k++)
                {
                    // If there are escaped control characters,
                    // Replace them with their actual value here
                    if (k + 1 < vFinal[j].length()
                            && vFinal[j][k] == '\\'
                            && (vFinal[j][k + 1] == 'n' || vFinal[j][k + 1] == 't' || vFinal[j][k + 1] == '"' || vFinal[j][k + 1] == ' ')
                            && !(vFinal[j].substr(k + 1, 3) == "tau"
                                 && ((checkDelimiter(vFinal[j].substr(k, 5)) && vFinal[j].length() >= k + 5) || (vFinal[j].length() == k + 4)))
                            && !(vFinal[j].substr(k + 1, 5) == "theta"
                                 && ((checkDelimiter(vFinal[j].substr(k, 7)) && vFinal[j].length() >= k + 7) || (vFinal[j].length() == k + 6)))
                            && !(vFinal[j].substr(k + 1, 2) == "nu"
                                 && ((checkDelimiter(vFinal[j].substr(k, 4)) && vFinal[j].length() >= k + 4) || (vFinal[j].length() == k + 3)))
                            && !(vFinal[j].substr(k + 1, 3) == "neq"
                                 && ((checkDelimiter(vFinal[j].substr(k, 5)) && vFinal[j].length() >= k + 5) || (vFinal[j].length() == k + 4)))
                       )
                    {
                        //\not\neq\ni
                        if (vFinal[j][k + 1] == 'n') // Line break
                        {
                            sConsoleOut += "\n";
                            sLine += "\n";
                            bLineBreaks = true;
                        }
                        else if (vFinal[j][k + 1] == 't') // tabulator
                        {
                            sConsoleOut += "\t";
                            sLine += "\t";
                        }
                        else if (vFinal[j][k + 1] == '"') // quotation mark
                        {
                            sConsoleOut += "\"";
                            if (!(parserFlags & KEEP_MASKED_QUOTES))
                                sLine += "\"";
                            else
                                sLine += "\\\"";
                        }
                        else if (vFinal[j][k + 1] == ' ') // backslash itself
                        {
                            sConsoleOut += "\\";
                            if (!(parserFlags & KEEP_MASKED_CONTROL_CHARS))
                                sLine += "\\";
                            else
                                sLine += "\\ ";
                        }
                        k += 1;
                    }
                    else
                    {
                        // Otherwise simply append the current character
                        if (vFinal[j][k] == '\n')
                            bLineBreaks = true;
                        sConsoleOut += vFinal[j][k];
                        sLine += vFinal[j][k];
                    }
                }
            }
            else
            {
                _parser.SetExpr(vFinal[j]);
                int nResults = 0;
                value_type* v = _parser.Eval(nResults);
                string stres;

                for (int k = 0; k < nResults-1; k++)
                {
                    stres = toCmdString(v[k]);
                    sLine += stres + ", ";
                    sConsoleOut += stres + ", ";
                }

                stres = toCmdString(v[nResults-1]);
                sLine += stres;
                sConsoleOut += stres;
            }
            // End the current string value with a quotation mark
            // if it is not a special case
            if (vFinal[j] != "\\n" && vFinal[j] != "\\t" && !(parserFlags & NO_QUOTES) && !StrRes.bOnlyLogicals && !vIsNoStringValue[j])
            {
                sConsoleOut += "\"";
                sLine += "\"";
            }

            // Break the loop, if this was the last string value
            if (j + 1 == vFinal.size())
                break;

            // Add a comma, if neither the next nor the current string
            // is a special case
            if (vFinal[j] != "\\n" && vFinal[j + 1] != "\\n" && vFinal[j] != "\\t" && vFinal[j + 1] != "\\t")
            {
                // If the last character was a line break
                if (sLine.find_last_not_of("\" ") != string::npos && sLine[sLine.find_last_not_of("\" ")] == '\n')
                {
                    sLine += ", ";
                    if (sConsoleOut.back() == '"')
                    {
                        sConsoleOut[sConsoleOut.rfind('\n')] = '"';
                        sConsoleOut.back() = '\n';
                        bLineBreaks = true;
                    }
                    continue;
                }

                // Append the commas
                sConsoleOut += ", ";
                sLine += ", ";
            }
        }

        // Replace all line break characters with the console
        // indentation, if that is needed
        if (bLineBreaks)
            replaceAll(sConsoleOut, "\n", "\n" + sConsoleIndent);

        return sConsoleOut;
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
            vPositions.push_back(matchParams(sLine, iter->first));

            if (vPositions.back())
                parserFlags |= iter->second;
        }

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
    /// the equal sign at \c eq_pos is an assignment
    /// operator and no boolean expression.
    ///
    /// \param sLine const string&
    /// \param eq_pos size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringParser::isAssignmentOperator(const string& sLine, size_t eq_pos)
    {
        if (!eq_pos || eq_pos >= sLine.length())
            return false;
        return sLine[eq_pos - 1] != '!' && sLine[eq_pos - 1] != '<' && sLine[eq_pos - 1] != '>' && sLine[eq_pos + 1] != '=';
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
            if (!(nQuotes % 2) && !isspace(sLine[i]))
                return false;
        }

        return true;
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
        vector<bool> vIsNoStringValue;

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
            {
                vFinal[n] = concatenateStrings(vFinal[n]);
            }

            // Determine, whether the current component is a string
            // or a numerical expression
            if (vFinal[n].front() != '"' && vFinal[n].back() != '"')
            {
                // Try to evaluate the numerical expression, parse it as
                // string and store it correspondingly
                try
                {
    //                _parser.SetExpr(vFinal[n]);
    //                if (vFinal[n].find('{') == string::npos)
    //                    vFinal[n] = toCmdString(_parser.Eval());
                    vIsNoStringValue.push_back(true);
                }
                catch (...)
                {
                    // The parser was not able to parse the expression,
                    // we keep it as a string
                    vIsNoStringValue.push_back(false);
                }
            }
            else
                vIsNoStringValue.push_back(false);
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
    /// \brief This member function performs the
    /// actual string concatenation of the passed
    /// string expression.
    ///
    /// \param sExpr const string&
    /// \return string
    ///
    /// This member is called by
    /// StringParser::applyElementaryStringOperations()
    /// for the concatenation.
    /////////////////////////////////////////////////
    string StringParser::concatenateStrings(const string& sExpr)
    {
        string sConcatenated = sExpr;
        size_t nQuotes = 0;

        for (unsigned int i = 0; i < sConcatenated.length(); i++)
        {
            if (sConcatenated[i] == '"' && (!i || sConcatenated[i-1] != '\\'))
                nQuotes++;

            // Search for the concatenation operator (aka "+")
            if (!(nQuotes % 2) && sConcatenated[i] == '+')
            {
                string sLeft = sConcatenated.substr(0, i);
                string sRight = sConcatenated.substr(i+1);

                StripSpaces(sLeft);
                StripSpaces(sRight);

                // Determine the correct concatenation process
                if (sLeft == "\"\"" && sRight != "\"\"")
                {
                    sConcatenated = " " + sRight;
                    i = 0;
                }
                else if (sLeft != "\"\"" && sRight == "\"\"")
                {
                    sConcatenated = sLeft;
                    break;
                }
                else if (sLeft.back() == '"' && sRight.front() == '"')
                {
                    sConcatenated = sLeft.substr(0, sLeft.length()-1) + sRight.substr(1);

                    // We removed some characters
                    i = sLeft.length()-2;

                    // We're now part of a string
                    nQuotes++;
                }

                // Everything not catched here is a strange mixture
            }
        }

        StripSpaces(sConcatenated);

        return sConcatenated;
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

        string sObject;
        bool bObjectContainsTablesOrClusters = false;

        // If the current line is a simple string,
        // Strip the surrounding spaces and return directly
        // This saves between 30-70% of the evaluation time
        if (isSimpleString(sLine))
        {
            StripSpaces(sLine);

            strRes.vResult.push_back(sLine);
            strRes.vNoStringVal.push_back(false);

            return strRes;
        }

        // Identify target vectors and parse this as a list
        size_t eq_pos = sLine.find('=');
        size_t brace_pos = sLine.find('{');
        size_t string_pos = sLine.find("string(");

        if (brace_pos != string::npos
            && eq_pos != string::npos
            && brace_pos < eq_pos
            && isAssignmentOperator(sLine, eq_pos))
        {
            // find the actual equal sign in the current string expression
            while (isInQuotes(sLine, eq_pos) && sLine.find('=', eq_pos + 1) != string::npos)
                eq_pos = sLine.find('=', eq_pos + 1);

            // Did we find an actual equal sign?
            if (!isInQuotes(sLine, eq_pos) && isAssignmentOperator(sLine, eq_pos))
            {
                // Store the left side and erase it from the original string
                string sLeftSide = sLine.substr(0, eq_pos + 1);
                sLine.erase(0, eq_pos + 1);

                // Apply the string parser core to the right side of the assignment
                // Create a string vector variable from the result of the string parser
                StringResult _res = eval(sLine, "");
                string strvar = createStringVectorVar(_res.vResult);

                if (!strvar.length())
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Combine the left side and the string vector variable to
                // a new assignment
                sLine = sLeftSide + strvar;
            }
        }
        else if (brace_pos != string::npos
            && string_pos != string::npos
            && eq_pos != string::npos
            && string_pos < brace_pos
            && eq_pos > string_pos
            && eq_pos < brace_pos
            && isAssignmentOperator(sLine, eq_pos))
        {
            // Examine the left side of the assignment
            for (unsigned int i = 0; i < sLine.find('='); i++)
            {
                // Search for a character, which isn't a whitespace
                if (sLine[i] != ' ')
                {
                    // If the current character is the beginning of "string()"
                    if (sLine.substr(i, 7) == "string(")
                    {
                        // Jump over the argument parenthesis
                        i += getMatchingParenthesis(sLine.substr(i + 6)) + 6;

                        // Find the next non-whitespace character and check, whether it is an equal sign
                        if (sLine.find_first_not_of(' ', i) == sLine.find('='))
                        {
                            // Store the left side and erase it from the original string
                            string sLeftSide = sLine.substr(0, sLine.find('=') + 1);
                            sLine.erase(0, sLine.find('=') + 1);

                            // Apply the string parser core to the right side of the assignment
                            // Create a string vector variable from the result of the string parser
                            StringResult _res = eval(sLine, "");
                            string strvar = createStringVectorVar(_res.vResult);

                            if (!strvar.length())
                                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                            // Combine the left side and the string vector variable to
                            // a new assignment
                            sLine = sLeftSide + strvar;
                        }
                        break;
                    }
                    else
                    {
                        // Otherwise parse as a vector expression
                        parser_VectorToExpr(sLine, _option);
                        break;
                    }
                }
            }
        }

        eq_pos = sLine.find('=');

        // Recurse for multiple store targets
        // Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.
        if (sLine.find(',') != string::npos && eq_pos != string::npos && !isInQuotes(sLine, eq_pos))
        {
            // Get the left part of the assignment
            string sStringObject = sLine.substr(0, eq_pos);

            // Ensure that the left part contains a data or a string object,
            // otherwise it is cleared
            if (sStringObject.substr(sStringObject.find_first_not_of(' '), 7) == "string("
                    || sStringObject.substr(sStringObject.find_first_not_of(' '), 5) == "data("
                    || _data.containsTablesOrClusters(sStringObject))
            {
                // Find the equal sign after the closing parenthesis
                size_t nPos = getMatchingParenthesis(sLine);
                nPos = sLine.find('=', nPos);

                // Ensure that it exists
                if (nPos == string::npos)
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                // Get the data object including the assignment operator and
                // cut this part from the original line
                if (sLine[nPos + 1] == '=')
                    nPos++;
                sStringObject = sLine.substr(0, nPos + 1);
                sLine.erase(0, nPos + 1);
            }
            else
                sStringObject.clear();

            // Remove whitespaces
            StripSpaces(sLine);

            // If the current line contains more than one string expressions
            if (sLine != getNextArgument(sLine, false))
            {
                string sRecursion = "";
                vector<string> vResult;

                // While the current line is not empty
                while (sLine.length())
                {
                    // Cut of the first argument
                    sRecursion = getNextArgument(sLine, true);

                    // Apply the string parser on the current argument and store its
                    // results in a common vector
                    StringResult _res = eval(sRecursion, "");

                    for (size_t n = 0; n < _res.vResult.size(); n++)
                        vResult.push_back(_res.vResult[n]);
                }

                // Create a new string vector variable from the results
                sLine = createStringVectorVar(vResult);
            }

            // Prepend the stored string object to the current input line
            if (sStringObject.length())
                sLine = sStringObject + sLine;
        }

        // Find the start of the asignee
        size_t n_pos = 0;
        eq_pos = sLine.find('=');

        if (!sObject.length()
            && eq_pos != string::npos
            && !isInQuotes(sLine, eq_pos)
            && isAssignmentOperator(sLine, eq_pos))
            n_pos = eq_pos + 1;

        // Get the string variables
        if (containsStringVars(sLine.substr(n_pos)))
            getStringValues(sLine, n_pos);

        // Does the current line contain candidates
        // for string functions?
        if (sLine.find('(') != string::npos)
        {
            // Apply the standard string functions
            sLine = applyStringFuncs(sLine);

            // Apply the special string functions
            sLine = applySpecialStringFuncs(sLine);

            // Get the position of the equal sign after the
            // application of all string functions
            eq_pos = sLine.find('=');
        }

        // Extract target object
        if (!sObject.length()
            && eq_pos != string::npos
            && !isInQuotes(sLine, eq_pos)
            && isAssignmentOperator(sLine, eq_pos))
        {
            if (sLine.substr(0, eq_pos).find("data(") != string::npos || _data.containsTablesOrClusters(sLine.substr(0, eq_pos)))
            {
                sObject = sLine.substr(0, eq_pos);
                sLine.erase(0, eq_pos + 1);
            }
        }
        else if (!sObject.length() && sCache.length())
        {
            sObject = sCache;
        }

        if (_data.containsTablesOrClusters(sObject))
            bObjectContainsTablesOrClusters = true;

        // Get the contents of "string()", "data()" and the other caches
        sLine = getDataForString(sLine, n_pos);

        // If the line now doesn't contain any strings, return with the onlyLogicals flag set
        if (!isStringExpression(sLine) && !bObjectContainsTablesOrClusters)
        {
            // make sure that there are parseable characters between the operators
            if (sLine.find_first_not_of("+-*/:?!.,;%&<>=^ ") != string::npos && bParseNumericals)
            {
                int nResults = 0;
                value_type* v = 0;

                // Parse the epression
                _parser.SetExpr(sLine);
                v = _parser.Eval(nResults);
                vAns = v[0];
                eq_pos = sLine.find('=');

                // Remove the left part of the assignment, because it's already assigned
                if (eq_pos != string::npos
                    && eq_pos
                    && eq_pos < sLine.length() + 1
                    && isAssignmentOperator(sLine, eq_pos))
                    sLine.erase(0, eq_pos + 1);

                StripSpaces(sLine);

                // Return with the numbers already contained
                return StringResult(sLine, v, nResults);
            }

            // return with the onlyLogicals flag
            return StringResult(sLine, true);
        }

        n_pos = 0;

        // If there are any opening parentheses, it is possible that we need
        // to pre-evaluate its contents
        if (sLine.find('(') != string::npos)
        {
            size_t nQuotes = 0;

            // Examine the whole command line
            for (size_t i = 0; i < sLine.length(); i++)
            {
                // Jump over string literals
                if (sLine[i] == '"')
                {
                    if (i && sLine[i - 1] == '\\')
                        continue;
                    nQuotes++;
                }

                // Consider the var parsing feature
                if (sLine[i] == '#' && !(nQuotes % 2))
                {
                    // Examine, whether the var parsing feature was used
                    // correctly and search for the end of its current application
                    for (size_t j = i; j < sLine.length(); j++)
                    {
                        if (sLine[j] == '"')
                            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                        if ((sLine[j] == '(' || sLine[j] == '{') && getMatchingParenthesis(sLine.substr(j)) != string::npos)
                            j += getMatchingParenthesis(sLine.substr(j));

                        if (sLine[j] == ' ' || sLine[j] == '+')
                        {
                            i = j;
                            break;
                        }

                        if (j + 1 == sLine.length())
                        {
                            i = j;
                        }
                    }
                }

                // Examine the current parenthesis: is it able to pre-evaluate the
                // current parenthesis?
                if (sLine[i] == '(' && !(nQuotes % 2))
                {
                    // Ensure that its counterpart exists
                    if (getMatchingParenthesis(sLine.substr(i)) == string::npos)
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, i);

                    size_t nPos = getMatchingParenthesis(sLine.substr(i)) + i;

                    // Ignore any calls to "string()"
                    if (i < 6 || (i >= 6 && sLine.substr(i - 6, 6) != "string"))
                    {
                        // The contents of the parenthesis
                        string sString = sLine.substr(i + 1, nPos - i - 1);

                        // Pre-evaluate the contents of the parenthesis
                        if (i > 0 && !checkDelimiter(sLine.substr(i - 1, nPos - i + 2))) // this is probably a numerical function. Keep the parentheses
                        {
                            // Do only something, if the contents are containings strings
                            if (isStringExpression(sString))
                            {
                                StringResult _res = eval(sString, "");
                                string strvar = createStringVectorVar(_res.vResult);

                                if (!strvar.length())
                                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                                sLine = sLine.substr(0, i + 1) + strvar + sLine.substr(nPos);
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
                                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                                sLine = sLine.substr(0, i) + strvar + sLine.substr(nPos + 1);
                            }
                        }
                    }
                }
            }
        }

        // Are there any vector braces left?
        if (sLine.find('{') != string::npos)
        {
            n_pos = 0;
            eq_pos = sLine.find('=');

            if (eq_pos != string::npos
                && isAssignmentOperator(sLine, eq_pos)
                && !sObject.length()
                && !isInQuotes(sLine, eq_pos))
                n_pos = eq_pos + 1;

            size_t nQuotes = 0;

            // Examine each vector brace
            for (size_t i = n_pos; i < sLine.length(); i++)
            {
                if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
                    nQuotes++;

                if (!(nQuotes % 2) && sLine[i] == '{')
                {
                    size_t nmatching = getMatchingParenthesis(sLine.substr(i));
                    string sVectorTemp = sLine.substr(i+1, nmatching-1);

                    // Does the vector brace contain colons? Then it
                    // might be a numerical vector expansion
                    if (sVectorTemp.find(':') != string::npos)
                    {
                        // Is it NOT a string expression?
                        if (!isStringExpression(sVectorTemp))
                        {
                            // Try to evaluate the vector as
                            // vector expansion
                            _parser.SetExpr("{" + sVectorTemp + "}");
                            int nRes = 0;
                            double* res = _parser.Eval(nRes);

                            // Store the result in a vector and
                            // create a temporary vector variable
                            vector<double> vRes(res, res + nRes);
                            sLine.replace(i, nmatching+1, _parser.CreateTempVectorVar(vRes));
                        }

                        continue;
                    }

                    StringResult tempres = eval(sVectorTemp, "");

                    if (!tempres.vResult.size())
                        throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

                    // Create a string vector variable from the returned vector
                    // if it is a string. Otherwise we create a new temporary
                    // vector from the parser using the numerical results
                    if (!tempres.bOnlyLogicals)
                        sVectorTemp = createStringVectorVar(tempres.vResult);
                    else
                        sVectorTemp = _parser.CreateTempVectorVar(tempres.vNumericalValues);

                    sLine.replace(i, nmatching+1, sVectorTemp);
                }
            }
        }

        // Strip all whitespaces and ensure, that there's something left
        StripSpaces(sLine);

        if (!sLine.length())
            return StringResult("");

        // If the current line doesn't contain any further string literals or return values
        // return it
        if (sLine.find('"') == string::npos && sLine.find('#') == string::npos && !containsStringVectorVars(sLine) && !bObjectContainsTablesOrClusters)
        {
            // Ensure that this is no false positive
            if (sLine.find("string(") != string::npos || containsStringVars(sLine))
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

            // return the current line
            return StringResult(sLine);
        }

        // Extract the object, if not already done
        eq_pos = sLine.find('=');

        if (!sObject.length()
            && eq_pos != string::npos
            && !isInQuotes(sLine, eq_pos)
            && isAssignmentOperator(sLine, eq_pos))
        {
            sObject = sLine.substr(0, eq_pos);
            sLine = sLine.substr(eq_pos + 1);
        }

        // Apply the "#" parser to the string
        sLine = numToString(sLine);

        // Split the list to a vector
        strRes.vResult = evaluateStringVectors(sLine);

        // Ensure that there is at least one result
        if (!strRes.vResult.size())
            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

        // Apply some elementary operations such as concatenation and logical operations
        strRes.vNoStringVal = applyElementaryStringOperations(strRes.vResult, strRes.bOnlyLogicals);

        // store the string results in the variables or inb "string()" respectively
        if (!storeStringResults(strRes.vResult, strRes.vNoStringVal, sObject))
            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);

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

        // The result of the string parser core has to be parsed, so that
        // it is readable in the terminal. This is done here in this
        // function
        string sConsoleOut = createStringOutput(StrRes, sLine, parserFlags);

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
        if (sExpression.find('"') != string::npos
			|| sExpression.find('#') != string::npos
			|| sExpression.find("string(") != string::npos
			|| sExpression.find("string_cast(") != string::npos
			|| sExpression.find("char(") != string::npos
			|| sExpression.find("valtostr(") != string::npos)
            return true;

		if (containsStringVars(sExpression) || containsStringVectorVars(sExpression))
            return true;

        const map<string,NumeRe::Cluster>& mClusterMap = _data.getClusterMap();

        for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
        {
            if (iter->second.isString())
            {
                size_t pos = sExpression.find(iter->first + "{");

                if (pos != string::npos && (!pos || isDelimiter(sExpression[pos-1])))
                    return true;
            }
        }

        return false;
    }
}

