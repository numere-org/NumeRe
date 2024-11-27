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
#include "../utils/stringtools.hpp"


// Index selectors
mu::Array numfnc_getElements(const mu::Array& a, const mu::Array& idx);

// Imaginary unit
mu::Array numfnc_imaginaryUnit(const mu::Array&);

// Actual math functions
mu::Array numfnc_real(const mu::Array&);
mu::Array numfnc_imag(const mu::Array&);
mu::Array numfnc_rect2polar(const mu::Array&);
mu::Array numfnc_polar2rect(const mu::Array&);
mu::Array numfnc_conj(const mu::Array&);
mu::Array numfnc_complex(const mu::Array&, const mu::Array&);
mu::Array numfnc_Factorial(const mu::Array&);
mu::Array numfnc_doubleFactorial(const mu::Array&);
mu::Array numfnc_Binom(const mu::Array&, const mu::Array&);
mu::Array numfnc_Sum(const mu::Array*, int);
mu::Array numfnc_Avg(const mu::Array*, int);
mu::Array numfnc_Min(const mu::Array*, int);
mu::Array numfnc_Max(const mu::Array*, int);
mu::Array numfnc_MinPos(const mu::Array&);
mu::Array numfnc_MaxPos(const mu::Array&);
mu::Array numfnc_Num(const mu::Array*, int);
mu::Array numfnc_Cnt(const mu::Array*, int);
mu::Array numfnc_Std(const mu::Array*, int);
mu::Array numfnc_product(const mu::Array*, int);
mu::Array numfnc_Norm(const mu::Array*, int);
mu::Array numfnc_Rms(const mu::Array*, int);
mu::Array numfnc_StdErr(const mu::Array*, int);
mu::Array numfnc_Skew(const mu::Array*, int);
mu::Array numfnc_Exc(const mu::Array*, int);
mu::Array numfnc_Med(const mu::Array*, int);
mu::Array numfnc_Pct(const mu::Array*, int);
mu::Array numfnc_rint(const mu::Array&);
mu::Array numfnc_round(const mu::Array&, const mu::Array&);
mu::Array numfnc_compare(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_and(const mu::Array*, int);
mu::Array numfnc_or(const mu::Array*, int);
mu::Array numfnc_xor(const mu::Array*, int);
mu::Array numfnc_polynomial(const mu::Array*, int);
mu::Array numfnc_logtoidx(const mu::Array*, int);
mu::Array numfnc_idxtolog(const mu::Array*, int);
mu::Array numfnc_order(const mu::Array*, int);
mu::Array numfnc_is_equal(const mu::Array*, int);
mu::Array numfnc_is_ordered(const mu::Array*, int);
mu::Array numfnc_is_unique(const mu::Array*, int);
mu::Array numfnc_pct_inv(const mu::Array& arr, const mu::Array& pct);
mu::Array numfnc_toRadian(const mu::Array&);
mu::Array numfnc_toDegree(const mu::Array&);
mu::Array numfnc_SphericalHarmonics(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_imSphericalHarmonics(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_Zernike(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_SphericalBessel(const mu::Array&, const mu::Array&);
mu::Array numfnc_SphericalNeumann(const mu::Array&, const mu::Array&);
mu::Array numfnc_LegendrePolynomial(const mu::Array&, const mu::Array&);
mu::Array numfnc_AssociatedLegendrePolynomial(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_LaguerrePolynomial(const mu::Array&, const mu::Array&);
mu::Array numfnc_AssociatedLaguerrePolynomial(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_HermitePolynomial(const mu::Array&, const mu::Array&);
mu::Array numfnc_BetheWeizsaecker(const mu::Array&, const mu::Array&);
mu::Array numfnc_Heaviside(const mu::Array&);
mu::Array numfnc_theta(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_phi(const mu::Array&, const mu::Array&);
mu::Array numfnc_erf(const mu::Array&);
mu::Array numfnc_erfc(const mu::Array&);
mu::Array numfnc_gamma(const mu::Array&);
mu::Array numfnc_AiryA(const mu::Array&);
mu::Array numfnc_AiryB(const mu::Array&);
mu::Array numfnc_RegularCylBessel(const mu::Array&, const mu::Array&);
mu::Array numfnc_IrregularCylBessel(const mu::Array&, const mu::Array&);
mu::Array numfnc_EllipticF(const mu::Array&, const mu::Array&);
mu::Array numfnc_EllipticE(const mu::Array&, const mu::Array&);
mu::Array numfnc_EllipticP(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_EllipticD(const mu::Array&, const mu::Array&);
mu::Array numfnc_beta(const mu::Array&, const mu::Array&);
mu::Array numfnc_zeta(const mu::Array&);
mu::Array numfnc_clausen(const mu::Array&);
mu::Array numfnc_digamma(const mu::Array&);
mu::Array numfnc_polygamma(const mu::Array&, const mu::Array&);
mu::Array numfnc_dilogarithm(const mu::Array&);
mu::Array numfnc_floor(const mu::Array&);
mu::Array numfnc_roof(const mu::Array&);
mu::Array numfnc_rect(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_ivl(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_studentFactor(const mu::Array&, const mu::Array&);
mu::Array numfnc_gcd(const mu::Array&, const mu::Array&);
mu::Array numfnc_lcm(const mu::Array&, const mu::Array&);
mu::Array numfnc_is_string(const mu::Array&);
mu::Array numfnc_isnan(const mu::Array&);
mu::Array numfnc_isvoid(const mu::Array&);
mu::Array numfnc_interval(const mu::Array&, const mu::Array&, const mu::Array&);
mu::Array numfnc_numereversion();
mu::Array numfnc_omp_threads();
mu::Array numfnc_pixelscale();
mu::Array numfnc_sleep(const mu::Array&);
mu::Array numfnc_exp(const mu::Array&);
mu::Array numfnc_abs(const mu::Array&);
mu::Array numfnc_sqrt(const mu::Array&);
mu::Array numfnc_sign(const mu::Array&);
mu::Array numfnc_log2(const mu::Array&);
mu::Array numfnc_log10(const mu::Array&);
mu::Array numfnc_ln(const mu::Array&);
mu::Array numfnc_log_b(const mu::Array&, const mu::Array&);

mu::Array numfnc_SinusCardinalis(const mu::Array&);
mu::Array numfnc_sin(const mu::Array&);
mu::Array numfnc_cos(const mu::Array&);
mu::Array numfnc_tan(const mu::Array&);
mu::Array numfnc_cot(const mu::Array&);
mu::Array numfnc_asin(const mu::Array&);
mu::Array numfnc_acos(const mu::Array&);
mu::Array numfnc_atan(const mu::Array&);
mu::Array numfnc_sinh(const mu::Array&);
mu::Array numfnc_cosh(const mu::Array&);
mu::Array numfnc_tanh(const mu::Array&);
mu::Array numfnc_asinh(const mu::Array&);
mu::Array numfnc_acosh(const mu::Array&);
mu::Array numfnc_atanh(const mu::Array&);
mu::Array numfnc_sec(const mu::Array&);
mu::Array numfnc_csc(const mu::Array&);
mu::Array numfnc_asec(const mu::Array&);
mu::Array numfnc_acsc(const mu::Array&);
mu::Array numfnc_sech(const mu::Array&);
mu::Array numfnc_csch(const mu::Array&);
mu::Array numfnc_asech(const mu::Array&);
mu::Array numfnc_acsch(const mu::Array&);
mu::Array timfnc_time();
mu::Array timfnc_clock();
mu::Array timfnc_today();
mu::Array timfnc_date(const mu::Array&, const mu::Array&);
mu::Array timfnc_weeknum(const mu::Array&);
mu::Array timfnc_as_date(const mu::Array&, const mu::Array&, const mu::Array&); // OPT=2
mu::Array timfnc_as_time(const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&, const mu::Array&); // OPT=4
mu::Array timfnc_get_utc_offset();
mu::Array timfnc_is_daylightsavingtime(const mu::Array&);
mu::Array timfnc_is_leap_year(const mu::Array&);

// Distribution functions
mu::Array rndfnc_perlin(const mu::Array& x, const mu::Array& y, const mu::Array& z, const mu::Array& seed, const mu::Array& freq, const mu::Array& octave, const mu::Array& persistence); // OPT=6
mu::Array rndfnc_rigedmultifractal(const mu::Array& x, const mu::Array& y, const mu::Array& z, const mu::Array& seed, const mu::Array& freq, const mu::Array& octave); // OPT=5
mu::Array rndfnc_billow(const mu::Array& x, const mu::Array& y, const mu::Array& z, const mu::Array& seed, const mu::Array& freq, const mu::Array& octave, const mu::Array& persistence); // OPT=6
mu::Array rndfnc_voronoi(const mu::Array& x, const mu::Array& y, const mu::Array& z, const mu::Array& seed, const mu::Array& freq, const mu::Array& displacement, const mu::Array& usedistance); // OPT=6
mu::Array rndfnc_Random(const mu::Array& xmin, const mu::Array& xmax, const mu::Array& n); // OPT=1
mu::Array rndfnc_gRandom(const mu::Array& avg, const mu::Array& stdev, const mu::Array& n); // OPT=1
mu::Array rndfnc_laplace_rd(const mu::Array& a, const mu::Array& n); // OPT=1
mu::Array rndfnc_laplace_pdf(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_laplace_cdf_p(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_laplace_cdf_q(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_laplace_inv_p(const mu::Array& p, const mu::Array& a);
mu::Array rndfnc_laplace_inv_q(const mu::Array& q, const mu::Array& a);
mu::Array rndfnc_cauchy_rd(const mu::Array& a, const mu::Array& n); // OPT=1
mu::Array rndfnc_cauchy_pdf(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_cauchy_cdf_p(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_cauchy_cdf_q(const mu::Array& x, const mu::Array& a);
mu::Array rndfnc_cauchy_inv_p(const mu::Array& p, const mu::Array& a);
mu::Array rndfnc_cauchy_inv_q(const mu::Array& q, const mu::Array& a);
mu::Array rndfnc_rayleigh_rd(const mu::Array& sigma, const mu::Array& n); // OPT=1
mu::Array rndfnc_rayleigh_pdf(const mu::Array& x, const mu::Array& sigma);
mu::Array rndfnc_rayleigh_cdf_p(const mu::Array& x, const mu::Array& sigma);
mu::Array rndfnc_rayleigh_cdf_q(const mu::Array& x, const mu::Array& sigma);
mu::Array rndfnc_rayleigh_inv_p(const mu::Array& p, const mu::Array& sigma);
mu::Array rndfnc_rayleigh_inv_q(const mu::Array& q, const mu::Array& sigma);
mu::Array rndfnc_landau_rd(const mu::Array& n); // OPT=1
mu::Array rndfnc_landau_pdf(const mu::Array& x);
mu::Array rndfnc_levyAlphaStable_rd(const mu::Array& c, const mu::Array& alpha, const mu::Array& n); // OPT=1
mu::Array rndfnc_fisher_f_rd(const mu::Array& nu1, const mu::Array& nu2, const mu::Array& n); // OPT=1
mu::Array rndfnc_fisher_f_pdf(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array rndfnc_fisher_f_cdf_p(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array rndfnc_fisher_f_cdf_q(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2);
mu::Array rndfnc_fisher_f_inv_p(const mu::Array& p, const mu::Array& nu1, const mu::Array& nu2);
mu::Array rndfnc_fisher_f_inv_q(const mu::Array& q, const mu::Array& nu1, const mu::Array& nu2);
mu::Array rndfnc_weibull_rd(const mu::Array& a, const mu::Array& b, const mu::Array& n); // OPT=1
mu::Array rndfnc_weibull_pdf(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array rndfnc_weibull_cdf_p(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array rndfnc_weibull_cdf_q(const mu::Array& x, const mu::Array& a, const mu::Array& b);
mu::Array rndfnc_weibull_inv_p(const mu::Array& p, const mu::Array& a, const mu::Array& b);
mu::Array rndfnc_weibull_inv_q(const mu::Array& q, const mu::Array& a, const mu::Array& b);
mu::Array rndfnc_student_t_rd(const mu::Array& nu, const mu::Array& n); // OPT=1
mu::Array rndfnc_student_t_pdf(const mu::Array& x, const mu::Array& nu);
mu::Array rndfnc_student_t_cdf_p(const mu::Array& x, const mu::Array& nu);
mu::Array rndfnc_student_t_cdf_q(const mu::Array& x, const mu::Array& nu);
mu::Array rndfnc_student_t_inv_p(const mu::Array& p, const mu::Array& nu);
mu::Array rndfnc_student_t_inv_q(const mu::Array& q, const mu::Array& nu);

// Operator functions
mu::Array oprt_Mod(const mu::Array&, const mu::Array&);
mu::Array oprt_XOR(const mu::Array&, const mu::Array&);
mu::Array oprt_BinOR(const mu::Array&, const mu::Array&);
mu::Array oprt_BinAND(const mu::Array&, const mu::Array&);

// Cast functions
mu::Array cast_category(const mu::Array& cats, const mu::Array& ids); // OPT=1
/////////////////////////////////////////////////
/// \brief Implements all numerical cast
/// functions as a template except for complex
/// value casts.
///
/// \param vals const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
template <class T>
mu::Array cast_numerical(const mu::Array& vals)
{
    mu::Array ret;
    ret.reserve(vals.size());

    for (size_t i = 0; i < vals.size(); i++)
    {
        const mu::Value& val = vals.get(i);

        if (val.isNumerical())
            ret.push_back(mu::Numerical( static_cast<T>(val.getNum().asF64()) ));
        else if (isConvertible(val.getStr(), CONVTYPE_DATE_TIME))
            ret.push_back(mu::Numerical( static_cast<T>(to_double(StrToTime(val.getStr()))) ));
        else if (isConvertible(val.getStr(), CONVTYPE_LOGICAL))
            ret.push_back(mu::Numerical( static_cast<T>(StrToLogical(val.getStr())) ));
        else if (isConvertible(val.getStr(), CONVTYPE_VALUE))
            ret.push_back(mu::Numerical( static_cast<T>(StrToCmplx(val.getStr()).real()) ));
        else
            ret.push_back(mu::Value(NAN));
    }

    return ret;
}

/////////////////////////////////////////////////
/// \brief Implements the complex numerical cast
/// functions as a template.
///
/// \param vals const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
template <class T>
mu::Array cast_numerical_cmplx(const mu::Array& vals)
{
    mu::Array ret;
    ret.reserve(vals.size());
    mu::NumericalType castType = std::is_same_v<T, double> ? mu::CF64 : mu::CF32;

    for (size_t i = 0; i < vals.size(); i++)
    {
        const mu::Value& val = vals.get(i);

        if (val.isNumerical())
            ret.push_back(mu::Numerical( static_cast<std::complex<T>>(val.getNum().asCF64()), castType ));
        else if (isConvertible(val.getStr(), CONVTYPE_DATE_TIME))
            ret.push_back(mu::Numerical( static_cast<std::complex<T>>(to_double(StrToTime(val.getStr()))), castType ));
        else if (isConvertible(val.getStr(), CONVTYPE_LOGICAL))
            ret.push_back(mu::Numerical( static_cast<std::complex<T>>(StrToLogical(val.getStr())), castType ));
        else if (isConvertible(val.getStr(), CONVTYPE_VALUE))
            ret.push_back(mu::Numerical( static_cast<std::complex<T>>(StrToCmplx(val.getStr())), castType ));
        else
            ret.push_back(mu::Value(NAN));
    }

    return ret;
}

#endif

