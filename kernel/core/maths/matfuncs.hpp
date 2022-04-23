/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef MATFUNCS_HPP
#define MATFUNCS_HPP

#define EIGENVALUES 0
#define EIGENVECTORS 1
#define DIAGONALIZE 2

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <random>
#include <map>
#include "matdatastructures.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"
#include "functionimplementation.hpp"
#include "statslogic.hpp"

/////////////////////////////////////////////////
/// \brief Simple helper for printing the matrix
/// dimensions to a string.
///
/// \param mat const Matrix&
/// \return std::string
///
/////////////////////////////////////////////////
static inline std::string printMatrixDim(const Matrix& mat)
{
    return mat.printDims();
}


/////////////////////////////////////////////////
/// \brief This static function returns a matrix
/// filled with the passed value of the defined
/// size.
///
/// \param n size_t
/// \param m size_t
/// \param val double
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix createFilledMatrix(size_t n, size_t m, const mu::value_type& val)
{
    return Matrix(n, m, val);
}


/////////////////////////////////////////////////
/// \brief This static function returns a matrix
/// filled with zeros of the defined size.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix createZeroesMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t n = funcData.nVal;
    size_t m = funcData.mVal;

    if (n && !m)
        m = n;

    if (!n || !m)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(n, m, 0.0);
}


/////////////////////////////////////////////////
/// \brief This static function returns a matrix
/// filled with ones of the defined size.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix createOnesMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t n = funcData.nVal;
    size_t m = funcData.mVal;

    if (n && !m)
        m = n;

    if (!n || !m)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(n, m, 1.0);
}


/////////////////////////////////////////////////
/// \brief This static function returns an
/// identity matrix of the defined size.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix identityMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (!funcData.nVal)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mIdentity(funcData.nVal, funcData.nVal, 0.0);

    for (int i = 0; i < funcData.nVal; i++)
    {
        _mIdentity(i, i) = 1.0;
    }

    return _mIdentity;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// trace of the passed matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix calcTrace(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (!funcData.mat1.isSquare())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1));

    Matrix _mReturn(1, 1, 0.0);

    for (unsigned int i = 0; i < funcData.mat1.rows(); i++)
    {
        _mReturn(0, 0) += funcData.mat1(i, i);
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// determinant of the passed matrix using the
/// LaPlace algorithm.
///
/// \param _mMatrix const Matrix&
/// \param vRemovedLines std::vector<int>
/// \return mu::value_type
///
/////////////////////////////////////////////////
static mu::value_type calcDeterminant(const Matrix& _mMatrix, std::vector<int> vRemovedLines)
{
    // simple Sonderfaelle
    if (_mMatrix.rows() == 1)
        return _mMatrix(0, 0);

    if (_mMatrix.rows() == 2)
        return _mMatrix(0, 0) * _mMatrix(1, 1) - _mMatrix(1, 0)*_mMatrix(0, 1);

    if (_mMatrix.rows() == 3)
    {
        return _mMatrix(0, 0)*_mMatrix(1, 1)*_mMatrix(2, 2)
            + _mMatrix(0, 1)*_mMatrix(1, 2)*_mMatrix(2, 0)
            + _mMatrix(0, 2)*_mMatrix(1, 0)*_mMatrix(2, 1)
            - _mMatrix(0, 2)*_mMatrix(1, 1)*_mMatrix(2, 0)
            - _mMatrix(0, 1)*_mMatrix(1, 0)*_mMatrix(2, 2)
            - _mMatrix(0, 0)*_mMatrix(1, 2)*_mMatrix(2, 1);
    }

    int nSign = 1;
    mu::value_type dDet = 0.0;

    for (unsigned int i = 0; i < _mMatrix.rows(); i++)
    {
        // Noch nicht entfernte Zeile?
        if (!(vRemovedLines[i] & 1))
        {
            // entferne Zeile i
            vRemovedLines[i] += 1;

            for (unsigned int j = 0; j < _mMatrix.rows(); j++)
            {
                // Noch nicht entfernte Spalte?
                if (!(vRemovedLines[j] & 2))
                {
                    if (_mMatrix(i, j) == 0.0)
                    {
                        nSign *= -1;
                        continue;
                    }
                    // entferne Spalte j
                    vRemovedLines[j] += 2;

                    // Berechne Determinante rekursiv
                    if (i+1 < _mMatrix.rows())
                        dDet += (double)nSign * _mMatrix(i, j) * calcDeterminant(_mMatrix, vRemovedLines);
                    else
                        dDet += (double)nSign * _mMatrix(i, j);

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
/// \brief This static function calculates the
/// determinant of the passed matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix getDeterminant(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (!funcData.mat1.isSquare())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1));

    std::vector<int> vRemovedLines(funcData.mat1.rows(), 0);
    return Matrix(1, 1, calcDeterminant(funcData.mat1, vRemovedLines));
}


/////////////////////////////////////////////////
/// \brief This static function calulates the
/// n-dimensional cross product ("curl").
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix calcCrossProduct(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), 1, 0.0);
    std::vector<int> vRemovedLines(funcData.mat1.rows(), 0);

    if (funcData.mat1.rows() == 1)
        return _mResult;

    if (funcData.mat1.rows()-1 != funcData.mat1.cols())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1));

    if (funcData.mat1.rows() == 2)
    {
        _mResult(0) = funcData.mat1(0, 1);
        _mResult(1) = -funcData.mat1(0, 0);
        return _mResult;
    }

    if (funcData.mat1.rows() == 3)
    {
        _mResult(0) = funcData.mat1(1, 0)*funcData.mat1(2, 1) - funcData.mat1(2, 0)*funcData.mat1(1, 1);
        _mResult(1) = funcData.mat1(2, 0)*funcData.mat1(0, 1) - funcData.mat1(0, 0)*funcData.mat1(2, 1);
        _mResult(2) = funcData.mat1(0, 0)*funcData.mat1(1, 1) - funcData.mat1(1, 0)*funcData.mat1(0, 1);
        return _mResult;
    }

    Matrix _mTemp = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols()+1, 0.0);

    #pragma omp parallel for
    for (unsigned int i = 0; i < funcData.mat1.rows(); i++)
    {
        for (unsigned int j = 0; j < funcData.mat1.cols(); j++)
        {
            _mTemp(i, j+1) = funcData.mat1(i, j);
        }
    }

    _mTemp(0, 0) = 1.0;
    _mResult(0, 0) = calcDeterminant(_mTemp, vRemovedLines);

    for (unsigned int i = 1; i < funcData.mat1.rows(); i++)
    {
        _mTemp(i-1, 0) = 0.0;
        _mTemp(i, 0) = 1.0;
        _mResult(i, 0) = calcDeterminant(_mTemp, vRemovedLines);
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
/// \param errorInfo const MatFuncErrorInfo&
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
__attribute__((force_align_arg_pointer)) static Matrix calcEigenVectsAndValues(const Matrix& _mMatrix, int nReturnType, const MatFuncErrorInfo& errorInfo)
{
    if (_mMatrix.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (!_mMatrix.isSquare())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(_mMatrix));

    Matrix _mEigenVals;
    Matrix _mEigenVects;

    Eigen::MatrixXcd mMatrix(_mMatrix.rows(), _mMatrix.rows());

    // Copy the passed matrix into an Eigen matrix
    #pragma omp parallel for
    for (unsigned int i = 0; i < _mMatrix.rows(); i++)
    {
        for (unsigned int j = 0; j < _mMatrix.rows(); j++)
        {
            mMatrix(i,j) = _mMatrix(i,j);
        }
    }

    // Construct an Eigen eigenvalue solver
    Eigen::ComplexEigenSolver<Eigen::MatrixXcd> eSolver(mMatrix);

    // Get eigenvalues, eigenvectors or the diagonal matrix depending
    // on the selected return type. Separate the result into real and
    // imaginary parts
    if (nReturnType == EIGENVALUES)
    {
        Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
        _mEigenVals.resize(_mMatrix.rows(), 1);

        for (unsigned int i = 0; i < _mEigenVals.rows(); i++)
        {
            _mEigenVals(i) = vEigenVals(i, 0);
        }
    }
    else if (nReturnType == EIGENVECTORS)
    {
        Eigen::EigenSolver<Eigen::MatrixXcd>::EigenvectorsType mEigenVects = eSolver.eigenvectors();
        _mEigenVects.resize(_mMatrix.rows(), _mMatrix.cols());

        #pragma omp parallel for
        for (unsigned int i = 0; i < _mEigenVects.rows(); i++)
        {
            for (unsigned int j = 0; j < _mEigenVects.cols(); j++)
            {
                _mEigenVects(i, j) = mEigenVects(i, j);
            }
        }
    }
    else if (nReturnType == DIAGONALIZE)
    {
        Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
        _mEigenVects.resize(_mMatrix.rows(), _mMatrix.cols());

        for (unsigned int i = 0; i < _mEigenVects.rows(); i++)
        {
            _mEigenVects(i, i) = vEigenVals(i, 0);
        }
    }

    // Return the corresponding result
    if (nReturnType == EIGENVALUES)
        return _mEigenVals;
    else
        return _mEigenVects;
}


/////////////////////////////////////////////////
/// \brief Calculates the eigenvectors of the
/// passed matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix calcEigenVects(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    return calcEigenVectsAndValues(funcData.mat1, EIGENVECTORS, errorInfo);
}


/////////////////////////////////////////////////
/// \brief Calculates the eigenvalues of the
/// passed matrix
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix calcEigenValues(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    return calcEigenVectsAndValues(funcData.mat1, EIGENVALUES, errorInfo);
}


/////////////////////////////////////////////////
/// \brief Diagonalizes the passed matrix using
/// its eigenvalues.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix diagonalize(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    return calcEigenVectsAndValues(funcData.mat1, DIAGONALIZE, errorInfo);
}


