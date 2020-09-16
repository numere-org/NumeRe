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

// static const variable instances for
// special index variables
static const char* IDXNONE = "<<N>>";
static const char* IDXEMPTY = "<<E>>";

using namespace std;
using namespace mu;

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data, const string& sArgument, const string& sCmd);
static void extractIndexList(StringView sArgument, vector<StringView>& vLines, vector<StringView>& vCols);
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex);
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<StringView>& vLines, vector<StringView>& vCols, const string& sCmd);
static void handleSingleCasualIndex(VectorIndex& _vIdx, vector<StringView>& vIndex, string& sIndexExpressions, vector<int>& vIndexNumbers, int sign);
static void expandIndexVectors(Indices& _idx, MemoryManager& _data, const string& sCmd);
static void expandStringIndexVectors(Indices& _idx, MemoryManager& _data);
static bool isCandidateForCasuals(vector<StringView>& vLines, vector<StringView>& vCols);
/*
 * --> Gibt DATENELEMENT-Indices als Ints in einem Indices-Struct zurueck <--
 * --> Index = -1, falls der Index nicht gefunden wurde/kein DATENELEMENT uebergeben wurde <--
 * --> Index = -2, falls der Index den gesamten Bereich erlaubt <--
 * --> Index = -3, falls der Index eine Stringreferenz ist <--
 * --> Gibt alle angegeben Indices-1 zurueck <--
 */
Indices getIndices(const string& sCmd, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    Indices _idx;
    string sTableName = "";
    size_t nPos = 0;

    // Check, whether indices are available
    if ((nPos = sCmd.find_first_of("({")) == string::npos)
        return _idx;

    // Find parenthesis position and get the matching closing parenthesis
    size_t nClosingParens = getMatchingParenthesis(sCmd.substr(nPos));

    // Return, if the closing parenthesis is missing
    if (nClosingParens == string::npos)
        return _idx;

    sTableName.assign(sCmd.begin(), sCmd.begin()+nPos);

    // Remove leading whitespaces and operators
    for (size_t i = 0; i < sTableName.length(); i++)
    {
        // First character of a table is either
        // and underscore or an alphabetic character.
        // Digits are not allowed
        if (sTableName[i] == '_' || isalpha(sTableName[i]))
        {
            if (i)
                sTableName.erase(0, i);

            break;
        }
    }

    // TODO: for loop might be wrong although more efficient
    //if (sTableName.find_first_of(" +-*/!=&|<>^?:%") != string::npos)
    //    sTableName.erase(0, sTableName.find_last_of(" +-*/!=&|<>^?:%")+1);

    _idx.sCompiledAccessEquation.assign(sCmd.begin()+nPos+1, sCmd.begin()+nPos+nClosingParens);// = sCmd.substr(nPos + 1, nClosingParens - 1);

    // Remove not necessary white spaces
    StripSpaces(_idx.sCompiledAccessEquation);

    // This is the handler for the abbreviation TABLE() == TABLE(:,:)
    if (!_idx.sCompiledAccessEquation.length())
    {
        _idx.row = VectorIndex(0LL, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0LL, VectorIndex::OPEN_END);
        return _idx;
    }

    // If the argument contains tables, get their values. This leads to a recursion!
    if (_data.containsTablesOrClusters(_idx.sCompiledAccessEquation))
        getDataElements(_idx.sCompiledAccessEquation, _parser, _data, _option);

    // update the dimension variables
    if (sCmd[nPos] == '(')
        _data.updateDimensionVariables(sTableName);
    else
        _data.updateClusterSizeVariables(sTableName);

       //_idx.row.setIndex(0,0);
       //_idx.col.setIndex(0,0);
       //return _idx;

    // If the argument contains a comma, handle it as a usual index list
    handleArgumentForIndices(_idx, _parser, _data, _idx.sCompiledAccessEquation, sCmd);

    return _idx;
}


