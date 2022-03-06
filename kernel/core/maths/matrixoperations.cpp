/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2015  Erik Haenel et al.

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
#include <Eigen/Dense>

#include "matrixoperations.hpp"
#include "matfuncs.hpp"
#include "parser_functions.hpp"
#include "../../kernel.hpp"
#include "../utils/stringtools.hpp"
#include "../io/logger.hpp"
#include <list>
#include <cmath>

#define EIGENVALUES 0
#define EIGENVECTORS 1
#define DIAGONALIZE 2

struct MatOpCache
{
    // Returned values
    std::vector<Matrix> vReturnedMatrices;

    // Cached tables or clusters
    // Structure definition
    struct MatOpDataAccess
    {
        std::string sName;
        Indices idx;
        int missingValues;
    };

    // Actual cache
    std::vector<MatOpDataAccess> vDataAccesses;
};

static Matrix evalMatOp(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option, MatOpCache& _cache);
static size_t getPrevMatMultiOp(const string& sCmd, size_t nLastPos);
static Matrix multiplyMatrices(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position);
static Matrix getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static vector<mu::value_type> calcDeltasForMatFill(const Matrix& _mMatrix, unsigned int nLine);
static void showMatrixResult(const Matrix& _mResult, const Settings& _option);
static Indices getIndicesForMatrix(const string& sCmd, const MatOpCache& _cache, Parser& _parser, MemoryManager& _data, const Settings& _option);
static bool containsMatrices(const string& sExpr, MemoryManager& _data);

Matrix transposeMatrix(const Matrix& _mMatrix);
Matrix createZeroesMatrix(unsigned int nLines, unsigned int nCols);