/////////////////////////////////////////////////
/// \brief This static function creates a
/// shuffled vector of nShuffle elements created
/// from a 1:nBase vector.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix createShuffledMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    unsigned int nShuffle = funcData.nVal;
    unsigned int nBase = funcData.mVal;

    if (!nBase && nShuffle)
        nBase = nShuffle;

    if (!nBase)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mBase = createFilledMatrix(nBase, 1, 0.0);
    static double dSeed = 1;

    if (nShuffle > nBase)
        nShuffle = nBase;

    // Create the base (unshuffled) vector
    for (size_t i = 0; i < nBase; i++)
        _mBase(i) = i+1;

    // Initialize the random number engine using the
    // time and the last random number created by the
    // random engine
    std::default_random_engine randGen((double)time(0)*dSeed);

    // Shuffle the vector by swapping the i-th shuffled
    // element with the i-th element
    for (size_t i = 0; i < nShuffle; i++)
    {
        std::uniform_real_distribution<double> randDist(i, nBase-1);

        int nIndex = rint(randDist(randGen));
        mu::value_type dTemp = _mBase(i);
        _mBase(i) = _mBase(nIndex);
        _mBase(nIndex) = dTemp;
    }

    std::uniform_real_distribution<double> randDist(1, nBase-1);

    // Update the seed
    dSeed = randDist(randGen);

    // Return only the requested vector length
    _mBase.resize(nShuffle, 1);
    return _mBase;
}


/////////////////////////////////////////////////
/// \brief Calculates the inverse matrix and
/// checks in advance, whether the matrix is
/// invertible.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix invertMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (!funcData.mat1.isSquare())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1));

    // Gauss-Elimination???
    Matrix _mInverse = identityMatrix(MatFuncData(funcData.mat1.rows()), errorInfo);
    Matrix _mToInvert = funcData.mat1;

    mu::value_type dDet = getDeterminant(funcData, errorInfo)(0);

    if (dDet == 0.0)
        throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, errorInfo.command, errorInfo.position);

    //Spezialfaelle mit analytischem Ausdruck
    if (_mToInvert.rows() == 1)
    {
        // eigentlich nicht zwangslaeufig existent... aber skalar und so...
        _mInverse(0) /= _mToInvert(0);
        return _mInverse;
    }
    else if (_mToInvert.rows() == 2)
    {
        _mInverse = _mToInvert;
        _mInverse(0, 0) = _mToInvert(1, 1);
        _mInverse(1, 1) = _mToInvert(0, 0);
        _mInverse(1, 0) *= -1.0;
        _mInverse(0, 1) *= -1.0;

        for (unsigned int i = 0; i < 2; i++)
        {
            _mInverse(i, 0) /= dDet;
            _mInverse(i, 1) /= dDet;
        }

        return _mInverse;
    }
    else if (_mToInvert.rows() == 3)
    {
        _mInverse(0, 0) =  (_mToInvert(1, 1)*_mToInvert(2, 2) - _mToInvert(2, 1)*_mToInvert(1, 2)) / dDet;
        _mInverse(1, 0) = -(_mToInvert(1, 0)*_mToInvert(2, 2) - _mToInvert(1, 2)*_mToInvert(2, 0)) / dDet;
        _mInverse(1, 0) =  (_mToInvert(1, 0)*_mToInvert(2, 1) - _mToInvert(1, 1)*_mToInvert(2, 0)) / dDet;

        _mInverse(0, 1) = -(_mToInvert(0, 1)*_mToInvert(2, 2) - _mToInvert(0, 2)*_mToInvert(2, 1)) / dDet;
        _mInverse(1, 1) =  (_mToInvert(0, 0)*_mToInvert(2, 2) - _mToInvert(0, 2)*_mToInvert(2, 0)) / dDet;
        _mInverse(2, 1) = -(_mToInvert(0, 0)*_mToInvert(2, 1) - _mToInvert(0, 1)*_mToInvert(2, 0)) / dDet;

        _mInverse(0, 2) =  (_mToInvert(0, 1)*_mToInvert(1, 2) - _mToInvert(0, 2)*_mToInvert(1, 1)) / dDet;
        _mInverse(1, 2) = -(_mToInvert(0, 0)*_mToInvert(1, 2) - _mToInvert(0, 2)*_mToInvert(1, 0)) / dDet;
        _mInverse(2, 2) =  (_mToInvert(0, 0)*_mToInvert(1, 1) - _mToInvert(0, 1)*_mToInvert(1, 0)) / dDet;

        return _mInverse;
    }

    // Allgemeiner Fall fuer n > 3
    for (unsigned int j = 0; j < _mToInvert.cols(); j++)
    {
        for (unsigned int i = j; i < _mToInvert.rows(); i++)
        {
            if (_mToInvert(i, j) != 0.0)
            {
                if (i != j) //vertauschen
                {
                    mu::value_type dElement;

                    for (unsigned int _j = 0; _j < _mToInvert.cols(); _j++)
                    {
                        dElement = _mToInvert(i, _j);
                        _mToInvert(i, _j) = _mToInvert(j, _j);
                        _mToInvert(j, _j) = dElement;
                        dElement = _mInverse(i, _j);
                        _mInverse(i, _j) = _mInverse(j, _j);
                        _mInverse(j, _j) = dElement;
                    }

                    i = j-1;
                }
                else //Gauss-Elimination
                {
                    mu::value_type dPivot = _mToInvert(i, j);

                    for (unsigned int _j = 0; _j < _mToInvert.cols(); _j++)
                    {
                        _mToInvert(i, _j) /= dPivot;
                        _mInverse(i, _j) /= dPivot;
                    }

                    for (unsigned int _i = i+1; _i < _mToInvert.rows(); _i++)
                    {
                        mu::value_type dFactor = _mToInvert(_i, j);

                        if (dFactor == 0.0) // Bereits 0???
                            continue;

                        for (unsigned int _j = 0; _j < _mToInvert.cols(); _j++)
                        {
                            _mToInvert(_i, _j) -= _mToInvert(i, _j)*dFactor;
                            _mInverse(_i, _j) -= _mInverse(i, _j)*dFactor;
                        }
                    }

                    break;
                }
            }
        }
    }

    // die Matrix _mToInvert() sollte nun Dreiecksgestalt besitzen. Jetzt den Gauss von unten her umkehren
    for (int j = (int)_mToInvert.rows()-1; j >= 0; j--)
    {
        if (_mToInvert(j, j) == 0.0) // Hauptdiagonale ist ein Element == 0??
            throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, errorInfo.command, errorInfo.position);

        if (_mToInvert(j, j) != 1.0)
        {
            for (unsigned int _j = 0; _j < _mInverse.cols(); _j++)
                _mInverse(j, _j) /= _mToInvert(j, j);

            _mToInvert(j, j) = 1.0;
        }

        for (int i = 0; i < j; i++)
        {
            for (unsigned int _j = 0; _j < _mInverse.cols(); _j++)
                _mInverse(i, _j) -= _mToInvert(i, j)*_mInverse(j, _j);

            _mToInvert(i, j) -= _mToInvert(i, j)*_mToInvert(j, j);
        }
    }

    return _mInverse;
}


/////////////////////////////////////////////////
/// \brief This static function transposes a
/// matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix transposeMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    Matrix _mTransposed(funcData.mat1);
    _mTransposed.transpose();
    return _mTransposed;
}


