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
 * Implementierung der Parser-Funktionen
 */
#include "functionimplementation.hpp"
#include "../ParserLib/muParserTemplateMagic.h"
#define _USE_MATH_DEFINES

#include <cstdlib>
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>
#include <locale>
#include <limits>
#include <ios>
#include <iomanip>
#include <numeric>
#include <ctime>
#include <csignal>
#include <boost/math/common_factor.hpp>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include <noise/noise.h>
#include <omp.h>
#include <windows.h>

#include "student_t.hpp"
#ifndef PARSERSTANDALONE
#include "../datamanagement/memorymanager.hpp"
#endif
#include "../utils/tools.hpp"
#include "../version.h"

int nErrorIndices[2] = {-1,-1};
std::string sErrorToken = "";
extern time_t tTimeZero;

/*
 * Ende der globalen Variablen
 */


/////////////////////////////////////////////////
/// \brief Multiplies a number with the imaginary
/// unit.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_imaginaryUnit(const mu::Array& v)
{
    mu::Array res;

    for (size_t i = 0; i < v.size(); i++)
    {
        res.push_back(mu::Numerical(std::complex<double>(v[i].getNum().asCF64().imag() != 0.0 ? -v[i].getNum().asCF64().imag() : 0.0,
                                                         v[i].getNum().asCF64().real())));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Extracts the real part of a complex
/// number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_real(const mu::Array& v)
{
    mu::Array res;

    for (size_t i = 0; i < v.size(); i++)
    {
        res.push_back(mu::Numerical(v[i].getNum().asF64()));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Extracts the imaginary part of a
/// complex number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_imag(const mu::Array& v)
{
    mu::Array res;

    for (size_t i = 0; i < v.size(); i++)
    {
        res.push_back(mu::Numerical(v[i].getNum().asCF64().imag()));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Converts a rectangular representation
/// into polar representation and returns it as a
/// new complex number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_rect2polar(const mu::Array& v)
{
    mu::Array res;

    for (size_t i = 0; i < v.size(); i++)
    {
        res.push_back(mu::Numerical(std::complex<double>(std::abs(v[i].getNum().asCF64()),
                                                         std::arg(v[i].getNum().asCF64()))));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Converts a polar representation into a
/// rectangular representation and returns it as
/// a new complex number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_polar2rect(const mu::Array& v)
{
    mu::Array res;

    for (size_t i = 0; i < v.size(); i++)
    {
        res.push_back(mu::Numerical(std::polar(v[i].getNum().asCF64().real(),
                                               v[i].getNum().asCF64().imag())));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Calculates the complex conjugate
/// number of the passed complex number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_conj(const mu::Array& v)
{
    return mu::apply(std::conj, v);
}


/////////////////////////////////////////////////
/// \brief Construct a complex number from two
/// real numbers.
///
/// \param re const mu::Array&
/// \param im const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_complex(const mu::Array& re, const mu::Array& im)
{
    mu::Array res;

    for (size_t i = 0; i < std::max(re.size(), im.size()); i++)
    {
        res.push_back(mu::Numerical(std::complex<double>(re[i].getNum().asF64(),
                                                         im[i].getNum().asF64())));
    }

    return res;
}


mu::Array numfnc_getElements(const mu::Array& a, const mu::Array& idx)
{
    mu::Array res;

    for (size_t i = 0; i < idx.size(); i++)
    {
        int64_t n = idx.get(i).getNum().asI64();

        if (n > 0 && (size_t)n <= a.size())
            res.push_back(a.get(n-1));
    }

    return res;
}


static std::complex<double> factorial_impl(const std::complex<double>& v)
{
    if (mu::isnan(v) || mu::isinf(v))
        return NAN;

    std::complex<double> vResult = 1.0; // Ausgabe-Variable

    // --> Falls v == 0 ist, dann ist die Fakultaet 1 und nicht 0. Fangen wir hier ab <--
    if (intCast(v) == 0)
        return 1;

    if (intCast(v) < 0)
        return NAN;

    /* --> Zaehlschleife, die die Fakultaet bildet: allerdings in der Form 1*2*3*...*(n-1)*n und nicht
     *     in der Form, wie sie normal definiert wird: n*(n-1)*(n-2)*...*3*2*1 <--
     */
    for (int i = 1; i <= abs(intCast(v)); i++)
    {
        vResult *= i;
    }

    return vResult;

}

/////////////////////////////////////////////////
/// \brief Function representing the faculty of
/// any natural number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Faculty(const mu::Array& v)
{
    return mu::apply(factorial_impl, v);
}


static std::complex<double> double_factorial_impl(const std::complex<double>& v)
{
    if (mu::isnan(v) || mu::isinf(v))
        return NAN;

    std::complex<double> vResult = 1.0;

    if (intCast(v) < 0)
        return NAN;

    for (int n = intCast(std::abs(v)); n > 0; n -= 2)
    {
        vResult *= n;
    }

    return vResult;
}


/////////////////////////////////////////////////
/// \brief Function representing the double
/// faculty of any natural number.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_doubleFaculty(const mu::Array& v)
{
    return mu::apply(double_factorial_impl, v);
}


static std::complex<double> binom_impl(const std::complex<double>& v1, const std::complex<double>& v2)
{
    if (mu::isnan(v1) || mu::isnan(v2) || mu::isinf(v1) || mu::isinf(v2))
        return NAN;
    /* --> Bevor wir die bekannte Formel verwenden, pruefen wir einige Spezialfaelle, die den
     *     Algorithmus deutlich beschleunigen. Hier sei der Artikel auf Wikipedia zum Binomial-
     *     koeffzienten empfohlen <--
     */
    if (intCast(v2) < 0 || intCast(v1) < 0)
        return NAN;
    else if (intCast(v2) > intCast(v1)) // v2 > v1 ==> binom = 0!
        return 0;
    else if (intCast(v1) == intCast(v2) || (intCast(v1) != 0 && intCast(v2) == 0)) // v1 == v2 oder v2 == 0 und v1 != 0 ==> binom = 1!
        return 1;
    else if (intCast(v2) == 1 || intCast(v2) == intCast(v1)-1) // v2 == 1 oder v2 == v1-1 ==> binom = v1!
        return intCast(v1);
    else if (intCast(v2) == 2 && intCast(v2) < intCast(v1)) // v2 == 2 und v2 < v1 ==> binom = v1*(v1-1) / v2!
        return intCast(v1)*(intCast(v1)-1)/intCast(v2);
    else
    {
        /* --> In allen anderen Faellen muessen wir den Binomialkoeffzienten muehsam mithilfe der Formel
         *     binom(v1,v2) = v1!/(v2!*(v1-v2)!) ausrechnen. Das machen wir, indem wir die Funktion
         *     numfnc_Faculty(mu::Array) aufrufen <--
         */
        return factorial_impl(v1) / (factorial_impl(v2)*factorial_impl(intCast(v1) - intCast(v2)));
    }
}


/////////////////////////////////////////////////
/// \brief Function representing the binomial
/// coefficient.
///
/// \param v1 const mu::Array&
/// \param v2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Binom(const mu::Array& v1, const mu::Array& v2)
{
    return mu::apply(binom_impl, v1, v2);
}


/////////////////////////////////////////////////
/// \brief This function returns the number of
/// valid elements in its array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Num(const mu::Array* vElements, int nElements)
{
    size_t elems;

    if (nElements == 1)
    {
        elems = vElements[0].size();

        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (!vElements[0][i].isValid())
                elems--;
        }
    }
    else
    {
        elems = nElements;

        for (int i = 0; i < nElements; i++)
        {
            if (!vElements[i].front().isValid())
                elems--;
        }
    }

    return mu::Array(mu::Value(elems));
    /*mu::Array ret;

    for (int i = 0; i < nElements; i++)
    {
        size_t elems = vElements[i].size();

        for (size_t j = 0; j < vElements[i].size(); j++)
        {
            if (!vElements[i][j].isValid())
                elems--;
        }

        ret.push_back(Numerical(elems));
    }

    return ret;*/
}


/////////////////////////////////////////////////
/// \brief This functioon simply returns the
/// number of elements in its array (even the
/// invalid ones).
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Cnt(const mu::Array* vElements, int nElements)
{
    if (nElements == 1)
        return mu::Array(mu::Value(vElements[0].size()));

    return mu::Array(mu::Value(nElements));
    /*mu::Array ret;

    for (int i = 0; i < nElements; i++)
    {
        ret.push_back(vElements[i].size());
    }

    return ret;*/
}


static mu::Value conj(const mu::Value& val)
{
    return mu::Numerical(std::conj(val.getNum().asCF64()));
}


/////////////////////////////////////////////////
/// \brief This function calculates the standard
/// deviation of the elements in the passed
/// array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Std(const mu::Array* vElements, int nElements)
{
    mu::Value vStd = 0.0;
    mu::Value vMean = numfnc_Avg(vElements, nElements).front();
    mu::Value vNum = numfnc_Num(vElements, nElements).front().getNum().asF64();

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (vElements[0][i].isValid())
                vStd += (vElements[0][i] - vMean) * conj(vElements[0][i] - vMean);
        }
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front().isValid())
                vStd += (vElements[i].front() - vMean) * conj(vElements[i].front() - vMean);
        }
    }

    return mu::Value(std::sqrt((vStd / (vNum-mu::Numerical(1.0))).getNum().asCF64()));
}


/////////////////////////////////////////////////
/// \brief This function calculates the product
/// of all elements in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_product(const mu::Array* vElements, int nElements)
{
    mu::Value vProd = 1.0;

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (vElements[0][i].isValid())
                vProd *= vElements[0][i];
        }
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front().isValid())
                vProd *= vElements[i].front();
        }
    }

    return vProd;
}


/////////////////////////////////////////////////
/// \brief This function calculates the vector
/// norm of the elements in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Norm(const mu::Array* vElements, int nElements)
{
    mu::Value vProd = 0.0;

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (vElements[0][i].isValid())
                vProd += vElements[0][i] * conj(vElements[0][i]);
        }
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front().isValid())
                vProd += vElements[i].front() * conj(vElements[i].front());
        }
    }

    return mu::Value(std::sqrt(vProd.getNum().asCF64()));
}


/////////////////////////////////////////////////
/// \brief This function calculates the median of
/// the elements in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Med(const mu::Array* vElements, int nElements)
{
#ifndef PARSERSTANDALONE
    Memory _mem;

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            _mem.writeData(i, 0, vElements[0][i]);
        }

        return mu::Value(_mem.med(VectorIndex(0, vElements[0].size()-1), VectorIndex(0)));
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            _mem.writeData(i, 0, vElements[i].front());
        }
    }

    return mu::Value(_mem.med(VectorIndex(0, nElements-1), VectorIndex(0)));
#endif // PARSERSTANDALONE
}


/////////////////////////////////////////////////
/// \brief This function calculates the selected
/// percentile of the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Pct(const mu::Array* vElements, int nElements)
{
#ifndef PARSERSTANDALONE
    Memory _mem;

    if (nElements == 2)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            _mem.writeData(i, 0, vElements[0][i]);
        }

        return mu::Value(_mem.pct(VectorIndex(0, vElements[0].size()-1), VectorIndex(0), vElements[1].front().getNum().asCF64()));
    }

    for (int i = 0; i < nElements-1; i++)
    {
        _mem.writeData(i, 0, vElements[i].front());
    }

    return mu::Value(_mem.pct(VectorIndex(0, nElements-2), VectorIndex(0), vElements[nElements-1].front().getNum().asCF64()));

#endif // PARSERSTANDALONE
}


