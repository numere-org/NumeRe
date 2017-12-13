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

#include "parser_functions.hpp"
#include "../kernel.hpp"

size_t parser_getPreviousMatrixMultiplicationOperator(const string& sCmd, size_t nLastPos);
Matrix parser_matrixMultiplication(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_subMatrixOperations(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_transposeMatrix(const Matrix& _mMatrix);
Matrix parser_InvertMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_IdentityMatrix(unsigned int nSize);
Matrix parser_OnesMatrix(unsigned int nLines, unsigned int nCols);
Matrix parser_ZeroesMatrix(unsigned int nLines, unsigned int nCols);
Matrix parser_getDeterminant(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_matFromCols(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_matFromColsFilled(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_matFromLines(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_matFromLinesFilled(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
vector<double> parser_calcDeltas(const Matrix& _mMatrix, unsigned int nLine);
Matrix parser_diagonalMatrix(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_solveLGS(const Matrix& _mMatrix, Parser& _parser, Define& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_calcCrossProduct(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_calcEigenVects(const Matrix& _mMatrix, int nReturnType, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_SplitMatrix(Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_calcTrace(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_IsSymmMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
void parser_makeReal(Matrix& _mMatrix);
double parser_calcDeterminant(const Matrix& _mMatrix, vector<int> vRemovedLines);
void parser_ShowMatrixResult(const Matrix& _mResult, const Settings& _option);
void parser_solveLGSSymbolic(const Matrix& _mMatrix, Parser& _parser, Define& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position);

Matrix parser_MatrixSize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixAnd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixOr(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixSum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixStd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixAvg(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixPrd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixCnt(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixNum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixNorm(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixMin(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixMax(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixMed(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixPct(const Matrix& _mMatrix, double dPercentage, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixCmp(const Matrix& _mMatrix, double dValue, int nType, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixReshape(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position);
Matrix parser_MatrixResize(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position);
//extern bool bSupressAnswer;

Matrix parser_matrixMultiplication(const Matrix& _mLeft, const Matrix& _mRight, const string& sCmd, const string& sExpr, size_t position)
{
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

Matrix parser_transposeMatrix(const Matrix& _mMatrix)
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

Matrix parser_IdentityMatrix(unsigned int nSize)
{
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

Matrix parser_OnesMatrix(unsigned int nLines, unsigned int nCols)
{
    Matrix _mOnes;
    vector<double> vLine(nCols, 1.0);
    for (unsigned int i = 0; i < nLines; i++)
    {
        _mOnes.push_back(vLine);
    }
    return _mOnes;
}

Matrix parser_ZeroesMatrix(unsigned int nLines, unsigned int nCols)
{
    Matrix _mZeroes;
    vector<double> vLine(nCols, 0.0);
    for (unsigned int i = 0; i < nLines; i++)
        _mZeroes.push_back(vLine);
    return _mZeroes;
}

Matrix parser_InvertMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    //cerr << _mMatrix.size() << "  " << _mMatrix[0].size() << endl;
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

bool parser_matrixOperations(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    vector<Indices> vIndices;
    vector<double> vMatrixVector;
    vector<vector<double> > vTarget;
    vector<string> vMatrixNames;
    vector<int> vMissingValues;
    string sTargetName = "";
    Indices _idx;

    bool bAllowMatrixClearing = false;

    // 1. Objekte ersetzen cmd
    // 2. Vektoren deklarieren
    // 3. Evalschleife durchführen

    // Kommando entfernen
    if (findCommand(sCmd).sString == "matop")
        sCmd.erase(0, findCommand(sCmd).nPos+5);
    if (findCommand(sCmd).sString == "mtrxop")
        sCmd.erase(0, findCommand(sCmd).nPos+6);
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

    if (sCmd.find("data(") == string::npos
        && !_data.containsCacheElements(sCmd)
        && sCmd.find("{") == string::npos
        && sCmd.find("det(") == string::npos
        && sCmd.find("invert(") == string::npos
        && sCmd.find("transpose(") == string::npos
        && sCmd.find("zero(") == string::npos
        && sCmd.find("one(") == string::npos
        && sCmd.find("matfl(") == string::npos
        && sCmd.find("matflf(") == string::npos
        && sCmd.find("matfc(") == string::npos
        && sCmd.find("matfcf(") == string::npos
        && sCmd.find("diag(") == string::npos
        && sCmd.find("solve(") == string::npos
        && sCmd.find("cross(") == string::npos
        && sCmd.find("eigenvals(") == string::npos
        && sCmd.find("eigenvects(") == string::npos
        && sCmd.find("diagonalize(") == string::npos
        && sCmd.find("trace(") == string::npos
        && sCmd.find("size(") == string::npos
        && sCmd.find("and(") == string::npos
        && sCmd.find("or(") == string::npos
        && sCmd.find("reshape(") == string::npos
        && sCmd.find("resize(") == string::npos
        && sCmd.find("identity(") == string::npos)
        throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, SyntaxError::invalid_position);

    // Rekursive Ausdruecke ersetzen
    if (sCmd.find("+=") != string::npos
        || sCmd.find("-=") != string::npos
        || sCmd.find("*=") != string::npos
        || sCmd.find("**=") != string::npos
        || sCmd.find("/=") != string::npos
        || sCmd.find("^=") != string::npos
        || sCmd.find("++") != string::npos
        || sCmd.find("--") != string::npos)
    {
        unsigned int nArgSepPos = 0;
        for (unsigned int i = 0; i < sCmd.length(); i++)
        {
            if (isInQuotes(sCmd, i, false))
                continue;
            if (sCmd[i] == '(')
                i += getMatchingParenthesis(sCmd.substr(i));
            if (sCmd[i] == ',')
                nArgSepPos = i;
            if (sCmd.substr(i,2) == "+="
                || sCmd.substr(i,2) == "-="
                || sCmd.substr(i,2) == "*="
                || sCmd.substr(i,2) == "/="
                || sCmd.substr(i,2) == "^=")
            {
                if (sCmd.find(',', i) != string::npos)
                {
                    for (unsigned int j = i; j < sCmd.length(); j++)
                    {
                        if (sCmd[j] == '(')
                            j += getMatchingParenthesis(sCmd.substr(j));
                        if (sCmd[j] == ',' || j+1 == sCmd.length())
                        {
                            if (!nArgSepPos && j+1 != sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(0, i)
                                    + sCmd[i]
                                    + "("
                                    + sCmd.substr(i+2, j-i-2)
                                    + ") "
                                    + sCmd.substr(j);
                            else if (nArgSepPos && j+1 != sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sCmd[i]
                                    + "("
                                    + sCmd.substr(i+2, j-i-2)
                                    + ") "
                                    + sCmd.substr(j);
                            else if (!nArgSepPos && j+1 == sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(0, i)
                                    + sCmd[i]
                                    + "("
                                    + sCmd.substr(i+2)
                                    + ") ";
                            else
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sCmd[i]
                                    + "("
                                    + sCmd.substr(i+2)
                                    + ") ";

                            for (unsigned int k = i; k < sCmd.length(); k++)
                            {
                                if (sCmd[k] == '(')
                                    k += getMatchingParenthesis(sCmd.substr(k));
                                if (sCmd[k] == ',')
                                {
                                    nArgSepPos = k;
                                    i = k;
                                    break;
                                }
                            }
                            //cerr << sCmd << " | nArgSepPos=" << nArgSepPos << endl;
                            break;
                        }
                    }
                }
                else
                {
                    if (!nArgSepPos)
                        sCmd = sCmd.substr(0, i)
                            + " = "
                            + sCmd.substr(0, i)
                            + sCmd[i]
                            + "("
                            + sCmd.substr(i+2)
                            + ")";
                    else
                        sCmd = sCmd.substr(0, i)
                            + " = "
                            + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                            + sCmd[i]
                            + "("
                            + sCmd.substr(i+2)
                            + ")";
                    break;
                }
            }
            if (sCmd.substr(i,3) == "**=")
            {
                if (sCmd.find(',', i) != string::npos)
                {
                    for (unsigned int j = i; j < sCmd.length(); j++)
                    {
                        if (sCmd[j] == '(')
                            j += getMatchingParenthesis(sCmd.substr(j));
                        if (sCmd[j] == ',' || j+1 == sCmd.length())
                        {
                            if (!nArgSepPos && j+1 != sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(0, i)
                                    + sCmd.substr(i,2)
                                    + "("
                                    + sCmd.substr(i+3, j-i-3)
                                    + ") "
                                    + sCmd.substr(j);
                            else if (nArgSepPos && j+1 != sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sCmd.substr(i,2)
                                    + "("
                                    + sCmd.substr(i+2, j-i-2)
                                    + ") "
                                    + sCmd.substr(j);
                            else if (!nArgSepPos && j+1 == sCmd.length())
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(0, i)
                                    + sCmd.substr(i,2)
                                    + "("
                                    + sCmd.substr(i+2)
                                    + ") ";
                            else
                                sCmd = sCmd.substr(0, i)
                                    + " = "
                                    + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sCmd.substr(i,2)
                                    + "("
                                    + sCmd.substr(i+2)
                                    + ") ";

                            for (unsigned int k = i; k < sCmd.length(); k++)
                            {
                                if (sCmd[k] == '(')
                                    k += getMatchingParenthesis(sCmd.substr(k));
                                if (sCmd[k] == ',')
                                {
                                    nArgSepPos = k;
                                    i = k;
                                    break;
                                }
                            }
                            //cerr << sCmd << " | nArgSepPos=" << nArgSepPos << endl;
                            break;
                        }
                    }
                }
                else
                {
                    if (!nArgSepPos)
                        sCmd = sCmd.substr(0, i)
                            + " = "
                            + sCmd.substr(0, i)
                            + sCmd[i]
                            + "("
                            + sCmd.substr(i+2)
                            + ")";
                    else
                        sCmd = sCmd.substr(0, i)
                            + " = "
                            + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                            + sCmd[i]
                            + "("
                            + sCmd.substr(i+2)
                            + ")";
                    break;
                }
            }
            if (sCmd.substr(i,2) == "++" || sCmd.substr(i,2) == "--")
            {
                if (!nArgSepPos)
                {
                    sCmd = sCmd.substr(0, i)
                        + " = "
                        + sCmd.substr(0, i)
                        + sCmd[i]
                        + "1"
                        + sCmd.substr(i+2);
                }
                else
                    sCmd = sCmd.substr(0, i)
                        + " = "
                        + sCmd.substr(nArgSepPos+1, i-nArgSepPos-1)
                        + sCmd[i]
                        + "1"
                        + sCmd.substr(i+2);
            }
        }
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sCmd = " << sCmd << endl;
    }
    // Target identifizieren
    if (sCmd.find('=') != string::npos
        && sCmd.find('=')
        && sCmd[sCmd.find('=')+1] != '='
        && sCmd[sCmd.find('=')-1] != '!'
        && sCmd[sCmd.find('=')-1] != '<'
        && sCmd[sCmd.find('=')-1] != '>'
        && sCmd.substr(0,sCmd.find('=')).find('(') != string::npos)
    {
        sTargetName = sCmd.substr(0,sCmd.find('='));
        StripSpaces(sTargetName);
        if (sTargetName.substr(0,5) == "data(")
            throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, sTargetName, sTargetName);
        if (sTargetName.find('(') == string::npos)
            throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, sTargetName, sTargetName);
        if (!_data.isCacheElement(sTargetName))
        {
            _data.addCache(sTargetName.substr(0,sTargetName.find('(')), _option);
        }
        _idx = parser_getIndices(sTargetName, _parser, _data, _option);
        if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sTargetName, sTargetName);
        sCmd.erase(0,sCmd.find('=')+1);
        if (!_idx.vI.size())
        {
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nI[1] != -2)
                _idx.nI[1]++;
            if (_idx.nJ[1] != -2)
                _idx.nJ[1]++;
        }

        sTargetName.erase(sTargetName.find('('));
    }
    else
    {
        sTargetName = "matrix";
        _idx.nI[0] = 0;
        _idx.nJ[0] = 0;
        _idx.nJ[1] = -2;
        _idx.nI[1] = -2;
        if (!_data.isCacheElement("matrix("))
        {
            _data.addCache("matrix", _option);
        }
        else
            bAllowMatrixClearing = true;
    }

    // Matrixmultiplikationen / Transpositionen / Invertierungen?
    // -> Submatrixoperationen ausfuehren
    Matrix _mResult = parser_subMatrixOperations(sCmd, _parser, _data, _functions, _option);

    // Target in Zielmatrix speichern
    if (bAllowMatrixClearing)
        _data.deleteBulk("matrix", 0, _data.getLines("matrix", true), 0, _data.getCols("matrix"));
    _data.setCacheSize(_idx.nI[0]+_mResult.size(), _idx.nJ[0]+_mResult[0].size(), -1);
    for (unsigned int i = 0; i < _mResult.size(); i++)
    {
        if (_idx.nI[1] == -2 || _idx.nI[1] - _idx.nI[1] > i)
        {
            for (unsigned int j = 0; j < _mResult[0].size(); j++)
            {
                if (_idx.nJ[1] == -2 || _idx.nJ[1] - _idx.nJ[0] > j)
                    _data.writeToCache((long long int)i+_idx.nI[0], (long long int)j+_idx.nJ[0], sTargetName, _mResult[i][j]);
                else
                    break;
            }
        }
        else
            break;
    }
    parser_ShowMatrixResult(_mResult, _option);

    return true;
}

Matrix parser_subMatrixOperations(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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

    //cerr << sCmd << endl;
    for (unsigned int i = iter_start; i < sCmd.length(); i++)
    {
        //cerr << i << "  " << sCmd[i] << endl;
        if (sCmd.substr(i,10) == "transpose("
            && getMatchingParenthesis(sCmd.substr(i+9)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,11))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+9))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_transposeMatrix(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option)));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+9))+10;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+9))+10, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,7) == "invert("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+6))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_InvertMatrix(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+6));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+6))+7, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,10) == "eigenvals("
            && getMatchingParenthesis(sCmd.substr(i+9)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,11))))
        {
            string sSubExpr = sCmd.substr(i+9, getMatchingParenthesis(sCmd.substr(i+9))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), 0, sCmd, sSubExpr, i+9));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+9))+10;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+9))+10, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,11) == "eigenvects("
            && getMatchingParenthesis(sCmd.substr(i+10)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,12))))
        {
            string sSubExpr = sCmd.substr(i+10, getMatchingParenthesis(sCmd.substr(i+10))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), 1, sCmd, sSubExpr, i+10));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+10))+11;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+10))+11, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,12) == "diagonalize("
            && getMatchingParenthesis(sCmd.substr(i+11)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,13))))
        {
            string sSubExpr = sCmd.substr(i+11, getMatchingParenthesis(sCmd.substr(i+11))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcEigenVects(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), 2, sCmd, sSubExpr, i+11));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+11))+12;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+11))+12, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,6) == "solve("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_solveLGS(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), _parser, _functions, _option, sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+5))+6, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,6) == "cross("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcCrossProduct(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+5))+6, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,6) == "trace("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+5))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_calcTrace(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+5));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+5))+6, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "det("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+3, getMatchingParenthesis(sCmd.substr(i+3))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_getDeterminant(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+3))+4, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+3))+4, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,5) == "zero("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            _parser.SetExpr(sSubExpr);
            v = _parser.Eval(nResults);
            if (nResults > 1)
                vReturnedMatrices.push_back(parser_ZeroesMatrix((unsigned int)v[0], (unsigned int)v[1]));
            else
                vReturnedMatrices.push_back(parser_ZeroesMatrix((unsigned int)v[0], (unsigned int)v[0]));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+8))+9, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,6) == "matfc("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+5))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+5))+6, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,6) == "matfl("
            && getMatchingParenthesis(sCmd.substr(i+5)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,7))))
        {
            string sSubExpr = sCmd.substr(i+6, getMatchingParenthesis(sCmd.substr(i+5))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromLines(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+5))+6;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+5))+6, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,7) == "matfcf("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromColsFilled(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+6))+7, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,7) == "matflf("
            && getMatchingParenthesis(sCmd.substr(i+6)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,8))))
        {
            string sSubExpr = sCmd.substr(i+7, getMatchingParenthesis(sCmd.substr(i+6))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_matFromLinesFilled(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+6))+7;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+6))+7, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,5) == "diag("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_diagonalMatrix(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,5) == "size("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixSize(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+4));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "and("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixAnd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,3) == "or("
            && getMatchingParenthesis(sCmd.substr(i+2)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,4))))
        {
            string sSubExpr = sCmd.substr(i+3, getMatchingParenthesis(sCmd.substr(i+2))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixOr(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+2));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+2))+3;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "sum("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixSum(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "avg("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixAvg(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "std("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixStd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "num("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixNum(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "cnt("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixCnt(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "min("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMin(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "max("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMax(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "prd("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixPrd(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,5) == "norm("
            && getMatchingParenthesis(sCmd.substr(i+4)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,6))))
        {
            string sSubExpr = sCmd.substr(i+5, getMatchingParenthesis(sCmd.substr(i+4))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixNorm(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+4))+5;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,4) == "med("
            && getMatchingParenthesis(sCmd.substr(i+3)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,5))))
        {
            string sSubExpr = sCmd.substr(i+4, getMatchingParenthesis(sCmd.substr(i+3))-1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            vReturnedMatrices.push_back(parser_MatrixMed(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option), sCmd, sSubExpr, i+3));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i+3))+4;
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
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
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i+4))+5, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd.substr(i,2) == "{{"
            && getMatchingParenthesis(sCmd.substr(i)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,3))))
        {
            string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            //cerr << sSubExpr << endl;
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i))+1, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
        }
        if (sCmd[i] == '{'
            && getMatchingParenthesis(sCmd.substr(i)) != string::npos
            && (!i || checkDelimiter(sCmd.substr(i-1,2))))
        {
            string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
            __sCmd += sCmd.substr(pos_back, i-pos_back);
            //cerr << sSubExpr << endl;
            vReturnedMatrices.push_back(parser_matFromCols(sSubExpr, _parser, _data, _functions, _option));
            pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
            //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i))+1, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
            __sCmd += "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]";
            i = pos_back-1;
            //cerr << sCmd << endl;
        }
        if (i > 14
            && sCmd[i] == '('
            && i == pos_back // only true, if the last iteration evaluated a subexpression
            && sCmd.find_last_not_of(' ',i-1) != string::npos
            && __sCmd.back() == ']') //...returnedMatrix[N](:,:)
        {
            int nMatrix = 0;
            nMatrix = StrToInt(__sCmd.substr(__sCmd.rfind('[')+1, __sCmd.rfind(']')-__sCmd.rfind('[')-1));
            if (__sCmd.substr(__sCmd.rfind('[')-14,15) == "returnedMatrix[")
            {
                string sSubExpr = sCmd.substr(i, getMatchingParenthesis(sCmd.substr(i))+1);
                //sCmd.erase(i, sSubExpr.length());
                pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
                //i--;
                vReturnedMatrices[nMatrix] = parser_getMatrixElements(sSubExpr, vReturnedMatrices[nMatrix], _parser, _data, _functions, _option);
                i = pos_back-1;
            }
        }
        if (sCmd[i] == '(')
        {
            if (sCmd.substr(i,getMatchingParenthesis(sCmd.substr(i))).find("**") != string::npos
                || (i > 1
                    && !_data.isCacheElement(sCmd.substr(sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)+1, i-sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)-1))
                    && sCmd.substr(sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)+1, i-sCmd.find_last_of(" +-*/!^%&|#(){}?:,<>=", i-1)-1) != "data"))
            {
                string sSubExpr = sCmd.substr(i+1, getMatchingParenthesis(sCmd.substr(i))-1);
                __sCmd += sCmd.substr(pos_back, i-pos_back);
                vReturnedMatrices.push_back(parser_subMatrixOperations(sSubExpr, _parser, _data, _functions, _option));
                pos_back = i+getMatchingParenthesis(sCmd.substr(i))+1;
                //sCmd.replace(i, getMatchingParenthesis(sCmd.substr(i))+1, "returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"]");
                __sCmd += "(returnedMatrix["+toString((int)vReturnedMatrices.size()-1)+"])";
                i = pos_back-1;
            }
        }
    }

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

    // 1. Objekte ersetzen cmd
    // 2. Vektoren deklarieren
    // 3. Evalschleife durchführen

    /*if (sCmd.find("data(") == string::npos
        && !_data.containsCacheElements(sCmd)
        && __sCmd.find("matrix[") == string::npos
        && __sCmd.find("returnedMatrix[") == string::npos)
        throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, SyntaxError::invalid_position);*/

    // Data und Caches ersetzen
    while (__sCmd.find("data(", nPos) != string::npos)
    {
        nPos = __sCmd.find("data(", nPos);
        if (nPos && !checkDelimiter(__sCmd.substr(nPos-1,6)))
        {
            nPos++;
            continue;
        }
        vIndices.push_back(parser_getIndices(__sCmd.substr(nPos), _parser, _data, _option));
        if (!vIndices[vIndices.size()-1].vI.size())
        {
            if (!parser_evalIndices("data", vIndices[vIndices.size()-1], _data))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, nPos);
        }
        vMatrixNames.push_back("data");
        if (parser_AddVectorComponent("", __sCmd.substr(0,nPos), __sCmd.substr(nPos+5+getMatchingParenthesis(__sCmd.substr(nPos+4))),false) == "0")
            vMissingValues.push_back(0);
        else
            vMissingValues.push_back(1);
        __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+4))+5, "matrix["+toString((int)vMatrixNames.size()-1)+"]");
    }
    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
    {
        nPos = 0;
        while (__sCmd.find(iter->first+"(", nPos) != string::npos)
        {
            nPos = __sCmd.find(iter->first+"(", nPos);
            if (nPos && !checkDelimiter(__sCmd.substr(nPos-1,(iter->first).length()+2)))
            {
                nPos++;
                continue;
            }
            vIndices.push_back(parser_getIndices(__sCmd.substr(nPos), _parser, _data, _option));
            if (!vIndices[vIndices.size()-1].vI.size())
            {
                if (!parser_evalIndices(iter->first, vIndices[vIndices.size()-1], _data))
                    throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, nPos);
            }
            vMatrixNames.push_back(iter->first);
            if (parser_AddVectorComponent("", __sCmd.substr(0,nPos), __sCmd.substr(nPos+1+(iter->first).length()+getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))),false) == "0")
                vMissingValues.push_back(0);
            else
                vMissingValues.push_back(1);
            __sCmd.replace(nPos, getMatchingParenthesis(__sCmd.substr(nPos+(iter->first).length()))+(iter->first).length()+1, "matrix["+toString((int)vMatrixNames.size()-1)+"]");
        }
    }

    //cerr << sCmd << endl;
    // Alle Datafiles/Caches ersetzt

    pos_back = sCmd.length();

    // Matrixmultiplikation
    if (__sCmd.find("**") != string::npos)
    {
        // Rechtsgebundene Auswertung
        for (int n = __sCmd.length()-1; n >= 0; n--)
        {
            if (__sCmd.substr(n,2) == "**")
            {
                pos_back = parser_getPreviousMatrixMultiplicationOperator(sCmd, pos_back)-1;
                Matrix _mLeft;
                Matrix _mRight;
                unsigned int nPositions[2];
                nPositions[1] = __sCmd.find(']',n)+1;
                string sElement = __sCmd.substr(__sCmd.find_first_not_of(' ', n+2));
                // Rechter Teil
                //cerr << "rechts" << endl;
                sElement.erase(sElement.find(']')+1);
                if (sElement.find_first_of("()+-*/^!%&|<>=?:,") != string::npos || sElement.find_first_of("[]") == string::npos)
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);
                //cerr << sElement.substr(0,7) << endl;
                if (sElement.substr(0,7) == "matrix[")
                {
                    vector<double> vLine;
                    unsigned int nthMatrix = StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')));
                    //cerr << nthMatrix << endl;
                    if (!vIndices[nthMatrix].vI.size())
                    {
                        for (unsigned int i = vIndices[nthMatrix].nI[0]; i < vIndices[nthMatrix].nI[1]; i++)
                        {
                            if (vIndices[nthMatrix].nJ[0] >= vIndices[nthMatrix].nJ[1])
                                vLine.push_back(vMissingValues[nthMatrix]);
                            else
                            {
                                for (long long int k = vIndices[nthMatrix].nJ[0]; k < vIndices[nthMatrix].nJ[1]; k++)
                                {
                                    if (_data.isValidEntry(vIndices[nthMatrix].nI[0]+i, k, vMatrixNames[nthMatrix]))
                                        vLine.push_back(_data.getElement(vIndices[nthMatrix].nI[0]+i, k, vMatrixNames[nthMatrix]));
                                    else
                                        vLine.push_back(NAN);
                                }
                            }
                            _mRight.push_back(vLine);
                            vLine.clear();
                        }
                    }
                    else
                    {
                        for (unsigned int i = 0; i < vIndices[nthMatrix].vI.size(); i++)
                        {
                            _mRight.push_back(_data.getElement(vector<long long int>(1,vIndices[nthMatrix].vI[i]), vIndices[nthMatrix].vJ, vMatrixNames[nthMatrix]));
                        }
                    }
                }
                else
                {
                    _mRight = vReturnedMatrices[StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')))];
                }

                // Linker Teil
                //cerr << "links" << endl;
                sElement = __sCmd.substr(0,__sCmd.find_last_of(']', n-1)+1);
                sElement.erase(0,sElement.find_last_of('[')-6);
                //cerr << sElement.substr(0,7) << endl;
                if (sElement.find_first_of("()+-*/^!%&|<>=?:,") != string::npos || sElement.find_first_of("[]") == string::npos)
                    throw SyntaxError(SyntaxError::NO_MATRIX_FOR_MATOP, sCmd, pos_back+1);

                if (sElement.substr(0,7) == "matrix[")
                {
                    nPositions[0] = __sCmd.rfind("matrix[",n);
                    vector<double> vLine;
                    unsigned int nthMatrix = StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')));
                    //cerr << nthMatrix << endl;
                    if (!vIndices[nthMatrix].vI.size())
                    {
                        for (unsigned int i = vIndices[nthMatrix].nI[0]; i < vIndices[nthMatrix].nI[1]; i++)
                        {
                            if (vIndices[nthMatrix].nJ[0] >= vIndices[nthMatrix].nJ[1])
                                vLine.push_back(vMissingValues[nthMatrix]);
                            else
                            {
                                for (long long int k = vIndices[nthMatrix].nJ[0]; k < vIndices[nthMatrix].nJ[1]; k++)
                                {
                                    if (_data.isValidEntry(vIndices[nthMatrix].nI[0]+i, k, vMatrixNames[nthMatrix]))
                                        vLine.push_back(_data.getElement(vIndices[nthMatrix].nI[0]+i, k, vMatrixNames[nthMatrix]));
                                    else
                                        vLine.push_back(NAN);
                                }
                            }
                            //cerr << vLine.size() << endl;
                            _mLeft.push_back(vLine);
                            vLine.clear();
                        }
                    }
                    else
                    {
                        for (unsigned int i = 0; i < vIndices[nthMatrix].vI.size(); i++)
                        {
                            _mLeft.push_back(_data.getElement(vector<long long int>(1,vIndices[nthMatrix].vI[i]), vIndices[nthMatrix].vJ, vMatrixNames[nthMatrix]));
                        }
                    }
                }
                else
                {
                    nPositions[0] = __sCmd.rfind("returnedMatrix[", n);
                    _mLeft = vReturnedMatrices[StrToInt(sElement.substr(sElement.find('[')+1, sElement.find(']')-1-sElement.find('[')))];
                }
                // Multiplizieren
                //cerr << "multiply" << endl;
                vReturnedMatrices.push_back(parser_matrixMultiplication(_mLeft, _mRight, sCmd, "", pos_back+1));

                // Ersetzen
                //cerr << "replace" << endl;
                __sCmd.replace(nPositions[0], nPositions[1]-nPositions[0], "returnedMatrix[" + toString((int)vReturnedMatrices.size()-1)+"]");
                n = nPositions[0];
            }
        }
    }


    sCmd = __sCmd;
    //cerr << sCmd << endl;

    // MaxCol identifizieren Index-fehler!
    for (unsigned int i = 0; i < vIndices.size(); i++)
    {
        if (sCmd.find("matrix["+toString((int)i)+"]") == string::npos)
            continue;
        if (vIndices[i].vI.size())
        {
            if (vIndices[i].vJ.size() > nColCount)
                nColCount = vIndices[i].vJ.size();
        }
        else
        {
            if (vIndices[i].nJ[1]-vIndices[i].nJ[0] > nColCount)
                nColCount = vIndices[i].nJ[1]-vIndices[i].nJ[0];
        }
    }
    for (unsigned int i = 0; i < vReturnedMatrices.size(); i++)
    {
        if (vReturnedMatrices[i][0].size() > nColCount && sCmd.find("returnedMatrix["+toString((int)i)+"]") != string::npos)
            nColCount = vReturnedMatrices[i][0].size();
    }

    // Vectoren lesen und zuweisen
    for (unsigned int j = 0; j < vIndices.size(); j++)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();
        if (vIndices[j].vI.size())
        {
            vMatrixVector = _data.getElement(vIndices[j].vI, vector<long long int>(1,vIndices[j].vJ[0]), vMatrixNames[j]);
        }
        else
        {
            if (vIndices[j].nJ[0] >= vIndices[j].nJ[1])
                vMatrixVector.push_back(vMissingValues[j]);
            else
            {
                for (long long int k = vIndices[j].nI[0]; k < vIndices[j].nI[1]; k++)
                {
                    if (_data.isValidEntry(k, vIndices[j].nJ[0], vMatrixNames[j]))
                        vMatrixVector.push_back(_data.getElement(k, vIndices[j].nJ[0], vMatrixNames[j]));
                    else
                        vMatrixVector.push_back(NAN);
                }
            }
        }
        if (sCmd.find("matrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("matrix["+toString((int)j)+"]", vMatrixVector);
    }
    for (unsigned int j = 0; j < vReturnedMatrices.size(); j++)
    {
        if (vMatrixVector.size())
            vMatrixVector.clear();
        if (!vReturnedMatrices[j][0].size())
            vMatrixVector.push_back(0.0);
        else
        {
            for (unsigned int k = 0; k < vReturnedMatrices[j].size(); k++)
            {
                vMatrixVector.push_back(vReturnedMatrices[j][k][0]);
            }
        }
        if (sCmd.find("returnedMatrix["+toString((int)j)+"]") != string::npos)
            _parser.SetVectorVar("returnedMatrix["+toString((int)j)+"]", vMatrixVector);
    }

    // sCmd als Expr zuweisen
    _parser.SetExpr(sCmd);

    // Auswerten
    v = _parser.Eval(nResults);

    // An Ziel zuweisen
    if (vMatrixVector.size())
        vMatrixVector.clear();
    for (int i = 0; i < nResults; i++)
    {
        //double d = v[i];
        vMatrixVector.push_back(v[i]);
    }
    _mTarget.push_back(vMatrixVector);
    if (vMatrixVector.size() > nLinesCount)
        nLinesCount = vMatrixVector.size();
    if (!nColCount && vMatrixVector.size())
        nColCount = 1;

    // Fuer die Zahl der Cols
    for (unsigned int i = 1; i < nColCount; i++)
    {
        // Vectoren lesen und zuweisen
        for (unsigned int j = 0; j < vIndices.size(); j++)
        {
            if (vMatrixVector.size())
                vMatrixVector.clear();
            if (vIndices[j].vI.size())
            {
                if (vIndices[j].vJ.size() <= i && (vIndices[j].vJ.size() > 1 || vIndices[j].vI.size() > 1))
                    vMatrixVector.push_back(vMissingValues[j]);
                else if (vIndices[j].vI.size() == 1 && vIndices[j].vJ.size() == 1)
                    continue;
                else
                {
                    vMatrixVector = _data.getElement(vIndices[j].vI, vector<long long int>(1, vIndices[j].vJ[i]), vMatrixNames[j]);
                }
            }
            else
            {
                if (vIndices[j].nJ[0]+i >= vIndices[j].nJ[1] && (vIndices[j].nJ[1]-vIndices[j].nJ[0] > 1 || vIndices[j].nI[1]-vIndices[j].nI[0] > 1))
                    vMatrixVector.push_back(vMissingValues[j]);
                else if (vIndices[j].nJ[1]-vIndices[j].nJ[0] <= 1 && vIndices[j].nI[1]-vIndices[j].nI[0] <= 1)
                {
                    continue;
                }
                else
                {
                    for (long long int k = vIndices[j].nI[0]; k < vIndices[j].nI[1]; k++)
                    {
                        if (_data.isValidEntry(k, vIndices[j].nJ[0]+i, vMatrixNames[j]))
                            vMatrixVector.push_back(_data.getElement(k, vIndices[j].nJ[0]+i, vMatrixNames[j]));
                        else
                            vMatrixVector.push_back(NAN);
                    }
                }
            }
            if (sCmd.find("matrix["+toString((int)j)+"]") != string::npos)
                _parser.SetVectorVar("matrix["+toString((int)j)+"]", vMatrixVector);
        }
        for (unsigned int j = 0; j < vReturnedMatrices.size(); j++)
        {
            if (vMatrixVector.size())
                vMatrixVector.clear();
            if (!vReturnedMatrices[j][0].size())
                vMatrixVector.push_back(0.0);
            else if (vReturnedMatrices[j].size() == 1 && vReturnedMatrices[j][0].size() == 1)
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
            if (sCmd.find("returnedMatrix["+toString((int)j)+"]") != string::npos)
                _parser.SetVectorVar("returnedMatrix["+toString((int)j)+"]", vMatrixVector);
        }

        // Auswerten
        v = _parser.Eval(nResults);

        // An Ziel zuweisen
        if (vMatrixVector.size())
            vMatrixVector.clear();
        for (int j = 0; j < nResults; j++)
            vMatrixVector.push_back(v[j]);
        _mTarget.push_back(vMatrixVector);
        if (vMatrixVector.size() > nLinesCount)
            nLinesCount = vMatrixVector.size();
    }

    // Transponieren und ggf. ergaenzen
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

    return _mResult;
}

