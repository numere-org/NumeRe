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
#include "../../kernel.hpp"
#include <vector>

using namespace std;
using namespace mu;

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data, StringView sArgument, StringView sCmd);
static void extractIndexList(StringView sCols, vector<StringView>& vLines, vector<StringView>& vCols);
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex);
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<StringView>& vLines, vector<StringView>& vCols, StringView sCmd);
static void handleSingleCasualIndex(VectorIndex& _vIdx, vector<StringView>& vIndex, string& sIndexExpressions, vector<int>& vIndexNumbers, int sign);
static void expandIndexVectors(Indices& _idx, MemoryManager& _data, StringView sCmd);
static void expandStringIndexVectors(Indices& _idx, MemoryManager& _data);


/////////////////////////////////////////////////
/// \brief Wrapper for the new getIndices
/// function interface.
///
/// \param sCmd StringView
/// \param _parser Parser&
/// \param _data MemoryManager&
/// \param _option const Settings&
/// \return Indices
///
/// \deprecated Marked as deprecated.
///
/////////////////////////////////////////////////
Indices getIndices(StringView sCmd, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    Indices _idx;
    getIndices(sCmd, _idx, _parser, _data, _option);
    return _idx;
}


/////////////////////////////////////////////////
/// \brief The new interface function to
/// determine the used indices in the current
/// expression part.
///
/// \param sCmd StringView
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data MemoryManager&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
void getIndices(StringView sCmd, Indices& _idx,  Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    StringView sTableName;
    StringView sIndices;
    size_t nPos = 0;

    // Check, whether indices are available
    if ((nPos = sCmd.find_first_of("({")) == string::npos)
        return;

    // Find parenthesis position and get the matching closing parenthesis
    size_t nClosingParens = getMatchingParenthesis(sCmd.subview(nPos));

    // Return, if the closing parenthesis is missing
    if (nClosingParens == string::npos)
        return;

    sTableName = sCmd.subview(0, nPos);

    // Remove leading whitespaces and operators
    for (size_t i = 0; i < sTableName.length(); i++)
    {
        // First character of a table is either
        // and underscore or an alphabetic character.
        // Digits are not allowed
        if (sTableName[i] == '_' || isalpha(sTableName[i]))
        {
            if (i)
                sTableName.trim_front(i);

            break;
        }
    }

    sIndices = sCmd.subview(nPos+1, nClosingParens-1);

    // Remove not necessary white spaces
    sIndices.strip();

    // This is the handler for the abbreviation TABLE() == TABLE(:,:)
    if (!sIndices.length())
    {
        _idx.row = VectorIndex(0LL, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0LL, VectorIndex::OPEN_END);
        return;
    }
    else
        _idx.sCompiledAccessEquation.assign(sIndices.begin(), sIndices.end());

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
        //return;

#warning TODO (numere#3#08/15/21): Checking for string variables here is inefficient
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_idx.sCompiledAccessEquation))
    {
        EndlessVector<std::string> idxDims = getAllArguments(_idx.sCompiledAccessEquation);
        g_logger.debug("_idx.sCompiledAccessEquation contains string vector vars " + _idx.sCompiledAccessEquation);
        _idx.sCompiledAccessEquation.clear();

        // Go through the dimensions
        for (std::string dim : idxDims)
        {
            // Only do s.th. if the index contains more than the colon
            if (dim.find_first_not_of(" :") != std::string::npos)
            {
                // Get all indices (the additional whitespace is needed to
                // correctly detect indices like 'IDX:')
                EndlessVector<std::string> idx = getAllIndices(dim + " ");
                dim.clear();

                // Go through every single index
                for (size_t i = 0; i < idx.size(); i++)
                {
                    if (idx[i] != "#" && NumeReKernel::getInstance()->getStringParser().isStringExpression(idx[i]))
                    {
                        std::string sDummy;
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(idx[i], sDummy, true);

                        if (idx[i].find(',') != std::string::npos)
                            idx[i] = "{" + idx[i] + "}";
                    }

                    dim += idx[i];

                    if (i+1 < idx.size())
                        dim += ":";
                }
            }

            // Combine everything together
            if (_idx.sCompiledAccessEquation.length())
                _idx.sCompiledAccessEquation += ",";

            _idx.sCompiledAccessEquation += dim;
        }

    }

    // If the argument contains a comma, handle it as a usual index list
    handleArgumentForIndices(_idx, _parser, _data, _idx.sCompiledAccessEquation, sCmd);
}


/*
 *   LOCAL FUNCTIONS FOR parser_getIndices()
 */

/////////////////////////////////////////////////
/// \brief This static function is the main
/// driver function for extracting the indices
/// into VectorIndex representations.
///
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data MemoryManager&
/// \param sArgument StringView
/// \param sCmd StringView
/// \return void
///
/////////////////////////////////////////////////
static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data, StringView sArgument, StringView sCmd)
{
    vector<StringView> vLines;
    vector<StringView> vCols;

    // extract the (textual) indices from the argument list and store it in sI and sJ
    extractIndexList(sArgument, vLines, vCols);
        //_idx.row.setIndex(0,0);
        //_idx.col.setIndex(0,0);
        //return;

    // Detect, whether the line indices are candidates
    // for vectors
    if (vLines.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.row, vLines.front());
    }

    // Detect, whether the column indices are candidates
    // for vectors
    if (vCols.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.col, vCols.front());
    }

    // Ensure that the indices are casuals and no indices
    if (vLines.size() > 1 || vCols.size() > 1)
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


