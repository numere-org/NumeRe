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
#include "parser_functions.hpp"
#include "../../kernel.hpp"
#include <list>
#include <cmath>

#define EIGENVALUES 0
#define EIGENVECTORS 1
#define DIAGONALIZE 2

static Matrix parser_subMatrixOperations(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static size_t parser_getPreviousMatrixMultiplicationOperator(const string& sCmd, size_t nLastPos);
static Matrix parser_matrixMultiplication(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static vector<double> parser_calcDeltas(const Matrix& _mMatrix, unsigned int nLine);
static bool parser_IsSymmMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static void parser_makeReal(Matrix& _mMatrix);
static double parser_calcDeterminant(const Matrix& _mMatrix, vector<int> vRemovedLines);
static void parser_ShowMatrixResult(const Matrix& _mResult, const Settings& _option);
static void parser_solveLGSSymbolic(const Matrix& _mMatrix, Parser& _parser, FunctionDefinitionManager& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position);
static Indices parser_getIndicesForMatrix(const string& sCmd, const vector<string>& vMatrixNames, const vector<Indices>& vIndices, const vector<Matrix>& vReturnedMatrices, Parser& _parser, MemoryManager& _data, const Settings& _option);
static map<string,string> createMatrixFunctionsMap();
static bool containsMatrices(const string& sExpr, MemoryManager& _data);



static Matrix parser_diagonalMatrix(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix parser_solveLGS(const Matrix& _mMatrix, Parser& _parser, FunctionDefinitionManager& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_calcCrossProduct(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_calcEigenVects(const Matrix& _mMatrix, int nReturnType, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_calcTrace(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_IdentityMatrix(unsigned int nSize);
static Matrix parser_OnesMatrix(unsigned int nLines, unsigned int nCols);
Matrix createZeroesMatrix(unsigned int nLines, unsigned int nCols);
static Matrix parser_shuffleMatrix(unsigned int nShuffle, unsigned int nBase);
static Matrix parser_getDeterminant(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_matFromCols(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix parser_matFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix parser_matFromLines(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix parser_matFromLinesFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
static Matrix parser_InvertMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix transposeMatrix(const Matrix& _mMatrix);
static Matrix parser_MatrixLogToIndex(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixIndexToLog(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixSize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixAnd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixOr(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixXor(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixSum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixStd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixAvg(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixPrd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixCnt(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixNum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixNorm(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixMin(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixMax(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixMinPos(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixMaxPos(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixMed(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixPct(const Matrix& _mMatrix, double dPercentage, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixCmp(const Matrix& _mMatrix, double dValue, int nType, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_Correlation(const Matrix& _mMatrix1, const Matrix& _mMatrix2, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_Covariance(const Matrix& _mMatrix1, const Matrix& _mMatrix2, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_Normalize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixReshape(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixResize(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixRepMat(const Matrix& _mMatrix, size_t n, size_t m, const string& sCmd, const string& sExpr, size_t position);
static Matrix parser_MatrixUnique(const Matrix& _mMatrix, size_t nDim, const string& sCmd, const string& sExpr, size_t position);
static std::vector<double> parser_getUniqueList(std::list<double>& _list);
static void parser_fillMissingMatrixElements(Matrix& _mMatrix);


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
    vector<Indices> vIndices;
    vector<double> vMatrixVector;
    vector<vector<double> > vTarget;
    vector<string> vMatrixNames;
    vector<int> vMissingValues;
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
                bAllowMatrixClearing = true;
        }
        else if (sTargetName[parens] == '{')
        {
            // Create a new cluster
            if (!_data.isCluster(sTargetName))
                _data.newCluster(sTargetName);

            isCluster = true;

            if (sTargetName.substr(sTargetName.find('{'),2) == "{}")
                bAllowMatrixClearing = true;
        }

        getIndices(sTargetName, _idx, _parser, _data, _option);

        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sTargetName, sTargetName);

        sCmd.erase(0, sCmd.find('=')+1);

        sTargetName.erase(sTargetName.find_first_of("({"));
    }
    else
    {
        sTargetName = "matrix";
        _idx.row = VectorIndex(0LL, VectorIndex::OPEN_END);
        _idx.col = VectorIndex(0LL, VectorIndex::OPEN_END);

        if (!_data.isTable("matrix("))
        {
            _data.addTable("matrix", _option);
        }
        else
            bAllowMatrixClearing = true;
    }

    // Matrixmultiplikationen / Transpositionen / Invertierungen?
    // -> Submatrixoperationen ausfuehren
    Matrix _mResult = parser_subMatrixOperations(sCmd, _parser, _data, _functions, _option);

    // Target in Zielmatrix speichern
    if (!isCluster)
    {
        // This target is a table
        if (bAllowMatrixClearing)
            _data.deleteBulk(sTargetName, 0, _data.getLines(sTargetName, false) - 1, 0, _data.getCols(sTargetName, false) - 1);

        // Prepare the target size
        _data.resizeTable(_idx.row.front()+_mResult.size(), _idx.col.front()+_mResult[0].size(), sTargetName);

        // Write the contents to the table
        for (unsigned int i = 0; i < _mResult.size(); i++)
        {
            if (_idx.row[i] != VectorIndex::INVALID)
            {
                for (unsigned int j = 0; j < _mResult[0].size(); j++)
                {
                    if (_idx.col[j] != VectorIndex::INVALID)
                        _data.writeToTable(_idx.row[i], _idx.col[j], sTargetName, _mResult[i][j]);
                    else
                        break;
                }
            }
            else
                break;
        }
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
    parser_ShowMatrixResult(_mResult, _option);

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
static Matrix parser_subMatrixOperations(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    string __sCmd;
    size_t pos_back = 0;
    size_t iter_start = 0;
    vector<Matrix> vReturnedMatrices;
    value_type* v = 0;
    int nResults = 0;

    // Check, whether there's a target vector available in this expression part
    if (sCmd.find('=') != string::npos
        && sCmd.find('=')
        && sCmd[sCmd.find('=')+1] != '='
        && sCmd[sCmd.find('=')-1] != '!'
        && sCmd[sCmd.find('=')-1] != '<'
        && sCmd[sCmd.find('=')-1] != '>'
        && sCmd.substr(0,sCmd.find('=')).find('{') != string::npos)
    {
        iter_start = sCmd.find('=')+1;
    }

    // Apply all known and requested matrix functions.
    // The equation will be segmentized and the evaluated
    // part will be appended to the evaluated equation.
    for (unsigned int i = iter_start; i < sCmd.length(); i++)
    {
        // transpose matrix
        if (sCmd.substr(i,10) == "transpose("
            && getMatchingParenthesis(sCmd.substr(i+9)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,11))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+9))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(transposeMatrix(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option)));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+9))+10;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // invert matrix (if possible)
        if (sCmd.substr(i,7) == "invert("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+6))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_InvertMatrix(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate eigenvalues
        if (sCmd.substr(i,10) == "eigenvals("
            && getMatchingParenthesis(sCmd.substr(i+9)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,11))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+9))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), EIGENVALUES, sCmd, sSubExpr, i+9));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+9))+10;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the eigenvectors
        if (sCmd.substr(i,11) == "eigenvects("
            && getMatchingParenthesis(sCmd.substr(i+10)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,12))))
        {
            string sSubExpr = sCmd.substr(i+10, getMatchingParenthesis(sCmd.substr(i+10))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), EIGENVECTORS, sCmd, sSubExpr, i+10));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+10))+11;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // diagonalize the matrix
        if (sCmd.substr(i,12) == "diagonalize("
            && getMatchingParenthesis(sCmd.substr(i+11)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,13))))
        {
            string sSubExpr = sCmd.substr(i+11, getMatchingParenthesis(sCmd.substr(i+11))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), DIAGONALIZE, sCmd, sSubExpr, i+11));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+11))+12;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // solve the linear system of equations
        if (sCmd.substr(i,6) == "solve("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_solveLGS(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), _parser, _functions, _option, sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the n-dimensional cross product
        if (sCmd.substr(i,6) == "cross("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcCrossProduct(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the trace of the matrix
        if (sCmd.substr(i,6) == "trace("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcTrace(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the determinant
        if (sCmd.substr(i,4) == "det("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+3, getMatchingParenthesis(sCmd.substr(i+3))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_getDeterminant(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // create a matrix filled with ones
        if (sCmd.substr(i,4) == "one("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            _parser.SetExpr(sSubExpr);
            v = _parser.Eval(nResults);
            if (nResults > 1)
                vReturnedMatrices.push_back(parser_OnesMatrix((unsigned int)v[0], (unsigned int)v[1]));
            else
                vReturnedMatrices.push_back(parser_OnesMatrix((unsigned int)v[0], (unsigned int)v[0]));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // create a matrix filled with zeros
        if (sCmd.substr(i,5) == "zero("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            _parser.SetExpr(sSubExpr);
            v = _parser.Eval(nResults);
            if (nResults > 1)
                vReturnedMatrices.push_back(createZeroesMatrix((unsigned int)v[0], (unsigned int)v[1]));
            else
                vReturnedMatrices.push_back(createZeroesMatrix((unsigned int)v[0], (unsigned int)v[0]));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // create a shuffled vector
        if (sCmd.substr(i,8) == "shuffle("
            && getMatchingParenthesis(sCmd.substr(i+7)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,9))))
        {
            string sSubExpr = sCmd.substr(i+8, getMatchingParenthesis(sCmd.substr(i+7))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            _parser.SetExpr(sSubExpr);
            v = _parser.Eval(nResults);
            if (nResults > 1)
                vReturnedMatrices.push_back(parser_shuffleMatrix((unsigned int)v[0], (unsigned int)v[1]));
            else
                vReturnedMatrices.push_back(parser_shuffleMatrix((unsigned int)v[0], (unsigned int)v[0]));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+7))+8;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the covariance of two
        // matrices
        if (sCmd.substr(i, 6) == "covar("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sMatrix2 = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+5))-1);
            string sMatrix1 = getNextArgument(sMatrix2, true);

            __sCmd += sCmd.substr(pos_back, i-pos_back);
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            vReturnedMatrices.push_back(parser_Covariance(parser_subMatrixOperations(sMatrix1, _parser, _data, _functions, _option), parser_subMatrixOperations(sMatrix2, _parser, _data, _functions, _option), sCmd, sMatrix1 + ", " + sMatrix2, i+5));
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // calculate the correlation of two
        // matrices
        if (sCmd.substr(i, 7) == "correl("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sMatrix2 = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            string sMatrix1 = getNextArgument(sMatrix2, true);

            __sCmd += sCmd.substr(pos_back, i-pos_back);
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            vReturnedMatrices.push_back(parser_Correlation(parser_subMatrixOperations(sMatrix1, _parser, _data, _functions, _option), parser_subMatrixOperations(sMatrix2, _parser, _data, _functions, _option), sCmd, sMatrix1 + ", " + sMatrix2, i+6));
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // create an identity matrix
        if (sCmd.substr(i,9) == "identity("
            && getMatchingParenthesis(sCmd.substr(i+8)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,10))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+8))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            _parser.SetExpr(sSubExpr);
            v = _parser.Eval(nResults);
            vReturnedMatrices.push_back(parser_IdentityMatrix((unsigned int)v[0]));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+8))+9;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // construct matrix from columns
        if (sCmd.substr(i,6) == "matfc("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+5))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // construct matrix from lines
        if (sCmd.substr(i,6) == "matfl("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+5))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromLines(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // construct matrix from columns and fill missing values
        if (sCmd.substr(i,7) == "matfcf("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromColsFilled(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // construct matrix from lines and fill missing values
        if (sCmd.substr(i,7) == "matflf("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromLinesFilled(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Create a square matrix with the passed elements on its main diagonal
        if (sCmd.substr(i,5) == "diag("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_diagonalMatrix(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Normalize the values in the matrix into the (absolute) range of [0,1]
        if (sCmd.substr(i,10) == "normalize("
            && getMatchingParenthesis(sCmd.substr(i+9)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,11))))
        {
            string sSubExpr = sCmd.substr(i+10, getMatchingParenthesis(sCmd.substr(i+9))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_Normalize(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+9));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+9))+10;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Convert logical values into indices
        if (sCmd.substr(i,9) == "logtoidx("
            && getMatchingParenthesis(sCmd.substr(i+8)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,10))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+8))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixLogToIndex(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+8));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+8))+9;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Convcert indices into logical values
        if (sCmd.substr(i,9) == "idxtolog("
            && getMatchingParenthesis(sCmd.substr(i+8)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,10))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+8))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixIndexToLog(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+8));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+8))+9;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Get the size of the matrix
        if (sCmd.substr(i,5) == "size("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixSize(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+4));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "and()" for matrices
        if (sCmd.substr(i,4) == "and("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixAnd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "xor()" for matrices
        if (sCmd.substr(i,4) == "xor("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixXor(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "or()" for matrices
        if (sCmd.substr(i,3) == "or("
            && getMatchingParenthesis(sCmd.substr(i+2)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,4))))
        {
            string sSubExpr = sCmd.substr(i+3, getMatchingParenthesis(sCmd.substr(i+2))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixOr(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+2));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+2))+3;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "sum()" for matrices
        if (sCmd.substr(i,4) == "sum("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixSum(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "avg()" for matrices
        if (sCmd.substr(i,4) == "avg("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixAvg(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "std()" for matrices
        if (sCmd.substr(i,4) == "std("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixStd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "num()" for matrices
        if (sCmd.substr(i,4) == "num("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixNum(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "cnt()" for matrices
        if (sCmd.substr(i,4) == "cnt("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixCnt(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "min()" for matrices
        if (sCmd.substr(i,4) == "min("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMin(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "max()" for matrices
        if (sCmd.substr(i,4) == "max("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMax(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "min()" for matrices
        if (sCmd.substr(i,7) == "minpos("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMinPos(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "max()" for matrices
        if (sCmd.substr(i,7) == "maxpos("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMaxPos(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "prd()" for matrices
        if (sCmd.substr(i,4) == "prd("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixPrd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "norm()" for matrices
        if (sCmd.substr(i,5) == "norm("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixNorm(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "med()" for matrices
        if (sCmd.substr(i,4) == "med("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMed(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "cmp()" for matrices
        if (sCmd.substr(i,4) == "cmp("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            int nRes;
            value_type* v;
            string sExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (!sExpr.length())
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+4, "cmp()");
            _parser.SetExpr(sExpr);
            v = _parser.Eval(nRes);
            if (nRes < 2)
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+4, "cmp()");
            __sCmd += sCmd.substr(pos_back, i-pos_back);

            vReturnedMatrices.push_back(parser_MatrixCmp(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), v[0], (int)v[1], sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "pct()" for matrices
        if (sCmd.substr(i,4) == "pct("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            int nRes;
            value_type* v;
            string sExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (!sExpr.length())
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+4, "pct()");
            _parser.SetExpr(sExpr);
            v = _parser.Eval(nRes);
            __sCmd += sCmd.substr(pos_back, i-pos_back);

            vReturnedMatrices.push_back(parser_MatrixPct(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), v[0], sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Reshape the passed matrix to fit to the specified dimensions
        if (sCmd.substr(i,8) == "reshape("
            && getMatchingParenthesis(sCmd.substr(i+7)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,9))))
        {
            int nRes;
            value_type* v;
            string sExpr = sCmd.substr(i+8, getMatchingParenthesis(sCmd.substr(i+7))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (!sExpr.length())
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+8, "reshape()");
            _parser.SetExpr(sExpr);
            v = _parser.Eval(nRes);
            if (nRes < 2)
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+8, "reshape()");
            __sCmd += sCmd.substr(pos_back, i-pos_back);

            vReturnedMatrices.push_back(parser_MatrixReshape(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), (size_t)v[0], (size_t)v[1], sCmd, sSubExpr, i+7));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+7))+8;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Resize the passed matrix to fit to the specified dimensions
        if (sCmd.substr(i,7) == "resize("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            int nRes;
            value_type* v;
            string sExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (!sExpr.length())
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+7, "resize()");
            _parser.SetExpr(sExpr);
            v = _parser.Eval(nRes);
            if (nRes < 2)
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+7, "resize()");
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixResize(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), (size_t)v[0], (size_t)v[1], sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Repeat the passed matrix to fit to the specified dimensions
        if (sCmd.substr(i,7) == "repmat("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            int nRes;
            value_type* v;
            string sExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (!sExpr.length())
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+7, "repmat()");
            _parser.SetExpr(sExpr);
            v = _parser.Eval(nRes);
            if (nRes < 2)
                throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, i+7, "repmat()");
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixRepMat(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), (size_t)v[0], (size_t)v[1], sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Apply "unique()" for matrices
        if (sCmd.substr(i,7) == "unique("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            int nRes;
            value_type* v;
            size_t nDim = 0;
            string sExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            string sSubExpr = getNextArgument(sExpr, true);
            if (sExpr.length())
            {
                _parser.SetExpr(sExpr);
                v = _parser.Eval(nRes);
                nDim = intCast(v[0]);
            }

            __sCmd += sCmd.substr(pos_back, i-pos_back);

            vReturnedMatrices.push_back(parser_MatrixUnique(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), nDim, sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Handle old vector syntax (will most probably be changed to matrix syntax)
        if (sCmd.substr(i,2) == "{{"
            && getMatchingParenthesis(sCmd.substr(i)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,3))))
        {
            string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }

        // Handle vectors
        if (sCmd[i] == '{'
            && getMatchingParenthesis(sCmd.substr(i)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,2))))
        {
            string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
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
                string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
                pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
                vReturnedMatrices[nMatrix] = parser_getMatrixElements(sSubExpr, vReturnedMatrices[nMatrix], _parser, _data, _functions, _option);
                i = pos_back-1;
            }
        }

        // Pre-evaluate parentheses
        if (sCmd[i] == '(')
        {
            if (sCmd.substr(i,getMatchingParenthesis(sCmd.substr(i))).find("**") != string::npos
                || (i > 1 && !_data.isTable(sCmd.substr(sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)+1, i-sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)-1))))
            {
                string sSubExpr = sCmd.substr(i+1, getMatchingParenthesis(sCmd.substr(i))-1);
                size_t closing_par_pos = i+getMatchingParenthesis(sCmd.substr(i));
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
                    {
                        __sCmd += sCmd.substr(pos_back, i-pos_back);
                    }

                    vReturnedMatrices.push_back(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option));
                    __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
                }
                else
                {
                    __sCmd += sCmd.substr(pos_back, i-pos_back) + "(";

                    // As this might be a usual function, evaluate each argument
                    // separately (if it is matrix expression) and combine them
                    // afterwards
                    while (sSubExpr.length())
                    {
                        string sExpr = getNextArgument(sSubExpr, true);

                        if (containsMatrices(sExpr, _data))
                        {
                            vReturnedMatrices.push_back(parser_subMatrixOperations(sExpr, _parser, _data, _functions, _option));
                            __sCmd += "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
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
        __sCmd += sCmd.substr(pos_back);

    vector<Indices> vIndices;
    vector<double> vMatrixVector;
    Matrix _mTarget;
    Matrix _mResult;
    vector<string> vMatrixNames;
    vector<int> vMissingValues;

    unsigned int nPos = 0;
    unsigned int nColCount = 0;
    unsigned int nLinesCount = 0;

    // Get the indices of the calls to the memory objects
    // all caches
    for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
    {
        nPos = 0;
        while (__sCmd.find(iter->first+"(", nPos) != string::npos)
        {
            nPos = __sCmd.find(iter->first+"(", nPos);

            // Check the delimiters
            if (nPos && !checkDelimiter(__sCmd.substr(nPos-1,(iter->first).length()+2)))
            {
                nPos++;
                continue;
            }

            // Get the indices
            vIndices.push_back(parser_getIndicesForMatrix(__sCmd.substr(nPos), vMatrixNames, vIndices, vReturnedMatrices, _parser, _data, _option));

            // Evaluate the indices
            if (!evaluateIndices(iter->first, vIndices[vIndices.size()-1], _data))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, nPos);

            // Store the name of the current data object
            vMatrixNames.push_back(iter->first);

            // Identify, which value to use for a missing value
            if (addMissingVectorComponent("", __sCmd.substr(0,nPos), __sCmd.substr(nPos+1+(iter->first).length()+getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))),false) == "0")
                vMissingValues.push_back(0);
            else
                vMissingValues.push_back(1);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))+(iter->first).length()+1, "_~matrix["+toString((int)vMatrixNames.size()-1)+"]");
        }
    }

    // now all clusters
    for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
    {
        nPos = 0;

        while (__sCmd.find(iter->first+"{", nPos) != string::npos)
        {
            nPos = __sCmd.find(iter->first+"{", nPos);

            // Check the delimiters
            if (nPos && !checkDelimiter(__sCmd.substr(nPos-1,(iter->first).length()+2)))
            {
                nPos++;
                continue;
            }

            // Get the indices
            Indices _idx = parser_getIndicesForMatrix(__sCmd.substr(nPos), vMatrixNames, vIndices, vReturnedMatrices, _parser, _data, _option);

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
            vReturnedMatrices.push_back(_mClusterMatrix);

            // Replace the current call with a standardized one
            __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))+(iter->first).length()+1, "_~returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
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
            if (__sCmd.substr(n,2) == "**")
            {
                // Find the last operator
                pos_back = parser_getPreviousMatrixMultiplicationOperator(sCmd, pos_back)-1;
                Matrix _mLeft;
                Matrix _mRight;
                unsigned int nPositions[2];
                nPositions[1] = __sCmd.find(']',n)+1;
                string sElement = __sCmd.substr(__sCmd.find_first_not_of(' ', n+2));

                // Right handed matrix expression
                sElement.erase(sElement.find(']')+1);
                if (sElement.find_first_of("()+-*/^!%&|<>=?:,") != string::npos || sElement.find_first_of("[]") == string::npos)
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                // It's either an access to a memory object
                // or the return value of a matrix function
                if (sElement.substr(0,9) == "_~matrix[")
                {
                    // memory access
                    vector<double> vLine;
                    unsigned int nthMatrix = StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')));

                    // Get the matrix elements
                    for (unsigned int i = 0; i < vIndices[nthMatrix].row.size(); i++)
                    {
                        _mRight.push_back(_data.getElement(VectorIndex(vIndices[nthMatrix].row[i]), vIndices[nthMatrix].col, vMatrixNames[nthMatrix]));
                    }
                }
                else
                {
                    // return value
                    _mRight = vReturnedMatrices[StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')))];
                }

                // Left handed matrix expression
                sElement = __sCmd.substr(0,__sCmd.find_last_of(']', n-1)+1);
                sElement.erase(0,sElement.find_last_of('[')-8);
                if (sElement.find_first_of("()+-*/^!%&|<>=?:,") != string::npos || sElement.find_first_of("[]") == string::npos)
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                // It's either an access to a memory object
                // or the return value of a matrix function
                if (sElement.substr(0,9) == "_~matrix[")
                {
                    // memory access
                    nPositions[0] = __sCmd.rfind("_~matrix[",n);
                    vector<double> vLine;
                    unsigned int nthMatrix = StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')));

                    // Get the matrix elements
                    for (unsigned int i = 0; i < vIndices[nthMatrix].row.size(); i++)
                    {
                        _mLeft.push_back(_data.getElement(VectorIndex(vIndices[nthMatrix].row[i]), vIndices[nthMatrix].col, vMatrixNames[nthMatrix]));
                    }
                }
                else
                {
                    // return value
                    nPositions[0] = __sCmd.rfind("_~returnedMatrix[", n);
                    _mLeft = vReturnedMatrices[StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')))];
                }

                // Perform the actual matrix multiplication
                vReturnedMatrices.push_back(parser_matrixMultiplication(_mLeft, _mRight, sCmd, "", pos_back+1));

                // Replace the current multiplication with its return value
                __sCmd.replace(nPositions[0], nPositions[1]-nPositions[0], "_~returnedMatrix[" + toString((int)vReturnedMatrices.size()-1)+"]");
                n = nPositions[0];
            }
        }
    }

    sCmd = __sCmd;

    // We need the maximal number of columns for the regular
    // evaluation of the expression
    //
    // Examine the memory accesses, which are still available
    // in the expression
    for (unsigned int i = 0; i < vIndices.size(); i++)
    {
        if (sCmd.find("_~matrix["+toString((int)i)+"]") == string::npos)
            continue;

        // Get the column count from the dimensions of the indices
        if (vIndices[i].col.size() > nColCount)
            nColCount = vIndices[i].col.size();
    }

    // Examine now the return values available in the expression
    for (unsigned int i = 0; i < vReturnedMatrices.size(); i++)
    {
        if (vReturnedMatrices[i][0].size() > nColCount && sCmd.find("_~returnedMatrix["+toString((int)i)+"]") != string::npos)
            nColCount = vReturnedMatrices[i][0].size();
    }

    // Read now the first column of every matrix in the expression
    // as vector for the parser
    //
    // start with the memory accesses
    for (unsigned int j = 0; j < vIndices.size(); j++)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();

        // Get the values using the incdices
        vMatrixVector = _data.getElement(vIndices[j].row, VectorIndex(vIndices[j].col.front()), vMatrixNames[j]);

        // Declare the corresponding vector variable
        if (sCmd.find("_~matrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("_~matrix["+toString((int)j)+"]", vMatrixVector);
    }

    // Get now the first columns of the returned values
    for (unsigned int j = 0; j < vReturnedMatrices.size(); j++)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();

        // Get the first column from the return values
        if (!vReturnedMatrices[j][0].size())
            vMatrixVector.push_back(0.0);
        else
        {
            for (unsigned int k = 0; k < vReturnedMatrices[j].size(); k++)
            {
                vMatrixVector.push_back(vReturnedMatrices[j][k][0]);
            }
        }

        // Declare the corresponding vector variable
        if (sCmd.find("_~returnedMatrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("_~returnedMatrix["+toString((int)j)+"]", vMatrixVector);
    }

    // Set the expression in the parser
    _parser.SetExpr(sCmd);

    // Evaluate the first columns
    v = _parser.Eval(nResults);

    if (vMatrixVector.size())
        vMatrixVector.clear();

    // Store the results of the first column
    // in a temporary vector
    for (int i = 0; i < nResults; i++)
    {
        vMatrixVector.push_back(v[i]);
    }

    // Append the temporary vector to the
    // target variable
    _mTarget.push_back(vMatrixVector);

    // Get the maximal row dimension
    if (vMatrixVector.size() > nLinesCount)
        nLinesCount = vMatrixVector.size();

    // Ensure that the column dimension is at least 1
    if (!nColCount && vMatrixVector.size())
        nColCount = 1;

    // Get now the next columns as vectors
    // and evaluate the expression with their
    // values
    for (unsigned int i = 1; i < nColCount; i++)
    {
        // Read the next columns of every matrix in the expression
        // as vector for the parser
        for (unsigned int j = 0; j < vIndices.size(); j++)
        {
            if (vMatrixVector.size())
                vMatrixVector.clear();

            if (vIndices[j].col.size() <= i && (vIndices[j].col.size() > 1 || vIndices[j].row.size() > 1))
                vMatrixVector.push_back(vMissingValues[j]);
            else if (vIndices[j].row.size() >= 1 && vIndices[j].col.size() == 1)
                continue;
            else
                vMatrixVector = _data.getElement(vIndices[j].row, VectorIndex(vIndices[j].col[i]), vMatrixNames[j]);

            if (sCmd.find("_~matrix["+toString((int)j)+"]") != string::npos)
                _parser.SetVectorVar("_~matrix["+toString((int)j)+"]", vMatrixVector);
        }

        // Get now the next columns of the returned values
        for (unsigned int j = 0; j < vReturnedMatrices.size(); j++)
        {
            if (vMatrixVector.size())
                vMatrixVector.clear();

            if (!vReturnedMatrices[j][0].size())
                vMatrixVector.push_back(0.0);
            else if (vReturnedMatrices[j].size() >= 1 && vReturnedMatrices[j][0].size() == 1)
                continue;
            else
            {
                for (unsigned int k = 0; k < vReturnedMatrices[j].size(); k++)
                {
                    if (vReturnedMatrices[j][0].size() <= i)
                        vMatrixVector.push_back(0.0);
                    else
                        vMatrixVector.push_back(vReturnedMatrices[j][k][i]);
                }
            }

            if (sCmd.find("_~returnedMatrix["+toString((int)j)+"]") != string::npos)
                _parser.SetVectorVar("_~returnedMatrix["+toString((int)j)+"]", vMatrixVector);
        }

        // Evaluate
        v = _parser.Eval(nResults);

        // Clear the temporary vector
        if (vMatrixVector.size())
            vMatrixVector.clear();

        // Store the results in the temporary
        // vector
        for (int j = 0; j < nResults; j++)
            vMatrixVector.push_back(v[j]);

        // Append the vector to the target matrix
        _mTarget.push_back(vMatrixVector);

        // Get the maximal row dimension
        if (vMatrixVector.size() > nLinesCount)
            nLinesCount = vMatrixVector.size();
    }

    // Transpose the target matrix and fill
    // the missing values with zeros
    for (unsigned int i = 0; i < nLinesCount; i++)
    {
        vMatrixVector.clear();
        for (unsigned int j = 0; j < nColCount; j++)
        {
            if (_mTarget[j].size() > i)
                vMatrixVector.push_back(_mTarget[j][i]);
            else
                vMatrixVector.push_back(0.0);
        }
        _mResult.push_back(vMatrixVector);
    }

    // return the actual result
    return _mResult;
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
static size_t parser_getPreviousMatrixMultiplicationOperator(const string& sCmd, size_t nLastPos)
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
static Matrix parser_matrixMultiplication(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mLeft.size() || !_mLeft[0].size() || !_mRight.size() || !_mRight[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mResult;
    vector<double> vLine;
    double dEntry = 0.0;

    if (_mRight.size() == 1 && _mRight[0].size() && _mLeft.size() == 1 && _mLeft[0].size() == _mRight[0].size())
    {
        //cerr << "vectormultiplication" << endl;
        for (unsigned int i = 0; i < _mLeft[0].size(); i++)
        {
            dEntry += _mLeft[0][i]*_mRight[0][i];
        }
        //cerr << dEntry << endl;
        vLine.push_back(dEntry);
        _mResult.push_back(vLine);
        //cerr << "returning" << endl;
        return  _mResult;
    }

    if (_mRight[0].size() == 1 && _mRight.size() && _mLeft[0].size() == 1 && _mLeft.size() == _mRight.size())
    {
        //cerr << "vectormultiplication" << endl;
        for (unsigned int i = 0; i < _mLeft.size(); i++)
        {
            dEntry += _mLeft[i][0]*_mRight[i][0];
        }
        //cerr << dEntry << endl;
        vLine.push_back(dEntry);
        _mResult.push_back(vLine);
        //cerr << "returning" << endl;
        return  _mResult;
    }
    //cerr << "dimension" << endl;
    if (_mRight.size() != _mLeft[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mLeft.size()) +"x"+ toString(_mLeft[0].size()) +" vs. "+ toString(_mRight.size()) +"x"+ toString(_mRight[0].size()));

    //cerr << "multiplication" << endl;
    for (unsigned int i = 0; i < _mLeft.size(); i++)
    {
        for (unsigned int j = 0; j < _mRight[0].size(); j++)
        {
            for (unsigned int k = 0; k < _mRight.size(); k++)
            {
                dEntry += _mLeft[i][k]*_mRight[k][j];
            }
            vLine.push_back(dEntry);
            dEntry = 0.0;
        }
        _mResult.push_back(vLine);
        vLine.clear();
    }
    //cerr << "returning" << endl;
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
    Matrix _mTransposed;
    vector<double> vLine;
    for (unsigned int j = 0; j < _mMatrix[0].size(); j++)
    {
        for (unsigned int i = 0; i < _mMatrix.size(); i++)
            vLine.push_back(_mMatrix[i][j]);
        _mTransposed.push_back(vLine);
        vLine.clear();
    }
    return _mTransposed;
}


/////////////////////////////////////////////////
/// \brief This static function returns an
/// identity matrix of the defined size.
///
/// \param nSize unsigned int
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_IdentityMatrix(unsigned int nSize)
{
    if (!nSize)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, "", SyntaxError::invalid_position);

    Matrix _mIdentity;
    vector<double> vLine;
    for (unsigned int i = 0; i < nSize; i++)
    {
        for (unsigned int j = 0; j < nSize; j++)
        {
            if (i == j)
                vLine.push_back(1.0);
            else
                vLine.push_back(0.0);
        }
        _mIdentity.push_back(vLine);
        vLine.clear();
    }
    return _mIdentity;
}


/////////////////////////////////////////////////
/// \brief This static function returns a matrix
/// filled with ones with the defined lines and
/// columns.
///
/// \param nLines unsigned int
/// \param nCols unsigned int
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_OnesMatrix(unsigned int nLines, unsigned int nCols)
{
    if (!nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, "", SyntaxError::invalid_position);

    Matrix _mOnes;
    vector<double> vLine(nCols, 1.0);
    for (unsigned int i = 0; i < nLines; i++)
    {
        _mOnes.push_back(vLine);
    }
    return _mOnes;
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

    Matrix _mZeroes;
    vector<double> vLine(nCols, 0.0);
    for (unsigned int i = 0; i < nLines; i++)
        _mZeroes.push_back(vLine);
    return _mZeroes;
}


/////////////////////////////////////////////////
/// \brief This static function creates a
/// shuffled vector of nShuffle elements created
/// from a 1:nBase vector.
///
/// \param nShuffle unsigned int
/// \param nBase unsigned int
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_shuffleMatrix(unsigned int nShuffle, unsigned int nBase)
{
    if (!nBase)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, "", SyntaxError::invalid_position);

    Matrix _mBase = createZeroesMatrix(nBase, 1);
    static double dSeed = 1;

    if (nShuffle > nBase)
        nShuffle = nBase;

    // Create the base (unshuffled) vector
    for (size_t i = 0; i < nBase; i++)
        _mBase[i][0] = i+1;

    // Initialize the random number engine using the
    // time and the last random number created by the
    // random engine
    default_random_engine randGen((double)time(0)*dSeed);

    // Shuffle the vector by swapping the i-th shuffled
    // element with the i-th element
    for (size_t i = 0; i < nShuffle; i++)
    {
        uniform_real_distribution<double> randDist(i, nBase-1);

        int nIndex = rint(randDist(randGen));
        double dTemp = _mBase[i][0];
        _mBase[i][0] = _mBase[nIndex][0];
        _mBase[nIndex][0] = dTemp;
    }

    uniform_real_distribution<double> randDist(1, nBase-1);

    // Update the seed
    dSeed = randDist(randGen);

    // Return only the requested vector length
    return Matrix(_mBase.begin(), _mBase.begin()+nShuffle);
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// inverse of the passed matrix, if it exists.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_InvertMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    // Gauss-Elimination???
    Matrix _mInverse = parser_IdentityMatrix(_mMatrix.size());
    Matrix _mToInvert = _mMatrix;

    //Spezialfaelle mit analytischem Ausdruck
    if (_mMatrix.size() == 1)
    {
        // eigentlich nicht zwangslaeufig existent... aber skalar und so...
        _mInverse[0][0] /= _mMatrix[0][0];
        return _mInverse;
    }
    else if (_mMatrix.size() == 2)
    {
        double dDet = _mToInvert[0][0]*_mToInvert[1][1] - _mToInvert[1][0]*_mToInvert[0][1];
        if (!dDet)
            throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, "", SyntaxError::invalid_position);
        _mInverse = _mToInvert;
        _mInverse[0][0] = _mToInvert[1][1];
        _mInverse[1][1] = _mToInvert[0][0];
        _mInverse[1][0] *= -1.0;
        _mInverse[0][1] *= -1.0;
        for (unsigned int i = 0; i < 2; i++)
        {
            _mInverse[i][0] /= dDet;
            _mInverse[i][1] /= dDet;
        }
        return _mInverse;
    }
    else if (_mMatrix.size() == 3)
    {
        double dDet = _mMatrix[0][0]*(_mMatrix[1][1]*_mMatrix[2][2] - _mMatrix[2][1]*_mMatrix[1][2])
                        - _mMatrix[0][1]*(_mMatrix[1][0]*_mMatrix[2][2] - _mMatrix[1][2]*_mMatrix[2][0])
                        + _mMatrix[0][2]*(_mMatrix[1][0]*_mMatrix[2][1] - _mMatrix[1][1]*_mMatrix[2][0]);
        if (!dDet)
            throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, "", SyntaxError::invalid_position);
        _mInverse[0][0] = (_mMatrix[1][1]*_mMatrix[2][2] - _mMatrix[2][1]*_mMatrix[1][2]) / dDet;
        _mInverse[1][0] = -(_mMatrix[1][0]*_mMatrix[2][2] - _mMatrix[1][2]*_mMatrix[2][0]) / dDet;
        _mInverse[2][0] = (_mMatrix[1][0]*_mMatrix[2][1] - _mMatrix[1][1]*_mMatrix[2][0]) / dDet;

        _mInverse[0][1] = -(_mMatrix[0][1]*_mMatrix[2][2] - _mMatrix[0][2]*_mMatrix[2][1]) / dDet;
        _mInverse[1][1] = (_mMatrix[0][0]*_mMatrix[2][2] - _mMatrix[0][2]*_mMatrix[2][0]) / dDet;
        _mInverse[2][1] = -(_mMatrix[0][0]*_mMatrix[2][1] - _mMatrix[0][1]*_mMatrix[2][0]) / dDet;

        _mInverse[0][2] = (_mMatrix[0][1]*_mMatrix[1][2] - _mMatrix[0][2]*_mMatrix[1][1]) / dDet;
        _mInverse[1][2] = -(_mMatrix[0][0]*_mMatrix[1][2] - _mMatrix[0][2]*_mMatrix[1][0]) / dDet;
        _mInverse[2][2] = (_mMatrix[0][0]*_mMatrix[1][1] - _mMatrix[0][1]*_mMatrix[1][0]) / dDet;

        return _mInverse;
    }

    // Allgemeiner Fall fuer n > 3
    for (unsigned int j = 0; j < _mToInvert.size(); j++)
    {
        for (unsigned int i = j; i < _mToInvert.size(); i++)
        {
            if (_mToInvert[i][j] != 0.0)
            {
                if (i != j) //vertauschen
                {
                    double dElement;
                    for (unsigned int _j = 0; _j < _mToInvert.size(); _j++)
                    {
                        dElement = _mToInvert[i][_j];
                        _mToInvert[i][_j] = _mToInvert[j][_j];
                        _mToInvert[j][_j] = dElement;
                        dElement = _mInverse[i][_j];
                        _mInverse[i][_j] = _mInverse[j][_j];
                        _mInverse[j][_j] = dElement;
                    }
                    i = j-1;
                }
                else //Gauss-Elimination
                {
                    double dPivot = _mToInvert[i][j];
                    for (unsigned int _j = 0; _j < _mToInvert.size(); _j++)
                    {
                        _mToInvert[i][_j] /= dPivot;
                        _mInverse[i][_j] /= dPivot;
                    }
                    for (unsigned int _i = i+1; _i < _mToInvert.size(); _i++)
                    {
                        double dFactor = _mToInvert[_i][j];
                        if (!dFactor) // Bereits 0???
                            continue;
                        for (unsigned int _j = 0; _j < _mToInvert.size(); _j++)
                        {
                            _mToInvert[_i][_j] -= _mToInvert[i][_j]*dFactor;
                            _mInverse[_i][_j] -= _mInverse[i][_j]*dFactor;
                        }
                    }
                    break;
                }
            }
            /*else if (_mToInvert[i][j] == 0.0 && j+1 == _mToInvert.size()) // Matrix scheint keinen vollen Rang zu besitzen
                throw MATRIX_IS_NOT_INVERTIBLE;*/
        }
    }
    // die Matrix _mToInvert() sollte nun Dreiecksgestalt besitzen. Jetzt den Gauss von unten her umkehren
    for (int j = (int)_mToInvert.size()-1; j >= 0; j--)
    {
        if (_mToInvert[j][j] == 0.0) // Hauptdiagonale ist ein Element == 0??
            throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, "", SyntaxError::invalid_position);
        if (_mToInvert[j][j] != 1.0)
        {
            for (unsigned int _j = 0; _j < _mInverse.size(); _j++)
                _mInverse[j][_j] /= _mToInvert[j][j];
            _mToInvert[j][j] = 1.0;
        }
        for (int i = 0; i < j; i++)
        {
            for (unsigned int _j = 0; _j < _mInverse.size(); _j++)
                _mInverse[i][_j] -= _mToInvert[i][j]*_mInverse[j][_j];
            _mToInvert[i][j] -= _mToInvert[i][j]*_mToInvert[j][j];
        }
    }

    return _mInverse;
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
static Matrix parser_matFromCols(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    return transposeMatrix(parser_matFromLines(sCmd, _parser, _data, _functions, _option));
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
static Matrix parser_matFromColsFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    return transposeMatrix(parser_matFromLinesFilled(sCmd, _parser, _data, _functions, _option));
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
static Matrix parser_matFromLines(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _matfl;
    value_type* v = 0;
    int nResults = 0;
    unsigned int nLineLength = 0;
    vector<double> vLine;
    if (!sCmd.length())
    {
        _matfl.push_back(vector<double>(1,NAN));
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
        _matfl.push_back(vector<double>(1,NAN));
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
static Matrix parser_matFromLinesFilled(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _matfl;
    value_type* v = 0;
    int nResults = 0;
    unsigned int nLineLength = 0;
    vector<double> vLine;
    if (!sCmd.length())
    {
        _matfl.push_back(vector<double>(1,NAN));
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
        _matfl.push_back(vector<double>(1,NAN));
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
            vector<double> vDeltas = parser_calcDeltas(_matfl, i);
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
/// \return vector<double>
///
/// This function is used by the \c matf*f()
/// functions to derive the filling logic.
/////////////////////////////////////////////////
static vector<double> parser_calcDeltas(const Matrix& _mMatrix, unsigned int nLine)
{
    vector<double> vDeltas;
    for (unsigned int j = 1; j < _mMatrix[nLine].size(); j++)
    {
        vDeltas.push_back(_mMatrix[nLine][j]-_mMatrix[nLine][j-1]);
    }
    return vDeltas;
}


/////////////////////////////////////////////////
/// \brief This static function creates a
/// diagonal matrix with the passed elements in
/// its main diagonal.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_diagonalMatrix(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
{
    Matrix _diag;
    value_type* v = 0;
    int nResults = 0;
    vector<double> vLine;
    if (!sCmd.length())
    {
        _diag.push_back(vector<double>(1,NAN));
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
        for (int n = 0; n < nResults; n++)
            vLine.push_back(v[n]);
    }
    if (!vLine.size())
    {
        _diag.push_back(vector<double>(1,NAN));
    }

    if (!vLine.size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, "", SyntaxError::invalid_position);

    _diag = createZeroesMatrix(vLine.size(),vLine.size());

    for (unsigned int i = 0; i < _diag.size(); i++)
    {
        _diag[i][i] = vLine[i];
    }

    return _diag;
}


/////////////////////////////////////////////////
/// \brief This static function returns the
/// determinant of the passed matrix.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_getDeterminant(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = parser_IdentityMatrix(1);
    vector<int> vRemovedLines(_mMatrix.size(), 0);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

    _mReturn[0][0] = parser_calcDeterminant(_mMatrix, vRemovedLines);
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// determinant of the passed matrix using the
/// LaPlace algorithm.
///
/// \param _mMatrix const Matrix&
/// \param vRemovedLines vector<int>
/// \return double
///
/////////////////////////////////////////////////
static double parser_calcDeterminant(const Matrix& _mMatrix, vector<int> vRemovedLines)
{
    // simple Sonderfaelle
    if (_mMatrix.size() == 1)
        return _mMatrix[0][0];
    if (_mMatrix.size() == 2)
        return _mMatrix[0][0] * _mMatrix[1][1] - _mMatrix[1][0]*_mMatrix[0][1];
    if (_mMatrix.size() == 3)
    {
        return _mMatrix[0][0]*_mMatrix[1][1]*_mMatrix[2][2]
            + _mMatrix[0][1]*_mMatrix[1][2]*_mMatrix[2][0]
            + _mMatrix[0][2]*_mMatrix[1][0]*_mMatrix[2][1]
            - _mMatrix[0][2]*_mMatrix[1][1]*_mMatrix[2][0]
            - _mMatrix[0][1]*_mMatrix[1][0]*_mMatrix[2][2]
            - _mMatrix[0][0]*_mMatrix[1][2]*_mMatrix[2][1];
    }
    int nSign = 1;
    double dDet = 0.0;
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        // Noch nicht entfernte Zeile?
        if (!(vRemovedLines[i] & 1))
        {
            // entferne Zeile i
            vRemovedLines[i] += 1;
            for (unsigned int j = 0; j < _mMatrix.size(); j++)
            {
                // Noch nicht entfernte Spalte?
                if (!(vRemovedLines[j] & 2))
                {
                    if (_mMatrix[i][j] == 0.0)
                    {
                        nSign *= -1;
                        continue;
                    }
                    // entferne Spalte j
                    vRemovedLines[j] += 2;
                    // Berechne Determinante rekursiv
                    if (i+1 < _mMatrix.size())
                        dDet += nSign * _mMatrix[i][j] * parser_calcDeterminant(_mMatrix, vRemovedLines);
                    else
                        dDet += nSign * _mMatrix[i][j];
                    // füge Spalte j wieder hinzu
                    vRemovedLines[j] -= 2;
                    // alternierendes Vorzeichen
                    nSign *= -1;
                }
            }
            vRemovedLines[i] -= 1;
            return dDet;
        }
    }
    return 1.0;
}


/////////////////////////////////////////////////
/// \brief This static function converts logical
/// vectors into matrix indices.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixLogToIndex(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    vector<int> vLines;
    vector<int> vRows;

    if (_mMatrix.size() == 1 || _mMatrix[0].size() == 1)
    {
        for (size_t i = 0; i < _mMatrix.size(); i++)
        {
            for (size_t j = 0; j < _mMatrix[0].size(); j++)
            {
                if (_mMatrix[i][j])
                {
                    vLines.push_back(i + j + 1);
                }
            }
        }
        if (!vLines.size())
            return createZeroesMatrix(1,1);
        Matrix _mReturn = createZeroesMatrix(vLines.size(), 1);
        for (size_t i = 0; i < vLines.size(); i++)
            _mReturn[i][0] = vLines[i];
        return _mReturn;
    }
    else
    {
        for (size_t i = 0; i < _mMatrix.size(); i++)
        {
            for (size_t j = 0; j < _mMatrix[0].size(); j++)
            {
                if (_mMatrix[i][j])
                {
                    vLines.push_back(i+1);
                    vRows.push_back(j+1);
                }
            }
        }
        if (!vLines.size())
            return createZeroesMatrix(1,1);
        Matrix _mReturn = createZeroesMatrix(vLines.size(), 2);
        for (size_t i = 0; i < vLines.size(); i++)
        {
            _mReturn[i][0] = vLines[i];
            _mReturn[i][1] = vRows[i];
        }
        return _mReturn;
    }
}


/////////////////////////////////////////////////
/// \brief This static function converts matrix
/// indices into logical vectors.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixIndexToLog(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mMatrix.size() == 1 || _mMatrix[0].size() == 1)
    {
        Matrix _mMatrixMax = parser_MatrixMax(_mMatrix, sCmd, sExpr, position);
        if (!_mMatrixMax[0][0] || _mMatrixMax[0][0] < 0)
            return createZeroesMatrix(1,1);
        Matrix _mReturn = createZeroesMatrix(_mMatrixMax[0][0], 1);
        for (size_t i = 0; i < _mMatrix.size(); i++)
        {
            for (size_t j = 0; j < _mMatrix[0].size(); j++)
            {
                if (_mMatrix[i][j] > 0)
                {
                    _mReturn[_mMatrix[i][j]-1][0] = 1.0;
                }
            }
        }
        return _mReturn;
    }
    else
    {
        vector<int> vCol;
        vector<int> vRow;

        if (_mMatrix.size() == 2 && _mMatrix[0].size() != 2)
        {
            for (size_t i = 0; i < _mMatrix[0].size(); i++)
            {
                vRow.push_back(_mMatrix[0][i]);
                vCol.push_back(_mMatrix[1][i]);
            }
        }
        else if (_mMatrix.size() != 2 && _mMatrix[0].size() == 2)
        {
            for (size_t i = 0; i < _mMatrix.size(); i++)
            {
                vRow.push_back(_mMatrix[i][0]);
                vCol.push_back(_mMatrix[i][1]);
            }
        }
        else
            return createZeroesMatrix(1,1);

        int nRowMax = 0;
        int nColMax = 0;

        for (size_t i = 0; i < vRow.size(); i++)
        {
            if (nRowMax < (int)vRow[i])
                nRowMax = (int)vRow[i];
            if (nColMax < (int)vCol[i])
                nColMax = (int)vCol[i];
        }

        if (!nColMax || !nRowMax)
            return createZeroesMatrix(1,1);

        Matrix _mReturn = createZeroesMatrix(nRowMax, nColMax);

        for (size_t i = 0; i < vRow.size(); i++)
        {
            if (vRow[i] > 0 && vCol[i] > 0)
                _mReturn[vRow[i]-1][vCol[i]-1] = 1.0;
        }

        return _mReturn;
    }
}


/////////////////////////////////////////////////
/// \brief This static function returns the size
/// of the passed matrix.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixSize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(2,1);
    if (_mMatrix.size() == 1 && _mMatrix[0].size() == 1 && isnan(_mMatrix[0][0]))
        return _mReturn;
    _mReturn[0][0] = _mMatrix.size();
    _mReturn[1][0] = _mMatrix[0].size();
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c and() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixAnd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = parser_IdentityMatrix(1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[i].size(); j++)
        {
            if (!_mMatrix[i][j])
            {
                _mReturn[0][0] = 0;
                return _mReturn;
            }
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c or() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixOr(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[i].size(); j++)
        {
            if (_mMatrix[i][j])
            {
                _mReturn[0][0] = 1;
                return _mReturn;
            }
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c xor() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixXor(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    bool isTrue = false;
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[i].size(); j++)
        {
            if (_mMatrix[i][j])
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return _mReturn;
            }
        }
    }
    if (isTrue)
        _mReturn[0][0] = 1;
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c sum() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixSum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!isnan(_mMatrix[i][j]))
                _mReturn[0][0] += _mMatrix[i][j];
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c std() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixStd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    Matrix _mAvg = parser_MatrixAvg(_mMatrix, sCmd, sExpr, position);
    Matrix _mNum = parser_MatrixNum(_mMatrix, sCmd, sExpr, position);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!isnan(_mMatrix[i][j]))
                _mReturn[0][0] += (_mMatrix[i][j] - _mAvg[0][0])*(_mMatrix[i][j] - _mAvg[0][0]);
        }
    }
    _mReturn[0][0] = sqrt(_mReturn[0][0])/(_mNum[0][0]-1);
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c avg() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixAvg(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    Matrix _mSum = parser_MatrixSum(_mMatrix, sCmd, sExpr, position);
    Matrix _mNum = parser_MatrixNum(_mMatrix, sCmd, sExpr, position);
    _mReturn[0][0] = _mSum[0][0] / _mNum[0][0];
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c prd() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixPrd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = parser_IdentityMatrix(1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!isnan(_mMatrix[i][j]))
                _mReturn[0][0] *= _mMatrix[i][j];
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c cnt() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixCnt(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    _mReturn[0][0] = _mMatrix.size() * _mMatrix[0].size();
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c num() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixNum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!isnan(_mMatrix[i][j]))
                _mReturn[0][0] += 1;
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c norm() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixNorm(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!isnan(_mMatrix[i][j]))
                _mReturn[0][0] += _mMatrix[i][j]*_mMatrix[i][j];
        }
    }
    _mReturn[0][0] = sqrt(_mReturn[0][0]);
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c min() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixMin(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!i && !j)
                _mReturn[0][0] = _mMatrix[i][j];
            else if (_mMatrix[i][j] < _mReturn[0][0] || isnan(_mReturn[0][0]))
                _mReturn[0][0] = _mMatrix[i][j];
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c max() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixMax(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (!i && !j)
                _mReturn[0][0] = _mMatrix[i][j];
            else if (_mMatrix[i][j] > _mReturn[0][0] || isnan(_mReturn[0][0]))
                _mReturn[0][0] = _mMatrix[i][j];
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c minpos() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixMinPos(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    return parser_MatrixCmp(_mMatrix, parser_MatrixMin(_mMatrix, sCmd, sExpr, position)[0][0], 0, sCmd, sExpr, position);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c maxpos() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixMaxPos(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    return parser_MatrixCmp(_mMatrix, parser_MatrixMax(_mMatrix, sCmd, sExpr, position)[0][0], 0, sCmd, sExpr, position);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c med() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixMed(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    Memory _mem;

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            _mem.writeData(j + i*_mMatrix.size(), 0, _mMatrix[i][j]);
        }
    }

    _mReturn[0][0] = _mem.med(VectorIndex(0, _mMatrix.size()*_mMatrix[0].size()-1), VectorIndex(0));
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c pct() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param dPercentage double
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixPct(const Matrix& _mMatrix, double dPercentage, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    Memory _mem;

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            _mem.writeData(j + i*_mMatrix.size(), 0, _mMatrix[i][j]);
        }
    }

    _mReturn[0][0] = _mem.pct(VectorIndex(0, (long long int)(_mMatrix.size()*_mMatrix[0].size())-1), VectorIndex(0), dPercentage);
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c cmp() function on the matrix elements.
///
/// \param _mMatrix const Matrix&
/// \param dValue double
/// \param nType int
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixCmp(const Matrix& _mMatrix, double dValue, int nType, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(1,1);
    Matrix _mCoords = createZeroesMatrix(2,1);
    _mCoords[0][0] = -1;

    double dKeep = dValue;

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            if (isnan(_mMatrix[i][j]) || isinf(_mMatrix[i][j]))
                continue;
            if (_mMatrix[i][j] == dValue)
            {
                if (!nType || abs(nType) <= 1)
                {
                    _mCoords[0][0] = i+1;
                    _mCoords[1][0] = j+1;
                    return _mCoords;
                }
                else
                    _mReturn[0][0] = _mMatrix[i][j];
                return _mReturn;
            }
            else if (nType > 0 && _mMatrix[i][j] > dValue)
            {
                if (_mCoords[0][0] == -1 || _mMatrix[i][j] < dKeep)
                {
                    dKeep = _mMatrix[i][j];
                    _mCoords[0][0] = i+1;
                    _mCoords[1][0] = j+1;
                }
                else
                    continue;
            }
            else if (nType < 0 && _mMatrix[i][j] < dValue)
            {
                if (_mCoords[0][0] == -1 || _mMatrix[i][j] > dKeep)
                {
                    dKeep = _mMatrix[i][j];
                    _mCoords[0][0] = i+1;
                    _mCoords[1][0] = j+1;
                }
                else
                    continue;
            }
        }
    }
    if (_mCoords[0][0] == -1)
        _mReturn[0][0] = NAN;
    else if (nType <= -2 || nType >= 2)
        _mReturn[0][0] = dKeep;
    else
    {
        return _mCoords;
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// cross- and auto-correlation matrix
/// calculation from the passed two matrices.
///
/// \param _mMatrix1 const Matrix&
/// \param _mMatrix2 const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/// If their sizes are not fitting, they are
/// resized to fit.
/////////////////////////////////////////////////
static Matrix parser_Correlation(const Matrix& _mMatrix1, const Matrix& _mMatrix2, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix1.size() || !_mMatrix1[0].size() || !_mMatrix2.size() || !_mMatrix2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    // Ensure that the size is non-zero
    if (!(_mMatrix1.size() && _mMatrix2.size()) || !(_mMatrix1[0].size() && _mMatrix2[0].size()))
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix1.size()) +"x"+ toString(_mMatrix1[0].size()) + ", " + toString(_mMatrix2.size()) +"x"+ toString(_mMatrix2[0].size()));

    // Resize the matrices to fit their counterparts
    Matrix mMatrix1 = parser_MatrixResize(_mMatrix1, max(_mMatrix1.size(), _mMatrix2.size()), max(_mMatrix1[0].size(), _mMatrix2[0].size()), sCmd, sExpr, position);
    Matrix mMatrix2 = parser_MatrixResize(_mMatrix2, max(_mMatrix1.size(), _mMatrix2.size()), max(_mMatrix1[0].size(), _mMatrix2[0].size()), sCmd, sExpr, position);

    int n = mMatrix1.size();
    int m = mMatrix1[0].size();

    // Create the target matrix
    Matrix mCorrelation = createZeroesMatrix(2*n-1, 2*m-1);

    // Calculate the elements of the matrix by applying
    // elementwise shifts to the matrices
    for (int i1 = 0; i1 < (int)mCorrelation.size(); i1++)
    {
        for (int j1 = 0; j1 < (int)mCorrelation[0].size(); j1++)
        {
            // These loops shall indicate the number of elements
            for (int i2 = 0; i2 < n + min(i1-n+1, n-i1-1); i2++)
            {
                for (int j2 = 0; j2 < m + min(j1-m+1, m-j1-1); j2++)
                {
                    // calculate the correlation of the current
                    // shift indicated by the other two loops
                    mCorrelation[i1][j1] += mMatrix1[i2 + max(0, i1-n+1)][j2 + max(0, j1-m+1)] * mMatrix2[i2 + max(0, n-i1-1)][j2 + max(0, m-j1-1)];
                }
            }
        }
    }

    return mCorrelation;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// covariance calculation of the passed two
/// matrices.
///
/// \param _mMatrix1 const Matrix&
/// \param _mMatrix2 const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_Covariance(const Matrix& _mMatrix1, const Matrix& _mMatrix2, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix1.size() || !_mMatrix1[0].size() || !_mMatrix2.size() || !_mMatrix2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    // Ensure that their size is equal
    if (_mMatrix1.size() != _mMatrix2.size() || _mMatrix1[0].size() != _mMatrix2[0].size() || !_mMatrix1.size() || !_mMatrix1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix1.size()) +"x"+ toString(_mMatrix1[0].size()) + " != " + toString(_mMatrix2.size()) +"x"+ toString(_mMatrix2[0].size()));

    // Prepare the target matrix
    Matrix mCovariance = createZeroesMatrix(1, 1);

    // Calculate the average values of both
    // matrices
    Matrix mAvg1 = parser_MatrixAvg(_mMatrix1, sCmd, sExpr, position);
    Matrix mAvg2 = parser_MatrixAvg(_mMatrix2, sCmd, sExpr, position);

    // Calculate the covariance value for each
    // component and sum it up
    for (size_t i = 0; i < _mMatrix1.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix2.size(); j++)
        {
            mCovariance[0][0] += (_mMatrix1[i][j] - mAvg1[0][0]) * (_mMatrix2[i][j] - mAvg2[0][0]);
        }
    }

    // Normalize the covariance value using
    // the number of elements
    mCovariance[0][0] /= (_mMatrix1.size() * _mMatrix1[0].size() - 1);

    return mCovariance;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// normalize function, which will normalize the
/// (absolute) data range of the passed matrix
/// into the range [0,1].
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_Normalize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mReturn = _mMatrix;
    Matrix _mMax = parser_MatrixMax(_mMatrix, sCmd, sExpr, position);
    Matrix _mMin = parser_MatrixMin(_mMatrix, sCmd, sExpr, position);

    double dMax = max(fabs(_mMax[0][0]), fabs(_mMin[0][0]));

    for (size_t  i = 0; i < _mReturn.size(); i++)
    {
        for (size_t j = 0; j < _mReturn[i].size(); j++)
        {
            _mReturn[i][j] /= dMax;
        }
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function changes the
/// number of rows and columns to fit the new
/// shape.
///
/// \param _mMatrix const Matrix&
/// \param nLines size_t
/// \param nCols size_t
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/// The total number of elements before and
/// afterwards must be identical.
/////////////////////////////////////////////////
static Matrix parser_MatrixReshape(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (nLines * nCols != _mMatrix.size() * _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(nLines) + "x" + toString(nCols) + "=" + toString(nLines*nCols) +" vs. "+ toString(_mMatrix.size()) + "x" +  toString(_mMatrix[0].size()) + "=" + toString(_mMatrix.size()*_mMatrix[0].size()));
    Matrix _mReturn = createZeroesMatrix(nLines, nCols);
    for (size_t i = 0; i < nLines*nCols; i++)
    {
        _mReturn[i / nCols][i % nCols] = _mMatrix[i / _mMatrix[0].size()][i % _mMatrix[0].size()];
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function changes the size
/// of the passed matrix to fit the new size.
///
/// \param _mMatrix const Matrix&
/// \param nLines size_t
/// \param nCols size_t
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix parser_MatrixResize(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (nLines == _mMatrix.size() && nCols == _mMatrix[0].size())
        return _mMatrix;
    Matrix _mReturn = createZeroesMatrix(nLines, nCols);
    for (size_t i = 0; i < min(nLines, _mMatrix.size()); i++)
    {
        for (size_t j = 0; j < min(nCols, _mMatrix[0].size()); j++)
        {
            _mReturn[i][j] = _mMatrix[i][j];
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function repeats the
/// passed matrix n and m times.
///
/// \param _mMatrix const Matrix&
/// \param n size_t
/// \param m size_t
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixRepMat(const Matrix& _mMatrix, size_t n, size_t m, const string& sCmd, const string& sExpr, size_t position)
{
    if (n == 0)
        n++;

    if (m == 0)
        m++;

    if ((n * _mMatrix.size()) / _mMatrix.size() != n || (m * _mMatrix[0].size()) / _mMatrix[0].size() != m)
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, sCmd, position);

    Matrix _mReturn = createZeroesMatrix(n * _mMatrix.size(), m * _mMatrix[0].size());

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            for (size_t _n = 0; _n < n; _n++)
            {
                for (size_t _m = 0; _m < m; _m++)
                {
                    _mReturn[i+_n*_mMatrix.size()][j+_m*_mMatrix[0].size()] = _mMatrix[i][j];
                }
            }
        }
    }

    return _mReturn;
}

/////////////////////////////////////////////////
/// \brief This static function implements the
/// \c unique(MAT,nDim) function.
///
/// \param _mMatrix const Matrix&
/// \param nDim size_t
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_MatrixUnique(const Matrix& _mMatrix, size_t nDim, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    // Create a std::list and the return value
    std::list<double> dataList;
    Matrix _mReturn;

    // Depending on the dimensions of the passed matrix, change
    // the evaluation method
    if (_mMatrix.size() == 1)
    {
        // Row vector
        dataList.assign(_mMatrix[0].begin(), _mMatrix[0].end());
        _mReturn.push_back(parser_getUniqueList(dataList));
    }
    else if (_mMatrix[0].size() == 1)
    {
        // Column vector
        for (size_t i = 0; i < _mMatrix.size(); i++)
        {
            dataList.push_back(_mMatrix[i][0]);
        }
        _mReturn.push_back(parser_getUniqueList(dataList));
        _mReturn = transposeMatrix(_mReturn);
    }
    else
    {
        // Matrix
        if (!nDim)
        {
            // nDim == 0 -> Roll out the total matrix and return it as a overall row vector
            Matrix retVal = parser_MatrixReshape(_mMatrix, 1, _mMatrix.size()*_mMatrix[0].size(), sCmd, sExpr, position);
            dataList.assign(retVal[0].begin(), retVal[0].end());
            _mReturn.push_back(parser_getUniqueList(dataList));
        }
        else if (nDim == 1)
        {
            // Make the rows unique
            for (size_t i = 0; i < _mMatrix.size(); i++)
            {
                dataList.clear();
                dataList.assign(_mMatrix[i].begin(), _mMatrix[i].end());
                _mReturn.push_back(parser_getUniqueList(dataList));
            }
            parser_fillMissingMatrixElements(_mReturn);
        }
        else
        {
            // Make the columns unique
            for (size_t j = 0; j < _mMatrix[0].size(); j++)
            {
                dataList.clear();
                for (size_t i = 0; i < _mMatrix.size(); i++)
                    dataList.push_back(_mMatrix[i][j]);
                _mReturn.push_back(parser_getUniqueList(dataList));
            }
            parser_fillMissingMatrixElements(_mReturn);
            _mReturn = transposeMatrix(_mReturn);
        }
    }
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// std::list::remove_if() called in
/// parser_getUniqueList()
///
/// \param value const double&
/// \return bool
///
/////////////////////////////////////////////////
static bool is_nan(const double& value)
{
    return isnan(value);
}


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the implementation of the \c unique()
/// function.
///
/// \param _list std::list<double>&
/// \return std::vector<double>
///
/////////////////////////////////////////////////
static std::vector<double> parser_getUniqueList(std::list<double>& _list)
{
    _list.remove_if(is_nan);

    if (_list.empty())
        return std::vector<double>(1, NAN);

    _list.sort();
    _list.unique();
    std::vector<double> vReturn(_list.begin(), _list.end());
    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This is a static helper function,
/// which will add the elements in the rows of
/// the passed matrix.
///
/// \param _mMatrix Matrix&
/// \return void
///
/////////////////////////////////////////////////
static void parser_fillMissingMatrixElements(Matrix& _mMatrix)
{
    size_t lines = _mMatrix.size();
    size_t columns = _mMatrix[0].size();

    // Get the maximal column count
    for (size_t i = 0; i < lines; i++)
    {
        if (_mMatrix[i].size() > columns)
            columns = _mMatrix[i].size();
    }

    // Add the missing elements to all other
    // rows
    for (size_t i = 0; i < lines; i++)
    {
        while (_mMatrix[i].size() < columns)
            _mMatrix[i].push_back(NAN);
    }
}

// LGS-Loesung auf Basis des Invert-Algorthmuses
/////////////////////////////////////////////////
/// \brief This static function will solve the
/// system of linear equations passed as matrix
/// using the Gauss elimination algorithm.
///
/// \param _mMatrix const Matrix&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _option const Settings&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_solveLGS(const Matrix& _mMatrix, Parser& _parser, FunctionDefinitionManager& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mResult = createZeroesMatrix(_mMatrix[0].size()-1,1);
    Matrix _mToSolve = _mMatrix;

    if (_mMatrix.size() == 1)
    {
        _mResult[0][0] = _mMatrix[0][1]/_mMatrix[0][0];
        return _mResult;
    }

    // Allgemeiner Fall fuer n > 2
    for (unsigned int j = 0; j < _mToSolve[0].size()-1; j++)
    {
        for (unsigned int i = j; i < _mToSolve.size(); i++)
        {
            if (_mToSolve[i][j] != 0.0)
            {
                if (i != j) //vertauschen
                {
                    double dElement;
                    for (unsigned int _j = 0; _j < _mToSolve[0].size(); _j++)
                    {
                        dElement = _mToSolve[i][_j];
                        _mToSolve[i][_j] = _mToSolve[j][_j];
                        _mToSolve[j][_j] = dElement;
                    }
                    i = j-1;
                }
                else //Gauss-Elimination
                {
                    double dPivot = _mToSolve[i][j];
                    for (unsigned int _j = 0; _j < _mToSolve[0].size(); _j++)
                    {
                        _mToSolve[i][_j] /= dPivot;
                    }
                    for (unsigned int _i = i+1; _i < _mToSolve.size(); _i++)
                    {
                        double dFactor = _mToSolve[_i][j];
                        if (!dFactor) // Bereits 0???
                            continue;
                        for (unsigned int _j = 0; _j < _mToSolve[0].size(); _j++)
                        {
                            _mToSolve[_i][_j] -= _mToSolve[i][_j]*dFactor;
                        }
                    }
                    break;
                }
            }
            /*else if (_mToSolve[i][j] == 0.0 && j+1 == _mToSolve.size()) // Matrix scheint keinen vollen Rang zu besitzen
                throw MATRIX_IS_NOT_INVERTIBLE;*/
        }
    }

    if ((_mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-2] == 0.0 && _mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-1] == 0.0)
        || _mToSolve.size()+1 < _mToSolve[0].size())
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
        parser_solveLGSSymbolic(_mToSolve, _parser, _functions, _option, sCmd, sExpr, position);
        return _mToSolve;
    }
    else if (_mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-2] == 0.0 && _mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-1] != 0.0)
        throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, sCmd, position);
    else if ((_mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-2] == 0.0 && _mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-1] == 0.0)
        || _mToSolve.size()+1 > _mToSolve[0].size())
    {
        // Ggf. Nullzeilen nach unten tauschen
        vector<bool> vIsZerosLine(_mToSolve.size(),true);
        for (unsigned int i = 0; i < _mToSolve.size(); i++)
        {
            for (unsigned int j = 0; j < _mToSolve[0].size(); j++)
            {
                if (_mToSolve[i][j])
                {
                    vIsZerosLine[i] = false;
                    break;
                }
            }
        }
        for (unsigned int i = 0; i < vIsZerosLine.size(); i++)
        {
            if (vIsZerosLine[i])
            {
                for (unsigned int _i = i+1; _i < vIsZerosLine.size(); _i++)
                {
                    if (!vIsZerosLine[_i])
                    {
                        double dElement;
                        for (unsigned int _j = 0; _j < _mToSolve[0].size(); _j++)
                        {
                            dElement = _mToSolve[i][_j];
                            _mToSolve[i][_j] = _mToSolve[_i][_j];
                            _mToSolve[_i][_j] = dElement;
                        }
                        vIsZerosLine[i] = false;
                        vIsZerosLine[_i] = true;
                        break;
                    }
                }
            }
        }

        if (_mToSolve[_mToSolve[0].size()-2][_mToSolve[0].size()-2] == 0.0 && _mToSolve[_mToSolve[2].size()-2][_mToSolve[0].size()-1] == 0.0)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
            parser_solveLGSSymbolic(_mToSolve, _parser, _functions, _option, sCmd, sExpr, position);
            return _mToSolve;
        }
        _mResult[_mResult[0].size()-2][0] = _mToSolve[_mToSolve[0].size()-2][_mToSolve[0].size()-1];
    }
    else
        _mResult[_mResult.size()-1][0] = _mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-1];
    for (int i = _mToSolve[0].size()-3; i >= 0; i--)
    {
        for (unsigned int j = 0; j < _mToSolve[0].size()-1; j++)
        {
            _mToSolve[i][_mToSolve[0].size()-1] -= _mToSolve[i][j]*_mResult[j][0];
            if (_mToSolve[i][j]*_mResult[j][0])
                _mToSolve[i][j] = 0.0;
        }
        if (_mToSolve[i][i] == 0.0 && _mToSolve[i][_mToSolve[0].size()-1] == 0.0)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
            parser_solveLGSSymbolic(_mToSolve, _parser, _functions, _option, sCmd, sExpr, position);
            return _mToSolve;
        }
        if (_mToSolve[i][i] == 0.0 && _mToSolve[i][_mToSolve[0].size()-1] != 0.0)
            throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, sCmd, position);
        _mResult[i][0] = _mToSolve[i][_mToSolve[0].size()-1];
    }
    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function calulates the