static Matrix createMatFromCols(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix createMatFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix createMatFromLines(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix createMatFromLinesFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);





/////////////////////////////////////////////////
/// \brief This function is the main interface to
/// the matrix operations.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool performMatrixOperation(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    // Create the cache
    MatOpCache _cache;
    string sTargetName = "";
    Indices _idx;

    bool bAllowMatrixClearing = false;
    bool isCluster = false;

    // Kommando entfernen
    if (findCommand(sCmd).sString == "matop")
        sCmd.erase(0, findCommand(sCmd).nPos+5);

    if (findCommand(sCmd).sString == "mtrxop")
        sCmd.erase(0, findCommand(sCmd).nPos+6);

    if (!_functions.call(sCmd))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

    // Ensure that there's at least a single
    // matrix operation available
    if (!containsMatrices(sCmd, _data))
        throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, SyntaxError::invalid_position);

    // Rekursive Ausdruecke ersetzen
    evalRecursiveExpressions(sCmd);

    // Target identifizieren
    if (sCmd.find('=') != string::npos
        && sCmd.find('=')
        && sCmd[sCmd.find('=')+1] != '='
        && sCmd[sCmd.find('=')-1] != '!'
        && sCmd[sCmd.find('=')-1] != '<'
        && sCmd[sCmd.find('=')-1] != '>'
        && sCmd.substr(0, sCmd.find('=')).find_first_of("({") != string::npos
        && sCmd[sCmd.find_first_not_of(' ')] != '{')
    {
        sTargetName = sCmd.substr(0, sCmd.find('='));
        StripSpaces(sTargetName);
        size_t parens = sTargetName.find_first_of("({");

        if (parens == string::npos)
            throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, sTargetName, sTargetName);

        // Distinguish between tables and clusters
        if (sTargetName[parens] == '(')
        {
            // Create a new table
            if (!_data.isTable(sTargetName))
                _data.addTable(sTargetName.substr(0, parens), _option);

            if (sTargetName.substr(sTargetName.find('('),2) == "()")
            {
                bAllowMatrixClearing = true;
                _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
                _idx.col = VectorIndex(0, VectorIndex::OPEN_END);
            }
        }
        else if (sTargetName[parens] == '{')
        {
            // Create a new cluster
            if (!_data.isCluster(sTargetName))
                _data.newCluster(sTargetName);

            isCluster = true;

            if (sTargetName.substr(sTargetName.find('{'),2) == "{}")
            {
                bAllowMatrixClearing = true;
                _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
                _idx.col = VectorIndex(0, VectorIndex::INVALID);
            }
        }

        if (!bAllowMatrixClearing)
            getIndices(sTargetName, _idx, _parser, _data, _option);

        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sTargetName, _idx.row.to_string() + ", " + _idx.col.to_string());

        sCmd.erase(0, sCmd.find('=')+1);

        sTargetName.erase(sTargetName.find_first_of("({"));
    }
    else
    {
        sTargetName = "matrix";
        _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0, VectorIndex::OPEN_END);

        if (!_data.isTable("matrix("))
            _data.addTable("matrix", _option);
        else
            bAllowMatrixClearing = true;
    }

    g_logger.debug("Start evalMatOp.");
    // Matrixmultiplikationen / Transpositionen / Invertierungen?
    // -> Submatrixoperationen ausfuehren
    Matrix _mResult = evalMatOp(sCmd, _parser, _data, _functions, _option, _cache);
    g_logger.debug("Stop evalMatOp.");
    _assertionHandler.checkAssertion(_mResult);

    // Safety check
    if (!_mResult.size())
        return false;

    // Target in Zielmatrix speichern
    if (!isCluster)
    {
        // This target is a table
        if (bAllowMatrixClearing)
            _data.deleteBulk(sTargetName, 0, _data.getLines(sTargetName, false) - 1, 0, _data.getCols(sTargetName, false) - 1);

        // Prepare the target size
        _data.resizeTable(_idx.col.front()+_mResult[0].size(), sTargetName);
        Memory* _table = _data.getTable(sTargetName);

        int maxrow = _idx.row.subidx(0, _mResult.size()).max();

        for (size_t j = 0; j < _mResult[0].size(); j++)
        {
            if (_idx.col[j] == VectorIndex::INVALID)
                break;

            _table->writeData(maxrow, _idx.col[j], 0.0);
        }

        // Write the contents to the table
        #pragma omp parallel for
        for (unsigned int j = 0; j < _mResult[0].size(); j++)
        {
            if (_idx.col[j] == VectorIndex::INVALID)
                continue;

            for (unsigned int i = 0; i < _mResult.size(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                _table->writeDataDirectUnsafe(_idx.row[i], _idx.col[j], _mResult[i][j]);
            }
        }

        _table->markModified();
    }
    else
    {
        // This target is a cluster, get a reference to it
        NumeRe::Cluster& cluster = _data.getCluster(sTargetName);

        if (bAllowMatrixClearing)
            cluster.clear();

        // Assign either the first column or the first line
        if (_mResult.size() == 1)
        {
            // Assign the first line
            for (size_t i = 0; i < _mResult[0].size(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                cluster.setDouble(_idx.row[i], _mResult[0][i]);
            }
        }
        else
        {
            // Assign the first column
            for (size_t i = 0; i < _mResult.size(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                cluster.setDouble(_idx.row[i], _mResult[i][0]);
            }
        }
    }

    // Display the result in the terminal
    showMatrixResult(_mResult, _option);

    return true;
}


/////////////////////////////////////////////////
/// \brief This is the actual worker function
/// for matrix operations. It will be called
/// recursively for functions and parentheses.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix evalMatOp(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option, MatOpCache& _cache)
{
    string __sCmd;
    size_t pos_back = 0;
    size_t iter_start = 0;

    mu::value_type* v = 0;
    int nResults = 0;
    static std::map<std::string, MatFuncDef> mMatrixFunctions = getMatrixFunctions();

    // Check, whether there's a target vector available in this expression part
    size_t nEqPos = sCmd.find('=');

    if (nEqPos != string::npos
        && nEqPos
        && sCmd[nEqPos+1] != '='
        && sCmd[nEqPos-1] != '!'
        && sCmd[nEqPos-1] != '<'
        && sCmd[nEqPos-1] != '>'
        && sCmd.substr(0, nEqPos).find('{') != string::npos)
    {
        iter_start = nEqPos+1;
    }

    // Apply all known and requested matrix functions.
    // The equation will be segmentized and the evaluated
    // part will be appended to the evaluated equation.
    for (unsigned int i = iter_start; i < sCmd.length(); i++)
    {
        size_t nMatchingParens;
        // Is the current character alphabetical and
        // either the first or prefixed with a delimiter?
        // Then this is a possible start for a function
        if ((!i || isDelimiter(sCmd[i-1])) && (isalpha(sCmd[i]) || sCmd[i] == '{'))
        {
            for (auto fIter = mMatrixFunctions.begin(); fIter != mMatrixFunctions.end(); ++fIter)
            {
                if (fIter->second.signature == MATSIG_INVALID)
                    continue;

                if (StringView(sCmd, i, fIter->first.length()+1) == fIter->first + "("
                    && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i+fIter->first.length()))) != string::npos)
                {
                    // Extract argument
                    std::string sSubExpr = sCmd.substr(i+fIter->first.length()+1, nMatchingParens-1);

                    // Create the error structure
                    MatFuncErrorInfo errorInfo(sCmd, sSubExpr, i+fIter->first.length());

                    // Handle the signatures and
                    // perform the recursions
                    switch (fIter->second.signature)
                    {
                        case MATSIG_INVALID:
                            break; // Just to avoid compiler warnings
                        case MATSIG_MAT:
                        {
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache)), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_NOPT:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            int n = 0;

                            if (sSubExpr.length())
                            {
                                _parser.SetExpr(sSubExpr);
                                v = _parser.Eval(nResults);
                                n = intCast(v[0]);
                            }

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _functions, _option, _cache), n), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_MAT:
                        {
                            std::string sMatrix1 = getNextArgument(sSubExpr, true);
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix1, _parser, _data, _functions, _option, _cache), evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache)), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_MAT_MAT:
                        {
                            std::string sMatrix1 = getNextArgument(sSubExpr, true);
                            std::string sMatrix2 = getNextArgument(sSubExpr, true);
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix1, _parser, _data, _functions, _option, _cache), evalMatOp(sMatrix2, _parser, _data, _functions, _option, _cache), evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache)), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_F:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);

                            _parser.SetExpr(sSubExpr);
                            v = _parser.Eval(nResults);
                            mu::value_type fVal = v[0];

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _functions, _option, _cache), fVal), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_F_N:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);

                            _parser.SetExpr(sSubExpr);
                            v = _parser.Eval(nResults);
                            mu::value_type fVal = v[0];
                            int n = intCast(v[1]);

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _functions, _option, _cache), fVal, n), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_N_MOPT:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);

                            _parser.SetExpr(sSubExpr);
                            v = _parser.Eval(nResults);
                            int n = intCast(v[0]);
                            int m = intCast(v[0]);

                            if (nResults > 1)
                                m = intCast(v[1]);

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _functions, _option, _cache), n, m), errorInfo));
                            break;
                        }
                        case MATSIG_N_MOPT:
                        {
                            _parser.SetExpr(sSubExpr);
                            v = _parser.Eval(nResults);

                            if (nResults > 1)
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(v[0]), intCast(v[1])), errorInfo));
                            else
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(v[0])), errorInfo));

                            break;
                        }
                    }

                    // Prepare leading part of the equation
                    __sCmd += sCmd.substr(pos_back, i-pos_back);

                    // Store the last position and advance the
                    // current position
                    pos_back = i+nMatchingParens+fIter->first.length()+1;
                    i = pos_back-1;

                    // append the returned matrix
                    __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                }
            }

            // construct matrix from columns -> MAT(...)
            if (sCmd.substr(i,6) == "matfc("
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i+5))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+6, nMatchingParens-1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+6;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from lines -> MAT(...)
            if (sCmd.substr(i,6) == "matfl("
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i+5))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+6, nMatchingParens-1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromLines(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+6;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from columns and fill missing values -> MAT(...)
            if (sCmd.substr(i,7) == "matfcf("
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i+6))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+7, nMatchingParens-1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromColsFilled(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+7;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from lines and fill missing values -> MAT(...)
            if (sCmd.substr(i,7) == "matflf("
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i+6))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+7, nMatchingParens-1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromLinesFilled(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+7;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // Handle old vector syntax (will most probably be changed to matrix syntax)
            if (sCmd.substr(i,2) == "{{"
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i, nMatchingParens+1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+1;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // Handle vectors
            if (sCmd[i] == '{'
                && (nMatchingParens = getMatchingParenthesis(StringView(sCmd, i))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i, nMatchingParens+1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+nMatchingParens+1;
                __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }
        }

        // Handle index selectors after a matrix function
        if (i > 14
            && sCmd[i] == '('
            && i == pos_back // only true, if the last iteration evaluated a subexpression
            && sCmd.find_last_not_of(' ',i-1) != string::npos
            && __sCmd.back() == ']') //...returnedMatrix[N](:,:)
        {
            int nMatrix = 0;
            nMatrix = StrToInt(__sCmd.substr(__sCmd.rfind('[')+1, __sCmd.rfind(']')-__sCmd.rfind('[')-1));
            if (__sCmd.substr(__sCmd.rfind('[')-16,17) == "_~returnedMatrix[")
            {
                nMatchingParens = getMatchingParenthesis(StringView(sCmd, i));
                string sSubExpr = sCmd.substr(i, nMatchingParens+1);
                pos_back = i+nMatchingParens+1;
                _cache.vReturnedMatrices[nMatrix] = getMatrixElements(sSubExpr, _cache.vReturnedMatrices[nMatrix], _parser, _data, _functions, _option);
                i = pos_back-1;
            }
        }

        // Pre-evaluate parentheses
        if (sCmd[i] == '(')
        {
            nMatchingParens = getMatchingParenthesis(StringView(sCmd, i));
            size_t nLastDelimiter;

            if (sCmd.substr(i, nMatchingParens).find("**") != string::npos
                || (i > 1
                    && (nLastDelimiter = sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)) != std::string::npos
                    && !_data.isTable(sCmd.substr(nLastDelimiter+1, i-nLastDelimiter-1))))
            {
                string sSubExpr = sCmd.substr(i+1, nMatchingParens-1);
                size_t closing_par_pos = i+nMatchingParens;

                if (sCmd.length() > closing_par_pos+1 && sCmd[closing_par_pos+1] == '(')
                {
                    if (i && (isalnum(sCmd[i-1]) || sCmd[i-1] == '_'))
                    {
                        for (int j = i-1; j >= 0; j--)
                        {
                            if ((j && !isalnum(sCmd[j-1]) && sCmd[j-1] != '_') || !j)
                            {
                                __sCmd += sCmd.substr(pos_back, j-pos_back);
                                sSubExpr = sCmd.substr(j, closing_par_pos+1 - j);
                                break;
                            }
                        }
                    }
                    else
                        __sCmd += sCmd.substr(pos_back, i-pos_back);

                    _cache.vReturnedMatrices.push_back(evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache));
                    __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                }
                else
                {
                    __sCmd += sCmd.substr(pos_back, i-pos_back);
                    bool isFunc = isalnum(__sCmd.back());

                    if (isFunc)
                        __sCmd += "(";

                    // As this might be a usual function, evaluate each argument
                    // separately (if it is matrix expression) and combine them
                    // afterwards
                    while (sSubExpr.length())
                    {
                        string sExpr = getNextArgument(sSubExpr, true);

                        if (containsMatrices(sExpr, _data))
                        {
                            _cache.vReturnedMatrices.push_back(evalMatOp(sExpr, _parser, _data, _functions, _option, _cache));
                            __sCmd += "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]";
                        }
                        else
                            __sCmd += sExpr;

                        if (sSubExpr.length())
                            __sCmd += ",";
                    }

                    if (isFunc)
                        __sCmd += ")";
                }

                pos_back = closing_par_pos+1;
                i = pos_back-1;
            }
        }
    }

    // Append the missing part of the equation
    if (pos_back < sCmd.length())
        __sCmd += sCmd.substr(pos_back);

    vector<mu::value_type> vMatrixVector;
    Matrix _mTarget;

    unsigned int nPos = 0;
    unsigned int nColCount = 0;

    // Get the indices of the calls to the memory objects
    // all caches
    for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
    {
        nPos = 0;

        while ((nPos = __sCmd.find(iter->first+"(", nPos)) != string::npos)
        {
            // Check the delimiters
            if (nPos && !isDelimiter(__sCmd[nPos-1]))
            {
                nPos++;
                continue;
            }

            MatOpCache::MatOpDataAccess _access;

            // Get the indices
            _access.idx = getIndicesForMatrix(__sCmd.substr(nPos), _cache, _parser, _data, _option);

            // Evaluate the indices
            if (!evaluateIndices(iter->first, _access.idx, _data))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, nPos);

            // Store the name of the current data object
            _access.sName = iter->first;

            // Identify, which value to use for a missing value
            if (addMissingVectorComponent("", __sCmd.substr(0,nPos), __sCmd.substr(nPos+1+(iter->first).length()+getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))),false) == "0")
                _access.missingValues = 0;
            else
                _access.missingValues = 1;

            // Add the cached access
            _cache.vDataAccesses.push_back(_access);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))+(iter->first).length()+1, "_~matrix["+toString((int)_cache.vDataAccesses.size()-1)+"]");
        }
    }

    // now all clusters
    for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
    {
        nPos = 0;

        while ((nPos = __sCmd.find(iter->first+"{", nPos)) != string::npos)
        {
            // Check the delimiters
            if (nPos && !isDelimiter(__sCmd[nPos-1]))
            {
                nPos++;
                continue;
            }

            // Get the indices
            Indices _idx = getIndicesForMatrix(__sCmd.substr(nPos), _cache, _parser, _data, _option);

            if (_idx.row.isOpenEnd())
                _idx.row.setRange(0, iter->second.size()-1);

            // Prepare a target matrix
            Matrix _mClusterMatrix = createZeroesMatrix(_idx.row.size(), 1);

            // Write the contents to the matrix
            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                _mClusterMatrix[i][0] = iter->second.getDouble(_idx.row[i]);
            }

            // Declare the cluster as a returned matrix (simplifies the
            // access logic further down)
            _cache.vReturnedMatrices.push_back(_mClusterMatrix);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))+(iter->first).length()+1, "_~returnedMatrix["+toString((int)_cache.vReturnedMatrices.size()-1)+"]");
        }
    }

    pos_back = sCmd.length();

    // Apply the matrix multiplication
    if (__sCmd.find("**") != string::npos)
    {
        // Matrix multiplication is right handed
        // -> start from the end of the expression
        for (int n = __sCmd.length()-1; n >= 0; n--)
        {
            // Theres's a matrix multiplication
            if (__sCmd.substr(n, 2) == "**")
            {
                // Find the last operator
                pos_back = getPrevMatMultiOp(sCmd, pos_back)-1;
                Matrix _mLeft;
                Matrix _mRight;
                unsigned int nPositions[2];
                nPositions[1] = __sCmd.find(']',n)+1;
                string sElement = __sCmd.substr(__sCmd.find_first_not_of(' ', n+2));

                // Right handed matrix expression
                size_t p;

                for (size_t i = 0; i < sElement.length(); i++)
                {
                    if ((sElement[i] == '(' || sElement[i] == '{' || sElement[i] == '[')
                         && (p = getMatchingParenthesis(StringView(sElement, i))) != std::string::npos)
                        i += p+1;

                    if (!isalnum(sElement[i]) && sElement[i] != '_' && sElement[i] != '~')
                    {
                        std::string sSubExpr = sElement.substr(0, i);
                        nPositions[1] = i+__sCmd.find_first_not_of(' ', n+2);
                        g_logger.debug("sSubExpr = " + sSubExpr);
                        _mRight = evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache);
                        g_logger.debug("_mRight.size() = " + toString(_mRight.size()) + "," + toString(_mRight[0].size()));
                        break;
                    }
                }

                if (!_mRight.size())
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                // Left handed matrix expression
                int nBraces = 0;
                p = __sCmd.find_last_not_of(" *", n);

                for (int i = p; i >= 0; i--)
                {
                    if (__sCmd[i] == ')' || __sCmd[i] == '}' || __sCmd[i] == ']')
                        nBraces++;

                    if (__sCmd[i] == '(' || __sCmd[i] == '{' || __sCmd[i] == '[')
                        nBraces--;

                    if (!nBraces
                        && (!i || (!isalnum(__sCmd[i-1]) && __sCmd[i-1] != '_' && __sCmd[i-1] != '~')))
                    {
                        std::string sSubExpr = __sCmd.substr(i, p - i+1);
                        nPositions[0] = i;
                        g_logger.debug("sSubExpr = " + sSubExpr);
                        _mLeft = evalMatOp(sSubExpr, _parser, _data, _functions, _option, _cache);
                        g_logger.debug("_mLeft.size() = " + toString(_mLeft.size()) + "," + toString(_mLeft[0].size()));
                        break;
                    }
                }

                if (!_mLeft.size())
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                // Perform the actual matrix multiplication
                _cache.vReturnedMatrices.push_back(multiplyMatrices(_mLeft, _mRight, sCmd, "", pos_back+1));

                // Replace the current multiplication with its return value
                __sCmd.replace(nPositions[0], nPositions[1]-nPositions[0], "_~returnedMatrix[" + toString((int)_cache.vReturnedMatrices.size()-1)+"]");
                n = nPositions[0];
            }
        }
    }

    sCmd = __sCmd;

    std::map<size_t, std::string> mDataMatrices;
    std::map<size_t, std::string> mReturnedMatrices;

    // We need the maximal number of columns for the regular
    // evaluation of the expression
    //
    // Examine the memory accesses, which are still available
    // in the expression
    for (unsigned int i = 0; i < _cache.vDataAccesses.size(); i++)
    {
        std::string sMatrixName = "_~matrix["+toString((int)i)+"]";

        if (sCmd.find(sMatrixName) == string::npos)
            continue;

        mDataMatrices[i] = sMatrixName;

        // Get the column count from the dimensions of the indices
        if (_cache.vDataAccesses[i].idx.col.size() > nColCount)
            nColCount = _cache.vDataAccesses[i].idx.col.size();
    }

    // Examine now the return values available in the expression
    for (unsigned int i = 0; i < _cache.vReturnedMatrices.size(); i++)
    {
        std::string sMatrixName = "_~returnedMatrix["+toString((int)i)+"]";

        if (sCmd.find(sMatrixName) == string::npos)
            continue;

        mReturnedMatrices[i] = sMatrixName;

        if (_cache.vReturnedMatrices[i][0].size() > nColCount)
            nColCount = _cache.vReturnedMatrices[i][0].size();
    }

    // Shortcut => only a single matrix
    if (mDataMatrices.size() == 1 && isEqualStripped(sCmd, mDataMatrices.begin()->second))
    {
        auto iter = mDataMatrices.begin();

        for (size_t i = 0; i < _cache.vDataAccesses[iter->first].idx.row.size(); i++)
        {
            _mTarget.push_back(_data.getElement(VectorIndex(_cache.vDataAccesses[iter->first].idx.row[i]),
                                                _cache.vDataAccesses[iter->first].idx.col,
                                                _cache.vDataAccesses[iter->first].sName));
        }

        return _mTarget;
    }

    if (mReturnedMatrices.size() == 1 && isEqualStripped(sCmd, mReturnedMatrices.begin()->second))
        return _cache.vReturnedMatrices[mReturnedMatrices.begin()->first];

    // Read now the first column of every matrix in the expression
    // as vector for the parser
    //
    // start with the memory accesses
    for (const auto& iter : mDataMatrices)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();

        // Get the values using the incdices
        const MatOpCache::MatOpDataAccess& _access = _cache.vDataAccesses[iter.first];
        vMatrixVector = _data.getElement(_access.idx.row, VectorIndex(_access.idx.col.front()), _access.sName);

        // Declare the corresponding vector variable
        _parser.SetVectorVar(iter.second, vMatrixVector);
    }

    // Get now the first columns of the returned values
    for (const auto& iter : mReturnedMatrices)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();

        // Get the first column from the return values
        if (!_cache.vReturnedMatrices[iter.first][0].size())
            vMatrixVector.push_back(0.0);
        else
        {
            for (unsigned int k = 0; k < _cache.vReturnedMatrices[iter.first].size(); k++)
            {
                vMatrixVector.push_back(_cache.vReturnedMatrices[iter.first][k][0]);
            }
        }

        // Declare the corresponding vector variable
        _parser.SetVectorVar(iter.second, vMatrixVector);
    }

    // Set the expression in the parser
    _parser.SetExpr(sCmd);

    // Evaluate the first columns
    v = _parser.Eval(nResults);

    // Ensure that the column dimension is at least 1
    if (!nColCount && nResults)
        nColCount = 1;

    // Append the vector to the target matrix
    _mTarget.resize(nResults, std::vector<mu::value_type>(nColCount, 0.0));

    for (int n = 0; n < nResults; n++)
    {
        _mTarget[n][0] = v[n];
    }

    g_logger.debug("Matrix loop.");

    // Get now the next columns as vectors
    // and evaluate the expression with their
    // values
    for (unsigned int i = 1; i < nColCount; i++)
    {
        // Read the next columns of every matrix in the expression
        // as vector for the parser
        for (const auto& iter : mDataMatrices)
        {
            std::vector<mu::value_type>* currentVec = _parser.GetVectorVar(iter.second);

            if (!currentVec)
                continue;

            const MatOpCache::MatOpDataAccess& _access = _cache.vDataAccesses[iter.first];

            if (_access.idx.col.size() <= i && (_access.idx.col.size() > 1 || _access.idx.row.size() > 1))
                currentVec->assign(1, _access.missingValues);
            else if (_access.idx.row.size() >= 1 && _access.idx.col.size() == 1)
                continue;
            else
                _data.copyElementsInto(currentVec, _access.idx.row, VectorIndex(_access.idx.col[i]), _access.sName);

            _parser.UpdateVectorVar(iter.second);
        }

        // Get now the next columns of the returned values
        for (const auto& iter : mReturnedMatrices)
        {
            std::vector<mu::value_type>* currentVec = _parser.GetVectorVar(iter.second);

            if (!currentVec)
                continue;

            if (!_cache.vReturnedMatrices[iter.first][0].size())
                currentVec->assign(1, 0.0);
            else if (_cache.vReturnedMatrices[iter.first].size() >= 1 && _cache.vReturnedMatrices[iter.first][0].size() == 1)
                continue;
            else
            {
                currentVec->resize(_cache.vReturnedMatrices[iter.first].size(), 0.0);

                for (unsigned int k = 0; k < _cache.vReturnedMatrices[iter.first].size(); k++)
                {
                    if (_cache.vReturnedMatrices[iter.first][0].size() <= i)
                        (*currentVec)[k] = 0.0;
                    else
                        (*currentVec)[k] = _cache.vReturnedMatrices[iter.first][k][i];
                }
            }

            _parser.UpdateVectorVar(iter.second);
        }

        // Evaluate
#warning TODO (numere#6#03/04/22): Needs parallelization for reasonable performance at this position.
        v = _parser.Eval(nResults);

        // Append the vector to the target matrix
        if (nResults > _mTarget.size())
            _mTarget.resize(nResults, std::vector<mu::value_type>(nColCount, 0.0));

        for (int n = 0; n < nResults; n++)
        {
            _mTarget[n][i] = v[n];
        }
    }

    return _mTarget;
}


