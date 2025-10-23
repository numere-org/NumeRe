/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025 Erik Haenel et al.

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

#ifndef MATFUNCIMPLEMENTATION
#define MATFUNCIMPLEMENTATION

#include "../ParserLib/muParser.h"

mu::Array oprt_MatMul(const mu::Array& A, const mu::Array& B); // A ** B
mu::Array oprt_transpose(const mu::Array& A); // A'

mu::Array matfnc_matfc(const mu::MultiArgFuncParams& cols);
mu::Array matfnc_matfr(const mu::MultiArgFuncParams& rows);
mu::Array matfnc_matfcf(const mu::MultiArgFuncParams& cols);
mu::Array matfnc_matfrf(const mu::MultiArgFuncParams& rows);

mu::Array matfnc_det(const mu::Array& A);
mu::Array matfnc_cross(const mu::Array& A);
mu::Array matfnc_trace(const mu::Array& A);
mu::Array matfnc_eigenvals(const mu::Array& A);
mu::Array matfnc_eigenvects(const mu::Array& A);
mu::Array matfnc_diagonalize(const mu::Array& A);
mu::Array matfnc_invert(const mu::Array& A);
mu::Array matfnc_transpose(const mu::Array& A, const mu::Array& dims); // OPT=1
mu::Array matfnc_size(const mu::Array& A);
mu::Array matfnc_cutoff(const mu::Array& A, const mu::Array& threshold, const mu::Array& mode);
mu::Array matfnc_movsum(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movstd(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movavg(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movprd(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movmed(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movmin(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movmax(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movnorm(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_movnum(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_zero(const mu::MultiArgFuncParams& n);
mu::Array matfnc_one(const mu::MultiArgFuncParams& n);
mu::Array matfnc_identity(const mu::Array& n);
mu::Array matfnc_shuffle(const mu::Array& shuffle, const mu::Array& base); // OPT=1
mu::Array matfnc_correl(const mu::Array& A, const mu::Array& B);
mu::Array matfnc_covar(const mu::Array& A, const mu::Array& B);
mu::Array matfnc_normalize(const mu::Array& A);
mu::Array matfnc_reshape(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_resize(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_repmat(const mu::Array& A, const mu::Array& n, const mu::Array& m); // OPT=1
mu::Array matfnc_unique(const mu::Array& A, const mu::Array& dim); // OPT=1
mu::Array matfnc_cumsum(const mu::Array& A, const mu::Array& dim); // OPT=1
mu::Array matfnc_cumprd(const mu::Array& A, const mu::Array& dim); // OPT=1
mu::Array matfnc_solve(const mu::Array& A, const mu::Array& B);
mu::Array matfnc_diag(const mu::MultiArgFuncParams& diagonal);
mu::Array matfnc_carttocyl(const mu::Array& cartesian);
mu::Array matfnc_carttopol(const mu::Array& cartesian);
mu::Array matfnc_cyltocart(const mu::Array& cylindrical);
mu::Array matfnc_cyltopol(const mu::Array& cylindrical);
mu::Array matfnc_poltocart(const mu::Array& polar);
mu::Array matfnc_poltocyl(const mu::Array& polar);
mu::Array matfnc_coordstogrid(const mu::Array& grid, const mu::Array& coords);
mu::Array matfnc_interpolate(const mu::Array& grid, const mu::Array& coords);
mu::Array matfnc_hcat(const mu::Array& A, const mu::Array& B);
mu::Array matfnc_vcat(const mu::Array& A, const mu::Array& B);
mu::Array matfnc_select(const mu::Array& data, const mu::Array& rows, const mu::Array& cols);
mu::Array matfnc_assemble(const mu::Array& rows, const mu::Array& cols, const mu::Array& data);
mu::Array matfnc_polylength(const mu::MultiArgFuncParams& poly); // Either a single matrix or a list of vertices
mu::Array matfnc_filter(const mu::Array& data, const mu::Array& kernel, const mu::Array& mode);
mu::Array matfnc_circshift(const mu::Array& A, const mu::Array& steps, const mu::Array& dim); // OPT=1
mu::Array matfnc_vectshift(const mu::Array& A, const mu::Array& steps, const mu::Array& dim); // OPT=1

#endif // MATFUNCIMPLEMENTATION

