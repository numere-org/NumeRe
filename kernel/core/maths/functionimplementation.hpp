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
value_type parser_Mega(value_type);
value_type parser_Giga(value_type);
value_type parser_Kilo(value_type);
value_type parser_Milli(value_type);
value_type parser_Micro(value_type);
value_type parser_Nano(value_type);

// Unit conversion callback functions
value_type parser_ElectronVolt(value_type);
value_type parser_Fermi(value_type);
value_type parser_Angstroem(value_type);
value_type parser_Barn(value_type);
value_type parser_Torr(value_type);
value_type parser_AstroUnit(value_type);
value_type parser_Lightyear(value_type);
value_type parser_Parsec(value_type);
value_type parser_Mile(value_type);
value_type parser_Yard(value_type);
value_type parser_Foot(value_type);
value_type parser_Inch(value_type);
value_type parser_Calorie(value_type);
value_type parser_PSI(value_type);
value_type parser_Knoten(value_type);
value_type parser_liter(value_type);
value_type parser_kmh(value_type);
value_type parser_mph(value_type);
value_type parser_Celsius(value_type);
value_type parser_Fahrenheit(value_type);
value_type parser_Curie(value_type);
value_type parser_Gauss(value_type);
value_type parser_Poise(value_type);
value_type parser_mol(value_type);

// Imaginary unit
value_type parser_imaginaryUnit(value_type);
value_type parser_real(value_type);
value_type parser_imag(value_type);
value_type parser_rect2polar(value_type);
value_type parser_polar2rect(value_type);
value_type parser_conj(value_type);
value_type parser_complex(value_type, value_type);

// Actual math functions
value_type parser_Faculty(value_type);
value_type parser_doubleFaculty(value_type);
value_type parser_Binom(value_type, value_type);
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
value_type parser_round(value_type, value_type);
value_type parser_compare(const value_type*, int);
value_type parser_and(const value_type*, int);
value_type parser_or(const value_type*, int);
value_type parser_xor(const value_type*, int);
value_type parser_polynomial(const value_type*, int);
value_type parser_perlin(const value_type*, int);
value_type parser_toRadian(value_type);
value_type parser_toDegree(value_type);
value_type parser_SphericalHarmonics(value_type, value_type, value_type, value_type);
value_type parser_imSphericalHarmonics(value_type, value_type, value_type, value_type);
value_type parser_Zernike(value_type, value_type, value_type, value_type);
value_type parser_ZernikeRadial(int, int, value_type);
value_type parser_SinusCardinalis(value_type);
value_type parser_SphericalBessel(value_type, value_type);
value_type parser_SphericalNeumann(value_type, value_type);
value_type parser_LegendrePolynomial(value_type, value_type);
value_type parser_AssociatedLegendrePolynomial(value_type, value_type, value_type);
value_type parser_LaguerrePolynomial(value_type, value_type);
value_type parser_AssociatedLaguerrePolynomial(value_type, value_type, value_type);
value_type parser_HermitePolynomial(value_type, value_type);
value_type parser_BetheWeizsaecker(value_type, value_type);
value_type parser_Heaviside(value_type);
value_type parser_theta(value_type, value_type, value_type);
value_type parser_phi(value_type, value_type);
value_type parser_Random(value_type, value_type);
value_type parser_gRandom(value_type, value_type);
value_type parser_erf(value_type);
value_type parser_erfc(value_type);
value_type parser_gamma(value_type);
value_type parser_AiryA(value_type);
value_type parser_AiryB(value_type);
value_type parser_RegularCylBessel(value_type, value_type);
value_type parser_IrregularCylBessel(value_type, value_type);
value_type parser_EllipticF(value_type, value_type);
value_type parser_EllipticE(value_type, value_type);
value_type parser_EllipticP(value_type, value_type, value_type);
value_type parser_EllipticD(value_type, value_type, value_type);
value_type parser_beta(value_type, value_type);
value_type parser_zeta(value_type);
value_type parser_clausen(value_type);
value_type parser_digamma(value_type);
value_type parser_polygamma(value_type, value_type);
value_type parser_dilogarithm(value_type);
value_type parser_floor(value_type);
value_type parser_roof(value_type);
value_type parser_rect(value_type, value_type, value_type);
value_type parser_ivl(value_type, value_type, value_type, value_type, value_type);
value_type parser_studentFactor(value_type, value_type);
value_type parser_gcd(value_type, value_type);
value_type parser_lcm(value_type, value_type);
value_type parser_is_string(value_type);
value_type parser_time();
value_type parser_clock();
value_type parser_date(value_type, value_type);
value_type parser_isnan(value_type);
value_type parser_interval(value_type, value_type, value_type);
value_type parser_numereversion();
value_type parser_cot(value_type);
value_type parser_sleep(value_type);
value_type parser_log_b(value_type, value_type);

// Operator functions
value_type parser_Not(value_type);
value_type parser_Ignore(value_type);
value_type parser_Mod(value_type, value_type);
value_type parser_XOR(value_type, value_type);
value_type parser_BinOR(value_type, value_type);
value_type parser_BinAND(value_type, value_type);

// Numerical variable factory
value_type* parser_AddVariable(const char_type*, void*);


#endif