/// n-dimensional cross product ("curl").
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_calcCrossProduct(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    Matrix _mResult = createZeroesMatrix(_mMatrix.size(),1);
    vector<int> vRemovedLines(_mMatrix.size(), 0);
    if (_mMatrix.size() == 1)
    {
        return _mResult;
    }
    if (_mMatrix.size()-1 != _mMatrix[0].size())
    {
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    }
    if (_mMatrix.size() == 2)
    {
        _mResult[0][0] = _mMatrix[0][1];
        _mResult[1][0] = -_mMatrix[0][0];
        return _mResult;
    }
    if (_mMatrix.size() == 3)
    {
        _mResult[0][0] = _mMatrix[1][0]*_mMatrix[2][1] - _mMatrix[2][0]*_mMatrix[1][1];
        _mResult[1][0] = _mMatrix[2][0]*_mMatrix[0][1] - _mMatrix[0][0]*_mMatrix[2][1];
        _mResult[2][0] = _mMatrix[0][0]*_mMatrix[1][1] - _mMatrix[1][0]*_mMatrix[0][1];
        return _mResult;
    }
    Matrix _mTemp = createZeroesMatrix(_mMatrix.size(), _mMatrix[0].size()+1);
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = 0; j < _mMatrix[0].size(); j++)
        {
            _mTemp[i][j+1] = _mMatrix[i][j];
        }
    }
    _mTemp[0][0] = 1.0;
    _mResult[0][0] = parser_calcDeterminant(_mTemp, vRemovedLines);

    for (unsigned int i = 1; i < _mMatrix.size(); i++)
    {
        _mTemp[i-1][0] = 0.0;
        _mTemp[i][0] = 1.0;
        _mResult[i][0] = parser_calcDeterminant(_mTemp, vRemovedLines);
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function does the whole
/// eigenvalues, eigenvectors and diagonalizing
/// stuff.
///
/// \param _mMatrix const Matrix&
/// \param nReturnType int
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/// If the results are complex then the real and
/// imaginary parts of the result are returned as
/// separate results:
/// \li for eigenvalues it's two columns of the
/// returned matrix
/// \li for eigenvectors or the diagonal matrix
/// it's a matrix with 2N columns, where the
/// imaginary part may be found in the columns
/// N+1 - 2N
///
/// \c __attribute__((force_align_arg_pointer))
/// fixes TDM-GCC Bug for wrong stack alignment.
/////////////////////////////////////////////////
__attribute__((force_align_arg_pointer)) static Matrix parser_calcEigenVects(const Matrix& _mMatrix, int nReturnType, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    Matrix _mEigenVals;
    Matrix _mEigenVects;

    Eigen::MatrixXd mMatrix(_mMatrix.size(), _mMatrix.size());

    // Copy the passed matrix into an Eigen matrix
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = 0; j < _mMatrix.size(); j++)
        {
            mMatrix(i,j) = _mMatrix[i][j];
        }
    }

    // For symmetric matrices the return value is always real
    // This is not true for asymmetric matrices
    if (parser_IsSymmMatrix(_mMatrix, sCmd, sExpr, position))
    {
        // Prepare return values
        _mEigenVals = createZeroesMatrix(_mMatrix.size(),1);
        _mEigenVects = createZeroesMatrix(_mMatrix.size(), _mMatrix.size());

        // Construct an Eigen eigenvalue solver
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eSolver(mMatrix);

        // Get eigenvalues, eigenvectors or the diagonal matrix depending
        // on the selected return type
        if (nReturnType == EIGENVALUES)
        {
            Eigen::VectorXd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVals.size(); i++)
            {
                _mEigenVals[i][0] = vEigenVals(i,0);
            }
        }
        else if (nReturnType == EIGENVECTORS)
        {
            Eigen::MatrixXd mEigenVects = eSolver.eigenvectors();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                for (unsigned int j = 0; j < _mEigenVects.size(); j++)
                {
                    _mEigenVects[i][j] = mEigenVects(i,j);
                }
            }
        }
        else if (nReturnType == DIAGONALIZE)
        {
            Eigen::VectorXd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                _mEigenVects[i][i] = vEigenVals(i,0);
            }
        }
    }
    else
    {
        // Prepare return values
        _mEigenVals = createZeroesMatrix(_mMatrix.size(),2);
        _mEigenVects = createZeroesMatrix(_mMatrix.size(), 2*_mMatrix.size());

        // Construct an Eigen eigenvalue solver
        Eigen::EigenSolver<Eigen::MatrixXd> eSolver(mMatrix);

        // Get eigenvalues, eigenvectors or the diagonal matrix depending
        // on the selected return type. Separate the result into real and
        // imaginary parts
        if (nReturnType == EIGENVALUES)
        {
            Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVals.size(); i++)
            {
                _mEigenVals[i][0] = real(vEigenVals(i,0));
                _mEigenVals[i][1] = imag(vEigenVals(i,0));
            }
            parser_makeReal(_mEigenVals);
        }
        else if (nReturnType == EIGENVECTORS)
        {
            Eigen::MatrixXcd mEigenVects = eSolver.eigenvectors();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                for (unsigned int j = 0; j < _mEigenVects.size(); j++)
                {
                    _mEigenVects[i][j] = real(mEigenVects(i, j));
                    _mEigenVects[i][j+_mEigenVects.size()] = imag(mEigenVects(i, j));
                }
            }
            parser_makeReal(_mEigenVects);
        }
        else if (nReturnType == DIAGONALIZE)
        {
            Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                _mEigenVects[i][i] = real(vEigenVals(i, 0));
                _mEigenVects[i][i+_mEigenVects.size()] = imag(vEigenVals(i, 0));
            }
            parser_makeReal(_mEigenVects);
        }
    }

    // Return the corresponding result
    if (nReturnType == EIGENVALUES)
        return _mEigenVals;
    else
        return _mEigenVects;
}