/////////////////////////////////////////////////
/// \brief This static function converts logical
/// vectors into matrix indices.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix logToIndex(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    std::vector<double> vLines;
    std::vector<double> vRows;

    if (funcData.mat1.rows() == 1 || funcData.mat1.cols() == 1)
    {
        for (size_t i = 0; i < funcData.mat1.rows(); i++)
        {
            for (size_t j = 0; j < funcData.mat1.cols(); j++)
            {
                if (funcData.mat1(i, j).real())
                    vLines.push_back(i + j + 1);
            }
        }

        if (!vLines.size())
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(vLines.size(), 1, 0.0);

        for (size_t i = 0; i < vLines.size(); i++)
            _mReturn(i) = vLines[i];

        return _mReturn;
    }
    else
    {
        for (size_t i = 0; i < funcData.mat1.rows(); i++)
        {
            for (size_t j = 0; j < funcData.mat1.cols(); j++)
            {
                if (funcData.mat1(i, j).real())
                {
                    vLines.push_back(i+1);
                    vRows.push_back(j+1);
                }
            }
        }

        if (!vLines.size())
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(vLines.size(), 2, 0.0);

        for (size_t i = 0; i < vLines.size(); i++)
        {
            _mReturn(i, 0) = vLines[i];
            _mReturn(i, 1) = vRows[i];
        }

        return _mReturn;
    }
}


/////////////////////////////////////////////////
/// \brief Driver code to calculate some
/// statistics from a matrix while being able to
/// use moving windows. The calculation is
/// switched to parallel mode, once the moving
/// window is large enough.
///
/// \param mat const Matrix&
/// \param logic StatsLogic
/// \param rowStart int
/// \param rowCount size_t
/// \param colStart int
/// \param colCount size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
static mu::value_type calculateStats(const Matrix& mat, StatsLogic logic, int rowStart, size_t rowCount, int colStart, size_t colCount)
{
    constexpr size_t MINTHREADCOUNT = 100;
    constexpr size_t MINELEMENTPERCOL = 100;

    std::vector<StatsLogic> operation(rowCount, logic);

    // Only apply multiprocessing, if there are really a lot of
    // elements to process
    if (rowCount >= MINTHREADCOUNT && colCount >= MINELEMENTPERCOL)
    {
        #pragma omp parallel for
        for (size_t i = 0; i < rowCount; i++)
        {
            if (rowStart + (int)i < 0 || rowStart + i >= mat.rows())
                continue;

            for (size_t j = 0; j < colCount; j++)
            {
                if (colStart + (int)j < 0 || colStart + j >= mat.cols())
                    continue;

                operation[i](mat(i+rowStart, j+colStart));
            }
        }
    }
    else
    {
        for (size_t i = 0; i < rowCount; i++)
        {
            if (rowStart + (int)i < 0 || rowStart + i >= mat.rows())
                continue;

            for (size_t j = 0; j < colCount; j++)
            {
                if (colStart + (int)j < 0 || colStart + j >= mat.cols())
                    continue;

                operation[i](mat(i+rowStart, j+colStart));
            }
        }
    }

    for (size_t i = 1; i < operation.size(); i++)
    {
        operation.front().combine(operation[i]);
    }

    return operation.front().m_val;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c max() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMax(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MAX, funcData.mat1(0).real()),
                                                   0, funcData.mat1.rows(), 0, funcData.mat1.cols()));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c max()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovMax(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MAX, funcData.mat1(i, j).real()),
                                                i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function converts matrix
/// indices into logical vectors.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix indexToLog(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.rows() == 1 || funcData.mat1.cols() == 1)
    {
        Matrix _mMatrixMax = matrixMax(funcData, errorInfo);

        if (!_mMatrixMax(0).real() || _mMatrixMax(0).real() < 0)
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(_mMatrixMax(0).real(), 1, 0.0);

        for (size_t i = 0; i < funcData.mat1.rows(); i++)
        {
            for (size_t j = 0; j < funcData.mat1.cols(); j++)
            {
                if (funcData.mat1(i, j).real() > 0)
                    _mReturn(funcData.mat1(i, j).real()-1) = 1.0;
            }
        }

        return _mReturn;
    }
    else
    {
        std::vector<double> vCol;
        std::vector<double> vRow;

        if (funcData.mat1.rows() == 2 && funcData.mat1.cols() != 2)
        {
            for (size_t i = 0; i < funcData.mat1.cols(); i++)
            {
                vRow.push_back(funcData.mat1(0, i).real());
                vCol.push_back(funcData.mat1(1, i).real());
            }
        }
        else if (funcData.mat1.rows() != 2 && funcData.mat1.cols() == 2)
        {
            for (size_t i = 0; i < funcData.mat1.rows(); i++)
            {
                vRow.push_back(funcData.mat1(i, 0).real());
                vCol.push_back(funcData.mat1(i, 1).real());
            }
        }
        else
            return createFilledMatrix(1, 1, 0.0);

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
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(nRowMax, nColMax, 0.0);

        for (size_t i = 0; i < vRow.size(); i++)
        {
            if (vRow[i] > 0 && vCol[i] > 0)
                _mReturn(vRow[i]-1, vCol[i]-1) = 1.0;
        }

        return _mReturn;
    }
}


