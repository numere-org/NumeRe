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

#ifndef MATDATASTRUCTURES_HPP
#define MATDATASTRUCTURES_HPP

#include <vector>
#include <string>
#include <complex>
#include <utility>

#include "../ui/error.hpp"
#include "../utils/stringtools.hpp"

/////////////////////////////////////////////////
/// \brief This class defines a dynamic size 2D
/// matrix with a single 1D internal buffer. If
/// the internal buffer is used directly within
/// the parser, parallelisation is far more
/// common.
/////////////////////////////////////////////////
class Matrix
{
    private:
        mu::Array m_storage;       ///< The internal buffer
        size_t m_rows;                               ///< Number of rows
        size_t m_cols;                               ///< Number of columns
        bool m_transpose;                            ///< Is this Matrix transposed

        /////////////////////////////////////////////////
        /// \brief Unchecked method to access indexed
        /// elements
        ///
        /// \param i size_t
        /// \param j size_t
        /// \return mu::Value&
        ///
        /////////////////////////////////////////////////
        mu::Value& get(size_t i, size_t j)
        {
            return m_transpose ? m_storage[i*m_cols + j] : m_storage[i + j*m_rows];
        }

        /////////////////////////////////////////////////
        /// \brief Const declared overload to the indexed
        /// access.
        ///
        /// \param i size_t
        /// \param j size_t
        /// \return const mu::Value&
        ///
        /////////////////////////////////////////////////
        const mu::Value& get(size_t i, size_t j) const
        {
            return m_transpose ? m_storage[i*m_cols + j] : m_storage[i + j*m_rows];
        }

        /////////////////////////////////////////////////
        /// \brief Bakes the transposition by swapping
        /// the actual elements instead of changing the
        /// read order.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void bakeTransposition()
        {
            if (m_transpose)
            {
                mu::Array buffer(m_cols*m_rows);

                for (size_t i = 0; i < m_rows; i++)
                {
                    for (size_t j = 0; j < m_cols; j++)
                    {
                        buffer[i + j*m_rows] = m_storage[i*m_cols + j];
                    }
                }

                std::swap(buffer, m_storage);
                m_transpose = false;
            }
        }

    public:
        /////////////////////////////////////////////////
        /// \brief Empty matrix default constructor.
        /////////////////////////////////////////////////
        Matrix() : m_rows(0u), m_cols(0u), m_transpose(false) {}

        /////////////////////////////////////////////////
        /// \brief Fill constructor
        ///
        /// \param r size_t
        /// \param c size_t
        /// \param init const mu::Value&
        ///
        /////////////////////////////////////////////////
        Matrix(size_t r, size_t c = 1u, const mu::Value& init = NAN) : m_rows(r), m_cols(c), m_transpose(false)
        {
            m_storage.resize(r*c, init);
        }

        /////////////////////////////////////////////////
        /// \brief Matrix copy constructor.
        ///
        /// \param mat const Matrix&
        ///
        /////////////////////////////////////////////////
        Matrix(const Matrix& mat)
        {
            assign(mat);
        }

        // We do not declare the move constructor by ourselves
        Matrix(Matrix&& mat) = default;

        /////////////////////////////////////////////////
        /// \brief Construct a matrix from a vector
        /// matrix.
        ///
        /// \param vectMatrix const std::vector<std::vector<std::complex<double>>>&
        ///
        /////////////////////////////////////////////////
        Matrix(const std::vector<std::vector<std::complex<double>>>& vectMatrix) : Matrix()
        {
            m_rows = vectMatrix.size();
            m_cols = vectMatrix[0].size();

            for (size_t i = 1; i < m_rows; i++)
            {
                m_cols = std::max(m_cols, vectMatrix[i].size());
            }

            m_storage.resize(m_rows*m_cols, NAN);

            for (size_t i = 0; i < m_rows; i++)
            {
                for (size_t j = 0; j < vectMatrix[i].size(); j++)
                    get(i, j) = vectMatrix[i][j];
            }
        }

