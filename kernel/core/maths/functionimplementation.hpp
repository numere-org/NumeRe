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

using namespace mu;

// Scaling operator callback functions
value_type parser_Mega(const value_type&);
value_type parser_Giga(const value_type&);
value_type parser_Kilo(const value_type&);
value_type parser_Milli(const value_type&);
value_type parser_Micro(const value_type&);
value_type parser_Nano(const value_type&);

// Unit conversion callback functions
value_type parser_ElectronVolt(const value_type&);
value_type parser_Fermi(const value_type&);
value_type parser_Angstroem(const value_type&);
value_type parser_Barn(const value_type&);
value_type parser_Torr(const value_type&);
value_type parser_AstroUnit(const value_type&);
value_type parser_Lightyear(const value_type&);
value_type parser_Parsec(const value_type&);
value_type parser_Mile(const value_type&);
value_type parser_Yard(const value_type&);
value_type parser_Foot(const value_type&);
value_type parser_Inch(const value_type&);
value_type parser_Calorie(const value_type&);
value_type parser_PSI(const value_type&);
value_type parser_Knoten(const value_type&);
value_type parser_liter(const value_type&);
value_type parser_kmh(const value_type&);
value_type parser_mph(const value_type&);
value_type parser_Celsius(const value_type&);
value_type parser_Fahrenheit(const value_type&);
value_type parser_Curie(const value_type&);
value_type parser_Gauss(const value_type&);
value_type parser_Poise(const value_type&);
value_type parser_mol(const value_type&);

// Imaginary unit
value_type parser_imaginaryUnit(const value_type&);
value_type parser_real(const value_type&);
value_type parser_imag(const value_type&);
value_type parser_rect2polar(const value_type&);
value_type parser_polar2rect(const value_type&);
value_type parser_conj(const value_type&);
value_type parser_complex(const value_type&, const value_type&);

// Actual math functions
value_type parser_Faculty(const value_type&);
value_type parser_doubleFaculty(const value_type&);
value_type parser_Binom(const value_type&, const value_type&);
value_type parser_Sum(const value_type*, int);
value_type parser_Avg(const value_type*, int);
value_type parser_Min(const value_type*, int);
value_type parser_Max(const value_type*, int);
value_type parser_MinPos(const value_type*, int);
value_type parser_MaxPos(const value_type*, int);
value_type parser_Num(const value_type*, int);
value_type parser_Cnt(const value_type*, int);
value_type parser_Std(const value_type*, int);
value_type parser_product(const value_type*, int);
value_type parser_Norm(const value_type*, int);
value_type parser_Med(const value_type*, int);
value_type parser_Pct(const value_type*, int);
value_type parser_round(const value_type&, const value_type&);
value_type parser_compare(const value_type*, int);
value_type parser_and(const value_type*, int);
value_type parser_or(const value_type*, int);
value_type parser_xor(const value_type*, int);
value_type parser_polynomial(const value_type*, int);
value_type parser_perlin(const value_type*, int);
value_type parser_toRadian(const value_type&);
value_type parser_toDegree(const value_type&);
value_type parser_SphericalHarmonics(const value_type&, const value_type&, const value_type&, const value_type&);
value_type parser_imSphericalHarmonics(const value_type&, const value_type&, const value_type&, const value_type&);
value_type parser_Zernike(const value_type&, const value_type&, const value_type&, const value_type&);
value_type parser_ZernikeRadial(int, int, const value_type&);
value_type parser_SinusCardinalis(const value_type&);
value_type parser_SphericalBessel(const value_type&, const value_type&);
value_type parser_SphericalNeumann(const value_type&, const value_type&);
value_type parser_LegendrePolynomial(const value_type&, const value_type&);
value_type parser_AssociatedLegendrePolynomial(const value_type&, const value_type&, const value_type&);
value_type parser_LaguerrePolynomial(const value_type&, const value_type&);
value_type parser_AssociatedLaguerrePolynomial(const value_type&, const value_type&, const value_type&);
value_type parser_HermitePolynomial(const value_type&, const value_type&);
value_type parser_BetheWeizsaecker(const value_type&, const value_type&);
value_type parser_Heaviside(const value_type&);
value_type parser_theta(const value_type&, const value_type&, const value_type&);
value_type parser_phi(const value_type&, const value_type&);
value_type parser_Random(const value_type&, const value_type&);
value_type parser_gRandom(const value_type&, const value_type&);
value_type parser_erf(const value_type&);
value_type parser_erfc(const value_type&);
value_type parser_gamma(const value_type&);
value_type parser_AiryA(const value_type&);
value_type parser_AiryB(const value_type&);
value_type parser_RegularCylBessel(const value_type&, const value_type&);
value_type parser_IrregularCylBessel(const value_type&, const value_type&);
value_type parser_EllipticF(const value_type&, const value_type&);
value_type parser_EllipticE(const value_type&, const value_type&);
value_type parser_EllipticP(const value_type&, const value_type&, const value_type&);
value_type parser_EllipticD(const value_type&, const value_type&, const value_type&);
value_type parser_beta(const value_type&, const value_type&);
value_type parser_zeta(const value_type&);
value_type parser_clausen(const value_type&);
value_type parser_digamma(const value_type&);
value_type parser_polygamma(const value_type&, const value_type&);
value_type parser_dilogarithm(const value_type&);
value_type parser_floor(const value_type&);
value_type parser_roof(const value_type&);
value_type parser_rect(const value_type&, const value_type&, const value_type&);
value_type parser_ivl(const value_type&, const value_type&, const value_type&, const value_type&, const value_type&);
value_type parser_studentFactor(const value_type&, const value_type&);
value_type parser_gcd(const value_type&, const value_type&);
value_type parser_lcm(const value_type&, const value_type&);
value_type parser_is_string(const value_type&);
value_type parser_time();
value_type parser_clock();
value_type parser_date(const value_type&, const value_type&);
value_type parser_weeknum(const value_type&);
value_type parser_isnan(const value_type&);
value_type parser_interval(const value_type&, const value_type&, const value_type&);
value_type parser_numereversion();
value_type parser_cot(const value_type&);
value_type parser_sleep(const value_type&);
value_type parser_log_b(const value_type&, const value_type&);
value_type parser_sec(const value_type&);
value_type parser_csc(const value_type&);
value_type parser_asec(const value_type&);
value_type parser_acsc(const value_type&);
value_type parser_sech(const value_type&);
value_type parser_csch(const value_type&);
value_type parser_asech(const value_type&);
value_type parser_acsch(const value_type&);

