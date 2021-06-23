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
#include <random>
#include <csignal>
#include <boost/math/common_factor.hpp>
#include <gsl/gsl_sf.h>
#include <noise/noise.h>

#include "student_t.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "../utils/tools.hpp"
#include "../version.h"


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
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mega(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1m.
///
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Milli(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1G.
///
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Giga(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e9;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1k.
///
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Kilo(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mu.
///
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Micro(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1n.
///
/// \param a_fVal value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Nano(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-9;
}


/////////////////////////////////////////////////
/// \brief Function representing the logical NOT
/// operator.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Not(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v==0;
}


/////////////////////////////////////////////////
/// \brief Identity function. Used for ignoring
/// functions and parameters in special cases.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Ignore(value_type v)
{
    return v;
}


// --> Einheitenumrechnung: eV, fm, A, b, Torr, AU, etc... <--
/////////////////////////////////////////////////
/// \brief Conversion function for 1eV.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ElectronVolt(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.60217657e-19;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1fm.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Fermi(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1A.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Angstroem(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bn.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Barn(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-28;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Torr/1mmhg.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Torr(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 101325/(double)760;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1AU.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AstroUnit(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 149597870700;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ly.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Lightyear(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 9460730472580800;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1pc.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Parsec(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 30856775777948584.2;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mi.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mile(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1609.344;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1yd.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Yard(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.9144;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ft.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Foot(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.3048;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1in.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Inch(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.0254;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1cal.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Calorie(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 4.1868;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1psi.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_PSI(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6894.75729;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kn.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Knoten(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 463.0 / 900.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1l.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_liter(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kmh.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_kmh(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mph.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_mph(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.609334 / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°C.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Celsius(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v + 273.15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°F.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Fahrenheit(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return (v + 459.67) * 5.0 / 9.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1C.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Curie(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 3.7e10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Gs.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Gauss(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-4;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Ps.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Poise(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-1;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mol.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_mol(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6.022140857E23;
}


/////////////////////////////////////////////////
/// \brief Function representing the faculty of
/// any natural number.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Faculty(value_type v)
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
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_doubleFaculty(value_type v)
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
/// \param v1 value_type
/// \param v2 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Binom(value_type v1, value_type v2)
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
    else if(intCast(v1) == intCast(v2) || (intCast(v1) != 0 && intCast(v2) == 0)) // v1 == v2 oder v2 == 0 und v1 != 0 ==> binom = 1!
        return 1;
    else if(intCast(v2) == 1 || intCast(v2) == intCast(v1)-1) // v2 == 1 oder v2 == v1-1 ==> binom = v1!
        return intCast(v1);
    else if(intCast(v2) == 2 && intCast(v2) < intCast(v1)) // v2 == 2 und v2 < v1 ==> binom = v1*(v1-1) / v2!
        return intCast(v1)*(intCast(v1)-1)/intCast(v2);
    else
    {
        /* --> In allen anderen Faellen muessen wir den Binomialkoeffzienten muehsam mithilfe der Formel
         *     binom(v1,v2) = v1!/(v2!*(v1-v2)!) ausrechnen. Das machen wir, indem wir die Funktion
         *     parser_Faculty(value_type) aufrufen <--
         */
        return parser_Faculty(v1) / (parser_Faculty(v2)*parser_Faculty( (value_type)( intCast(v1) - intCast(v2) ) ));
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
            vStd += (vElements[i] - vMean) * (vElements[i] - vMean);
    }

    vStd = sqrt(vStd / (value_type)(parser_Num(vElements, nElements)-1));
    return vStd;
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
            vResult += vElements[i] * vElements[i];
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

    if (vElements[nElements-1] > 0)
        nType = RETURN_GE;
    else if (vElements[nElements-1] < 0)
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
        if (isnan(vElements[i]) || vElements[i] == 0)
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
        if (vElements[i] != 0 && !isnan(vElements[i]))
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
        if (vElements[i] != 0 && !isnan(vElements[i]))
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
            return perlinNoise.GetValue(vElements[0], 0, 0);
        case 2:
            return perlinNoise.GetValue(vElements[0], vElements[1], 0);
        case 3:
            return perlinNoise.GetValue(vElements[0], vElements[1], vElements[2]);
        case 7: // fallthrough intended
            perlinNoise.SetPersistence(vElements[6]);
        case 6: // fallthrough intended
            perlinNoise.SetOctaveCount(intCast(vElements[5]));
        case 5: // fallthrough intended
            perlinNoise.SetFrequency(vElements[4]);
        case 4:
            perlinNoise.SetSeed(intCast(vElements[3]));
            return perlinNoise.GetValue(vElements[0], vElements[1], vElements[2]);
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
    value_type fRes = vElements[0];

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(fRes))
            break;

        if (!isnan(vElements[i]))
            fRes = vElements[i];
    }

    if (isnan(fRes))
        return fRes;

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(vElements[i]))
            fRes = std::min(fRes, vElements[i]);
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
    value_type fRes = vElements[0];

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(fRes))
            break;

        if (!isnan(vElements[i]))
            fRes = vElements[i];
    }

    if (isnan(fRes))
        return fRes;

    for (int i = 0; i < nElements; ++i)
    {
        if (!isnan(vElements[i]))
            fRes = std::max(fRes, vElements[i]);
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
/// \param vToRound value_type
/// \param vDecimals value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_round(value_type vToRound, value_type vDecimals)
{
    if (isinf(vToRound) || isinf(vDecimals) || isnan(vToRound) || isnan(vDecimals))
        return NAN;

    double dDecimals = std::pow(10, -abs(intCast(vDecimals)));
    vToRound = vToRound / dDecimals;
    vToRound = std::round(vToRound);
    vToRound = vToRound * dDecimals;
    return vToRound;
}


/////////////////////////////////////////////////
/// \brief This function converts degree to
/// radian.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_toRadian(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;

    return v / 180.0 * M_PI;
}