        /////////////////////////////////////////////////
        /// \brief Assign constructor from large array.
        ///
        /// \param r size_t
        /// \param c size_t
        /// \param vVals const mu::Array&
        ///
        /////////////////////////////////////////////////
        Matrix(size_t r, size_t c, const mu::Array& vVals)
        {
            assign(r, c, vVals);
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload.
        ///
        /// \param mat const Matrix&
        /// \return Matrix&
        ///
        /////////////////////////////////////////////////
        Matrix& operator=(const Matrix& mat)
        {
            assign(mat);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assign a matrix
        ///
        /// \param mat const Matrix&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const Matrix& mat)
        {
            m_rows = mat.m_rows;
            m_cols = mat.m_cols;
            m_storage = mat.m_storage;
            m_transpose = mat.m_transpose;
        }

        /////////////////////////////////////////////////
        /// \brief Assign a large array.
        ///
        /// \param r size_t
        /// \param c size_t
        /// \param vVals const mu::Array&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(size_t r, size_t c, const mu::Array& vVals)
        {
            m_storage.assign(vVals.begin(), vVals.begin()+std::min(r*c, vVals.size()));
            m_rows = r;
            m_cols = c;
            m_storage.resize(m_rows*m_cols);
            m_transpose = false;
        }

        /////////////////////////////////////////////////
        /// \brief Assign a vector.
        ///
        /// \param r size_t
        /// \param c size_t
        /// \param vVals const std::vector<std::complex<double>>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(size_t r, size_t c, const std::vector<std::complex<double>>& vVals)
        {
            m_storage.assign(vVals.begin(), vVals.begin()+std::min(r*c, vVals.size()));
            m_rows = r;
            m_cols = c;
            m_storage.resize(m_rows*m_cols);
            m_transpose = false;
        }

        /////////////////////////////////////////////////
        /// \brief Get a reference to the internal data
        /// structure.
        ///
        /// \return mu::Array&
        ///
        /////////////////////////////////////////////////
        mu::Array& data()
        {
            return m_storage;
        }

        /////////////////////////////////////////////////
        /// \brief Get a const reference to the internal
        /// data structure.
        ///
        /// \return const mu::Array&
        ///
        /////////////////////////////////////////////////
        const mu::Array& data() const
        {
            return m_storage;
        }

        /////////////////////////////////////////////////
        /// \brief Transpose this matrix.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void transpose()
        {
            std::swap(m_rows, m_cols);
            m_transpose = !m_transpose;
        }

        /////////////////////////////////////////////////
        /// \brief The rows of this matrix.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rows() const
        {
            return m_rows;
        }

        /////////////////////////////////////////////////
        /// \brief The cols of this matrix.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t cols() const
        {
            return m_cols;
        }

        /////////////////////////////////////////////////
        /// \brief Get both dimensions at once.
        ///
        /// \return std::pair<size_t,size_t>
        ///
        /////////////////////////////////////////////////
        std::pair<size_t,size_t> dims() const
        {
            return std::make_pair(m_rows, m_cols);
        }

        /////////////////////////////////////////////////
        /// \brief Check, whether this matrix is actually
        /// a scalar.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isScalar() const
        {
            return m_rows == 1 && m_cols == 1;
        }

        /////////////////////////////////////////////////
        /// \brief Simple function to identify square
        /// matrices.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isSquare() const
        {
            return m_rows == m_cols;
        }

        /////////////////////////////////////////////////
        /// \brief Simple function to identify empty
        /// matrices.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isEmpty() const
        {
            return !m_rows || !m_cols;
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if this matrix contains
        /// invalid values.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool containsInvalidValues() const
        {
            for (auto& v : m_storage)
            {
                if (!v.isValid())
                    return true;
            }

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Convert the dimensions to a printable
        /// string.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string printDims() const
        {
            return toString(m_rows) + "x" + toString(m_cols);
        }

        /////////////////////////////////////////////////
        /// \brief Get a reference to the indexed
        /// element.
        ///
        /// \param i size_t
        /// \param j size_t
        /// \return std::complex<double>&
        ///
        /////////////////////////////////////////////////
        std::complex<double>& operator()(size_t i, size_t j = 0u)
        {
            if (i >= m_rows || j >= m_cols)
                throw SyntaxError(SyntaxError::INVALID_INDEX, "INTERNAL INDEXING ERROR",
                                  SyntaxError::invalid_position,
                                  "MAT(" + toString(i) + "," + toString(j) + ") vs. size = " + printDims());

            return get(i, j).getNum().val;
        }

        /////////////////////////////////////////////////
        /// \brief Get a const reference to the indexed
        /// element.
        ///
        /// \param i size_t
        /// \param j size_t
        /// \return const std::complex<double>&
        ///
        /////////////////////////////////////////////////
        const std::complex<double>& operator()(size_t i, size_t j = 0u) const
        {
            if (i >= m_rows || j >= m_cols)
                throw SyntaxError(SyntaxError::INVALID_INDEX, "INTERNAL INDEXING ERROR",
                                  SyntaxError::invalid_position,
                                  "MAT(" + toString(i) + "," + toString(j) + ") vs. size = " + printDims());

            return get(i, j).getNum().val;
        }

        /////////////////////////////////////////////////
        /// \brief Resize the matrix to another
        /// dimension. New elements will be
        /// zero-initialized.
        ///
        /// \param r size_t
        /// \param c size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        void resize(size_t r, size_t c)
        {
            if (r == m_rows && c == m_cols)
                return;

            // Use a buffer
            mu::Array buffer(r*c);

            // Copy the remaining elements
            #pragma omp parallel for
            for (size_t i = 0; i < std::min(r, m_rows); i++)
            {
                for (size_t j = 0; j < std::min(c, m_cols); j++)
                {
                    buffer[i + j*r] = m_storage[i + j*m_rows];
                }
            }

            // Swap storage with the buffer
            std::swap(m_storage, buffer);

            // Update the sizes
            m_rows = r;
            m_cols = c;
        }

        /////////////////////////////////////////////////
        /// \brief Extend this matrix to be prepared for
        /// the acutal mathematical operation. Scalars
        /// are not touched and vectors are repmat()-ed
        /// to enable elementwise operations.
        ///
        /// \param r size_t
        /// \param c size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        void extend(size_t r, size_t c)
        {
            if (isScalar())
                return;

            // We need to bake the transposition
            bakeTransposition();

            if (r != m_rows || c != m_cols)
            {
                // Keep the relevant scalar dimensions
                bool rowScalar = m_rows == 1 && r > 1;
                bool colScalar = m_cols == 1 && c > 1;

                // Resize the matrix
                resize(r, c);

                if (rowScalar)
                {
                    for (size_t i = 1; i < m_rows; i++)
                    {
                        for (size_t j = 0; j < m_cols; j++)
                        {
                            get(i, j) = get(0, j);
                        }
                    }
                }
                else if (colScalar)
                {
                    for (size_t j = 1; j < m_cols; j++)
                    {
                        for (size_t i = 0; i < m_rows; i++)
                        {
                            get(i, j) = get(i, 0);
                        }
                    }
                }
            }
        }

        /////////////////////////////////////////////////
        /// \brief Multiply a matrix from the right to
        /// this matrix.
        ///
        /// \param mat const Matrix&
        /// \return Matrix&
        ///
        /////////////////////////////////////////////////
        Matrix& operator*=(const Matrix& mat)
        {
            assign(operator*(mat));
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Multiply this matrix with a matrix
        /// from the right.
        ///
        /// \param mat const Matrix&
        /// \return Matrix
        ///
        /////////////////////////////////////////////////
        Matrix operator*(const Matrix& mat) const
        {
            if (mat.m_rows != m_cols)
                throw SyntaxError(SyntaxError::WRONG_MATRIX_DIMENSIONS_FOR_MATOP, "INTERNAL INDEXING ERROR",
                                  SyntaxError::invalid_position,
                                  printDims() + " vs. " + mat.printDims());

            Matrix ret(m_rows, mat.m_cols, mu::Value(0.0));

            #pragma omp parallel for
            for (size_t i = 0; i < ret.m_rows; i++)
            {
                for (size_t j = 0; j < ret.m_cols; j++)
                {
                    for (size_t k = 0; k < m_cols; k++)
                    {
                        // Using the private method avoids addressing
                        // issues occuring with transposed matrices
                        ret.get(i, j) += get(i, k) * mat.get(k, j);
                    }
                }
            }

            return ret;
        }

        /////////////////////////////////////////////////
        /// \brief Horizontally concatenate two matrices.
        ///
        /// \param mat const Matrix&
        /// \return Matrix
        ///
        /////////////////////////////////////////////////
        Matrix hcat(const Matrix& mat) const
        {
            size_t cols = m_cols + mat.m_cols;
            size_t rows = std::max(m_rows, mat.m_rows);

            Matrix mRet(rows, cols);

            // Copy this matrix
            for (size_t i = 0; i < m_rows; i++)
            {
                for (size_t j = 0; j < m_cols; j++)
                {
                    mRet.get(i, j) = get(i, j);
                }
            }

            // Copy other matrix
            for (size_t i = 0; i < mat.m_rows; i++)
            {
                for (size_t j = 0; j < mat.m_cols; j++)
                {
                    mRet.get(i, j+m_cols) = mat.get(i, j);
                }
            }

            return mRet;
        }

        /////////////////////////////////////////////////
        /// \brief Vertically concatenate two matrices.
        ///
        /// \param mat const Matrix&
        /// \return Matrix
        ///
        /////////////////////////////////////////////////
        Matrix vcat(const Matrix& mat) const
        {
            size_t cols = std::max(m_cols, mat.m_cols);
            size_t rows = m_rows + mat.m_rows;

            Matrix mRet(rows, cols);

            // Copy this matrix
            for (size_t i = 0; i < m_rows; i++)
            {
                for (size_t j = 0; j < m_cols; j++)
                {
                    mRet.get(i, j) = get(i, j);
                }
            }

            // Copy other matrix
            for (size_t i = 0; i < mat.m_rows; i++)
            {
                for (size_t j = 0; j < mat.m_cols; j++)
                {
                    mRet.get(i+m_rows, j) = mat.get(i, j);
                }
            }

            return mRet;
        }
};



// Erster Index: No. of Line; zweiter Index: No. of Col (push_back verwendet dazu stets zeilen!)
/////////////////////////////////////////////////
/// \brief Defines a Matrix.
/////////////////////////////////////////////////
//typedef std::vector<std::vector<std::complex<double>> > Matrix;


/////////////////////////////////////////////////
/// \brief Defines the needed data for each
/// "standard" matrix function.
/////////////////////////////////////////////////
struct MatFuncData
{
    const Matrix& mat1;
    const Matrix& mat2;
    const Matrix& mat3;
    mu::Value fVal;
    int nVal;
    int mVal;

    MatFuncData() : mat1(Matrix()), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const MatFuncData& m) = default;
    MatFuncData(MatFuncData&& m) = default;
    MatFuncData(const Matrix& _mat, int n = 0) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(n), mVal(0) {}
    MatFuncData(const Matrix& _mat1, const Matrix& _mat2, int n) : mat1(_mat1), mat2(_mat2), mat3(Matrix()), fVal(NAN), nVal(n), mVal(0) {}
    MatFuncData(const Matrix& _mat1, const Matrix& _mat2) : mat1(_mat1), mat2(_mat2), mat3(Matrix()), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat1, const Matrix& _mat2, const Matrix& _mat3) : mat1(_mat1), mat2(_mat2), mat3(_mat3), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat, mu::Value f) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(f), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat, mu::Value f, int n) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(f), nVal(n), mVal(0) {}
    MatFuncData(const Matrix& _mat, int n, int m) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(n), mVal(m) {}
    MatFuncData(int n, int m = 0) : mat1(Matrix()), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(n), mVal(m) {}
};


/////////////////////////////////////////////////
/// \brief Defines the needed information for
/// displaying a reasonable error information.
/////////////////////////////////////////////////
struct MatFuncErrorInfo
{
    const std::string& command;
    const std::string& expression;
    size_t position;