static mu::Value compare_impl(const mu::Array& vElements, const mu::Value& value, const mu::Value& mode)
{
    enum
    {
        RETURN_VALUE = 1,
        RETURN_LE = 2,
        RETURN_GE = 4,
        RETURN_FIRST = 8
    };

    int nType = 0;

    mu::Value vRef = value;
    mu::Value vKeep = vRef;
    int nKeep = -1;

    if (mode > mu::Value(0.0))
        nType = RETURN_GE;
    else if (mode < mu::Value(0.0))
        nType = RETURN_LE;

    switch (std::abs(mode.getNum().asI64()))
    {
        case 2:
            nType |= RETURN_VALUE;
            break;
        case 3:
            nType |= RETURN_FIRST;
            break;
        case 4:
            nType |= RETURN_FIRST | RETURN_VALUE;
            break;
    }

    for (size_t i = 0; i < vElements.size(); i++)
    {
        if (!vElements[i].isValid())
            continue;

        if (vElements[i] == vRef)
        {
            if (nType & RETURN_VALUE)
                return vElements[i];

            return i+1;
        }
        else if (nType & RETURN_GE && vElements[i] > vRef)
        {
            if (nType & RETURN_FIRST)
            {
                if (nType & RETURN_VALUE)
                    return vElements[i];

                return i+1;
            }

            if (nKeep == -1 || vElements[i] < vKeep)
            {
                vKeep = vElements[i];
                nKeep = i;
            }
            else
                continue;
        }
        else if (nType & RETURN_LE && vElements[i] < vRef)
        {
            if (nType & RETURN_FIRST)
            {
                if (nType & RETURN_VALUE)
                    return vElements[i];

                return i+1;
            }

            if (nKeep == -1 || vElements[i] > vKeep)
            {
                vKeep = vElements[i];
                nKeep = i;
            }
            else
                continue;
        }
    }

    if (nKeep == -1)
        return NAN;
    else if (nType & RETURN_VALUE)
        return vKeep;

    return nKeep+1;
}



/////////////////////////////////////////////////
/// \brief This function searches for elements of
/// a specified type in the passed array.
///
/// \param vElements const mu::Array&
/// \param value const mu::Array&
/// \param mode const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_compare(const mu::Array& vElements, const mu::Array& value, const mu::Array& mode)
{
    mu::Array res;

    for (size_t i = 0; i < std::max(value.size(), mode.size()); i++)
    {
        res.push_back(compare_impl(vElements, value.get(i), mode.get(i)));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief This function calculates the logical
/// AND operation between all elements in the
/// passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_and(const mu::Array* vElements, int nElements)
{
    if (nElements == 1)
        return mu::Value(mu::all(vElements[0]));
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (!vElements[i].front())
                return mu::Value(false);
        }
    }

    return mu::Value(true);
}


/////////////////////////////////////////////////
/// \brief This function calculates the logical
/// OR operation between all elements in the
/// passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_or(const mu::Array* vElements, int nElements)
{
    if (nElements == 1)
        return mu::Value(mu::any(vElements[0]));
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front())
                return mu::Value(true);
        }
    }

    return mu::Value(false);
}


/////////////////////////////////////////////////
/// \brief This function calculates the logical
/// XOR operation between all elements in the
/// passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_xor(const mu::Array* vElements, int nElements)
{
    bool isTrue = false;

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (vElements[0][i])
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return mu::Value(false);
            }
        }
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front())
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return mu::Value(false);
            }
        }
    }

    return mu::Value(isTrue);
}


/////////////////////////////////////////////////
/// \brief This function implements an abstract
/// polynomial of an arbitrary order.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_polynomial(const mu::Array* vElements, int nElements)
{
    if (!nElements)
        return mu::Value(NAN);
    else if (nElements == 1)
        return mu::Value(0.0);

    mu::Array res = vElements[1];

    for (int i = 2; i < nElements; i++)
        res += vElements[i] * vElements[0].pow(mu::Numerical(i-1));

    return res;
}


/////////////////////////////////////////////////
/// \brief This function implements the perlin
/// noise function.
///
/// \param x const mu::Array&
/// \param y const mu::Array& OPTIONAL
/// \param z const mu::Array& OPTIONAL
/// \param seed const mu::Array& OPTIONAL
/// \param freq const mu::Array& OPTIONAL
/// \param octave const mu::Array& OPTIONAL
/// \param persistence const mu::Array& OPTIONAL
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_perlin(const mu::Array& x, const mu::Array& y, const mu::Array& z, const mu::Array& seed, const mu::Array& freq, const mu::Array& octave, const mu::Array& persistence)
{
    noise::module::Perlin perlinNoise;

    mu::Array res;

    for (size_t i = 0; i < std::max({x.size(), y.size(), z.size(), seed.size(), freq.size(), octave.size(), persistence.size()}); i++)
    {
        if (!persistence.isDefault())
            perlinNoise.SetPersistence(persistence.get(i).getNum().asF64());

        if (!octave.isDefault())
            perlinNoise.SetOctaveCount(octave.get(i).getNum().asI64());

        if (!freq.isDefault())
            perlinNoise.SetFrequency(freq.get(i).getNum().asF64());

        if (!seed.isDefault())
            perlinNoise.SetSeed(seed.get(i).getNum().asI64());

        if (z.isDefault() && y.isDefault())
            res.push_back(perlinNoise.GetValue(x.get(i).getNum().asF64(),
                                               0,
                                               0));
        else if (z.isDefault())
            res.push_back(perlinNoise.GetValue(x.get(i).getNum().asF64(),
                                               y.get(i).getNum().asF64(),
                                               0));
        else
            res.push_back(perlinNoise.GetValue(x.get(i).getNum().asF64(),
                                               y.get(i).getNum().asF64(),
                                               z.get(i).getNum().asF64()));
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief Adaption of the logtoidx() function
/// for 1D data arrays.
///
/// \param v const mu::Array*
/// \param n int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_logtoidx(const mu::Array* v, int n)
{
    mu::Array vIdx;

    if (n == 1)
    {
        for (size_t i = 0; i < v[0].size(); i++)
        {
            if (v[0][i].isValid() && v[0][i])
                vIdx.push_back(mu::Value(i+1));
        }
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (v[i].front().isValid() && v[i].front())
                vIdx.push_back(mu::Value(i+1));
        }
    }

    if (!vIdx.size())
        vIdx.push_back(mu::Value(0));

    return vIdx;
}


/////////////////////////////////////////////////
/// \brief Adaption of the idxtolog() function
/// for 1D data arrays.
///
/// \param v const mu::Array*
/// \param n int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_idxtolog(const mu::Array* v, int n)
{
    if (!n)
        return mu::Value(false);

    mu::Array maxIdx = numfnc_Max(v, n);

    if (mu::isnan(maxIdx.front().getNum().asF64()))
        return mu::Value(false);

    mu::Array vLogical;
    vLogical.resize(maxIdx.front().getNum().asF64(), mu::Value(false));

    if (n == 1)
    {
        for (size_t i = 0; i < v[0].size(); i++)
        {
            if (v[0][i].isValid() || v[0][i] > mu::Value(0))
                vLogical[v[0][i].getNum().asI64()-1] = mu::Value(true);
        }
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (v[i].front().isValid() || v[i].front() > mu::Value(0))
                vLogical[v[i].front().getNum().asI64()-1] = mu::Value(true);
        }
    }

    return vLogical;
}


/////////////////////////////////////////////////
/// \brief Function for getting the order of an
/// array.
///
/// \param v const mu::Array*
/// \param n int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_order(const mu::Array* v, int n)
{
    if (!n)
        return mu::Value(false);

    mu::Array index;

    if (n == 1)
    {
        for (size_t i = 1; i <= v[0].size(); i++)
        {
            index.push_back(i);
        }

        auto sorter = [=](const mu::Value& v1, const mu::Value& v2)
            {return v[0][v1.getNum().asI64()-1] < v[0][v2.getNum().asI64()-1];};
        std::sort(index.begin(), index.end(), sorter);
    }
    else
    {
        for (int i = 1; i <= n; i++)
        {
            index.push_back(i);
        }

        auto sorter = [=](const mu::Value& v1, const mu::Value& v2)
            {return v[v1.getNum().asI64()-1].front() < v[v2.getNum().asI64()-1].front();};
        std::sort(index.begin(), index.end(), sorter);
    }

    return index;
}


/////////////////////////////////////////////////
/// \brief This function summarizes all elements
/// in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Sum(const mu::Array* vElements, int nElements)
{
    mu::Value fRes;

    if (nElements == 1)
    {
        for (size_t i = 0; i < vElements[0].size(); i++)
        {
            if (vElements[0][i].isValid())
                fRes += vElements[0][i];
        }
    }
    else
    {
        for (int i = 0; i < nElements; i++)
        {
            if (vElements[i].front().isValid())
                fRes += vElements[i].front();
        }
    }

    return fRes;
}


/////////////////////////////////////////////////
/// \brief This function calculates the average
/// of all elements in passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Avg(const mu::Array* vElements, int nElements)
{
    return numfnc_Sum(vElements, nElements) / mu::Value(numfnc_Num(vElements, nElements).front().getNum().asCF64());
}