/////////////////////////////////////////////////
/// \brief This static function returns the size
/// of the passed matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixSize(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(2, 1, 0.0);

    if (funcData.mat1.rows() == 1 && funcData.mat1.cols() == 1 && isnan(funcData.mat1(0)))
        return _mReturn;

    _mReturn(0) = funcData.mat1.rows();
    _mReturn(1) = funcData.mat1.cols();

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c and() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixAnd(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            if (funcData.mat1(i, j) == 0.0)
                return createFilledMatrix(1, 1, 0.0);
        }
    }

    return createFilledMatrix(1, 1, 1.0);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c or() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixOr(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            if (funcData.mat1(i, j) != 0.0)
                return createFilledMatrix(1, 1, 1.0);
        }
    }

    return createFilledMatrix(1, 1, 0.0);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c xor() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixXor(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    bool isTrue = false;

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            if (funcData.mat1(i, j) != 0.0)
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return createFilledMatrix(1, 1, 0.0);
            }
        }
    }

    if (isTrue)
        return createFilledMatrix(1, 1, 1.0);

    return createFilledMatrix(1, 1, 0.0);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c sum() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixSum(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADD),
                                                   0, funcData.mat1.rows(), 0, funcData.mat1.cols()));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c sum()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovSum(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADD),
                                                i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c num() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixNum(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_NUM),
                                                   0, funcData.mat1.rows(), 0, funcData.mat1.cols()));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c num()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovNum(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_NUM),
                                                i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c avg() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixAvg(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mSum = matrixSum(funcData, errorInfo);
    Matrix _mNum = matrixNum(funcData, errorInfo);

    return createFilledMatrix(1, 1, _mSum(0) / _mNum(0));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c avg()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovAvg(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
            {
                mu::value_type sum = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADD),
                                                    i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);

                double num = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_NUM),
                                            i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1).real();

                if (num > 0)
                    _mResult(i, j) = sum / num;
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c std() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixStd(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mAvg = matrixAvg(funcData, errorInfo);
    Matrix _mNum = matrixNum(funcData, errorInfo);
    Matrix _mReturn = createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADDSQSUB, 0.0, _mAvg(0)),
                                                              0, funcData.mat1.rows(), 0, funcData.mat1.cols()));

    _mReturn(0) = std::sqrt(_mReturn(0)/(_mNum(0).real()-1.0));
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c std()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovStd(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
            {
                mu::value_type sum = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADD),
                                                    i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);

                double num = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_NUM),
                                            i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1).real();

                mu::value_type std = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADDSQSUB, 0.0, sum/num),
                                                    i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);

                if (num > 1)
                    _mResult(i, j) = std::sqrt(std / (num - 1.0));
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c prd() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixPrd(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MULT, 1.0),
                                                   0, funcData.mat1.rows(), 0, funcData.mat1.cols()));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c prd()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovPrd(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MULT, 1.0),
                                                i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c cnt() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixCnt(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, funcData.mat1.rows() * funcData.mat1.cols());
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c norm() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixNorm(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADDSQ),
                                                              0, funcData.mat1.rows(), 0, funcData.mat1.cols()));

    _mReturn(0) = std::sqrt(_mReturn(0));
    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c norm()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovNorm(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = std::sqrt(calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_ADDSQ),
                                                          i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1));
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c min() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMin(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MIN, funcData.mat1(0).real()),
                                                   0, funcData.mat1.rows(), 0, funcData.mat1.cols()));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c min()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovMin(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
                _mResult(i, j) = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MIN, funcData.mat1(i, j).real()),
                                                i-funcData.nVal, 2*funcData.nVal+1, j-funcData.mVal, 2*funcData.mVal+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c cmp() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixCmp(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);
    Matrix _mCoords = createFilledMatrix(2, 1, 0.0);
    _mCoords(0) = -1;

    mu::value_type dValue = funcData.fVal;
    int nType = funcData.nVal;
    mu::value_type dKeep = dValue;

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            if (isnan(funcData.mat1(i, j)) || isinf(funcData.mat1(i, j)))
                continue;

            if (funcData.mat1(i, j) == dValue)
            {
                if (!nType || abs(nType) <= 1)
                {
                    _mCoords(0) = i+1;
                    _mCoords(1) = j+1;
                    return _mCoords;
                }
                else
                    _mReturn(0) = funcData.mat1(i, j);

                return _mReturn;
            }
            else if (nType > 0 && funcData.mat1(i, j).real() > dValue.real())
            {
                if (_mCoords(0) == -1.0 || funcData.mat1(i, j).real() < dKeep.real())
                {
                    dKeep = funcData.mat1(i, j).real();
                    _mCoords(0) = i+1;
                    _mCoords(1) = j+1;
                }
                else
                    continue;
            }
            else if (nType < 0 && funcData.mat1(i, j).real() < dValue.real())
            {
                if (_mCoords(0) == -1.0 || funcData.mat1(i, j).real() > dKeep.real())
                {
                    dKeep = funcData.mat1(i, j).real();
                    _mCoords(0) = i+1;
                    _mCoords(1) = j+1;
                }
                else
                    continue;
            }
        }
    }

    if (_mCoords(0) == -1.0)
        _mReturn(0) = NAN;
    else if (nType <= -2 || nType >= 2)
        _mReturn(0) = dKeep;
    else
    {
        return _mCoords;
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c minpos() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMinPos(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return matrixCmp(MatFuncData(funcData.mat1, matrixMin(funcData, errorInfo)(0), 0), errorInfo);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c maxpos() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMaxPos(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return matrixCmp(MatFuncData(funcData.mat1, matrixMax(funcData, errorInfo)(0), 0), errorInfo);
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c med() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMed(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Memory _mem;

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            _mem.writeData(j + i*funcData.mat1.rows(), 0, funcData.mat1(i, j));
        }
    }

    return createFilledMatrix(1, 1, _mem.med(VectorIndex(0, funcData.mat1.rows()*funcData.mat1.cols()-1), VectorIndex(0)));
}


/////////////////////////////////////////////////
/// \brief Moving window version of the \c med()
/// function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixMovMed(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.nVal < 0 || funcData.mVal < 0
        || 2*funcData.nVal+1 > (int)funcData.mat1.rows() || 2*funcData.mVal+1 > (int)funcData.mat1.cols())
        throw SyntaxError(SyntaxError::INVALID_STATS_WINDOW_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.rows(), funcData.mat1.cols(), NAN);

    #pragma omp parallel for
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!isnan(funcData.mat1(i, j)))
            {
                Memory _mem;

                for (int n = 0; n < 2*funcData.nVal+1; n++)
                {
                    if (i+n-funcData.nVal < 0 || i+n-funcData.nVal >= (int)funcData.mat1.rows())
                        continue;

                    for (int m = 0; m < 2*funcData.mVal+1; m++)
                    {
                        if (j+m-funcData.mVal < 0 || j+m-funcData.mVal >= (int)funcData.mat1.cols())
                            continue;

                        _mem.writeData(m + n*(2*funcData.mVal+1), 0, funcData.mat1(i+n-funcData.nVal, j+m-funcData.mVal));
                    }
                }

                _mResult(i, j) = _mem.med(VectorIndex(0, (2*funcData.nVal+1)*(2*funcData.mVal+1)), VectorIndex(0));
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// \c pct() function on the matrix elements.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixPct(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Memory _mem;

    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            _mem.writeData(j + i*funcData.mat1.rows(), 0, funcData.mat1(i, j));
        }
    }

    return createFilledMatrix(1, 1, _mem.pct(VectorIndex(0, (long long int)(funcData.mat1.rows()*funcData.mat1.cols())-1), VectorIndex(0), funcData.fVal));
}