/////////////////////////////////////////////////
/// \brief This static function will search for
/// the position of the next (left-hand) matrix
/// multiplication operator.
///
/// \param sCmd const string&
/// \param nLastPos size_t
/// \return size_t
///
/////////////////////////////////////////////////
static size_t getPrevMatMultiOp(const string& sCmd, size_t nLastPos)
{
    int nQuotes = 0;

    for (int i = nLastPos; i >= 0; i--)
    {
        if (sCmd[i] == '(' || sCmd[i] == '{')
            nQuotes++;

        if (sCmd[i] == ')' || sCmd[i] == '}')
            nQuotes--;

        if (!(nQuotes%2) && sCmd.substr(i,2) == "**")
            return i;
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief This static function performs the
/// multiplication of two matrices.
///
/// \param _mLeft const Matrix&
/// \param _mRight const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix multiplyMatrices(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mLeft.size() || !_mLeft[0].size() || !_mRight.size() || !_mRight[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mResult;
    vector<mu::value_type> vLine;
    mu::value_type dEntry = 0.0;

    if (_mRight.size() == 1 && _mRight[0].size() && _mLeft.size() == 1 && _mLeft[0].size() == _mRight[0].size())
    {
        for (unsigned int i = 0; i < _mLeft[0].size(); i++)
        {
            dEntry += _mLeft[0][i]*_mRight[0][i];
        }

        vLine.push_back(dEntry);
        _mResult.push_back(vLine);

        return  _mResult;
    }

    if (_mRight[0].size() == 1 && _mRight.size() && _mLeft[0].size() == 1 && _mLeft.size() == _mRight.size())
    {
        for (unsigned int i = 0; i < _mLeft.size(); i++)
        {
            dEntry += _mLeft[i][0]*_mRight[i][0];
        }

        vLine.push_back(dEntry);
        _mResult.push_back(vLine);

        return  _mResult;
    }

    if (_mRight.size() != _mLeft[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mLeft.size()) +"x"+ toString(_mLeft[0].size()) +" vs. "+ toString(_mRight.size()) +"x"+ toString(_mRight[0].size()));

    _mResult = createZeroesMatrix(_mLeft.size(), _mRight[0].size());

    #pragma omp parallel for
    for (unsigned int i = 0; i < _mLeft.size(); i++)
    {
        for (unsigned int j = 0; j < _mRight[0].size(); j++)
        {
            for (unsigned int k = 0; k < _mRight.size(); k++)
            {
                _mResult[i][j] += _mLeft[i][k]*_mRight[k][j];
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function will transpose
/// the passed matrix (exchange rows with
/// columns).
///
/// \param _mMatrix const Matrix&
/// \return Matrix
///
/////////////////////////////////////////////////
Matrix transposeMatrix(const Matrix& _mMatrix)
{
    return transposeMatrix(MatFuncData(_mMatrix), MatFuncErrorInfo("", "", SyntaxError::invalid_position));
}


/////////////////////////////////////////////////
/// \brief This function returns a matrix filled
/// with zeros with the defined lines and columns.
///
/// \param nLines unsigned int
/// \param nCols unsigned int
/// \return Matrix
///
/// This function is not static, because it is
/// used from somewhere else.
/////////////////////////////////////////////////
Matrix createZeroesMatrix(unsigned int nLines, unsigned int nCols)
{
    if (!nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, "", SyntaxError::invalid_position);

    return createFilledMatrix(nLines, nCols, 0.0);
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed columns.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix createMatFromCols(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    return transposeMatrix(createMatFromLines(sCmd, _parser, _data, _functions, _option));
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed columns.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/// Missing elements are filled up logically.
/////////////////////////////////////////////////
static Matrix createMatFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    return transposeMatrix(createMatFromLinesFilled(sCmd, _parser, _data, _functions, _option));
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed lines.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix createMatFromLines(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _matfl;
    value_type* v = 0;
    int nResults = 0;
    unsigned int nLineLength = 0;
    vector<mu::value_type> vLine;
    if (!sCmd.length())
    {
        _matfl.push_back(vector<mu::value_type>(1,NAN));
    }
    if (!_functions.call(sCmd))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);
    if (_data.containsTablesOrClusters(sCmd))
    {
        getDataElements(sCmd, _parser, _data, _option);
    }
    while (sCmd.length())
    {
        if (!getNextArgument(sCmd, false).length())
            break;
        _parser.SetExpr(getNextArgument(sCmd, true));
        v = _parser.Eval(nResults);
        if ((unsigned)nResults > nLineLength)
            nLineLength = (unsigned)nResults;
        for (int n = 0; n < nResults; n++)
            vLine.push_back(v[n]);
        _matfl.push_back(vLine);
        vLine.clear();
    }
    if (!_matfl.size())
    {
        _matfl.push_back(vector<mu::value_type>(1,NAN));
    }

    // Groesse ggf. korrigieren

    for (unsigned int i = 0; i < _matfl.size(); i++)
    {
        _matfl[i].resize(nLineLength, 0.0);
    }

    return _matfl;
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed lines.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/// Missing elements are filled up logically.
/////////////////////////////////////////////////
static Matrix createMatFromLinesFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _matfl;
    value_type* v = 0;
    int nResults = 0;
    unsigned int nLineLength = 0;
    vector<mu::value_type> vLine;
    if (!sCmd.length())
    {
        _matfl.push_back(vector<mu::value_type>(1,NAN));
    }
    if (!_functions.call(sCmd))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);
    if (_data.containsTablesOrClusters(sCmd))
    {
        getDataElements(sCmd, _parser, _data, _option);
    }
    while (sCmd.length())
    {
        if (!getNextArgument(sCmd, false).length())
            break;
        _parser.SetExpr(getNextArgument(sCmd, true));
        v = _parser.Eval(nResults);
        if ((unsigned)nResults > nLineLength)
            nLineLength = (unsigned)nResults;
        for (int n = 0; n < nResults; n++)
            vLine.push_back(v[n]);
        _matfl.push_back(vLine);
        vLine.clear();
    }
    if (!_matfl.size())
    {
        _matfl.push_back(vector<mu::value_type>(1,NAN));
    }

    // Groesse entsprechend der Logik korrigieren

    for (unsigned int i = 0; i < _matfl.size(); i++)
    {
        if (_matfl[i].size() == 1)
        {
            // nur ein Element: wiederholen
            _matfl[i].resize(nLineLength, _matfl[i][0]);
        }
        else
        {
            vector<mu::value_type> vDeltas = calcDeltasForMatFill(_matfl, i);
            while (_matfl[i].size() < nLineLength)
            {
                _matfl[i].push_back(_matfl[i].back() + vDeltas[(_matfl[i].size()+1) % vDeltas.size()]);
            }
        }
    }

    return _matfl;
}


/////////////////////////////////////////////////
/// \brief This static function is used to
/// calculate the differences between consecutive
/// matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param nLine unsigned int
/// \return vector<mu::value_type>
///
/// This function is used by the \c matf*f()
/// functions to derive the filling logic.
/////////////////////////////////////////////////
static vector<mu::value_type> calcDeltasForMatFill(const Matrix& _mMatrix, unsigned int nLine)
{
    vector<mu::value_type> vDeltas;
    for (unsigned int j = 1; j < _mMatrix[nLine].size(); j++)
    {
        vDeltas.push_back(_mMatrix[nLine][j]-_mMatrix[nLine][j-1]);
    }
    return vDeltas;
}


/////////////////////////////////////////////////
/// \brief This static function extracts parts of
/// the passed matrix based upon the passed index
/// equations.
///
/// \param sExpr string&
/// \param _mMatrix const Matrix&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _mReturn;
    Indices _idx = getIndices(sExpr, _mMatrix, _parser, _data, _option);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _mMatrix.size()-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _mMatrix[0].size()-1);

    _mReturn = createZeroesMatrix(_idx.row.size(), _idx.col.size());

    for (unsigned int i = 0; i < _idx.row.size(); i++)
    {
        for (unsigned int j = 0; j < _idx.col.size(); j++)
        {
            if (_idx.row[i] >= (int)_mMatrix.size() || _idx.col[j] >= (int)_mMatrix[0].size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

            _mReturn[i][j] = _mMatrix[_idx.row[i]][_idx.col[j]];
        }
    }

    return _mReturn;
}


static std::string formatMatrixRow(const Matrix& _mResult, const Settings& _option, size_t row)
{
    const size_t FIELDLENGTH = 21;
    const size_t FIELDLENGTH_W_FILLER = 23;
    const size_t PRECISION = 7;

    std::string sRow;

    for (unsigned int col = 0; col < _mResult[row].size(); col++)
    {
        if (_mResult[row].size() > (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER
            && (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 == col)
        {
            sRow += strfill("..., ", FIELDLENGTH_W_FILLER);
            col = _mResult[row].size() - (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 - 1;
            continue;
        }

        sRow += strfill(toString(_mResult[row][col], PRECISION), FIELDLENGTH);

        if (col+1 < _mResult[row].size())
            sRow += ", ";
    }

    return sRow;
}


/////////////////////////////////////////////////
/// \brief This static function formats and
/// prints the calculated matrix to the terminal.
///
/// \param _mResult const Matrix&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void showMatrixResult(const Matrix& _mResult, const Settings& _option)
{
    if (!_option.systemPrints() || NumeReKernel::bSupressAnswer)
        return;

    const size_t FIELDLENGTH = 21;
    const size_t FIELDLENGTH_W_FILLER = 23;

    NumeReKernel::toggleTableStatus();

    if (_mResult.size() > 10)
    {
        for (unsigned int i = 0; i < _mResult.size(); i++)
        {
            if (!i)
                NumeReKernel::printPreFmt("|   /");
            else if (i+1 == _mResult.size())
                NumeReKernel::printPreFmt("|   \\");
            else if (i == 5)
                NumeReKernel::printPreFmt("|-> |");
            else
                NumeReKernel::printPreFmt("|   |");

            if (i == 5)
            {
                for (unsigned int j = 0; j < _mResult[0].size(); j++)
                {
                    if (_mResult[0].size() > (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER
                        && (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 == j)
                    {
                        NumeReKernel::printPreFmt(strfill("..., ", FIELDLENGTH_W_FILLER));
                        j = _mResult[0].size() - (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 - 1;
                        continue;
                    }

                    NumeReKernel::printPreFmt(strfill("...", FIELDLENGTH));

                    if (j+1 < _mResult[0].size())
                        NumeReKernel::printPreFmt(", ");
                }

                i = _mResult.size()-6;
            }
            else
                NumeReKernel::printPreFmt(formatMatrixRow(_mResult, _option, i));

            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.size())
                NumeReKernel::printPreFmt(" /\n");
            else
                NumeReKernel::printPreFmt(" |\n");
        }
    }
    else if (_mResult.size() == 1)
    {
        if (_mResult[0].size() == 1)
            NumeReKernel::print("(" + toString(_mResult[0][0], _option.getPrecision()) + ")");
        else
            NumeReKernel::print("(" + formatMatrixRow(_mResult, _option, 0) + " )");
    }
    else
    {
        for (unsigned int i = 0; i < _mResult.size(); i++)
        {
            if (!i && _mResult.size() == 2)
                NumeReKernel::printPreFmt("|-> /");
            else if (!i)
                NumeReKernel::printPreFmt("|   /");
            else if (i+1 == _mResult.size())
                NumeReKernel::printPreFmt("|   \\");
            else if (i == (_mResult.size()-1)/2)
                NumeReKernel::printPreFmt("|-> |");
            else
                NumeReKernel::printPreFmt("|   |");

            NumeReKernel::printPreFmt(formatMatrixRow(_mResult, _option, i));

            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.size())
                NumeReKernel::printPreFmt(" /\n");
            else
                NumeReKernel::printPreFmt(" |\n");
        }
    }

    NumeReKernel::flush();
    NumeReKernel::toggleTableStatus();
    return;
}


/////////////////////////////////////////////////
/// \brief This function creates an Indices
/// object, which is filled with the indices
/// specified by the index equation.
///
/// \param sCmd const string&
/// \param _mMatrix const Matrix&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return Indices
///
/////////////////////////////////////////////////
Indices getIndices(const string& sCmd, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    Indices _idx;
    string sI[2] = {"<<NONE>>", "<<NONE>>"};
    string sJ[2] = {"<<NONE>>", "<<NONE>>"};
    string sArgument = "";
    unsigned int nPos = 0;
    int nParenthesis = 0;
    value_type* v = 0;
    int nResults = 0;

    if (sCmd.find('(') == string::npos)
        return _idx;

    nPos = sCmd.find('(');

    for (unsigned int n = nPos; n < sCmd.length(); n++)
    {
        if (sCmd[n] == '(')
            nParenthesis++;

        if (sCmd[n] == ')')
            nParenthesis--;

        if (!nParenthesis)
        {
            sArgument = sCmd.substr(nPos+1, n-nPos-1);
            break;
        }
    }

    StripSpaces(sArgument);

    if (_data.containsTablesOrClusters(sArgument))
        getDataElements(sArgument, _parser, _data, _option);

    // --> Kurzschreibweise!
    if (!sArgument.length())
    {
        _idx.row = VectorIndex(0LL, _mMatrix.size());
        _idx.col = VectorIndex(0LL, _mMatrix[0].size());
        return _idx;
    }

    if (sArgument.find(',') != string::npos)
    {
        nParenthesis = 0;
        nPos = 0;

        for (unsigned int n = 0; n < sArgument.length(); n++)
        {
            if (sArgument[n] == '(' || sArgument[n] == '{')
                nParenthesis++;

            if (sArgument[n] == ')' || sArgument[n] == '}')
                nParenthesis--;

            if (sArgument[n] == ':' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else if (n == nPos)
                {
                    sJ[0] = "<<EMPTY>>";
                }
                else
                {
                    sJ[0] = sArgument.substr(nPos, n-nPos);
                }

                nPos = n+1;
            }

            if (sArgument[n] == ',' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else
                {
                    if (n == nPos)
                        sI[1] = "<<EMPTY>>";
                    else
                        sI[1] = sArgument.substr(nPos, n - nPos);
                }

                nPos = n+1;
            }
        }

        if (sJ[0] == "<<NONE>>")
        {
            if (nPos < sArgument.length())
                sJ[0] = sArgument.substr(nPos);
            else
                sJ[0] = "<<EMPTY>>";
        }
        else if (nPos < sArgument.length())
            sJ[1] = sArgument.substr(nPos);
        else
            sJ[1] = "<<EMPTY>>";

        // --> Vektor prüfen <--
        if (sI[0] != "<<NONE>>" && sI[1] == "<<NONE>>")
        {
            _parser.SetExpr(sI[0]);
            v = _parser.Eval(nResults);

            if (nResults > 1)
            {
                _idx.row = VectorIndex(v, nResults, 0);
            }
            else
                _idx.row.front() = intCast(v[0])-1;
        }

        if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
        {
            _parser.SetExpr(sJ[0]);
            v = _parser.Eval(nResults);

            if (nResults > 1)
            {
                _idx.col = VectorIndex(v, nResults, 0);
            }
            else
                _idx.col.front() = intCast(v[0])-1;
        }

        for (int n = 0; n < 2; n++)
        {
            if (sI[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.row.back() = _mMatrix.size()-1;
                else
                    _idx.row.front() = 0;
            }
            else if (sI[n] != "<<NONE>>")
            {
                _parser.SetExpr(sI[n]);
                _idx.row.setIndex(n, intCast(_parser.Eval())-1);

                if (isnan(_parser.Eval().real()) || isinf(_parser.Eval().real()) || _parser.Eval().real() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position, sI[n]);
            }

            if (sJ[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.col.back() = _mMatrix[0].size()-1;
                else
                    _idx.col.front() = 0;
            }
            else if (sJ[n] != "<<NONE>>")
            {
                _parser.SetExpr(sJ[n]);
                _idx.col.setIndex(n, intCast(_parser.Eval())-1);

                if (isnan(_parser.Eval().real()) || isinf(_parser.Eval().real()) || _parser.Eval().real() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position, sJ[n]);
            }
        }
    }

    return _idx;
}


/////////////////////////////////////////////////
/// \brief Simple wrapper for the function
/// parser_ShowMatrixResult() to be accessible
/// from the outside.
///
/// \param _mMatrix const vector<vector<mu::value_type>>&
/// \return void
///
/////////////////////////////////////////////////
void showMatrix(const vector<vector<mu::value_type> >& _mMatrix)
{
    showMatrixResult(_mMatrix, NumeReKernel::getInstance()->getSettings());
}

/////////////////////////////////////////////////
/// \brief Static helper function for
/// parser_getIndicesForMatrix(), which will
/// handle the return values of matrix evaluations.
///
/// \param _sCmd const string&
/// \param vReturnedMatrices const vector<Matrix>&
/// \param _parser Parser&
/// \return void
///
/////////////////////////////////////////////////
static void parser_declareMatrixReturnValuesForIndices(const string& _sCmd, const vector<Matrix>& vReturnedMatrices, Parser& _parser)
{
    for (unsigned int j = 0; j < vReturnedMatrices.size(); j++)
    {
        vector<mu::value_type> v;
        if (vReturnedMatrices[j].size() == 1 && vReturnedMatrices[j][0].size() == 1)
        {
            v.push_back(vReturnedMatrices[j][0][0]);
        }
        else if (vReturnedMatrices[j].size() == 1 && vReturnedMatrices[j][0].size() > 1)
        {
            v = vReturnedMatrices[j][0];
        }
        else
        {
            for (size_t i = 0; i < vReturnedMatrices[j].size(); i++)
                v.push_back(vReturnedMatrices[j][i][0]);
        }

        // Declare the corresponding vector variable
        if (_sCmd.find("_~returnedMatrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("_~returnedMatrix["+toString((int)j)+"]", v);
    }
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// parser_getIndicesForMatrix(), which will
/// handle the indices of already parsed datafile
/// matrices.
///
/// \param _sCmd string&
/// \param _cache const MatOpCache&
/// \param Parser&_parser
/// \param _data Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void parser_declareDataMatrixValuesForIndices(string& _sCmd, const MatOpCache& _cache, Parser&_parser, MemoryManager& _data)
{
    for (unsigned int j = 0; j < _cache.vDataAccesses.size(); j++)
    {
        vector<mu::value_type> v;
        const MatOpCache::MatOpDataAccess& _access = _cache.vDataAccesses[j];

        // Get the values using the indices
        if (_access.idx.row.size() > _access.idx.col.size())
            v = _data.getElement(_access.idx.row, VectorIndex(_access.idx.col[0]), _access.sName);
        else
            v = _data.getElement(VectorIndex(_access.idx.row[0]), _access.idx.col, _access.sName);

        // Declare the corresponding vector variable
        if (_sCmd.find("_~matrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("_~matrix["+toString((int)j)+"]", v);
    }
}


/////////////////////////////////////////////////
/// \brief Static wrapper function for resolving
/// already parsed datafile matrix elements and
/// evaluated matrix expressions, which are used
/// as indices for datafile matrices.
///
/// \param sCmd const string&
/// \param _cache const MatOpCache&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return Indices
///
/////////////////////////////////////////////////
static Indices getIndicesForMatrix(const string& sCmd, const MatOpCache& _cache, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    string _sCmd = sCmd;

    // Declare the return values of the former matrix calculations
    // to the parser by extracting the values and creating a vector
    // variable
    parser_declareMatrixReturnValuesForIndices(_sCmd, _cache.vReturnedMatrices, _parser);

    // Declare the already parsed data object matrices in the
    // current expressions by parsing their indices, extracting
    // the corresponding values and creating a vector variable
    parser_declareDataMatrixValuesForIndices(_sCmd, _cache, _parser, _data);

    // Return the calculated indices
    return getIndices(_sCmd, _parser, _data, _option);
}


/////////////////////////////////////////////////
/// \brief This static function evaluates,
/// whether there are matrix expressions in the
/// passed expression.
///
/// \param sExpr const string&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
static bool containsMatrices(const string& sExpr, MemoryManager& _data)
{
    if (_data.containsTablesOrClusters(sExpr) || sExpr.find('{') != string::npos)
        return true;

    static map<string, MatFuncDef> mMatrixFunctionsMap = getMatrixFunctions();

    // search for any matrix function
    for (auto iter = mMatrixFunctionsMap.begin(); iter != mMatrixFunctionsMap.end(); ++iter)
    {
        if (!iter->second.isPureMatFunc)
            continue;

        size_t match = sExpr.find(iter->first+"(");

        if (match != string::npos && (!match || (!isalpha(sExpr[match-1]) && sExpr[match-1] != '_')))
            return true;
    }

    return false;
}

