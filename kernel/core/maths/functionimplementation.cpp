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
#include <noise/noise.h>

#include "student_t.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "../utils/tools.hpp"
#include "../version.h"

using namespace std;

int nErrorIndices[2] = {-1,-1};
string sErrorToken = "";
extern time_t tTimeZero;
volatile sig_atomic_t exitsignal = 0;

/*
 * Ende der globalen Variablen
 */

// --> Umrechnungsfunktionen: diese werden aufgerufen, wenn eine spezielle Syntax verwendet wird <--
/////////////////////////////////////////////////
/// \brief Conversion function for 1M.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mega(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1m.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Milli(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1G.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Giga(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e9;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1k.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Kilo(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mu.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Micro(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1n.
///
/// \param a_fVal const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Nano(const value_type& a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-9;
}


/////////////////////////////////////////////////
/// \brief Function representing the logical NOT
/// operator.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Not(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v == 0.0;
}


/////////////////////////////////////////////////
/// \brief Identity function. Used for ignoring
/// functions and parameters in special cases.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Ignore(const value_type& v)
{
    return v;
}


// --> Einheitenumrechnung: eV, fm, A, b, Torr, AU, etc... <--
/////////////////////////////////////////////////
/// \brief Conversion function for 1eV.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ElectronVolt(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.60217657e-19;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1fm.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Fermi(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1A.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Angstroem(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bn.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Barn(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-28;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Torr/1mmhg.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Torr(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 101325.0/760.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1AU.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AstroUnit(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 149597870700.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ly.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Lightyear(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 9460730472580800.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1pc.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Parsec(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 30856775777948584.2;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mi.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mile(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1609.344;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1yd.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Yard(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.9144;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ft.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Foot(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.3048;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1in.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Inch(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.0254;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1cal.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Calorie(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 4.1868;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1psi.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_PSI(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6894.75729;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kn.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Knoten(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 463.0 / 900.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1l.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_liter(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kmh.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_kmh(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mph.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_mph(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.609334 / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°C.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Celsius(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v + 273.15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°F.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Fahrenheit(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return (v + 459.67) * 5.0 / 9.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1C.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Curie(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 3.7e10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Gs.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Gauss(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-4;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Ps.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Poise(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-1;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mol.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_mol(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6.022140857E23;
}


/////////////////////////////////////////////////
/// \brief Multiplies a number with the imaginary
/// unit.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_imaginaryUnit(const value_type& v)
{
    return value_type(v.imag() != 0.0 ? -v.imag() : 0.0, v.real());
}


/////////////////////////////////////////////////
/// \brief Extracts the real part of a complex
/// number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_real(const value_type& v)
{
    return v.real();
}


/////////////////////////////////////////////////
/// \brief Extracts the imaginary part of a
/// complex number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_imag(const value_type& v)
{
    return v.imag();
}


/////////////////////////////////////////////////
/// \brief Converts a rectangular representation
/// into polar representation and returns it as a
/// new complex number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_rect2polar(const value_type& v)
{
    return value_type(std::abs(v), std::arg(v));
}


/////////////////////////////////////////////////
/// \brief Converts a polar representation into a
/// rectangular representation and returns it as
/// a new complex number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_polar2rect(const value_type& v)
{
    return std::polar(v.real(), v.imag());
}


/////////////////////////////////////////////////
/// \brief Calculates the complex conjugate
/// number of the passed complex number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_conj(const value_type& v)
{
    return std::conj(v);
}


/////////////////////////////////////////////////
/// \brief Construct a complex number from two
/// real numbers.
///
/// \param re const value_type&
/// \param im const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_complex(const value_type& re, const value_type& im)
{
    return value_type(re.real(), im.real());
}


/////////////////////////////////////////////////
/// \brief Function representing the faculty of
/// any natural number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Faculty(const value_type& v)
{
    if (isnan(v) || isinf(v))
        return NAN;
    value_type vResult = 1.0; // Ausgabe-Variable
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
/// \brief Function representing the double
/// faculty of any natural number.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_doubleFaculty(const value_type& v)
{
    if (isnan(v) || isinf(v))
        return NAN;
    value_type vResult = 1.0;
    if (intCast(v) < 0)
        return NAN;
    for (int n = intCast(fabs(v)); n > 0; n -= 2)
    {
        vResult *= n;
    }
    return vResult;
}


/////////////////////////////////////////////////
/// \brief Function representing the binomial
/// coefficient.
///
/// \param v1 const value_type&
/// \param v2 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Binom(const value_type& v1, const value_type& v2)
{
    if (isnan(v1) || isnan(v2) || isinf(v1) || isinf(v2))
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
         *     parser_Faculty(value_type) aufrufen <--
         */
        return parser_Faculty(v1) / (parser_Faculty(v2)*parser_Faculty(intCast(v1) - intCast(v2)));
    }
}