/////////////////////////////////////////////////
/// \brief This static function changes the size
/// of the passed matrix to fit the new size.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/// Missing elements are filled up with zeros.
/////////////////////////////////////////////////
static Matrix matrixResize(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t nLines = funcData.fVal.real();
    size_t nCols = funcData.nVal;

    if (funcData.mat1.isEmpty() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (nLines == funcData.mat1.rows() && nCols == funcData.mat1.cols())
        return funcData.mat1;

    Matrix _mReturn(funcData.mat1);
    _mReturn.resize(nLines, nCols);

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// cross- and auto-correlation matrix
/// calculation from the passed two matrices.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/// If their sizes are not fitting, they are
/// resized to fit.
/////////////////////////////////////////////////
static Matrix correlation(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty() || funcData.mat2.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Ensure that the size is non-zero
    if (!(funcData.mat1.rows() && funcData.mat2.rows()) || !(funcData.mat1.cols() && funcData.mat2.cols()))
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + ", " + printMatrixDim(funcData.mat2));

    int n = std::max(funcData.mat1.rows(), funcData.mat2.rows());
    int m = std::max(funcData.mat1.cols(), funcData.mat2.cols());

    // Resize the matrices to fit their counterparts
    Matrix mMatrix1(funcData.mat1);
    mMatrix1.resize(n, m);

    Matrix mMatrix2(funcData.mat2);
    mMatrix2.resize(n, m);

    // Create the target matrix
    Matrix mCorrelation = createFilledMatrix(2*n-1, 2*m-1, 0.0);

    // Calculate the elements of the matrix by applying
    // elementwise shifts to the matrices
    #pragma omp parallel for
    for (int i1 = 0; i1 < (int)mCorrelation.rows(); i1++)
    {
        for (int j1 = 0; j1 < (int)mCorrelation.cols(); j1++)
        {
            // These loops shall indicate the number of elements
            for (int i2 = 0; i2 < n + std::min(i1-n+1, n-i1-1); i2++)
            {
                for (int j2 = 0; j2 < m + std::min(j1-m+1, m-j1-1); j2++)
                {
                    // calculate the correlation of the current
                    // shift indicated by the other two loops
                    mCorrelation(i1, j1) += mMatrix1(i2 + std::max(0, i1-n+1), j2 + std::max(0, j1-m+1))
                                            * mMatrix2(i2 + std::max(0, n-i1-1), j2 + std::max(0, m-j1-1));
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
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix covariance(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty() || funcData.mat2.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Ensure that their size is equal
    if (funcData.mat1.rows() != funcData.mat2.rows() || funcData.mat1.cols() != funcData.mat2.cols())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " != " + printMatrixDim(funcData.mat2));

    // Prepare the target matrix
    Matrix mCovariance = createFilledMatrix(1, 1, 0.0);

    // Calculate the average values of both
    // matrices
    Matrix mAvg1 = matrixAvg(MatFuncData(funcData.mat1), errorInfo);
    Matrix mAvg2 = matrixAvg(MatFuncData(funcData.mat2), errorInfo);

    // Calculate the covariance value for each
    // component and sum it up
    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat2.rows(); j++)
        {
            mCovariance(0) += (funcData.mat1(i, j) - mAvg1(0)) * (funcData.mat2(i, j) - mAvg2(0));
        }
    }

    // Normalize the covariance value using
    // the number of elements
    mCovariance(0) /= (funcData.mat1.rows() * funcData.mat1.cols() - 1);

    return mCovariance;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// normalize function, which will normalize the
/// (absolute) data range of the passed matrix
/// into the range [0,1].
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix normalize(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = funcData.mat1;
    Matrix _mMax = matrixMax(funcData, errorInfo);
    Matrix _mMin = matrixMin(funcData, errorInfo);

    double dMax = std::max(fabs(_mMax(0)), fabs(_mMin(0)));

    #pragma omp parallel for
    for (size_t  i = 0; i < _mReturn.rows(); i++)
    {
        for (size_t j = 0; j < _mReturn.cols(); j++)
        {
            _mReturn(i, j) /= dMax;
        }
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function changes the
/// number of rows and columns to fit the new
/// shape.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/// The total number of elements before and
/// afterwards must be identical.
/////////////////////////////////////////////////
static Matrix matrixReshape(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t nLines = funcData.fVal.real();
    size_t nCols = funcData.nVal;

    if (funcData.mat1.isEmpty() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (nLines * nCols != funcData.mat1.rows() * funcData.mat1.cols())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          toString(nLines) + "x" + toString(nCols) + "=" + toString(nLines*nCols) +" vs. "
                          + printMatrixDim(funcData.mat1) + "=" + toString(funcData.mat1.rows()*funcData.mat1.cols()));

    Matrix _mReturn = createFilledMatrix(nLines, nCols, 0.0);

    for (size_t i = 0; i < nLines*nCols; i++)
    {
        _mReturn(i / nCols, i % nCols) = funcData.mat1(i / funcData.mat1.cols(), i % funcData.mat1.cols());
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function repeats the
/// passed matrix n and m times.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixRepMat(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t n = funcData.fVal.real();
    size_t m = funcData.nVal;

    if (n == 0)
        n++;

    if (m == 0)
        m++;

    if ((n * funcData.mat1.rows()) / funcData.mat1.rows() != n || (m * funcData.mat1.cols()) / funcData.mat1.cols() != m)
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(n * funcData.mat1.rows(), m * funcData.mat1.cols(), 0.0);

    #pragma omp parallel for
    for (size_t i = 0; i < funcData.mat1.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            for (size_t _n = 0; _n < n; _n++)
            {
                for (size_t _m = 0; _m < m; _m++)
                {
                    _mReturn(i+_n*funcData.mat1.rows(), j+_m*funcData.mat1.cols()) = funcData.mat1(i, j);
                }
            }
        }
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// std::list::remove_if() called in
/// getUniqueList().
///
/// \param value const mu::value_type&
/// \return bool
///
/////////////////////////////////////////////////
static bool is_nan(const mu::value_type& value)
{
    return isnan(value);
}


/////////////////////////////////////////////////
/// \brief Static helper function for std::sort()
/// called in getUniqueList(). Determines a
/// real-only order.
///
/// \param value1 const mu::value_type&
/// \param value2 const mu::value_type&
/// \return bool
///
/////////////////////////////////////////////////
static bool isSmallerRealOnly(const mu::value_type& value1, const mu::value_type& value2)
{
    return value1.real() < value2.real();
}


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the implementation of the \c unique()
/// function.
///
/// \param _list std::list<mu::value_type>&
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
static std::vector<mu::value_type> getUniqueList(std::list<mu::value_type>& _list)
{
    _list.remove_if(is_nan);

    if (_list.empty())
        return std::vector<mu::value_type>(1, NAN);

    std::vector<mu::value_type> vReturn;

    for (auto& val : _list)
    {
        if (std::find(vReturn.begin(), vReturn.end(), val) == vReturn.end())
            vReturn.push_back(val);
    }

    std::sort(vReturn.begin(), vReturn.end(), isSmallerRealOnly);

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// \c unique(MAT,nDim) function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix matrixUnique(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Create a std::list and the return value
    std::list<mu::value_type> dataList;
    Matrix _mReturn;

    // Depending on the dimensions of the passed matrix, change
    // the evaluation method
    if (funcData.mat1.rows() == 1)
    {
        // Row vector
        dataList.assign(funcData.mat1.data().begin(), funcData.mat1.data().end());
        auto uniqueList = getUniqueList(dataList);
        _mReturn.assign(1, uniqueList.size(), uniqueList);
    }
    else if (funcData.mat1.cols() == 1)
    {
        // Column vector
        dataList.assign(funcData.mat1.data().begin(), funcData.mat1.data().end());
        auto uniqueList = getUniqueList(dataList);
        _mReturn.assign(uniqueList.size(), 1, uniqueList);
    }
    else
    {
        // Matrix
        if (!funcData.nVal)
        {
            // funcData.nVal == 0 -> Roll out the total matrix and return it as a overall row vector
            Matrix retVal = matrixReshape(MatFuncData(funcData.mat1, mu::value_type(1.0), funcData.mat1.rows()*funcData.mat1.cols()),
                                          errorInfo);
            dataList.assign(retVal.data().begin(), retVal.data().end());
            auto uniqueList = getUniqueList(dataList);
            _mReturn.assign(uniqueList.size(), 1, uniqueList);
        }
        else if (funcData.nVal == 1)
        {
            std::vector<std::vector<mu::value_type>> mBuffer;

            // Make the rows unique
            for (size_t i = 0; i < funcData.mat1.rows(); i++)
            {
                dataList.clear();

                for (size_t j = 0; j < funcData.mat1.cols(); j++)
                    dataList.push_back(funcData.mat1(i, j));

                mBuffer.push_back(getUniqueList(dataList));
            }

            _mReturn = Matrix(mBuffer);
        }
        else
        {
            std::vector<std::vector<mu::value_type>> mBuffer;

            // Make the columns unique
            for (size_t j = 0; j < funcData.mat1.cols(); j++)
            {
                dataList.clear();

                for (size_t i = 0; i < funcData.mat1.rows(); i++)
                    dataList.push_back(funcData.mat1(i, j));

                mBuffer.push_back(getUniqueList(dataList));
            }

            _mReturn = Matrix(mBuffer);
            _mReturn.transpose();
        }
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function solves the system
/// of linear equations symbolically.
///
/// \param _mMatrix const Matrix&
/// \param errorInfo const MatFuncErrorInfo&
/// \return void
///
/////////////////////////////////////////////////
static void solveLGSSymbolic(const Matrix& _mMatrix, const MatFuncErrorInfo& errorInfo)
{
    if (_mMatrix.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    std::string sSolution = "sle(";
    std::vector<std::string> vResult(_mMatrix.cols()-1, "");
    bool bIsZeroesLine = true;
    unsigned int nVarCount = 0;

    Matrix _mToSolve = createFilledMatrix(_mMatrix.cols()-1, _mMatrix.cols(), 0.0);
    Matrix _mCoefficents = createFilledMatrix(_mMatrix.cols()-1, _mMatrix.cols(), 0.0);

    #pragma omp parallel for
    for (unsigned int i = 0; i < std::min(_mMatrix.rows(), _mMatrix.cols()-1); i++)
    {
        for (unsigned int j = 0; j < _mMatrix.cols(); j++)
        {
            _mToSolve(i, j) = _mMatrix(i, j);
        }
    }

    for (int i = _mToSolve.rows()-1; i >= 0; i--)
    {
        bIsZeroesLine = true;

        for (unsigned int j = 0; j < _mToSolve.cols(); j++)
        {
            if (bIsZeroesLine && _mToSolve(i, j) == 0.0)
            {
                bIsZeroesLine = false;
                break;
            }
        }

        if (bIsZeroesLine)
        {
            _mCoefficents(i, i) = 1.0;
            nVarCount++;
        }
        else
        {
            // Konstanter Term
            _mCoefficents(i, _mCoefficents.cols()-1) = _mToSolve(i, _mToSolve.cols()-1);

            for (unsigned int j = i+1; j < _mToSolve.cols()-1; j++)
            {
                if (_mToSolve(i, j) != 0.0)
                {
                    // Konstanter Term
                    _mCoefficents(i, _mCoefficents.cols()-1) -= _mToSolve(i, j) * _mCoefficents(j, _mCoefficents.cols()-1);

                    // Alle Koeffizienten
                    for (unsigned int n = i+1; n < _mCoefficents.cols()-1; n++)
                    {
                        _mCoefficents(i, n) -= _mToSolve(i, j) * _mCoefficents(j, n);
                    }
                }
            }
        }
    }

    for (unsigned int i = 0; i < _mCoefficents.rows(); i++)
    {
        for (unsigned int j = 0; j < _mCoefficents.cols()-1; j++)
        {
            if (_mCoefficents(i, j) != 0.0)
            {
                if (fabs(_mCoefficents(i, j)) != 1.0)
                    vResult[i] += toString(_mCoefficents(i, j), 5) + "*";

                if (_mCoefficents(i, j) == -1.0)
                    vResult[i] += "-";

                vResult[i] += ('z'-vResult.size()+1+j);
            }
        }

        if (_mCoefficents(i, _mCoefficents.cols()-1) != 0.0)
            vResult[i] += "+" + toString(_mCoefficents(i, _mCoefficents.cols()-1), 5);

        while (vResult[i].find("+-") != std::string::npos)
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

    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

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
/// \brief This static function will solve the
/// system of linear equations passed as matrix
/// using the Gauss elimination algorithm.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix solveLGS(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.cols()-1, 1, 0.0);
    Matrix _mToSolve = funcData.mat1;

    if (_mToSolve.rows() == 1)
    {
        _mResult(0) = _mToSolve(0, 1)/_mToSolve(0);
        return _mResult;
    }

    // Allgemeiner Fall fuer n > 2
    for (unsigned int j = 0; j < _mToSolve.cols()-1; j++)
    {
        for (unsigned int i = j; i < _mToSolve.rows(); i++)
        {
            if (_mToSolve(i, j) != 0.0)
            {
                if (i != j) //vertauschen
                {
                    mu::value_type dElement;

                    for (unsigned int _j = 0; _j < _mToSolve.cols(); _j++)
                    {
                        dElement = _mToSolve(i, _j);
                        _mToSolve(i, _j) = _mToSolve(j, _j);
                        _mToSolve(j, _j) = dElement;
                    }

                    i = j-1;
                }
                else //Gauss-Elimination
                {
                    mu::value_type dPivot = _mToSolve(i, j);

                    for (unsigned int _j = 0; _j < _mToSolve.cols(); _j++)
                    {
                        _mToSolve(i, _j) /= dPivot;
                    }

                    #pragma omp parallel for
                    for (unsigned int _i = i+1; _i < _mToSolve.rows(); _i++)
                    {
                        mu::value_type dFactor = _mToSolve(_i, j);

                        if (dFactor == 0.0) // Bereits 0???
                            continue;

                        for (unsigned int _j = 0; _j < _mToSolve.cols(); _j++)
                        {
                            _mToSolve(_i, _j) -= _mToSolve(i, _j)*dFactor;
                        }
                    }

                    break;
                }
            }
            /*else if (_mToSolve[i][j] == 0.0 && j+1 == _mToSolve.size()) // Matrix scheint keinen vollen Rang zu besitzen
                throw MATRIX_IS_NOT_INVERTIBLE;*/
        }
    }

    if ((_mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-2) == 0.0 && _mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-1) == 0.0)
        || _mToSolve.rows()+1 < _mToSolve.cols())
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
        solveLGSSymbolic(_mToSolve, errorInfo);
        return _mToSolve;
    }
    else if (_mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-2) == 0.0 && _mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-1) != 0.0)
        throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, errorInfo.command, errorInfo.position);
    else if ((_mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-2) == 0.0 && _mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-1) == 0.0)
        || _mToSolve.rows()+1 > _mToSolve.cols())
    {
        // Ggf. Nullzeilen nach unten tauschen
        std::vector<bool> vIsZerosLine(_mToSolve.rows(), true);

        for (unsigned int i = 0; i < _mToSolve.rows(); i++)
        {
            for (unsigned int j = 0; j < _mToSolve.cols(); j++)
            {
                if (_mToSolve(i, j) != 0.0)
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
                        mu::value_type dElement;

                        for (unsigned int _j = 0; _j < _mToSolve.cols(); _j++)
                        {
                            dElement = _mToSolve(i, _j);
                            _mToSolve(i, _j) = _mToSolve(_i, _j);
                            _mToSolve(_i, _j) = dElement;
                        }

                        vIsZerosLine[i] = false;
                        vIsZerosLine[_i] = true;
                        break;
                    }
                }
            }
        }

        if (_mToSolve(_mToSolve.cols()-2, _mToSolve.cols()-2) == 0.0 && _mToSolve(_mToSolve.cols()-2, _mToSolve.cols()-1) == 0.0)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
            solveLGSSymbolic(_mToSolve, errorInfo);
            return _mToSolve;
        }

        _mResult(_mResult.cols()-2, 0) = _mToSolve(_mToSolve.cols()-2, _mToSolve.cols()-1);
    }
    else
        _mResult(_mResult.rows()-1, 0) = _mToSolve(_mToSolve.rows()-1, _mToSolve.cols()-1);

    for (int i = _mToSolve.cols()-3; i >= 0; i--)
    {
        for (unsigned int j = 0; j < _mToSolve.cols()-1; j++)
        {
            _mToSolve(i, _mToSolve.cols()-1) -= _mToSolve(i, j)*_mResult(j, 0);

            if (_mToSolve(i, j)*_mResult(j, 0) != 0.0)
                _mToSolve(i, j) = 0.0;
        }

        if (_mToSolve(i, i) == 0.0 && _mToSolve(i, _mToSolve.cols()-1) == 0.0)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("ERR_NR_2101_0_LGS_HAS_NO_UNIQUE_SOLUTION")));
            solveLGSSymbolic(_mToSolve, errorInfo);
            return _mToSolve;
        }

        if (_mToSolve(i, i) == 0.0 && _mToSolve(i, _mToSolve.cols()-1) != 0.0)
            throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, errorInfo.command, errorInfo.position);

        _mResult(i, 0) = _mToSolve(i, _mToSolve.cols()-1);
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function implements the
/// "diag()" function.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix diagonalMatrix(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _reshapedMat = matrixReshape(MatFuncData(funcData.mat1, mu::value_type(funcData.mat1.rows() * funcData.mat1.cols()), 1),
                                        errorInfo);
    Matrix _diagonalMat = createFilledMatrix(_reshapedMat.rows(), _reshapedMat.rows(), 0.0);

    for (size_t i = 0; i < _reshapedMat.rows(); i++)
    {
        _diagonalMat(i, i) = _reshapedMat(i);
    }

    return _diagonalMat;
}