/////////////////////////////////////////////////
/// \brief This function calculates the minimal
/// value of all elements in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Min(const mu::Array* vElements, int nElements)
{
    if (!nElements)
        return mu::Value(NAN);

    mu::Value res = vElements[0][0];

    if (nElements == 1)
    {
        for (size_t i = 1; i < vElements[0].size(); i++)
        {
            if (!res.isValid() || (vElements[0][i].isValid() && vElements[0][i] < res))
                res = vElements[0][i];
        }
    }
    else
    {
        for (int i = 1; i < nElements; i++)
        {
            if (!res.isValid() || (vElements[i].front().isValid() && vElements[i].front() < res))
                res = vElements[i].front();
        }
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief This function calculates the maximal
/// value of all elements in the passed array.
///
/// \param vElements const mu::Array*
/// \param nElements int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Max(const mu::Array* vElements, int nElements)
{
    if (!nElements)
        return mu::Value(NAN);

    mu::Value res = vElements[0][0];

    if (nElements == 1)
    {
        for (size_t i = 1; i < vElements[0].size(); i++)
        {
            if (!res.isValid() || (vElements[0][i].isValid() && vElements[0][i] > res))
                res = vElements[0][i];
        }
    }
    else
    {
        for (int i = 1; i < nElements; i++)
        {
            if (!res.isValid() || (vElements[i].front().isValid() && vElements[i].front() > res))
                res = vElements[i].front();
        }
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief This function returns the index of the
/// (first) minimal value in the array.
///
/// \param vElements const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_MinPos(const mu::Array& vElements)
{
    return numfnc_compare(vElements, numfnc_Min(&vElements, 1), mu::Value(0.0));
}


/////////////////////////////////////////////////
/// \brief This function returns the index of the
/// (first) maximal value in the array.
///
/// \param vElements const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_MaxPos(const mu::Array& vElements)
{
    return numfnc_compare(vElements, numfnc_Max(&vElements, 1), mu::Value(0.0));
}


static std::complex<double> round_impl(const std::complex<double>& vToRound, const std::complex<double>& vDecimals)
{
    if (mu::isinf(vToRound) || mu::isinf(vDecimals) || mu::isnan(vToRound) || mu::isnan(vDecimals))
        return NAN;

    double dDecimals = intPower(10, -abs(intCast(vDecimals)));
    std::complex<double> vRounded = vToRound / dDecimals;
    vRounded = std::complex<double>(std::round(vRounded.real()), std::round(vRounded.imag()));
    return vRounded * dDecimals;
}


static mu::Value numfnc_rint(const mu::Value& val)
{
    const mu::Numerical& nval = val.getNum();

    if (nval.getType() <= mu::Numerical::UI64)
        return nval;

    if (nval.getType() <= mu::Numerical::F64 || nval.isInt())
        return (int64_t)std::rint(nval.asCF64().real());

    return std::complex<double>(std::rint(nval.asCF64().real()), std::rint(nval.asCF64().imag()));
}

/////////////////////////////////////////////////
/// \brief Implements the rint() function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_rint(const mu::Array& a)
{
    return mu::apply(numfnc_rint, a);
}


/////////////////////////////////////////////////
/// \brief This function rounds the passed value
/// to the selected number of decimals.
///
/// \param vToRound const mu::Array&
/// \param vDecimals const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_round(const mu::Array& vToRound, const mu::Array& vDecimals)
{
    return mu::apply(round_impl, vToRound, vDecimals);
}


/////////////////////////////////////////////////
/// \brief This function converts degree to
/// radian.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_toRadian(const mu::Array& v)
{
    return v * mu::Value(M_PI/180.0);
}


/////////////////////////////////////////////////
/// \brief This function converts radian to
/// degree.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_toDegree(const mu::Array& v)
{
    return v * mu::Value(180.0 / M_PI);
}


static std::complex<double> LegendrePolynomial_impl(const std::complex<double>& vn, const std::complex<double>& v)
{
    if (mu::isinf(vn) || mu::isnan(vn) || mu::isinf(v) || mu::isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    std::complex<double> dResult = 0.0;
    for (int k = 0; k <= n/2; k++)
    {
        dResult += intPower(-1,k)*binom_impl(n,k)*binom_impl(2*(n-k),n)*intPower(v,n-2*k);
    }
    dResult *= intPower(2, -n);
    return dResult;
}


static std::complex<double> AssociatedLegendrePolynomial_impl(const std::complex<double>& vl, const std::complex<double>& vm, const std::complex<double>& v)
{
    if (mu::isinf(vl) || mu::isnan(vl) || mu::isinf(vm) || mu::isnan(vm) || mu::isinf(v) || mu::isnan(v))
        return NAN;
    int l = intCast(fabs(vl));
    int m = intCast(fabs(vm));
    if (m > l)
        return NAN;
    if (!m)
        return LegendrePolynomial_impl(l,v);
    else if (m < 0)
        return intPower(-1.0,m)* factorial_impl(l-m) / factorial_impl(l+m) * AssociatedLegendrePolynomial_impl(l,m,v);
    else if (l == m)
        return intPower(-1.0,l)*double_factorial_impl((2.0*l-1.0))*pow(1.0-v*v,(double)l/2.0);//intPower(sqrt(1-v*v), l);
    else if (m == l-1)
        return v*(2.0*l-1.0)*intPower(-1.0,l-1)*double_factorial_impl((2.0*l-3.0))*pow(1.0-v*v,((double)l-1.0)/2.0);//intPower(sqrt(1-v*v), l-1);
    else
        return 1.0/(double)(l-m)*(v*(2.0*l-1)*AssociatedLegendrePolynomial_impl(l-1,m,v) - (double)(l+m-1)*AssociatedLegendrePolynomial_impl(l-2,m,v));

    return 0.0;
}


static std::complex<double> SphericalHarmonics_impl(const std::complex<double>& vl, const std::complex<double>& vm, const std::complex<double>& theta, const std::complex<double>& phi)
{
    if (std::isinf(vl.real()) || std::isnan(vl.real())
        || std::isinf(vm.real()) || std::isnan(vm.real())
        || std::isinf(theta.real()) || std::isnan(theta.real())
        || std::isinf(phi.real()) || std::isnan(phi.real()))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return std::sqrt((2.0*l+1.0) * factorial_impl(l-m) / (4.0 * M_PI * factorial_impl(l+m)))
            * AssociatedLegendrePolynomial_impl(l, m, std::cos(theta.real())) * std::exp(std::complex<double>(0, m*phi.real()));

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the real part
/// of the selected spherical harmonics.
///
/// \param vl const mu::Array&
/// \param vm const mu::Array&
/// \param theta const mu::Array&
/// \param phi const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_SphericalHarmonics(const mu::Array& vl, const mu::Array& vm, const mu::Array& theta, const mu::Array& phi)
{
    return mu::apply(SphericalHarmonics_impl, vl, vm, theta, phi);
}


static std::complex<double> imSphericalHarmonics_impl(const std::complex<double>& vl, const std::complex<double>& vm, const std::complex<double>& theta, const std::complex<double>& phi)
{
    if (std::isinf(vl.real()) || std::isnan(vl.real())
        || std::isinf(vm.real()) || std::isnan(vm.real())
        || std::isinf(theta.real()) || std::isnan(theta.real())
        || std::isinf(phi.real()) || std::isnan(phi.real()))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return std::sqrt((2.0*l+1.0) * factorial_impl(l-m) / (4.0 * M_PI * factorial_impl(l+m)))
            * AssociatedLegendrePolynomial_impl(l, m, std::cos(theta.real())) * std::sin(m*phi.real());

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the imaginary
/// part of the selected spherical harmonics.
///
/// \param vl const mu::Array&
/// \param vm const mu::Array&
/// \param theta const mu::Array&
/// \param phi const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_imSphericalHarmonics(const mu::Array& vl, const mu::Array& vm, const mu::Array& theta, const mu::Array& phi)
{
    return mu::apply(imSphericalHarmonics_impl, vl, vm, theta, phi);
}


/////////////////////////////////////////////////
/// \brief This function calculates the radial
/// part of the Zernike polynomials.
///
/// \param n int
/// \param m int
/// \param rho const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> zernikeRadial_impl(int n, int m, const std::complex<double>& rho)
{
    std::complex<double> vReturn = 0;
    std::complex<double> vNorm = 0;

    if (std::abs(rho) > 1.0)
        return NAN;

    if ((n-m) % 2)
        return 0.0;

    for (int k = 0; k <= (n-m)/2; k++)
    {
        if (k % 2)
        {
            vReturn -= factorial_impl(n-k)*intPower(rho, n-2*k)/(factorial_impl(k)*factorial_impl((n+m)/2.0-k)*factorial_impl((n-m)/2.0-k));
            vNorm -= factorial_impl(n-k)/(factorial_impl(k)*factorial_impl((n+m)/2.0-k)*factorial_impl((n-m)/2.0-k));
        }
        else
        {
            vReturn += factorial_impl(n-k)*intPower(rho, n-2*k)/(factorial_impl(k)*factorial_impl((n+m)/2.0-k)*factorial_impl((n-m)/2.0-k));
            vNorm += factorial_impl(n-k)/(factorial_impl(k)*factorial_impl((n+m)/2.0-k)*factorial_impl((n-m)/2.0-k));
        }
    }

    return vReturn/vNorm;
}


static std::complex<double> Zernike_impl(const std::complex<double>& vn, const std::complex<double>& vm, const std::complex<double>& rho, const std::complex<double>& phi)
{
    if (std::isinf(vn.real()) || std::isnan(vn.real())
        || std::isinf(vm.real()) || std::isnan(vm.real())
        || mu::isinf(rho) || mu::isnan(rho)
        || mu::isinf(phi) || mu::isnan(phi))
        return NAN;

    int n = intCast(vn);
    int m = intCast(vm);

    if (n < abs(m))
        return NAN;

    if (m < 0)
        return zernikeRadial_impl(n, -m, rho) * std::sin(-(double)m*phi);
    else
        return zernikeRadial_impl(n, m, rho) * std::cos((double)m*phi);
}


/////////////////////////////////////////////////
/// \brief This function calculates the selected
/// Zernike polynomials.
///
/// \param vn const mu::Array&
/// \param vm const mu::Array&
/// \param rho const mu::Array&
/// \param phi const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Zernike(const mu::Array& vn, const mu::Array& vm, const mu::Array& rho, const mu::Array& phi)
{
    return mu::apply(Zernike_impl, vn, vm, rho, phi);
}


static std::complex<double> SinusCardinalis_impl(const std::complex<double>& v)
{
    if (mu::isinf(v) || mu::isnan(v))
        return NAN;

    if (v == 0.0)
        return 1.0;
    else
        return std::sin(v)/v;
}

/////////////////////////////////////////////////
/// \brief This function calculates the cardinal
/// sine of x.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_SinusCardinalis(const mu::Array& v)
{
    return mu::apply(SinusCardinalis_impl, v);
}


static std::complex<double> SphericalBessel_impl(const std::complex<double>& vn, const std::complex<double>& vc)
{
    if (mu::isinf(vn) || mu::isinf(vc) || mu::isnan(vn) || mu::isnan(vc))
        return NAN;
    int n = intCast(fabs(vn));
    double v = vc.real();
    if (!n && v == 0.0)
        return 1.0;
    else if (!n)
        return sin(v)/v;
    else if (n && v == 0.0)
        return 0.0;
    else if (n == 1)
        return sin(v)/(v*v) - cos(v)/v;
    else if (n == 2)
        return (3.0/(v*v)-1.0)*sin(v)/v-3.0*cos(v)/(v*v);
    else if (n == 3)
        return (15.0/(v*v*v)-6.0/v)*sin(v)/v-(15.0/(v*v)-1.0)*cos(v)/v;
    else if (n == 4)
        return 5.0/(v*v*v*v)*(2.0*v*v-21.0)*cos(v) + 1.0/(v*v*v*v*v)*(v*v*v*v - 45.0*v*v + 105.0)*sin(v);
    else if (n == 5)
        return 15.0/(v*v*v*v*v*v)*(v*v*v*v - 28.0*v*v + 63.0) * sin(v) + 1.0/(v*v*v*v*v)*(-v*v*v*v + 105.0*v*v - 945.0)*cos(v);
    else if (n == 6)
        return (-intPower(v,6)+210.0*v*v*v*v-4725.0*v*v+10395.0)*sin(v)/intPower(v,7)-21.0*(v*v*v*v-60.0*v*v+495.0)*cos(v)/intPower(v,6);
    else if (n == 7)
        return (intPower(v,6)-378.0*v*v*v*v+17325.0*v*v-135135.0)*cos(v)/intPower(v,7) - 7.0*(4.0*intPower(v,6)-450.0*v*v*v*v+8910.0*v*v-19305.0)*sin(v)/intPower(v,8);
    else if (n == 8)
        return 9.0*(4.0*intPower(v,6)-770.0*v*v*v*v+30030.0*v*v-225225.0)*cos(v)/intPower(v,8)+(intPower(v,8)-630.0*intPower(v,6)+51975.0*v*v*v*v-945945.0*v*v+2027025.0)*sin(v)/intPower(v,9);
    else if (n == 9)
        return 45.0*(intPower(v,8)-308.0*intPower(v,6)+21021.0*v*v*v*v-360360.0*v*v+765765.0)*sin(v)/intPower(v,10)+(-intPower(v,8)+990.0*intPower(v,6)-135135.0*v*v*v*v+4729725.0*v*v-34459425.0)*cos(v)/intPower(v,9);
    else if (n == 10)
        return (-intPower(v,10)+1485.0*intPower(v,8)-315315.0*intPower(v,6)+18918900.0*v*v*v*v-310134825.0*v*v+654729075.0)*sin(v)/intPower(v,11)-55.0*(intPower(v,8)-468.0*intPower(v,6)+51597.0*v*v*v*v-1670760.0*v*v+11904165.0)*cos(v)/intPower(v,10);
    else
    {
        return gsl_sf_bessel_jl(intCast(vn), fabs(v));
    }
    return 0.0;
}



/////////////////////////////////////////////////
/// \brief This function calculates the spherical
/// bessel function.
///
/// \param vn const mu::Array&
/// \param vc const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_SphericalBessel(const mu::Array& vn, const mu::Array& vc)
{
    return mu::apply(SphericalBessel_impl, vn, vc);
}


static std::complex<double> SphericalNeumann_impl(const std::complex<double>& vn, const std::complex<double>& vc)
{
    if (mu::isinf(vn) || mu::isnan(vn) || mu::isinf(vc) || mu::isnan(vc))
        return NAN;
    int n = intCast(fabs(vn));
    double v = vc.real();
    if (v == 0.0)
        return INFINITY;
    else if (!n)
        return -cos(v)/v;
    else if (n == 1)
        return -cos(v)/(v*v) - sin(v)/v;
    else if (n == 2)
        return (-3.0/(v*v)+1.0)*cos(v)/v - 3.0*sin(v)/(v*v);
    else if (n == 3)
        return (-15.0/(v*v*v)+6.0/v)*cos(v)/v - (15.0/(v*v)-1.0)*sin(v)/v;
    else if (n == 4)
        return 5.0/(v*v*v*v)*(2.0*v*v-21.0)*sin(v) + 1.0/(v*v*v*v*v)*(-v*v*v*v+45.0*v*v - 105.0)*cos(v);
    else if (n == 5)
        return 1.0/(v*v*v*v*v)*(-v*v*v*v + 105.0*v*v - 945.0)*sin(v) - 15.0/(v*v*v*v*v*v)*(v*v*v*v - 28.0*v*v + 63.0)*cos(v);
    else if (n == 6)
        return (intPower(v,6)-210.0*v*v*v*v+4725.0*v*v-10395.0)*cos(v)/intPower(v,7)-21.0*(v*v*v*v-60.0*v*v+495.0)*sin(v)/intPower(v,6);
    else if (n == 7)
        return 7.0*(4.0*intPower(v,6)-450.0*v*v*v*v+8910.0*v*v-19305.0)*cos(v)/intPower(v,8)+(intPower(v,6)-378.0*v*v*v*v-17325.0*v*v-135135.0)*sin(v)/intPower(v,7);
    else if (n == 8)
        return 9.0*(4.0*intPower(v,6)-770.0*v*v*v*v+30030.0*v*v-225225.0)*sin(v)/intPower(v,8)+(-intPower(v,8)+630.0*intPower(v,6)-51975.0*v*v*v*v+945945.0*v*v-2027025.0)*cos(v)/intPower(v,9);
    else if (n == 9)
        return (-intPower(v,8)+990.0*intPower(v,6)-135135.0*v*v*v*v+4729725.0*v*v-34459425.0)*sin(v)/intPower(v,9)-45.0*(intPower(v,8)-308.0*intPower(v,6)+21021.0*v*v*v*v-360360.0*v*v-765765.0)*cos(v)/intPower(v,10);
    else if (n == 10)
        return (intPower(v,10)-1485.0*intPower(v,8)+315315.0*intPower(v,6)-18918900.0*v*v*v*v+310134825.0*v*v-654729075.0)*cos(v)/intPower(v,11)-55.0*(intPower(v,8)-468.0*intPower(v,6)+51597.0*v*v*v*v-1670760.0*v*v+11904165.0)*sin(v)/intPower(v,10);
    else
    {
        return gsl_sf_bessel_yl(intCast(vn), fabs(v));
    }
    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the spherical
/// von Neumann function.
///
/// \param vn const mu::Array&
/// \param vc const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_SphericalNeumann(const mu::Array& vn, const mu::Array& vc)
{
    return mu::apply(SphericalNeumann_impl, vn, vc);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Legendre
/// polynomials of the selected order.
///
/// \param vn const mu::Array&
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_LegendrePolynomial(const mu::Array& vn, const mu::Array& v)
{
    return mu::apply(LegendrePolynomial_impl, vn, v);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Legendre polynomials of the
/// selected order.
///
/// \param vl const mu::Array&
/// \param vm const mu::Array&
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_AssociatedLegendrePolynomial(const mu::Array& vl, const mu::Array& vm, const mu::Array& v)
{
    return mu::apply(AssociatedLegendrePolynomial_impl, vl, vm, v);
}


static std::complex<double> LaguerrePolynomial_impl(const std::complex<double>& vn, const std::complex<double>& v)
{
    if (mu::isinf(vn) || mu::isnan(vn) || mu::isinf(v) || mu::isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    std::complex<double> dResult = 0.0;
    for (int k = 0; k <= n; k++)
    {
        dResult += intPower(-v,k)*binom_impl(n,k)/factorial_impl(k);
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the Laguerre
/// polynomials of the selected order.
///
/// \param vn const mu::Array&
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_LaguerrePolynomial(const mu::Array& vn, const mu::Array& v)
{
    return mu::apply(LaguerrePolynomial_impl, vn, v);
}


static std::complex<double> AssociatedLaguerrePolynomial_impl(const std::complex<double>& vn, const std::complex<double>& vk, const std::complex<double>& v)
{
    if (mu::isinf(vn) || mu::isnan(vn) || mu::isinf(vk) || mu::isnan(vk) || mu::isinf(v) || mu::isnan(v))
        return NAN;
    int n = intCast(fabs(vn));
    int k = intCast(fabs(vk));
//    if (k > n)
//        return NAN;

    if (n == 0)
        return 1.0;

    std::complex<double> dResult = 0.0;
    std::complex<double> vFaculty = factorial_impl(n+k);
    for (int m = 0; m <= n; m++)
    {
        dResult += vFaculty * intPower(-v,m) / (factorial_impl(n-m)*factorial_impl(k+m)*factorial_impl(m));
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Laguerre polynomials of the
/// selected order.
///
/// \param vn const mu::Array&
/// \param vk const mu::Array&
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_AssociatedLaguerrePolynomial(const mu::Array& vn, const mu::Array& vk, const mu::Array& v)
{
    return mu::apply(AssociatedLaguerrePolynomial_impl, vn, vk, v);
}


static std::complex<double> HermitePolynomial_impl(const std::complex<double>& vn, const std::complex<double>& v)
{
    if (mu::isinf(vn) || mu::isnan(vn) || mu::isinf(v) || mu::isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    switch (n)
    {
        case 0: return 1.0;
        case 1: return 2.0*v;
        case 2: return 4.0*v*v - 2.0;
        case 3: return 8.0*v*v*v - 12.0*v;
        case 4: return 16.0*v*v*v*v - 48.0*v*v + 12.0;
        default: return 2.0*v*HermitePolynomial_impl(n-1,v) - 2.0*(double)(n-1)*HermitePolynomial_impl(n-2,v);
    }
    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the Hermite
/// polynomials of the selected order.
///
/// \param vn const mu::Array&
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_HermitePolynomial(const mu::Array& vn, const mu::Array& v)
{
    return mu::apply(HermitePolynomial_impl, vn, v);
}


static std::complex<double> BetheWeizsaecker_impl(const std::complex<double>& vN, const std::complex<double>& vZ)
{
    if (std::isinf(vN.real()) || std::isnan(vN.real()) || std::isinf(vZ.real()) || std::isnan(vZ.real()))
        return NAN;
    // nan/inf
    double a_V = 15.67;
    double a_S = 17.23;
    double a_F = 23.2875;
    double a_C = 0.714;
    double a_p = 11.2;
    double A = vN.real() + vZ.real();
    double dEnergy = 0.0;
    int delta = 0;
    unsigned int N = (unsigned int)intCast(round_impl(vN,0));
    unsigned int Z = (unsigned int)intCast(round_impl(vZ,0));

    if (A < 0 || vZ.real() < 0 || vN.real() < 0)
        return NAN;
    if (A == 0)
        return 0.0;
    if (N % 2 && Z % 2)
        delta = -1;
    else if (!(N % 2 || Z % 2))
        delta = 1;

    dEnergy = a_V*A - a_S*pow(A,2.0/3.0) - a_F*(vN.real()-vZ.real())*(vN.real()-vZ.real())/A - a_C*vZ.real()*(vZ.real()-1)/pow(A,1.0/3.0) + (double)delta*a_p/sqrt(A);
    if (dEnergy >= 0)
        return dEnergy;
    else
        return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the nucleic
/// core binding energy according the Bethe
/// Weizsäcker formula.
///
/// \param vN const mu::Array&
/// \param vZ const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_BetheWeizsaecker(const mu::Array& vN, const mu::Array& vZ)
{
    return mu::apply(BetheWeizsaecker_impl, vN, vZ);
}


/////////////////////////////////////////////////
/// \brief This function represents the Heaviside
/// (theta) function.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Heaviside(const mu::Array& v)
{
    return v >= mu::Value(0.0);
}


static std::complex<double> phi_impl(const std::complex<double>& x, const std::complex<double>& y)
{
    if (isinf(x.real()) || isnan(x.real()) || isinf(y.real()) || isnan(y.real()))
        return NAN;
    if (y.real() < 0)
        return M_PI+abs(M_PI + atan2(y.real(), x.real()));
    return atan2(y.real(), x.real());
}


/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the x axis in the x-y plane (the
/// azimuthal angle phi).
///
/// \param x const mu::Array&
/// \param y const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_phi(const mu::Array& x, const mu::Array& y)
{
    return mu::apply(phi_impl, x, y);
}


std::complex<double> theta_impl(const std::complex<double>& x, const std::complex<double>& y, const std::complex<double>& z)
{
    if (mu::isinf(x) || mu::isnan(x) || mu::isinf(y) || mu::isnan(y) || mu::isinf(z) || mu::isnan(z))
        return NAN;
    if (x == 0.0 && y == 0.0 && z == 0.0)
        return M_PI/2;
    return acos(z/sqrt(x*conj(x)+y*conj(y)+z*conj(z)));
}


/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the z axis in any z-r plane (the
/// polar angle theta).
///
/// \param x const mu::Array&
/// \param y const mu::Array&
/// \param z const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_theta(const mu::Array& x, const mu::Array& y, const mu::Array& z)
{
    return mu::apply(theta_impl, x, y, z);
}


static std::complex<double> rand_impl(const std::complex<double>& vRandMin, const std::complex<double>& vRandMax)
{
    if (mu::isinf(vRandMin) || mu::isnan(vRandMin) || mu::isinf(vRandMax) || mu::isnan(vRandMax))
        return NAN;

    static std::uniform_real_distribution<double> randDist(0, 1);
    return randDist(getRandGenInstance()) * (vRandMax - vRandMin) + vRandMin;
}


static std::complex<double> gauss_rand_impl(const std::complex<double>& vRandAvg, const std::complex<double>& vRandstd)
{
    if (mu::isinf(vRandAvg) || mu::isnan(vRandAvg) || mu::isinf(vRandstd) || mu::isnan(vRandstd))
        return NAN;

    static std::normal_distribution<double> randDist(0, 1);
    return randDist(getRandGenInstance()) * fabs(vRandstd) + vRandAvg;
}


/////////////////////////////////////////////////
/// \brief This function returns a uniformly
/// distributed random number between both
/// boundaries.
///
/// \param vRandMin const mu::Array&
/// \param vRandMax const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_Random(const mu::Array& vRandMin, const mu::Array& vRandMax, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max({vRandMax.size(), vRandMin.size(), nRandCount}); i++)
    {
        ret.push_back(mu::Value(rand_impl(vRandMin.get(i).getNum().asCF64(), vRandMax.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function returns a gaussian
/// distributed random number using the passed
/// values as mean and standard deviation.
///
/// \param vRandAvg const mu::Array&
/// \param vRandStd const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_gRandom(const mu::Array& vRandAvg, const mu::Array& vRandStd, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max({vRandAvg.size(), vRandStd.size(), nRandCount}); i++)
    {
        ret.push_back(mu::Value(gauss_rand_impl(vRandAvg.get(i).getNum().asCF64(), vRandStd.get(i).getNum().asCF64())));
    }

    return ret;
}

static std::complex<double> erf_impl(const std::complex<double>& x)
{
    if (mu::isinf(x.real()) || mu::isnan(x.real()))
        return NAN;
    return std::erf(x.real());
}

static std::complex<double> erfc_impl(const std::complex<double>& x)
{
    if (mu::isinf(x.real()) || mu::isnan(x.real()))
        return NAN;
    return std::erfc(x.real());
}


/////////////////////////////////////////////////
/// \brief This function calculates the gaussian
/// error function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_erf(const mu::Array& x)
{
    return mu::apply(erf_impl, x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// complementary gaussian error function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_erfc(const mu::Array& x)
{
    return mu::apply(erfc_impl, x);
}

static std::complex<double> gamma_impl(const std::complex<double>& x)
{
    if (mu::isinf(x) || mu::isnan(x))
        return NAN;

    if (x.imag() == 0.0)
        return tgamma(x.real());

    gsl_sf_result lng;
    gsl_sf_result arg;
    gsl_sf_lngamma_complex_e(x.real(), x.imag(), &lng, &arg);

    return std::polar(std::exp(lng.val), arg.val);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// riemannian Gamma function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_gamma(const mu::Array& x)
{
    return mu::apply(gamma_impl, x);
}


static std::complex<double> AiryA_impl(const std::complex<double>& x)
{
    return gsl_sf_airy_Ai(x.real(), GSL_PREC_DOUBLE);
}


static std::complex<double> AiryB_impl(const std::complex<double>& x)
{
    return gsl_sf_airy_Bi(x.real(), GSL_PREC_DOUBLE);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Ai(x).
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_AiryA(const mu::Array& x)
{
    return mu::apply(AiryA_impl, x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Bi(x).
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_AiryB(const mu::Array& x)
{
    return mu::apply(AiryB_impl, x);
}


static std::complex<double> RegularCylBessel_impl(const std::complex<double>& n, const std::complex<double>& x)
{
    if (n.real() >= 0.0)
        return gsl_sf_bessel_Jn(intCast(n), x.real());
    else
        return NAN;
}


static std::complex<double> IrregularCylBessel_impl(const std::complex<double>& n, const std::complex<double>& x)
{
    if (x != 0.0 && n.real() >= 0.0)
        return x.real()/fabs(x.real())*gsl_sf_bessel_Yn(intCast(n), fabs(x.real()));
    else
        return -INFINITY;
}


/////////////////////////////////////////////////
/// \brief This function calculates the regulary
/// bessel function.
///
/// \param n const mu::Array&
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_RegularCylBessel(const mu::Array& n, const mu::Array& x)
{
    return mu::apply(RegularCylBessel_impl, n, x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// irregulary bessel (von Neumann) function.
///
/// \param n const mu::Array&
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_IrregularCylBessel(const mu::Array& n, const mu::Array& x)
{
    return mu::apply(IrregularCylBessel_impl, n, x);
}


static std::complex<double> EllipticF_impl(const std::complex<double>& phic, const std::complex<double>& kc)
{
    double k = kc.real();
    double phi = phic.real();

    if (mu::isnan(k) || mu::isnan(phi) || mu::isinf(k) || mu::isinf(phi))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2)
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi/M_PI_2));

        if (phi < 0)
            nSign = -1;

        if (!(nMultiple % 2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_Kcomp(k,0) + gsl_sf_ellint_F(fabs(phi)-nMultiple*M_PI_2, k, 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_Kcomp(k,0) - gsl_sf_ellint_F(M_PI_2-(fabs(phi)-nMultiple*M_PI_2), k, 0));
    }

    return gsl_sf_ellint_F(phi, k, 0);
}


static std::complex<double> EllipticE_impl(const std::complex<double>& phic, const std::complex<double>& kc)
{
    double phi = phic.real();
    double k = kc.real();

    if (mu::isnan(k) || mu::isnan(phi) || mu::isinf(k) || mu::isinf(phi))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2)
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi/M_PI_2));

        if (phi < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_Ecomp(k,0) + gsl_sf_ellint_E(fabs(phi)-nMultiple*M_PI_2, k, 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_Ecomp(k,0) - gsl_sf_ellint_E(M_PI_2-(fabs(phi)-nMultiple*M_PI_2), k, 0));
    }

    return gsl_sf_ellint_E(phi, k, 0);
}


static std::complex<double> EllipticP_impl(const std::complex<double>& phi, const std::complex<double>& n, const std::complex<double>& k)
{
    if (mu::isnan(k.real()) || mu::isnan(phi.real()) || mu::isinf(k.real()) || mu::isinf(phi.real()) || mu::isnan(n.real()) || mu::isinf(n.real()))
        return NAN;

    if (k.real() < 0 || k.real() >= 1)
        return NAN;

    if (phi.real() < 0 || phi.real() > M_PI_2)
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi.real()/M_PI_2));

        if (phi.real() < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_P(M_PI_2, k.real(), n.real(),0) + gsl_sf_ellint_P(fabs(phi.real())-nMultiple*M_PI_2, k.real(), n.real(), 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_P(M_PI_2, k.real(), n.real(),0) - gsl_sf_ellint_P(M_PI_2-(fabs(phi.real())-nMultiple*M_PI_2), k.real(), n.real(), 0));
    }

    return gsl_sf_ellint_P(phi.real(), k.real(), n.real(), 0);
}


static std::complex<double> EllipticD_impl(const std::complex<double>& phi, const std::complex<double>& k)
{
    if (mu::isnan(k.real()) || mu::isnan(phi.real()) || mu::isinf(k.real()) || mu::isinf(phi.real()))
        return NAN;

    if (k.real() < 0 || k.real() >= 1)
        return NAN;

#ifdef NR_HAVE_GSL2
    if (phi.real() < 0 || phi.real() > M_PI_2)
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi.real()/M_PI_2));

        if (phi.real() < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_D(M_PI_2, k.real(), 0) + gsl_sf_ellint_D(fabs(phi.real())-nMultiple*M_PI_2, k.real(), 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_D(M_PI_2, k.real(), 0) - gsl_sf_ellint_D(M_PI_2-(fabs(phi.real())-nMultiple*M_PI_2), k.real(), 0));
    }

    return gsl_sf_ellint_D(phi.real(), k.real(), 0);
#else
    if (phi.real() < 0 || phi.real() > M_PI_2)
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi.real()/M_PI_2));

        if (phi.real() < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_D(M_PI_2, k.real(), 0.0, 0) + gsl_sf_ellint_D(fabs(phi.real())-nMultiple*M_PI_2, k.real(), 0.0, 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_D(M_PI_2, k.real(), 0.0, 0) - gsl_sf_ellint_D(M_PI_2-(fabs(phi.real())-nMultiple*M_PI_2), k.real(), 0.0, 0));
    }

    return gsl_sf_ellint_D(phi.real(), k.real(), 0.0, 0);
#endif

}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal F(phi,k).
///
/// \param phic const mu::Array&
/// \param kc const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_EllipticF(const mu::Array& phic, const mu::Array& kc)
{
    return mu::apply(EllipticF_impl, phic, kc);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal E(phi,k).
///
/// \param phic const mu::Array&
/// \param kc const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_EllipticE(const mu::Array& phic, const mu::Array& kc)
{
    return mu::apply(EllipticE_impl, phic, kc);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal Pi(phi,n,k).
///
/// \param phi const mu::Array&
/// \param n const mu::Array&
/// \param k const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_EllipticP(const mu::Array& phi, const mu::Array& n, const mu::Array& k)
{
    return mu::apply(EllipticP_impl, phi, n, k);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal D(phi,k).
///
/// \param phi const mu::Array&
/// \param k const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_EllipticD(const mu::Array& phi, const mu::Array& k)
{
    return mu::apply(EllipticD_impl, phi, k);
}


static std::complex<double> beta_impl(const std::complex<double>& a, const std::complex<double>& b)
{
    if (mu::isnan(a.real()) || mu::isnan(b.real()) || mu::isinf(a.real()) || mu::isinf(b.real()))
        return NAN;

    if ((intCast(a) == (int)a.real() && a.real() < 0) || (intCast(b) == (int)b.real() && b.real() < 0))
        return NAN;

    return gsl_sf_beta(a.real(), b.real());
}

/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Beta function.
///
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_beta(const mu::Array& a, const mu::Array& b)
{
    return mu::apply(beta_impl, a, b);
}


/////////////////////////////////////////////////
/// \brief Calculates the sum of binomial
/// coefficients from k to N.
///
/// \param k int
/// \param N int
/// \return double
///
/////////////////////////////////////////////////
static double ek(int k, int N)
{
    double sum = 0;
    static std::vector<double> vLookUp;

    if ((int)vLookUp.size() > k-1)
        return vLookUp[k-1];

    for (int j = k; j <= N; j++)
        sum += binom_impl((double)N, (double)j).real();

    vLookUp.push_back(sum);

    return sum;
}


/////////////////////////////////////////////////
/// \brief Calculates the complex-valued
/// Riemannian Zeta function for complex numbers
/// with Re(s) >= 0.
///
/// \param s const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> complex_zeta(const std::complex<double>& s)
{
    if (s == 1.0)
        return NAN;

    std::complex<double> sum;
    static const int N = 20;
    static const double coeff = intPower(0.5, N);

    for (int k = 1; k <= N; k++)
    {
        sum += ((k-1) % 2 ? -1.0 : 1.0) / std::pow(k, s);
    }

    for (int k = N+1; k <= 2*N; k++)
    {
        sum += coeff * ((k-1) % 2 ? -1.0 : 1.0) * ek(k-N, N) / std::pow(k, s);
    }

    return 1.0 / (1.0-std::pow(2.0, 1.0-s)) * sum;
}


static std::complex<double> zeta_impl(const std::complex<double>& s)
{
    if (mu::isnan(s) || mu::isinf(s))
        return NAN;

    // Use the functional equation to swap negative
    // real numbers into the positive half-plane
    if (s.real() < 0.0)
        return std::pow(2.0, s)*std::pow(M_PI, s-1.0)*sin(0.5*M_PI*s)*gamma_impl(1.0-s)*complex_zeta(1.0-s);

    return complex_zeta(s);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Zeta function.
///
/// \param s const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_zeta(const mu::Array& s)
{
    return mu::apply(zeta_impl, s);
}


static std::complex<double> clausen_impl(const std::complex<double>& x)
{
    if (mu::isnan(x.real()) || mu::isinf(x.real()))
        return NAN;

    return gsl_sf_clausen(x.real());
}



/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Clausen function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_clausen(const mu::Array& x)
{
    return mu::apply(clausen_impl, x);
}


static std::complex<double> digamma_impl(const std::complex<double>& x)
{
    if (mu::isnan(x.real()) || mu::isinf(x.real()))
        return NAN;

    if (x.real() == 0.0)
        return NAN;

    if ((int)x.real() == intCast(x) && x.real() > 0)
        return gsl_sf_psi_int(intCast(x));
    else
        return gsl_sf_psi(x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Digamma function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_digamma(const mu::Array& x)
{
    return mu::apply(digamma_impl, x);
}


static std::complex<double> polygamma_impl(const std::complex<double>& n, const std::complex<double>& x)
{
    if (mu::isnan(n.real()) || mu::isnan(x.real()) || mu::isinf(n.real()) || mu::isinf(x.real()) || x.real() <= 0 || n.real() < 0)
        return NAN;

    return gsl_sf_psi_n(intCast(n), x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the value if the
/// Polygamma function.
///
/// \param n const mu::Array&
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_polygamma(const mu::Array& n, const mu::Array& x)
{
    return mu::apply(polygamma_impl, n, x);
}


static std::complex<double> dilogarithm_impl(const std::complex<double>& x)
{
    if (mu::isnan(x) || mu::isinf(x))
        return NAN;

    if (x.imag() == 0.0)
        return gsl_sf_dilog(x.real());

    gsl_sf_result re;
    gsl_sf_result im;

    gsl_sf_complex_dilog_xy_e(x.real(), x.imag(), &re, &im);
    return std::complex<double>(re.val, im.val);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Dilogarithm Li2(x).
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_dilogarithm(const mu::Array& x)
{
    return mu::apply(dilogarithm_impl, x);
}


static std::complex<double> floor_impl(const std::complex<double>& x)
{
    return std::complex<double>(std::floor(x.real()), std::floor(x.imag()));
}

static std::complex<double> roof_impl(const std::complex<double>& x)
{
    return std::complex<double>(std::ceil(x.real()), std::ceil(x.imag()));
}


/////////////////////////////////////////////////
/// \brief This is the floor function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_floor(const mu::Array& x)
{
    return mu::apply(floor_impl, x);
}


/////////////////////////////////////////////////
/// \brief This is the roof (ceil) function.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_roof(const mu::Array& x)
{
    return mu::apply(roof_impl, x);
}


/////////////////////////////////////////////////
/// \brief This is the rect function.
///
/// \param x const mu::Array&
/// \param x0 const mu::Array&
/// \param x1 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_rect(const mu::Array& x, const mu::Array& x0, const mu::Array& x1)
{
    return x > x0 && x < x1;
}

static std::complex<double> ivl_impl(const std::complex<double>& x, const std::complex<double>& x0, const std::complex<double>& x1, const std::complex<double>& lborder, const std::complex<double>& rborder)
{
    double lb = lborder.real();
    double rb = rborder.real();
    if (lb < 0)
        lb = 0;

    if (lb > 2)
        lb = 2;

    if (rb < 0)
        rb = 0;

    if (rb > 2)
        rb = 2;

    if (std::isnan(x.real()))
        return 0;

    if (x.real() < x0.real() && lb)
        return 0;
    else if (x.real() < x0.real())
        return 1;

    if (x.real() == x0.real() && lb != 2)
        return 1;
    else if (x.real() == x0.real() && lb == 2)
        return 0;

    if (x.real() > x1.real() && rb)
        return 0;
    else if (x.real() > x1.real())
        return 1;

    if (x.real() == x1.real() && rb != 2)
        return 1;
    else if (x.real() == x1.real() && rb == 2)
        return 0;

    return 1;
}

/////////////////////////////////////////////////
/// \brief This function describes an interval
/// with borders of a selected type (including,
/// excluding, ignore).
///
/// \param x const mu::Array&
/// \param x0 const mu::Array&
/// \param x1 const mu::Array&
/// \param lborder const mu::Array&
/// \param rborder const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_ivl(const mu::Array& x, const mu::Array& x0, const mu::Array& x1, const mu::Array& lborder, const mu::Array& rborder)
{
    return mu::apply(ivl_impl, x, x0, x1, lborder, rborder);
}


static std::complex<double> studentFactor_impl(const std::complex<double>& vFreedoms, const std::complex<double>& vAlpha)
{
    if (vAlpha.real() >= 1.0 || vAlpha.real() <= 0.0 || vFreedoms.real() < 2.0)
        return NAN;

    return student_t(intCast(vFreedoms), vAlpha.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the Student
/// factor s_t for the selected degrees of
/// freedom and a confidence interval.
///
/// \param vFreedoms const mu::Array&
/// \param vAlpha const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_studentFactor(const mu::Array& vFreedoms, const mu::Array& vAlpha)
{
    return mu::apply(studentFactor_impl, vFreedoms, vAlpha);
}

static std::complex<double> gcd_impl(const std::complex<double>& n, const std::complex<double>& k)
{
    return boost::math::gcd(intCast(n), intCast(k));
}

static std::complex<double> lcm_impl(const std::complex<double>& n, const std::complex<double>& k)
{
    return boost::math::lcm(intCast(n), intCast(k));
}

/////////////////////////////////////////////////
/// \brief This function returns the greatest
/// common divisor of both argments.
///
/// \param n const mu::Array&
/// \param k const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_gcd(const mu::Array& n, const mu::Array& k)
{
    return mu::apply(gcd_impl, n, k);
}


/////////////////////////////////////////////////
/// \brief This function returns the least common
/// multiple of both arguments.
///
/// \param n const mu::Array&
/// \param k const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_lcm(const mu::Array& n, const mu::Array& k)
{
    return mu::apply(lcm_impl, n, k);
}


static std::complex<double> mod_impl(const std::complex<double>& v1, const std::complex<double>& v2)
{
    if (std::isinf(v2.real()) || std::isnan(v2.real()) || std::isinf(v2.imag()) || std::isnan(v2.imag()))
        return NAN;

    std::complex<double> div = v1 / v2;
    div = std::complex<double>(std::floor(div.real()),
                               std::floor(div.imag()));

    return v1 - div * v2;
}


/////////////////////////////////////////////////
/// \brief This function represents the Modulo
/// operator.
///
/// \param v1 const mu::Array&
/// \param v2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_Mod(const mu::Array& v1, const mu::Array& v2)
{
    return mu::apply(mod_impl, v1, v2);
}


static std::complex<double> xor_impl(const std::complex<double>& v1, const std::complex<double>& v2)
{
    if (mu::isinf(v1) || mu::isnan(v1) || mu::isinf(v2) || mu::isnan(v2))
        return NAN;

    return (v1 != 0.0) xor (v2 != 0.0);
}


static std::complex<double> binOr_impl(const std::complex<double>& v1, const std::complex<double>& v2)
{
    if (mu::isinf(v1) || mu::isnan(v1) || mu::isinf(v2) || mu::isnan(v2))
        return NAN;

    return intCast(v1) | intCast(v2);
}


static std::complex<double> binAnd_impl(const std::complex<double>& v1, const std::complex<double>& v2)
{
    if (mu::isinf(v1) || mu::isnan(v1) || mu::isinf(v2) || mu::isnan(v2))
        return NAN;

    return intCast(v1) & intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function represent the XOR
/// operator.
///
/// \param v1 const mu::Array&
/// \param v2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_XOR(const mu::Array& v1, const mu::Array& v2)
{
    return mu::apply(xor_impl, v1, v2);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// OR operator.
///
/// \param v1 const mu::Array&
/// \param v2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_BinOR(const mu::Array& v1, const mu::Array& v2)
{
    return mu::apply(binOr_impl, v1, v2);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// AND operator.
///
/// \param v1 const mu::Array&
/// \param v2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array oprt_BinAND(const mu::Array& v1, const mu::Array& v2)
{
    return mu::apply(binAnd_impl, v1, v2);
}


/////////////////////////////////////////////////
/// \brief This function is a numerical version
/// of the string is_string() function. Used as a
/// fallback.
///
/// \param v const mu::Array&
/// \return const mu::Array&
///
/////////////////////////////////////////////////
mu::Array numfnc_is_string(const mu::Array& v)
{
    mu::Array res;

    for (const auto& val : v)
    {
        switch (val.getType())
        {
            case mu::TYPE_VOID:
                res.push_back(mu::Value());
                break;
            case mu::TYPE_STRING:
                res.push_back(mu::Value(true));
                break;
            default:
                res.push_back(mu::Value(false));
        }
    }

    return res;
}


/////////////////////////////////////////////////
/// \brief This function returns the current UNIX
/// time.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_time()
{
    return mu::Value(sys_time_now());
}


/////////////////////////////////////////////////
/// \brief This function returns the current CPU
/// clock count.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_clock()
{
    return mu::Value((int64_t)clock());
}


/////////////////////////////////////////////////
/// \brief Sleeps for the passed amount of
/// milliseconds and returns this number.
///
/// \param ms const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_sleep(const mu::Array& ms)
{
    int64_t msec = ms.front().getNum().asI64();
    Sleep(msec);
    return mu::Value(msec);
}


mu::Array numfnc_exp(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Exp, a);
}


static std::complex<double> numfnc_abs(const std::complex<double>& val)
{
    if (val.imag() == 0.0)
        return std::abs(val.real());

    return std::abs(val);
}

mu::Array numfnc_abs(const mu::Array& a)
{
    return mu::apply(numfnc_abs, a);
}


mu::Array numfnc_sqrt(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Sqrt, a);
}


static std::complex<double> numfnc_sign(const std::complex<double>& val)
{
    return std::complex<double>(val.real() == 0.0 ? 0 : (val.real() > 0.0 ? 1.0 : -1.0),
                                val.imag() == 0.0 ? 0 : (val.imag() > 0.0 ? 1.0 : -1.0));
}

mu::Array numfnc_sign(const mu::Array& a)
{
    return mu::apply(numfnc_sign, a);
}

mu::Array numfnc_log2(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Log2, a);
}


mu::Array numfnc_log10(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Log10, a);
}


mu::Array numfnc_ln(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Log, a);
}


static std::complex<double> log_b_impl(const std::complex<double>& b, const std::complex<double>& x)
{
    if (std::isnan(b.real()) || mu::isnan(x) || std::isinf(b.real()) || x.real() <= 0.0 || b.real() <= 0.0)
        return NAN;

    if (mu::isinf(x))
        return INFINITY;

    return log10(x) / log10(b.real());
}


/////////////////////////////////////////////////
/// \brief Calculates the logarithm of x using
/// the base b.
///
/// \param b const mu::Array&
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_log_b(const mu::Array& b, const mu::Array& x)
{
    return mu::apply(log_b_impl, b, x);
}


/////////////////////////////////////////////////
/// \brief Returns the version number of NumeRe
/// as a natural number.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_numereversion()
{
    return mu::Value(100.0*AutoVersion::MAJOR+10.0*AutoVersion::MINOR + AutoVersion::BUILD + std::atof(AutoVersion::UBUNTU_VERSION_STYLE) / 100.0);
}


/////////////////////////////////////////////////
/// \brief Returns the number of available OMP
/// threads.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_omp_threads()
{
    return mu::Value(omp_get_max_threads());
}


static mu::Value date_impl(const mu::Value& vTime, const mu::Value& vType)
{
    sys_time_point tp = to_timePoint(vTime.getNum().asF64());
    int nType = 0;

    if (vType.getType() == mu::TYPE_STRING)
    {
        const std::string& sType = vType.getStr();

        if (sType == "ymd")
            nType = -1;
        else if (sType == "hms")
            nType = -2;
        else if (sType == "year")
            nType = 1;
        else if (sType == "mth")
            nType = 2;
        else if (sType == "day")
            nType = 3;
        else if (sType == "hour")
            nType = 4;
        else if (sType == "min")
            nType = 5;
        else if (sType == "sec")
            nType = 6;
    }
    else
        nType = std::rint(vType.getNum().asI64());

    time_stamp ltm = getTimeStampFromTimePoint(tp);

    switch (nType)
    {
        case 1:
            return int(ltm.m_ymd.year());
        case 2:
            return unsigned(ltm.m_ymd.month());
        case 3:
            return unsigned(ltm.m_ymd.day());
        case 4:
            return ltm.m_hours.count();
        case 5:
            return ltm.m_minutes.count();
        case 6:
            return ltm.m_seconds.count();
        case 7:
            return ltm.m_millisecs.count();
        case 8:
            return ltm.m_microsecs.count();
        case -1:
            return int(ltm.m_ymd.year())*10000.0 + unsigned(ltm.m_ymd.month())*100.0 + unsigned(ltm.m_ymd.day());
        case -2:
            return ltm.m_hours.count()*10000.0 + ltm.m_minutes.count()*100.0 + ltm.m_seconds.count() + ltm.m_millisecs.count()*1.0e-3;
        default:
            return (int(ltm.m_ymd.year())*10000.0 + unsigned(ltm.m_ymd.month())*100.0 + unsigned(ltm.m_ymd.day()))*1000000.0
                + ltm.m_hours.count()*10000.0 + ltm.m_minutes.count()*100.0 + ltm.m_seconds.count() + ltm.m_millisecs.count()*1.0e-3;
    }

    return 0;
}


static std::complex<double> weeknum_impl(const std::complex<double>& vTime)
{
    return getWeekNum(to_timePoint(vTime.real()));
}


/////////////////////////////////////////////////
/// \brief This function converts UNIX time
/// values into a selected part of a time stamp.
///
/// \param vTime const mu::Array&
/// \param vType const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_date(const mu::Array& vTime, const mu::Array& vType)
{
    return mu::apply(date_impl, vTime, vType);
}


/////////////////////////////////////////////////
/// \brief This function returns the calendar
/// week associated with the passed time value.
///
/// \param vTime const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_weeknum(const mu::Array& vTime)
{
    return mu::apply(weeknum_impl, vTime);
}


/////////////////////////////////////////////////
/// \brief Returns, whether the selected value is
/// NaN.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_isnan(const mu::Array& v)
{
    return v != v;
}


static std::complex<double> interval_impl(const std::complex<double>& v, const std::complex<double>& vLeft, const std::complex<double>& vRight)
{
    if (vRight.real() <= vLeft.real())
        return NAN;

    if (v.real() <= vRight.real() && v.real() >= vLeft.real())
        return v.real();

    return NAN;
}


static std::complex<double> cot_impl(const std::complex<double>& x)
{
    if (mu::isnan(x) || mu::isinf(x))
        return NAN;

    if (sin(x) == 0.0)
        return INFINITY;

    return std::cos(x) / std::sin(x);
}


/////////////////////////////////////////////////
/// \brief This function numerically defines a
/// valid value range (the value is set to NaN,
/// if outside of this range).
///
/// \param v const mu::Array&
/// \param vLeft const mu::Array&
/// \param vRight const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_interval(const mu::Array& v, const mu::Array& vLeft, const mu::Array& vRight)
{
    return mu::apply(interval_impl, v, vLeft, vRight);
}


/////////////////////////////////////////////////
/// \brief This function returns the cotangent of
/// the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_cot(const mu::Array& x)
{
    return mu::apply(cot_impl, x);
}


mu::Array numfnc_sin(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Sin, a);
}


mu::Array numfnc_cos(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Cos, a);
}


mu::Array numfnc_tan(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Tan, a);
}


mu::Array numfnc_asin(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ASin, a);
}


mu::Array numfnc_acos(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ACos, a);
}


mu::Array numfnc_atan(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ATan, a);
}


mu::Array numfnc_sinh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Sinh, a);
}


mu::Array numfnc_cosh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Cosh, a);
}


mu::Array numfnc_tanh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::Tanh, a);
}


mu::Array numfnc_asinh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ASinh, a);
}


mu::Array numfnc_acosh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ACosh, a);
}


mu::Array numfnc_atanh(const mu::Array& a)
{
    return mu::apply(mu::MathImpl<std::complex<double>>::ATanh, a);
}


/////////////////////////////////////////////////
/// \brief This function returns the secant of
/// the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_sec(const mu::Array& x)
{
    return mu::Value(1.0) / mu::apply(std::cos, x);
}


/////////////////////////////////////////////////
/// \brief This function returns the cosecant of
/// the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_csc(const mu::Array& x)
{
    return mu::Value(1.0) / mu::apply(std::sin, x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// secant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_asec(const mu::Array& x)
{
    return mu::apply(std::acos, mu::Value(1.0) / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// cosecant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_acsc(const mu::Array& x)
{
    return mu::apply(std::asin, mu::Value(1.0) / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the hyperbolic
/// secant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_sech(const mu::Array& x)
{
    return mu::Value(1.0) / mu::apply(std::cosh, x);
}


/////////////////////////////////////////////////
/// \brief This function returns the hyperbolic
/// cosecant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_csch(const mu::Array& x)
{
    return mu::Value(1.0) / mu::apply(std::sinh, x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// hyperbolic secant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_asech(const mu::Array& x)
{
    return mu::apply(std::acosh, mu::Value(1.0) / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// hyperbolic cosecant of the passed value.
///
/// \param x const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_acsch(const mu::Array& x)
{
    return mu::apply(std::asinh, mu::Value(1.0) / x);
}


static mu::Value as_date_impl(const mu::Value& year, const mu::Value& month, const mu::Value& day)
{
    time_stamp ts;
    ts.m_ymd = date::year{year.getNum().asI64()}/date::month{month.getNum().asI64()}/date::day{day.getNum().asI64()};
    return mu::Value(getTimePointFromTimeStamp(ts));
}


static mu::Value as_time_impl(const mu::Value& hours, const mu::Value& minutes, const mu::Value& seconds, const mu::Value& milliseconds, const mu::Value& microseconds)
{
    time_stamp ts;
    ts.m_hours = std::chrono::hours(hours.getNum().asI64());
    ts.m_minutes = std::chrono::minutes(minutes.getNum().asI64());
    ts.m_seconds = std::chrono::seconds(seconds.getNum().asI64());
    ts.m_millisecs = std::chrono::milliseconds(milliseconds.getNum().asI64());
    ts.m_microsecs = std::chrono::microseconds(microseconds.getNum().asI64());

    date::year y{1970u};
    date::month m{1u};
    date::day d{1u};
    ts.m_ymd = y/m/d;

    return mu::Value(getTimePointFromTimeStamp(ts));
}


/////////////////////////////////////////////////
/// \brief This function returns the date from
/// the passed vElements.
///
/// \param year const mu::Array&
/// \param month const mu::Array& OPTIONAL
/// \param day const mu::Array& OPTIONAL
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_as_date(const mu::Array& year, const mu::Array& month, const mu::Array& day)
{
    return mu::apply(as_date_impl, year,
                     month.isDefault() ? mu::Value(1.0) : month,
                     day.isDefault() ? mu::Value(1.0) : day);
}


/////////////////////////////////////////////////
/// \brief This function returns the time from
/// the passed vElements
///
/// \param h const mu::Array&
/// \param m const mu::Array& OPTIONAL
/// \param s const mu::Array& OPTIONAL
/// \param ms const mu::Array& OPTIONAL
/// \param mus const mu::Array& OPTIONAL
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array timfnc_as_time(const mu::Array& h, const mu::Array& m, const mu::Array& s, const mu::Array& ms, const mu::Array& mus)
{
    return mu::apply(as_time_impl, h,
                     m.isDefault() ? mu::Value(0.0) : m,
                     s.isDefault() ? mu::Value(0.0) : s,
                     ms.isDefault() ? mu::Value(0.0) : ms,
                     mus.isDefault() ? mu::Value(0.0) : mus);
}

/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Laplace distribution function.
///
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_rd(const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(a) || a.imag() != 0 || a.real() <= 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_laplace(getGslRandGenInstance(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Laplace distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_pdf(const std::complex<double>& x, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(a) || x.imag() != 0 || a.imag() != 0 || a.real() == 0.0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_laplace_pdf(x.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Laplace distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_cdf_p(const std::complex<double>& x, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(a) || x.imag() != 0 || a.imag() != 0 || a.real() <= 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_laplace_P(x.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Laplace distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_cdf_q(const std::complex<double>& x, const std::complex<double>& a)
{
    // Get the result from the existing p variant
    return std::complex<double>(1) - rndfnc_laplace_cdf_p(x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Laplace distribution function.
///
/// \param p const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_inv_p(const std::complex<double>& p, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(p) || mu::isnan(a) || p.imag() != 0 || a.imag() != 0 || a.real() <= 0 || p.real() < 0 || p.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_laplace_Pinv(p.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Laplace distribution function.
///
/// \param q const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_laplace_inv_q(const std::complex<double>& q, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(q) || mu::isnan(a) || q.imag() != 0 || a.imag() != 0 || a.real() <= 0 || q.real() < 0 || q.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_laplace_Qinv(q.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Cauchy distribution function.
///
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_rd(const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(a) || a.imag() != 0 || a.real() <= 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_cauchy(getGslRandGenInstance(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Cauchy distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_pdf(const std::complex<double>& x, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(a) || x.imag() != 0 || a.imag() != 0 || a.real() == 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_cauchy_pdf(x.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Cauchy distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_cdf_p(const std::complex<double>& x, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(a) || x.imag() != 0 || a.imag() != 0 || a.real() <= 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_cauchy_P(x.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Cauchy distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_cdf_q(const std::complex<double>& x, const std::complex<double>& a)
{
    // Get the result from the existing p variant
    return std::complex<double>(1) - rndfnc_cauchy_cdf_p(x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Cauchy distribution function.
///
/// \param p const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_inv_p(const std::complex<double>& p, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(p) || mu::isnan(a) || p.imag() != 0 || a.imag() != 0 || a.real() <= 0 || p.real() < 0 || p.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_cauchy_Pinv(p.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Cauchy distribution function.
///
/// \param q const std::complex<double>&
/// \param a const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_cauchy_inv_q(const std::complex<double>& q, const std::complex<double>& a)
{
    // Check the input values
    if (mu::isnan(q) || mu::isnan(a) || q.imag() != 0 || a.imag() != 0 || a.real() <= 0 || q.real() < 0 || q.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_cauchy_Qinv(q.real(), a.real());
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Rayleigh distribution function.
///
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_rd(const std::complex<double>& sigma)
{
    // Check the input values
    if (mu::isnan(sigma) || sigma.imag() != 0 || sigma.real() <= 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_rayleigh(getGslRandGenInstance(), sigma.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Rayleigh distribution function.
///
/// \param x const std::complex<double>&
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_pdf(const std::complex<double>& x, const std::complex<double>& sigma)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(sigma) || x.imag() != 0 || sigma.imag() != 0 || sigma.real() == 0 || x.real() < 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_rayleigh_pdf(x.real(), sigma.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Rayleigh distribution function.
///
/// \param x const std::complex<double>&
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_cdf_p(const std::complex<double>& x, const std::complex<double>& sigma)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(sigma) || x.imag() != 0 || sigma.imag() != 0 || sigma.real() <= 0 || x.real() < 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_rayleigh_P(x.real(), sigma.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Rayleigh distribution function.
///
/// \param x const std::complex<double>&
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_cdf_q(const std::complex<double>& x, const std::complex<double>& sigma)
{
    // Get the result from the existing p variant
    return std::complex<double>(1) - rndfnc_rayleigh_cdf_p(x, sigma);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Rayleigh distribution function.
///
/// \param p const std::complex<double>&
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_inv_p(const std::complex<double>& p, const std::complex<double>& sigma)
{
    // Check the input values
    if (mu::isnan(p) || mu::isnan(sigma) || p.imag() != 0 || sigma.imag() != 0 || sigma.real() <= 0 || p.real() < 0 || p.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_rayleigh_Pinv(p.real(), sigma.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Rayleigh distribution function.
///
/// \param q const std::complex<double>&
/// \param sigma const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_rayleigh_inv_q(const std::complex<double>& q, const std::complex<double>& sigma)
{
    // Check the input values
    if (mu::isnan(q) || mu::isnan(sigma) || q.imag() != 0 || sigma.imag() != 0 || sigma.real() <= 0 || q.real() < 0 || q.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_rayleigh_Qinv(q.real(), sigma.real());
}


/////////////////////////////////////////////////
/// \brief his function computes the probability
/// density function p at a given position for the
/// Landau distribution function.
///
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_landau_pdf(const std::complex<double>& x)
{
    // Check the input values
    if (mu::isnan(x) || x.imag() != 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_landau_pdf(x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Levy alpha-stable distribution function.
///
/// \param c const std::complex<double>&
/// \param alpha const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_levyAlphaStable_rd(const std::complex<double>& c, const std::complex<double>& alpha)
{
    // Check the input values
    if (mu::isnan(c) || c.imag() != 0 || c.real() < 0 || mu::isnan(alpha) || alpha.imag() != 0 || alpha.real() <= 0 || alpha.real() > 2)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_levy(getGslRandGenInstance(), c.real(), alpha.real());
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Fisher F-distribution function.
///
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_rd(const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Check the input values
    if (mu::isnan(nu1) || nu1.imag() != 0 || nu1.real() <= 0 || mu::isnan(nu2) || nu2.imag() != 0 || nu2.real() <= 0 || !isInt(nu1.real()) || !isInt(nu2.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_fdist(getGslRandGenInstance(), intCast(nu1.real()), intCast(nu2.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Fisher F-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_pdf(const std::complex<double>& x, const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(nu1) || mu::isnan(nu2) || x.imag() != 0 || nu1.imag() != 0 || nu2.imag() != 0 || nu1.real() <= 0 || nu2.real() <= 0 || x.real() < 0 || !isInt(nu1.real()) || !isInt(nu2.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_fdist_pdf(x.real(), intCast(nu1.real()), intCast(nu2.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Fisher F-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_cdf_p(const std::complex<double>& x, const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(nu1) || mu::isnan(nu2) || x.imag() != 0 || nu1.imag() != 0 || nu2.imag() != 0 || nu1.real() <= 0 || nu2.real() <= 0 || x.real() < 0 || !isInt(nu1.real()) || !isInt(nu2.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_fdist_P(x.real(), intCast(nu1.real()), intCast(nu2.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Fisher F-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_cdf_q(const std::complex<double>& x, const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Get the result from the existing p variant
    return std::complex<double>(1) - rndfnc_fisher_f_cdf_p(x, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Fisher F-distribution function.
///
/// \param p const std::complex<double>&
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_inv_p(const std::complex<double>& p, const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Check the input values
    if (mu::isnan(p) || mu::isnan(nu1) || mu::isnan(nu2) || p.imag() != 0 || nu1.imag() != 0 || nu2.imag() != 0 || p.real() < 0 || p.real() > 1 || nu1.real() <= 0 || nu2.real() <= 0 || !isInt(nu1.real()) || !isInt(nu2.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_fdist_Pinv(p.real(), intCast(nu1.real()), intCast(nu2.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Fisher F-distribution function.
///
/// \param q const std::complex<double>&
/// \param nu1 const std::complex<double>&
/// \param nu2 const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_fisher_f_inv_q(const std::complex<double>& q, const std::complex<double>& nu1, const std::complex<double>& nu2)
{
    // Check the input values
    if (mu::isnan(q) || mu::isnan(nu1) || mu::isnan(nu2) || q.imag() != 0 || nu1.imag() != 0 || nu2.imag() != 0 || q.real() < 0 || q.real() > 1 || nu1.real() <= 0 || nu2.real() <= 0 || !isInt(nu1.real()) || !isInt(nu2.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_fdist_Qinv(q.real(), intCast(nu1.real()), intCast(nu2.real()));
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Weibull-distribution function.
///
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_rd(const std::complex<double>& a, const std::complex<double>& b)
{
    // Check the input values
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b))
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_weibull(getGslRandGenInstance(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Weibull-distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_pdf(const std::complex<double>& x, const std::complex<double>& a, const std::complex<double>& b)
{
    // Check the input values
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b) || mu::isnan(x) || x.real() < 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_weibull_pdf(x.real(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Weibull-distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_cdf_p(const std::complex<double>& x, const std::complex<double>& a, const std::complex<double>& b)
{
    // Check the input values
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b) || mu::isnan(x) || x.real() < 0)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_weibull_P(x.real(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Weibull-distribution function.
///
/// \param x const std::complex<double>&
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_cdf_q(const std::complex<double>& x, const std::complex<double>& a, const std::complex<double>& b)
{
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b) || mu::isnan(x) || x.real() < 0)
        return NAN;

    // Get the result from the probability density function
    return gsl_cdf_weibull_Q(x.real(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Weibull-distribution function.
///
/// \param p const std::complex<double>&
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_inv_p(const std::complex<double>& p, const std::complex<double>& a, const std::complex<double>& b)
{
    // Check the input values
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b) || mu::isnan(p) || p.real() < 0 || p.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_weibull_Pinv(p.real(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Weibull-distribution function.
///
/// \param q const std::complex<double>&
/// \param a const std::complex<double>&
/// \param b const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_weibull_inv_q(const std::complex<double>& q, const std::complex<double>& a, const std::complex<double>& b)
{
    // Check the input values
    if (mu::isnan(a) || a.real() == 0 || mu::isnan(b) || mu::isnan(q) || q.real() < 0 || q.real() > 1)
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_weibull_Qinv(q.real(), a.real(), b.real());
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Student t-distribution function.
///
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_rd(const std::complex<double>& nu)
{
    // Check the input values
    if (mu::isnan(nu) || nu.imag() != 0 || nu.real() <= 0 || !isInt(nu.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_tdist(getGslRandGenInstance(), intCast(nu.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Student t-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_pdf(const std::complex<double>& x, const std::complex<double>& nu)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(nu) || x.imag() != 0 || nu.imag() != 0 || nu.real() <= 0 || !isInt(nu.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_ran_tdist_pdf(x.real(), intCast(nu.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Student t-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_cdf_p(const std::complex<double>& x, const std::complex<double>& nu)
{
    // Check the input values
    if (mu::isnan(x) || mu::isnan(nu) || x.imag() != 0 || nu.imag() != 0 || nu.real() <= 0 || !isInt(nu.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_tdist_P(x.real(), intCast(nu.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Student t-distribution function.
///
/// \param x const std::complex<double>&
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_cdf_q(const std::complex<double>& x, const std::complex<double>& nu)
{
    // Get the result from the existing p variant
    return std::complex<double>(1) - rndfnc_student_t_cdf_p(x, nu);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Student t-distribution function.
///
/// \param p const std::complex<double>&
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_inv_p(const std::complex<double>& p, const std::complex<double>& nu)
{
    // Check the input values
    if (mu::isnan(p) || mu::isnan(nu) || p.imag() != 0 || nu.imag() != 0 || p.real() < 0 || p.real() > 1 || nu.real() <= 0 || !isInt(nu.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_tdist_Pinv(p.real(), intCast(nu.real()));
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Student t-distribution function.
///
/// \param q const std::complex<double>&
/// \param nu const std::complex<double>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> rndfnc_student_t_inv_q(const std::complex<double>& q, const std::complex<double>& nu)
{
    // Check the input values
    if (mu::isnan(q) || mu::isnan(nu) || q.imag() != 0 || nu.imag() != 0 || q.real() < 0 || q.real() > 1 || nu.real() <= 0 || !isInt(nu.real()))
        return NAN;

    // Get the value from the probability density function
    return gsl_cdf_tdist_Qinv(q.real(), intCast(nu.real()));
}



/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Laplace distribution function.
///
/// \param a const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_rd(const mu::Array& a, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max(a.size(), nRandCount); i++)
    {
        ret.push_back(mu::Value(rndfnc_laplace_rd(a.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Laplace distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_pdf(const mu::Array& x, const mu::Array& a)
{
    return mu::apply(rndfnc_laplace_pdf, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Laplace distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_cdf_p(const mu::Array& x, const mu::Array& a)
{
    return mu::apply(rndfnc_laplace_cdf_p, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Laplace distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_cdf_q(const mu::Array& x, const mu::Array& a)
{
    // Get the result from the existing p variant
    return mu::apply(rndfnc_laplace_cdf_q, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Laplace distribution function.
///
/// \param p const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_inv_p(const mu::Array& p, const mu::Array& a)
{
    return mu::apply(rndfnc_laplace_inv_p, p, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Laplace distribution function.
///
/// \param q const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_laplace_inv_q(const mu::Array& q, const mu::Array& a)
{
    return mu::apply(rndfnc_laplace_inv_q, q, a);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Cauchy distribution function.
///
/// \param a const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_rd(const mu::Array& a, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max(a.size(), nRandCount); i++)
    {
        ret.push_back(mu::Value(rndfnc_cauchy_rd(a.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Cauchy distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_pdf(const mu::Array& x, const mu::Array& a)
{
    return mu::apply(rndfnc_cauchy_pdf, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Cauchy distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_cdf_p(const mu::Array& x, const mu::Array& a)
{
    return mu::apply(rndfnc_cauchy_cdf_p, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Cauchy distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_cdf_q(const mu::Array& x, const mu::Array& a)
{
    return mu::apply(rndfnc_cauchy_cdf_q, x, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Cauchy distribution function.
///
/// \param p const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_inv_p(const mu::Array& p, const mu::Array& a)
{
    return mu::apply(rndfnc_cauchy_inv_p, p, a);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Cauchy distribution function.
///
/// \param q const mu::Array&
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_cauchy_inv_q(const mu::Array& q, const mu::Array& a)
{
    return mu::apply(rndfnc_cauchy_inv_q, q, a);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Rayleigh distribution function.
///
/// \param sigma const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_rd(const mu::Array& sigma, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max(sigma.size(), nRandCount); i++)
    {
        ret.push_back(mu::Value(rndfnc_rayleigh_rd(sigma.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Rayleigh distribution function.
///
/// \param x const mu::Array&
/// \param sigma const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_pdf(const mu::Array& x, const mu::Array& sigma)
{
    return mu::apply(rndfnc_rayleigh_pdf, x, sigma);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Rayleigh distribution function.
///
/// \param x const mu::Array&
/// \param sigma const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_cdf_p(const mu::Array& x, const mu::Array& sigma)
{
    return mu::apply(rndfnc_rayleigh_cdf_p, x, sigma);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Rayleigh distribution function.
///
/// \param x const mu::Array&
/// \param sigma const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_cdf_q(const mu::Array& x, const mu::Array& sigma)
{
    return mu::apply(rndfnc_rayleigh_cdf_q, x, sigma);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Rayleigh distribution function.
///
/// \param p const mu::Array&
/// \param sigma const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_inv_p(const mu::Array& p, const mu::Array& sigma)
{
    return mu::apply(rndfnc_rayleigh_inv_p, p, sigma);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Rayleigh distribution function.
///
/// \param q const mu::Array&
/// \param sigma const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_rayleigh_inv_q(const mu::Array& q, const mu::Array& sigma)
{
    return mu::apply(rndfnc_rayleigh_inv_q, q, sigma);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Landau distribution function.
///
/// \param n const mu::Array& OPTIONAL
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_landau_rd(const mu::Array& n)
{
    if (n.isDefault())
    {
        // Get the value from the probability density function
        return mu::Value(gsl_ran_landau(getGslRandGenInstance()));
    }

    mu::Array ret;

    for (size_t i = 0; (int64_t)i < n.getAsScalarInt(); i++)
    {
        ret.push_back(mu::Value(gsl_ran_landau(getGslRandGenInstance())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief his function computes the probability
/// density function p at a given position for the
/// Landau distribution function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_landau_pdf(const mu::Array& x)
{
    return mu::apply(rndfnc_landau_pdf, x);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Levy alpha-stable distribution function.
///
/// \param c const mu::Array&
/// \param alpha const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_levyAlphaStable_rd(const mu::Array& c, const mu::Array& alpha, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max({c.size(), alpha.size(), nRandCount}); i++)
    {
        ret.push_back(mu::Value(rndfnc_levyAlphaStable_rd(c.get(i).getNum().asCF64(), alpha.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Fisher F-distribution function.
///
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_rd(const mu::Array& nu1, const mu::Array& nu2, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max({nu1.size(), nu2.size(), nRandCount}); i++)
    {
        ret.push_back(mu::Value(rndfnc_fisher_f_rd(nu1.get(i).getNum().asCF64(), nu2.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Fisher F-distribution function.
///
/// \param x const mu::Array&
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_pdf(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2)
{
    return mu::apply(rndfnc_fisher_f_pdf, x, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Fisher F-distribution function.
///
/// \param x const mu::Array&
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_cdf_p(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2)
{
    return mu::apply(rndfnc_fisher_f_cdf_p, x, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Fisher F-distribution function.
///
/// \param x const mu::Array&
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_cdf_q(const mu::Array& x, const mu::Array& nu1, const mu::Array& nu2)
{
    return mu::apply(rndfnc_fisher_f_cdf_q, x, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Fisher F-distribution function.
///
/// \param p const mu::Array&
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_inv_p(const mu::Array& p, const mu::Array& nu1, const mu::Array& nu2)
{
    return mu::apply(rndfnc_fisher_f_inv_p, p, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Fisher F-distribution function.
///
/// \param q const mu::Array&
/// \param nu1 const mu::Array&
/// \param nu2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_fisher_f_inv_q(const mu::Array& q, const mu::Array& nu1, const mu::Array& nu2)
{
    return mu::apply(rndfnc_fisher_f_inv_q, q, nu1, nu2);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Weibull-distribution function.
///
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_rd(const mu::Array& a, const mu::Array& b, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max({a.size(), b.size(), nRandCount}); i++)
    {
        ret.push_back(mu::Value(rndfnc_weibull_rd(a.get(i).getNum().asCF64(), b.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Weibull-distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_pdf(const mu::Array& x, const mu::Array& a, const mu::Array& b)
{
    return mu::apply(rndfnc_weibull_pdf, x, a, b);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Weibull-distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_cdf_p(const mu::Array& x, const mu::Array& a, const mu::Array& b)
{
    return mu::apply(rndfnc_weibull_cdf_p, x, a, b);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Weibull-distribution function.
///
/// \param x const mu::Array&
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_cdf_q(const mu::Array& x, const mu::Array& a, const mu::Array& b)
{
    return mu::apply(rndfnc_weibull_cdf_q, x, a, b);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Weibull-distribution function.
///
/// \param p const mu::Array&
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_inv_p(const mu::Array& p, const mu::Array& a, const mu::Array& b)
{
    return mu::apply(rndfnc_weibull_inv_p, p, a, b);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Weibull-distribution function.
///
/// \param q const mu::Array&
/// \param a const mu::Array&
/// \param b const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_weibull_inv_q(const mu::Array& q, const mu::Array& a, const mu::Array& b)
{
    return mu::apply(rndfnc_weibull_inv_q, q, a, b);
}


/////////////////////////////////////////////////
/// \brief This function returns a random value
/// from the Student t-distribution function.
///
/// \param nu const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_rd(const mu::Array& nu, const mu::Array& n)
{
    size_t nRandCount = 1;

    if (!n.isDefault())
        nRandCount = n.getAsScalarInt();

    mu::Array ret;

    for (size_t i = 0; i < std::max(nu.size(), nRandCount); i++)
    {
        ret.push_back(mu::Value(rndfnc_student_t_rd(nu.get(i).getNum().asCF64())));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief This function computes the probability
/// density function p at a given position for the
/// Student t-distribution function.
///
/// \param x const mu::Array&
/// \param nu const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_pdf(const mu::Array& x, const mu::Array& nu)
{
    return mu::apply(rndfnc_student_t_pdf, x, nu);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function p at a given position for the
/// Student t-distribution function.
///
/// \param x const mu::Array&
/// \param nu const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_cdf_p(const mu::Array& x, const mu::Array& nu)
{
    return mu::apply(rndfnc_student_t_cdf_p, x, nu);
}


/////////////////////////////////////////////////
/// \brief This function computes the cumulative
/// density function q at a given position for the
/// Student t-distribution function.
///
/// \param x const mu::Array&
/// \param nu const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_cdf_q(const mu::Array& x, const mu::Array& nu)
{
    return mu::apply(rndfnc_student_t_cdf_q, x, nu);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function p at a given
/// position for the Student t-distribution function.
///
/// \param p const mu::Array&
/// \param nu const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_inv_p(const mu::Array& p, const mu::Array& nu)
{
    return mu::apply(rndfnc_student_t_inv_p, p, nu);
}


/////////////////////////////////////////////////
/// \brief This function computes the inverse of
/// the cumulative density function q at a given
/// position for the Student t-distribution function.
///
/// \param q const mu::Array&
/// \param nu const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array rndfnc_student_t_inv_q(const mu::Array& q, const mu::Array& nu)
{
    return mu::apply(rndfnc_student_t_inv_q, q, nu);
}