/////////////////////////////////////////////////
/// \brief This function returns the number of
/// valid elements in its array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Num(const value_type* vElements, int nElements)
{
    int nReturn = nElements;

    for (int i = 0; i < nElements; i++)
    {
        if (isnan(vElements[i]) || isinf(vElements[i]))
            nReturn--;
    }

    return nReturn;
}


/////////////////////////////////////////////////
/// \brief This functioon simply returns the
/// number of elements in its array (even the
/// invalid ones).
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Cnt(const value_type* vElements, int nElements)
{
    return nElements;
}


/////////////////////////////////////////////////
/// \brief This function calculates the standard
/// deviation of the elements in the passed
/// array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Std(const value_type* vElements, int nElements)
{
    value_type vStd = 0.0;
    value_type vMean = parser_Avg(vElements, nElements);

    for (int i = 0; i < nElements; i++)
    {
        if (!isnan(vElements[i]))
            vStd += (vElements[i] - vMean) * conj(vElements[i] - vMean);
    }

    return sqrt(vStd / (parser_Num(vElements, nElements)-1.0));
}


/////////////////////////////////////////////////
/// \brief This function calculates the product
/// of all elements in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_product(const value_type* vElements, int nElements)
{
    value_type vProd = 1.0;

    for (int i = 0; i < nElements; i++)
    {
        if (!isnan(vElements[i]))
            vProd *= vElements[i];
    }

    return vProd;
}


/////////////////////////////////////////////////
/// \brief This function calculates the vector
/// norm of the elements in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Norm(const value_type* vElements, int nElements)
{
    value_type vResult = 0.0;

    for (int i = 0; i < nElements; i++)
    {
        if (!isnan(vElements[i]))
            vResult += vElements[i] * conj(vElements[i]);
    }

    return sqrt(vResult);
}


/////////////////////////////////////////////////
/// \brief This function calculates the median of
/// the elements in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Med(const value_type* vElements, int nElements)
{
    Memory _mem;

    for (int i = 0; i < nElements; i++)
        _mem.writeData(i, 0, vElements[i]);

    return _mem.med(VectorIndex(0, nElements-1), VectorIndex(0));
}


/////////////////////////////////////////////////
/// \brief This function calculates the selected
/// percentile of the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Pct(const value_type* vElements, int nElements)
{
    Memory _mem;

    for (int i = 0; i < nElements-1; i++)
        _mem.writeData(i, 0, vElements[i]);

    return _mem.pct(VectorIndex(0, nElements-2), VectorIndex(0), vElements[nElements-1]);
}