/////////////////////////////////////////////////
/// \brief Converts cartesian to cylindrical
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix cartToCyl(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = std::sqrt(intPower(funcData.mat1(i, 0), 2) + intPower(funcData.mat1(i, 1), 2));
        _mReturn(i, 1) = parser_phi(funcData.mat1(i, 0), funcData.mat1(i, 1));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Converts cartesian to polar
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix cartToPolar(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    if (funcData.mat1.cols() == 2)
        return cartToCyl(funcData, errorInfo);

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = std::sqrt(intPower(funcData.mat1(i, 0), 2)
                                   + intPower(funcData.mat1(i, 1), 2)
                                   + intPower(funcData.mat1(i, 2), 2));
        _mReturn(i, 1) = parser_phi(funcData.mat1(i, 0), funcData.mat1(i, 1));
        _mReturn(i, 2) = parser_theta(funcData.mat1(i, 0), funcData.mat1(i, 1), funcData.mat1(i, 2));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Converts cylindrical to cartesian
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix cylToCart(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = funcData.mat1(i, 0) * cos(funcData.mat1(i, 1));
        _mReturn(i, 1) = funcData.mat1(i, 0) * sin(funcData.mat1(i, 1));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Converts cylindrical to polar
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix cylToPolar(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    if (funcData.mat1.cols() == 2)
        return funcData.mat1;

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = std::sqrt(intPower(funcData.mat1(i, 0), 2) + intPower(funcData.mat1(i, 2), 2));
        _mReturn(i, 2) = parser_theta(funcData.mat1(i, 0) * cos(funcData.mat1(i, 1)),
                                      funcData.mat1(i, 0) * sin(funcData.mat1(i, 1)),
                                      funcData.mat1(i, 2));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Converts polar to cartesian
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix polarToCart(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    if (funcData.mat1.cols() == 2)
        return cylToCart(funcData, errorInfo);

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = funcData.mat1(i, 0) * cos(funcData.mat1(i, 1)) * sin(funcData.mat1(i, 2));
        _mReturn(i, 1) = funcData.mat1(i, 0) * sin(funcData.mat1(i, 1)) * sin(funcData.mat1(i, 2));
        _mReturn(i, 2) = funcData.mat1(i, 0) * cos(funcData.mat1(i, 2));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Converts polar to cylindrical
/// coordinates.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix polarToCyl(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 3 || funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x3");

    Matrix _mReturn = funcData.mat1;

    #pragma omp parallel for
    for (size_t i = 0; i < _mReturn.rows(); i++)
    {
        _mReturn(i, 0) = funcData.mat1(i, 0) * sin(funcData.mat1(i, 2));

        if (funcData.mat1.cols() == 3)
            _mReturn(i, 2) = funcData.mat1(i, 0) * cos(funcData.mat1(i, 2));
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief This static function finds the nearest
/// lower grid axis value.
///
/// \param gaxes const Matrix&
/// \param axis size_t
/// \param axisval double
/// \return size_t
///
/////////////////////////////////////////////////
static size_t findNearestLowerGridAxisValue(const Matrix& gaxes, size_t axis, double axisval)
{
    int sign = gaxes(0, axis).real() > gaxes(gaxes.cols()-1, axis).real() ? -1 : 1;

    for (size_t i = 0; i < gaxes.rows(); i++)
    {
        if (sign * gaxes(i, axis).real() >= sign * axisval)
        {
            if (i)
                return i-1;

            return 0u;
        }
    }

    return gaxes.rows()-1;
}


/////////////////////////////////////////////////
/// \brief This static function converts floating
/// point coordinates to grid coordinates by
/// using the grid axes as reference values.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix coordsToGrid(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.rows() < 2 || !funcData.mat1.cols() || funcData.mat2.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() > 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat1.rows()) + "x2");

    if (funcData.mat2.cols() > 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat2) + " vs. "+ toString(funcData.mat2.rows()) + "x2");

    if (funcData.mat2.cols() == 2 && funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. "+ toString(funcData.mat2.rows()) + "x2");

    Matrix gcoords = funcData.mat2;

    #pragma omp parallel for
    for (size_t i = 0; i < gcoords.rows(); i++)
    {
        for (size_t j = 0; j < gcoords.cols(); j++)
        {
            size_t pos = findNearestLowerGridAxisValue(funcData.mat1, j, gcoords(i, j).real()); // find the lower grid axis value assuming sorted axis
            mu::value_type off = gcoords(i, j) - funcData.mat1(pos, j); // should be smaller than grid interval, but might be negative
            mu::value_type interval = pos+1 < funcData.mat1.rows()
                ? funcData.mat1(pos+1, j) - funcData.mat1(pos, j)
                : funcData.mat1(pos, j) - funcData.mat1(pos-1, j); // the grid interval. Might also be negative

            gcoords(i, j) = pos + 1 + off.real() / interval.real(); // if off == interval, then pos+1, else pos + (<1) and +1 due to zero-based coords
        }
    }

    return gcoords;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// bilinearInterpolation().
///
/// \param mat const Matrix&
/// \param row int
/// \param col int
/// \return mu::value_type
///
/////////////////////////////////////////////////
static mu::value_type readMat(const Matrix& mat, int row, int col)
{
    if (row < (int)mat.rows() && col < (int)mat.cols() && row >= 0 && col >= 0)
        return mat(row, col);
    else
        return NAN;
}


/////////////////////////////////////////////////
/// \brief Performs the bilinear interpolation of
/// the matrix value at the selected coordinates.
///
/// \param mat const Matrix&
/// \param row double
/// \param col double
/// \return double
///
/////////////////////////////////////////////////
static mu::value_type bilinearInterpolation(const Matrix& mat, double row, double col)
{
    if (std::isnan(row) || std::isnan(col))
        return NAN;

    // Find the base index
    int nBaseLine = intCast(row) + (row < 0 ? -1 : 0);
    int nBaseCol = intCast(col) + (col < 0 ? -1 : 0);

    // Get the decimal part of the double indices
    double x = row - nBaseLine;
    double y = col - nBaseCol;

    // Find the surrounding four entries
    mu::value_type f00 = readMat(mat, nBaseLine, nBaseCol);
    mu::value_type f10 = readMat(mat, nBaseLine+1, nBaseCol);
    mu::value_type f01 = readMat(mat, nBaseLine, nBaseCol+1);
    mu::value_type f11 = readMat(mat, nBaseLine+1, nBaseCol+1);

    // If all are NAN, return NAN
    if (isnan(f00) && isnan(f01) && isnan(f10) && isnan(f11))
        return NAN;

    // Otherwise set NAN to zero
    f00 = isnan(f00) ? 0.0 : f00;
    f10 = isnan(f10) ? 0.0 : f10;
    f01 = isnan(f01) ? 0.0 : f01;
    f11 = isnan(f11) ? 0.0 : f11;

    //     f(0,0) (1-x) (1-y) + f(1,0) x (1-y) + f(0,1) (1-x) y + f(1,1) x y
    return f00*(1-x)*(1-y)    + f10*x*(1-y)    + f01*(1-x)*y    + f11*x*y;
}


/////////////////////////////////////////////////
/// \brief This static function wraps the
/// bilinear interpolation algorithm for
/// interpolating the values of the first matrix
/// in the coordinates of the second matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix interpolate(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty() || funcData.mat2.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat2.cols() >= 2 && funcData.mat1.cols() <= 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. " + printMatrixDim(funcData.mat2));

    Matrix interp = createFilledMatrix(funcData.mat2.rows(), std::max(1u, funcData.mat2.cols()-1), 0.0);

    // Interpolate all values in the matrix mat2. First
    // column contains the row values, all remaining contain
    // the corresponding col values
    #pragma omp parallel for
    for (size_t i = 0; i < funcData.mat2.rows(); i++)
    {
        if (funcData.mat2.cols() >= 2)
        {
            for (size_t j = 1; j < funcData.mat2.cols(); j++)
            {
                interp(i, j-1) = bilinearInterpolation(funcData.mat1, funcData.mat2(i, 0).real()-1.0, funcData.mat2(i, j).real()-1.0);
            }
        }
        else
            interp(i, 0) = bilinearInterpolation(funcData.mat1, funcData.mat2(i, 0).real()-1.0, 0.0);
    }

    return interp;
}


/////////////////////////////////////////////////
/// \brief This static functions concatenates
/// two matrices horizontally. This is, the number
/// of rows stays the same and the columns are
/// appended to the ones of the other matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix hcat(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    // Check the dimensions of the two matrices
    if (funcData.mat1.rows() != funcData.mat2.rows())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. " + printMatrixDim(funcData.mat2));

    return funcData.mat1.hcat(funcData.mat2);
}


/////////////////////////////////////////////////
/// \brief This static functions concatenates
/// two matrices vertically. This is, the number
/// of columns stays the same and the rows are
/// appended to the ones of the other matrix.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix vcat(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    // Check the dimensions of the two matrices
    if (funcData.mat1.cols() != funcData.mat2.cols())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. " + printMatrixDim(funcData.mat2));

    return funcData.mat1.vcat(funcData.mat2);
}


/////////////////////////////////////////////////
/// \brief Extracts a selection from a matrix
/// iterating through two matrices simultaneously.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix selection(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty()
        || funcData.mat2.isEmpty()
        || funcData.mat3.isEmpty())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. " + printMatrixDim(funcData.mat2));

    // Store the scalar state
    bool isScalar[2] = {funcData.mat2.isScalar(),
                        funcData.mat3.isScalar()};

    if (!isScalar[0]
        && !isScalar[1]
        && (funcData.mat2.rows() != funcData.mat3.rows() || funcData.mat2.cols() != funcData.mat3.cols()))
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat2) + " vs. " + printMatrixDim(funcData.mat3));

    // Prepare the return value
    Matrix selected = createFilledMatrix(std::max(funcData.mat2.rows(), funcData.mat3.rows()),
                                         std::max(funcData.mat2.cols(), funcData.mat3.cols()), NAN);

    int row = 0;
    int col = 0;

    // Store the scalar values
    if (isScalar[0])
        row = intCast(funcData.mat2(0))-1;

    if (isScalar[1])
        col = intCast(funcData.mat3(0))-1;

    #pragma omp parallel for firstprivate(row,col)
    for (size_t i = 0; i < selected.rows(); i++)
    {
        for (size_t j = 0; j < selected.cols(); j++)
        {
            // Get the values, if they are no scalars
            if (!isScalar[0])
                row = intCast(funcData.mat2(i, j))-1;

            if (!isScalar[1])
                col = intCast(funcData.mat3(i, j))-1;

            // Extract the selected value
            if (row >= 0 && row < (int)funcData.mat1.rows()
                && col >= 0 && col < (int)funcData.mat1.cols())
                selected(i, j) = funcData.mat1(row, col);
        }
    }

    return selected;
}


