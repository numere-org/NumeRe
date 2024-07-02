/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


/*
 * Deklarationen saemtlicher Parser-Funktionen
 */
#ifndef FUNCTIONIMPLEMENTATION_HPP
#define FUNCTIONIMPLEMENTATION_HPP

#include "../ParserLib/muParser.h"
#include "units.hpp"

// Imaginary unit
mu::Array parser_imaginaryUnit(const mu::Array&);
mu::Array parser_real(const mu::Array&);
mu::Array parser_imag(const mu::Array&);
mu::Array parser_rect2polar(const mu::Array&);
mu::Array parser_polar2rect(const mu::Array&);
mu::Array parser_conj(const mu::Array&);
mu::Array parser_complex(const mu::Array&, const mu::Array&);

// Actual math functions
mu::Array parser_Faculty(const mu::Array&);
mu::Array parser_doubleFaculty(const mu::Array&);
mu::Array parser_Binom(const mu::Array&, const mu::Array&);
mu::Array parser_Sum(const mu::Array*, int);
mu::Array parser_Avg(const mu::Array*, int);
mu::Array parser_Min(const mu::Array*, int);
mu::Array parser_Max(const mu::Array*, int);
mu::Array parser_MinPos(const mu::Array&);
mu::Array parser_MaxPos(const mu::Array&);
mu::Array parser_Num(const mu::Array*, int);
mu::Array parser_Cnt(const mu::Array*, int);
mu::Array parser_Std(const mu::Array*, int);
mu::Array parser_product(const mu::Array*, int);
mu::Array parser_Norm(const mu::Array*, int);
mu::Array parser_Med(const mu::Array*, int);
mu::Array parser_Pct(const mu::Array*, int);
mu::Array parser_round(const mu::Array&, const mu::Array&);
mu::Array parser_compare(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_and(const mu::Array*, int);
mu::Array parser_or(const mu::Array*, int);
mu::Array parser_xor(const mu::Array*, int);
mu::Array parser_polynomial(const mu::Array*, int);
mu::Array parser_perlin(const mu::Array*, int);
mu::Array parser_toRadian(const mu::Array&);
mu::Array parser_toDegree(const mu::Array&);
mu::Array parser_SphericalHarmonics(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_imSphericalHarmonics(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_Zernike(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_SinusCardinalis(const mu::Array&);
mu::Array parser_SphericalBessel(const mu::Array&, const mu::Array&);
mu::Array parser_SphericalNeumann(const mu::Array&, const mu::Array&);
mu::Array parser_LegendrePolynomial(const mu::Array&, const mu::Array&);
mu::Array parser_AssociatedLegendrePolynomial(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_LaguerrePolynomial(const mu::Array&, const mu::Array&);
mu::Array parser_AssociatedLaguerrePolynomial(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_HermitePolynomial(const mu::Array&, const mu::Array&);
mu::Array parser_BetheWeizsaecker(const mu::Array&, const mu::Array&);
mu::Array parser_Heaviside(const mu::Array&);
mu::Array parser_theta(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_phi(const mu::Array&, const mu::Array&);
mu::Array parser_Random(const mu::Array&, const mu::Array&);
mu::Array parser_gRandom(const mu::Array&, const mu::Array&);
mu::Array parser_erf(const mu::Array&);
mu::Array parser_erfc(const mu::Array&);
mu::Array parser_gamma(const mu::Array&);
mu::Array parser_AiryA(const mu::Array&);
mu::Array parser_AiryB(const mu::Array&);
mu::Array parser_RegularCylBessel(const mu::Array&, const mu::Array&);
mu::Array parser_IrregularCylBessel(const mu::Array&, const mu::Array&);
mu::Array parser_EllipticF(const mu::Array&, const mu::Array&);
mu::Array parser_EllipticE(const mu::Array&, const mu::Array&);
mu::Array parser_EllipticP(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_EllipticD(const mu::Array&, const mu::Array&);
mu::Array parser_beta(const mu::Array&, const mu::Array&);
mu::Array parser_zeta(const mu::Array&);
mu::Array parser_clausen(const mu::Array&);
mu::Array parser_digamma(const mu::Array&);
mu::Array parser_polygamma(const mu::Array&, const mu::Array&);
mu::Array parser_dilogarithm(const mu::Array&);
mu::Array parser_floor(const mu::Array&);
mu::Array parser_roof(const mu::Array&);
mu::Array parser_rect(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_ivl(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_studentFactor(const mu::Array&, const mu::Array&);
mu::Array parser_gcd(const mu::Array&, const mu::Array&);
mu::Array parser_lcm(const mu::Array&, const mu::Array&);
mu::Array parser_is_string(const mu::Array&);
mu::Array parser_time();
mu::Array parser_clock();
mu::Array parser_date(const mu::Array&, const mu::Array&);
mu::Array parser_weeknum(const mu::Array&);
mu::Array parser_isnan(const mu::Array&);
mu::Array parser_interval(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array parser_numereversion();
mu::Array parser_omp_threads();
mu::Array parser_cot(const mu::Array&);
mu::Array parser_sleep(const mu::Array&);
mu::Array parser_log_b(const mu::Array&, const mu::Array&);
mu::Array parser_sec(const mu::Array&);
mu::Array parser_csc(const mu::Array&);
mu::Array parser_asec(const mu::Array&);
mu::Array parser_acsc(const mu::Array&);
mu::Array parser_sech(const mu::Array&);
mu::Array parser_csch(const mu::Array&);
mu::Array parser_asech(const mu::Array&);
mu::Array parser_acsch(const mu::Array&);
mu::Array parser_as_date(const mu::Array*, int);
mu::Array parser_as_time(const mu::Array*, int);

// Distribution functions
mu::Array parser_rd_laplace_rd(const mu::Array& a);
mu::Array parser_rd_laplace_pdf(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_laplace_cdf_p(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_laplace_cdf_q(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_laplace_inv_p(const mu::Array& p, const mu::Array& a);
mu::Array parser_rd_laplace_inv_q(const mu::Array& q, const mu::Array& a);
mu::Array parser_rd_cauchy_rd(const mu::Array& a);
mu::Array parser_rd_cauchy_pdf(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_cauchy_cdf_p(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_cauchy_cdf_q(const mu::Array& x, const mu::Array& a);
mu::Array parser_rd_cauchy_inv_p(const mu::Array& p, const mu::Array& a);
mu::Array parser_rd_cauchy_inv_q(const mu::Array& q, const mu::Array& a);
mu::Array parser_rd_rayleigh_rd(const mu::Array& sigma);
mu::Array parser_rd_rayleigh_pdf(const mu::Array& x, const mu::Array& sigma);
mu::Array parser_rd_rayleigh_cdf_p(const mu::Array& x, const mu::Array& sigma);
mu::Array parser_rd_rayleigh_cdf_q(const mu::Array& x, const mu::Array& sigma);
mu::Array parser_rd_rayleigh_inv_p(const mu::Array& p, const mu::Array& sigma);
mu::Array parser_rd_rayleigh_inv_q(const mu::Array& q, const mu::Array& sigma);
mu::Array parser_rd_landau_rd();
mu::Array parser_rd_landau_pdf(const mu::Array& x);
mu::Array parser_rd_levyAlphaStable_rd(const mu::Array& c, const mu::Array& alpha);
mu::Array parser_rd_fisher_f_rd(const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_fisher_f_pdf(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_fisher_f_cdf_p(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_fisher_f_cdf_q(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_fisher_f_inv_p(const mu::Array& p, const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_fisher_f_inv_q(const mu::Array& q, const mu::Array& nu1, const mu::Array& nu2);
mu::Array parser_rd_weibull_rd(const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_weibull_pdf(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_weibull_cdf_p(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_weibull_cdf_q(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_weibull_inv_p(const mu::Array& p, const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_weibull_inv_q(const mu::Array& q, const mu::Array& a, const mu::Array& b);
mu::Array parser_rd_student_t_rd(const mu::Array& nu);
mu::Array parser_rd_student_t_pdf(const mu::Array& x, const mu::Array& nu);
mu::Array parser_rd_student_t_cdf_p(const mu::Array& x, const mu::Array& nu);
mu::Array parser_rd_student_t_cdf_q(const mu::Array& x, const mu::Array& nu);
mu::Array parser_rd_student_t_inv_p(const mu::Array& p, const mu::Array& nu);
mu::Array parser_rd_student_t_inv_q(const mu::Array& q, const mu::Array& nu);

// Operator functions
mu::Array parser_Mod(const mu::Array&, const mu::Array&);
mu::Array parser_XOR(const mu::Array&, const mu::Array&);
mu::Array parser_BinOR(const mu::Array&, const mu::Array&);
mu::Array parser_BinAND(const mu::Array&, const mu::Array&);

// Numerical variable factory
mu::Array* parser_AddVariable(const mu::char_type*, void*);


#endif

