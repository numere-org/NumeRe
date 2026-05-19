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
#include "cluster.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"
#include <vector>

using namespace std;
using namespace mu;

enum INDEX_TYPE
{
    INDEX_ROW,
    INDEX_COL,
    INDEX_LAYER
};

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data,
                                     StringView sArgument, StringView sCmd, StringView sTableName, bool isAssignment);
static void extractIndexList(StringView sCols,
                             vector<StringView>& vLines, vector<StringView>& vCols, vector<StringView>& vLayers);
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex, StringView sTableName,
                               StringView sCmd, bool isAssignment, INDEX_TYPE type);
static void handleCasualIndices(Parser& _parser, Indices& _idx,
                                vector<StringView>& vLines, vector<StringView>& vCols, vector<StringView>& vLayers,
                                StringView sTableName, StringView sCmd, bool isAssignment);
static void handleSingleCasualIndex(VectorIndex& _vIdx, vector<StringView>& vIndexDef,
                                    string& sIndexExpressions, vector<std::pair<INDEX_TYPE, size_t>>& vIndex, INDEX_TYPE type);
static void expandIndexVectors(Indices& _idx, MemoryManager& _data, StringView sCmd);


/////////////////////////////////////////////////
/// \brief Wrapper for the new getIndices
/// function interface.
///
/// \param sCmd StringView
/// \param _parser Parser&
/// \param _data MemoryManager&
/// \param isAssignment bool
/// \return Indices
///
/// \deprecated Marked as deprecated.
///
/////////////////////////////////////////////////
Indices getIndices(StringView sCmd, Parser& _parser, MemoryManager& _data, bool isAssignment)
{
    Indices _idx;
    getIndices(sCmd, _idx, _parser, _data, isAssignment);
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
/// \param isAssignment bool
/// \return void
///
/////////////////////////////////////////////////
void getIndices(StringView sCmd, Indices& _idx,  Parser& _parser, MemoryManager& _data, bool isAssignment)
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
    if (nClosingParens == std::string::npos)
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
        _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0, VectorIndex::OPEN_END);
        _idx.layer = VectorIndex(0, VectorIndex::OPEN_END);
        return;
    }

    _idx.sCompiledAccessEquation.assign(sIndices.begin(), sIndices.end());
// 510
    // If the argument contains tables, get their values. This leads to a recursion!
    if (_data.containsTables(_idx.sCompiledAccessEquation))
        getDataElements(_idx.sCompiledAccessEquation, _parser, _data);
// 1000

    // update the dimension variables
    if (sCmd[nPos] == '(')
        _data.updateDimensionVariables(sTableName);

        //_idx.row.setIndex(0,0);
        //_idx.col.setIndex(0,0);
        //return;
// 1050

    // If the argument contains a comma, handle it as a usual index list
    handleArgumentForIndices(_idx, _parser, _data, _idx.sCompiledAccessEquation, sCmd, sTableName, isAssignment);

    // Check indices here
    if (!_idx.row.checkRange() || !_idx.col.checkRange() || !_idx.layer.checkRange())
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), nPos+1,
                          _idx.row.to_string() + ", " + _idx.col.to_string() + ", " + _idx.layer.to_string());