/////////////////////////////////////////////////
/// \brief This static function tries to remove
/// the imaginary part of the returned
/// eigenvalues.
///
/// \param _mMatrix Matrix&
/// \return void
///
/////////////////////////////////////////////////
static void parser_makeReal(Matrix& _mMatrix)
{
    if (_mMatrix[0].size() < 2 || (_mMatrix[0].size() % 2))
        return;

    // Try to find a non-zero imaginary value
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        // imaginary values are found in the second half of the columns
        for (unsigned int j = _mMatrix[0].size()/2; j < _mMatrix[0].size(); j++)
        {
            if (_mMatrix[i][j])
                return;
        }
    }

    // The matrix is completely real, remove the unnecessary empty columns
    if (_mMatrix[0].size() == 2)
    {
        for (unsigned int i = 0; i < _mMatrix.size(); i++)
            _mMatrix[i].pop_back();
    }
    else
    {
        for (unsigned int i = 0; i < _mMatrix.size(); i++)
        {
            _mMatrix[i].erase(_mMatrix[i].begin() + _mMatrix[i].size()/2, _mMatrix[i].end());
        }
    }
    return;
}


/////////////////////////////////////////////////
/// \brief This static function determines,
/// whether the passed matrix is symmetric or not.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return bool
///
/////////////////////////////////////////////////
static bool parser_IsSymmMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

    // Try to find a value, which is not symmetric
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = i; j < _mMatrix.size(); j++)
        {
            // Is this value not symmetric?
            if (_mMatrix[i][j] != _mMatrix[j][i])
                return false;
        }
    }

    // Is symmetric
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// trace of the passed matrix.
///
/// \param _mMatrix const Matrix&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix parser_calcTrace(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    Matrix _mReturn = createZeroesMatrix(1,1);
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        _mReturn[0][0] += _mMatrix[i][i];
    }
    return _mReturn;
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
static Matrix parser_getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option)
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
            if (_idx.row[i] >= _mMatrix.size() || _idx.col[j] >= _mMatrix[0].size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);

            _mReturn[i][j] = _mMatrix[_idx.row[i]][_idx.col[j]];
        }
    }

    return _mReturn;
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
static void parser_ShowMatrixResult(const Matrix& _mResult, const Settings& _option)
{
    if (!_option.systemPrints() || NumeReKernel::bSupressAnswer)
        return;
    //(_option.getWindow()-1-15) / (_option.getPrecision()+9) _mResult.size() > (_option.getWindow()-1-15) / (4+9)
    NumeReKernel::toggleTableStatus();
    if (_mResult.size() > 10)
    {
        for (unsigned int i = 0; i < _mResult.size(); i++)
        {
            if (!i)
            {
                NumeReKernel::printPreFmt("|   /");
            }
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
                    if (_mResult[0].size() > (_option.getWindow()-2-15) / (4+9)
                        && (_option.getWindow()-2-15) / (4+9) / 2 == j)
                    {
                        NumeReKernel::printPreFmt(strfill("..., ", 11));
                        ///cerr << std::setfill(' ') << std::setw(11) << "..., ";
                        j = _mResult[0].size() - (_option.getWindow()-2-15) / (4+9) / 2 - 1;
                        continue;
                    }
                    NumeReKernel::printPreFmt(strfill("...", 11));
                    ///cerr << std::setfill(' ') << std::setw(11) << "...";
                    if (j+1 < _mResult[0].size())
                        NumeReKernel::printPreFmt(", ");
                        ///cerr << ", ";
                }
                i = _mResult.size()-6;
            }
            else
            {
                for (unsigned int j = 0; j < _mResult[0].size(); j++)
                {
                    if (_mResult[0].size() > (_option.getWindow()-2-15) / (4+9)
                        && (_option.getWindow()-2-15) / (4+9) / 2 == j)
                    {
                        NumeReKernel::printPreFmt(strfill("..., ", 11));
                        ///cerr << std::setfill(' ') << std::setw(11) << "..., ";
                        j = _mResult[0].size() - (_option.getWindow()-2-15) / (4+9) / 2 - 1;
                        continue;
                    }
                    NumeReKernel::printPreFmt(strfill(toString(_mResult[i][j], 4), 11));
                    ///cerr << std::setfill(' ') << std::setw(11) << toString(_mResult[i][j],4);
                    if (j+1 < _mResult[0].size())
                        NumeReKernel::printPreFmt(", ");
                        ///cerr << ", ";
                }
            }
            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.size())
            {
                NumeReKernel::printPreFmt(" /\n");
            }
            else
                NumeReKernel::printPreFmt(" |\n");
        }
    }
    else if (_mResult.size() == 1)
    {
        if (_mResult[0].size() == 1)
            NumeReKernel::print("(" + toString(_mResult[0][0], _option) + ")");
        else
        {
            NumeReKernel::printPreFmt("|-> (");
            for (unsigned int i = 0; i < _mResult[0].size(); i++)
            {
                if (_mResult[0].size() > (_option.getWindow()-2-15) / (4+9)
                    && (_option.getWindow()-2-15) / (4+9) / 2 == i)
                {
                    NumeReKernel::printPreFmt(strfill("..., ", 11));
                    ///cerr << std::setfill(' ') << std::setw(11) << "..., ";
                    i = _mResult[0].size() - (_option.getWindow()-2-15) / (4+9) / 2 - 1;
                    continue;
                }
                NumeReKernel::printPreFmt(strfill(toString(_mResult[0][i], 4), 11));
                ///cerr << std::setfill(' ') << std::setw(11) << toString(_mResult[0][i],4);
                if (i+1 < _mResult[0].size())
                    NumeReKernel::printPreFmt(", ");
                    ///cerr << ", ";
            }
            NumeReKernel::printPreFmt(" )\n");
            ///cerr << " )" << endl;
        }
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
            for (unsigned int j = 0; j < _mResult[0].size(); j++)
            {
                if (_mResult[0].size() > (_option.getWindow()-2-15) / (4+9)
                    && ((_option.getWindow()-2-15) / (4+9)) / 2 == j)
                {
                    NumeReKernel::printPreFmt(strfill("..., ", 11));
                    ///cerr << std::setfill(' ') << std::setw(11) << "..., ";
                    j = _mResult[0].size() - (_option.getWindow()-2-15) / (4+9) / 2-1;
                    continue;
                }
                NumeReKernel::printPreFmt(strfill(toString(_mResult[i][j], 4), 11));
                ///cerr << std::setfill(' ') << std::setw(11) << toString(_mResult[i][j],4);
                if (j+1 < _mResult[0].size())
                    NumeReKernel::printPreFmt(", ");
                    ///cerr << ", ";
            }
            if (!i)
                NumeReKernel::printPreFmt(" \\\n");
            else if (i+1 == _mResult.size())
            {
                NumeReKernel::printPreFmt(" /\n");
            }
            else
                NumeReKernel::printPreFmt(" |\n");
        }
    }
    NumeReKernel::flush();
    NumeReKernel::toggleTableStatus();
    return;
}