// Distribution functions
value_type parser_rd_laplace_rd(const value_type& sigma);
value_type parser_rd_laplace_cdf_p(const value_type& x, const value_type& sigma);
value_type parser_rd_laplace_cdf_q(const value_type& x, const value_type& sigma);
value_type parser_rd_laplace_inv_p(const value_type& p, const value_type& sigma);
value_type parser_rd_laplace_inv_q(const value_type& q, const value_type& sigma);
value_type parser_rd_cauchy_rd(const value_type& a);
value_type parser_rd_cauchy_cdf_p(const value_type& x, const value_type& a);
value_type parser_rd_cauchy_cdf_q(const value_type& x, const value_type& a);
value_type parser_rd_cauchy_inv_p(const value_type& p, const value_type& a);
value_type parser_rd_cauchy_inv_q(const value_type& q, const value_type& a);
value_type parser_rd_rayleigh_rd(const value_type& sigma);
value_type parser_rd_rayleigh_cdf_p(const value_type& x, const value_type& sigma);
value_type parser_rd_rayleigh_cdf_q(const value_type& x, const value_type& sigma);
value_type parser_rd_rayleigh_inv_p(const value_type& p, const value_type& sigma);
value_type parser_rd_rayleigh_inv_q(const value_type& q, const value_type& sigma);
value_type parser_rd_landau_rd();
value_type parser_rd_levyAlphaStable_rd(const value_type& c, const value_type& alpha);
value_type parser_rd_fisher_f_rd(const value_type& nu1, const value_type& nu2);
value_type parser_rd_fisher_f_cdf_p(const value_type& x, const value_type& nu1, const value_type& nu2);
value_type parser_rd_fisher_f_cdf_q(const value_type& x, const value_type& nu1, const value_type& nu2);
value_type parser_rd_fisher_f_inv_p(const value_type& p, const value_type& nu1, const value_type& nu2);
value_type parser_rd_fisher_f_inv_q(const value_type& q, const value_type& nu1, const value_type& nu2);
value_type parser_rd_student_t_rd(const value_type& nu);
value_type parser_rd_student_t_cdf_p(const value_type& x, const value_type& nu);
value_type parser_rd_student_t_cdf_q(const value_type& x, const value_type& nu);
value_type parser_rd_student_t_inv_p(const value_type& p, const value_type& nu);
value_type parser_rd_student_t_inv_q(const value_type& q, const value_type& nu);

// Operator functions
value_type parser_Not(const value_type&);
value_type parser_Ignore(const value_type&);
value_type parser_Mod(const value_type&, const value_type&);
value_type parser_XOR(const value_type&, const value_type&);
value_type parser_BinOR(const value_type&, const value_type&);
value_type parser_BinAND(const value_type&, const value_type&);

// Numerical variable factory
value_type* parser_AddVariable(const char_type*, void*);


#endif

