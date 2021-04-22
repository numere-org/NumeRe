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
#include <random>
#include <map>
#include "matdatastructures.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"
#include "functionimplementation.hpp"


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
static Matrix createFilledMatrix(size_t n, size_t m, double val)
{
    Matrix mat;
    std::vector<double> vLine(m, val);

    for (size_t i = 0; i < n; i++)
        mat.push_back(vLine);

    return mat;
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

    Matrix _mIdentity;
    std::vector<double> vLine;

    for (int i = 0; i < funcData.nVal; i++)
    {
        for (int j = 0; j < funcData.nVal; j++)
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.size() != funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()));

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (unsigned int i = 0; i < funcData.mat1.size(); i++)
    {
        _mReturn[0][0] += funcData.mat1[i][i];
    }

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
static double calcDeterminant(const Matrix& _mMatrix, vector<int> vRemovedLines)
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
                        dDet += nSign * _mMatrix[i][j] * calcDeterminant(_mMatrix, vRemovedLines);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = identityMatrix(MatFuncData(1), errorInfo);
    std::vector<int> vRemovedLines(funcData.mat1.size(), 0);

    if (funcData.mat1.size() != funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()));

    _mReturn[0][0] = calcDeterminant(funcData.mat1, vRemovedLines);
    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1.size(), 1, 0.0);
    vector<int> vRemovedLines(funcData.mat1.size(), 0);

    if (funcData.mat1.size() == 1)
        return _mResult;

    if (funcData.mat1.size()-1 != funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()));

    if (funcData.mat1.size() == 2)
    {
        _mResult[0][0] = funcData.mat1[0][1];
        _mResult[1][0] = -funcData.mat1[0][0];
        return _mResult;
    }

    if (funcData.mat1.size() == 3)
    {
        _mResult[0][0] = funcData.mat1[1][0]*funcData.mat1[2][1] - funcData.mat1[2][0]*funcData.mat1[1][1];
        _mResult[1][0] = funcData.mat1[2][0]*funcData.mat1[0][1] - funcData.mat1[0][0]*funcData.mat1[2][1];
        _mResult[2][0] = funcData.mat1[0][0]*funcData.mat1[1][1] - funcData.mat1[1][0]*funcData.mat1[0][1];
        return _mResult;
    }

    Matrix _mTemp = createFilledMatrix(funcData.mat1.size(), funcData.mat1[0].size()+1, 0.0);

    for (unsigned int i = 0; i < funcData.mat1.size(); i++)
    {
        for (unsigned int j = 0; j < funcData.mat1[0].size(); j++)
        {
            _mTemp[i][j+1] = funcData.mat1[i][j];
        }
    }

    _mTemp[0][0] = 1.0;
    _mResult[0][0] = calcDeterminant(_mTemp, vRemovedLines);

    for (unsigned int i = 1; i < funcData.mat1.size(); i++)
    {
        _mTemp[i-1][0] = 0.0;
        _mTemp[i][0] = 1.0;
        _mResult[i][0] = calcDeterminant(_mTemp, vRemovedLines);
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief This static function determines,
/// whether the passed matrix is symmetric or not.
///
/// \param _mMatrix const Matrix&
/// \param errorInfo const MatFuncErrorInfo&
/// \return bool
///
/////////////////////////////////////////////////
static bool isSymmMatrix(const Matrix& _mMatrix, const MatFuncErrorInfo& errorInfo)
{
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

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
/// \brief This static function tries to remove
/// the imaginary part of the returned
/// eigenvalues.
///
/// \param _mMatrix Matrix&
/// \return void
///
/////////////////////////////////////////////////
static void makeReal(Matrix& _mMatrix)
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
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (_mMatrix.size() != _mMatrix[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(_mMatrix.size()) +"x"+ toString(_mMatrix[0].size()));

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
    if (isSymmMatrix(_mMatrix, errorInfo))
    {
        // Prepare return values
        _mEigenVals = createFilledMatrix(_mMatrix.size(), 1, 0.0);
        _mEigenVects = createFilledMatrix(_mMatrix.size(), _mMatrix.size(), 0.0);

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
        _mEigenVals = createFilledMatrix(_mMatrix.size(), 2, 0.0);
        _mEigenVects = createFilledMatrix(_mMatrix.size(), 2*_mMatrix.size(), 0.0);

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

            makeReal(_mEigenVals);
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

            makeReal(_mEigenVects);
        }
        else if (nReturnType == DIAGONALIZE)
        {
            Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();

            for (unsigned int i = 0; i < _mEigenVects.size(); i++)
            {
                _mEigenVects[i][i] = real(vEigenVals(i, 0));
                _mEigenVects[i][i+_mEigenVects.size()] = imag(vEigenVals(i, 0));
            }

            makeReal(_mEigenVects);
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
        _mBase[i][0] = i+1;

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
        double dTemp = _mBase[i][0];
        _mBase[i][0] = _mBase[nIndex][0];
        _mBase[nIndex][0] = dTemp;
    }

    std::uniform_real_distribution<double> randDist(1, nBase-1);

    // Update the seed
    dSeed = randDist(randGen);

    // Return only the requested vector length
    return Matrix(_mBase.begin(), _mBase.begin()+nShuffle);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.size() != funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()));

    // Gauss-Elimination???
    Matrix _mInverse = identityMatrix(MatFuncData(funcData.mat1.size()), errorInfo);
    Matrix _mToInvert = funcData.mat1;

    double dDet = getDeterminant(funcData, errorInfo)[0][0];

    if (!dDet)
        throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, errorInfo.command, errorInfo.position);

    //Spezialfaelle mit analytischem Ausdruck
    if (_mToInvert.size() == 1)
    {
        // eigentlich nicht zwangslaeufig existent... aber skalar und so...
        _mInverse[0][0] /= _mToInvert[0][0];
        return _mInverse;
    }
    else if (_mToInvert.size() == 2)
    {
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
    else if (_mToInvert.size() == 3)
    {
        _mInverse[0][0] = (_mToInvert[1][1]*_mToInvert[2][2] - _mToInvert[2][1]*_mToInvert[1][2]) / dDet;
        _mInverse[1][0] = -(_mToInvert[1][0]*_mToInvert[2][2] - _mToInvert[1][2]*_mToInvert[2][0]) / dDet;
        _mInverse[2][0] = (_mToInvert[1][0]*_mToInvert[2][1] - _mToInvert[1][1]*_mToInvert[2][0]) / dDet;

        _mInverse[0][1] = -(_mToInvert[0][1]*_mToInvert[2][2] - _mToInvert[0][2]*_mToInvert[2][1]) / dDet;
        _mInverse[1][1] = (_mToInvert[0][0]*_mToInvert[2][2] - _mToInvert[0][2]*_mToInvert[2][0]) / dDet;
        _mInverse[2][1] = -(_mToInvert[0][0]*_mToInvert[2][1] - _mToInvert[0][1]*_mToInvert[2][0]) / dDet;

        _mInverse[0][2] = (_mToInvert[0][1]*_mToInvert[1][2] - _mToInvert[0][2]*_mToInvert[1][1]) / dDet;
        _mInverse[1][2] = -(_mToInvert[0][0]*_mToInvert[1][2] - _mToInvert[0][2]*_mToInvert[1][0]) / dDet;
        _mInverse[2][2] = (_mToInvert[0][0]*_mToInvert[1][1] - _mToInvert[0][1]*_mToInvert[1][0]) / dDet;

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
        }
    }

    // die Matrix _mToInvert() sollte nun Dreiecksgestalt besitzen. Jetzt den Gauss von unten her umkehren
    for (int j = (int)_mToInvert.size()-1; j >= 0; j--)
    {
        if (_mToInvert[j][j] == 0.0) // Hauptdiagonale ist ein Element == 0??
            throw SyntaxError(SyntaxError::MATRIX_IS_NOT_INVERTIBLE, errorInfo.command, errorInfo.position);

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
    Matrix _mTransposed;
    std::vector<double> vLine;

    for (unsigned int j = 0; j < funcData.mat1[0].size(); j++)
    {
        for (unsigned int i = 0; i < funcData.mat1.size(); i++)
            vLine.push_back(funcData.mat1[i][j]);

        _mTransposed.push_back(vLine);
        vLine.clear();
    }

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    vector<int> vLines;
    vector<int> vRows;

    if (funcData.mat1.size() == 1 || funcData.mat1[0].size() == 1)
    {
        for (size_t i = 0; i < funcData.mat1.size(); i++)
        {
            for (size_t j = 0; j < funcData.mat1[0].size(); j++)
            {
                if (funcData.mat1[i][j])
                    vLines.push_back(i + j + 1);
            }
        }

        if (!vLines.size())
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(vLines.size(), 1, 0.0);

        for (size_t i = 0; i < vLines.size(); i++)
            _mReturn[i][0] = vLines[i];

        return _mReturn;
    }
    else
    {
        for (size_t i = 0; i < funcData.mat1.size(); i++)
        {
            for (size_t j = 0; j < funcData.mat1[0].size(); j++)
            {
                if (funcData.mat1[i][j])
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
            _mReturn[i][0] = vLines[i];
            _mReturn[i][1] = vRows[i];
        }

        return _mReturn;
    }
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!i && !j)
                _mReturn[0][0] = funcData.mat1[i][j];
            else if (funcData.mat1[i][j] > _mReturn[0][0] || std::isnan(_mReturn[0][0]))
                _mReturn[0][0] = funcData.mat1[i][j];
        }
    }

    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1.size() == 1 || funcData.mat1[0].size() == 1)
    {
        Matrix _mMatrixMax = matrixMax(funcData, errorInfo);

        if (!_mMatrixMax[0][0] || _mMatrixMax[0][0] < 0)
            return createFilledMatrix(1, 1, 0.0);

        Matrix _mReturn = createFilledMatrix(_mMatrixMax[0][0], 1, 0.0);

        for (size_t i = 0; i < funcData.mat1.size(); i++)
        {
            for (size_t j = 0; j < funcData.mat1[0].size(); j++)
            {
                if (funcData.mat1[i][j] > 0)
                    _mReturn[funcData.mat1[i][j]-1][0] = 1.0;
            }
        }

        return _mReturn;
    }
    else
    {
        vector<int> vCol;
        vector<int> vRow;

        if (funcData.mat1.size() == 2 && funcData.mat1[0].size() != 2)
        {
            for (size_t i = 0; i < funcData.mat1[0].size(); i++)
            {
                vRow.push_back(funcData.mat1[0][i]);
                vCol.push_back(funcData.mat1[1][i]);
            }
        }
        else if (funcData.mat1.size() != 2 && funcData.mat1[0].size() == 2)
        {
            for (size_t i = 0; i < funcData.mat1.size(); i++)
            {
                vRow.push_back(funcData.mat1[i][0]);
                vCol.push_back(funcData.mat1[i][1]);
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
                _mReturn[vRow[i]-1][vCol[i]-1] = 1.0;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(2, 1, 0.0);

    if (funcData.mat1.size() == 1 && funcData.mat1[0].size() == 1 && isnan(funcData.mat1[0][0]))
        return _mReturn;

    _mReturn[0][0] = funcData.mat1.size();
    _mReturn[1][0] = funcData.mat1[0].size();

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[i].size(); j++)
        {
            if (!funcData.mat1[i][j])
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[i].size(); j++)
        {
            if (funcData.mat1[i][j])
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    bool isTrue = false;

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[i].size(); j++)
        {
            if (funcData.mat1[i][j])
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!std::isnan(funcData.mat1[i][j]))
                _mReturn[0][0] += funcData.mat1[i][j];
        }
    }

    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!std::isnan(funcData.mat1[i][j]))
                _mReturn[0][0] += 1;
        }
    }

    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mSum = matrixSum(funcData, errorInfo);
    Matrix _mNum = matrixNum(funcData, errorInfo);

    return createFilledMatrix(1, 1, _mSum[0][0] / _mNum[0][0]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);
    Matrix _mAvg = matrixAvg(funcData, errorInfo);
    Matrix _mNum = matrixNum(funcData, errorInfo);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!std::isnan(funcData.mat1[i][j]))
                _mReturn[0][0] += (funcData.mat1[i][j] - _mAvg[0][0])*(funcData.mat1[i][j] - _mAvg[0][0]);
        }
    }

    _mReturn[0][0] = sqrt(_mReturn[0][0])/(_mNum[0][0]-1);
    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 1.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!std::isnan(funcData.mat1[i][j]))
                _mReturn[0][0] *= funcData.mat1[i][j];
        }
    }

    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return createFilledMatrix(1, 1, funcData.mat1.size() * funcData.mat1[0].size());
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!std::isnan(funcData.mat1[i][j]))
                _mReturn[0][0] += funcData.mat1[i][j]*funcData.mat1[i][j];
        }
    }

    _mReturn[0][0] = sqrt(_mReturn[0][0]);
    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (!i && !j)
                _mReturn[0][0] = funcData.mat1[i][j];
            else if (funcData.mat1[i][j] < _mReturn[0][0] || std::isnan(_mReturn[0][0]))
                _mReturn[0][0] = funcData.mat1[i][j];
        }
    }

    return _mReturn;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(1, 1, 0.0);
    Matrix _mCoords = createFilledMatrix(2, 1, 0.0);
    _mCoords[0][0] = -1;

    double dValue = funcData.fVal;
    int nType = funcData.nVal;
    double dKeep = dValue;

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            if (std::isnan(funcData.mat1[i][j]) || std::isinf(funcData.mat1[i][j]))
                continue;

            if (funcData.mat1[i][j] == dValue)
            {
                if (!nType || abs(nType) <= 1)
                {
                    _mCoords[0][0] = i+1;
                    _mCoords[1][0] = j+1;
                    return _mCoords;
                }
                else
                    _mReturn[0][0] = funcData.mat1[i][j];

                return _mReturn;
            }
            else if (nType > 0 && funcData.mat1[i][j] > dValue)
            {
                if (_mCoords[0][0] == -1 || funcData.mat1[i][j] < dKeep)
                {
                    dKeep = funcData.mat1[i][j];
                    _mCoords[0][0] = i+1;
                    _mCoords[1][0] = j+1;
                }
                else
                    continue;
            }
            else if (nType < 0 && funcData.mat1[i][j] < dValue)
            {
                if (_mCoords[0][0] == -1 || funcData.mat1[i][j] > dKeep)
                {
                    dKeep = funcData.mat1[i][j];
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return matrixCmp(MatFuncData(funcData.mat1, matrixMin(funcData, errorInfo)[0][0], 0), errorInfo);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    return matrixCmp(MatFuncData(funcData.mat1, matrixMax(funcData, errorInfo)[0][0], 0), errorInfo);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Memory _mem;

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            _mem.writeData(j + i*funcData.mat1.size(), 0, funcData.mat1[i][j]);
        }
    }

    return createFilledMatrix(1, 1, _mem.med(VectorIndex(0, funcData.mat1.size()*funcData.mat1[0].size()-1), VectorIndex(0)));
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Memory _mem;

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            _mem.writeData(j + i*funcData.mat1.size(), 0, funcData.mat1[i][j]);
        }
    }

    return createFilledMatrix(1, 1, _mem.pct(VectorIndex(0, (long long int)(funcData.mat1.size()*funcData.mat1[0].size())-1), VectorIndex(0), funcData.fVal));
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
    size_t nLines = funcData.fVal;
    size_t nCols = funcData.nVal;

    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (nLines == funcData.mat1.size() && nCols == funcData.mat1[0].size())
        return funcData.mat1;

    Matrix _mReturn = createFilledMatrix(nLines, nCols, 0.0);

    for (size_t i = 0; i < min(nLines, funcData.mat1.size()); i++)
    {
        for (size_t j = 0; j < min(nCols, funcData.mat1[0].size()); j++)
        {
            _mReturn[i][j] = funcData.mat1[i][j];
        }
    }

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !funcData.mat2.size() || !funcData.mat2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Ensure that the size is non-zero
    if (!(funcData.mat1.size() && funcData.mat2.size()) || !(funcData.mat1[0].size() && funcData.mat2[0].size()))
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()) + ", " + toString(funcData.mat2.size()) +"x"+ toString(funcData.mat2[0].size()));

    // Resize the matrices to fit their counterparts
    Matrix mMatrix1 = matrixResize(MatFuncData(funcData.mat1, max(funcData.mat1.size(), funcData.mat2.size()), max(funcData.mat1[0].size(), funcData.mat2[0].size())), errorInfo);
    Matrix mMatrix2 = matrixResize(MatFuncData(funcData.mat2, max(funcData.mat1.size(), funcData.mat2.size()), max(funcData.mat1[0].size(), funcData.mat2[0].size())), errorInfo);

    int n = mMatrix1.size();
    int m = mMatrix1[0].size();

    // Create the target matrix
    Matrix mCorrelation = createFilledMatrix(2*n-1, 2*m-1, 0.0);

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
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix covariance(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !funcData.mat2.size() || !funcData.mat2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Ensure that their size is equal
    if (funcData.mat1.size() != funcData.mat2.size() || funcData.mat1[0].size() != funcData.mat2[0].size() || !funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) +"x"+ toString(funcData.mat1[0].size()) + " != " + toString(funcData.mat2.size()) +"x"+ toString(funcData.mat2[0].size()));

    // Prepare the target matrix
    Matrix mCovariance = createFilledMatrix(1, 1, 0.0);

    // Calculate the average values of both
    // matrices
    Matrix mAvg1 = matrixAvg(MatFuncData(funcData.mat1), errorInfo);
    Matrix mAvg2 = matrixAvg(MatFuncData(funcData.mat2), errorInfo);

    // Calculate the covariance value for each
    // component and sum it up
    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat2.size(); j++)
        {
            mCovariance[0][0] += (funcData.mat1[i][j] - mAvg1[0][0]) * (funcData.mat2[i][j] - mAvg2[0][0]);
        }
    }

    // Normalize the covariance value using
    // the number of elements
    mCovariance[0][0] /= (funcData.mat1.size() * funcData.mat1[0].size() - 1);

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = funcData.mat1;
    Matrix _mMax = matrixMax(funcData, errorInfo);
    Matrix _mMin = matrixMin(funcData, errorInfo);

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
/// \param funcData const MatFuncData&
/// \param errorInfo const MatFuncErrorInfo&
/// \return Matrix
///
/// The total number of elements before and
/// afterwards must be identical.
/////////////////////////////////////////////////
static Matrix matrixReshape(const MatFuncData& funcData, const MatFuncErrorInfo& errorInfo)
{
    size_t nLines = funcData.fVal;
    size_t nCols = funcData.nVal;

    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !nLines || !nCols)
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (nLines * nCols != funcData.mat1.size() * funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(nLines) + "x" + toString(nCols) + "=" + toString(nLines*nCols) +" vs. "+ toString(funcData.mat1.size()) + "x" +  toString(funcData.mat1[0].size()) + "=" + toString(funcData.mat1.size()*funcData.mat1[0].size()));

    Matrix _mReturn = createFilledMatrix(nLines, nCols, 0.0);

    for (size_t i = 0; i < nLines*nCols; i++)
    {
        _mReturn[i / nCols][i % nCols] = funcData.mat1[i / funcData.mat1[0].size()][i % funcData.mat1[0].size()];
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
    size_t n = funcData.fVal;
    size_t m = funcData.nVal;

    if (n == 0)
        n++;

    if (m == 0)
        m++;

    if ((n * funcData.mat1.size()) / funcData.mat1.size() != n || (m * funcData.mat1[0].size()) / funcData.mat1[0].size() != m)
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, errorInfo.command, errorInfo.position);

    Matrix _mReturn = createFilledMatrix(n * funcData.mat1.size(), m * funcData.mat1[0].size(), 0.0);

    for (size_t i = 0; i < funcData.mat1.size(); i++)
    {
        for (size_t j = 0; j < funcData.mat1[0].size(); j++)
        {
            for (size_t _n = 0; _n < n; _n++)
            {
                for (size_t _m = 0; _m < m; _m++)
                {
                    _mReturn[i+_n*funcData.mat1.size()][j+_m*funcData.mat1[0].size()] = funcData.mat1[i][j];
                }
            }
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
static std::vector<double> getUniqueList(std::list<double>& _list)
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
static void fillMissingMatrixElements(Matrix& _mMatrix)
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    // Create a std::list and the return value
    std::list<double> dataList;
    Matrix _mReturn;

    // Depending on the dimensions of the passed matrix, change
    // the evaluation method
    if (funcData.mat1.size() == 1)
    {
        // Row vector
        dataList.assign(funcData.mat1[0].begin(), funcData.mat1[0].end());
        _mReturn.push_back(getUniqueList(dataList));
    }
    else if (funcData.mat1[0].size() == 1)
    {
        // Column vector
        for (size_t i = 0; i < funcData.mat1.size(); i++)
        {
            dataList.push_back(funcData.mat1[i][0]);
        }

        _mReturn.push_back(getUniqueList(dataList));
        _mReturn = transposeMatrix(MatFuncData(_mReturn), errorInfo);
    }
    else
    {
        // Matrix
        if (!funcData.nVal)
        {
            // funcData.nVal == 0 -> Roll out the total matrix and return it as a overall row vector
            Matrix retVal = matrixReshape(MatFuncData(funcData.mat1, 1, funcData.mat1.size()*funcData.mat1[0].size()), errorInfo);
            dataList.assign(retVal[0].begin(), retVal[0].end());
            _mReturn.push_back(getUniqueList(dataList));
        }
        else if (funcData.nVal == 1)
        {
            // Make the rows unique
            for (size_t i = 0; i < funcData.mat1.size(); i++)
            {
                dataList.clear();
                dataList.assign(funcData.mat1[i].begin(), funcData.mat1[i].end());
                _mReturn.push_back(getUniqueList(dataList));
            }

            fillMissingMatrixElements(_mReturn);
        }
        else
        {
            // Make the columns unique
            for (size_t j = 0; j < funcData.mat1[0].size(); j++)
            {
                dataList.clear();

                for (size_t i = 0; i < funcData.mat1.size(); i++)
                    dataList.push_back(funcData.mat1[i][j]);

                _mReturn.push_back(getUniqueList(dataList));
            }

            fillMissingMatrixElements(_mReturn);
            _mReturn = transposeMatrix(MatFuncData(_mReturn), errorInfo);
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
    if (!_mMatrix.size() || !_mMatrix[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    string sSolution = "sle(";
    vector<string> vResult(_mMatrix[0].size()-1, "");
    bool bIsZeroesLine = true;
    unsigned int nVarCount = 0;

    Matrix _mToSolve = createFilledMatrix(_mMatrix[0].size()-1, _mMatrix[0].size(), 0.0);
    Matrix _mCoefficents = createFilledMatrix(_mMatrix[0].size()-1, _mMatrix[0].size(), 0.0);

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
                    vResult[i] += toString(_mCoefficents[i][j], 5) + "*";

                if (_mCoefficents[i][j] == -1.0)
                    vResult[i] += "-";

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _mResult = createFilledMatrix(funcData.mat1[0].size()-1, 1, 0.0);
    Matrix _mToSolve = funcData.mat1;

    if (_mToSolve.size() == 1)
    {
        _mResult[0][0] = _mToSolve[0][1]/_mToSolve[0][0];
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
        solveLGSSymbolic(_mToSolve, errorInfo);
        return _mToSolve;
    }
    else if (_mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-2] == 0.0 && _mToSolve[_mToSolve.size()-1][_mToSolve[0].size()-1] != 0.0)
        throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, errorInfo.command, errorInfo.position);
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
            solveLGSSymbolic(_mToSolve, errorInfo);
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
            solveLGSSymbolic(_mToSolve, errorInfo);
            return _mToSolve;
        }

        if (_mToSolve[i][i] == 0.0 && _mToSolve[i][_mToSolve[0].size()-1] != 0.0)
            throw SyntaxError(SyntaxError::LGS_HAS_NO_SOLUTION, errorInfo.command, errorInfo.position);

        _mResult[i][0] = _mToSolve[i][_mToSolve[0].size()-1];
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    Matrix _reshapedMat = matrixReshape(MatFuncData(funcData.mat1, funcData.mat1.size() * funcData.mat1[0].size(), 1), errorInfo);
    Matrix _diagonalMat = createFilledMatrix(_reshapedMat.size(), _reshapedMat.size(), 0.0);

    for (size_t i = 0; i < _reshapedMat.size(); i++)
    {
        _diagonalMat[i][i] = _reshapedMat[i][0];
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = parser_Norm(&funcData.mat1[i][0], 2);
        _mReturn[i][1] = parser_phi(funcData.mat1[i][0], funcData.mat1[i][1]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    if (funcData.mat1[0].size() == 2)
        return cartToCyl(funcData, errorInfo);

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = parser_Norm(&funcData.mat1[i][0], 3);
        _mReturn[i][1] = parser_phi(funcData.mat1[i][0], funcData.mat1[i][1]);
        _mReturn[i][2] = parser_theta(funcData.mat1[i][0], funcData.mat1[i][1], funcData.mat1[i][2]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = funcData.mat1[i][0] * cos(funcData.mat1[i][1]);
        _mReturn[i][1] = funcData.mat1[i][0] * sin(funcData.mat1[i][1]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    if (funcData.mat1[0].size() == 2)
        return funcData.mat1;

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = sqrt(funcData.mat1[i][0] * funcData.mat1[i][0] + funcData.mat1[i][2] * funcData.mat1[i][2]);
        _mReturn[i][2] = parser_theta(funcData.mat1[i][0] * cos(funcData.mat1[i][1]), funcData.mat1[i][0] * sin(funcData.mat1[i][1]), funcData.mat1[i][2]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    if (funcData.mat1[0].size() == 2)
        return cylToCart(funcData, errorInfo);

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = funcData.mat1[i][0] * cos(funcData.mat1[i][1]) * sin(funcData.mat1[i][2]);
        _mReturn[i][1] = funcData.mat1[i][0] * sin(funcData.mat1[i][1]) * sin(funcData.mat1[i][2]);
        _mReturn[i][2] = funcData.mat1[i][0] * cos(funcData.mat1[i][2]);
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 3 || funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x3");

    Matrix _mReturn = funcData.mat1;

    for (size_t i = 0; i < _mReturn.size(); i++)
    {
        _mReturn[i][0] = funcData.mat1[i][0] * sin(funcData.mat1[i][2]);

        if (funcData.mat1[0].size() == 3)
            _mReturn[i][2] = funcData.mat1[i][0] * cos(funcData.mat1[i][2]);
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
    int sign = gaxes.front()[axis] > gaxes.back()[axis] ? -1 : 1;

    for (size_t i = 0; i < gaxes.size(); i++)
    {
        if (sign * gaxes[i][axis] >= sign * axisval)
        {
            if (i)
                return i-1;

            return 0u;
        }
    }

    return gaxes.size()-1;
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
    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !funcData.mat2.size() || !funcData.mat2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat1[0].size() > 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat1.size()) + "x2");

    if (funcData.mat2[0].size() > 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat2.size()) + "x" + toString(funcData.mat2[0].size()) + " vs. "+ toString(funcData.mat2.size()) + "x2");

    if (funcData.mat2[0].size() == 2 && funcData.mat1[0].size() < 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat2.size()) + "x2");

    Matrix gcoords = funcData.mat2;

    for (size_t i = 0; i < gcoords.size(); i++)
    {
        for (size_t j = 0; j < gcoords[i].size(); j++)
        {
            size_t pos = findNearestLowerGridAxisValue(funcData.mat1, j, gcoords[i][j]); // find the lower grid axis value assuming sorted axis
            double off = gcoords[i][j] - funcData.mat1[pos][j]; // should be smaller than grid interval, but might be negative
            double interval = pos+1 < funcData.mat1.size() ? funcData.mat1[pos+1][j] - funcData.mat1[pos][j] : funcData.mat1[pos][j] - funcData.mat1[pos-1][j]; // the grid interval. Might also be negative

            gcoords[i][j] = pos + 1 + off / interval; // if off == interval, then pos+1, else pos + (<1) and +1 due to zero-based coords
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
/// \return double
///
/////////////////////////////////////////////////
static double readMat(const Matrix& mat, int row, int col)
{
    if (row < (int)mat.size() && col < (int)mat[0].size() && row >= 0 && col >= 0)
        return mat[row][col];
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
static double bilinearInterpolation(const Matrix& mat, double row, double col)
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
    double f00 = readMat(mat, nBaseLine, nBaseCol);
    double f10 = readMat(mat, nBaseLine+1, nBaseCol);
    double f01 = readMat(mat, nBaseLine, nBaseCol+1);
    double f11 = readMat(mat, nBaseLine+1, nBaseCol+1);

    // If all are NAN, return NAN
    if (std::isnan(f00) && std::isnan(f01) && std::isnan(f10) && std::isnan(f11))
        return NAN;

    // Otherwise set NAN to zero
    f00 = std::isnan(f00) ? 0.0 : f00;
    f10 = std::isnan(f10) ? 0.0 : f10;
    f01 = std::isnan(f01) ? 0.0 : f01;
    f11 = std::isnan(f11) ? 0.0 : f11;

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
    if (!funcData.mat1.size() || !funcData.mat1[0].size() || !funcData.mat2.size() || !funcData.mat2[0].size())
        throw SyntaxError(SyntaxError::MATRIX_CANNOT_HAVE_ZERO_SIZE, errorInfo.command, errorInfo.position);

    if (funcData.mat2[0].size() >= 2 && funcData.mat1[0].size() <= 2)
        throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, errorInfo.command, errorInfo.position, toString(funcData.mat1.size()) + "x" + toString(funcData.mat1[0].size()) + " vs. "+ toString(funcData.mat2.size()) + "x"+ toString(funcData.mat2[0].size()));

    Matrix interp = createFilledMatrix(funcData.mat2.size(), std::max(1u, funcData.mat2[0].size()-1), 0.0);

    // Interpolate all values in the matrix mat2. First
    // column contains the row values, all remaining contain
    // the corresponding col values
    for (size_t i = 0; i < funcData.mat2.size(); i++)
    {
        if (funcData.mat2[i].size() >= 2)
        {
            for (size_t j = 1; j < funcData.mat2[i].size(); j++)
            {
                interp[i][j-1] = bilinearInterpolation(funcData.mat1, funcData.mat2[i][0]-1.0, funcData.mat2[i][j]-1.0);
            }
        }
        else
            interp[i][0] = bilinearInterpolation(funcData.mat1, funcData.mat2[i][0]-1.0, 0.0);
    }

    return interp;
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

    // For finding matrix functions
    mFunctions["matfl"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matflf"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matfc"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);
    mFunctions["matfcf"] = MatFuncDef(MATSIG_INVALID, invalidMatrixFunction);

    return mFunctions;
}

#endif // MATFUNCS_HPP