/////////////////////////////////////////////////
/// \brief This static function solves the system
/// of linear equations symbolically.
///
/// \param _mMatrix const Matrix&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _option const Settings&
/// \param sCmd const string&
/// \param sExpr const string&
/// \param position size_t
/// \return void
///
/////////////////////////////////////////////////
static void parser_solveLGSSymbolic(const Matrix& _mMatrix, Parser& _parser, FunctionDefinitionManager& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, sCmd, position);

    string sSolution = "sle(";
    vector<string> vResult(_mMatrix[0].size()-1, "");
    bool bIsZeroesLine = true;
    unsigned int nVarCount = 0;
    Matrix _mToSolve = createZeroesMatrix(_mMatrix[0].size()-1, _mMatrix[0].size());
    Matrix _mCoefficents = createZeroesMatrix(_mMatrix[0].size()-1, _mMatrix[0].size());
    for (unsigned int i = 0; i < min(_mMatrix.size(), _mMatrix[0].size()-1); i++)
    {
        for (unsigned int j = 0; j < _mMatrix[0].size(); j++)
        {
            _mToSolve[i][j] = _mMatrix[i][j];
        }
    }

    for (int i = _mToSolve.size()-1; i >= 0; i--)
    {
        bIsZeroesLine = true;
        for (unsigned int j = 0; j < _mToSolve[0].size(); j++)
        {
            if (bIsZeroesLine && _mToSolve[i][j])
            {
                bIsZeroesLine = false;
                break;
            }
        }
        if (bIsZeroesLine)
        {
            _mCoefficents[i][i] = 1.0;
            nVarCount++;
        }
        else
        {
            // Konstanter Term
            _mCoefficents[i][_mCoefficents[0].size()-1] = _mToSolve[i][_mToSolve[0].size()-1];
            for (unsigned int j = i+1; j < _mToSolve[0].size()-1; j++)
            {
                if (_mToSolve[i][j])
                {
                    // Konstanter Term
                    _mCoefficents[i][_mCoefficents[0].size()-1] -= _mToSolve[i][j] * _mCoefficents[j][_mCoefficents[0].size()-1];
                    // Alle Koeffizienten
                    for (unsigned int n = i+1; n < _mCoefficents[0].size()-1; n++)
                    {
                        _mCoefficents[i][n] -= _mToSolve[i][j] * _mCoefficents[j][n];
                    }
                }
            }
        }
    }

    for (unsigned int i = 0; i < _mCoefficents.size(); i++)
    {
        for (unsigned int j = 0; j < _mCoefficents[0].size()-1; j++)
        {
            if (_mCoefficents[i][j])
            {
                if (fabs(_mCoefficents[i][j]) != 1.0)
                {
                    vResult[i] += toString(_mCoefficents[i][j], 5) + "*";
                }
                if (_mCoefficents[i][j] == -1.0)
                {
                    vResult[i] += "-";
                }
                vResult[i] += ('z'-vResult.size()+1+j);
            }
        }
        if (_mCoefficents[i][_mCoefficents[0].size()-1])
            vResult[i] += "+" + toString(_mCoefficents[i][_mCoefficents[0].size()-1], 5);
        while (vResult[i].find("+-") != string::npos)
            vResult[i].erase(vResult[i].find("+-"),1);
    }

    for (unsigned int i = 0; i < nVarCount; i++)
    {
        sSolution += ('z'-nVarCount+i+1);
    }
    sSolution += ") := {";
    for (unsigned int i = 0; i < vResult.size(); i++)
    {
        sSolution += vResult[i];
        if (i < vResult.size()-1)
            sSolution += ",";
    }
    sSolution += "}";

    NumeReKernel::print(sSolution);
    sSolution += " "+_lang.get("MATOP_SOLVELGSSYMBOLIC_DEFINECOMMENT");

    bool bDefinitionSuccess = false;

    if (!_functions.isDefined(sSolution))
        bDefinitionSuccess = _functions.defineFunc(sSolution);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sSolution)) != sSolution)
        bDefinitionSuccess = _functions.defineFunc(sSolution, true);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sSolution)) == sSolution)
        return;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

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

                if (isnan(_parser.Eval()) || isinf(_parser.Eval()) || _parser.Eval() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
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

                if (isnan(_parser.Eval()) || isinf(_parser.Eval()) || _parser.Eval() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
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
/// \param _mMatrix const vector<vector<double>>&
/// \return void
///
/////////////////////////////////////////////////
void showMatrix(const vector<vector<double> >& _mMatrix)
{
    parser_ShowMatrixResult(_mMatrix, NumeReKernel::getInstance()->getSettings());
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
        vector<double> v;
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
/// \param vMatrixNames const vector<string>&
/// \param vIndices const vector<Indices>&
/// \param Parser&_parser
/// \param _data Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void parser_declareDataMatrixValuesForIndices(string& _sCmd, const vector<string>& vMatrixNames, const vector<Indices>& vIndices, Parser&_parser, MemoryManager& _data)
{
    for (unsigned int j = 0; j < vIndices.size(); j++)
    {
        vector<double> v;

        // Get the values using the indices
        if (vIndices[j].row.size() > vIndices[j].col.size())
            v = _data.getElement(vIndices[j].row, VectorIndex(vIndices[j].col[0]), vMatrixNames[j]);
        else
            v = _data.getElement(VectorIndex(vIndices[j].row[0]), vIndices[j].col, vMatrixNames[j]);

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
/// \param vMatrixNames const vector<string>&
/// \param vIndices const vector<Indices>&
/// \param vReturnedMatrices const vector<Matrix>&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return Indices
///
/////////////////////////////////////////////////
static Indices parser_getIndicesForMatrix(const string& sCmd, const vector<string>& vMatrixNames, const vector<Indices>& vIndices, const vector<Matrix>& vReturnedMatrices, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
    string _sCmd = sCmd;

    // Declare the return values of the former matrix calculations
    // to the parser by extracting the values and creating a vector
    // variable
    parser_declareMatrixReturnValuesForIndices(_sCmd, vReturnedMatrices, _parser);

    // Declare the already parsed data object matrices in the
    // current expressions by parsing their indices, extracting
    // the corresponding values and creating a vector variable
    parser_declareDataMatrixValuesForIndices(_sCmd, vMatrixNames, vIndices, _parser, _data);

    // Return the calculated indices
    return getIndices(_sCmd, _parser, _data, _option);
}


/////////////////////////////////////////////////
/// \brief This static function creates the
/// matrix function map.
///
/// \return map<string,string>
///
/////////////////////////////////////////////////
static map<string,string> createMatrixFunctionsMap()
{
    map<string,string> mFunctionMap;

    mFunctionMap["det"] = "det(";
    mFunctionMap["invert"] = "invert(";
    mFunctionMap["transpose"] = "transpose(";
    mFunctionMap["zero"] = "zero(";
    mFunctionMap["one"] = "one(";
    mFunctionMap["matfl"] = "matfl(";
    mFunctionMap["matflf"] = "matflf(";
    mFunctionMap["matfc"] = "matfc(";
    mFunctionMap["matfcf"] = "matfcf(";
    mFunctionMap["diag"] = "diag(";
    mFunctionMap["solve"] = "solve(";
    mFunctionMap["cross"] = "cross(";
    mFunctionMap["diagonalize"] = "diagonalize(";
    mFunctionMap["eigenvals"] = "eigenvals(";
    mFunctionMap["eigenvects"] = "eigenvects(";
    mFunctionMap["trace"] = "trace(";
    mFunctionMap["size"] = "size(";
    mFunctionMap["reshape"] = "reshape(";
    mFunctionMap["resize"] = "resize(";
    mFunctionMap["logtoidx"] = "logtoidx(";
    mFunctionMap["idxtolog"] = "idxtolog(";
    mFunctionMap["unique"] = "unique(";
    mFunctionMap["identity"] = "identity(";
    mFunctionMap["covar"] = "covar(";
    mFunctionMap["correl"] = "correl(";
    mFunctionMap["normalize"] = "normalize(";
    mFunctionMap["shuffle"] = "shuffle(";

    return mFunctionMap;
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

    static map<string, string> mMatrixFunctionsMap = createMatrixFunctionsMap();

    // search for any matrix function
    for (auto iter = mMatrixFunctionsMap.begin(); iter != mMatrixFunctionsMap.end(); ++iter)
    {
        size_t match = sExpr.find(iter->first+"(");

        if (match != string::npos && (!match || (!isalpha(sExpr[match-1]) && sExpr[match-1] != '_')))
            return true;
    }

    return false;
}