// 1800
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
/// \param sTableName StringView
/// \param isAssignment bool
/// \return void
///
/////////////////////////////////////////////////
static void handleArgumentForIndices(Indices& _idx, Parser& _parser, MemoryManager& _data, StringView sArgument, StringView sCmd, StringView sTableName, bool isAssignment)
{
    std::vector<StringView> vLines;
    std::vector<StringView> vCols;
    std::vector<StringView> vLayers;

    // extract the (textual) indices from the argument list and store it in sI and sJ
    extractIndexList(sArgument, vLines, vCols, vLayers);
//        _idx.row.setIndex(0,0);
//        _idx.col.setIndex(0,0);
//        return;
//1450
    // Detect, whether the line indices are candidates
    // for vectors
    if (vLines.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.row, vLines.front(), sTableName, sCmd, isAssignment, INDEX_ROW);
    }

    // Detect, whether the column indices are candidates
    // for vectors
    if (vCols.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.col, vCols.front(), sTableName, sCmd, isAssignment, INDEX_COL);
    }

    // Detect, whether the layer indices are candidates
    // for vectors
    if (vLayers.size() == 1)
    {
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx.layer, vLayers.front(), sTableName, sCmd, isAssignment, INDEX_LAYER);
    }

    // Ensure that the indices are casuals and no indices
    if (vLines.size() > 1 || vCols.size() > 1 || vLayers.size() > 1)
    {
        // Handle the casual indices
        handleCasualIndices(_parser, _idx, vLines, vCols, vLayers, sTableName, sCmd, isAssignment);
    }

    if (_idx.row.numberOfNodes() > 2 || _idx.col.numberOfNodes() > 2 || _idx.layer.numberOfNodes() > 2)
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
/// \param sIndices StringView
/// \param vLines vector<StringView>&
/// \param vCols vector<StringView>&
/// \param vLayers vector<StringView>&
/// \return void
///
/////////////////////////////////////////////////
static void extractIndexList(StringView sIndices, vector<StringView>& vLines, vector<StringView>& vCols, vector<StringView>& vLayers)
{
    // Split line and column indices at
    // the comma (if it is available). Otherwise
    // only the line indices are available
    StringView sLines(getNextViewedArgument(sIndices));
    StringView sCols(getNextViewedArgument(sIndices));

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
    // empty StringView instance.
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

    // If the layer indices are available,
    // split them also. Otherwise use an
    // empty StringView instance.
    if (sIndices.length())
    {
        openEnd = sIndices.back() == ':';

        vLayers.push_back(getNextViewedIndex(sIndices));

        // As long as there's a column index
        // available, separate it here
        while (sIndices.length())
        {
            vLayers.push_back(getNextViewedIndex(sIndices));
        }

        if (openEnd)
            vLayers.push_back(StringView());
    }
    else
        vLayers.push_back(StringView());
}


/////////////////////////////////////////////////
/// \brief Converts possible string indices into
/// column indices for the respective table.
///
/// \param a const mu::Array&
/// \param sTableName StringView
/// \param isAssignment bool
/// \param type INDEX_TYPE
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array stringToNumIndex(const mu::Array& a, StringView sTableName, bool isAssignment, INDEX_TYPE type)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    mu::Array ret;
    ret.reserve(a.count());

    for (size_t i = 0; i < a.size(); i++)
    {
        if (a.get(i).isString())
        {
            std::vector<size_t> indices;

            // Find the columns if any
            if (type == INDEX_COL)
                indices = _data.findCols(sTableName.to_string(), {a.get(i).getStr()}, false, isAssignment);
            else if (type == INDEX_LAYER)
                indices = _data.findLayers(sTableName.to_string(), {a.get(i).getStr()}, false, isAssignment);

            // insert the found columns
            ret.insert(ret.end(), indices.begin(), indices.end());
        }
        else
            ret.emplace_back(a.get(i));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This static function will evaluate the
/// indices and it tries to match it to a vector.
///
/// \param _parser Parser&
/// \param _vIdx VectorIndex&
/// \param sIndex StringView
/// \param sTableName StringView
/// \param sCmd StringView
/// \param isAssignment bool
/// \param type INDEX_TYPE
/// \return void
///
/////////////////////////////////////////////////
static void handleIndexVectors(Parser& _parser, VectorIndex& _vIdx, StringView sIndex, StringView sTableName, StringView sCmd, bool isAssignment, INDEX_TYPE type)
{
    mu::Array v;

    if (!sIndex.length())
        _vIdx.front() = 0;
    else if (sIndex == "#")
        _vIdx.front() = VectorIndex::STRING;
    else
    {
        _parser.SetExpr(sIndex);
        mu::Array v = stringToNumIndex(_parser.Eval(), sTableName, isAssignment, type);

        if (!v.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), SyntaxError::invalid_position, v.print());

        if (v.size() > 1) // vector
            _vIdx = VectorIndex(v);
        else if (v.front().isValid()) // single index
            _vIdx.front() = v.getAsScalarInt() - 1;
    }
}


