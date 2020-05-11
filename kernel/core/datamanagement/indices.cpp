/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include "dataaccess.hpp"
#include "../utils/tools.hpp"
#include <vector>

using namespace std;

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, Datafile& _data, string& sArgument, const string& sCmd);
static void extractIndexList(string& sArgument, vector<string>& vLines, vector<string>& vCols);
static void handleIndexVectors(Parser& _parser, Indices& _idx, vector<string>& vLines, vector<string>& vCols);
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<string>& vLines, vector<string>& vCols, const string& sCmd);
static void expandIndexVectors(Indices& _idx, Datafile& _data, const string& sCmd);
static void expandStringIndexVectors(Indices& _idx, Datafile& _data);
static bool isCandidateForVectors(vector<string>& vLines, vector<string>& vCols);
static bool isCandidateForCasuals(vector<string>& vLines, vector<string>& vCols);
/*
 * --> Gibt DATENELEMENT-Indices als Ints in einem Indices-Struct zurueck <--
 * --> Index = -1, falls der Index nicht gefunden wurde/kein DATENELEMENT uebergeben wurde <--
 * --> Index = -2, falls der Index den gesamten Bereich erlaubt <--
 * --> Index = -3, falls der Index eine Stringreferenz ist <--
 * --> Gibt alle angegeben Indices-1 zurueck <--
 */
Indices getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _idx;

    string sArgument = "";
    string sTableName = "";
    unsigned int nPos = 0;

    // Check, whether indices are available
    if (sCmd.find_first_of("({") == string::npos)
        return _idx;

    // Find parenthesis position and get the matching closing parenthesis
    nPos = sCmd.find_first_of("({");
    size_t nClosingParens = getMatchingParenthesis(sCmd.substr(sCmd.find_first_of("({")));

    // Return, if the closing parenthesis is missing
    if (nClosingParens == string::npos)
        return _idx;

    sTableName = sCmd.substr(0, sCmd.find_first_of("({"));

    // Remove leading whitespaces and operators
    if (sTableName.find_first_of(" +-*/!=&|<>^?:%") != string::npos)
        sTableName.erase(0, sTableName.find_last_of(" +-*/!=&|<>^?:%")+1);

    StripSpaces(sTableName);

    sArgument = sCmd.substr(nPos + 1, nClosingParens - 1);

    // Remove not necessary white spaces
    StripSpaces(sArgument);

    // This is the handler for the abbreviation TABLE() == TABLE(:,:)
    if (!sArgument.length())
    {
        _idx.row = VectorIndex(0LL, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0LL, VectorIndex::OPEN_END);
        return _idx;
    }

    // If the argument contains tables, get their values. This leads to a recursion!
    if (sArgument.find("data(") != string::npos || _data.containsTablesOrClusters(sArgument))
        getDataElements(sArgument, _parser, _data, _option);

    // update the dimension variables
    if (sCmd[nPos] == '(')
        _data.updateDimensionVariables(sTableName);
    else
        _data.updateClusterSizeVariables(sTableName);

    // Store the obtained argument (this will contain the results from the recursion)
    _idx.sCompiledAccessEquation = sArgument;

    // If the argument contains a comma, handle it as a usual index list
    handleArgumentForIndices(_idx, _parser, _data, sArgument, sCmd);

    return _idx;
}


/*
 *   LOCAL FUNCTIONS FOR parser_getIndices()
 */

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, Datafile& _data, string& sArgument, const string& sCmd)
{
    vector<string> vLines {"<<NONE>>", "<<NONE>>"};
    vector<string> vCols {"<<NONE>>", "<<NONE>>"};

    // extract the (textual) indices from the argument list and store it in sI and sJ
    extractIndexList(sArgument, vLines, vCols);

    // Ensure that the access equation contains at least one vector
    if (isCandidateForVectors(vLines, vCols))
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx, vLines, vCols);
    }

    // Ensure that the indices are casuals and no indices
    if (isCandidateForCasuals(vLines, vCols))
    {
        // Handle the casual indices
        handleCasualIndices(_parser, _idx, vLines, vCols, sCmd);
    }

    if (_idx.row.numberOfNodes() > 2 || _idx.col.numberOfNodes() > 2)
    {
        // Expand the casual indices to vectors if needed
        expandIndexVectors(_idx, _data, sCmd);
    }
}

// separates the argument into its up to four parts and returns the position after the last operator
static void extractIndexList(string& sArgument, vector<string>& vLines, vector<string>& vCols)
{
    string sLines;
    string sCols = sArgument + " ";
    size_t nElement = 0;

    // Split line and column indices at
    // the comma (if it is available). Otherwise
    // only the line indices are available
    sLines = getNextArgument(sCols, true) + " ";

    // Split the line indices
    vLines[0] = getNextIndex(sLines);

    if (sLines.length())
        vLines[1] = sLines;

    // If the column indices are available,
    // split them also. Otherwise use the
    // <<EMPTY>> tag. It's possible that the
    // column indices are more than two.
    if (sCols.length())
    {
        vCols[0] = getNextIndex(sCols);

        // As long as there's a column index
        // available, separate it here
        while (sCols.length())
        {
            nElement++;

            if (vCols.size() <= nElement)
                vCols.push_back("");

            vCols[nElement] = getNextIndex(sCols);
        }
    }
    else
        vCols[0] = "<<EMPTY>>";

    // Ensure that the indices are not only whitespaces
    for (size_t i = 0; i < 2; i++)
    {
        if (vLines[i].find_first_not_of(' ') == string::npos)
            vLines[i] = "<<EMPTY>>";

        if (vCols[i].find_first_not_of(' ') == string::npos)
            vCols[i] = "<<EMPTY>>";
    }
}