/////////////////////////////////////////////////
/// \brief This function converts radian to
/// degree.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_toDegree(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;

    return v / M_PI * 180.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the real part
/// of the selected spherical harmonics.
///
/// \param vl value_type
/// \param vm value_type
/// \param theta value_type
/// \param phi value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalHarmonics(value_type vl, value_type vm, value_type theta, value_type phi)
{
    if (isinf(vl) || isnan(vl)
        || isinf(vm) || isnan(vm)
        || isinf(theta) || isnan(theta)
        || isinf(phi) || isnan(phi))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return sqrt((double)(2.0*l+1.0) * parser_Faculty(l-m) / (4.0 * M_PI * parser_Faculty(l+m)))*parser_AssociatedLegendrePolynomial(l,m,cos(theta))*cos(m*phi);

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the imaginary
/// part of the selected spherical harmonics.
///
/// \param vl value_type
/// \param vm value_type
/// \param theta value_type
/// \param phi value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_imSphericalHarmonics(value_type vl, value_type vm, value_type theta, value_type phi)
{
    if (isinf(vl) || isnan(vl)
        || isinf(vm) || isnan(vm)
        || isinf(theta) || isnan(theta)
        || isinf(phi) || isnan(phi))
        return NAN;

    int l = intCast(fabs(vl));
    int m = intCast(vm);

    if (abs(m) > l)
        return NAN;
    else
        return sqrt((double)(2.0*l+1.0) * parser_Faculty(l-m) / (4.0 * M_PI * parser_Faculty(l+m)))*parser_AssociatedLegendrePolynomial(l,m,cos(theta))*sin(m*phi);

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the selected
/// Zernike polynomials.
///
/// \param vn value_type
/// \param vm value_type
/// \param rho value_type
/// \param phi value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Zernike(value_type vn, value_type vm, value_type rho, value_type phi)
{
    if (isinf(vn) || isnan(vn)
        || isinf(vm) || isnan(vm)
        || isinf(rho) || isnan(rho)
        || isinf(phi) || isnan(phi))
        return NAN;

    int n = intCast(vn);
    int m = intCast(vm);

    if (n < abs(m))
        return NAN;

    if (m < 0)
        return parser_ZernikeRadial(n, -m, rho) * sin(-m*phi);
    else
        return parser_ZernikeRadial(n, m, rho) * cos(m*phi);
}


/////////////////////////////////////////////////
/// \brief This function calculates the radial
/// part of the Zernike polynomials.
///
/// \param n int
/// \param m int
/// \param rho value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ZernikeRadial(int n, int m, value_type rho)
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
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SinusCardinalis(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    if (!v)
        return 1.0;
    else
        return sin(v)/v;
}


/////////////////////////////////////////////////
/// \brief This function calculates the spherical
/// bessel function.
///
/// \param vn value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalBessel(value_type vn, value_type v)
{
    if (isinf(vn) || isinf(v) || isnan(vn) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));
    if (!n && !v)
        return 1.0;
    else if (!n)
        return sin(v)/v;
    else if (n && !v)
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
        return (-intPower(v,10)+1485.0*intPower(v,8)-315315.0*intPower(v,6)+18918900.0*v*v*v*v-310134825.0*v*v+654729075.0)*sin(v)/intPower(v,11)-55.0*(intPower(v,8)-468.0*intPower(v,6)+51597.0*v*v*v*v-1670760*v*v+11904165.0)*cos(v)/intPower(v,10);
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
/// \param vn value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_SphericalNeumann(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));
    if (!v)
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
/// \param vn value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_LegendrePolynomial(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    long double dResult = 0.0;
    for (int k = 0; k <= n/2; k++)
    {
        dResult += (long double)(intPower(-1,k)*parser_Binom(n,k)*parser_Binom(2*(n-k),n)*intPower(v,n-2*k));
    }
    dResult *= intPower(2, -n);
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Legendre polynomials of the
/// selected order.
///
/// \param vl value_type
/// \param vm value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AssociatedLegendrePolynomial(value_type vl, value_type vm, value_type v)
{
    if (isinf(vl) || isnan(vl) || isinf(vm) || isnan(vm) || isinf(v) || isnan(v))
        return NAN;
    int l = intCast(fabs(vl));
    int m = intCast(fabs(vm));
    if (m > l)
        return NAN;
    if (!m)
        return parser_LegendrePolynomial(l,v);
    else if (vm < 0)
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
/// \param vn value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_LaguerrePolynomial(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));

    long double dResult = 0.0;
    for (int k = 0; k <= n; k++)
    {
        dResult += (long double)(intPower(-v,k)*parser_Binom(n,k)/parser_Faculty(k));
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// associated Laguerre polynomials of the
/// selected order.
///
/// \param vn value_type
/// \param vk value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AssociatedLaguerrePolynomial(value_type vn, value_type vk, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(vk) || isnan(vk) || isinf(v) || isnan(v))
        return NAN;
    int n = intCast(fabs(vn));
    int k = intCast(fabs(vk));
    if (k > n)
        return NAN;
    long double dResult = 0.0;
    value_type vFaculty = parser_Faculty(n+k);
    for (int m = 0; m <= n; m++)
    {
        dResult += (long double)(vFaculty * intPower(-v,m) / (parser_Faculty(n-m)*parser_Faculty(k+m)*parser_Faculty(m)));
    }
    return dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the Hermite
/// polynomials of the selected order.
///
/// \param vn value_type
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_HermitePolynomial(value_type vn, value_type v)
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
/// \param vN value_type
/// \param vZ value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BetheWeizsaecker(value_type vN, value_type vZ)
{
    if (isinf(vN) || isnan(vN) || isinf(vZ) || isnan(vZ))
        return NAN;
    // nan/inf
    double a_V = 15.67;
    double a_S = 17.23;
    double a_F = 23.2875;
    double a_C = 0.714;
    double a_p = 11.2;
    double A = vN + vZ;
    double dEnergy = 0.0;
    int delta = 0;
    unsigned int N = (unsigned int)intCast(parser_round(vN,0));
    unsigned int Z = (unsigned int)intCast(parser_round(vZ,0));


    if (A < 0 || vZ < 0 || vN < 0)
        return NAN;
    if (A == 0)
        return 0.0;
    if (N % 2 && Z % 2)
        delta = -1;
    else if (!(N % 2 || Z % 2))
        delta = 1;


    dEnergy = a_V*A - a_S*pow(A,2.0/3.0) - a_F*(vN-vZ)*(vN-vZ)/A - a_C*vZ*(vZ-1)/pow(A,1.0/3.0) + (double)delta*a_p/sqrt(A);
    if (dEnergy >= 0)
        return dEnergy;
    else
        return 0.0;
}


/////////////////////////////////////////////////
/// \brief This function represents the Heaviside
/// (theta) function.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Heaviside(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    if (v < 0.0)
        return 0.0;
    else
        return 1.0;
}


/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the x axis in the x-y plane (the
/// azimuthal angle phi).
///
/// \param x value_type
/// \param y value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_phi(value_type x, value_type y)
{
    if (isinf(x) || isnan(x) || isinf(y) || isnan(y))
        return NAN;
    if (y < 0)
        return M_PI+abs(M_PI + atan2(y,x));
    return atan2(y,x);
}


// --> Polarwinkel theta <--
/////////////////////////////////////////////////
/// \brief This function calculates the angle of
/// a vector and the z axis in any z-r plane (the
/// polar angle theta).
///
/// \param x value_type
/// \param y value_type
/// \param z value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_theta(value_type x, value_type y, value_type z)
{
    if (isinf(x) || isnan(x) || isinf(y) || isnan(y) || isinf(z) || isnan(z))
        return NAN;
    if (!x && !y && !z)
        return M_PI/2;
    return acos(z/sqrt(x*x+y*y+z*z));
}


/////////////////////////////////////////////////
/// \brief This function returns a uniformly
/// distributed random number between both
/// boundaries.
///
/// \param vRandMin value_type
/// \param vRandMax value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Random(value_type vRandMin, value_type vRandMax)
{
    if (isinf(vRandMin) || isnan(vRandMin) || isinf(vRandMax) || isnan(vRandMax))
        return NAN;

    const int nRandSet = 1000;
    static double dRandBuffer[nRandSet];
    static int nRandPointer = -1;
    nRandPointer++;

    if (!nRandPointer || nRandPointer == nRandSet)
    {
        if (!nRandPointer || dRandBuffer[0] == 0.0)
            dRandBuffer[0] = 1;

        nRandPointer = 0;
        default_random_engine randGen((dRandBuffer[0]*(double)time(0)));
        uniform_real_distribution<double> randDist(0,1);

        for (int i = 0; i < nRandSet; i++)
            dRandBuffer[i] = randDist(randGen);
    }

    return dRandBuffer[nRandPointer]*(vRandMax-vRandMin)+vRandMin;//randDist(randGen);
}


/////////////////////////////////////////////////
/// \brief This function returns a gaussian
/// distributed random number using the passed
/// values as mean and standard deviation.
///
/// \param vRandAvg value_type
/// \param vRandstd value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gRandom(value_type vRandAvg, value_type vRandstd)
{
    if (isinf(vRandAvg) || isnan(vRandAvg) || isinf(vRandstd) || isnan(vRandstd))
        return NAN;

    const int nRandSet = 1000;
    static double dRandBuffer[nRandSet];
    static int nRandPointer = -1;
    nRandPointer++;

    if (!nRandPointer || nRandPointer == nRandSet)
    {
        if (!nRandPointer || dRandBuffer[0] == 0.0)
            dRandBuffer[0] = 1;

        nRandPointer = 0;
        default_random_engine randGen((dRandBuffer[0]*(double)time(0)));
        normal_distribution<double> randDist(0,1);

        for (int i = 0; i < nRandSet; i++)
            dRandBuffer[i] = randDist(randGen);
    }

    return dRandBuffer[nRandPointer]*fabs(vRandstd)+vRandAvg;//randDist(randGen);
}


/////////////////////////////////////////////////
/// \brief This function calculates the gaussian
/// error function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_erf(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return erf(x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// complementary gaussian error function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_erfc(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return erfc(x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// riemannian Gamma function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gamma(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return tgamma(x);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Ai(x).
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AiryA(value_type x)
{
    return gsl_sf_airy_Ai(x, GSL_PREC_DOUBLE);
}


/////////////////////////////////////////////////
/// \brief This function calculates the Airy
/// function Bi(x).
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_AiryB(value_type x)
{
    return gsl_sf_airy_Bi(x, GSL_PREC_DOUBLE);
}


/////////////////////////////////////////////////
/// \brief This function calculates the regulary
/// bessel function.
///
/// \param n value_type
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_RegularCylBessel(value_type n, value_type x)
{
    if (n >= 0.0)
        return gsl_sf_bessel_Jn(intCast(n), x);
    else
        return NAN;
}


/////////////////////////////////////////////////
/// \brief This function calculates the
/// irregulary bessel (von Neumann) function.
///
/// \param n value_type
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_IrregularCylBessel(value_type n, value_type x)
{
    if (x != 0.0 && n >= 0.0)
        return x/fabs(x)*gsl_sf_bessel_Yn(intCast(n), fabs(x));
    else
        return -INFINITY;
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal F(phi,k).
///
/// \param phi value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticF(value_type phi, value_type k)
{
    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2) /// FIXME
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi/M_PI_2));

        if (phi < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
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
/// \param phi value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticE(value_type phi, value_type k)
{
    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2) /// FIXME
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
/// \param phi value_type
/// \param n value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticP(value_type phi, value_type n, value_type k)
{
    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi) || isnan(n) || isinf(n))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2) /// FIXME
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi/M_PI_2));

        if (phi < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_P(M_PI_2,k,n,0) + gsl_sf_ellint_P(fabs(phi)-nMultiple*M_PI_2, k, n, 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_P(M_PI_2,k,n,0) - gsl_sf_ellint_P(M_PI_2-(fabs(phi)-nMultiple*M_PI_2), k, n, 0));
    }

    return gsl_sf_ellint_P(phi, k, n, 0);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// elliptic intergal D(phi,n,k).