size_t parser_getPreviousMatrixMultiplicationOperator(const string& sCmd, size_t nLastPos)
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

Matrix parser_matFromCols(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    return parser_transposeMatrix(parser_matFromLines(sCmd, _parser, _data, _functions, _option));
}

Matrix parser_matFromColsFilled(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    return parser_transposeMatrix(parser_matFromLinesFilled(sCmd, _parser, _data, _functions, _option));
}

Matrix parser_matFromLines(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);
    if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
    {
        parser_GetDataElement(sCmd, _parser, _data, _option);
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

Matrix parser_matFromLinesFilled(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);
    if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
    {
        parser_GetDataElement(sCmd, _parser, _data, _option);
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

vector<double> parser_calcDeltas(const Matrix& _mMatrix, unsigned int nLine)
{
    vector<double> vDeltas;
    for (unsigned int j = 1; j < _mMatrix[nLine].size(); j++)
    {
        vDeltas.push_back(_mMatrix[nLine][j]-_mMatrix[nLine][j-1]);
    }
    return vDeltas;
}

Matrix parser_diagonalMatrix(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    Matrix _diag;
    value_type* v = 0;
    int nResults = 0;
    vector<double> vLine;
    if (!sCmd.length())
    {
        _diag.push_back(vector<double>(1,NAN));
    }
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);
    if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
    {
        parser_GetDataElement(sCmd, _parser, _data, _option);
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

    _diag = parser_ZeroesMatrix(vLine.size(),vLine.size());

    for (unsigned int i = 0; i < _diag.size(); i++)
    {
        _diag[i][i] = vLine[i];
    }

    return _diag;
}

Matrix parser_getDeterminant(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_IdentityMatrix(1);
    vector<int> vRemovedLines(_mMatrix.size(), 0);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

    _mReturn[0][0] = parser_calcDeterminant(_mMatrix, vRemovedLines);
    return _mReturn;
}

double parser_calcDeterminant(const Matrix& _mMatrix, vector<int> vRemovedLines)
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


Matrix parser_MatrixSize(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(2,1);
    _mReturn[0][0] = _mMatrix.size();
    _mReturn[1][0] = _mMatrix[0].size();
    return _mReturn;
}

Matrix parser_MatrixAnd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
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

Matrix parser_MatrixOr(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixSum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixStd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixAvg(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    Matrix _mSum = parser_MatrixSum(_mMatrix, sCmd, sExpr, position);
    Matrix _mNum = parser_MatrixNum(_mMatrix, sCmd, sExpr, position);
    _mReturn[0][0] = _mSum[0][0] / _mNum[0][0];
    return _mReturn;
}

Matrix parser_MatrixPrd(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
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

Matrix parser_MatrixCnt(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    _mReturn[0][0] = _mMatrix.size() * _mMatrix[0].size();
    return _mReturn;
}

Matrix parser_MatrixNum(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixNorm(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixMin(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixMax(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
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

Matrix parser_MatrixMed(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    Datafile _cache;

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            _cache.writeToCache(j + i*_mMatrix.size(), 0, "cache", _mMatrix[i][j]);
        }
    }

    _mReturn[0][0] = _cache.med("cache", 0, _mMatrix.size()*_mMatrix[0].size());
    return _mReturn;
}

Matrix parser_MatrixPct(const Matrix& _mMatrix, double dPercentage, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    Datafile _cache;

    for (size_t i = 0; i < _mMatrix.size(); i++)
    {
        for (size_t j = 0; j < _mMatrix[0].size(); j++)
        {
            _cache.writeToCache(j + i*_mMatrix.size(), 0, "cache", _mMatrix[i][j]);
        }
    }

    _mReturn[0][0] = _cache.pct("cache", 0, (long long int)(_mMatrix.size()*_mMatrix[0].size()), 0, -1, dPercentage);
    return _mReturn;
}

Matrix parser_MatrixCmp(const Matrix& _mMatrix, double dValue, int nType, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    Matrix _mCoords = parser_ZeroesMatrix(2,1);
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

Matrix parser_MatrixReshape(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position)
{
    if (nLines * nCols != _mMatrix.size() * _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(nLines) + "x" + toString(nCols) + "=" + toString(nLines*nCols) +" vs. "+ toString(_mMatrix.size()) + "x" +  toString(_mMatrix[0].size()) + "=" + toString(_mMatrix.size()*_mMatrix[0].size()));
    Matrix _mReturn = parser_ZeroesMatrix(nLines, nCols);
    for (size_t i = 0; i < nLines*nCols; i++)
    {
        _mReturn[i / nCols][i % nCols] = _mMatrix[i / _mMatrix[0].size()][i % _mMatrix[0].size()];
    }
    return _mReturn;
}

Matrix parser_MatrixResize(const Matrix& _mMatrix, size_t nLines, size_t nCols, const string& sCmd, const string& sExpr, size_t position)
{
    if (!nLines || !nCols)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(nLines) + "x" + toString(nCols));
    if (nLines == _mMatrix.size() && nCols == _mMatrix[0].size())
        return _mMatrix;
    Matrix _mReturn = parser_ZeroesMatrix(nLines, nCols);
    for (size_t i = 0; i < min(nLines, _mMatrix.size()); i++)
    {
        for (size_t j = 0; j < min(nCols, _mMatrix[0].size()); j++)
        {
            _mReturn[i][j] = _mMatrix[i][j];
        }
    }
    return _mReturn;
}


// LGS-Loesung auf Basis des Invert-Algorthmuses
Matrix parser_solveLGS(const Matrix& _mMatrix, Parser& _parser, Define& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mResult = parser_ZeroesMatrix(_mMatrix[0].size()-1,1);
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

// n-dimensionales Kreuzprodukt
Matrix parser_calcCrossProduct(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    Matrix _mResult = parser_ZeroesMatrix(_mMatrix.size(),1);
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
    Matrix _mTemp = parser_ZeroesMatrix(_mMatrix.size(), _mMatrix[0].size()+1);
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

Matrix parser_calcEigenVects(const Matrix& _mMatrix, int nReturnType, const string& sCmd, const string& sExpr, size_t position)
{
    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    Matrix _mEigenVals;
    Matrix _mEigenVects; // Temporaere Zuweisung, um die Matrix ggf. Symmetrisch zu machen
    //Matrix _mTriangular;
    Eigen::MatrixXd mMatrix(_mMatrix.size(), _mMatrix.size());
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = 0; j < _mMatrix.size(); j++)
        {
            mMatrix(i,j) = _mMatrix[i][j];
        }
    }
    if (parser_IsSymmMatrix(_mMatrix, sCmd, sExpr, position))
    {
        _mEigenVals = parser_ZeroesMatrix(_mMatrix.size(),1);
        _mEigenVects = parser_ZeroesMatrix(_mMatrix.size(), _mMatrix.size());
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eSolver(mMatrix);

        if (!nReturnType)
        {
            Eigen::VectorXd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVals.size(); i++)
            {
                _mEigenVals[i][0] = vEigenVals(i,0);
            }
        }
        else if (nReturnType == 1)
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
        else
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
        _mEigenVals = parser_ZeroesMatrix(_mMatrix.size(),2);
        _mEigenVects = parser_ZeroesMatrix(_mMatrix.size(), 2*_mMatrix.size());
        Eigen::EigenSolver<Eigen::MatrixXd> eSolver(mMatrix);

        if (!nReturnType)
        {
            Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVals.size(); i++)
            {
                _mEigenVals[i][0] = real(vEigenVals(i,0));
                _mEigenVals[i][1] = imag(vEigenVals(i,0));
            }
            parser_makeReal(_mEigenVals);
        }
        else if (nReturnType == 1)
        {
            Eigen::MatrixXcd mEigenVects = eSolver.eigenvectors();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                for (unsigned int j = 0; j < _mEigenVects.size(); j++)
                {
                    _mEigenVects[i][2*j] = real(mEigenVects(i,j));
                    _mEigenVects[i][2*j+1] = imag(mEigenVects(i,j));
                }
            }
            parser_makeReal(_mEigenVects);
        }
        else
        {
            Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                _mEigenVects[i][2*i] = real(vEigenVals(i,0));
                _mEigenVects[i][2*i+1] = imag(vEigenVals(i,0));
            }
            parser_makeReal(_mEigenVects);
        }
    }

    if (!nReturnType)
        return _mEigenVals;
    else
        return _mEigenVects;
}

void parser_makeReal(Matrix& _mMatrix)
{
    if (_mMatrix[0].size() < 2 || (_mMatrix[0].size() % 2))
        return;

    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = 1; j < _mMatrix[0].size(); j+=2)
        {
            if (_mMatrix[i][j])
                return;
        }
    }
    if (_mMatrix[0].size() == 2)
    {
        for (unsigned int i = 0; i < _mMatrix.size(); i++)
            _mMatrix[i].pop_back();
    }
    else
    {
        for (unsigned int i = 0; i < _mMatrix.size(); i++)
        {
            for (int j = _mMatrix[i].size()-1; j > 0; j -= 2)
            {
                _mMatrix[i].erase(_mMatrix[i].begin()+j);
            }
        }
    }
    return;
}

