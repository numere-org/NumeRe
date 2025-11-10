/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#include "matfuncimplementation.hpp"
#include "functionimplementation.hpp"
#include "statslogic.hpp"
#include "matdatastructures.hpp"
#include "../ParserLib/muParserError.h"
#include "../structures.hpp"
#include "../datamanagement/memory.hpp"

#define EIGENVALUES 0
#define EIGENVECTORS 1
#define DIAGONALIZE 2

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <numeric>
#include <mgl2/mgl.h>
#include <queue>

enum
{
    MATRIX_MUST_BE_SQUARE = 0x1,
    MATRIX_MUST_BE_2D = 0x2,
    MATRIX_MUST_NOT_CONTAIN_INVALID_VALUES = 0x4
};

/////////////////////////////////////////////////
/// \brief Internal helper function for combined
/// input checks.
///
/// \param mat const mu::Array&
/// \param conditionList int
/// \return void
///
/////////////////////////////////////////////////
static void checkInputMatrix(const mu::Array& mat, int conditionList)
{
    if (!mat.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::DimSizes dimSizes = mat.getDimSizes();

    if (conditionList & MATRIX_MUST_BE_SQUARE)
    {
        if (dimSizes.size() < 2)
            throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, mat.printDims());

        for (size_t i = 1; i < dimSizes.size(); i++)
        {
            if (dimSizes[i] != dimSizes[0])
                throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, mat.printDims());
        }
    }

    if ((conditionList & MATRIX_MUST_BE_2D) && dimSizes.size() != 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, mat.printDims());

    if (conditionList & MATRIX_MUST_NOT_CONTAIN_INVALID_VALUES)
    {
        if (mat.call("num").getAsScalarInt() != (int64_t)mat.size())
            throw mu::ParserError(mu::ecMATRIX_INVALID_VALS);
    }
}


