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

using namespace std;

static Matrix evalMatOp(string& sCmd, Parser& _parser, MemoryManager& _data, MatOpCache& _cache);
static size_t getPrevMatMultiOp(StringView sCmd, size_t nLastPos);
static Matrix multiplyMatrices(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position);
static Matrix getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data);
static vector<std::complex<double>> calcDeltasForMatFill(const std::vector<std::vector<std::complex<double>>>& _mMatrix, size_t nLine);
static void showMatrixResult(const Matrix& _mResult, const Settings& _option);
static Indices getIndicesForMatrix(StringView sCmd, const MatOpCache& _cache, Parser& _parser, MemoryManager& _data);
static bool containsMatrices(const string& sExpr, MemoryManager& _data);

Matrix transposeMatrix(const Matrix& _mMatrix);
Matrix createZeroesMatrix(size_t nLines, size_t nCols);

static Matrix createMatFromCols(string& sCmd, Parser& _parser, MemoryManager& _data);
static Matrix createMatFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data);
static Matrix createMatFromLines(string& sCmd, Parser& _parser, MemoryManager& _data);
static Matrix createMatFromLinesFilled(string& sCmd, Parser& _parser, MemoryManager& _data);





/////////////////////////////////////////////////
/// \brief This function is the main interface to
/// the matrix operations.
///
/// \param sCmd std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool performMatrixOperation(std::string& sCmd, mu::Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
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
            getIndices(sTargetName, _idx, _parser, _data, true);

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
    Matrix _mResult = evalMatOp(sCmd, _parser, _data, _cache);
    g_logger.debug("Stop evalMatOp.");
    _assertionHandler.checkAssertion(_mResult);

    // Safety check
    if (_mResult.isEmpty())
        return false;

    // Target in Zielmatrix speichern
    if (!isCluster)
    {
        // This target is a table
        if (bAllowMatrixClearing)
            _data.deleteBulk(sTargetName, 0, _data.getLines(sTargetName, false) - 1, 0, _data.getCols(sTargetName, false) - 1);

        // Prepare the target size
        _data.resizeTable(_idx.col.front()+_mResult.cols(), sTargetName);
        Memory* _table = _data.getTable(sTargetName);

        int maxrow = _idx.row.subidx(0, _mResult.rows()).max();

        for (size_t j = 0; j < _mResult.cols(); j++)
        {
            if (_idx.col[j] == VectorIndex::INVALID)
                break;

            _table->writeData(maxrow, _idx.col[j], mu::Value(0.0, false));
        }

        // Write the contents to the table
        #pragma omp parallel for
        for (size_t j = 0; j < _mResult.cols(); j++)
        {
            if (_idx.col[j] == VectorIndex::INVALID)
                continue;

            for (size_t i = 0; i < _mResult.rows(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                _table->writeDataDirectUnsafe(_idx.row[i], _idx.col[j], _mResult(i, j));
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
        if (_mResult.rows() == 1)
        {
            // Assign the first line
            for (size_t i = 0; i < _mResult.cols(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                cluster.setDouble(_idx.row[i], _mResult(0, i));
            }
        }
        else
        {
            // Assign the first column
            for (size_t i = 0; i < _mResult.rows(); i++)
            {
                if (_idx.row[i] == VectorIndex::INVALID)
                    break;

                cluster.setDouble(_idx.row[i], _mResult(i));
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
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix evalMatOp(string& sCmd, Parser& _parser, MemoryManager& _data, MatOpCache& _cache)
{
    string __sCmd;
    size_t pos_back = 0;
    size_t iter_start = 0;

    static std::map<std::string, MatFuncDef> mMatrixFunctions = getMatrixFunctions();

    // Check, whether there's a target vector available in this expression part
    size_t nEqPos = sCmd.find('=');

    if (nEqPos != string::npos
        && nEqPos
        && sCmd[nEqPos+1] != '='
        && sCmd[nEqPos-1] != '!'
        && sCmd[nEqPos-1] != '<'
        && sCmd[nEqPos-1] != '>'
        && StringView(sCmd, 0, nEqPos).find('{') != string::npos)
    {
        iter_start = nEqPos+1;
    }

    StringView cmd(sCmd);

    // Apply all known and requested matrix functions.
    // The equation will be segmentized and the evaluated
    // part will be appended to the evaluated equation.
    for (size_t i = iter_start; i < sCmd.length(); i++)
    {
        size_t nMatchingParens = 0;
        // Is the current character alphabetical and
        // either the first or prefixed with a delimiter?
        // Then this is a possible start for a function
        if ((!i || isDelimiter(sCmd[i-1])) && (isalpha(sCmd[i]) || sCmd[i] == '{'))
        {
            for (auto fIter = mMatrixFunctions.begin(); fIter != mMatrixFunctions.end(); ++fIter)
            {
                if (fIter->second.signature == MATSIG_INVALID)
                    continue;

                if (cmd.match(fIter->first+"(", i)
                    && (nMatchingParens = getMatchingParenthesis(cmd.subview(i+fIter->first.length()))) != string::npos)
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
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sSubExpr, _parser, _data, _cache)),
                                                                                  errorInfo));
                            break;
                        }
                        case MATSIG_MAT_NOPT:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            int n = 0;

                            if (sSubExpr.length())
                            {
                                Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                                n = intCast(mres.data().front());
                            }

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _cache), n),
                                                                                  errorInfo));
                            break;
                        }
                        case MATSIG_MAT_MAT:
                        {
                            std::string sMatrix1 = getNextArgument(sSubExpr, true);
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix1, _parser, _data, _cache),
                                                                                              evalMatOp(sSubExpr, _parser, _data, _cache)),
                                                                                  errorInfo));
                            break;
                        }
                        case MATSIG_MAT_MAT_MAT:
                        {
                            std::string sMatrix1 = getNextArgument(sSubExpr, true);
                            std::string sMatrix2 = getNextArgument(sSubExpr, true);
                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix1, _parser, _data, _cache),
                                                                                              evalMatOp(sMatrix2, _parser, _data, _cache),
                                                                                              evalMatOp(sSubExpr, _parser, _data, _cache)),
                                                                                  errorInfo));
                            break;
                        }
                        case MATSIG_MAT_MAT_N:
                        {
                            std::string sMatrix1 = getNextArgument(sSubExpr, true);
                            std::string sMatrix2 = getNextArgument(sSubExpr, true);
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                            int n = intCast(mres.data().front());

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix1, _parser, _data, _cache),
                                                                                              evalMatOp(sMatrix2, _parser, _data, _cache), n),
                                                                                  errorInfo));
                            break;
                        }
                        case MATSIG_MAT_F:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                            std::complex<double> fVal = mres.data().front();

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _cache),
                                                                                              fVal), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_F_N:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                            std::complex<double> fVal = mres.data().front();
                            int n = 0;

                            if (mres.data().size() > 1)
                                n = intCast(mres.data()[1]);

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _cache),
                                                                                              fVal, n), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_N_MAUTO:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                            int n = intCast(mres.data().front());
                            int m = intCast(mres.data().front());

                            if (mres.data().size() > 1)
                                m = intCast(mres.data()[1]);

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _cache),
                                                                                              n, m), errorInfo));
                            break;
                        }
                        case MATSIG_MAT_N_MOPT:
                        {
                            std::string sMatrix = getNextArgument(sSubExpr, true);
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);
                            int n = intCast(mres.data().front());
                            int m = 0;

                            if (mres.data().size() > 1)
                                m = intCast(mres.data()[1]);

                            _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(evalMatOp(sMatrix, _parser, _data, _cache),
                                                                                              n, m), errorInfo));
                            break;
                        }
                        case MATSIG_N_MAUTO:
                        {
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);

                            if (mres.data().size() > 1)
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(mres.data()[0]),
                                                                                                  intCast(mres.data()[1])), errorInfo));
                            else
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(mres.data()[0]),
                                                                                                  intCast(mres.data()[0])), errorInfo));

                            break;
                        }
                        case MATSIG_N_MOPT:
                        {
                            Matrix mres = evalMatOp(sSubExpr, _parser, _data, _cache);

                            if (mres.data().size() > 1)
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(mres.data()[0]),
                                                                                                  intCast(mres.data()[1])), errorInfo));
                            else
                                _cache.vReturnedMatrices.push_back(fIter->second.func(MatFuncData(intCast(mres.data()[0])), errorInfo));

                            break;
                        }
                    }

                    // Prepare leading part of the equation
                    __sCmd.append(sCmd, pos_back, i-pos_back);

                    // Store the last position and advance the
                    // current position
                    pos_back = i+nMatchingParens+fIter->first.length()+1;
                    i = pos_back-1;

                    // append the returned matrix
                    __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                }
            }

            // construct matrix from columns -> MAT(...)
            if (cmd.match("matfc(", i)
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i+5))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+6, nMatchingParens-1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+6;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from lines -> MAT(...)
            if (cmd.match("matfl(", i)
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i+5))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+6, nMatchingParens-1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromLines(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+6;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from columns and fill missing values -> MAT(...)
            if (cmd.match("matfcf(", i)
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i+6))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+7, nMatchingParens-1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromColsFilled(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+7;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // construct matrix from lines and fill missing values -> MAT(...)
            if (cmd.match("matflf(", i)
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i+6))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+7, nMatchingParens-1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromLinesFilled(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+7;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // Handle old vector syntax (will most probably be changed to matrix syntax)
            if (cmd.match("{{", i)
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i+1, nMatchingParens-1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+1;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }

            // Handle vectors
            if (sCmd[i] == '{'
                && (nMatchingParens = getMatchingParenthesis(cmd.subview(i))) != string::npos)
            {
                string sSubExpr = sCmd.substr(i, nMatchingParens+1);
                __sCmd.append(sCmd, pos_back, i-pos_back);
                _cache.vReturnedMatrices.push_back(createMatFromCols(sSubExpr, _parser, _data));
                pos_back = i+nMatchingParens+1;
                __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                i = pos_back-1;
            }
        }

        // Handle index selectors after a matrix function
        if (i > 14
            && sCmd[i] == '('
            && i == pos_back // only true, if the last iteration evaluated a subexpression
            && sCmd.find_last_not_of(' ',i-1) != string::npos
            && (__sCmd.back() == ']' || __sCmd.back() == ')')) //...returnedMatrix[N](:,:)
        {
            int nMatrix = 0;
            nMatrix = StrToInt(__sCmd.substr(__sCmd.rfind('[')+1, __sCmd.rfind(']')-__sCmd.rfind('[')-1));
            if (__sCmd.substr(__sCmd.rfind('[')-16,17) == "_~returnedMatrix[")
            {
                nMatchingParens = getMatchingParenthesis(cmd.subview(i));
                string sSubExpr = sCmd.substr(i, nMatchingParens+1);
                pos_back = i+nMatchingParens+1;
                _cache.vReturnedMatrices[nMatrix] = getMatrixElements(sSubExpr, _cache.vReturnedMatrices[nMatrix], _parser, _data);
                i = pos_back-1;
            }
        }

        // Pre-evaluate parentheses
        if (sCmd[i] == '(')
        {
            nMatchingParens = getMatchingParenthesis(cmd.subview(i));
            //size_t nLastDelimiter;

            if (sCmd.substr(i, nMatchingParens).find("**") != string::npos
                || (sCmd.length() > i+nMatchingParens+1 && sCmd[i+nMatchingParens+1] == '('))
                // Removed this due to unnecessary double evaluation (might be necessary tough!)
                /*|| (i > 1
                    && (nLastDelimiter = sCmd.find_last_of(" /+-*!^%&|#(){}?:,<>=", i-1)) != std::string::npos
                    && !_data.isTable(sCmd.substr(nLastDelimiter+1, i-nLastDelimiter-1))))*/
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
                                __sCmd.append(sCmd, pos_back, j-pos_back);
                                sSubExpr = sCmd.substr(j, closing_par_pos+1 - j);
                                break;
                            }
                        }
                    }
                    else
                        __sCmd.append(sCmd, pos_back, i-pos_back);

                    _cache.vReturnedMatrices.push_back(evalMatOp(sSubExpr, _parser, _data, _cache));
                    __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                }
                else
                {
                    __sCmd.append(sCmd, pos_back, i-pos_back);
                    __sCmd += "(";

                    // As this might be a usual function, evaluate each argument
                    // separately (if it is matrix expression) and combine them
                    // afterwards
                    while (sSubExpr.length())
                    {
                        string sExpr = getNextArgument(sSubExpr, true);

                        if (containsMatrices(sExpr, _data))
                        {
                            _cache.vReturnedMatrices.push_back(evalMatOp(sExpr, _parser, _data, _cache));
                            __sCmd += "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]";
                        }
                        else
                            __sCmd += sExpr;

                        if (sSubExpr.length())
                            __sCmd += ",";
                    }

                    __sCmd += ")";
                }

                pos_back = closing_par_pos+1;
                i = pos_back-1;
            }
        }
    }

    // Append the missing part of the equation
    if (pos_back < sCmd.length())
        __sCmd.append(sCmd, pos_back);

    size_t nPos = 0;

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
            _access.idx = getIndicesForMatrix(StringView(__sCmd, nPos), _cache, _parser, _data);

            // Evaluate the indices
            if (!evaluateIndices(iter->first, _access.idx, _data))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, nPos);

            // Store the name of the current data object
            _access.sName = iter->first;

            // Identify, which value to use for a missing value
            if (addMissingVectorComponent("", __sCmd.substr(0,nPos), __sCmd.substr(nPos+1+(iter->first).length()+getMatchingParenthesis(StringView(__sCmd, nPos+(iter->first).length()))),false) == "0")
                _access.missingValues = 0;
            else
                _access.missingValues = 1;

            // Add the cached access
            _cache.vDataAccesses.push_back(_access);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(StringView(__sCmd, nPos+(iter->first).length()))+(iter->first).length()+1, "_~matrix["+toString(_cache.vDataAccesses.size()-1)+"]");
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
            Indices _idx = getIndicesForMatrix(StringView(__sCmd, nPos), _cache, _parser, _data);

            if (_idx.row.isOpenEnd())
                _idx.row.setRange(0, iter->second.size()-1);

            // Prepare a target matrix
            Matrix _mClusterMatrix = createZeroesMatrix(_idx.row.size(), 1);

            // Write the contents to the matrix
            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                _mClusterMatrix(i) = iter->second.getDouble(_idx.row[i]);
            }

            // Declare the cluster as a returned matrix (simplifies the
            // access logic further down)
            _cache.vReturnedMatrices.push_back(_mClusterMatrix);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(StringView(__sCmd, nPos+(iter->first).length()))+(iter->first).length()+1,
                           "_~returnedMatrix["+toString(_cache.vReturnedMatrices.size()-1)+"]");
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
            if (StringView(__sCmd, n, 2) == "**")
            {
                // Find the last operator
                pos_back = getPrevMatMultiOp(sCmd, pos_back)-1;
                Matrix _mLeft;
                Matrix _mRight;
                size_t nPositions[2];
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
                        _mRight = evalMatOp(sSubExpr, _parser, _data, _cache);
                        g_logger.debug("_mRight.size() = " + _mRight.printDims());
                        break;
                    }
                }

                if (_mRight.isEmpty())
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
                        _mLeft = evalMatOp(sSubExpr, _parser, _data, _cache);
                        g_logger.debug("_mLeft.size() = " + _mLeft.printDims());
                        break;
                    }
                }

                if (_mLeft.isEmpty())
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                // Perform the actual matrix multiplication
                _cache.vReturnedMatrices.push_back(multiplyMatrices(_mLeft, _mRight, sCmd, "", pos_back+1));

                // Replace the current multiplication with its return value
                __sCmd.replace(nPositions[0], nPositions[1]-nPositions[0],
                               "_~returnedMatrix[" + toString(_cache.vReturnedMatrices.size()-1)+"]");
                n = nPositions[0];
            }
        }
    }

    sCmd = __sCmd;

    std::map<size_t, std::string> mDataMatrices;
    std::map<size_t, std::string> mReturnedMatrices;
    size_t nColCount = 1; // Minimal size of a matrix
    size_t nRowCount = 1; // Minimal size of a matrix

    // We need the maximal number of columns for the regular
    // evaluation of the expression
    //
    // Examine the memory accesses, which are still available
    // in the expression
    for (size_t i = 0; i < _cache.vDataAccesses.size(); i++)
    {
        std::string sMatrixName = "_~matrix["+toString(i)+"]";

        if (sCmd.find(sMatrixName) == string::npos)
            continue;

        mDataMatrices[i] = sMatrixName;

        // Get the column count from the dimensions of the indices
        nRowCount = std::max(nRowCount, _cache.vDataAccesses[i].idx.row.size());
        nColCount = std::max(nColCount, _cache.vDataAccesses[i].idx.col.size());
    }

    // Examine now the return values available in the expression
    for (size_t i = 0; i < _cache.vReturnedMatrices.size(); i++)
    {
        std::string sMatrixName = "_~returnedMatrix["+toString(i)+"]";

        if (sCmd.find(sMatrixName) == string::npos)
            continue;

        mReturnedMatrices[i] = sMatrixName;
        nRowCount = std::max(nRowCount, _cache.vReturnedMatrices[i].rows());
        nColCount = std::max(nColCount, _cache.vReturnedMatrices[i].cols());
    }

    // Shortcut => only a single matrix
    if (mDataMatrices.size() == 1 && isEqualStripped(sCmd, mDataMatrices.begin()->second))
    {
        auto iter = mDataMatrices.begin();
        return _data.getTable(_cache.vDataAccesses[iter->first].sName)->readMemAsMatrix(_cache.vDataAccesses[iter->first].idx.row,
                                                                                        _cache.vDataAccesses[iter->first].idx.col);
    }

    if (mReturnedMatrices.size() == 1 && isEqualStripped(sCmd, mReturnedMatrices.begin()->second))
        return _cache.vReturnedMatrices[mReturnedMatrices.begin()->first];

    // Read now the first column of every matrix in the expression
    // as vector for the parser
    //
    // start with the memory accesses
    for (const auto& iter : mDataMatrices)
    {
        // Get the values using the incdices
        const MatOpCache::MatOpDataAccess& _access = _cache.vDataAccesses[iter.first];
        Matrix mat(_data.getTable(_access.sName)->readMemAsMatrix(_access.idx.row, _access.idx.col));

        // Extend this matrix to have enough elements
        mat.extend(nRowCount, nColCount);

        // Declare the corresponding vector variable
        _parser.SetInternalVar(iter.second, mat.data());
    }

    // Get now the first columns of the returned values
    for (const auto& iter : mReturnedMatrices)
    {
        // Extend every matrix to have enough elements
        _cache.vReturnedMatrices[iter.first].extend(nRowCount, nColCount);

        // Declare the corresponding vector variable
        _parser.SetInternalVar(iter.second, _cache.vReturnedMatrices[iter.first].data());
    }

    // Set the expression in the parser
    _parser.SetExpr("{" + sCmd + "}");

    g_logger.debug("Matrix calculation");

    mu::Array v;

    // Evaluate the first columns
    v = _parser.Eval();

    // Create and return a matrix from the calculated
    // results
    return Matrix((v.size() > 1 && nRowCount == 1 && nColCount == 1) ? v.size() : nRowCount, // special case for multiple return values
                  nColCount,
                  v);
}