bool parser_IsSymmMatrix(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = i; j < _mMatrix.size(); j++)
        {
            if (_mMatrix[i][j] != _mMatrix[j][i])
                return false;
        }
    }
    return true;
}

Matrix parser_SplitMatrix(Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    Matrix _mTriangular = _mMatrix;
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        for (unsigned int j = i; j < _mMatrix.size(); j++)
        {
            if (i == j)
                _mMatrix[i][j] = 0.0;
            else
            {// A = (S + T) ==> T = A - S
                _mTriangular[i][j] = 0.0;
                _mTriangular[j][i] -= _mMatrix[j][i];
                _mMatrix[j][i] = _mMatrix[i][j];
            }
        }
    }
    return _mTriangular;
}

Matrix parser_calcTrace(const Matrix& _mMatrix, const string& sCmd, const string& sExpr, size_t position)
{
    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, sCmd, position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));
    Matrix _mReturn = parser_ZeroesMatrix(1,1);
    for (unsigned int i = 0; i < _mMatrix.size(); i++)
    {
        _mReturn[0][0] += _mMatrix[i][i];
    }
    return _mReturn;
}

Matrix parser_getMatrixElements(string& sExpr, const Matrix& _mMatrix, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    Matrix _mReturn;
    Indices _idx = parser_getIndices(sExpr, _mMatrix, _parser, _data, _option);


    if (_idx.vI.size() && _idx.vJ.size())
    {
        _mReturn = parser_ZeroesMatrix(_idx.vI.size(), _idx.vJ.size());
        for (unsigned int i = 0; i < _idx.vI.size(); i++)
        {
            for (unsigned int j = 0; j < _idx.vJ.size(); j++)
            {
                if (_idx.vI[i] >= _mMatrix.size() || _idx.vJ[j] >= _mMatrix[0].size())
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
                _mReturn[i][j] = _mMatrix[_idx.vI[i]][_idx.vJ[j]];
            }
        }
    }
    else
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);

        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0]+1;
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _mMatrix.size();
        else
            _idx.nI[1]++;

        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0]+1;
        else if (_idx.nJ[1] == -2)
            _idx.nJ[1] = _mMatrix[0].size();
        else
            _idx.nJ[1]++;

        if (_idx.nI[0] > _mMatrix.size() || _idx.nI[1] > _mMatrix.size() || _idx.nJ[0] > _mMatrix[0].size() || _idx.nJ[1] > _mMatrix[0].size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);

        _mReturn = parser_ZeroesMatrix(_idx.nI[1]-_idx.nI[0], _idx.nJ[1]-_idx.nJ[0]);

        for (unsigned int i = 0; i < _idx.nI[1]-_idx.nI[0]; i++)
        {
            for (unsigned int j = 0; j < _idx.nJ[1]-_idx.nJ[0]; j++)
            {
                _mReturn[i][j] = _mMatrix[i+_idx.nI[0]][j+_idx.nJ[0]];
            }
        }
    }
    return _mReturn;
}