/////////////////////////////////////////////////
/// \brief Assembles a matrix from coordinates
/// and values (a datagrid replacement without
/// interpolation).
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix assemble(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty()
        || funcData.mat2.isEmpty()
        || funcData.mat3.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Store the scalar state
    bool isScalar[2] = {funcData.mat1.isScalar(),
                        funcData.mat2.isScalar()};

    if (!isScalar[0]
        && !isScalar[1]
        && (funcData.mat1.rows() != funcData.mat2.rows()
            || funcData.mat1.cols() != funcData.mat2.cols()
            || funcData.mat1.rows() != funcData.mat3.rows()
            || funcData.mat1.cols() != funcData.mat3.cols()))
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1) + " vs. " + printMatrixDim(funcData.mat2) + " vs. " + printMatrixDim(funcData.mat3));

    // Prepare the return value
    int rows = calculateStats(funcData.mat1, StatsLogic(StatsLogic::OPERATION_MAX, funcData.mat1(0).real()),
                              0, funcData.mat1.rows(), 0, funcData.mat1.cols()).real();

    int cols = calculateStats(funcData.mat2, StatsLogic(StatsLogic::OPERATION_MAX, funcData.mat2(0).real()),
                              0, funcData.mat2.rows(), 0, funcData.mat2.cols()).real();

    // Prepare the filled matrix
    Matrix assembled = createFilledMatrix(rows, cols, NAN);

    int row = 0;
    int col = 0;

    // Store the scalar values
    if (isScalar[0])
        row = intCast(funcData.mat1(0))-1;

    if (isScalar[1])
        col = intCast(funcData.mat2(0))-1;

    #pragma omp parallel for firstprivate(row,col)
    for (size_t i = 0; i < funcData.mat3.rows(); i++)
    {
        for (size_t j = 0; j < funcData.mat3.cols(); j++)
        {
            // Get the values, if they are no scalars
            if (!isScalar[0])
                row = intCast(funcData.mat1(i, j))-1;

            if (!isScalar[1])
                col = intCast(funcData.mat2(i, j))-1;

            assembled(row, col) = funcData.mat3(i, j);
        }
    }

    return assembled;
}