// This function will evaluate the indices and it tries to match it to a vector
static void handleIndexVectors(Parser& _parser, Indices& _idx, vector<string>& vLines, vector<string>& vCols)
{
    value_type* v;
    int nResults;

    // Evaluate the line indices
    if (vLines[0] != "<<NONE>>" && vLines[1] == "<<NONE>>")
    {
        if (vLines[0] == "#")
            _idx.row.front() = VectorIndex::STRING;
        else if (vLines[0] == "<<EMPTY>>")
            _idx.row.front() = 0;
        else
        {
            _parser.SetExpr(vLines[0]);
            v = _parser.Eval(nResults);

            if (nResults > 1)
            {
                // vector
                _idx.row = VectorIndex(v, nResults, 0);
            }
            else // single index
                _idx.row.front() = intCast(v[0]) - 1;
        }
    }

    // Evalute the column indices
    if (vCols[0] != "<<NONE>>" && vCols[1] == "<<NONE>>")
    {
        if (vCols[0] == "#")
            _idx.col.front() = VectorIndex::STRING;
        else if (vCols[0] == "<<EMPTY>>")
            _idx.col.front() = 0;
        else
        {
            _parser.SetExpr(vCols[0]);
            v = _parser.Eval(nResults);

            if (nResults > 1)
            {
                // vector
                _idx.col = VectorIndex(v, nResults, 0);
            }
            else // single index
                _idx.col.front() = intCast(v[0]) - 1;
        }
    }
}

// This function will evaluate all indices at once and store them into the Indices object
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<string>& vLines, vector<string>& vCols, const string& sCmd)
{
    value_type* v = 0;
    int nResults = 0;
    string sIndexExpressions;
    vector<int> vIndexNumbers;

    // Go through all indices and connect them to one single equation
    // store the assignment of the indices
    if (!_idx.row.isValid())
    {
        for (size_t n = 0; n < vLines.size(); n++)
        {
            if (vLines[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.row.setIndex(n, VectorIndex::OPEN_END); //special one: last possible index
                else
                    _idx.row.front() = 0;
            }
            else if (vLines[n] != "<<NONE>>")
            {
                if (_idx.row.front() != VectorIndex::STRING)
                {
                    // connect the indices
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";

                    sIndexExpressions += vLines[n];
                    // Store the assignment (lines are positive)
                    vIndexNumbers.push_back((n + 1));
                }
            }
        }
    }

    // Go through the column indices separately,
    // because they might be more than two
    if (!_idx.col.isValid())
    {
        for (size_t n = 0; n < vCols.size(); n++)
        {
            if (vCols[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.col.setIndex(n, VectorIndex::OPEN_END); // special one: last possible index
                else
                    _idx.col.front() = 0;
            }
            else if (vCols[n] != "<<NONE>>")
            {
                if (_idx.col.front() != VectorIndex::STRING)
                {
                    // connect the indices
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";

                    sIndexExpressions += vCols[n];
                    // store the assignments (columns are negative)
                    vIndexNumbers.push_back(-(n + 1));
                }
            }
        }
    }

    // If the index expression list has a length, evaluate it
    if (sIndexExpressions.length())
    {
        _parser.SetExpr(sIndexExpressions);
        v = _parser.Eval(nResults);

        // check whether the number of the results is matching
        if ((size_t)nResults != vIndexNumbers.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        // map the results to their assignments
        for (int i = 0; i < nResults; i++)
        {
            if (isinf(v[i])) // infinity => last possible index
                v[i] = -1; // only -1 because it will be decremented in the following lines
            else if (isnan(v[i]) || intCast(v[i]) <= 0)
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

            if (vIndexNumbers[i] > 0)
                _idx.row.setIndex(vIndexNumbers[i] - 1, intCast(v[i]) - 1);
            else
                _idx.col.setIndex(abs(vIndexNumbers[i]) - 1, intCast(v[i]) - 1);
        }
    }
}

// This function will expand casual indices into vectors
static void expandIndexVectors(Indices& _idx, Datafile& _data, const string& sCmd)
{
    // Get the cache name from the command string
    // should now only contain the name of the table
    string sCache = sCmd.substr(0, sCmd.find_first_of("({"));
    bool isCluster = sCmd[sCmd.find_first_of("({")] == '{';

    // remove leading whitespaces
    if (sCache.find(' ') != string::npos)
        sCache.erase(0, sCache.rfind(' ') + 1);

    // check, whether it exists
    if (!sCache.length())
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);

    // Ensure that the indices are valid
    if (!_idx.row.isValid() || !_idx.col.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

    // Is it the "string" object?
    if (sCache == "string")
    {
        expandStringIndexVectors(_idx, _data);
        return;
    }

    // If the cache is not really a cache
    if (sCache.find("data") == string::npos && !isCluster && !_data.isTable(sCache))
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
}

// This static function expands the indices into vectors, if the
// the current object is the string object
static void expandStringIndexVectors(Indices& _idx, Datafile& _data)
{
    if (_idx.row.isOpenEnd())
    {
        _idx.row.setRange(0, _data.getStringElements()-1);
    }

    if (_idx.col.isOpenEnd())
    {
        _idx.col.setRange(0, _data.getStringCols()-1);
    }
}

// This function will ensure that at least one of the indices contains a vector
static bool isCandidateForVectors(vector<string>& vLines, vector<string>& vCols)
{
    return (vLines[1] == "<<NONE>>" || vCols[1] == "<<NONE>>");
}

// This function will ensure that at least one index is a casual one
static bool isCandidateForCasuals(vector<string>& vLines, vector<string>& vCols)
{
    return (vLines[1] != "<<NONE>>" || vCols[1] != "<<NONE>>");
}