/*
 *   LOCAL FUNCTIONS FOR parser_getIndices()
 */

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data, const string& sArgument, const string& sCmd)
{
    vector<StringView> vLines;
    vector<StringView> vCols;

    // extract the (textual) indices from the argument list and store it in sI and sJ
    extractIndexList(sArgument, vLines, vCols);

    // Detect, whether the line indices are candidates
    // for vectors
    if (vLines.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.row, vLines.front());
    }
        _idx.row.setIndex(0,0);
        _idx.col.setIndex(0,0);
        return;

    // Detect, whether the column indices are candidates
    // for vectors
    if (vCols.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.col, vCols.front());
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
static void extractIndexList(StringView sCols, vector<StringView>& vLines, vector<StringView>& vCols)
{
    // Split line and column indices at
    // the comma (if it is available). Otherwise
    // only the line indices are available
    StringView sLines = getNextViewedArgument(sCols);

    bool openEnd = sLines.back() == ':';

    // Split the line indices
    vLines.push_back(std::move(getNextViewedIndex(sLines)));

    sLines.strip();

    if (sLines.length())
        vLines.push_back(sLines);
    else if (openEnd)
        vLines.push_back(StringView());

    // If the column indices are available,
    // split them also. Otherwise use the
    // <<EMPTY>> tag. It's possible that the
    // column indices are more than two.
    if (sCols.length())
    {
        openEnd = sCols.back() == ':';

        vCols.push_back(std::move(getNextViewedIndex(sCols)));

        // As long as there's a column index
        // available, separate it here
        while (sCols.length())
        {
            vCols.push_back(getNextViewedIndex(sCols));
        }

        if (openEnd)
            vCols.push_back(StringView());
    }
    else
        vCols.push_back(StringView());

    // Ensure that the indices are not only whitespaces
    /*for (size_t i = 0; i < vLines.size(); i++)
    {
        if (!vLines[i].length())
            vLines[i] = IDXEMPTY;
    }

    for (size_t i = 0; i < vCols.size(); i++)
    {
        if (!vCols[i].length())
            vCols[i] = IDXEMPTY;
    }*/
}

// This function will evaluate the indices and it tries to match it to a vector
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex)
{
    value_type* v;
    int nResults;

    if (!sIndex.length())
        _vIdx.front() = 0;
    else if (sIndex == "#")
        _vIdx.front() = VectorIndex::STRING;
    else
    {
        _parser.SetExpr(sIndex);
        v = _parser.Eval(nResults);

        if (nResults > 1)
        {
            // vector
            _vIdx = VectorIndex(v, nResults, 0);
        }
        else // single index
            _vIdx.front() = intCast(v[0]) - 1;
    }
}

static void handleSingleCasualIndex(VectorIndex& _vIdx, vector<StringView>& vIndex, string& sIndexExpressions, vector<int>& vIndexNumbers, int sign)
{
    for (size_t n = 0; n < vIndex.size(); n++)
    {
        if (!vIndex[n].length())
        {
            if (n)
                _vIdx.setIndex(n, VectorIndex::OPEN_END); //special one: last possible index
            else
                _vIdx.front() = 0;
        }
        else
        {
            if (_vIdx.front() != VectorIndex::STRING)
            {
                // connect the indices
                if (sIndexExpressions.length())
                    sIndexExpressions += ",";

                sIndexExpressions += vIndex[n].to_string();
                // Store the assignment (lines are positive)
                vIndexNumbers.push_back(sign*(n + 1));
            }
        }
    }
}

// This function will evaluate all indices at once and store them into the Indices object
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<StringView>& vLines, vector<StringView>& vCols, const string& sCmd)
{
    value_type* v = 0;
    int nResults = 0;
    string sIndexExpressions;
    vector<int> vIndexNumbers;

    // Go through all indices and connect them to one single equation
    // store the assignment of the indices
    if (!_idx.row.isValid())
        handleSingleCasualIndex(_idx.row, vLines, sIndexExpressions, vIndexNumbers, 1);

    // Go through the column indices separately,
    // because they might be more than two
    if (!_idx.col.isValid())
        handleSingleCasualIndex(_idx.col, vCols, sIndexExpressions, vIndexNumbers, -1);

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
static void expandIndexVectors(Indices& _idx, MemoryManager& _data, const string& sCmd)
{
    // Get the cache name from the command string
    // should now only contain the name of the table
    string sCache = sCmd.substr(0, sCmd.find_first_of("({"));
    bool isCluster = sCmd.back() == '{';

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
    if (!isCluster && !_data.isTable(sCache))
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
}

// This static function expands the indices into vectors, if the
// the current object is the string object
static void expandStringIndexVectors(Indices& _idx, MemoryManager& _data)
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

// This function will ensure that at least one index is a casual one
static bool isCandidateForCasuals(vector<StringView>& vLines, vector<StringView>& vCols)
{
    return (vLines.size() > 1 || vCols.size() > 1);
}