///
/// \param phi value_type
/// \param n value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_EllipticD(value_type phi, value_type n, value_type k)
{
    if (isnan(k) || isnan(phi) || isinf(k) || isinf(phi) || isnan(n) || isinf(n))
        return NAN;

    if (k < 0 || k >= 1)
        return NAN;

    if (phi < 0 || phi > M_PI_2) /// FIXME
    {
        int nSign = 1;
        int nMultiple = floor(fabs(phi/M_PI_2));

        if (phi < 0)
            nSign = -1;

        if (!(nMultiple%2)) // even
            return nSign*(nMultiple*gsl_sf_ellint_D(M_PI_2,k,n,0) + gsl_sf_ellint_D(fabs(phi)-nMultiple*M_PI_2, k, n, 0));
        else // odd
            return nSign*((nMultiple+1)*gsl_sf_ellint_D(M_PI_2,k,n,0) - gsl_sf_ellint_D(M_PI_2-(fabs(phi)-nMultiple*M_PI_2), k, n, 0));
    }

    return gsl_sf_ellint_D(phi, k, n, 0);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Beta function.
///
/// \param a value_type
/// \param b value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_beta(value_type a, value_type b)
{
    if (isnan(a) || isnan(b) || isinf(a) || isinf(b))
        return NAN;

    if ((intCast(a) == (int)a && a < 0) || (intCast(b) == (int)b && b < 0))
        return NAN;

    return gsl_sf_beta(a, b);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Zeta function.
///
/// \param n value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_zeta(value_type n)
{
    if (isnan(n) || isinf(n))
        return NAN;

    if (n == 1)
        return NAN;

    if (intCast(n) == (int)n)
        return gsl_sf_zeta_int(intCast(n));
    else
        return gsl_sf_zeta(n);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Clausen function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_clausen(value_type x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    return gsl_sf_clausen(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Digamma function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_digamma(value_type x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    if (x == 0)
        return NAN;

    if ((int)x == intCast(x) && x > 0)
        return gsl_sf_psi_int(intCast(x));
    else
        return gsl_sf_psi(x);
}


/////////////////////////////////////////////////
/// \brief This function returns the value if the
/// Polygamma function.
///
/// \param n value_type
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_polygamma(value_type n, value_type x)
{
    if (isnan(n) || isnan(x) || isinf(n) || isinf(x) || x <= 0 || n < 0)
        return NAN;

    return gsl_sf_psi_n(intCast(n), x);
}


/////////////////////////////////////////////////
/// \brief This function returns the value of the
/// Dilogarithm Li2(x).
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_dilogarithm(value_type x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    return gsl_sf_dilog(x);
}


/////////////////////////////////////////////////
/// \brief This is the floor function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_floor(value_type x)
{
    return floor(x);
}


/////////////////////////////////////////////////
/// \brief This is the roof (ceil) function.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_roof(value_type x)
{
    return ceil(x);
}


/////////////////////////////////////////////////
/// \brief This is the rect function.
///
/// \param x value_type
/// \param x0 value_type
/// \param x1 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_rect(value_type x, value_type x0, value_type x1)
{
    return (x > x1 || x < x0) ? 0 : 1;
}


/////////////////////////////////////////////////
/// \brief This function describes an interval
/// with borders of a selected type (including,
/// excluding, ignore).
///
/// \param x value_type
/// \param x0 value_type
/// \param x1 value_type
/// \param lborder value_type
/// \param rborder value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_ivl(value_type x, value_type x0, value_type x1, value_type lborder, value_type rborder)
{
    if (lborder < 0)
        lborder = 0;

    if (lborder > 2)
        lborder = 2;

    if (rborder < 0)
        rborder = 0;

    if (rborder > 2)
        rborder = 2;

    if (x < x0 && lborder)
        return 0;
    else if (x < x0)
        return 1;

    if (x == x0 && lborder != 2)
        return 1;
    else if (x == x0 && lborder == 2)
        return 0;

    if (x > x1 && rborder)
        return 0;
    else if (x > x1)
        return 1;

    if (x == x1 && rborder != 2)
        return 1;
    else if (x == x1 && rborder == 2)
        return 0;

    return 1;
}


/////////////////////////////////////////////////
/// \brief This function returns the Student
/// factor s_t for the selected degrees of
/// freedom and a confidence interval.
///
/// \param vFreedoms value_type
/// \param vAlpha value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_studentFactor(value_type vFreedoms, value_type vAlpha)
{
    if (vAlpha >= 1.0 || vAlpha <= 0.0 || vFreedoms < 2.0)
        return NAN;

    return student_t(intCast(vFreedoms), vAlpha);
}


/////////////////////////////////////////////////
/// \brief This function returns the greatest
/// common divisor of both argments.
///
/// \param n value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_gcd(value_type n, value_type k)
{
    return boost::math::gcd(intCast(n), intCast(k));
}


/////////////////////////////////////////////////
/// \brief This function returns the least common
/// multiple of both arguments.
///
/// \param n value_type
/// \param k value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_lcm(value_type n, value_type k)
{
    return boost::math::lcm(intCast(n), intCast(k));
}


/////////////////////////////////////////////////
/// \brief This function represents the Modulo
/// operator.
///
/// \param v1 value_type
/// \param v2 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_Mod(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return intCast(v1) % intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function represent the XOR
/// operator.
///
/// \param v1 value_type
/// \param v2 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_XOR(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return intCast(v1) xor intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// OR operator.
///
/// \param v1 value_type
/// \param v2 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BinOR(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;

    return intCast(v1) | intCast(v2);
}


/////////////////////////////////////////////////
/// \brief This function represents the binary
/// AND operator.
///
/// \param v1 value_type
/// \param v2 value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_BinAND(value_type v1, value_type v2)
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
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_is_string(value_type v)
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
    return _time64(0);
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
/// \param milliseconds value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_sleep(value_type milliseconds)
{
    Sleep(intCast(milliseconds));
    return intCast(milliseconds);
}


/////////////////////////////////////////////////
/// \brief Calculates the logarithm of x using
/// the base b.
///
/// \param b value_type
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_log_b(value_type b, value_type x)
{
    if (isnan(b) || isnan(x) || isinf(b) || x <= 0.0 || b <= 0.0)
        return NAN;

    if (isinf(x))
        return INFINITY;

    return log10(x) / log10(b);
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
    return 100.0*AutoVersion::MAJOR+10.0*AutoVersion::MINOR + AutoVersion::BUILD;
}


/////////////////////////////////////////////////
/// \brief This function converts UNIX time
/// values into a selected part of a time stamp.
///
/// \param vTime value_type
/// \param vType value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_date(value_type vTime, value_type vType)
{
    time_t tTime = (time_t)rint(vTime);
    int nType = (int)rint(vType);
    tm *ltm = localtime(&tTime);

    switch (nType)
    {
        case 1:
            return 1900+ltm->tm_year;
        case 2:
            return 1+ltm->tm_mon;
        case 3:
            return ltm->tm_mday;
        case 4:
            return ltm->tm_hour;
        case 5:
            return ltm->tm_min;
        case 6:
            return ltm->tm_sec;
        case -1:
            return (1900+ltm->tm_year)*10000.0+(1+ltm->tm_mon)*100.0+(ltm->tm_mday);
        case -2:
            return (ltm->tm_hour)*10000.0+(ltm->tm_min)*100.0+ltm->tm_sec;
        default:
            return ((1900+ltm->tm_year)*10000.0+(1+ltm->tm_mon)*100.0+(ltm->tm_mday))*1000000.0+(ltm->tm_hour)*10000.0+(ltm->tm_min)*100.0+ltm->tm_sec;
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief Returns, whether the selected value is
/// NaN.
///
/// \param v value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_isnan(value_type v)
{
    return v != v;
}


/////////////////////////////////////////////////
/// \brief This function numerically defines a
/// valid value range (the value is set to NaN,
/// if outside of this range).
///
/// \param v value_type
/// \param vLeft value_type
/// \param vRight value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_interval(value_type v, value_type vLeft, value_type vRight)
{
    if (vRight <= vLeft)
        return NAN;

    if (v <= vRight && v >= vLeft)
        return v;
    else
        return NAN;
}


/////////////////////////////////////////////////
/// \brief This function returns the cotangent of
/// the passed value.
///
/// \param x value_type
/// \return value_type
///
/////////////////////////////////////////////////
value_type parser_cot(value_type x)
{
    if (isnan(x) || isinf(x))
        return NAN;

    if (!sin(x))
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
    std::list<double*>* m_lDataStorage = static_cast<std::list<double*>* >(a_pUserData);

    // Create the storage for a new variable
    m_lDataStorage->push_back(new double);
    *(m_lDataStorage->back()) = 0.0;

    // Return the address of the newly created storage
    return m_lDataStorage->back();
}