void parser_ShowMatrixResult(const Matrix& _mResult, const Settings& _option)
{
    if (!_option.getSystemPrintStatus() || NumeReKernel::bSupressAnswer)
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

void parser_solveLGSSymbolic(const Matrix& _mMatrix, Parser& _parser, Define& _functions, const Settings& _option, const string& sCmd, const string& sExpr, size_t position)
{
    string sSolution = "sle(";
    vector<string> vResult(_mMatrix[0].size()-1, "");
    bool bIsZeroesLine = true;
    unsigned int nVarCount = 0;
    Matrix _mToSolve = parser_ZeroesMatrix(_mMatrix[0].size()-1, _mMatrix[0].size());
    Matrix _mCoefficents = parser_ZeroesMatrix(_mMatrix[0].size()-1, _mMatrix[0].size());
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

    if (!_functions.isDefined(sSolution))
        _functions.defineFunc(sSolution, _parser, _option);
    else if (_functions.getDefine(_functions.getFunctionIndex(sSolution)) != sSolution)
        _functions.defineFunc(sSolution, _parser, _option, true);

    return;
}


Indices parser_getIndices(const string& sCmd, const Matrix& _mMatrix, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _idx;
    string sI[2] = {"<<NONE>>", "<<NONE>>"};
    string sJ[2] = {"<<NONE>>", "<<NONE>>"};
    string sArgument = "";
    unsigned int nPos = 0;
    int nParenthesis = 0;
    value_type* v = 0;
    int nResults = 0;
    for (int i = 0; i < 2; i++)
    {
        _idx.nI[i] = -1;
        _idx.nJ[i] = -1;
    }
    //cerr << sCmd << endl;
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
    if (sArgument.find("data(") != string::npos || _data.containsCacheElements(sArgument))
        parser_GetDataElement(sArgument, _parser, _data, _option);
    // --> Kurzschreibweise!
    if (!sArgument.length())
    {
        _idx.nI[0] = 0;
        _idx.nJ[0] = 0;
        _idx.nI[1] = -2;
        _idx.nJ[1] = -2;
        return _idx;
    }
    //cerr << sArgument << endl;
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
                for (int n = 0; n < nResults; n++)
                    _idx.vI.push_back((int)v[n]-1);
            }
            else
                _idx.nI[0] = (int)v[0]-1;
        }
        if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
        {
            _parser.SetExpr(sJ[0]);
            v = _parser.Eval(nResults);
            if (nResults > 1)
            {
                for (int n = 0; n < nResults; n++)
                    _idx.vJ.push_back((int)v[n]-1);
            }
            else
                _idx.nJ[0] = (int)v[0]-1;
        }

        for (int n = 0; n < 2; n++)
        {
            //cerr << sI[n] << endl;
            //cerr << sJ[n] << endl;
            if (sI[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nI[n] = -2;
                else
                    _idx.nI[0] = 0;
            }
            else if (sI[n] != "<<NONE>>")
            {
                if (_idx.vI.size())
                    continue;
                _parser.SetExpr(sI[n]);
                _idx.nI[n] = (int)_parser.Eval()-1;
                if (isnan(_parser.Eval()) || isinf(_parser.Eval()) || _parser.Eval() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
            }
            if (sJ[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nJ[n] = -2;
                else
                    _idx.nJ[0] = 0;
            }
            else if (sJ[n] != "<<NONE>>")
            {
                if (_idx.vJ.size())
                    continue;
                _parser.SetExpr(sJ[n]);
                _idx.nJ[n] = (int)_parser.Eval()-1;
                if (isnan(_parser.Eval()) || isinf(_parser.Eval()) || _parser.Eval() <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
            }
        }
        if (_idx.vI.size() || _idx.vJ.size())
        {
            if (!_idx.vI.size())
            {
                if (_idx.nI[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
                if (_idx.nI[1] == -2)
                {
                    for (long long int i = _idx.nI[0]; i < (long long int)_mMatrix.size(); i++)
                        _idx.vI.push_back(i);
                }
                else if (_idx.nI[1] == -1)
                    _idx.vI.push_back(_idx.nI[0]);
                else
                {
                    for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                        _idx.vI.push_back(i);
                }
            }
            if (!_idx.vJ.size())
            {
                if (_idx.nJ[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, "", SyntaxError::invalid_position);
                if (_idx.nJ[1] == -2)
                {
                    for (long long int j = _idx.nJ[0]; j < (long long int)_mMatrix[0].size(); j++)
                        _idx.vJ.push_back(j);
                }
                else if (_idx.nJ[1] == -1)
                    _idx.vJ.push_back(_idx.nJ[0]);
                else
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                        _idx.vJ.push_back(j);
                }
            }
        }
    }
    return _idx;
}