    MatFuncErrorInfo(const std::string& _sCmd, const std::string& _sExpr, size_t pos) : command(_sCmd), expression(_sExpr), position(pos) {}
};


/////////////////////////////////////////////////
/// \brief Defines the MatFunc type
/////////////////////////////////////////////////
typedef Matrix (*MatFunc)(const MatFuncData&, const MatFuncErrorInfo&);


/////////////////////////////////////////////////
/// \brief Defines the different matrix function
/// signatures.
/////////////////////////////////////////////////
enum MatFuncSignature
{
    MATSIG_INVALID,
    MATSIG_MAT,
    MATSIG_MAT_NOPT,
    MATSIG_MAT_MAT,
    MATSIG_MAT_MAT_MAT,
    MATSIG_MAT_MAT_N,
    MATSIG_MAT_F,
    MATSIG_MAT_F_N,
    MATSIG_MAT_N_MAUTO,
    MATSIG_MAT_N_MOPT,
    MATSIG_N_MAUTO,
    MATSIG_N_MOPT
};


/////////////////////////////////////////////////
/// \brief Defines a matrix function itself by
/// containing the signature, the pointer to the
/// implementation and an information, whether
/// it is a pure matrix function (or an overload).
/////////////////////////////////////////////////
struct MatFuncDef
{
    MatFuncSignature signature;
    MatFunc func;
    bool isPureMatFunc;

    MatFuncDef() : signature(MATSIG_INVALID), func(nullptr), isPureMatFunc(false) {}
    MatFuncDef(MatFuncSignature sig, MatFunc f, bool isMatFunc = true) : signature(sig), func(f), isPureMatFunc(isMatFunc) {}
};

#endif // MATDATASTRUCTURES_HPP

