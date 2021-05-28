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


// Erster Index: No. of Line; zweiter Index: No. of Col (push_back verwendet dazu stets zeilen!)
/////////////////////////////////////////////////
/// \brief Defines a Matrix.
/////////////////////////////////////////////////
typedef std::vector<std::vector<double> > Matrix;


/////////////////////////////////////////////////
/// \brief Defines the needed data for each
/// "standard" matrix function.
/////////////////////////////////////////////////
struct MatFuncData
{
    const Matrix& mat1;
    const Matrix& mat2;
    const Matrix& mat3;
    double fVal;
    int nVal;
    int mVal;

    MatFuncData() : mat1(Matrix()), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat, int n = 0) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(NAN), nVal(n), mVal(0) {}
    MatFuncData(const Matrix& _mat1, const Matrix& _mat2) : mat1(_mat1), mat2(_mat2), mat3(Matrix()), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat1, const Matrix& _mat2, const Matrix& _mat3) : mat1(_mat1), mat2(_mat2), mat3(_mat3), fVal(NAN), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat, double f) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(f), nVal(0), mVal(0) {}
    MatFuncData(const Matrix& _mat, double f, int n) : mat1(_mat), mat2(Matrix()), mat3(Matrix()), fVal(f), nVal(n), mVal(0) {}
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
    MATSIG_MAT_F,
    MATSIG_MAT_F_N,
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