/////////////////////////////////////////////////
/// \brief This function searches for elements of
/// a specified type in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_compare(const value_type* vElements, int nElements)
{
    enum
    {
        RETURN_VALUE = 1,
        RETURN_LE = 2,
        RETURN_GE = 4,
        RETURN_FIRST = 8
    };

    int nType = 0;

    if (nElements < 3)
        return NAN;

    value_type vRef = vElements[nElements-2];
    value_type vKeep = vRef;
    int nKeep = -1;

    if (vElements[nElements-1].real() > 0)
        nType = RETURN_GE;
    else if (vElements[nElements-1].real() < 0)
        nType = RETURN_LE;

    switch (intCast(fabs(vElements[nElements-1])))
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

    for (int i = 0; i < nElements-2; i++)
    {
        if (isnan(vElements[i]) || isinf(vElements[i]))
            continue;

        if (vElements[i] == vRef)
        {
            if (nType & RETURN_VALUE)
                return vElements[i];

            return i+1;
        }
        else if (nType & RETURN_GE && vElements[i].real() > vRef.real())
        {
            if (nType & RETURN_FIRST)
            {
                if (nType & RETURN_VALUE)
                    return vElements[i].real();

                return i+1;
            }

            if (nKeep == -1 || vElements[i].real() < vKeep.real())
            {
                vKeep = vElements[i].real();
                nKeep = i;
            }
            else
                continue;
        }
        else if (nType & RETURN_LE && vElements[i].real() < vRef.real())
        {
            if (nType & RETURN_FIRST)
            {
                if (nType & RETURN_VALUE)
                    return vElements[i].real();

                return i+1;
            }

            if (nKeep == -1 || vElements[i].real() > vKeep.real())
            {
                vKeep = vElements[i].real();
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
/// \brief This function calculates the logical
/// AND operation between all elements in the
/// passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_and(const value_type* vElements, int nElements)
{
    for (int i = 0; i < nElements; i++)
    {
        if (isnan(vElements[i]) || vElements[i] == 0.0)
            return 0.0;
    }

    return 1.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the logical
/// OR operation between all elements in the
/// passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_or(const value_type* vElements, int nElements)
{
    for (int i = 0; i < nElements; i++)
    {
        if (vElements[i] != 0.0 && !isnan(vElements[i]))
            return 1.0;
    }

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the logical
/// XOR operation between all elements in the
/// passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_xor(const value_type* vElements, int nElements)
{
    bool isTrue = false;
    for (int i = 0; i < nElements; i++)
    {
        if (vElements[i] != 0.0 && !isnan(vElements[i]))
        {
            if (!isTrue)
                isTrue = true;
            else
                return 0.0;
        }
    }
    if (isTrue)
        return 1.0;
    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function implements an abstract
/// polynomial of an arbitrary order.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_polynomial(const value_type* vElements, int nElements)
{
    if (!nElements)
        return NAN;
    else if (nElements == 1)
        return 0.0;

    value_type dResult = vElements[1];

    for (int i = 2; i < nElements; i++)
        dResult += vElements[i] * intPower(vElements[0], i-1);

    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function implements the perlin
/// noise function.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_perlin(const value_type* vElements, int nElements)
{
    // perlin(x,y,z,seed,freq,oct,pers)
    if (!nElements)
        return NAN;

    noise::module::Perlin perlinNoise;

    switch (nElements)
    {
        case 1:
            return perlinNoise.GetValue(vElements[0].real(), 0, 0);
        case 2:
            return perlinNoise.GetValue(vElements[0].real(), vElements[1].real(), 0);
        case 3:
            return perlinNoise.GetValue(vElements[0].real(), vElements[1].real(), vElements[2].real());
        case 7: // fallthrough intended
            perlinNoise.SetPersistence(vElements[6].real());
        case 6: // fallthrough intended
            perlinNoise.SetOctaveCount(intCast(vElements[5]));
        case 5: // fallthrough intended
            perlinNoise.SetFrequency(vElements[4].real());
        case 4:
            perlinNoise.SetSeed(intCast(vElements[3].real()));
            return perlinNoise.GetValue(vElements[0].real(), vElements[1].real(), vElements[2].real());
    }

    return NAN;
}


/////////////////////////////////////////////////
/// \brief This function summarizes all elements
/// in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Sum(const value_type* vElements, int nElements)
{
    value_type fRes = 0;

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(vElements[i]))
            fRes += vElements[i];
    }

    return fRes;
}


/////////////////////////////////////////////////
/// \brief This function calculates the average
/// of all elements in passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Avg(const value_type* vElements, int nElements)
{
    return parser_Sum(vElements, nElements) / parser_Num(vElements, nElements);
}


/////////////////////////////////////////////////
/// \brief This function calculates the minimal
/// value of all elements in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Min(const value_type* vElements, int nElements)
{
    value_type fRes = vElements[0].real();

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(fRes))
            break;

        if (!isnan(vElements[i].real()))
            fRes = vElements[i].real();
    }

    if (isnan(fRes))
        return fRes;

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(vElements[i].real()))
            fRes = std::min(fRes.real(), vElements[i].real());
    }

    return fRes;
}


/////////////////////////////////////////////////
/// \brief This function calculates the maximal
/// value of all elements in the passed array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Max(const value_type* vElements, int nElements)
{
    value_type fRes = vElements[0].real();

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(fRes))
            break;

        if (!isnan(vElements[i].real()))
            fRes = vElements[i].real();
    }

    if (isnan(fRes))
        return fRes;

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(vElements[i].real()))
            fRes = std::max(fRes.real(), vElements[i].real());
    }

    return fRes;
}


/////////////////////////////////////////////////
/// \brief This function returns the index of the
/// (first) minimal value in the array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_MinPos(const value_type* vElements, int nElements)
{
    vector<value_type> vData(vElements, vElements+nElements);
    vData.push_back(parser_Min(vElements, nElements));
    vData.push_back(0);

    return parser_compare(&vData[0], vData.size());
}


/////////////////////////////////////////////////
/// \brief This function returns the index of the
/// (first) maximal value in the array.
///
/// \param vElements const value_type*
/// \param nElements int
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_MaxPos(const value_type* vElements, int nElements)
{
    vector<value_type> vData(vElements, vElements+nElements);
    vData.push_back(parser_Max(vElements, nElements));
    vData.push_back(0);

    return parser_compare(&vData[0], vData.size());
}


/////////////////////////////////////////////////
/// \brief This function rounds the passed value
/// to the selected number of decimals.
///
/// \param vToRound const value_type&
/// \param vDecimals const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_round(const value_type& vToRound, const value_type& vDecimals)
{
    if (isinf(vToRound) || isinf(vDecimals) || isnan(vToRound) || isnan(vDecimals))
        return NAN;

    double dDecimals = intPower(10, -abs(intCast(vDecimals)));
    value_type vRounded = vToRound / dDecimals;
    vRounded = value_type(std::round(vRounded.real()), std::round(vRounded.imag()));
    return vRounded * dDecimals;
}


/////////////////////////////////////////////////
/// \brief This function converts degree to
/// radian.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_toRadian(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;

    return v / 180.0 * M_PI;
}