/////////////////////////////////////////////////
/// \brief Calculates the length of an open
/// polygon. If the circumference of a closed
/// polygon shall be calculated, then append the
/// first vertex at the end of the polygon.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix polyLength(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (funcData.mat1.isEmpty())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.cols() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position,
                          printMatrixDim(funcData.mat1));

    Matrix mRes = createZeroesMatrix(1, 1);

    for (size_t i = 0; i < funcData.mat1.rows()-1; i++)
    {
        double sum = 0;

        for (size_t j = 0; j < funcData.mat1.cols(); j++)
        {
            sum += ((funcData.mat1(i+1, j) - funcData.mat1(i, j)) * conj(funcData.mat1(i+1, j) - funcData.mat1(i, j))).real();
        }

        mRes(0) += sqrt(sum);
    }

    return mRes;
}


/////////////////////////////////////////////////
/// \brief Static invalid matrix function, which
/// will always throw an error.
///
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix invalidMatrixFunction(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    throw SyntaxError(SyntaxError::FUNCTION_ERROR, errorInfo.command, errorInfo.position);
}


/////////////////////////////////////////////////
/// \brief Returns a map containing all declared
/// and "standard" matrix functions.
///
/// \return std::map<std::string,MatFuncDef>
///
/////////////////////////////////////////////////
static std::map<std::string,MatFuncDef> getMatrixFunctions()
{
    std::map<std::string,MatFuncDef> mFunctions;

    mFunctions["cross"] = MatFuncDef(MATSIG_MAT, calcCrossProduct);
    mFunctions["det"] = MatFuncDef(MATSIG_MAT, getDeterminant);
    mFunctions["trace"] = MatFuncDef(MATSIG_MAT, calcTrace);
    mFunctions["eigenvals"] = MatFuncDef(MATSIG_MAT, calcEigenValues);
    mFunctions["eigenvects"] = MatFuncDef(MATSIG_MAT, calcEigenVects);
    mFunctions["diagonalize"] = MatFuncDef(MATSIG_MAT, diagonalize);
    mFunctions["invert"] = MatFuncDef(MATSIG_MAT, invertMatrix);
    mFunctions["transpose"] = MatFuncDef(MATSIG_MAT, transposeMatrix);
    mFunctions["logtoidx"] = MatFuncDef(MATSIG_MAT, logToIndex);
    mFunctions["idxtolog"] = MatFuncDef(MATSIG_MAT, indexToLog);
    mFunctions["size"] = MatFuncDef(MATSIG_MAT, matrixSize);
    mFunctions["and"] = MatFuncDef(MATSIG_MAT, matrixAnd, false);
    mFunctions["or"] = MatFuncDef(MATSIG_MAT, matrixOr, false);
    mFunctions["xor"] = MatFuncDef(MATSIG_MAT, matrixXor, false);
    mFunctions["sum"] = MatFuncDef(MATSIG_MAT, matrixSum, false);
    mFunctions["std"] = MatFuncDef(MATSIG_MAT, matrixStd, false);
    mFunctions["avg"] = MatFuncDef(MATSIG_MAT, matrixAvg, false);
    mFunctions["prd"] = MatFuncDef(MATSIG_MAT, matrixPrd, false);
    mFunctions["cnt"] = MatFuncDef(MATSIG_MAT, matrixCnt, false);
    mFunctions["num"] = MatFuncDef(MATSIG_MAT, matrixNum, false);
    mFunctions["norm"] = MatFuncDef(MATSIG_MAT, matrixNorm, false);
    mFunctions["min"] = MatFuncDef(MATSIG_MAT, matrixMin, false);
    mFunctions["max"] = MatFuncDef(MATSIG_MAT, matrixMax, false);
    mFunctions["minpos"] = MatFuncDef(MATSIG_MAT, matrixMinPos, false);
    mFunctions["maxpos"] = MatFuncDef(MATSIG_MAT, matrixMaxPos, false);
    mFunctions["med"] = MatFuncDef(MATSIG_MAT, matrixMed, false);
    mFunctions["pct"] = MatFuncDef(MATSIG_MAT_F, matrixPct, false);
    mFunctions["cmp"] = MatFuncDef(MATSIG_MAT_F_N, matrixCmp, false);
    mFunctions["movsum"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovSum);
    mFunctions["movstd"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovStd);
    mFunctions["movavg"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovAvg);
    mFunctions["movprd"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovPrd);
    mFunctions["movmed"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovMed);
    mFunctions["movmin"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovMin);
    mFunctions["movmax"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovMax);
    mFunctions["movnorm"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovNorm);
    mFunctions["movnum"] = MatFuncDef(MATSIG_MAT_N_MOPT, matrixMovNum);
    mFunctions["zero"] = MatFuncDef(MATSIG_N_MOPT, createZeroesMatrix);
    mFunctions["one"] = MatFuncDef(MATSIG_N_MOPT, createOnesMatrix);
    mFunctions["identity"] = MatFuncDef(MATSIG_N_MOPT, identityMatrix);
    mFunctions["shuffle"] = MatFuncDef(MATSIG_N_MOPT, createShuffledMatrix);
    mFunctions["correl"] = MatFuncDef(MATSIG_MAT_MAT, correlation);
    mFunctions["covar"] = MatFuncDef(MATSIG_MAT_MAT, covariance);
    mFunctions["normalize"] = MatFuncDef(MATSIG_MAT, normalize);
    mFunctions["reshape"] = MatFuncDef(MATSIG_MAT_F_N, matrixReshape);
    mFunctions["resize"] = MatFuncDef(MATSIG_MAT_F_N, matrixResize);
    mFunctions["repmat"] = MatFuncDef(MATSIG_MAT_F_N, matrixRepMat);
    mFunctions["unique"] = MatFuncDef(MATSIG_MAT_NOPT, matrixUnique);
    mFunctions["solve"] = MatFuncDef(MATSIG_MAT, solveLGS);
    mFunctions["diag"] = MatFuncDef(MATSIG_MAT, diagonalMatrix);
    mFunctions["carttocyl"] = MatFuncDef(MATSIG_MAT, cartToCyl);
    mFunctions["carttopol"] = MatFuncDef(MATSIG_MAT, cartToPolar);
    mFunctions["cyltocart"] = MatFuncDef(MATSIG_MAT, cylToCart);
    mFunctions["cyltopol"] = MatFuncDef(MATSIG_MAT, cylToPolar);
    mFunctions["poltocart"] = MatFuncDef(MATSIG_MAT, polarToCart);
    mFunctions["poltocyl"] = MatFuncDef(MATSIG_MAT, polarToCyl);
    mFunctions["coordstogrid"] = MatFuncDef(MATSIG_MAT_MAT, coordsToGrid);
    mFunctions["interpolate"] = MatFuncDef(MATSIG_MAT_MAT, interpolate);
    mFunctions["hcat"] = MatFuncDef(MATSIG_MAT_MAT, hcat);
    mFunctions["vcat"] = MatFuncDef(MATSIG_MAT_MAT, vcat);
    mFunctions["select"] = MatFuncDef(MATSIG_MAT_MAT_MAT, selection);
    mFunctions["assemble"] = MatFuncDef(MATSIG_MAT_MAT_MAT, assemble);
    mFunctions["polylength"] = MatFuncDef(MATSIG_MAT, polyLength);

    // For finding matrix functions
    mFunctions["matfl"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matflf"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matfc"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matfcf"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);

    return mFunctions;
}

#endif // MATFUNCS_HPP