/////////////////////////////////////////////////
/// \brief This static function will search for
/// the position of the next (left-hand) matrix
/// multiplication operator.
///
/// \param sCmd StringView
/// \param nLastPos size_t
/// \return size_t
///
/////////////////////////////////////////////////
static size_t getPrevMatMultiOp(StringView sCmd, size_t nLastPos)
{
    int nQuotes = 0;

    for (int i = nLastPos; i >= 0; i--)
    {
        if (sCmd[i] == '(' || sCmd[i] == '{')
            nQuotes++;

        if (sCmd[i] == ')' || sCmd[i] == '}')
            nQuotes--;

        if (!(nQuotes%2) && sCmd.match("**", i))
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
    if (_mLeft.isEmpty() || _mRight.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mRight.rows() != _mLeft.cols())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position,
                          _mLeft.printDims() +" vs. "+ _mRight.printDims());

    return _mLeft * _mRight;
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
/// \param nLines size_t
/// \param nCols size_t
/// \return Matrix
///
/// This function is not static, because it is
/// used from somewhere else.
/////////////////////////////////////////////////
Matrix createZeroesMatrix(size_t nLines, size_t nCols)
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
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix createMatFromCols(string& sCmd, Parser& _parser, MemoryManager& _data)
{
    return transposeMatrix(createMatFromLines(sCmd, _parser, _data));
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed columns.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return Matrix
///
/// Missing elements are filled up logically.
/////////////////////////////////////////////////
static Matrix createMatFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data)
{
    return transposeMatrix(createMatFromLinesFilled(sCmd, _parser, _data));
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed lines.
///
/// \param sCmd std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix createMatFromLines(std::string& sCmd, mu::Parser& _parser, MemoryManager& _data)
{
    std::vector<std::vector<std::complex<double>>> _matfl;
    size_t nLineLength = 0;
    std::vector<std::complex<double>> vLine;

    if (!sCmd.length())
        _matfl.push_back(vector<std::complex<double>>(1,NAN));

    if (_data.containsTablesOrClusters(sCmd))
        getDataElements(sCmd, _parser, _data);

    while (sCmd.length())
    {
        if (!getNextArgument(sCmd, false).length())
            break;

        _parser.SetExpr(getNextArgument(sCmd, true));
        mu::Array v = _parser.Eval();

        if (v.size() > nLineLength)
            nLineLength = v.size();

        for (size_t n = 0; n < v.size(); n++)
            vLine.push_back(v[n].getNum().asCF64());

        _matfl.push_back(vLine);
        vLine.clear();
    }

    if (!_matfl.size())
        _matfl.push_back(vector<std::complex<double>>(1, NAN));

    // Groesse ggf. korrigieren
    for (size_t i = 0; i < _matfl.size(); i++)
    {
        _matfl[i].resize(nLineLength, 0.0);
    }

    return Matrix(_matfl);
}


/////////////////////////////////////////////////
/// \brief This static function creates a matrix
/// from the passed lines.
///
/// \param sCmd std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \return Matrix
///
/// Missing elements are filled up logically.
/////////////////////////////////////////////////
static Matrix createMatFromLinesFilled(std::string& sCmd, mu::Parser& _parser, MemoryManager& _data)
{
    std::vector<std::vector<std::complex<double>>> _matfl;
    size_t nLineLength = 0;
    vector<std::complex<double>> vLine;

    if (!sCmd.length())
        _matfl.push_back(vector<std::complex<double>>(1, NAN));

    if (_data.containsTablesOrClusters(sCmd))
        getDataElements(sCmd, _parser, _data);

    while (sCmd.length())
    {
        if (!getNextArgument(sCmd, false).length())
            break;

        _parser.SetExpr(getNextArgument(sCmd, true));
        mu::Array v = _parser.Eval();

        if (v.size() > nLineLength)
            nLineLength = v.size();

        for (size_t n = 0; n < v.size(); n++)
            vLine.push_back(v[n].getNum().asCF64());

        _matfl.push_back(vLine);
        vLine.clear();
    }

    if (!_matfl.size())
        _matfl.push_back(vector<std::complex<double>>(1, NAN));

    // Groesse entsprechend der Logik korrigieren
    for (size_t i = 0; i < _matfl.size(); i++)
    {
        if (_matfl[i].size() == 1)
        {
            // nur ein Element: wiederholen
            _matfl[i].resize(nLineLength, _matfl[i][0]);
        }
        else
        {
            std::vector<std::complex<double>> vDeltas = calcDeltasForMatFill(_matfl, i);

            while (_matfl[i].size() < nLineLength)
            {
                _matfl[i].push_back(_matfl[i].back() + vDeltas[(_matfl[i].size()+1) % vDeltas.size()]);
            }
        }
    }

    return Matrix(_matfl);
}


/////////////////////////////////////////////////
/// \brief This static function is used to
/// calculate the differences between consecutive
/// matrix elements.
///
/// \param _mMatrix const std::vector<std::vector<std::complex<double>>>&
/// \param nLine size_t
/// \return std::vector<std::complex<double>>
///
/// This function is used by the \c matf*f()
/// functions to derive the filling logic.
/////////////////////////////////////////////////
static std::vector<std::complex<double>> calcDeltasForMatFill(const std::vector<std::vector<std::complex<double>>>& _mMatrix, size_t nLine)
{
    std::vector<std::complex<double>> vDeltas;

    for (size_t j = 1; j < _mMatrix[nLine].size(); j++)
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
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data)
{
    Matrix _mReturn;
    Indices _idx = getIndices(sExpr, _mMatrix, _parser, _data);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _mMatrix.rows()-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _mMatrix.cols()-1);

    _mReturn = createZeroesMatrix(_idx.row.size(), _idx.col.size());

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (_idx.row[i] >= (int)_mMatrix.rows() || _idx.col[j] >= (int)_mMatrix.cols())
                throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position,
                                  _idx.row.to_string() + ", " + _idx.col.to_string());

            _mReturn(i, j) = _mMatrix(_idx.row[i], _idx.col[j]);
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

    for (size_t col = 0; col < _mResult.cols(); col++)
    {
        if (_mResult.cols() > (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER
            && (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 == col)
        {
            sRow += strfill("..., ", FIELDLENGTH_W_FILLER);
            col = _mResult.cols() - (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 - 1;
            continue;
        }

        sRow += strfill(toString(_mResult(row, col), PRECISION), FIELDLENGTH);

        if (col+1 < _mResult.cols())
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

    if (_mResult.rows() > 10)
    {
        for (size_t i = 0; i < _mResult.rows(); i++)
        {
            if (!i)
                NumeReKernel::printPreFmt("|   /");
            else if (i+1 == _mResult.rows())
                NumeReKernel::printPreFmt("|   \\");
            else if (i == 5)
                NumeReKernel::printPreFmt("|-> |");
            else
                NumeReKernel::printPreFmt("|   |");

            if (i == 5)
            {
                for (size_t j = 0; j < _mResult.cols(); j++)
                {
                    if (_mResult.cols() > (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER
                        && (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 == j)
                    {
                        NumeReKernel::printPreFmt(strfill("..., ", FIELDLENGTH_W_FILLER));
                        j = _mResult.cols() - (_option.getWindow()-2-15) / FIELDLENGTH_W_FILLER / 2 - 1;
                        continue;
                    }

                    NumeReKernel::printPreFmt(strfill("...", FIELDLENGTH));

                    if (j+1 < _mResult.cols())
                        NumeReKernel::printPreFmt(", ");
                }

                i = _mResult.rows()-6;
            }
            else
                NumeReKernel::printPreFmt(formatMatrixRow(_mResult, _option, i));

            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.rows())
                NumeReKernel::printPreFmt(" /\n");
            else
                NumeReKernel::printPreFmt(" |\n");
        }
    }
    else if (_mResult.rows() == 1)
    {
        if (_mResult.cols() == 1)
            NumeReKernel::print("(" + toString(_mResult(0), _option.getPrecision()) + ")");
        else
            NumeReKernel::print("(" + formatMatrixRow(_mResult, _option, 0) + " )");
    }
    else
    {
        for (size_t i = 0; i < _mResult.rows(); i++)
        {
            if (!i && _mResult.rows() == 2)
                NumeReKernel::printPreFmt("|-> /");
            else if (!i)
                NumeReKernel::printPreFmt("|   /");
            else if (i+1 == _mResult.rows())
                NumeReKernel::printPreFmt("|   \\");
            else if (i == (_mResult.rows()-1)/2)
                NumeReKernel::printPreFmt("|-> |");
            else
                NumeReKernel::printPreFmt("|   |");

            NumeReKernel::printPreFmt(formatMatrixRow(_mResult, _option, i));

            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.rows())
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
/// \return Indices
///
/////////////////////////////////////////////////
Indices getIndices(const string& sCmd, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data)
{
    Indices _idx;
    string sI[2] = {"<<NONE>>", "<<NONE>>"};
    string sJ[2] = {"<<NONE>>", "<<NONE>>"};
    string sArgument = "";
    size_t nPos = 0;
    int nParenthesis = 0;

    if (sCmd.find('(') == string::npos)
        return _idx;

    nPos = sCmd.find('(');

    for (size_t n = nPos; n < sCmd.length(); n++)
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
        getDataElements(sArgument, _parser, _data);

    // --> Kurzschreibweise!
    if (!sArgument.length())
    {
        _idx.row = VectorIndex(0LL, _mMatrix.rows());
        _idx.col = VectorIndex(0LL, _mMatrix.cols());
        return _idx;
    }

    if (sArgument.find(',') != string::npos)
    {
        nParenthesis = 0;
        nPos = 0;

        for (size_t n = 0; n < sArgument.length(); n++)
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
                    sJ[0] = "<<EMPTY>>";
                else
                    sJ[0] = sArgument.substr(nPos, n-nPos);

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
            mu::Array v = _parser.Eval();

            if (v.size() > 1)
                _idx.row = VectorIndex(v);
            else
                _idx.row.front() = v.getAsScalarInt()-1;
        }

        if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
        {
            _parser.SetExpr(sJ[0]);
            mu::Array v = _parser.Eval();

            if (v.size() > 1)
                _idx.col = VectorIndex(v);
            else
                _idx.col.front() = v.getAsScalarInt()-1;
        }

        for (int n = 0; n < 2; n++)
        {
            if (sI[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.row.back() = _mMatrix.rows()-1;
                else
                    _idx.row.front() = 0;
            }
            else if (sI[n] != "<<NONE>>")
            {
                _parser.SetExpr(sI[n]);
                mu::Array v = _parser.Eval();

                if (mu::isnan(v.front().getNum()) || mu::isinf(v.front().getNum().asCF64()) || v.front().getNum().asF64() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position, sI[n]);

                _idx.row.setIndex(n, v.getAsScalarInt()-1);
            }

            if (sJ[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.col.back() = _mMatrix.cols()-1;
                else
                    _idx.col.front() = 0;
            }
            else if (sJ[n] != "<<NONE>>")
            {
                _parser.SetExpr(sJ[n]);
                mu::Array v = _parser.Eval();

                if (mu::isnan(v.front().getNum()) || mu::isinf(v.front().getNum().asCF64()) || v.front().getNum().asF64() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position, sJ[n]);

                _idx.col.setIndex(n, v.getAsScalarInt()-1);
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
/// \param _mMatrix const vector<vector<std::complex<double>>>&
/// \return void
///
/////////////////////////////////////////////////
void showMatrix(const vector<vector<std::complex<double>>>& _mMatrix)
{
    showMatrixResult(Matrix(_mMatrix), NumeReKernel::getInstance()->getSettings());
}

/////////////////////////////////////////////////
/// \brief Static helper function for
/// parser_getIndicesForMatrix(), which will
/// handle the return values of matrix evaluations.
///
/// \param _sCmd StringView
/// \param vReturnedMatrices const vector<Matrix>&
/// \param _parser mu::Parser&
/// \return void
///
/////////////////////////////////////////////////
static void parser_declareMatrixReturnValuesForIndices(StringView _sCmd, const std::vector<Matrix>& vReturnedMatrices, mu::Parser& _parser)
{
    for (size_t j = 0; j < vReturnedMatrices.size(); j++)
    {
        mu::Array v;

        if (vReturnedMatrices[j].isScalar())
            v.push_back(vReturnedMatrices[j](0));
        else if (vReturnedMatrices[j].rows() == 1
                 || vReturnedMatrices[j].cols() == 1)
            v = vReturnedMatrices[j].data();
        else
        {
            for (size_t i = 0; i < vReturnedMatrices[j].rows(); i++)
                v.push_back(vReturnedMatrices[j](i));
        }

        // Declare the corresponding vector variable
        if (_sCmd.find("_~returnedMatrix["+toString(j)+"]") != std::string::npos)
            _parser.SetInternalVar("_~returnedMatrix["+toString(j)+"]", v);
    }
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// parser_getIndicesForMatrix(), which will
/// handle the indices of already parsed datafile
/// matrices.
///
/// \param _sCmd StringView
/// \param _cache const MatOpCache&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void parser_declareDataMatrixValuesForIndices(StringView _sCmd, const MatOpCache& _cache, mu::Parser&_parser, MemoryManager& _data)
{
    for (size_t j = 0; j < _cache.vDataAccesses.size(); j++)
    {
        mu::Array v;
        const MatOpCache::MatOpDataAccess& _access = _cache.vDataAccesses[j];

        // Get the values using the indices
        if (_access.idx.row.size() > _access.idx.col.size())
            v = _data.getElement(_access.idx.row, VectorIndex(_access.idx.col[0]), _access.sName);
        else
            v = _data.getElement(VectorIndex(_access.idx.row[0]), _access.idx.col, _access.sName);

        // Declare the corresponding vector variable
        if (_sCmd.find("_~matrix["+toString(j)+"]") != std::string::npos)
            _parser.SetInternalVar("_~matrix["+toString(j)+"]", v);
    }
}


/////////////////////////////////////////////////
/// \brief Static wrapper function for resolving
/// already parsed datafile matrix elements and
/// evaluated matrix expressions, which are used
/// as indices for datafile matrices.
///
/// \param sCmd StringView
/// \param _cache const MatOpCache&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return Indices
///
/////////////////////////////////////////////////
static Indices getIndicesForMatrix(StringView sCmd, const MatOpCache& _cache, Parser& _parser, MemoryManager& _data)
{
    // Declare the return values of the former matrix calculations
    // to the parser by extracting the values and creating a vector
    // variable
    parser_declareMatrixReturnValuesForIndices(sCmd, _cache.vReturnedMatrices, _parser);

    // Declare the already parsed data object matrices in the
    // current expressions by parsing their indices, extracting
    // the corresponding values and creating a vector variable
    parser_declareDataMatrixValuesForIndices(sCmd, _cache, _parser, _data);

    // Return the calculated indices
    return getIndices(sCmd, _parser, _data, false);
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