/////////////////////////////////////////////////
/// \brief This static function separates the
/// argument into its row and column parts and
/// returns them as vectors of StringView
/// instances.
///
/// \param sCols StringView
/// \param vLines vector<StringView>&
/// \param vCols vector<StringView>&
/// \return void
///
/////////////////////////////////////////////////
static void extractIndexList(StringView sCols, vector<StringView>& vLines, vector<StringView>& vCols)
{
    // Split line and column indices at
    // the comma (if it is available). Otherwise
    // only the line indices are available
    StringView sLines(getNextViewedArgument(sCols));

    bool openEnd = sLines.back() == ':';

    // Split the line indices
    vLines.push_back(getNextViewedIndex(sLines));

    sLines.strip();

    if (sLines.length())
        vLines.push_back(sLines);
    else if (openEnd)
        vLines.push_back(StringView());

    // If the column indices are available,
    // split them also. Otherwise use an
    // empty StringView instance. It's possible that the
    // column indices are more than two.
    if (sCols.length())
    {
        openEnd = sCols.back() == ':';

        vCols.push_back(getNextViewedIndex(sCols));

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
}


/////////////////////////////////////////////////
/// \brief This static function will evaluate the
/// indices and it tries to match it to a vector.
///
/// \param _parser Parser&
/// \param _vIdx VectorIndex&
/// \param sIndex StringView
/// \return void
///
/////////////////////////////////////////////////
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex)
{
    mu::value_type* v;
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
        else if (!isnan(v[0].real()) && intCast(v[0]) > 0) // single index
            _vIdx.front() = intCast(v[0]) - 1;
    }
}


/////////////////////////////////////////////////
/// \brief This static function is a helper
/// function for handleCasualIndices(), which
/// will be applied to row and column indices
/// separately.
///
/// \param _vIdx VectorIndex&
/// \param vIndex vector<StringView>&
/// \param sIndexExpressions string&
/// \param vIndexNumbers vector<int>&
/// \param sign int
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This static function is a simple
/// helper to convert the numerical results into
/// a string, representing a parsed index
/// expression. This function is used in case of
/// an exception.
///
/// \param v value_type*
/// \param vIndexNumbers const vector<int>
/// \return std::string
///
/////////////////////////////////////////////////
static std::string convertToString(value_type* v, const vector<int> vIndexNumbers)
{
    std::string sIndexExpression;

    for (size_t i = 0; i < vIndexNumbers.size(); i++)
    {
        if (i)
        {
            if (vIndexNumbers[i-1] > 0 && vIndexNumbers[i] < 0)
                sIndexExpression += ", ";
            else
                sIndexExpression += ':';
        }

        sIndexExpression += toString(v[i], 5);
    }

    return sIndexExpression;
}


/////////////////////////////////////////////////
/// \brief This function will evaluate all
/// indices, which are interpreted as casual
/// indices, at once and store them into the
/// Indices object.
///
/// \param _parser Parser&
/// \param _idx Indices&
/// \param vLines vector<StringView>&
/// \param vCols vector<StringView>&
/// \param sCmd StringView
/// \return void
///
/////////////////////////////////////////////////
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<StringView>& vLines, vector<StringView>& vCols, StringView sCmd)
{
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
		int nResults;
        mu::value_type* v = _parser.Eval(nResults);

        // check whether the number of the results is matching
        if ((size_t)nResults != vIndexNumbers.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), SyntaxError::invalid_position, sIndexExpressions);

        // map the results to their assignments
        for (int i = 0; i < nResults; i++)
        {
            if (isinf(v[i].real())) // infinity => last possible index
                v[i] = -1; // only -1 because it will be decremented in the following lines
            else if (isnan(v[i].real()) || intCast(v[i]) <= 0LL)
            {
                std::string sToken;

                if (vIndexNumbers.front() > 0 && vIndexNumbers.back() < 0)
                    sToken = convertToString(v, vIndexNumbers);
                else if (vIndexNumbers.front() > 0)
                {
                    sToken = convertToString(v, vIndexNumbers);
                    sToken += ", " + _idx.col.to_string();
                }
                else
                {
                    sToken = _idx.row.to_string() + ", ";
                    sToken += convertToString(v, vIndexNumbers);
                }

                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), SyntaxError::invalid_position, sToken);
            }

            if (vIndexNumbers[i] > 0)
                _idx.row.setIndex(vIndexNumbers[i] - 1, intCast(v[i]) - 1);
            else
                _idx.col.setIndex(abs(vIndexNumbers[i]) - 1, intCast(v[i]) - 1);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function will expand casual
/// indices into vectors. Actually, it is only
/// used for the special case of a string vector.
///
/// \param _idx Indices&
/// \param _data MemoryManager&
/// \param sCmd StringView
/// \return void
///
/////////////////////////////////////////////////
static void expandIndexVectors(Indices& _idx, MemoryManager& _data, StringView sCmd)
{
    // Get the cache name from the command string
    // should now only contain the name of the table
    StringView sCache = sCmd.subview(0, sCmd.find_first_of("({"));
    bool isCluster = sCmd[sCache.length()] == '{';

    // remove whitespaces
    sCache.strip();

    // check, whether it exists
    if (!sCache.length())
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd.to_string(), SyntaxError::invalid_position);

    // Ensure that the indices are valid
    if (!_idx.row.isValid() || !_idx.col.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

    // Is it the "string" object?
    if (sCache == "string")
    {
        expandStringIndexVectors(_idx, _data);
        return;
    }

    // If the cache is not really a cache
    if (!isCluster && !_data.isTable(sCache.to_string()))
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd.to_string(), SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This static function expands the
/// indices into vectors, if the current object
/// is the string object.
///
/// \param _idx Indices&
/// \param _data MemoryManager&
/// \return void
///
/////////////////////////////////////////////////
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