/////////////////////////////////////////////////
/// \brief This static function is a helper
/// function for handleCasualIndices(), which
/// will be applied to row and column indices
/// separately.
///
/// \param _vIdx VectorIndex&
/// \param vIndexDef vector<StringView>&
/// \param sIndexExpressions string&
/// \param vIndex vector<std::pair<INDEX_TYPE, size_t>>&
/// \param type INDEX_TYPE
/// \return void
///
/////////////////////////////////////////////////
static void handleSingleCasualIndex(VectorIndex& _vIdx, vector<StringView>& vIndexDef, string& sIndexExpressions,
                                    vector<std::pair<INDEX_TYPE, size_t>>& vIndex, INDEX_TYPE type)
{
    for (size_t n = 0; n < vIndexDef.size(); n++)
    {
        if (!vIndexDef[n].length())
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

                sIndexExpressions += vIndexDef[n].to_string();

                // Store the assignment
                vIndex.push_back(std::make_pair(type, n));
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
/// \param v const StackItem*
/// \param vIndex const vector<std::pair<INDEX_TYPE, size_t>>&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string convertToString(const StackItem* v, const vector<std::pair<INDEX_TYPE, size_t>>& vIndex)
{
    std::string sIndexExpression;

    for (size_t i = 0; i < vIndex.size(); i++)
    {
        if (i)
        {
            if (vIndex[i-1].first != vIndex[i].first)
                sIndexExpression += ", ";
            else
                sIndexExpression += ':';
        }

        sIndexExpression += v[i].get().print();
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
/// \param vLayers vector<StringView>&
/// \param sTableName StringView
/// \param sCmd StringView
/// \param isAssignment bool
/// \return void
///
/////////////////////////////////////////////////
static void handleCasualIndices(Parser& _parser, Indices& _idx, vector<StringView>& vLines, vector<StringView>& vCols, vector<StringView>& vLayers, StringView sTableName, StringView sCmd, bool isAssignment)
{
    string sIndexExpressions;
    vector<std::pair<INDEX_TYPE, size_t>> vIndex;

    // Go through all indices and connect them to one single equation
    // store the assignment of the indices
    if (!_idx.row.isValid())
        handleSingleCasualIndex(_idx.row, vLines, sIndexExpressions, vIndex, INDEX_ROW);

    // Go through the column indices separately,
    // because they might be more than two
    if (!_idx.col.isValid())
        handleSingleCasualIndex(_idx.col, vCols, sIndexExpressions, vIndex, INDEX_COL);

    // Go through the column indices separately,
    // because they might be more than two
    if (!_idx.layer.isValid())
        handleSingleCasualIndex(_idx.layer, vLayers, sIndexExpressions, vIndex, INDEX_LAYER);

    // If the index expression list has a length, evaluate it
    if (sIndexExpressions.length())
    {
        _parser.SetExpr(sIndexExpressions);
		int nResults;
        const mu::StackItem* v = _parser.Eval(nResults);

        // check whether the number of the results is matching
        if ((size_t)nResults != vIndex.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(), SyntaxError::invalid_position, sIndexExpressions);

        // map the results to their assignments
        for (int i = 0; i < nResults; i++)
        {
            if (!v[i].get().size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(),
                                  SyntaxError::invalid_position, v[i].get().print());

            mu::Array idx = stringToNumIndex(v[i].get(), sTableName, isAssignment, vIndex[i].first);

            if (isinf(idx.front().getNum().asF64())) // infinity => last possible index
                idx.front() = mu::Value(-1); // only -1 because it will be decremented in the following lines
            else if (!idx.front().isValid() || idx.getAsScalarInt() <= 0LL)
            {
                std::string sToken;

                if (vIndex.front().first != vIndex.back().first)
                    sToken = convertToString(v, vIndex);
                else if (vIndex.front().first == INDEX_ROW)
                {
                    sToken = convertToString(v, vIndex);
                    sToken += ", " + _idx.col.to_string();
                }
                else if (vIndex.front().first == INDEX_LAYER)
                {
                    sToken = _idx.row.to_string() + ", " + _idx.col.to_string() + ", ";
                    sToken += convertToString(v, vIndex);
                }
                else
                {
                    sToken = _idx.row.to_string() + ", ";
                    sToken += convertToString(v, vIndex);
                }

                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd.to_string(),
                                  SyntaxError::invalid_position, sToken);
            }

            switch (vIndex[i].first)
            {
            case INDEX_ROW:
                _idx.row.setIndex(vIndex[i].second, idx.getAsScalarInt() - 1);
                break;
            case INDEX_COL:
                _idx.col.setIndex(vIndex[i].second, idx.getAsScalarInt() - 1);
                break;
            case INDEX_LAYER:
                _idx.layer.setIndex(vIndex[i].second, idx.getAsScalarInt() - 1);
                break;
            }
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
    if (!_idx.row.isValid() || !_idx.col.isValid() || !_idx.layer.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX,
                          sCmd.to_string(),
                          SyntaxError::invalid_position,
                          _idx.row.to_string() + ", " + _idx.col.to_string() + ", " + _idx.layer.to_string());

    // If the cache is not really a cache
    if (!isCluster && !_data.isTable(sCache.to_string()))
        throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd.to_string(), SyntaxError::invalid_position);
}