/////////////////////////////////////////////////
/// \brief This function converts radian to
/// degree.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_toDegree(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;

    return v / M_PI * 180.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the real part
/// of the selected spherical harmonics.
///
/// \param vl const value_type&
/// \param vm const value_type&
/// \param theta const value_type&
/// \param phi const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalHarmonics(const value_type& vl, const value_type& vm, const value_type& theta, const value_type& phi)
{
    if (isinf(vl.real()) || isnan(vl.real())
        || isinf(vm.real()) || isnan(vm.real())
        || isinf(theta.real()) || isnan(theta.real())
        || isinf(phi.real()) || isnan(phi.real()))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return sqrt((2.0*l+1.0) * parser_Faculty(l-m) / (4.0 * M_PI * parser_Faculty(l+m)))
            * parser_AssociatedLegendrePolynomial(l, m, cos(theta.real())) * exp(value_type(0, m*phi.real()));

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the imaginary
/// part of the selected spherical harmonics.
///
/// \param vl const value_type&
/// \param vm const value_type&
/// \param theta const value_type&
/// \param phi const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_imSphericalHarmonics(const value_type& vl, const value_type& vm, const value_type& theta, const value_type& phi)
{
    if (isinf(vl.real()) || isnan(vl.real())
        || isinf(vm.real()) || isnan(vm.real())
        || isinf(theta.real()) || isnan(theta.real())
        || isinf(phi.real()) || isnan(phi.real()))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return sqrt((2.0*l+1.0) * parser_Faculty(l-m) / (4.0 * M_PI * parser_Faculty(l+m)))
            * parser_AssociatedLegendrePolynomial(l, m, cos(theta.real())) * sin(m*phi.real());

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the selected
/// Zernike polynomials.
///
/// \param vn const value_type&
/// \param vm const value_type&
/// \param rho const value_type&
/// \param phi const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Zernike(const value_type& vn, const value_type& vm, const value_type& rho, const value_type& phi)
{
    if (isinf(vn.real()) || isnan(vn.real())
        || isinf(vm.real()) || isnan(vm.real())
        || isinf(rho) || isnan(rho)
        || isinf(phi) || isnan(phi))
        return NAN;

    int n = intCast(vn);
    int m = intCast(vm);

    if (n < abs(m))
        return NAN;

    if (m < 0)
        return parser_ZernikeRadial(n, -m, rho) * sin(-(double)m*phi);
    else
        return parser_ZernikeRadial(n, m, rho) * cos((double)m*phi);
}


/////////////////////////////////////////////////
/// \brief This function calculates the radial
/// part of the Zernike polynomials.
///
/// \param n int
/// \param m int
/// \param rho const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ZernikeRadial(int n, int m, const value_type& rho)
{
    value_type vReturn = 0;
    value_type vNorm = 0;

    if (fabs(rho) > 1.0)
        return NAN;

    if ((n-m) % 2)
        return 0.0;

    for (int k = 0; k <= (n-m)/2; k++)
    {
        if (k % 2)
        {
            vReturn -= parser_Faculty(n-k)*intPower(rho, n-2*k)/(parser_Faculty(k)*parser_Faculty((n+m)/2.0-k)*parser_Faculty((n-m)/2.0-k));
            vNorm -= parser_Faculty(n-k)/(parser_Faculty(k)*parser_Faculty((n+m)/2.0-k)*parser_Faculty((n-m)/2.0-k));
        }
        else
        {
            vReturn += parser_Faculty(n-k)*intPower(rho, n-2*k)/(parser_Faculty(k)*parser_Faculty((n+m)/2.0-k)*parser_Faculty((n-m)/2.0-k));
            vNorm += parser_Faculty(n-k)/(parser_Faculty(k)*parser_Faculty((n+m)/2.0-k)*parser_Faculty((n-m)/2.0-k));
        }
    }

    return vReturn/vNorm;
}


/////////////////////////////////////////////////
/// \brief This function calculates the cardinal
/// sine of x.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SinusCardinalis(const value_type& v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    if (v == 0.0)
        return 1.0;
    else
        return sin(v)/v;
}


/////////////////////////////////////////////////
/// \brief This function calculates the spherical
/// bessel function.
///
/// \param vn const value_type&
/// \param vc const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalBessel(const value_type& vn, const value_type& vc)
{
    if (isinf(vn) || isinf(vc) || isnan(vn) || isnan(vc))
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
/// von Neumann function.
///
/// \param vn const value_type&
/// \param vc const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalNeumann(const value_type& vn, const value_type& vc)
{
    if (isinf(vn) || isnan(vn) || isinf(vc) || isnan(vc))
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
/// \brief This function calculates the Legendre
/// polynomials of the selected order.
///
/// \param vn const value_type&
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_LegendrePolynomial(const value_type& vn, const value_type& v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    value_type dResult = 0.0;
    for (int k = 0; k <= n/2; k++)
    {
        dResult += intPower(-1,k)*parser_Binom(n,k)*parser_Binom(2*(n-k),n)*intPower(v,n-2*k);
    }
    dResult *= intPower(2, -n);
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Legendre polynomials of the
/// selected order.
///
/// \param vl const value_type&
/// \param vm const value_type&
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AssociatedLegendrePolynomial(const value_type& vl, const value_type& vm, const value_type& v)
{
    if (isinf(vl) || isnan(vl) || isinf(vm) || isnan(vm) || isinf(v) || isnan(v))
        return NAN;
    int l = intCast(fabs(vl));
    int m = intCast(fabs(vm));
    if (m > l)
        return NAN;
    if (!m)
        return parser_LegendrePolynomial(l,v);
    else if (m < 0)
        return intPower(-1.0,m)* parser_Faculty(l-m) / parser_Faculty(l+m) * parser_AssociatedLegendrePolynomial(l,m,v);
    else if (l == m)
        return intPower(-1.0,l)*parser_doubleFaculty((2.0*l-1.0))*pow(1.0-v*v,(double)l/2.0);//intPower(sqrt(1-v*v), l);
    else if (m == l-1)
        return v*(2.0*l-1.0)*intPower(-1.0,l-1)*parser_doubleFaculty((2.0*l-3.0))*pow(1.0-v*v,((double)l-1.0)/2.0);//intPower(sqrt(1-v*v), l-1);
    else
        return 1.0/(double)(l-m)*(v*(2.0*l-1)*parser_AssociatedLegendrePolynomial(l-1,m,v) - (double)(l+m-1)*parser_AssociatedLegendrePolynomial(l-2,m,v));

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the Laguerre
/// polynomials of the selected order.
///
/// \param vn const value_type&
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_LaguerrePolynomial(const value_type& vn, const value_type& v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    value_type dResult = 0.0;
    for (int k = 0; k <= n; k++)
    {
        dResult += intPower(-v,k)*parser_Binom(n,k)/parser_Faculty(k);
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Laguerre polynomials of the
/// selected order.
///
/// \param vn const value_type&
/// \param vk const value_type&
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AssociatedLaguerrePolynomial(const value_type& vn, const value_type& vk, const value_type& v)
{
    if (isinf(vn) || isnan(vn) || isinf(vk) || isnan(vk) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));
    int k = intCast(fabs(vk));
    if (k > n)
        return NAN;
    value_type dResult = 0.0;
    value_type vFaculty = parser_Faculty(n+k);
    for (int m = 0; m <= n; m++)
    {
        dResult += vFaculty * intPower(-v,m) / (parser_Faculty(n-m)*parser_Faculty(k+m)*parser_Faculty(m));
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the Hermite
/// polynomials of the selected order.
///
/// \param vn const value_type&
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_HermitePolynomial(const value_type& vn, const value_type& v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    switch (n)
    {
        case 0: return 1.0;
        case 1: return 2.0*v;
        case 2: return 4.0*v*v - 2.0;
        case 3: return 8.0*v*v*v - 12.0*v;
        case 4: return 16.0*v*v*v*v - 48.0*v*v + 12.0;
        default: return 2.0*v*parser_HermitePolynomial(n-1,v) - 2.0*(double)(n-1)*parser_HermitePolynomial(n-2,v);
    }
    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the nucleic
/// core binding energy according the Bethe
/// Weizsäcker formula.
///
/// \param vN const value_type&
/// \param vZ const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BetheWeizsaecker(const value_type& vN, const value_type& vZ)
{
    if (isinf(vN.real()) || isnan(vN.real()) || isinf(vZ.real()) || isnan(vZ.real()))
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
    unsigned int N = (unsigned int)intCast(parser_round(vN,0));
    unsigned int Z = (unsigned int)intCast(parser_round(vZ,0));


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
/// \brief This function represents the Heaviside
/// (theta) function.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Heaviside(const value_type& v)
{
    if (isinf(v.real()) || isnan(v.real()))
        return NAN;

    return v.real() >= 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the x axis in the x-y plane (the
/// azimuthal angle phi).
///
/// \param x const value_type&
/// \param y const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_phi(const value_type& x, const value_type& y)
{
    if (isinf(x.real()) || isnan(x.real()) || isinf(y.real()) || isnan(y.real()))
        return NAN;
    if (y.real() < 0)
        return M_PI+abs(M_PI + atan2(y.real(), x.real()));
    return atan2(y.real(), x.real());
}


// --> Polarwinkel theta <--
/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the z axis in any z-r plane (the
/// polar angle theta).
///
/// \param x const value_type&
/// \param y const value_type&
/// \param z const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_theta(const value_type& x, const value_type& y, const value_type& z)
{
    if (isinf(x) || isnan(x) || isinf(y) || isnan(y) || isinf(z) || isnan(z))
        return NAN;
    if (x == 0.0 && y == 0.0 && z == 0.0)
        return M_PI/2;
    return acos(z/sqrt(x*conj(x)+y*conj(y)+z*conj(z)));
}


/////////////////////////////////////////////////
/// \brief This function returns a uniformly
/// distributed random number between both
/// boundaries.
///
/// \param vRandMin const value_type&
/// \param vRandMax const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Random(const value_type& vRandMin, const value_type& vRandMax)
{
    if (isinf(vRandMin) || isnan(vRandMin) || isinf(vRandMax) || isnan(vRandMax))
        return NAN;

    static uniform_real_distribution<double> randDist(0, 1);
    return randDist(getRandGenInstance()) * (vRandMax - vRandMin) + vRandMin;
}


/////////////////////////////////////////////////
/// \brief This function returns a gaussian
/// distributed random number using the passed
/// values as mean and standard deviation.
///
/// \param vRandAvg const value_type&
/// \param vRandstd const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gRandom(const value_type& vRandAvg, const value_type& vRandstd)
{
    if (isinf(vRandAvg) || isnan(vRandAvg) || isinf(vRandstd) || isnan(vRandstd))
        return NAN;

    static normal_distribution<double> randDist(0, 1);
    return randDist(getRandGenInstance()) * fabs(vRandstd) + vRandAvg;
}


/////////////////////////////////////////////////
/// \brief This function calculates the gaussian
/// error function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_erf(const value_type& x)
{
    if (isinf(x.real()) || isnan(x.real()))
        return NAN;
    return erf(x.real());
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// complementary gaussian error function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_erfc(const value_type& x)
{
    if (isinf(x.real()) || isnan(x.real()))
        return NAN;
    return erfc(x.real());
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// riemannian Gamma function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gamma(const value_type& x)
{
    if (isinf(x) || isnan(x))
        return NAN;

    if (x.imag() == 0.0)
        return tgamma(x.real());

    gsl_sf_result lng;
    gsl_sf_result arg;
    gsl_sf_lngamma_complex_e(x.real(), x.imag(), &lng, &arg);

    return std::polar(std::exp(lng.val), arg.val);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Ai(x).
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AiryA(const value_type& x)
{
    return gsl_sf_airy_Ai(x.real(), GSL_PREC_DOUBLE);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Bi(x).
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AiryB(const value_type& x)
{
    return gsl_sf_airy_Bi(x.real(), GSL_PREC_DOUBLE);
}


/////////////////////////////////////////////////
/// \brief This function calculates the regulary
/// bessel function.
///
/// \param n const value_type&
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_RegularCylBessel(const value_type& n, const value_type& x)
{
    if (n.real() >= 0.0)
        return gsl_sf_bessel_Jn(intCast(n), x.real());
    else
        return NAN;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// irregulary bessel (von Neumann) function.
///
/// \param n const value_type&
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_IrregularCylBessel(const value_type& n, const value_type& x)
{
    if (x != 0.0 && n.real() >= 0.0)
        return x.real()/fabs(x.real())*gsl_sf_bessel_Yn(intCast(n), fabs(x.real()));
    else
        return -INFINITY;
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal F(phi,k).
///
/// \param phic const value_type&
/// \param kc const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticF(const value_type& phic, const value_type& kc)
{
    double k = kc.real();
    double phi = phic.real();

    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi))
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


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal E(phi,k).
///
/// \param phic const value_type&
/// \param kc const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticE(const value_type& phic, const value_type& kc)
{
    double phi = phic.real();
    double k = kc.real();

    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi))
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


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal Pi(phi,n,k).
///
/// \param phi const value_type&
/// \param n const value_type&
/// \param k const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticP(const value_type& phi, const value_type& n, const value_type& k)
{
    if (isnan(k.real()) || isnan(phi.real()) || isinf(k.real()) || isinf(phi.real()) || isnan(n.real()) || isinf(n.real()))
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


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal D(phi,n,k).
///
/// \param phi const value_type&
/// \param n const value_type&
/// \param k const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticD(const value_type& phi, const value_type& n, const value_type& k)
{
    if (isnan(k.real()) || isnan(phi.real()) || isinf(k.real()) || isinf(phi.real()) || isnan(n.real()) || isinf(n.real()))
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
            return nSign*(nMultiple*gsl_sf_ellint_D(M_PI_2, k.real(), n.real(), 0) + gsl_sf_ellint_D(fabs(phi.real())-nMultiple*M_PI_2, k.real(), n.real(), 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_D(M_PI_2, k.real(), n.real(), 0) - gsl_sf_ellint_D(M_PI_2-(fabs(phi.real())-nMultiple*M_PI_2), k.real(), n.real(), 0));
    }

    return gsl_sf_ellint_D(phi.real(), k.real(), n.real(), 0);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Beta function.
///
/// \param a const value_type&
/// \param b const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_beta(const value_type& a, const value_type& b)
{
    if (isnan(a.real()) || isnan(b.real()) || isinf(a.real()) || isinf(b.real()))
        return NAN;

    if ((intCast(a) == (int)a.real() && a.real() < 0) || (intCast(b) == (int)b.real() && b.real() < 0))
        return NAN;

    return gsl_sf_beta(a.real(), b.real());
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
        sum += parser_Binom((double)N, (double)j).real();

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


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Zeta function.
///
/// \param s const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_zeta(const value_type& s)
{
    if (isnan(s) || isinf(s))
        return NAN;

    // Use the functional equation to swap negative
    // real numbers into the positive half-plane
    if (s.real() < 0.0)
        return std::pow(2.0, s)*std::pow(M_PI, s-1.0)*sin(0.5*M_PI*s)*parser_gamma(1.0-s)*complex_zeta(1.0-s);

    return complex_zeta(s);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Clausen function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_clausen(const value_type& x)
{
    if (isnan(x.real()) || isinf(x.real()))
        return NAN;

    return gsl_sf_clausen(x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Digamma function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_digamma(const value_type& x)
{
    if (isnan(x.real()) || isinf(x.real()))
        return NAN;

    if (x.real() == 0.0)
        return NAN;

    if ((int)x.real() == intCast(x) && x.real() > 0)
        return gsl_sf_psi_int(intCast(x));
    else
        return gsl_sf_psi(x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the value if the
/// Polygamma function.
///
/// \param n const value_type&
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_polygamma(const value_type& n, const value_type& x)
{
    if (isnan(n.real()) || isnan(x.real()) || isinf(n.real()) || isinf(x.real()) || x.real() <= 0 || n.real() < 0)
        return NAN;

    return gsl_sf_psi_n(intCast(n), x.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Dilogarithm Li2(x).
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_dilogarithm(const value_type& x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    if (x.imag() == 0.0)
        return gsl_sf_dilog(x.real());

    gsl_sf_result re;
    gsl_sf_result im;

    gsl_sf_complex_dilog_xy_e(x.real(), x.imag(), &re, &im);
    return value_type(re.val, im.val);
}


/////////////////////////////////////////////////
/// \brief This is the floor function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_floor(const value_type& x)
{
    return value_type(floor(x.real()), floor(x.imag()));
}


/////////////////////////////////////////////////
/// \brief This is the roof (ceil) function.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_roof(const value_type& x)
{
    return value_type(ceil(x.real()), ceil(x.imag()));
}


/////////////////////////////////////////////////
/// \brief This is the rect function.
///
/// \param x const value_type&
/// \param x0 const value_type&
/// \param x1 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_rect(const value_type& x, const value_type& x0, const value_type& x1)
{
    return (x.real() > x1.real() || x.real() < x0.real()) ? 0.0 : 1.0;
}


/////////////////////////////////////////////////
/// \brief This function describes an interval
/// with borders of a selected type (including,
/// excluding, ignore).
///
/// \param x const value_type&
/// \param x0 const value_type&
/// \param x1 const value_type&
/// \param lborder const value_type&
/// \param rborder const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ivl(const value_type& x, const value_type& x0, const value_type& x1, const value_type& lborder, const value_type& rborder)
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
/// \brief This function returns the Student
/// factor s_t for the selected degrees of
/// freedom and a confidence interval.
///
/// \param vFreedoms const value_type&
/// \param vAlpha const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_studentFactor(const value_type& vFreedoms, const value_type& vAlpha)
{
    if (vAlpha.real() >= 1.0 || vAlpha.real() <= 0.0 || vFreedoms.real() < 2.0)
        return NAN;

    return student_t(intCast(vFreedoms), vAlpha.real());
}


/////////////////////////////////////////////////
/// \brief This function returns the greatest
/// common divisor of both argments.
///
/// \param n const value_type&
/// \param k const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gcd(const value_type& n, const value_type& k)
{
    return boost::math::gcd(intCast(n), intCast(k));
}


/////////////////////////////////////////////////
/// \brief This function returns the least common
/// multiple of both arguments.
///
/// \param n const value_type&
/// \param k const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_lcm(const value_type& n, const value_type& k)
{
    return boost::math::lcm(intCast(n), intCast(k));
}


/////////////////////////////////////////////////
/// \brief This function represents the Modulo
/// operator.
///
/// \param v1 const value_type&
/// \param v2 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mod(const value_type& v1, const value_type& v2)
{
    if (std::isinf(v2.real()) || std::isnan(v2.real()) || std::isinf(v2.imag()) || std::isnan(v2.imag()))
        return NAN;

    mu::value_type div = v1 / v2;
    div = mu::value_type(std::floor(div.real()),
                         std::floor(div.imag()));

    return v1 - div * v2;
}


/////////////////////////////////////////////////
/// \brief This function represent the XOR
/// operator.
///
/// \param v1 const value_type&
/// \param v2 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_XOR(const value_type& v1, const value_type& v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return (v1 != 0.0) xor (v2 != 0.0);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// OR operator.
///
/// \param v1 const value_type&
/// \param v2 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BinOR(const value_type& v1, const value_type& v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return intCast(v1) | intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// AND operator.
///
/// \param v1 const value_type&
/// \param v2 const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BinAND(const value_type& v1, const value_type& v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return intCast(v1) & intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function is a numerical version
/// of the string is_string() function. Used as a
/// fallback.
///
/// \param v const value_type&
/// \return const value_type&
///
/////////////////////////////////////////////////
value_type parser_is_string(const value_type& v)
{
    if (isnan(v) || isinf(v))
        return NAN;

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function returns the current UNIX
/// time.
///
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_time()
{
    return to_double(sys_time_now());
}


/////////////////////////////////////////////////
/// \brief This function returns the current CPU
/// clock count.
///
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_clock()
{
    return (double)clock();
}


/////////////////////////////////////////////////
/// \brief Sleeps for the passed amount of
/// milliseconds and returns this number.
///
/// \param milliseconds const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_sleep(const value_type& milliseconds)
{
    Sleep(intCast(milliseconds));
    return intCast(milliseconds);
}


/////////////////////////////////////////////////
/// \brief Calculates the logarithm of x using
/// the base b.
///
/// \param b const value_type&
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_log_b(const value_type& b, const value_type& x)
{
    if (isnan(b.real()) || isnan(x) || isinf(b.real()) || x.real() <= 0.0 || b.real() <= 0.0)
        return NAN;

    if (isinf(x))
        return INFINITY;

    return log10(x) / log10(b.real());
}


/////////////////////////////////////////////////
/// \brief Returns the version number of NumeRe
/// as a natural number.
///
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_numereversion()
{
    return 100.0*AutoVersion::MAJOR+10.0*AutoVersion::MINOR + AutoVersion::BUILD + std::atof(AutoVersion::UBUNTU_VERSION_STYLE) / 100.0;
}


/////////////////////////////////////////////////
/// \brief This function converts UNIX time
/// values into a selected part of a time stamp.
///
/// \param vTime const value_type&
/// \param vType const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_date(const value_type& vTime, const value_type& vType)
{
    sys_time_point tp = to_timePoint(vTime.real());
    int nType = (int)rint(vType.real());
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


/////////////////////////////////////////////////
/// \brief Returns, whether the selected value is
/// NaN.
///
/// \param v const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_isnan(const value_type& v)
{
    return v != v;
}


/////////////////////////////////////////////////
/// \brief This function numerically defines a
/// valid value range (the value is set to NaN,
/// if outside of this range).
///
/// \param v const value_type&
/// \param vLeft const value_type&
/// \param vRight const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_interval(const value_type& v, const value_type& vLeft, const value_type& vRight)
{
    if (vRight.real() <= vLeft.real())
        return NAN;

    if (v.real() <= vRight.real() && v.real() >= vLeft.real())
        return v.real();

    return NAN;
}


/////////////////////////////////////////////////
/// \brief This function returns the cotangent of
/// the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_cot(const value_type& x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    if (sin(x) == 0.0)
        return INFINITY;

    return cos(x) / sin(x);
}


/////////////////////////////////////////////////
/// \brief This function represents the numerical
/// variable factory. New memory is allocated in
/// this function and stored in an internal list
/// managed by the parser.
///
/// \param a_szName const char_type*
/// \param a_pUserData void*
/// \return value_type*
///
/////////////////////////////////////////////////
value_type* parser_AddVariable(const char_type* a_szName, void* a_pUserData)
{
    // Cast the passed void pointer to a the data storage list
    std::list<mu::value_type*>* m_lDataStorage = static_cast<std::list<mu::value_type*>* >(a_pUserData);

    // Create the storage for a new variable
    m_lDataStorage->push_back(new mu::value_type);
    *(m_lDataStorage->back()) = 0.0;

    // Return the address of the newly created storage
    return m_lDataStorage->back();
}


/////////////////////////////////////////////////
/// \brief This function returns the secant of
/// the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_sec(const value_type& x)
{
    return 1.0 / std::cos(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the cosecant of
/// the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_csc(const value_type& x)
{
    return 1.0 / std::sin(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// secant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_asec(const value_type& x)
{
    return std::acos(1.0 / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// cosecant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_acsc(const value_type& x)
{
    return std::asin(1.0 / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the hyperbolic
/// secant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_sech(const value_type& x)
{
    return 1.0 / std::cosh(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the hyperbolic
/// cosecant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_csch(const value_type& x)
{
    return 1.0 / std::sinh(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// hyperbolic secant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_asech(const value_type& x)
{
    return std::acosh(1.0 / x);
}


/////////////////////////////////////////////////
/// \brief This function returns the inverse
/// hyperbolic cosecant of the passed value.
///
/// \param x const value_type&
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_acsch(const value_type& x)
{
    return std::asinh(1.0 / x);
}