/////////////////////////////////////////////////
/// \brief Implementation of the matrix
/// multiplication operator.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_MatMul(const mu::Array& A, const mu::Array& B)
{
    mu::DimSizes dimA = A.getDimSizes();
    mu::DimSizes dimB = B.getDimSizes();

    if (dimB.size() < 2)
        dimB.push_back(1ull);

    if (dimA.size() < 2 || dimB[0] != dimA[1])
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, A.printDims() + " vs. " + B.printDims());

    mu::Array ret(dimA[0]*dimB[1], mu::Value(0.0));
    ret.setDimSizes({dimA[0], dimB[1]});

    #pragma omp parallel for if (A.isParallelizable() && B.isParallelizable())
    for (size_t i = 0; i < dimA[0]; i++)
    {
        for (size_t j = 0; j < dimB[1]; j++)
        {
            for (size_t k = 0; k < dimA[1]; k++)
            {
                // Using the private method avoids addressing
                // issues occuring with transposed matrices
                ret.get(i, j) += A.get(i, k) * B.get(k, j);
            }
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Row-column transpose operator (does
/// not affect other dimensions).
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_transpose(const mu::Array& A)
{
    return matfnc_transpose(A, mu::Array());
}


/////////////////////////////////////////////////
/// \brief Create a matrix from a list of columns.
///
/// \param cols const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_matfc(const mu::MultiArgFuncParams& cols)
{
    size_t rowCount = 1ull;
    size_t colCount = cols.count();

    for (size_t j = 0; j < colCount; j++)
    {
        rowCount = std::max(rowCount, cols[j].size());
    }

    mu::Array mat(rowCount*colCount, mu::Value(0.0));
    mat.setDimSizes({rowCount, colCount});

    for (size_t j = 0; j < colCount; j++)
    {
        for (size_t i = 0; i < cols[j].size(); i++)
        {
            mat.get(i, j) = cols[j].get(i);
        }
    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief Create a matrix from a list of rows.
///
/// \param rows const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_matfr(const mu::MultiArgFuncParams& rows)
{
    size_t rowCount = rows.count();
    size_t colCount = 1ull;

    for (size_t i = 0; i < rowCount; i++)
    {
        colCount = std::max(colCount, rows[i].size());
    }

    mu::Array mat(rowCount*colCount, mu::Value(0.0));
    mat.setDimSizes({rowCount, colCount});

    for (size_t i = 0; i < rowCount; i++)
    {
        for (size_t j = 0; j < rows[i].size(); j++)
        {
            mat.get(i, j) = rows[i].get(j);
        }
    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief This static function is used to
/// calculate the differences between consecutive
/// matrix elements.
///
/// \param vect const mu::Array&
/// \return mu::Array
///
/// This function is used by the \c matf*f()
/// functions to derive the filling logic.
/////////////////////////////////////////////////
static mu::Array calcDeltasForMatFill(const mu::Array& vect)
{
    mu::Array deltas;

    if (vect.size() == 1ull)
        deltas.push_back(0.0);
    else
    {
        for (size_t j = 1; j < vect.size(); j++)
        {
            deltas.push_back(vect.get(j) - vect.get(j-1));
        }
    }

    return deltas;
}


/////////////////////////////////////////////////
/// \brief Create a matrix from a list of columns
/// and fill the missing elements using pattern
/// logic.
///
/// \param cols const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_matfcf(const mu::MultiArgFuncParams& cols)
{
    size_t rowCount = 1ull;
    size_t colCount = cols.count();

    std::vector<mu::Array> deltaVect;
    deltaVect.reserve(colCount);

    for (size_t j = 0; j < colCount; j++)
    {
        rowCount = std::max(rowCount, cols[j].size());
        deltaVect.push_back(calcDeltasForMatFill(cols[j]));
    }

    mu::Array mat(rowCount*colCount, mu::Value(0.0));
    mat.setDimSizes({rowCount, colCount});

    for (size_t j = 0; j < colCount; j++)
    {
        for (size_t i = 0; i < cols[j].size(); i++)
        {
            mat.get(i, j) = cols[j].get(i);
        }

        for (size_t i = cols[j].size(); i < rowCount; i++)
        {
            mat.get(i, j) = mat.get(i-1, j) + deltaVect[j].get(i % deltaVect[j].size());
        }
    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief Create a matrix from a list of rows
/// and fill the missing elements using pattern
/// logic.
///
/// \param rows const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_matfrf(const mu::MultiArgFuncParams& rows)
{
    size_t rowCount = rows.count();
    size_t colCount = 1ull;

    std::vector<mu::Array> deltaVect;
    deltaVect.reserve(rowCount);

    for (size_t i = 0; i < rowCount; i++)
    {
        colCount = std::max(colCount, rows[i].size());
        deltaVect.push_back(calcDeltasForMatFill(rows[i]));
    }

    mu::Array mat(rowCount*colCount, mu::Value(0.0));
    mat.setDimSizes({rowCount, colCount});

    for (size_t i = 0; i < rowCount; i++)
    {
        for (size_t j = 0; j < rows[i].size(); j++)
        {
            mat.get(i, j) = rows[i].get(j);
        }

        for (size_t j = rows[i].size(); j < colCount; j++)
        {
            mat.get(i, j) = mat.get(i, j-1) + deltaVect[i].get(j % deltaVect[i].size());
        }
    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// determinant of the passed matrix using the
/// LaPlace algorithm.
///
/// \param _mMatrix const mu::Array&
/// \param vRemovedRows std::vector<int>
/// \return mu::Value
///
/////////////////////////////////////////////////
static mu::Value calcDeterminant(const mu::Array& _mMatrix, std::vector<int> vRemovedRows)
{
    // simple Sonderfaelle
    if (_mMatrix.rows() == 1)
        return _mMatrix.get(0);

    if (_mMatrix.rows() == 2)
        return _mMatrix.get(0, 0) * _mMatrix.get(1, 1) - _mMatrix.get(1, 0)*_mMatrix.get(0, 1);

    if (_mMatrix.rows() == 3)
    {
        return _mMatrix.get(0, 0)*_mMatrix.get(1, 1)*_mMatrix.get(2, 2)
            + _mMatrix.get(0, 1)*_mMatrix.get(1, 2)*_mMatrix.get(2, 0)
            + _mMatrix.get(0, 2)*_mMatrix.get(1, 0)*_mMatrix.get(2, 1)
            - _mMatrix.get(0, 2)*_mMatrix.get(1, 1)*_mMatrix.get(2, 0)
            - _mMatrix.get(0, 1)*_mMatrix.get(1, 0)*_mMatrix.get(2, 2)
            - _mMatrix.get(0, 0)*_mMatrix.get(1, 2)*_mMatrix.get(2, 1);
    }

    int nSign = 1;
    mu::Value dDet(0.0);

    for (size_t i = 0; i < _mMatrix.rows(); i++)
    {
        // Noch nicht entfernte Zeile?
        if (!(vRemovedRows[i] & 1))
        {
            // entferne Zeile i
            vRemovedRows[i] += 1;

            for (size_t j = 0; j < _mMatrix.rows(); j++)
            {
                // Noch nicht entfernte Spalte?
                if (!(vRemovedRows[j] & 2))
                {
                    if (_mMatrix.get(i, j) == mu::Value(0.0))
                    {
                        nSign *= -1;
                        continue;
                    }
                    // entferne Spalte j
                    vRemovedRows[j] += 2;

                    // Berechne Determinante rekursiv
                    if (i+1 < _mMatrix.rows())
                        dDet += mu::Value(nSign) * _mMatrix.get(i, j) * calcDeterminant(_mMatrix, vRemovedRows);
                    else
                        dDet += mu::Value(nSign) * _mMatrix.get(i, j);

                    // füge Spalte j wieder hinzu
                    vRemovedRows[j] -= 2;
                    // alternierendes Vorzeichen
                    nSign *= -1;
                }
            }

            vRemovedRows[i] -= 1;
            return dDet;
        }
    }

    return mu::Value(1.0);
}


/////////////////////////////////////////////////
/// \brief Impelementation of the det() function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_det(const mu::Array& A)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D | MATRIX_MUST_BE_SQUARE);

    std::vector<int> vRemovedRows(A.rows(), 0);
    return calcDeterminant(A, vRemovedRows);
}


/////////////////////////////////////////////////
/// \brief Implementation of the cross() function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cross(const mu::Array& A)
{
    if (!A.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::Array ret(A.rows(), mu::Value(0.0));
    ret.setDimSizes({A.rows(), 1ull});
    std::vector<int> vRemovedRows(A.rows(), 0);

    if (A.rows() == 1)
        return ret;

    if (A.rows()-1 != A.cols())
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, A.printDims());

    if (A.rows() == 2)
    {
        ret.get(0) = A.get(0, 1);
        ret.get(1) = -A.get(0, 0);
        return ret;
    }

    if (A.rows() == 3)
    {
        ret.get(0) = A.get(1, 0)*A.get(2, 1) - A.get(2, 0)*A.get(1, 1);
        ret.get(1) = A.get(2, 0)*A.get(0, 1) - A.get(0, 0)*A.get(2, 1);
        ret.get(2) = A.get(0, 0)*A.get(1, 1) - A.get(1, 0)*A.get(0, 1);
        return ret;
    }

    mu::Array _mTemp(A.rows()*(A.cols()+1), mu::Value(0.0));
    _mTemp.setDimSizes({A.rows(), A.cols()+1});

    #pragma omp parallel for if (A.isParallelizable())
    for (size_t i = 0; i < A.rows(); i++)
    {
        for (size_t j = 0; j < A.cols(); j++)
        {
            _mTemp.get(i, j+1) = A.get(i, j);
        }
    }

    _mTemp.get(0) = 1.0;
    ret.get(0) = calcDeterminant(_mTemp, vRemovedRows);

    for (size_t i = 1; i < A.rows(); i++)
    {
        _mTemp.get(i-1, 0) = 0.0;
        _mTemp.get(i, 0) = 1.0;
        ret.get(i) = calcDeterminant(_mTemp, vRemovedRows);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the trace() function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_trace(const mu::Array& A)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D | MATRIX_MUST_BE_SQUARE);

    mu::Value trace(0.0);

    for (size_t i = 0; i < A.rows(); i++)
    {
        trace += A.get(i, i);
    }

    return trace;
}


/////////////////////////////////////////////////
/// \brief This static function does the whole
/// eigenvalues, eigenvectors and diagonalizing
/// stuff.
///
/// \param A const mu::Array&
/// \param nReturnType int
/// \return mu::Array
///
/// \c __attribute__((force_align_arg_pointer))
/// fixes TDM-GCC Bug for wrong stack alignment.
/////////////////////////////////////////////////
__attribute__((force_align_arg_pointer)) static mu::Array calcEigenVectsAndValues(const mu::Array& A, int nReturnType)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D | MATRIX_MUST_BE_SQUARE | MATRIX_MUST_NOT_CONTAIN_INVALID_VALUES);

    mu::Array _mEigenVals;
    mu::Array _mEigenVects;

    Eigen::MatrixXcd mMatrix(A.rows(), A.rows());

    // Copy the passed matrix into an Eigen matrix
    #pragma omp parallel for if (A.isParallelizable())
    for (size_t i = 0; i < A.rows(); i++)
    {
        for (size_t j = 0; j < A.rows(); j++)
        {
            mMatrix(i,j) = A.get(i,j).as_cmplx();
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
        _mEigenVals.resize(A.rows()); // No "real" vector

        for (size_t i = 0; i < _mEigenVals.rows(); i++)
        {
            _mEigenVals.get(i) = vEigenVals(i, 0);
        }
    }
    else if (nReturnType == EIGENVECTORS)
    {
        Eigen::EigenSolver<Eigen::MatrixXcd>::EigenvectorsType mEigenVects = eSolver.eigenvectors();
        _mEigenVects.resize(A.rows()*A.cols());
        _mEigenVects.setDimSizes({A.rows(), A.cols()});

        #pragma omp parallel for
        for (size_t i = 0; i < _mEigenVects.rows(); i++)
        {
            for (size_t j = 0; j < _mEigenVects.cols(); j++)
            {
                _mEigenVects.get(i, j) = mEigenVects(i, j);
            }
        }
    }
    else if (nReturnType == DIAGONALIZE)
    {
        Eigen::VectorXcd vEigenVals = eSolver.eigenvalues();
        _mEigenVects.resize(A.rows()*A.cols(), mu::Value(0.0));
        _mEigenVects.setDimSizes({A.rows(), A.cols()});

        for (size_t i = 0; i < _mEigenVects.rows(); i++)
        {
            _mEigenVects.get(i, i) = vEigenVals(i, 0);
        }
    }

    // Return the corresponding result
    if (nReturnType == EIGENVALUES)
        return _mEigenVals;
    else
        return _mEigenVects;
}


/////////////////////////////////////////////////
/// \brief Implementation of the eigenvals()
/// function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_eigenvals(const mu::Array& A)
{
    return calcEigenVectsAndValues(A, EIGENVALUES);
}


/////////////////////////////////////////////////
/// \brief Implementation of the eigenvects()
/// function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_eigenvects(const mu::Array& A)
{
    return calcEigenVectsAndValues(A, EIGENVECTORS);
}


/////////////////////////////////////////////////
/// \brief Implementation if the diagonalize()
/// function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_diagonalize(const mu::Array& A)
{
    return calcEigenVectsAndValues(A, DIAGONALIZE);
}


/////////////////////////////////////////////////
/// \brief Implementation of the invert()
/// function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_invert(const mu::Array& A)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D | MATRIX_MUST_BE_SQUARE | MATRIX_MUST_NOT_CONTAIN_INVALID_VALUES);

    // Gauss-Elimination???
    mu::Array _mInverse = matfnc_identity(mu::Value(A.rows()));
    mu::Value dDet = matfnc_det(A).get(0);

    if (!dDet)
        throw mu::ParserError(mu::ecMATRIX_NOT_INVERTIBLE);

    //Spezialfaelle mit analytischem Ausdruck
    if (A.rows() == 1)
    {
        // eigentlich nicht zwangslaeufig existent... aber skalar und so...
        _mInverse.get(0) /= A.get(0);
        return _mInverse;
    }
    else if (A.rows() == 2)
    {
        _mInverse.get(0, 0) = A.get(1, 1) / dDet;
        _mInverse.get(1, 1) = A.get(0, 0) / dDet;
        _mInverse.get(0, 1) = -A.get(0, 1) / dDet;
        _mInverse.get(1, 0) = -A.get(1, 0) / dDet;

        return _mInverse;
    }
    else if (A.rows() == 3)
    {
        _mInverse.get(0, 0) =  (A.get(1, 1)*A.get(2, 2) - A.get(2, 1)*A.get(1, 2)) / dDet;
        _mInverse.get(1, 0) = -(A.get(1, 0)*A.get(2, 2) - A.get(1, 2)*A.get(2, 0)) / dDet;
        _mInverse.get(2, 0) =  (A.get(1, 0)*A.get(2, 1) - A.get(1, 1)*A.get(2, 0)) / dDet;

        _mInverse.get(0, 1) = -(A.get(0, 1)*A.get(2, 2) - A.get(0, 2)*A.get(2, 1)) / dDet;
        _mInverse.get(1, 1) =  (A.get(0, 0)*A.get(2, 2) - A.get(0, 2)*A.get(2, 0)) / dDet;
        _mInverse.get(2, 1) = -(A.get(0, 0)*A.get(2, 1) - A.get(0, 1)*A.get(2, 0)) / dDet;

        _mInverse.get(0, 2) =  (A.get(0, 1)*A.get(1, 2) - A.get(0, 2)*A.get(1, 1)) / dDet;
        _mInverse.get(1, 2) = -(A.get(0, 0)*A.get(1, 2) - A.get(0, 2)*A.get(1, 0)) / dDet;
        _mInverse.get(2, 2) =  (A.get(0, 0)*A.get(1, 1) - A.get(0, 1)*A.get(1, 0)) / dDet;

        return _mInverse;
    }

    mu::Array _mToInvert = A;

    // Allgemeiner Fall fuer n > 3
    for (size_t j = 0; j < _mToInvert.cols(); j++)
    {
        for (size_t i = j; i < _mToInvert.rows(); i++)
        {
            if (_mToInvert.get(i, j) != mu::Value(0.0))
            {
                if (i != j) //vertauschen
                {
                    mu::Value dElement;

                    for (size_t _j = 0; _j < _mToInvert.cols(); _j++)
                    {
                        dElement = _mToInvert.get(i, _j);
                        _mToInvert.get(i, _j) = _mToInvert.get(j, _j);
                        _mToInvert.get(j, _j) = dElement;
                        dElement = _mInverse.get(i, _j);
                        _mInverse.get(i, _j) = _mInverse.get(j, _j);
                        _mInverse.get(j, _j) = dElement;
                    }

                    i = j-1;
                }
                else //Gauss-Elimination
                {
                    mu::Value dPivot = _mToInvert.get(i, j);

                    for (size_t _j = 0; _j < _mToInvert.cols(); _j++)
                    {
                        _mToInvert.get(i, _j) /= dPivot;
                        _mInverse.get(i, _j) /= dPivot;
                    }

                    for (size_t _i = i+1; _i < _mToInvert.rows(); _i++)
                    {
                        mu::Value dFactor = _mToInvert.get(_i, j);

                        if (dFactor == mu::Value(0.0)) // Bereits 0???
                            continue;

                        for (size_t _j = 0; _j < _mToInvert.cols(); _j++)
                        {
                            _mToInvert.get(_i, _j) -= _mToInvert.get(i, _j)*dFactor;
                            _mInverse.get(_i, _j) -= _mInverse.get(i, _j)*dFactor;
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
        if (_mToInvert.get(j, j) == mu::Value(0.0)) // Hauptdiagonale ist ein Element == 0??
            throw mu::ParserError(mu::ecMATRIX_NOT_INVERTIBLE);

        if (_mToInvert.get(j, j) != mu::Value(1.0))
        {
            for (size_t _j = 0; _j < _mInverse.cols(); _j++)
                _mInverse.get(j, _j) /= _mToInvert.get(j, j);

            _mToInvert.get(j, j) = 1.0;
        }

        for (int i = 0; i < j; i++)
        {
            for (size_t _j = 0; _j < _mInverse.cols(); _j++)
                _mInverse.get(i, _j) -= _mToInvert.get(i, j)*_mInverse.get(j, _j);

            _mToInvert.get(i, j) -= _mToInvert.get(i, j)*_mToInvert.get(j, j);
        }
    }

    return _mInverse;
}


/////////////////////////////////////////////////
/// \brief Implementation of the transpose()
/// function.
///
/// \param A const mu::Array&
/// \param dims const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_transpose(const mu::Array& A, const mu::Array& dims)
{
    if (A.isScalar())
        return A;

    mu::DimSizes sizes = A.getDimSizes();
    std::vector<size_t> indexMap(sizes.size());

    if (dims.isDefault() || dims.size() < 2)
    {
        for (size_t i = 0; i < indexMap.size(); i++)
        {
            indexMap[i] = i;
        }

        std::swap(sizes[0], sizes[1]);
        std::swap(indexMap[0], indexMap[1]);
    }
    else
    {
        VectorIndex dimIdx(dims);
        int maxDim = dimIdx.max();

        if (!dimIdx.isPermutation())
            throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, dims.print());

        if ((int)sizes.size() < maxDim)
        {
            sizes.resize(maxDim, 1ull);
            indexMap.resize(maxDim);
        }

        for (size_t i = 0; i < indexMap.size(); i++)
        {
            indexMap[i] = i;
        }

        mu::DimSizes newSizes(sizes);

        for (size_t i = 0; i < dimIdx.size(); i++)
        {
            indexMap[i] = dimIdx[i];
            newSizes[i] = sizes[indexMap[i]];
        }

        sizes = newSizes;
    }

    mu::Array ret(A.size());
    ret.setDimSizes(sizes);

    mu::IndexIterator targetIter(sizes);
    const mu::IndexTuple& targetIndex = targetIter.index();
    mu::IndexTuple sourceIndex = targetIndex;

    do
    {
        for (size_t i = 0; i < indexMap.size(); i++)
        {
            sourceIndex[indexMap[i]] = targetIndex[i];
        }

        ret.get(targetIndex) = A.get(sourceIndex);
    } while (targetIter.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the size() function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_size(const mu::Array& A)
{
    return A.getDimSizes();
}


/////////////////////////////////////////////////
/// \brief Implementation of the cutoff()
/// function.
///
/// \param A const mu::Array&
/// \param threshold const mu::Array&
/// \param _mode const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cutoff(const mu::Array& A, const mu::Array& threshold, const mu::Array& _mode)
{
    // Define the thresholds
    mu::Value thresHigh(NAN);
    mu::Value thresLow(NAN);

    // Get the input values
    mu::Value thresInput = threshold.get(0);
    int mode = _mode.getAsScalarInt();

    // Check if the mode is within the acceptable range
    /*if (std::abs(mode) > 2)
        throw SyntaxError(SyntaxError::CUTOFF_MODE_INVALID, errorInfo.command, errorInfo.position);*/

    // Get the min and max values of the matrix for the threshold calculation
    mu::Value matMin = 0;
    mu::Value matMax = 0;

    // If necessary get the min and max values of the matrices
    if (std::abs(mode) < 2)
    {
        matMin = A.call("min").get(0);
        matMax = A.call("max").get(0);
    }

    // Set the thresholds and the data to insert
    switch (mode)
    {
        case -2:
            // Absolut value, cut all values below
            {
                thresHigh = NAN;
                thresLow = thresInput;
            }
            break;
        case 2:
            // Absolut value, cut all values above
            {
                thresHigh = thresInput;
                thresLow = NAN;
            }
            break;
        case 1:
            // Percentage value, cut all values above
            {
                thresHigh = matMin + (matMax - matMin) * thresInput;
                thresLow = NAN;
            }
            break;
        case -1:
            // Percentage value, cut all values below
            {
                thresHigh = NAN;
                thresLow = matMin + (matMax - matMin) * thresInput;
            }
            break;
        case 0:
            // Central percent of values
            {
                thresHigh = matMin + (matMax - matMin) * mu::Value(0.5) + (matMax - matMin) * thresInput * mu::Value(0.5);
                thresLow = matMin + (matMax - matMin) * mu::Value(0.5) - (matMax - matMin) * thresInput * mu::Value(0.5);
            }
            break;
    }

    // Get a copy of the matrix to process
    mu::Array cutMatrix = A;

    // Apply the calculated threshold to all matrix elements
    for (size_t i = 0; i < cutMatrix.size(); i++)
    {

        if (!mu::isnan(thresLow) && cutMatrix.get(i) < thresLow)
            cutMatrix.get(i) = thresLow;

        if (!mu::isnan(thresHigh) && cutMatrix.get(i) > thresHigh)
            cutMatrix.get(i) = thresHigh;
    }

    return cutMatrix;
}


/////////////////////////////////////////////////
/// \brief Driver code to calculate some
/// statistics from a matrix while being able to
/// use moving windows. The calculation is
/// switched to parallel mode, once the moving
/// window is large enough.
///
/// \param mat const mu::Array&
/// \param logic StatsLogic
/// \param rowStart int
/// \param rowCount size_t
/// \param colStart int
/// \param colCount size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
static mu::Value calculateStats(const mu::Array& mat, StatsLogic logic, int rowStart, size_t rowCount, int colStart, size_t colCount)
{
    constexpr size_t MINTHREADCOUNT = 100;
    constexpr size_t MINELEMENTPERCOL = 100;

    std::vector<StatsLogic> operation(rowCount, logic);

    // Only apply multiprocessing, if there are really a lot of
    // elements to process
    #pragma omp parallel for if (mat.isParallelizable() && rowCount >= MINTHREADCOUNT && colCount >= MINELEMENTPERCOL)
    for (size_t i = 0; i < rowCount; i++)
    {
        if (rowStart + (int)i < 0 || rowStart + i >= mat.rows())
            continue;

        for (size_t j = 0; j < colCount; j++)
        {
            if (colStart + (int)j < 0 || colStart + j >= mat.cols())
                continue;

            operation[i](mat.get(i+rowStart, j+colStart));
        }
    }

    for (size_t i = 1; i < operation.size(); i++)
    {
        operation.front().combine(operation[i]);
    }

    return operation.front().m_val;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movsum()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movsum(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = calculateStats(A, StatsLogic(StatsLogic::OPERATION_ADD),
                                                    i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movstd()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movstd(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
            {
                mu::Value sum = calculateStats(A, StatsLogic(StatsLogic::OPERATION_ADD),
                                               i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);

                mu::Value num = calculateStats(A, StatsLogic(StatsLogic::OPERATION_NUM),
                                               i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);

                mu::Value std = calculateStats(A, StatsLogic(StatsLogic::OPERATION_ADDSQSUB, 0.0, sum/num),
                                               i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);

                if (num > mu::Value(1))
                    _mResult.get(i, j) = std::sqrt(std.as_cmplx() / (num.as_cmplx() - 1.0));
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movavg()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movavg(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
            {
                mu::Value sum = calculateStats(A, StatsLogic(StatsLogic::OPERATION_ADD),
                                               i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);

                mu::Value num = calculateStats(A, StatsLogic(StatsLogic::OPERATION_NUM),
                                               i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);

                if (num > mu::Value(0))
                    _mResult.get(i, j) = sum / num;
            }
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movprd()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movprd(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = calculateStats(A, StatsLogic(StatsLogic::OPERATION_MULT, 1.0),
                                                    i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movmed()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movmed(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

#ifndef PARSERSTANDALONE
    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
            {
                Memory _mem;

                for (int n = 0; n < 2*rowStep+1; n++)
                {
                    if (i+n-rowStep < 0 || i+n-rowStep >= (int)A.rows())
                        continue;

                    for (int m = 0; m < 2*colStep+1; m++)
                    {
                        if (j+m-colStep < 0 || j+m-colStep >= (int)A.cols())
                            continue;

                        _mem.writeData(m + n*(2*colStep+1), 0, A.get(i+n-rowStep, j+m-colStep));
                    }
                }

                _mResult.get(i, j) = _mem.med(VectorIndex(0, (2*rowStep+1)*(2*colStep+1)), VectorIndex(0));
            }
        }
    }
#endif // PARSERSTANDALONE

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movmin()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movmin(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = calculateStats(A, StatsLogic(StatsLogic::OPERATION_MIN, A.get(i, j)),
                                                    i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movmax()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movmax(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = calculateStats(A, StatsLogic(StatsLogic::OPERATION_MAX, A.get(i, j)),
                                                    i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movnorm()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movnorm(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = std::sqrt(calculateStats(A, StatsLogic(StatsLogic::OPERATION_ADDSQ),
                                                              i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1).as_cmplx());
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the movnum()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_movnum(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);

    int rowStep;
    int colStep;

    if (m.isDefault())
    {
        if (n.size() > 1)
        {
            rowStep = n.get(0).getNum().asI64();
            colStep = n.get(1).getNum().asI64();
        }
        else
        {
            rowStep = n.getAsScalarInt();
            colStep = rowStep;
        }
    }
    else
    {
        rowStep = n.getAsScalarInt();
        colStep = m.getAsScalarInt();
    }

    if (rowStep < 0 || colStep < 0 || 2*rowStep+1 > (int)A.rows() || 2*colStep+1 > (int)A.cols())
        throw mu::ParserError(mu::ecINVALID_WINDOW_SIZE, toString(rowStep) + " x " + toString(colStep) + " vs. " + A.printDims());

    mu::Array _mResult(A.rows()*A.cols(), mu::Value(NAN));
    _mResult.setDimSizes({A.rows(), A.cols()});

    #pragma omp parallel for if (A.isParallelizable())
    for (int i = 0; i < (int)_mResult.rows(); i++)
    {
        for (int j = 0; j < (int)_mResult.cols(); j++)
        {
            if (!mu::isnan(A.get(i, j)))
                _mResult.get(i, j) = calculateStats(A, StatsLogic(StatsLogic::OPERATION_NUM),
                                                    i-rowStep, 2*rowStep+1, j-colStep, 2*colStep+1);
        }
    }

    return _mResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the zero() function.
///
/// \param n const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_zero(const mu::MultiArgFuncParams& n)
{
    if (n.count() == 1)
    {
        if (n[0].size() == 1)
        {
            int64_t dim = n[0].getAsScalarInt();
            mu::Array ret(dim*dim, mu::Value(0.0));
            ret.setDimSizes({dim, dim});
            return ret;
        }

        mu::Array ret(n[0].call("prd").getAsScalarInt(), mu::Value(0.0));
        mu::DimSizes dimSizes(n[0].size(), 1ull);

        for (size_t i = 0; i < n[0].size(); i++)
        {
            dimSizes[i] = n[0].get(i).getNum().asUI64();
        }

        ret.setDimSizes(dimSizes);
        return ret;
    }

    mu::DimSizes dimSizes(n.count(), 1ull);
    size_t numElements = 1ull;

    for (size_t i = 0; i < n.count(); i++)
    {
        dimSizes[i] = n[i].getAsScalarInt();
        numElements *= dimSizes[i];
    }

    mu::Array ret(numElements, mu::Value(0.0));
    ret.setDimSizes(dimSizes);
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the one() function.
///
/// \param n const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_one(const mu::MultiArgFuncParams& n)
{
    if (n.count() == 1)
    {
        if (n[0].size() == 1)
        {
            int64_t dim = n[0].getAsScalarInt();
            mu::Array ret(dim*dim, mu::Value(1.0));
            ret.setDimSizes({dim, dim});
            return ret;
        }

        mu::Array ret(n[0].call("prd").getAsScalarInt(), mu::Value(1.0));
        mu::DimSizes dimSizes(n[0].size(), 1ull);

        for (size_t i = 0; i < n[0].size(); i++)
        {
            dimSizes[i] = n[0].get(i).getNum().asUI64();
        }

        ret.setDimSizes(dimSizes);
        return ret;
    }

    mu::DimSizes dimSizes(n.count(), 1ull);
    size_t numElements = 1ull;

    for (size_t i = 0; i < n.count(); i++)
    {
        dimSizes[i] = n[i].getAsScalarInt();
        numElements *= dimSizes[i];
    }

    mu::Array ret(numElements, mu::Value(1.0));
    ret.setDimSizes(dimSizes);
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the identity()
/// function.
///
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_identity(const mu::Array& n)
{
    int dim = n.getAsScalarInt();

    if (dim < 1)
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::Array ret(dim*dim, mu::Value(0.0));
    ret.setDimSizes({dim, dim});

    for (int d = 0; d < dim; d++)
    {
        ret.get(d, d) = mu::Value(1.0);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the correl()
/// function.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_correl(const mu::Array& A, const mu::Array& B)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);
    checkInputMatrix(B, MATRIX_MUST_BE_2D);

    // Ensure that the size is non-zero
    if (!(A.rows() && B.rows()) || !(A.cols() && B.cols()))
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, A.printDims() + ", " + B.printDims());

    int n = std::max(A.rows(), B.rows());
    int m = std::max(A.cols(), B.cols());

    // Create the target matrix
    mu::Array mCorrelation((2*n-1)*(2*m-1), mu::Value(0.0));
    mCorrelation.setDimSizes({2*n-1, 2*m-1});

    // Calculate the elements of the matrix by applying
    // elementwise shifts to the matrices
    #pragma omp parallel for if (A.isParallelizable() && B.isParallelizable())
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
                    mCorrelation.get(i1, j1) += A.get(i2 + std::max(0, i1-n+1), j2 + std::max(0, j1-m+1))
                                               * B.get(i2 + std::max(0, n-i1-1), j2 + std::max(0, m-j1-1));
                }
            }
        }
    }

    return mCorrelation;
}


/////////////////////////////////////////////////
/// \brief Implementation of the covar() function.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_covar(const mu::Array& A, const mu::Array& B)
{
    checkInputMatrix(A, 0);
    checkInputMatrix(B, 0);

    // Ensure that the size is non-zero
    if (A.getDimSizes() != B.getDimSizes())
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID, A.printDims() + " != " + B.printDims());

    // Prepare the target
    mu::Value covariance(0.0);

    // Calculate the average values of both
    // matrices
    mu::Value mAvg1 = A.call("avg").get(0);
    mu::Value mAvg2 = B.call("avg").get(0);

    // Calculate the covariance value for each
    // component and sum it up
    for (size_t i = 0; i < A.size(); i++)
    {
        covariance += (A.get(i) - mAvg1) * (B.get(i) - mAvg2);
    }

    // Normalize the covariance value using
    // the number of elements
    covariance /= (A.size() - 1);

    return covariance;
}


/////////////////////////////////////////////////
/// \brief Implementation of the normalize()
/// function.
///
/// \param A const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_normalize(const mu::Array& A)
{
    if (!A.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::Array ret;
    ret.assign(A);
    mu::Value _mMax = A.call("max").get(0);
    mu::Value _mMin = A.call("min").get(0);

    mu::Value dMax = std::max(std::abs(_mMax.getNum().asF64()), std::abs(_mMin.getNum().asF64()));

    #pragma omp parallel for
    for (size_t i = 0; i < ret.size(); i++)
    {
        ret.get(i) /= dMax;
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the reshape()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_reshape(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    mu::DimSizes dimSizes;

    if (m.isDefault())
    {
        for (size_t i = 0; i < n.size(); i++)
        {
            dimSizes.push_back(n.get(i).getNum().asUI64());
        }
    }
    else
    {
        dimSizes.push_back(n.getAsScalarInt());
        dimSizes.push_back(m.getAsScalarInt());
    }

    size_t numElems = mu::getNumElements(dimSizes);

    if (!A.size() || numElems == 0ull || !dimSizes.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (numElems != A.size())
    {
        mu::Array t;
        t.setDimSizes(dimSizes);
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              t.printDims() + " = " + toString(numElems) + " vs. "
                               + A.printDims() + " = " + toString(A.size()));
    }

    mu::Array ret = A;
    ret.setDimSizes(dimSizes);
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the resize()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_resize(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    mu::DimSizes dimSizes;

    if (m.isDefault())
    {
        for (size_t i = 0; i < n.size(); i++)
        {
            dimSizes.push_back(n.get(i).getNum().asUI64());
        }
    }
    else
    {
        dimSizes.push_back(n.getAsScalarInt());
        dimSizes.push_back(m.getAsScalarInt());
    }

    size_t numElems = mu::getNumElements(dimSizes);

    if (!A.size() || numElems == 0ull || !dimSizes.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::Array ret(numElems, mu::Value(NAN));
    ret.setDimSizes(dimSizes);

    mu::MatrixView AView(A);
    AView.setDimSizes(dimSizes);

    for (size_t i = 0; i < ret.size(); i++)
    {
        if (AView.get(i).getType() != mu::TYPE_NEUTRAL)
            ret.get(i) = AView.get(i);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the repmat()
/// function.
///
/// \param A const mu::Array&
/// \param n const mu::Array&
/// \param m const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_repmat(const mu::Array& A, const mu::Array& n, const mu::Array& m)
{
    mu::DimSizes repetitions;
    mu::DimSizes dimSizes;

    if (m.isDefault())
    {
        for (size_t i = 0; i < n.size(); i++)
        {
            repetitions.push_back(std::max(1ull, n.get(i).getNum().asUI64()));
            dimSizes.push_back(repetitions.back() * A.getSize(i));
        }
    }
    else
    {
        repetitions.push_back(std::max(1LL, n.getAsScalarInt()));
        dimSizes.push_back(repetitions.back() * A.rows());
        repetitions.push_back(std::max(1LL, m.getAsScalarInt()));
        dimSizes.push_back(repetitions.back() * A.cols());
    }

    size_t numReps = mu::getNumElements(repetitions);

    if (!A.size() || numReps == 0ull || !dimSizes.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::Array ret(mu::getNumElements(dimSizes));
    ret.setDimSizes(dimSizes);

    mu::DimSizes dimSizesA = A.getDimSizes();

    // Extend the dimensions of the matrix A
    if (dimSizesA.size() < dimSizes.size())
        dimSizesA.resize(dimSizes.size(), 1ull);

    mu::IndexIterator matrixIter(dimSizesA);
    const mu::IndexTuple& iter = matrixIter.index();
    mu::IndexTuple targetIndex(iter.size());

    do
    {
        mu::IndexIterator repIter(repetitions);
        const mu::IndexTuple& reps = repIter.index();

        do
        {
            for (size_t i = 0; i < targetIndex.size(); i++)
            {
                targetIndex[i] = iter[i] + reps[i] * A.getSize(i);
            }

            ret.get(targetIndex) = A.get(iter);
        } while (repIter.next());
    } while (matrixIter.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the unique()
/// function.
///
/// \param A const mu::Array&
/// \param dim const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_unique(const mu::Array& A, const mu::Array& dim)
{
    if (!A.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    // Depending on the dimensions of the passed matrix, change
    // the evaluation method
    // Matrix
    if (dim.isDefault() || dim.getAsScalarInt() == 0)
    {
        mu::Array ret;
        ret.reserve(A.size());

        for (size_t i = 0; i < A.size(); i++)
        {
            bool insert = true;

            for (size_t j = 0; j < ret.size() && insert; j++)
            {
                if (A.get(i) == ret.get(j))
                    insert = false;
            }

            if (insert)
                ret.push_back(A.get(i));
        }

        return ret;
    }

    size_t uniqueDim = dim.getAsScalarInt()-1;
    size_t uniqueDimSize = A.getSize(uniqueDim);

    if (uniqueDimSize == 1ull)
        return A;

    mu::DimSizes dimSizes = A.getDimSizes();
    dimSizes[uniqueDim] = 1ull;

    std::vector<size_t> uniqueCopies;

    for (size_t i = 0; i < uniqueDimSize; i++)
    {
        bool insert = true;

        for (size_t j = 0; j < i && insert; j++)
        {
            mu::IndexIterator iterator(dimSizes);

            do
            {
                mu::IndexTuple iter_i = iterator.index();
                mu::IndexTuple iter_j = iterator.index();

                iter_i[uniqueDim] = i;
                iter_j[uniqueDim] = j;

                if (A.get(iter_i) == A.get(iter_j))
                {
                    insert = false;
                }

            } while (iterator.next() && insert);
        }

        if (insert)
            uniqueCopies.push_back(i);
    }

    mu::Array ret(mu::getNumElements(dimSizes)*uniqueCopies.size());

    for (size_t i = 0; i < uniqueCopies.size(); i++)
    {
        mu::IndexIterator iterator(dimSizes);

        do
        {
            mu::IndexTuple source = iterator.index();
            mu::IndexTuple target = iterator.index();

            source[uniqueDim] = uniqueCopies[i];
            target[uniqueDim] = i;

            ret.get(target) = A.get(source);

        } while (iterator.next());
    }

    dimSizes[uniqueDim] = uniqueCopies.size();
    ret.setDimSizes(dimSizes);

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the cumsum()
/// function.
///
/// \param A const mu::Array&
/// \param dim const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cumsum(const mu::Array& A, const mu::Array& dim)
{
    mu::Array ret;
    ret.assign(A);

    if (dim.isDefault() || dim.getAsScalarInt() == 0)
    {
        for (size_t i = 1; i < ret.size(); i++)
        {
            ret.get(i) += ret.get(i-1);
        }

        ret.setDimSizes({});
        return ret;
    }

    size_t cumdim = dim.getAsScalarInt()-1;
    size_t cumDimSize = A.getSize(cumdim);

    if (cumDimSize == 1ull)
        return ret;

    mu::DimSizes dimSizes = A.getDimSizes();
    dimSizes[cumdim] = 1ull;

    mu::IndexIterator iterator(dimSizes);

    do
    {
        mu::IndexTuple iter_curr = iterator.index();
        mu::IndexTuple iter_prev = iterator.index();

        for (size_t i = 1; i < cumDimSize; i++)
        {
            iter_curr[cumdim] = i;
            iter_prev[cumdim] = i-1;
            ret.get(iter_curr) += ret.get(iter_prev);
        }
    } while (iterator.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the cumprd()
/// function.
///
/// \param A const mu::Array&
/// \param dim const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cumprd(const mu::Array& A, const mu::Array& dim)
{
    mu::Array ret;
    ret.assign(A);

    if (dim.isDefault() || dim.getAsScalarInt() == 0)
    {
        for (size_t i = 1; i < ret.size(); i++)
        {
            ret.get(i) *= ret.get(i-1);
        }

        ret.setDimSizes({});
        return ret;
    }

    size_t cumdim = dim.getAsScalarInt()-1;
    size_t cumDimSize = A.getSize(cumdim);

    if (cumDimSize == 1ull)
        return ret;

    mu::DimSizes dimSizes = A.getDimSizes();
    dimSizes[cumdim] = 1ull;

    mu::IndexIterator iterator(dimSizes);

    do
    {
        mu::IndexTuple iter_curr = iterator.index();
        mu::IndexTuple iter_prev = iterator.index();

        for (size_t i = 1; i < cumDimSize; i++)
        {
            iter_curr[cumdim] = i;
            iter_prev[cumdim] = i-1;
            ret.get(iter_curr) *= ret.get(iter_prev);
        }
    } while (iterator.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Try to solve the system of linear
/// equations Ax = B.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
__attribute__((force_align_arg_pointer)) mu::Array matfnc_solve(const mu::Array& A, const mu::Array& B)
{
    checkInputMatrix(A, MATRIX_MUST_BE_2D);
    checkInputMatrix(B, 0);

    Eigen::MatrixXcd mA(A.rows(), A.cols());
    Eigen::MatrixXcd mB(B.rows(), B.cols());

    // Copy the passed matrix into an Eigen matrix
    #pragma omp parallel for if (A.isParallelizable())
    for (size_t i = 0; i < A.rows(); i++)
    {
        for (size_t j = 0; j < A.cols(); j++)
        {
            mA(i,j) = A.get(i,j).as_cmplx();
        }
    }

    // Copy the passed matrix into an Eigen matrix
    #pragma omp parallel for if (B.isParallelizable())
    for (size_t i = 0; i < B.rows(); i++)
    {
        for (size_t j = 0; j < B.cols(); j++)
        {
            mB(i,j) = B.get(i,j).as_cmplx();
        }
    }

    Eigen::MatrixXcd result = mA.colPivHouseholderQr().solve(mB);

    mu::Array ret(result.rows()*result.cols());
    ret.setDimSizes({result.rows(), result.cols()});

    #pragma omp parallel for
    for (size_t i = 0; i < ret.rows(); i++)
    {
        for (size_t j = 0; j < ret.cols(); j++)
        {
            ret.get(i,j) = result(i,j);
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the diag() function.
///
/// \param diagonal const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_diag(const mu::MultiArgFuncParams& diagonal)
{
    if (diagonal.count() == 1)
    {
        mu::Array ret(diagonal[0].size()*diagonal[0].size(), mu::Value(0.0));
        ret.setDimSizes({diagonal[0].size(), diagonal[0].size()});

        for (size_t i = 0; i < diagonal[0].size(); i++)
        {
            ret.get(i, i) = diagonal[0].get(i);
        }

        return ret;
    }

    mu::Array ret(diagonal.count()*diagonal.count(), mu::Value(0.0));
    ret.setDimSizes({diagonal.count(), diagonal.count()});

    for (size_t i = 0; i < diagonal.count(); i++)
    {
        ret.get(i, i) = diagonal[i].get(0);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the carttocyl()
/// function.
///
/// \param cartesian const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_carttocyl(const mu::Array& cartesian)
{
    if (!cartesian.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (cartesian.cols() > 3 || cartesian.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              cartesian.printDims() + " vs. "+ toString(cartesian.rows()) + " x 3");

    mu::Array ret;
    ret.assign(cartesian);

    #pragma omp parallel for if (cartesian.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = numfnc_sqrt(cartesian.get(i, 0).pow(2) + cartesian.get(i, 1).pow(2)).front();
        ret.get(i, 1) = numfnc_phi(cartesian.get(i, 0), cartesian.get(i, 1)).front();
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the carttopol()
/// function.
///
/// \param cartesian const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_carttopol(const mu::Array& cartesian)
{
    if (!cartesian.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (cartesian.cols() > 3 || cartesian.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              cartesian.printDims() + " vs. "+ toString(cartesian.rows()) + " x 3");

    if (cartesian.cols() == 2)
        return matfnc_carttocyl(cartesian);

    mu::Array ret;
    ret.assign(cartesian);

    #pragma omp parallel for if (cartesian.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = numfnc_sqrt(cartesian.get(i, 0).pow(2)
                           + cartesian.get(i, 1).pow(2)
                           + cartesian.get(i, 2).pow(2)).front();
        ret.get(i, 1) = numfnc_phi(cartesian.get(i, 0), cartesian.get(i, 1)).front();
        ret.get(i, 2) = numfnc_theta(cartesian.get(i, 0), cartesian.get(i, 1), cartesian.get(i, 2)).front();
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the cyltocart()
/// function.
///
/// \param cylindrical const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cyltocart(const mu::Array& cylindrical)
{
    if (!cylindrical.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (cylindrical.cols() > 3 || cylindrical.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              cylindrical.printDims() + " vs. "+ toString(cylindrical.rows()) + " x 3");

    mu::Array ret;
    ret.assign(cylindrical);

    #pragma omp parallel for if (cylindrical.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = cylindrical.get(i, 0) * numfnc_cos(cylindrical.get(i, 1)).front();
        ret.get(i, 1) = cylindrical.get(i, 0) * numfnc_sin(cylindrical.get(i, 1)).front();
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the cyltopol()
/// function.
///
/// \param cylindrical const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_cyltopol(const mu::Array& cylindrical)
{
    if (!cylindrical.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (cylindrical.cols() > 3 || cylindrical.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              cylindrical.printDims() + " vs. "+ toString(cylindrical.rows()) + " x 3");

    if (cylindrical.cols() == 2)
        return cylindrical;

    mu::Array ret;
    ret.assign(cylindrical);

    #pragma omp parallel for if (cylindrical.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = numfnc_sqrt(cylindrical.get(i, 0).pow(2) + cylindrical.get(i, 2).pow(2)).front();
        ret.get(i, 2) = numfnc_theta(cylindrical.get(i, 0) * numfnc_cos(cylindrical.get(i, 1)),
                                     cylindrical.get(i, 0) * numfnc_sin(cylindrical.get(i, 1)),
                                     cylindrical.get(i, 2)).front();
    }

    return ret;

}


/////////////////////////////////////////////////
/// \brief Implementation of the poltocart()
/// function.
///
/// \param polar const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_poltocart(const mu::Array& polar)
{
    if (!polar.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (polar.cols() > 3 || polar.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              polar.printDims() + " vs. "+ toString(polar.rows()) + " x 3");

    if (polar.cols() == 2)
        return matfnc_cyltocart(polar);

    mu::Array ret;
    ret.assign(polar);

    #pragma omp parallel for if (polar.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = polar.get(i, 0) * numfnc_cos(polar.get(i, 1)).front() * numfnc_sin(polar.get(i, 2)).front();
        ret.get(i, 1) = polar.get(i, 0) * numfnc_sin(polar.get(i, 1)).front() * numfnc_sin(polar.get(i, 2)).front();
        ret.get(i, 2) = polar.get(i, 0) * numfnc_cos(polar.get(i, 2)).front();
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the poltocyl()
/// function.
///
/// \param polar const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_poltocyl(const mu::Array& polar)
{
    if (!polar.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (polar.cols() > 3 || polar.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              polar.printDims() + " vs. "+ toString(polar.rows()) + " x 3");

    mu::Array ret;
    ret.assign(polar);

    #pragma omp parallel for if (polar.isParallelizable())
    for (size_t i = 0; i < ret.rows(); i++)
    {
        ret.get(i, 0) = polar.get(i, 0) * numfnc_sin(polar.get(i, 2)).front();

        if (polar.cols() == 3)
            ret.get(i, 2) = polar.get(i, 0) * numfnc_cos(polar.get(i, 2)).front();
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This static function finds the nearest
/// lower grid axis value.
///
/// \param gaxes const mu::Array&
/// \param axis size_t
/// \param axisval const mu::Value&
/// \return size_t
///
/////////////////////////////////////////////////
static size_t findNearestLowerGridAxisValue(const mu::Array& gaxes, size_t axis, const mu::Value& axisval)
{
    mu::Value sign = gaxes.get(0, axis) > gaxes.get(gaxes.cols()-1, axis) ? -1 : 1;

    for (size_t i = 0; i < gaxes.rows(); i++)
    {
        if (sign * gaxes.get(i, axis) >= sign * axisval)
        {
            if (i)
                return i-1;

            return 0u;
        }
    }

    return gaxes.rows()-1;
}


/////////////////////////////////////////////////
/// \brief Implementation of the coordstogrid()
/// function.
///
/// \param grid const mu::Array&
/// \param coords const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_coordstogrid(const mu::Array& grid, const mu::Array& coords)
{
    if (grid.rows() < 2 || !grid.cols() || !coords.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (grid.cols() > 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              grid.printDims() + " vs. "+ toString(grid.rows()) + " x 2");

    if (coords.cols() > 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              coords.printDims() + " vs. "+ toString(coords.rows()) + " x 2");

    if (coords.cols() == 2 && grid.cols() < 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              grid.printDims() + " vs. "+ coords.printDims());

    mu::Array gcoords;
    gcoords.assign(coords);

    #pragma omp parallel for if (grid.isParallelizable())
    for (size_t i = 0; i < gcoords.rows(); i++)
    {
        for (size_t j = 0; j < gcoords.cols(); j++)
        {
            size_t pos = findNearestLowerGridAxisValue(grid, j, gcoords.get(i, j)); // find the lower grid axis value assuming sorted axis
            mu::Value off = gcoords.get(i, j) - grid.get(pos, j); // should be smaller than grid interval, but might be negative
            mu::Value interval = pos+1 < grid.rows()
                ? grid.get(pos+1, j) - grid.get(pos, j)
                : grid.get(pos, j) - grid.get(pos-1, j); // the grid interval. Might also be negative

            gcoords.get(i, j) = mu::Value(pos + 1) + off / interval; // if off == interval, then pos+1, else pos + (<1) and +1 due to zero-based coords
        }
    }

    return gcoords;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// bilinearInterpolation().
///
/// \param mat const mu::Array&
/// \param row int
/// \param col int
/// \return mu::Value
///
/////////////////////////////////////////////////
static mu::Value readMat(const mu::Array& mat, int row, int col)
{
    if (row < (int)mat.rows() && col < (int)mat.cols() && row >= 0 && col >= 0)
        return mat.get(row, col);
    else
        return NAN;
}


/////////////////////////////////////////////////
/// \brief Performs the bilinear interpolation of
/// the matrix value at the selected coordinates.
///
/// \param mat const mu::Array&
/// \param row const mu::Value&
/// \param col const mu::Value&
/// \return mu::Value
///
/////////////////////////////////////////////////
static mu::Value bilinearInterpolation(const mu::Array& mat, const mu::Value& row, const mu::Value& col = mu::Value(0.0))
{
    if (mu::isnan(row) || mu::isnan(col))
        return mu::Value(NAN);

    static const mu::Value ZERO = 0.0;
    static const mu::Value ONE = 1.0;

    // Find the base index
    int nBaseLine = intCast(row.as_cmplx()) + (row < ZERO ? -1 : 0);
    int nBaseCol = intCast(col.as_cmplx()) + (col < ZERO ? -1 : 0);

    // Get the decimal part of the double indices
    mu::Value x = row - mu::Value(nBaseLine);
    mu::Value y = col - mu::Value(nBaseCol);

    // Find the surrounding four entries
    mu::Value f00 = readMat(mat, nBaseLine, nBaseCol);
    mu::Value f10 = readMat(mat, nBaseLine+1, nBaseCol);
    mu::Value f01 = readMat(mat, nBaseLine, nBaseCol+1);
    mu::Value f11 = readMat(mat, nBaseLine+1, nBaseCol+1);

    // If all are NAN, return NAN
    if (mu::isnan(f00) && mu::isnan(f01) && mu::isnan(f10) && mu::isnan(f11))
        return mu::Value(NAN);

    // Otherwise set NAN to zero
    f00 = mu::isnan(f00) ? ZERO : f00;
    f10 = mu::isnan(f10) ? ZERO : f10;
    f01 = mu::isnan(f01) ? ZERO : f01;
    f11 = mu::isnan(f11) ? ZERO : f11;

    //     f(0,0) (1-x) (1-y) + f(1,0) x (1-y) + f(0,1) (1-x) y + f(1,1) x y
    return f00*(ONE-x)*(ONE-y)    + f10*x*(ONE-y)    + f01*(ONE-x)*y    + f11*x*y;
}


/////////////////////////////////////////////////
/// \brief Implementation of the interpolate()
/// function.
///
/// \param grid const mu::Array&
/// \param coords const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_interpolate(const mu::Array& grid, const mu::Array& coords)
{
    if (!grid.size() || !coords.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    if (coords.cols() >= 2 && grid.cols() <= 2)
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              grid.printDims() + " vs. " + coords.printDims());

    mu::Array interp(coords.rows() * std::max(1ull, coords.cols()-1), mu::Value(0.0));
    interp.setDimSizes({coords.rows(), std::max(1ull, coords.cols()-1)});

    // Interpolate all values in the matrix coords. First
    // column contains the row values, all remaining contain
    // the corresponding col values
    #pragma omp parallel for if (grid.isParallelizable() && coords.isParallelizable())
    for (size_t i = 0; i < coords.rows(); i++)
    {
        if (coords.cols() >= 2)
        {
            for (size_t j = 1; j < coords.cols(); j++)
            {
                interp.get(i, j-1) = bilinearInterpolation(grid, coords.get(i, 0)-mu::Value(1.0), coords.get(i, j)-mu::Value(1.0));
            }
        }
        else
            interp.get(i, 0) = bilinearInterpolation(grid, coords.get(i, 0)-mu::Value(1.0));
    }

    return interp;
}


/////////////////////////////////////////////////
/// \brief Implementation of the hcat() function.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_hcat(const mu::Array& A, const mu::Array& B)
{
    mu::MatrixView viewA(A);
    mu::MatrixView viewB(B);
    viewA.mergeDimSizes(viewB);
    mu::DimSizes dimSizes = viewA.m_dimSizes;

    if (dimSizes.size() < 2)
        dimSizes.push_back(2);
    else
        dimSizes[1] = A.cols() + B.cols();

    mu::Array ret(mu::getNumElements(dimSizes), mu::Value(NAN));
    ret.setDimSizes(dimSizes);

    mu::IndexIterator iteratorA(A.getDimSizes());
    const mu::IndexTuple& iterA = iteratorA.index();

    do
    {
        ret.get(iterA) = A.get(iterA);
    } while (iteratorA.next());

    mu::IndexIterator iteratorB(B.getDimSizes());
    const mu::IndexTuple& iterB = iteratorB.index();

    do
    {
        mu::IndexTuple iter = iterB;

        if (iter.size() < 2)
            iter.push_back(A.cols());
        else
            iter[1] += A.cols();

        ret.get(iter) = B.get(iterB);
    } while (iteratorB.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the vcat() function.
///
/// \param A const mu::Array&
/// \param B const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_vcat(const mu::Array& A, const mu::Array& B)
{
    mu::MatrixView viewA(A);
    mu::MatrixView viewB(B);
    viewA.mergeDimSizes(viewB);
    mu::DimSizes dimSizes = viewA.m_dimSizes;

    dimSizes[0] = A.rows() + B.rows();

    mu::Array ret(mu::getNumElements(dimSizes), mu::Value(NAN));
    ret.setDimSizes(dimSizes);

    mu::IndexIterator iteratorA(A.getDimSizes());
    const mu::IndexTuple& iterA = iteratorA.index();

    do
    {
        ret.get(iterA) = A.get(iterA);
    } while (iteratorA.next());

    mu::IndexIterator iteratorB(B.getDimSizes());
    const mu::IndexTuple& iterB = iteratorB.index();

    do
    {
        mu::IndexTuple iter = iterB;
        iter[0] += A.rows();
        ret.get(iter) = B.get(iterB);
    } while (iteratorB.next());

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the select()
/// function.
///
/// \param data const mu::Array&
/// \param rows const mu::Array&
/// \param cols const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_select(const mu::Array& data, const mu::Array& rows, const mu::Array& cols)
{
    if (!data.size()
        || !rows.size()
        || !cols.size())
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              data.printDims() + " vs. " + rows.printDims());

    // Store the scalar state
    bool isScalar[2] = {rows.isScalar(),
                        cols.isScalar()};

    if (!isScalar[0]
        && !isScalar[1]
        && (rows.rows() != cols.rows() || rows.cols() != cols.cols()))
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              rows.printDims() + " vs. " + cols.printDims());

    mu::DimSizes dimSizes = {std::max(rows.rows(), cols.rows()), std::max(rows.cols(), cols.cols())};
    // Prepare the return value
    mu::Array selected(dimSizes[0]*dimSizes[1], mu::Value(NAN));
    selected.setDimSizes(dimSizes);

    int row = 0;
    int col = 0;

    // Store the scalar values
    if (isScalar[0])
        row = rows.getAsScalarInt()-1;

    if (isScalar[1])
        col = cols.getAsScalarInt()-1;

    #pragma omp parallel for if (data.isParallelizable() && rows.isParallelizable() && cols.isParallelizable()) firstprivate(row,col)
    for (size_t i = 0; i < selected.rows(); i++)
    {
        for (size_t j = 0; j < selected.cols(); j++)
        {
            // Get the values, if they are no scalars
            if (!isScalar[0])
                row = intCast(rows.get(i, j).getNum().asF64())-1;

            if (!isScalar[1])
                col = intCast(cols.get(i, j).getNum().asF64())-1;

            // Extract the selected value
            if (row >= 0 && row < (int)data.rows()
                && col >= 0 && col < (int)data.cols())
                selected.get(i, j) = data.get(row, col);
        }
    }

    return selected;
}


/////////////////////////////////////////////////
/// \brief Implementation of the assemble()
/// function.
///
/// \param rows const mu::Array&
/// \param cols const mu::Array&
/// \param data const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_assemble(const mu::Array& rows, const mu::Array& cols, const mu::Array& data)
{
    if (!data.size()
        || !rows.size()
        || !cols.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    // Store the scalar state
    bool isScalar[2] = {rows.isScalar(),
                        cols.isScalar()};

    if (!isScalar[0]
        && !isScalar[1]
        && (rows.rows() != cols.rows()
            || rows.cols() != cols.cols()
            || rows.rows() != data.rows()
            || rows.cols() != data.cols()))
        throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                              rows.printDims() + " vs. " + cols.printDims() + " vs. " + data.printDims());

    // Prepare the return value
    int rowCount = rows.call("max").getAsScalarInt();
    int colCount = cols.call("max").getAsScalarInt();

    // Prepare the filled matrix
    mu::Array assembled(rowCount*colCount, mu::Value(NAN));
    assembled.setDimSizes({rowCount, colCount});

    int row = 0;
    int col = 0;

    // Store the scalar values
    if (isScalar[0])
        row = rows.getAsScalarInt()-1;

    if (isScalar[1])
        col = cols.getAsScalarInt()-1;

    #pragma omp parallel for if (data.isParallelizable() && rows.isParallelizable() && cols.isParallelizable()) firstprivate(row,col)
    for (size_t i = 0; i < data.rows(); i++)
    {
        for (size_t j = 0; j < data.cols(); j++)
        {
            // Get the values, if they are no scalars
            if (!isScalar[0])
                row = intCast(rows.get(i, j).getNum().asF64())-1;

            if (!isScalar[1])
                col = intCast(cols.get(i, j).getNum().asF64())-1;

            assembled.get(row, col) = data.get(i, j);
        }
    }

    return assembled;
}


/////////////////////////////////////////////////
/// \brief Implementation of the polylength()
/// function.
///
/// \param poly const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_polylength(const mu::MultiArgFuncParams& poly)
{
    mu::Value len(0.0);

    if (poly.count() == 1)
    {
        if (!poly[0].size())
            throw mu::ParserError(mu::ecMATRIX_EMPTY);

        if (poly[0].cols() < 2)
            throw mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                                  poly[0].printDims());

        for (size_t i = 0; i < poly[0].rows()-1; i++)
        {
            double sum = 0;

            for (size_t j = 0; j < poly[0].cols(); j++)
            {
                sum += ((poly[0].get(i+1, j).as_cmplx() - poly[0].get(i, j).as_cmplx())
                        * std::conj(poly[0].get(i+1, j).as_cmplx() - poly[0].get(i, j).as_cmplx())).real();
            }

            len += std::sqrt(sum);
        }
    }
    else if (poly.count() >= 2)
    {
        size_t maxDim = 0;

        for (size_t n = 0; n < poly.count(); n++)
        {
            maxDim = std::max(maxDim, poly[n].size());
        }

        for (size_t n = 0; n < poly.count()-1; n++)
        {
            double sum = 0;

            for (size_t i = 0; i < maxDim; i++)
            {
                sum += ((poly[n+1].get(i).as_cmplx() - poly[n].get(i).as_cmplx())
                        * std::conj(poly[n+1].get(i).as_cmplx() - poly[n].get(i).as_cmplx())).real();
            }

            len += std::sqrt(sum);
        }
    }
    else
        mu::ParserError(mu::ecMATRIX_DIMS_INVALID,
                        poly[0].printDims());

    return len;
}


/////////////////////////////////////////////////
/// \brief This static helper function applies
/// the kernel to the subset of a matrix using
/// the provided indices.
///
/// \param kernel const mu::Array&
/// \param data const mu::Array&
/// \param rows const VectorIndex&
/// \param cols const VectorIndex&
/// \return mu::Value
///
/////////////////////////////////////////////////
static mu::Value applyKernel(const mu::Array& kernel, const mu::Array& data, const VectorIndex& rows, const VectorIndex& cols, const size_t startRow, const size_t startCol)
{
    // Init the result to zero
    mu::Value result(0);

    // Perform element wise multiplication and sum up each result
    for (size_t i = 0; i < kernel.rows(); i++)
    {
        for (size_t j = 0; j < kernel.cols(); j++)
        {
            result += kernel.get(i, j) * data.get(rows[startRow + i], cols[startCol + j]);
        }
    }

    return result;
}


/////////////////////////////////////////////////
/// \brief Function that allows the user to apply
/// a customer filter kernel to a matrix while
/// applying different boundary conditions.
///
/// \param data const mu::Array&
/// \param kernel const mu::Array&
/// \param mode int
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array matrixRasterFilter(const mu::Array& data, const mu::Array& kernel, int mode)
{
    // Store the input dimensions for later use
    size_t inputRows = data.rows();
    size_t inputCols = data.cols();

    // Define the offset that is half the filter size
    size_t offsetRows = (kernel.rows() - 1) / 2;
    size_t offsetCols = (kernel.cols() - 1) / 2;

    // Generate the vectors with indices that represent the matrix rows and cols
    VectorIndex rows(0, inputRows - 1);
    VectorIndex cols(0, inputCols - 1);

    // Add additional indices that represent the boundary condition
    switch (mode)
    {
        case 0: // boundary clamp
            rows.prepend(std::vector<int>(offsetRows, 0));
            rows.append(std::vector<int>(offsetRows, inputRows - 1));
            cols.prepend(std::vector<int>(offsetCols, 0));
            cols.append(std::vector<int>(offsetCols, inputCols - 1));
            break;
        case 1: // boundary reflect
            rows.prepend(VectorIndex(offsetRows, 1));
            rows.append(VectorIndex(inputRows - 2, inputRows - offsetRows - 1));
            cols.prepend(VectorIndex(offsetCols, 1));
            cols.append(VectorIndex(inputCols - 2, inputCols - offsetCols - 1));
            break;
    }

    // Generate the result matrix
    mu::Array filtered(inputRows*inputCols, mu::Value(NAN));
    filtered.setDimSizes({inputRows, inputCols});

    // Calculate the results by applying the filter to the input pixel by pixel
    #pragma omp parallel for if (data.isParallelizable() && kernel.isParallelizable())
    for (int i = 0; i < (int)filtered.rows(); i++)
    {
        for (int j = 0; j < (int)filtered.cols(); j++)
        {
            // Multiply the two matrices using a helper function
            filtered.get(i, j) = applyKernel(kernel, data, rows, cols, i, j);
        }
    }

    return filtered;
}


/////////////////////////////////////////////////
/// \brief Helper function that performs the
/// actual convolution. This includes replacing
/// invalid values, transformations, multiplication
/// and inverse transformation.
///
/// \param mat1 Matrix&
/// \param mat2 Matrix&
/// \return Matrix
///
/////////////////////////////////////////////////
static Matrix convolution(Matrix& mat1, Matrix& mat2)
{
    //Transform matrix 1
    mglDataC _fftData1;

    // Link the existing matrix to the fft object. Avoids copying the data
    std::vector<std::complex<double>>& m1 = mat1.data();
    _fftData1.Link(&m1[0], mat1.rows(), mat1.cols());

    // Replace all nans with zeros
    for (size_t i = 0; i < mat1.rows(); i++)
        for (size_t j = 0; j < mat1.cols(); j++)
            mat1(i, j) = mu::isnan(mat1(i, j)) ? std::complex<double>(0.0) : mat1(i, j);

    // Perform the transformation
    _fftData1.FFT("xy");

    //Transform matrix 2
    mglDataC _fftData2;

    std::vector<std::complex<double>>& m2 = mat2.data();

    // Link the existing matrix to the fft object. Avoids copying the data
    _fftData2.Link(&m2[0], mat2.rows(), mat2.cols());

    // Replace all nans with zeros
    for (size_t i = 0; i < mat2.rows(); i++)
        for (size_t j = 0; j < mat2.cols(); j++)
            mat2(i, j) = mu::isnan(mat2(i, j)) ? std::complex<double>(0.0) : mat2(i, j);

    // Perform the transformation
    _fftData2.FFT("xy");

    // Perform the convolution as an element wise multiplication
    for (size_t i = 0; i < mat1.rows(); i++)
        for (size_t j = 0; j < mat1.cols(); j++)
            mat1(i, j) = mat1(i, j) * mat2(i, j);

    // Take the inverse of the convolution result
    _fftData1.FFT("ixy");

    //mat1.data() = m1;

    // Write results to matrix
    return mat1;
}


/////////////////////////////////////////////////
/// \brief Function that calculates a convolution
/// using a transformation into the fourier space.
///
/// \param data const mu::Array&
/// \param kernel const mu::Array&
/// \param mode int
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array matrixConvolution(const mu::Array& data, const mu::Array& kernel, int mode)
{
    // Store the input dimensions for later use
    size_t inputRows = data.rows();
    size_t inputCols = data.cols();

    // Define the offset that is half the filter size
    size_t offsetRows = (kernel.rows() - 1) / 2;
    size_t offsetCols = (kernel.cols() - 1) / 2;

    // Generate the vectors with indices that represent the matrix rows and cols
    VectorIndex rows(0, inputRows - 1);
    VectorIndex cols(0, inputCols - 1);

    // Add additional indices that represent the boundary condition
    switch (mode)
    {
        case 0: // boundary clamp
            rows.prepend(std::vector<int>(offsetRows, 0));
            rows.append(std::vector<int>(offsetRows, inputRows - 1));
            cols.prepend(std::vector<int>(offsetCols, 0));
            cols.append(std::vector<int>(offsetCols, inputCols - 1));
            break;
        case 1: // boundary reflect
            rows.prepend(VectorIndex(offsetRows, 1));
            rows.append(VectorIndex(inputRows - 2, inputRows - offsetRows - 1));
            cols.prepend(VectorIndex(offsetCols, 1));
            cols.append(VectorIndex(inputCols - 2, inputCols - offsetCols - 1));
            break;
    }

    // Generate the result matrix
    mu::Array filtered(inputRows*inputCols, mu::Value(NAN));
    filtered.setDimSizes({inputRows, inputCols});

    // Calculate the results by calculating the fourier transformation, performing the
    // convolution by multiplication in fourier space and finally applying the inverse fft

    // Get the complete matrix from the previously generated indices
    Matrix mat(rows.size(), cols.size());
    for (size_t i = 0; i < mat.rows(); i++)
        for (size_t j = 0; j < mat.cols(); j++)
            mat(i, j) = data.get(rows[i], cols[j]);

    // Extend the kernel with zeros to the same size as the extended matrix
    Matrix filterKernel(rows.size(), cols.size(), mu::Value(0.0));
    for (size_t i = 0; i < kernel.rows(); i++)
        for (size_t j = 0; j < kernel.cols(); j++)
            filterKernel(i + offsetRows + inputRows / 2 - kernel.rows() / 2,
                         j + offsetCols + inputCols / 2 - kernel.cols() / 2) = kernel.get(i, j);

    // Call the convolution function
    Matrix extendedResult = convolution(mat, filterKernel);

    // Extract the relevant part of the result and perform the axis shift
    for (size_t i = 0; i < inputRows; i++)
        for (size_t j = 0; j < inputCols; j++)
            filtered.get(i, j) = extendedResult(i + (i >= inputRows/2 ? -(int)inputRows/2 : inputRows/2 + inputRows % 2 + 2 * offsetRows),
                                                j + (j >= inputCols/2 ? -(int)inputCols/2 : inputCols/2 + inputCols % 2 + 2 * offsetCols));

    return filtered;
}


/////////////////////////////////////////////////
/// \brief Implementation of the filter()
/// function.
///
/// \param data const mu::Array&
/// \param kernel const mu::Array&
/// \param mode const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_filter(const mu::Array& data, const mu::Array& kernel, const mu::Array& mode)
{
    // Check that matrix and the filter are not empty
    checkInputMatrix(data, MATRIX_MUST_BE_2D);
    checkInputMatrix(kernel, MATRIX_MUST_BE_2D);

    // Check if filter size is valid for the given matrix, check that filter has an uneven number of rows and cols
    if (kernel.rows() > data.rows() || kernel.cols() > data.cols() || !(kernel.rows() % 2) || !(kernel.cols() % 2))
        throw mu::ParserError(mu::ecINVALID_FILTER_SIZE, kernel.printDims() + " vs. " + data.printDims());

    int boundaryMode = 0;

    if (!mode.isDefault())
        boundaryMode = mode.getAsScalarInt();

    if (boundaryMode > 1 || boundaryMode < 0)
        boundaryMode = 0;

    // Select the method to use
    if (kernel.rows() * kernel.cols() > 200)
        return matrixConvolution(data, kernel, boundaryMode);

    return matrixRasterFilter(data, kernel, boundaryMode);
}


/////////////////////////////////////////////////
/// \brief Implementation of the circshift()
/// function.
///
/// \param A const mu::Array&
/// \param steps const mu::Array&
/// \param dim const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_circshift(const mu::Array& A, const mu::Array& steps, const mu::Array& dim)
{
    // Design decision: due to exponential higher degrees of freedom in higher order tensor on
    // how to enumerate things after a shift dimension was chosen, we decided to keep all other
    // dimensions in the same order. This means that the shift dimension is just moved in front
    // of the major dimension (rows), i.e., the index tuple ijk will always be kij, if k is the
    // shift dimension, and never kji.
    int shiftDim = 0;
    int shiftSteps = steps.getAsScalarInt();

    if (!dim.isDefault())
        shiftDim = dim.getAsScalarInt() - 1;

    // Check that matrix and the filter are not empty
    if (!A.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::DimSizes dimSizes = A.getDimSizes();

    if (dimSizes.size() <= (size_t)shiftDim || !shiftSteps || !(std::abs(shiftSteps) % A.size()))
        return A;

    mu::Array shifted(A.size());
    shifted.setDimSizes(dimSizes);

    shiftSteps = (shiftSteps < 0 ? -1 : 1) * (std::abs(shiftSteps) % A.size());
    std::vector<size_t> indexMap(dimSizes.size());

    for (size_t i = 0; i < indexMap.size(); i++)
    {
        indexMap[i] = i;
    }

    if (shiftDim)
    {
        indexMap.insert(indexMap.begin(), indexMap[shiftDim]);
        indexMap.erase(indexMap.begin()+shiftDim+1);
        dimSizes.insert(dimSizes.begin(), dimSizes[shiftDim]);
        dimSizes.erase(dimSizes.begin()+shiftDim+1);
    }

    mu::IndexIterator sourceIter(dimSizes);
    mu::IndexIterator targetIter(dimSizes);

    const mu::IndexTuple& virtualSourceIndex = sourceIter.index();
    const mu::IndexTuple& virtualTargetIndex = targetIter.index();

    mu::IndexTuple sourceIndex(virtualSourceIndex.size());
    mu::IndexTuple targetIndex(virtualTargetIndex.size());

    std::queue<mu::IndexTuple> indexBuffer;

    // Buffer the elements shifted from or to the end
    while (indexBuffer.size() < std::abs(shiftSteps))
    {
        if (shiftSteps > 0)
        {
            for (size_t i = 0; i < indexMap.size(); i++)
            {
                targetIndex[indexMap[i]] = virtualTargetIndex[i];
            }

            indexBuffer.push(targetIndex);
            targetIter.next();
        }
        else
        {
            for (size_t i = 0; i < indexMap.size(); i++)
            {
                sourceIndex[indexMap[i]] = virtualSourceIndex[i];
            }

            indexBuffer.push(sourceIndex);
            sourceIter.next();
        }
    }

    // Copy the intermediate elements
    while (true)
    {
        for (size_t i = 0; i < indexMap.size(); i++)
        {
            targetIndex[indexMap[i]] = virtualTargetIndex[i];
            sourceIndex[indexMap[i]] = virtualSourceIndex[i];
        }

        shifted.get(targetIndex) = A.get(sourceIndex);

        if (!sourceIter.more() || !targetIter.more())
            break;

        sourceIter.next();
        targetIter.next();
    }

    // Copy the elements shifted from or to the end
    while (indexBuffer.size())
    {
        if (shiftSteps > 0)
        {
            sourceIter.next();

            for (size_t i = 0; i < indexMap.size(); i++)
            {
                sourceIndex[indexMap[i]] = virtualSourceIndex[i];
            }

            shifted.get(indexBuffer.front()) = A.get(sourceIndex);
        }
        else
        {
            targetIter.next();

            for (size_t i = 0; i < indexMap.size(); i++)
            {
                targetIndex[indexMap[i]] = virtualTargetIndex[i];
            }

            shifted.get(targetIndex) = A.get(indexBuffer.front());
        }

        indexBuffer.pop();
    }

    return shifted;
}


/////////////////////////////////////////////////
/// \brief Implementation of the vectshift()
/// function.
///
/// \param A const mu::Array&
/// \param steps const mu::Array&
/// \param dim const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array matfnc_vectshift(const mu::Array& A, const mu::Array& steps, const mu::Array& dim)
{
    int shiftDim = 0;
    int shiftSteps = steps.getAsScalarInt();

    if (!dim.isDefault())
        shiftDim = dim.getAsScalarInt()-1;

    // Check that matrix and the filter are not empty
    if (!A.size())
        throw mu::ParserError(mu::ecMATRIX_EMPTY);

    mu::DimSizes dimSizes = A.getDimSizes();

    if (dimSizes.size() <= (size_t)shiftDim || !shiftSteps || !(std::abs(shiftSteps) % dimSizes[shiftDim]))
        return A;

    mu::Array shifted(A.size());
    shifted.setDimSizes(dimSizes);

    shiftSteps = (shiftSteps < 0 ? -1 : 1) * (std::abs(shiftSteps) % dimSizes[shiftDim]);
    int dimSize = (int)dimSizes[shiftDim];
    mu::IndexIterator iterator(dimSizes);
    const mu::IndexTuple& targetIter = iterator.index();

    do
    {
        mu::IndexTuple sourceIter = targetIter;
        // Calculate the previous position of the dimension index and copy it to the current target dimension index
        sourceIter[shiftDim] = ((((int)sourceIter[shiftDim] - shiftSteps) % dimSize) + dimSize) % dimSize;

        shifted.get(targetIter) = A.get(sourceIter);
    } while (iterator.next());

    return shifted;
}









