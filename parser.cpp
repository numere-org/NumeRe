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

#include "parser.hpp"

extern value_type vAns;
bool bSupressAnswer = false;
extern Integration_Vars parser_iVars;
int nErrorIndices[2] = {-1,-1};
string sErrorToken = "";
extern Plugin _plugin;
extern time_t tTimeZero;
volatile sig_atomic_t exitsignal = 0;

/*
 * Ende der globalen Variablen
 */

// --> Umrechnungsfunktionen: diese werden aufgerufen, wenn eine spezielle Syntax verwendet wird <--
value_type parser_Mega(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e6;
}

value_type parser_Milli(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-3;
}

value_type parser_Giga(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e9;
}

value_type parser_Kilo(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e3;
}

value_type parser_Micro(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-6;
}

value_type parser_Nano(value_type a_fVal)
{
    if (isinf(a_fVal) || isnan(a_fVal))
        return NAN;
    return a_fVal * 1e-9;
}

value_type parser_Not(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v==0;
}

value_type parser_Ignore(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v;
}

// --> Einheitenumrechnung: eV, fm, A, b, Torr, AU, etc... <--
value_type parser_ElectronVolt(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.60217657e-19;
}

value_type parser_Fermi(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-15;
}

value_type parser_Angstroem(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-10;
}

value_type parser_Barn(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-28;
}

value_type parser_Torr(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 101325/(double)760;
}

value_type parser_AstroUnit(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 149597870700;
}

value_type parser_Lightyear(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 9460730472580800;
}

value_type parser_Parsec(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 30856775777948584.2;
}

value_type parser_Mile(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1609.344;
}

value_type parser_Yard(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.9144;
}

value_type parser_Foot(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.3048;
}

value_type parser_Inch(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 0.0254;
}

value_type parser_Calorie(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 4.1868;
}

value_type parser_PSI(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6894.75729;
}

value_type parser_Knoten(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 463.0 / 900.0;
}

value_type parser_liter(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-3;
}

value_type parser_kmh(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v / 3.6;
}

value_type parser_mph(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1.609334 / 3.6;
}

value_type parser_Celsius(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v + 273.15;
}

value_type parser_Fahrenheit(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return (v + 459.67) * 5.0 / 9.0;
}

value_type parser_Curie(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 3.7e10;
}

value_type parser_Gauss(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-4;
}

value_type parser_Poise(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 1e-1;
}
value_type parser_mol(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v * 6.022140857E23;
}


/* --> Funktion zur Berechnung der Fakultaet einer natuerlichen Zahl. Wir wandeln hier alle
 *     value_type (double) explizit in (unsigned) integer um, da die Fakultaet nur fuer
 *     natuerliche Zahlen definiert ist. <--
 */
value_type parser_Faculty(value_type v)
{
    if (isnan(v) || isinf(v))
        return NAN;
    value_type vResult = 1.0; // Ausgabe-Variable
    // --> Falls v == 0 ist, dann ist die Fakultaet 1 und nicht 0. Fangen wir hier ab <--
    if ((int)v == 0)
        return 1;
    if ((int)v < 0)
        return NAN;

    /* --> Zaehlschleife, die die Fakultaet bildet: allerdings in der Form 1*2*3*...*(n-1)*n und nicht
     *     in der Form, wie sie normal definiert wird: n*(n-1)*(n-2)*...*3*2*1 <--
     */
    for (int i = 1; i <= abs((int)v); i++)
    {
        vResult *= i;
    }
    return vResult;
}

// --> Berechnet die Doppel-Fakultaet einer natuerlichen Zahl <--
value_type parser_doubleFaculty(value_type v)
{
    if (isnan(v) || isinf(v))
        return NAN;
    value_type vResult = 1.0;
    if ((int)v < 0)
        return NAN;
    for (int n = (int)fabs(v); n > 0; n -= 2)
    {
        vResult *= n;
    }
    return vResult;
}


/* --> Funktion zur Berechnung eines Binomialkoeffizienten aus den Werten v1 und v2. Auch
 *     hier werden die value_types in integer umgewandelt, da auch der Binomialkoeffizient
 *     nur fuer natuerliche Zahlen definiert ist <--
 */
value_type parser_Binom(value_type v1, value_type v2)
{
    if (isnan(v1) || isnan(v2) || isinf(v1) || isinf(v2))
        return NAN;
    /* --> Bevor wir die bekannte Formel verwenden, pruefen wir einige Spezialfaelle, die den
     *     Algorithmus deutlich beschleunigen. Hier sei der Artikel auf Wikipedia zum Binomial-
     *     koeffzienten empfohlen <--
     */
    if ((int)v2 < 0 || (int)v1 < 0)
        return NAN;
    else if ((int)v2 > (int)v1) // v2 > v1 ==> binom = 0!
        return 0;
    else if((int)v1 == (int)v2 || ((int)v1 != 0 && (int)v2 == 0)) // v1 == v2 oder v2 == 0 und v1 != 0 ==> binom = 1!
        return 1;
    else if((int)v2 == 1 || (int)v2 == (int)v1-1) // v2 == 1 oder v2 == v1-1 ==> binom = v1!
        return (int)v1;
    else if((int)v2 == 2 && (int)v2 < (int)v1) // v2 == 2 und v2 < v1 ==> binom = v1*(v1-1) / v2!
        return (int)v1*((int)v1-1)/(int)v2;
    else
    {
        /* --> In allen anderen Faellen muessen wir den Binomialkoeffzienten muehsam mithilfe der Formel
         *     binom(v1,v2) = v1!/(v2!*(v1-v2)!) ausrechnen. Das machen wir, indem wir die Funktion
         *     parser_Faculty(value_type) aufrufen <--
         */
        return parser_Faculty(v1) / (parser_Faculty(v2)*parser_Faculty( (value_type)( (int)v1-(int)v2 ) ));
    }
}

// --> Diese Funktion gibt einfach nur die Anzahl an Elementen in sich zurueck <--
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

// --> Diese Funktion gibt einfach nur die Anzahl an Elementen in sich zurueck <--
value_type parser_Cnt(const value_type* vElements, int nElements)
{
    return nElements;
}

// --> Diese Funktion berechnet die Standardabweichung von n Argumenten <---
value_type parser_Std(const value_type* vElements, int nElements)
{
    value_type vStd = 0.0;
    value_type vMean = 0.0;

    for (int i = 0; i < nElements; i++)
    {
        if (isnan(vElements[i]) || isinf(vElements[i]))
            return NAN;
        vMean += vElements[i];
    }
    vMean = vMean / (value_type)nElements;

    for (int i = 0; i < nElements; i++)
    {
        vStd += (vElements[i] - vMean) * (vElements[i] - vMean);
    }
    vStd = sqrt(vStd / (value_type)(nElements-1));
    return vStd;
}

// --> Diese Funktion berechnet das Produkt der gegebenen Werte <--
value_type parser_product(const value_type* vElements, int nElements)
{
    value_type vProd = 1.0;
    for (int i = 0; i < nElements; i++)
    {
        if (isinf(vElements[i]) || isnan(vElements[i]))
            return NAN;
        vProd *= vElements[i];
    }
    return vProd;
}

// --> Diese Funktion berechnet die Norm der gegebenen Werte <--
value_type parser_Norm(const value_type* vElements, int nElements)
{
    value_type vResult = 0.0;
    for (int i = 0; i < nElements; i++)
    {
        if (isinf(vElements[i]) || isnan(vElements[i]))
            return NAN;
        vResult += vElements[i] * vElements[i];
    }
    return sqrt(vResult);
}

// --> Diese Funktion berechnet den Median mehrerer Werte <--
value_type parser_Med(const value_type* vElements, int nElements)
{
    Datafile _cache;
    _cache.setCacheStatus(true);

    for (int i = 0; i < nElements; i++)
        _cache.writeToCache(i,0,"cache",vElements[i]);
    return _cache.med("cache", 0,nElements);
}

// --> Diese Funktion berechnet das x-te Perzentil mehrerer Werte <--
value_type parser_Pct(const value_type* vElements, int nElements)
{
    Datafile _cache;
    _cache.setCacheStatus(true);

    for (int i = 0; i < nElements-1; i++)
        _cache.writeToCache(i,0,"cache",vElements[i]);
    return _cache.pct("cache", 0, nElements, 0, -1, vElements[nElements-1]);
}

// --> Analogie zur Excel-Funktion VERGLEICH() <--
value_type parser_compare(const value_type* vElements, int nElements)
{
    int nType = 0;
    if (nElements < 3)
        return NAN;
    value_type vRef = vElements[nElements-2];
    value_type vKeep = vRef;
    int nKeep = -1;
    if (vElements[nElements-1] == 0)
    {
        nType = 0;
    }
    else if (vElements[nElements-1] < 0)
    {
        nType = -1;
    }
    else
    {
        nType = 1;
    }
    for (int i = 0; i < nElements-2; i++)
    {
        if (isnan(vElements[i]) || isinf(vElements[i]))
            continue;
        if (vElements[i] == vRef)
        {
            if (!nType || fabs(vElements[nElements-1]) <= 1)
                return i+1;
            else
                return vElements[i];
        }
        else if (nType == 1 && vElements[i] > vRef)
        {
            if (nKeep == -1 || vElements[i] < vKeep)
            {
                vKeep = vElements[i];
                nKeep = i;
            }
            else
                continue;
        }
        else if (nType == -1 && vElements[i] < vRef)
        {
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
    else if (vElements[nElements-1] <= -2 || vElements[nElements-1] >= 2)
        return vKeep;
    else
        return nKeep+1;
}

// --> Diese Funktion rundet einen Wert auf eine angegebene Zahl an Nachkommastellen <--
value_type parser_round(value_type vToRound, value_type vDecimals)
{
    if (isinf(vToRound) || isinf(vDecimals) || isnan(vToRound) || isnan(vDecimals))
        return NAN;
    double dDecimals = std::pow(10, -abs((int)vDecimals));
    vToRound = vToRound / dDecimals;
    vToRound = std::round(vToRound);
    vToRound = vToRound * dDecimals;
    return vToRound;
}

// --> Diese Funktion rechnet einen Gradwert in einen Radianwert um <--
value_type parser_toRadian(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v / 180.0 * M_PI;
}

// --> Diese Funktion rechnet einen Radianwert in einen Gradwert um <--
value_type parser_toDegree(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    return v / M_PI * 180.0;
}

// --> Diese Funktion berechnet den numerischen Wert der Kugelflaechenfunktionen bis zur Ordnung l = 5 <--
value_type parser_SphericalHarmonics(value_type vl, value_type vm, value_type theta, value_type phi)
{
    if (isinf(vl) || isnan(vl)
        || isinf(vm) || isnan(vm)
        || isinf(theta) || isnan(theta)
        || isinf(phi) || isnan(phi))
        return NAN;
    int l = (int)fabs(vl);
    int m = (int)vm;
    if (abs(m) > l)
    {
        return NAN;
    }
    else
    {
        return sqrt((double)(2.0*l+1.0) * parser_Faculty(l-m) / (4.0 * M_PI * parser_Faculty(l+m)))*parser_AssociatedLegendrePolynomial(l,m,cos(theta))*cos(m*phi);
    }
    return 0.0;
}

// --> Diese Funktion berechneten den sinc(x) <--
value_type parser_SinusCardinalis(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    if (!v)
        return 1.0;
    else
        return sin(v)/v;
}

// --> Diese Funktion berechnet den numerischen Wert der sphaerischen Besselfunktionen bis zur Ordnung n = 5 <--
value_type parser_SphericalBessel(value_type vn, value_type v)
{
    if (isinf(vn) || isinf(v) || isnan(vn) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);
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
        return gsl_sf_bessel_jl((int)vn,fabs(v));
        /*long double dResult = 0.0;
        for (int k = 0; k <= 30+2*n; k++)
        {
            dResult += (long double)(intPower(-v*v, k)*parser_Faculty(k+n)/(parser_Faculty(k)*parser_Faculty(2*(k+n)+1)));
        }
        dResult *= (long double)(intPower(2*v,n));
        return dResult;*/
    }
    return 0.0;
}

// --> Diese Funktion berechnet den numerischen Wert der sphaerischen Neumannfunktionen bis zur Ordnung n = 5 <--
value_type parser_SphericalNeumann(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);
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
        return gsl_sf_bessel_yl((int)vn, fabs(v));
        /*long double dResult = 0.0;
        for (int k = 0; k <= 30+2*n; k++)
        {
            dResult += (long double)(intPower(-v*v, k)*parser_Faculty(k-n)/(parser_Faculty(k)*parser_Faculty(2*(k-n))));
        }
        dResult *= (long double)(intPower(-v,-(n+1))*intPower(2,-n));
        return dResult;*/
    }
    return 0.0;
}

// --> Diese Funktion berechnet den numerischen Wert der Legendre-Polynome bis zur Ordnung n = infty <--
value_type parser_LegendrePolynomial(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);

    long double dResult = 0.0;
    for (int k = 0; k <= n/2; k++)
    {
        dResult += (long double)(intPower(-1,k)*parser_Binom(n,k)*parser_Binom(2*(n-k),n)*intPower(v,n-2*k));
    }
    dResult *= intPower(2, -n);
    return dResult;
}

// --> Diese Funktion berechnet den numerischen Wert der Assoziierten Legendre-Polynome bis zur Ordnung n = 6 <--
value_type parser_AssociatedLegendrePolynomial(value_type vl, value_type vm, value_type v)
{
    if (isinf(vl) || isnan(vl) || isinf(vm) || isnan(vm) || isinf(v) || isnan(v))
        return NAN;
    int l = (int)fabs(vl);
    int m = (int)fabs(vm);
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

// --> Diese Funktion berechnet den numerischen Wert der Laguerre-Polynome bis zur Ordnung n = infty <--
value_type parser_LaguerrePolynomial(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);

    long double dResult = 0.0;
    for (int k = 0; k <= n; k++)
    {
        dResult += (long double)(intPower(-v,k)*parser_Binom(n,k)/parser_Faculty(k));
    }
    return dResult;
}

// --> Diese Funktion berechnet den numerischen Wert der Assoziierten Laguerre-Polynome bis zur Ordnung n = infty <--
value_type parser_AssociatedLaguerrePolynomial(value_type vn, value_type vk, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(vk) || isnan(vk) || isinf(v) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);
    int k = (int)fabs(vk);
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

// --> Diese Funktion berechnet den numerischen Wert der Hermite-Polynome bis zur Ordnung n = infty <--
value_type parser_HermitePolynomial(value_type vn, value_type v)
{
    if (isinf(vn) || isnan(vn) || isinf(v) || isnan(v))
        return NAN;
    int n = (int)fabs(vn);

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

// --> Diese Funktion berechnet den numerischen Wert der Kernbindungsenergie nach Bethe-Weizsaecker <--
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
    unsigned int N = (unsigned int)parser_round(vN,0);
    unsigned int Z = (unsigned int)parser_round(vZ,0);


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

// --> Heaviside-(Theta-)Funktion <--
value_type parser_Heaviside(value_type v)
{
    if (isinf(v) || isnan(v))
        return NAN;
    if (v < 0.0)
        return 0.0;
    else
        return 1.0;
}

// --> Azimuthwinkel phi <--
value_type parser_phi(value_type x, value_type y)
{
    if (isinf(x) || isnan(x) || isinf(y) || isnan(y))
        return NAN;
    if (y < 0)
        return M_PI+abs(M_PI + atan2(y,x));
    return atan2(y,x);
}

// --> Polarwinkel theta <--
value_type parser_theta(value_type x, value_type y, value_type z)
{
    if (isinf(x) || isnan(x) || isinf(y) || isnan(y) || isinf(z) || isnan(z))
        return NAN;
    if (!x && !y && !z)
        return M_PI/2;
    return acos(z/sqrt(x*x+y*y+z*z));
}

// --> Generiert eine Zufallszahl zwischen vRandMin und vRandMax <--
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

// --> Generiert eine Zufallszahl zwischen vRandMin und vRandMax <--
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

// --> Berechnet die Gauss'sche Fehlerfunktion von x <--
value_type parser_erf(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return erf(x);
}

// --> Berechnet die komplementaere Gauss'sche Fehlerfunktion von x <--
value_type parser_erfc(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return erfc(x);
}

// --> Berechnet den Wert der Gammafunktion an der Stelle x <--
value_type parser_gamma(value_type x)
{
    if (isinf(x) || isnan(x))
        return NAN;
    return tgamma(x);
}

// --> Airy-Funktion Ai(x) <--
value_type parser_AiryA(value_type x)
{
    return gsl_sf_airy_Ai(x, GSL_PREC_DOUBLE);
}

// --> Airy-Funktion Bi(x) <--
value_type parser_AiryB(value_type x)
{
    return gsl_sf_airy_Bi(x, GSL_PREC_DOUBLE);
}

// --> Bessel J_n(x) <--
value_type parser_RegularCylBessel(value_type n, value_type x)
{
    if (n >= 0.0)
        return gsl_sf_bessel_Jn((int)n, x);
    else
        return NAN;
}

// --> Bessel Y_n(x) <--
value_type parser_IrregularCylBessel(value_type n, value_type x)
{
    if (x != 0.0 && n >= 0.0)
        return x/fabs(x)*gsl_sf_bessel_Yn((int)n, fabs(x));
    else
        return -INFINITY;
}

// --> floor-Funktion <--
value_type parser_floor(value_type x)
{
    return floor(x);
}

// --> roof-Funktion <--
value_type parser_roof(value_type x)
{
    return ceil(x);
}

// --> Rechteckfunktion <--
value_type parser_rect(value_type x, value_type x0, value_type x1)
{
    return (x > x1 || x < x0) ? 0 : 1;
}

// --> Student-Faktor <--
value_type parser_studentFactor(value_type vFreedoms, value_type vAlpha)
{
    if (vAlpha >= 1.0 || vAlpha <= 0.0 || vFreedoms < 2.0)
        return NAN;
    boost::math::students_t dist((int)vFreedoms-1);
    return boost::math::quantile(boost::math::complement(dist, (1.0-vAlpha)/2.0));
}

// --> Greatest commom divisor <--
value_type parser_gcd(value_type n, value_type k)
{
    return boost::math::gcd((int)n, (int)k);
}

// --> Least common multiple <--
value_type parser_lcm(value_type n, value_type k)
{
    return boost::math::lcm((int)n, (int)k);
}


// --> Modulo-Operator <--
value_type parser_Mod(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;
    return (int)v1 % (int)v2;
}

// --> XOR-Operator <--
value_type parser_XOR(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;
    return (v1 && !v2) || (!v1 && v2);
}

// --> binary OR-Operator <--
value_type parser_BinOR(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;
    return ((int)v1 | (int)v2);
}

// --> binary AND-Operator <--
value_type parser_BinAND(value_type v1, value_type v2)
{
    if (isinf(v1) || isnan(v1) || isinf(v2) || isnan(v2))
        return NAN;
    return ((int)v1 & (int)v2);
}

// --> value_type-Repraesentation der is_string()-Funktion <--
value_type parser_is_string(value_type v)
{
    if (isnan(v) || isinf(v))
        return NAN;
    return 0.0;
}

// --> Gibt die Zeit zurueck <--
value_type parser_time()
{
    return time(0);
}

// --> Wandelt Zeitangaben in Sekunden in ein Datum um <--
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

// --> Gibt true zurueck, wenn der Wert == NAN, false sonst <--
value_type parser_isnan(value_type v)
{
    return v != v;
}

// --> Beschraenkt einen Ausdruck auf ein Intervall <--
value_type parser_interval(value_type v, value_type vLeft, value_type vRight)
{
    if (vRight <= vLeft)
        return NAN;
    if (v <= vRight && v >= vLeft)
        return v;
    else
        return NAN;
}

// --> Diese Funktion wird zu Beginn von NumeRe aufgerufen und testet den muParser <--
value_type parser_SelfTest(Parser& _parser)
{
    mu::Test::ParserTester pt;
    pt.Run();
    return 0;
}

void terminationHandler(int nSigNum)
{
    exitsignal = 1;
    return;
}

BOOL WINAPI EventHandler(DWORD dwCtrlType)
{
    return TRUE;
}


// --> Zentrale Funktion: dies repraesentiert die Grundfunktionalitaet des NumeRe-Rechner-Modus <--
int parser_Calc(Datafile& _data, Output& _out, Parser& _parser, Settings& _option, Define& _functions, PlotData& _pData, Script& _script, Procedure& _procedure, ofstream& oLogFile)
{
    _parser.DefineVar(_T("ans"), &vAns);        // Deklariere die spezielle Variable "ans", die stets, das letzte Ergebnis speichert und die vier Standardvariablen
    _parser.DefineVar(parser_iVars.sName[0], &parser_iVars.vValue[0][0]);
    _parser.DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);
    _parser.DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);
    _parser.DefineVar(parser_iVars.sName[3], &parser_iVars.vValue[3][0]);

    // --> Syntax fuer die Umrechnungsfunktionen definieren und die zugehoerigen Funktionen deklarieren <--
    _parser.DefinePostfixOprt(_T("'G"), parser_Giga);
    _parser.DefinePostfixOprt(_T("'M"), parser_Mega);
    _parser.DefinePostfixOprt(_T("'k"), parser_Kilo);
    _parser.DefinePostfixOprt(_T("'m"), parser_Milli);
    _parser.DefinePostfixOprt(_T("'mu"), parser_Micro);
    //_parser.DefinePostfixOprt(_T("µ"), parser_Micro);
    _parser.DefinePostfixOprt(_T("'n"), parser_Nano);
    _parser.DefinePostfixOprt(_T("~"), parser_Ignore);

    // --> Einheitenumrechnungen: Werden aufgerufen durch WERT'EINHEIT <--
    _parser.DefinePostfixOprt(_T("'eV"), parser_ElectronVolt);
    _parser.DefinePostfixOprt(_T("'fm"), parser_Fermi);
    _parser.DefinePostfixOprt(_T("'A"), parser_Angstroem);
    _parser.DefinePostfixOprt(_T("'b"), parser_Barn);
    _parser.DefinePostfixOprt(_T("'Torr"), parser_Torr);
    _parser.DefinePostfixOprt(_T("'AU"), parser_AstroUnit);
    _parser.DefinePostfixOprt(_T("'ly"), parser_Lightyear);
    _parser.DefinePostfixOprt(_T("'pc"), parser_Parsec);
    _parser.DefinePostfixOprt(_T("'mile"), parser_Mile);
    _parser.DefinePostfixOprt(_T("'yd"), parser_Yard);
    _parser.DefinePostfixOprt(_T("'ft"), parser_Foot);
    _parser.DefinePostfixOprt(_T("'in"), parser_Inch);
    _parser.DefinePostfixOprt(_T("'cal"), parser_Calorie);
    _parser.DefinePostfixOprt(_T("'psi"), parser_PSI);
    _parser.DefinePostfixOprt(_T("'kn"), parser_Knoten);
    _parser.DefinePostfixOprt(_T("'l"), parser_liter);
    _parser.DefinePostfixOprt(_T("'kmh"), parser_kmh);
    _parser.DefinePostfixOprt(_T("'mph"), parser_mph);
    _parser.DefinePostfixOprt(_T("'TC"), parser_Celsius);
    _parser.DefinePostfixOprt(_T("'TF"), parser_Fahrenheit);
    _parser.DefinePostfixOprt(_T("'Ci"), parser_Curie);
    _parser.DefinePostfixOprt(_T("'Gs"), parser_Gauss);
    _parser.DefinePostfixOprt(_T("'Ps"), parser_Poise);
    _parser.DefinePostfixOprt(_T("'mol"), parser_mol);

    // --> Logisches NICHT <--
    _parser.DefineInfixOprt(_T("!"), parser_Not);

    // --> Eigene Konstanten <--
    _parser.DefineConst(_T("_g"), 9.80665);
    _parser.DefineConst(_T("_c"), 299792458);
    _parser.DefineConst(_T("_elek_feldkonst"), 8.854187817e-12);
    _parser.DefineConst(_T("_n_avogadro"), 6.02214129e23);
    _parser.DefineConst(_T("_k_boltz"), 1.3806488e-23);
    _parser.DefineConst(_T("_elem_ladung"), 1.602176565e-19);
    _parser.DefineConst(_T("_h"), 6.62606957e-34);
    _parser.DefineConst(_T("_hbar"), 1.054571726e-34);
    _parser.DefineConst(_T("_m_elektron"), 9.10938291e-31);
    _parser.DefineConst(_T("_m_proton"), 1.672621777e-27);
    _parser.DefineConst(_T("_m_neutron"), 1.674927351e-27);
    _parser.DefineConst(_T("_m_muon"), 1.883531475e-28);
    _parser.DefineConst(_T("_m_tau"), 3.16747e-27);
    _parser.DefineConst(_T("_magn_feldkonst"), 1.25663706144e-6);
    _parser.DefineConst(_T("_m_erde"), 5.9726e24);
    _parser.DefineConst(_T("_m_sonne"), 1.9885e30);
    _parser.DefineConst(_T("_r_erde"), 6.378137e6);
    _parser.DefineConst(_T("_r_sonne"), 6.9551e8);
    _parser.DefineConst(_T("true"), 1);
    _parser.DefineConst(_T("_theta_weinberg"), 0.49097621387892);
    _parser.DefineConst(_T("false"), 0);
    _parser.DefineConst(_T("_2pi"), 6.283185307179586476925286766559);
    _parser.DefineConst(_T("_R"), 8.3144622);
    _parser.DefineConst(_T("_alpha_fs"), 7.2973525698E-3);
    _parser.DefineConst(_T("_mu_bohr"), 9.27400968E-24);
    _parser.DefineConst(_T("_mu_kern"), 5.05078353E-27);
    _parser.DefineConst(_T("_m_amu"), 1.660538921E-27);
    _parser.DefineConst(_T("_r_bohr"), 5.2917721092E-11);
    _parser.DefineConst(_T("_G"), 6.67384E-11);
    _parser.DefineConst(_T("_coul_norm"), 8987551787.99791145324707);
    _parser.DefineConst(_T("nan"), NAN);
    _parser.DefineConst(_T("inf"), INFINITY);
    _parser.DefineConst(_T("void"), NAN);

    // --> Die Fakultaet und den Binomialkoeffzienten als mathemat. Funktion deklarieren <--
    _parser.DefineFun(_T("faculty"), parser_Faculty, false);                        // faculty(n)
    _parser.DefineFun(_T("dblfacul"), parser_doubleFaculty, false);                 // dblfacul(n)
    _parser.DefineFun(_T("binom"), parser_Binom, false);                            // binom(Wert1,Wert2)
    _parser.DefineFun(_T("num"), parser_Num, true);                                 // num(a,b,c,...)
    _parser.DefineFun(_T("cnt"), parser_Cnt, true);                                 // num(a,b,c,...)
    _parser.DefineFun(_T("std"), parser_Std, false);                                // std(a,b,c,...)
    _parser.DefineFun(_T("prd"), parser_product, false);                            // prd(a,b,c,...)
    _parser.DefineFun(_T("round"), parser_round, false);                            // round(x,n)
    _parser.DefineFun(_T("radian"), parser_toRadian, true);                         // radian(alpha)
    _parser.DefineFun(_T("degree"), parser_toDegree, true);                         // degree(x)
    _parser.DefineFun(_T("Y"), parser_SphericalHarmonics, true);                    // Y(l,m,theta,phi)
    _parser.DefineFun(_T("sinc"), parser_SinusCardinalis, true);                    // sinc(x)
    _parser.DefineFun(_T("sbessel"), parser_SphericalBessel, true);                 // sbessel(n,x)
    _parser.DefineFun(_T("sneumann"), parser_SphericalNeumann, true);               // sneumann(n,x)
    _parser.DefineFun(_T("bessel"), parser_RegularCylBessel, true);                 // bessel(n,x)
    _parser.DefineFun(_T("neumann"), parser_IrregularCylBessel, true);              // neumann(n,x)
    _parser.DefineFun(_T("legendre"), parser_LegendrePolynomial, true);             // legendre(n,x)
    _parser.DefineFun(_T("legendre_a"), parser_AssociatedLegendrePolynomial, true); // legendre_a(l,m,x)
    _parser.DefineFun(_T("laguerre"), parser_LaguerrePolynomial, true);             // laguerre(n,x)
    _parser.DefineFun(_T("laguerre_a"), parser_AssociatedLaguerrePolynomial, true); // laguerre_a(n,k,x)
    _parser.DefineFun(_T("hermite"), parser_HermitePolynomial, true);               // hermite(n,x)
    _parser.DefineFun(_T("betheweizsaecker"), parser_BetheWeizsaecker, true);       // betheweizsaecker(N,Z)
    _parser.DefineFun(_T("heaviside"), parser_Heaviside, true);                     // heaviside(x)
    _parser.DefineFun(_T("phi"), parser_phi, true);                                 // phi(x,y)
    _parser.DefineFun(_T("theta"), parser_theta, true);                             // theta(x,y,z)
    _parser.DefineFun(_T("norm"), parser_Norm, true);                               // norm(x,y,z,...)
    _parser.DefineFun(_T("med"), parser_Med, true);                                 // med(x,y,z,...)
    _parser.DefineFun(_T("pct"), parser_Pct, true);                                 // pct(x,y,z,...)
    _parser.DefineFun(_T("rand"), parser_Random, false);                            // rand(left,right)
    _parser.DefineFun(_T("gauss"), parser_gRandom, false);                          // gauss(mean,std)
    _parser.DefineFun(_T("erf"), parser_erf, false);                                // erf(x)
    _parser.DefineFun(_T("erfc"), parser_erfc, false);                              // erfc(x)
    _parser.DefineFun(_T("gamma"), parser_gamma, false);                            // gamma(x)
    _parser.DefineFun(_T("cmp"), parser_compare, false);                            // cmp(crit,a,b,c,...,type)
    _parser.DefineFun(_T("is_string"), parser_is_string, false);                    // is_string(EXPR)
    _parser.DefineFun(_T("to_value"), parser_Ignore, false);                        // to_value(STRING)
    _parser.DefineFun(_T("time"), parser_time, false);                              // time()
    _parser.DefineFun(_T("date"), parser_date, false);                              // date(TIME,TYPE)
    _parser.DefineFun(_T("is_nan"), parser_isnan, true);                            // is_nan(x)
    _parser.DefineFun(_T("range"), parser_interval, true);                          // range(x,left,right)
    _parser.DefineFun(_T("Ai"), parser_AiryA, true);                                // Ai(x)
    _parser.DefineFun(_T("Bi"), parser_AiryB, true);                                // Bi(x)
    _parser.DefineFun(_T("floor"), parser_floor, true);                             // floor(x)
    _parser.DefineFun(_T("roof"), parser_roof, true);                               // roof(x)
    _parser.DefineFun(_T("rect"), parser_rect, true);                               // rect(x,x0,x1)
    _parser.DefineFun(_T("student_t"), parser_studentFactor, true);                 // student_t(number,confidence)
    _parser.DefineFun(_T("gcd"), parser_gcd, true);                                 // gcd(x,y)
    _parser.DefineFun(_T("lcm"), parser_lcm, true);                                 // lcm(x,y)

    // --> Operatoren <--
    _parser.DefineOprt(_T("%"), parser_Mod, prMUL_DIV, oaLEFT, true);
    _parser.DefineOprt(_T("|||"), parser_XOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt(_T("|"), parser_BinOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt(_T("&"), parser_BinAND, prLOGIC, oaLEFT, true);

    // --> VAR-FACTORY Deklarieren (Irgendwo muessen die ganzen Variablen-Werte ja auch gespeichert werden) <--
    _parser.SetVarFactory(parser_AddVariable, &_parser);


    if (!BI_FileExists(_option.getExePath()+"\\numere.ini"))
    {
        // --> Ist das der erste Start? Dann zeigen wir die "Firststart"-Infos an <--
        parser_FirstStart(_option);
        cerr << "|" << endl;
        cerr << LineBreak("|-> Das war erst einmal das Wichtigste. Nun ist es an dir, dich hier einfach mal in aller Ruhe umzusehen. Versuch einfach mal, einen Ausdruck oder ein Kommando einzugeben!$Und falls mal was schief geht: Hilfe findest du unter \"help THEMA\" oder \"find BEGRIFFE\". Und wenn ich gar nicht verstehe, was du willst, werde ich auch selbst versuchen, den passenden Artikel in der Hilfe zu finden ...$(Mit \"quit\" beendest du mich)", _option) << endl;
    }
    else
    {
        // --> Wenn das nicht der erste Start ist: Kurzuebersicht ueber die Parser-Kommandos <--
        parser_Help(_option);
        if (_option.getbShowHints())
            doc_TipOfTheDay(_option);
        cerr << "|" << endl;
        cerr << LineBreak("|-> Einen Ausdruck oder ein Kommando eingeben ...$(Siehe \"help\" oder \"help expression\" für weitere Informationen)", _option) << endl;

    }

    /*cerr << (unsigned short)(BYTE)'Ä' << "Ä " << (BYTE)142 << " "
         << (unsigned short)(BYTE)'ä' << "ä " << (BYTE)132 << " "
         << (unsigned short)(BYTE)'Ö' << "Ö " << (BYTE)153 << " "
         << (unsigned short)(BYTE)'ö' << "ö " << (BYTE)148 << " "
         << (unsigned short)(BYTE)'Ü' << "Ü " << (BYTE)154 << " "
         << (unsigned short)(BYTE)'ü' << "ü " << (BYTE)129 << " "
         << (unsigned short)(BYTE)'ß' << "ß " << (BYTE)225 << endl;
    cerr << "Teststring: " << toSystemCodePage("ÄaäAÖöoOÜeEuUiIüß") << endl;*/
    //cerr << (unsigned short)(BYTE)'§' << "§" << (BYTE)245 << endl;
    // --> Deklarieren der benoetigten Variablen <--
    int i_pos[2];               // Index-Variablen fuer die Speichervariable (Zeilen)
    string si_pos[2];           // String-Index-Variablen (Zeilen)
    int j_pos[2];               // siehe Oben (Spalten)
    string sj_pos[2];           // siehe Oben (Spalten)
    //bool bSegmentationFault;    // Fehler-Bool
    bool bWriteToCache;         // TRUE, wenn das/die errechneten Ergebnisse in den Cache geschrieben werden sollen
    bool bMultLinCol[2];        // TRUE, wenn es sich nicht um ein einzelnes Element, sondern einen ganzen
                                // Zeilen-/Spalten-Bereich handelt
    string sLine_Temp;          // Temporaerer String fuer die Eingabe
    string sCache;              // Zwischenspeicher fuer die Cache-Koordinaten
    string sKeep = "";          // Zwei '\' am Ende einer Zeile ermoeglichen es, dass die Eingabe auf mehrere Zeilen verteilt wird.
                                // Die vorherige Zeile wird hierin zwischengespeichert
    string sCmdCache = "";
    string sPlotCompose = "";
    string sLine = "";
    value_type* v = 0;          // Ergebnisarray
    int nNum = 0;               // Zahl der Ergebnisse in value_type* v

    /* --> FOR-Loop ohne Bedingungen ist eigentlich eine Endlos-Schleife. Wir gehen durch ein Kommando sicher,
     *     dass diese Schleife auch wieder verlassen werden kann <--
     */
    //SetConsoleCtrlHandler((PHANDLER_ROUTINE)EventHandler,TRUE);
    //_parser.SetExpr("ans");
    signal(SIGTERM, terminationHandler);
    signal(SIGABRT, terminationHandler);
    signal(SIGINT, SIG_IGN);
    while (!exitsignal)
    {
        // --> Bei jedem Schleifendurchlauf muessen die benoetigten Variablen zurueckgesetzt werden <--
        for (int i = 0; i < 2; i++)
        {
            i_pos[i] = -1;
            j_pos[i] = -1;
            si_pos[i] = "";
            sj_pos[i] = "";
            bMultLinCol[i] = false;
        }
        if (_parser.mVarMapPntr)
            _parser.mVarMapPntr = 0;
        cerr.precision(_option.getPrecision());
        cin.clear();

        _parser.ClearVectorVars();
        //bSegmentationFault = false;
        bWriteToCache = false;
        bSupressAnswer = false;
        sLine_Temp = "";
        sCache = "";

        if (_procedure.getPath() != _option.getProcsPath())
        {
            _procedure.setPath(_option.getProcsPath(), true, _procedure.getProgramPath());
            _option.setProcPath(_procedure.getPath());
        }
        // --> "try {...} catch() {...}" verwenden wir, um Parser-Exceptions abzufangen und korrekt auszuwerten <--
        try
        {
            if (!sCmdCache.length())
            {
                if (_data.pausedOpening())
                {
                    _data.openFromCmdLine(_option, "", true);
                    if (_data.isValid())
                    {
                        cerr << "|" << endl;
                        cerr << LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Daten aus " <<_data.getDataFileName("data") << " wurden in den Speicher geladen." << endl;
                    }
                }
                if (_script.getAutoStart())
                {
                    cerr << "|" << endl;
                    cerr << LineBreak("|-> Starte Script \"" + _script.getScriptFileName() + "\" ...", _option) << endl;
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Starte Script " << _script.getScriptFileName() << endl;
                    _script.openScript();
                }
                if (_script.wasLastCommand())
                {
                    cerr << LineBreak("|-> Die Ausführung des Scripts \"" + _script.getScriptFileName() + "\" wurde erfolgreich abgeschlossen.", _option) << endl;
                    _data.setPluginCommands(_procedure.getPluginNames());
                }
                // --> Werden die Eingaben gerade in "_procedure" geschrieben ? <--
                if (_procedure.getLoop() == 0 && !_procedure.is_writing() && !sPlotCompose.length())
                {
                    // --> Nein? Uebliche Eingabe <--
                    if (_option.getbDebug())
                    {
                        cerr << "|-> DEBUG: _data.getLastSaved() = " << _data.getLastSaved() << endl;
                        cerr << "|-> DEBUG: time(0) = " << time(0) << endl;
                    }

                    /* --> Falls ungespeicherte Eintraege im Cache liegen und der schon seit dem gegebenen
                     *     Intervall nicht mehr gespeichert wurde, wird das hier erledigt <--
                     */
                    if (time(0) - _data.getLastSaved() >= _option.getAutoSaveInterval())
                        BI_Autosave(_data, _out, _option);

                    // --> Wenn gerade ein Script ausgefuehrt wird, brauchen wir die Eingabe-Pfeile nicht <--
                    if (!(_script.isValid() && _script.isOpen()) && !_option.readCmdCache().length())
                    {
                        if (!sKeep.length())
                            cerr << "|" << endl;
                        cerr << "|<- ";
                    }
                    if (_option.readCmdCache().length())
                        cerr << "|" << endl;
                }
                else if (_procedure.getLoop() && !(_script.isValid() && _script.isOpen()))
                {
                    // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                    cerr << "|" << _procedure.getCurrentBlock();
                    if (_procedure.getCurrentBlock() == "IF")
                    {
                        if (_procedure.getLoop() > 1)
                            cerr << "---";
                        else
                            cerr << "-";
                    }
                    else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getLoop() > 1)
                        cerr << "-";
                    else
                    {
                        if (_procedure.getLoop() > 1)
                            cerr << "--";
                    }
                    cerr << std::setfill('-') << std::setw(2*_procedure.getLoop()-2) << "> ";
                }
                else if (_procedure.is_writing() && !(_script.isValid() && _script.isOpen()))
                {
                    cerr << "|PROC> ";
                }
                else if (!_procedure.is_writing() && !(_script.isValid() && _script.isOpen()) && sPlotCompose.length())
                {
                    cerr << "|COMP> ";
                }

                _data.setCacheStatus(false);
                // --> Erneuere den Fenstertitel der Konsole <--
                SetConsTitle(_data, _option, _script.getScriptFileNameShort());

                // --> Einlese-Variable <--
                sLine = "";

                // --> Wenn gerade ein Script aktiv ist, lese dessen naechste Zeile, sonst nehme eine Zeile von std::cin <--
                if (_script.isValid() && _script.isOpen())
                    sLine = _script.getNextScriptCommand();
                else if (_option.readCmdCache().length())
                {
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Führe übergebene Kommandozeilenparameter aus:" << endl;
                    sLine = _option.readCmdCache(true);
                }
                else
                {
                    std::getline(std::cin, sLine);
                }
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sLine = " << sLine << endl;
                // --> Leerzeichen und Tabulatoren entfernen <--
                StripSpaces(sLine);
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == '\t')
                        sLine[i] = ' ';
                }
                if (findCommand(sLine).sString != "help"
                    && findCommand(sLine).sString != "find"
                    && findCommand(sLine).sString != "search"
                    && (sLine.find('(') != string::npos || sLine.find('{') != string::npos))
                {
                    if (!validateParenthesisNumber(sLine))
                        throw UNMATCHED_PARENTHESIS;
                }

                // --> Keine Laenge? Ignorieren! <--
                if (!sLine.length() || sLine[0] == '@')
                    continue;
                if (sLine.find("<helpindex>") != string::npos && sLine.find("</helpindex>") != string::npos)
                {
                    _procedure.addHelpIndex(sLine.substr(0,sLine.find("<<>>")),getArgAtPos(sLine, sLine.find("id=")+3));
                    sLine.erase(0,sLine.find("<<>>")+4);
                    _option.addToDocIndex(sLine);
                    _plugin = _procedure;
                    continue;
                }

                // --> Kommando "global" entfernen <--
                if (findCommand(sLine).sString == "global")
                {
                    sLine = sLine.substr(findCommand(sLine).nPos+6);
                    StripSpaces(sLine);
                }
                // --> Wenn die Laenge groesser als 2 ist, koennen '\' am Ende sein <--
                if (sLine.length() > 2)
                {
                    if (sLine.substr(sLine.length()-2,2) == "\\\\")
                    {
                        // --> Ergaenze die Eingabe zu sKeep und beginne einen neuen Schleifendurchlauf <--
                        sKeep += sLine.substr(0,sLine.length()-2);
                        continue;
                    }
                }

                /* --> Steht etwas in sKeep? Ergaenze die aktuelle Eingabe, weise dies
                 *     wiederum an sLine zu und loesche den Inhalt von sKeep <--
                 */
                if (sKeep.length())
                {
                    sKeep += sLine;
                    sLine = sKeep;
                    sKeep = "";
                }
            }
            if ((sCmdCache.length() || sLine.find(';') != string::npos) && !_procedure.is_writing() && findCommand(sLine).sString != "procedure")
            {
                //cerr << sCmdCache << endl;
                //cerr << sLine << endl;
                if (sCmdCache.length())
                {
                    while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                        sCmdCache.erase(0,1);
                    if (!sCmdCache.length())
                        continue;
                    if (sCmdCache.find(';') != string::npos)
                    {
                        for (unsigned int i = 0; i < sCmdCache.length(); i++)
                        {
                            if (sCmdCache[i] == ';' && !isInQuotes(sCmdCache, i))
                            {
                                bSupressAnswer = true;
                                sLine = sCmdCache.substr(0,i);
                                sCmdCache.erase(0,i+1);
                                break;
                            }
                            if (i == sCmdCache.length()-1)
                            {
                                sLine = sCmdCache;
                                sCmdCache.clear();
                                break;
                            }
                        }
                    }
                    else
                    {
                        sLine = sCmdCache;
                        sCmdCache.clear();
                    }
                }
                else if (sLine.find(';') == sLine.length()-1)
                {
                    bSupressAnswer = true;
                    sLine.pop_back();
                }
                else
                {
                    for (unsigned int i = 0; i < sLine.length(); i++)
                    {
                        if (sLine[i] == ';' && !isInQuotes(sLine, i))
                        {
                            if (i != sLine.length()-1)
                                sCmdCache = sLine.substr(i+1);
                            sLine.erase(i);
                            bSupressAnswer = true;
                        }
                        if (i == sLine.length()-1)
                        {
                            break;
                        }
                    }
                }
            }
            if (oLogFile.is_open())
                oLogFile << toString(time(0) - tTimeZero, true) << "> " << sLine << endl;
            if (GetAsyncKeyState(VK_ESCAPE) && _script.isValid() && _script.isOpen())
            {
                if (_option.getbUseESCinScripts())
                    throw PROCESS_ABORTED_BY_USER;
            }
            GetAsyncKeyState(VK_ESCAPE);
            if ((findCommand(sLine).sString == "compose"
                    || findCommand(sLine).sString == "endcompose"
                    || sPlotCompose.length())
                && findCommand(sLine).sString != "quit"
                && findCommand(sLine).sString != "help")
            {
                if (!sPlotCompose.length() && findCommand(sLine).sString == "compose")
                {
                    sPlotCompose = "plotcompose ";
                    continue;
                }
                else if (findCommand(sLine).sString == "abort")
                {
                    sPlotCompose = "";
                    cerr << "|-> Deklaration abgebrochen." << endl;
                    continue;
                }
                else if (findCommand(sLine).sString != "endcompose")
                {
                    string sCommand = findCommand(sLine).sString;
                    if (sCommand.substr(0,4) == "plot"
                        || sCommand.substr(0,4) == "grad"
                        || sCommand.substr(0,4) == "dens"
                        || sCommand.substr(0,4) == "draw"
                        || sCommand.substr(0,4) == "vect"
                        || sCommand.substr(0,4) == "cont"
                        || sCommand.substr(0,4) == "surf"
                        || sCommand.substr(0,4) == "mesh")
                        sPlotCompose += sLine + " <<COMPOSE>> ";
                    continue;
                }
                else
                {
                    sLine = sPlotCompose;
                    sPlotCompose = "";
                }
            }

            if (_script.isValid() && _script.isOpen() && _script.installProcedures() && _script.getInstallInfoString().length())
            {
                if (matchParams(_script.getInstallInfoString(), "type", '='))
                {
                    if (getArgAtPos(_script.getInstallInfoString(), matchParams(_script.getInstallInfoString(), "type", '=')).find("TYPE_PLUGIN") != string::npos)
                    {
                        _procedure.declareNewPlugin(_script.getInstallInfoString());
                        _plugin = _procedure;
                    }
                }
            }
            if (findCommand(sLine).sString == "uninstall")
            {
                string sPlugin = fromSystemCodePage(getArgAtPos(sLine, findCommand(sLine).nPos+9));
                sPlugin = _procedure.deletePlugin(sPlugin);
                if (sPlugin.length())
                {
                    _plugin = _procedure;
                    if (sPlugin != "<<NO_HLP_ENTRY>>")
                    {
                        while (sPlugin.find(';') != string::npos)
                            sPlugin[sPlugin.find(';')] = ',';
                        while (sPlugin.length())
                        {
                            _option.removeFromDocIndex(getNextArgument(sPlugin, true));
                        }
                    }
                    cerr << "|-> Plugin erfolgreich entfernt." << endl;
                }
                else
                    cerr << "|-> Das Plugin konnte nicht gefunden werden." << endl;
                continue;
            }

            if (_procedure.is_writing() || findCommand(sLine).sString == "procedure")
            {
                if (!_procedure.writeProcedure(sLine))
                    cerr << LineBreak("|-> FEHLER: Konnte die Prozedur nicht in eine Datei schreiben!", _option) << endl;
                continue;
            }

            if (sLine.find("to_cmd(") != string::npos && !_procedure.getLoop())
            {
                unsigned int nPos = 0;
                while (sLine.find("to_cmd(", nPos) != string::npos)
                {
                    nPos = sLine.find("to_cmd(", nPos) + 6;
                    if (isInQuotes(sLine, nPos))
                        continue;
                    unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));
                    if (nParPos == string::npos)
                        throw UNMATCHED_PARENTHESIS;
                    string sCmdString = sLine.substr(nPos+1, nParPos-1);
                    StripSpaces(sCmdString);
                    if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString))
                    {
                        sCmdString += " -nq";
                        parser_StringParser(sCmdString, sCache, _data, _parser, _option, true);
                        sCache = "";
                    }
                    sLine = sLine.substr(0, nPos-6) + sCmdString + sLine.substr(nPos + nParPos+1);
                    nPos -= 5;
                }
            }
            // --> Prozeduren abarbeiten <--
            if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos && !_procedure.getLoop())
            {
                //cerr << sLine << endl;
                unsigned int nPos = 0;
                int nProc = 0;
                while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
                {
                    unsigned int nParPos = 0;
                    nPos = sLine.find('$', nPos) + 1;
                    string __sName = sLine.substr(nPos, sLine.find('(', nPos)-nPos);
                    string __sVarList = "";
                    if (sLine[nPos] == '\'')
                    {
                        __sName = sLine.substr(nPos+1, sLine.find('\'', nPos+1)-nPos-1);
                        nParPos = sLine.find('(', nPos+1+__sName.length());
                    }
                    else
                        nParPos = sLine.find('(', nPos);
                    __sVarList = sLine.substr(nParPos);
                    nParPos += getMatchingParenthesis(sLine.substr(nParPos));
                    __sVarList = __sVarList.substr(1,getMatchingParenthesis(__sVarList)-1);

                    if (!isInQuotes(sLine, nPos, true))
                    {
                        //cerr << "__sName = " << __sName << endl;
                        Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script);
                        if (!_procedure.getReturnType())
                            sLine = sLine.substr(0, nPos-1) + sLine.substr(nParPos+1);
                        else
                        {
                            _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos-1, nParPos+1, "PROC~["+__sName+"~ROOT_"+toString(nProc)+"]");
                            nProc++;
                        }
                        /* if (_rTemp.sStringVal.length())
                            sLine = sLine.substr(0, nPos-1) + _rTemp.sStringVal + sLine.substr(nParPos+1);
                        else
                            sLine = sLine.substr(0,nPos-1) + toCmdString(_rTemp.dNumVal) + sLine.substr(nParPos+1);*/
                    }
                    nPos += __sName.length() + __sVarList.length()+1;

                    //sCurrentProcedureName = sProcNames.substr(sProcNames.rfind(';',sProcNames.rfind(';')-1)+1, sProcNames.rfind(';')-sProcNames.rfind(';',sProcNames.rfind(';')-1)-1);
                    //sProcNames = sProcNames.substr(0,sProcNames.rfind(';'));
                }
                StripSpaces(sLine);
                if (!sLine.length())
                    continue;
            }

            // --> Ist das letzte Zeichen ein ';'? Dann weise bSupressAnswer TRUE zu <--
            /*while (sLine.back() == ';')
            {
                sLine.pop_back();
                StripSpaces(sLine);
                bSupressAnswer = true;
            }*/
            // --> Gibt es "??"? Dann rufe die Prompt-Funktion auf <--
            if (!_procedure.getLoop() && sLine.find("??") != string::npos && sLine.substr(0,4) != "help")
                sLine = parser_Prompt(sLine);

            if (_procedure.isPluginCmd(sLine) && !_procedure.getLoop())
            {
                if (_procedure.evalPluginCmd(sLine))
                {
                    _option.setSystemPrintStatus(false);
                    Returnvalue _rTemp = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script);
                    if (_rTemp.vStringVal.size() && sLine.find("<<RETURNVAL>>") != string::npos)
                    {
                        string sReturn = "{";
                        for (unsigned int v = 0; v < _rTemp.vStringVal.size(); v++)
                            sReturn += _rTemp.vStringVal[v]+",";
                        sReturn.back() = '}';
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
                    }
                    else if (!_rTemp.vStringVal.size() && sLine.find("<<RETURNVAL>>") != string::npos)
                    {
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "~PLUGIN["+_procedure.getPluginProcName()+"~ROOT]");
                        vAns = _rTemp.vNumVal[0];
                        _parser.SetVectorVar("~PLUGIN["+_procedure.getPluginProcName()+"~ROOT]", _rTemp.vNumVal);
                    }
                    _option.setSystemPrintStatus(true);
                    if (!sLine.length())
                        continue;
                }
                else
                {
                    continue;
                }
            }
            //cerr << sLine << endl;
            if (findCommand(sLine, "explicit").sString == "explicit")
            {
                sLine.erase(findCommand(sLine, "explicit").nPos,8);
                StripSpaces(sLine);
            }
            //cerr << sLine << endl;
            /* --> Die Keyword-Suche soll nur funktionieren, wenn keine Schleife eingegeben wird, oder wenn eine
             *     eine Schleife eingegeben wird, dann nur in den wenigen Spezialfaellen, die zum Nachschlagen
             *     eines Keywords noetig sind ("list", "help", "find", etc.) <--
             */
            if (!_procedure.getLoop()
                || sLine.substr(0,4) == "help"
                || sLine.substr(0,3) == "man"
                || sLine.substr(0,4) == "quit"
                || sLine.substr(0,4) == "list"
                || sLine.substr(0,4) == "find"
                || sLine.substr(0,6) == "search")
            {
                switch (BI_CheckKeyword(sLine, _data, _out, _option, _parser, _functions, _pData, _script, true))
                {
                    case  0: break; // Kein Keyword: Mit dem Parser auswerten
                    case  1:        // Keyword: Naechster Schleifendurchlauf!
                        SetConsTitle(_data, _option);
                        continue;
                    case -1: return 0;  // Keyword "quit"
                    case  2: return 1;  // Keyword "mode"
                }
            }

            // --> Wenn die call()-Methode FALSE zurueckgibt, ist etwas schief gelaufen! <--
            if (!_functions.call(sLine, _option))
                throw FUNCTION_ERROR;

            // --> Prozeduren abarbeiten <--
            if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos && !_procedure.getLoop())
            {
                unsigned int nPos = 0;
                int nProc = 0;
                while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
                {
                    unsigned int nParPos = 0;
                    nPos = sLine.find('$', nPos) + 1;
                    string __sName = sLine.substr(nPos, sLine.find('(', nPos)-nPos);
                    nParPos = sLine.find('(', nPos);
                    nParPos += getMatchingParenthesis(sLine.substr(nParPos));
                    string __sVarList = sLine.substr(sLine.find('(',nPos));
                    __sVarList = __sVarList.substr(+1,getMatchingParenthesis(__sVarList)-1);

                    if (!isInQuotes(sLine, nPos))
                    {
                        Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script);
                        if (!_procedure.getReturnType())
                            sLine = sLine.substr(0, nPos-1) + sLine.substr(nParPos+1);
                        else
                        {
                            _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos-1, nParPos+1, "PROC~["+__sName+"~ROOT_"+toString(nProc)+"]");
                            nProc++;
                        }
                        /*if (_rTemp.sStringVal.length())
                            sLine = sLine.substr(0,nPos-1) + _rTemp.sStringVal + sLine.substr(nParPos+1);
                        else
                            sLine = sLine.substr(0,nPos-1) + toCmdString(_rTemp.dNumVal) + sLine.substr(nParPos+1);*/
                    }
                    nPos += __sName.length() + __sVarList.length()+1;

                }
                StripSpaces(sLine);
                if (!sLine.length())
                    continue;
            }
            else if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) == string::npos)
            {
                sLine = "";
                continue;
            }

            // --> Nochmals ueberzaehlige Leerzeichen entfernen <--
            StripSpaces(sLine);

            if (sLine.find("+=") != string::npos
                || sLine.find("-=") != string::npos
                || sLine.find("*=") != string::npos
                || sLine.find("/=") != string::npos
                || sLine.find("^=") != string::npos
                || sLine.find("++") != string::npos
                || sLine.find("--") != string::npos)
            {
                unsigned int nArgSepPos = 0;
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (isInQuotes(sLine, i, false))
                        continue;
                    if (sLine[i] == '(' || sLine[i] == '{')
                        i += getMatchingParenthesis(sLine.substr(i));
                    if (sLine[i] == ',')
                        nArgSepPos = i;
                    if (sLine.substr(i,2) == "+="
                        || sLine.substr(i,2) == "-="
                        || sLine.substr(i,2) == "*="
                        || sLine.substr(i,2) == "/="
                        || sLine.substr(i,2) == "^=")
                    {
                        if (sLine.find(',', i) != string::npos)
                        {
                            for (unsigned int j = i; j < sLine.length(); j++)
                            {
                                if (sLine[j] == '(')
                                    j += getMatchingParenthesis(sLine.substr(j));
                                if (sLine[j] == ',' || j+1 == sLine.length())
                                {
                                    if (!nArgSepPos && j+1 != sLine.length())
                                        sLine = sLine.substr(0, i)
                                            + " = "
                                            + sLine.substr(0, i)
                                            + sLine[i]
                                            + "("
                                            + sLine.substr(i+2, j-i-2)
                                            + ") "
                                            + sLine.substr(j);
                                    else if (nArgSepPos && j+1 != sLine.length())
                                        sLine = sLine.substr(0, i)
                                            + " = "
                                            + sLine.substr(nArgSepPos+1, i-nArgSepPos-1)
                                            + sLine[i]
                                            + "("
                                            + sLine.substr(i+2, j-i-2)
                                            + ") "
                                            + sLine.substr(j);
                                    else if (!nArgSepPos && j+1 == sLine.length())
                                        sLine = sLine.substr(0, i)
                                            + " = "
                                            + sLine.substr(0, i)
                                            + sLine[i]
                                            + "("
                                            + sLine.substr(i+2)
                                            + ") ";
                                    else
                                        sLine = sLine.substr(0, i)
                                            + " = "
                                            + sLine.substr(nArgSepPos+1, i-nArgSepPos-1)
                                            + sLine[i]
                                            + "("
                                            + sLine.substr(i+2)
                                            + ") ";

                                    for (unsigned int k = i; k < sLine.length(); k++)
                                    {
                                        if (sLine[k] == '(')
                                            k += getMatchingParenthesis(sLine.substr(k));
                                        if (sLine[k] == ',')
                                        {
                                            nArgSepPos = k;
                                            i = k;
                                            break;
                                        }
                                    }
                                    //cerr << sLine << " | nArgSepPos=" << nArgSepPos << endl;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (!nArgSepPos)
                                sLine = sLine.substr(0, i)
                                    + " = "
                                    + sLine.substr(0, i)
                                    + sLine[i]
                                    + "("
                                    + sLine.substr(i+2)
                                    + ")";
                            else
                                sLine = sLine.substr(0, i)
                                    + " = "
                                    + sLine.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sLine[i]
                                    + "("
                                    + sLine.substr(i+2)
                                    + ")";
                            break;
                        }
                    }
                    if (sLine.substr(i,2) == "++" || sLine.substr(i,2) == "--")
                    {
                        if (!nArgSepPos)
                        {
                            sLine = sLine.substr(0, i)
                                + " = "
                                + sLine.substr(0, i)
                                + sLine[i]
                                + "1"
                                + sLine.substr(i+2);
                        }
                        else
                            sLine = sLine.substr(0, i)
                                + " = "
                                + sLine.substr(nArgSepPos+1, i-nArgSepPos-1)
                                + sLine[i]
                                + "1"
                                + sLine.substr(i+2);
                    }
                }
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sLine = " << sLine << endl;
            }

            // --> Befinden wir uns in einem Loop? Dann ist nLoop > -1! <--
            if (_procedure.getLoop() || sLine.substr(0,3) == "for" || sLine.substr(0,2) == "if" || sLine.substr(0,5) == "while")
            {
                // --> Die Zeile in den Ausdrucksspeicher schreiben, damit sie spaeter wiederholt aufgerufen werden kann <--
                _procedure.setCommand(sLine, _parser, _data, _functions, _option, _out, _pData, _script);
                /* --> So lange wir im Loop sind und nicht endfor aufgerufen wurde, braucht die Zeile nicht an den Parser
                 *     weitergegeben werden. Wir ignorieren daher den Rest dieser for(;;)-Schleife <--
                 */
                continue;
            }

            // --> Gibt es "??" ggf. nochmal? Dann rufe die Prompt-Funktion auf <--
            if (sLine.find("??") != string::npos)
                sLine = parser_Prompt(sLine);

            // --> Gibt es "data(" oder "cache("? Dann rufe die GetDataElement-Methode auf <--
            if (!containsStrings(sLine)
                && !_data.containsStringVars(sLine)
                && (sLine.find("data(") != string::npos || _data.containsCacheElements(sLine)))
            {
                //cerr << "get data element (parser)" << endl;
                sCache = parser_GetDataElement(sLine, _parser, _data, _option);
                if (sCache.length() && sCache.find('#') == string::npos)
                    bWriteToCache = true;
            }
            // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
            /*if (sLine.find("{{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
            {
                parser_VectorToExpr(sLine, _option);
            }*/

            // --> Workaround fuer den x = x+1-Bug: In diesem Fall sollte die Eingabe x := x+1 lauten und wird hier weiterverarbeitet <--
            while (sLine.find(":=") != string::npos)
            {
                sLine.erase(sLine.find(":="),1);
            }

            // --> String-Syntax ("String" oder #VAR)? String-Parser aufrufen und mit dem naechsten Schleifendurchlauf fortfahren <--
            if (containsStrings(sLine) || _data.containsStringVars(sLine))
            {
                int nReturn = parser_StringParser(sLine, sCache, _data, _parser, _option);
                if (nReturn)
                {
                    if (nReturn == 1)
                        continue;
                    if (sCache.length() && _data.containsCacheElements(sCache) && !bWriteToCache)
                        bWriteToCache = true;
                }
                else
                {
                    throw STRING_ERROR;
                }
            }

            // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
            if (sLine.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
            {
                parser_VectorToExpr(sLine, _option);
            }


            // --> Wenn die Ergebnisse in den Cache geschrieben werden sollen, bestimme hier die entsprechenden Koordinaten <--
            if (bWriteToCache)
            {
                //cerr << sCache << endl;
                StripSpaces(sCache);
                while (sCache[0] == '(')
                    sCache.erase(0,1);
                si_pos[0] = sCache.substr(sCache.find('('));
                parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);

                //cerr << si_pos[0] << " " << sj_pos[0] << endl;

                if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
                    parser_GetDataElement(si_pos[0], _parser, _data, _option);
                if (sj_pos[0].find("data(") != string::npos || _data.containsCacheElements(sj_pos[0]))
                    parser_GetDataElement(sj_pos[0], _parser, _data, _option);

                bWriteToCache = true;

                //cerr << si_pos[0] << " " << sj_pos[0] << endl;

                if (si_pos[0].find(':') == string::npos && sj_pos[0].find(':') == string::npos)
                {
                    StripSpaces(si_pos[0]);
                    StripSpaces(sj_pos[0]);
                    if (!si_pos[0].length() || !sj_pos[0].length())
                    {
                        continue;
                    }
                    _parser.SetExpr(si_pos[0] + "," + sj_pos[0]);
                    _parser.Eval();
                    value_type* v = 0;
                    int nResults = _parser.GetNumResults();
                    v = _parser.Eval(nResults);
                    i_pos[0] = (int)v[0]-1;
                    if (i_pos[0] < 0)
                        i_pos[0] = 0;
                    i_pos[1] = i_pos[0];
                    j_pos[0] = (int)v[1]-1;
                    if (j_pos[0] < 0)
                        j_pos[0] = 0;
                    j_pos[1] = j_pos[0];
                }
                else
                {
                    if (si_pos[0].find(":") != string::npos)
                    {
                        si_pos[1] = si_pos[0].substr(si_pos[0].find(":")+1);
                        si_pos[0] = si_pos[0].substr(0, si_pos[0].find(":"));
                        bMultLinCol[0] = true;
                    }
                    if (sj_pos[0].find(":") != string::npos)
                    {
                        sj_pos[1] = sj_pos[0].substr(sj_pos[0].find(":")+1);
                        sj_pos[0] = sj_pos[0].substr(0, sj_pos[0].find(":"));
                        bMultLinCol[1] = true;
                    }
                    if (bMultLinCol[0] && bMultLinCol[1])
                    {
                        throw NO_MATRIX;
                    }
                    if (parser_ExprNotEmpty(si_pos[0]))
                    {
                        _parser.SetExpr(si_pos[0]);
                        i_pos[0] = (int)_parser.Eval();
                        i_pos[0]--;
                    }
                    else
                        i_pos[0] = 0;

                    if (i_pos[0] < 0)
                        i_pos[0] = 0;

                    if (parser_ExprNotEmpty(sj_pos[0]))
                    {
                        _parser.SetExpr(sj_pos[0]);
                        j_pos[0] = (int)_parser.Eval();
                        j_pos[0]--;
                    }
                    else
                        j_pos[0] = 0;

                    if (j_pos[0] < 0)
                        j_pos[0] = 0;

                    if (parser_ExprNotEmpty(si_pos[1]) && bMultLinCol[0])
                    {
                        _parser.SetExpr(si_pos[1]);
                        i_pos[1] = (int)_parser.Eval();
                        i_pos[1]--;
                        parser_CheckIndices(i_pos[0], i_pos[1]);
                    }
                    else if (bMultLinCol[0])
                        si_pos[1] = "inf";
                    else
                        i_pos[1] = i_pos[0];

                    if (parser_ExprNotEmpty(sj_pos[1]) && bMultLinCol[1])
                    {
                        _parser.SetExpr(sj_pos[1]);
                        j_pos[1] = (int)_parser.Eval();
                        j_pos[1]--;
                        parser_CheckIndices(j_pos[0], j_pos[1]);
                    }
                    else if (bMultLinCol[1])
                        sj_pos[1] = "inf";
                    else
                        j_pos[1] = j_pos[0];
                }
            }

            // --> Ausdruck an den Parser uebergeben und einmal auswerten <--
            if (sLine + " " != _parser.GetExpr())
                _parser.SetExpr(sLine);


            // --> Jetzt weiss der Parser, wie viele Ergebnisse er berechnen muss <--
            v = _parser.Eval(nNum);
            if (nNum > 1)
            {
                //value_type *v = _parser.Eval(nNum);
                vAns = v[0];
                if (!bSupressAnswer)
                {
                    //cerr << std::setprecision(_option.getPrecision());
                    int nLineBreak = parser_LineBreak(_option);
                    cerr << "|-> ans = {";
                    for (int i = 0; i < nNum; ++i)
                    {
                        cerr << std::setfill(' ') << std::setw(_option.getPrecision()+7) << std::setprecision(_option.getPrecision()) << v[i];
                        if (i < nNum-1)
                            cerr << ", ";
                        if (nNum + 1 > nLineBreak && !((i+1) % nLineBreak) && i < nNum-1)
                            cerr << "...\n|          ";
                    }
                    cerr << "}" << endl;
                }
                if (bWriteToCache)
                {
                    if (bMultLinCol[0] || bMultLinCol[1])
                    {
                        if (si_pos[1] == "inf")
                            i_pos[1] = i_pos[0] + nNum;
                        if (sj_pos[1] == "inf")
                            j_pos[1] = j_pos[1] + nNum;
                        for (int i = i_pos[0]; i <= i_pos[1]; i++)
                        {
                            for (int j = j_pos[0]; j <= j_pos[1]; j++)
                            {
                                if ((i - i_pos[0] == nNum && i_pos[0] != i_pos[1]) || (j - j_pos[0] == nNum && j_pos[0] != j_pos[1]))
                                    break;
                                if (i_pos[0] != i_pos[1])
                                {
                                    if (!_data.writeToCache(i,j,sCache.substr(0,sCache.find('(')), (double)v[i-i_pos[0]]))
                                        break;
                                }
                                else if (!_data.writeToCache(i,j,sCache.substr(0,sCache.find('(')), (double)v[j-j_pos[0]]))
                                    break;
                            }
                        }
                    }
                    else
                    {
                        mu::console() << _T("|-> Ergebnisse wurden in Spalte ") << j_pos[0]+1 << _T(" ab Element ") << i_pos[0]+1 << _T(" geschrieben!\n");
                        for (int i = i_pos[0]; i < i_pos[0] + nNum; i++)
                        {
                            if (!_data.writeToCache(i, j_pos[0], sCache.substr(0,sCache.find('(')), (double)v[i-i_pos[0]]))
                                break;
                        }
                    }
                }
            }
            else
            {
                //vAns = _parser.Eval();
                vAns = v[0];
                /*if (isinf(vAns))
                {
                    cerr << "INF catch!" << endl;
                }*/
                if (bWriteToCache)
                {
                    if (_option.getbDebug())
                        mu::console() << _T("|-> DEBUG: i_pos = ") << i_pos[0] <<  _T(", j_pos = ") << j_pos[0] << endl;
                    _data.writeToCache(i_pos[0], j_pos[0], sCache.substr(0,sCache.find('(')), (double)vAns);
                }
                if (!bSupressAnswer)
                    cerr << std::setprecision(_option.getPrecision()) << "|-> ans = " << vAns << endl;
            }
        }
        catch (mu::Parser::exception_type &e)
        {
            _option.setSystemPrintStatus(true);
            // --> Vernuenftig formatierte Fehlermeldungen <--
            unsigned int nErrorPos = (int)e.GetPos();
            make_hline();
            if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
                cerr << "|-> NUMERE: DEBUGGER   [FEHLER IM AUSDRUCK]" << endl;
            else
                cerr << "|-> FEHLER IM AUSDRUCK" << endl;
            make_hline();

            // --> Eigentliche Fehlermeldung <--
            cerr << LineBreak("|-> " + e.GetMsg(), _option) << endl;
            cerr << "|   Ausdruck:  " << LineBreak("\"" + e.GetExpr() + "\"", _option, true, 15, 15) << endl;

            /* --> Ausdruecke, die laenger als 63 Zeichen sind, passen nicht in die Zeile. Wir stellen nur die ersten
             *     60 Zeichen gefolgt von "..." dar <--
             */
            // --> Fehlerhaftes/Unerwartetes Objekt <--
            if (e.GetToken().length())
                cerr << "|   Objekt:    \"" << e.GetToken()    << "\"" << endl;

            /* --> Position des Fehlers im Ausdruck: Wir stellen um den Fehler nur einen Ausschnitt
             *     des Ausdrucks in der Laenge von etwa 63 Zeichen dar und markieren die Fehlerposition
             *     durch ein darunter angezeigten Zirkumflex "^" <--
             */
            if (e.GetExpr().length() > 63 && nErrorPos > 31 && nErrorPos < e.GetExpr().length()-32)
            {
                cerr << "|  " << (char)218 << "Position:" << (char)191 << " \"..." << e.GetExpr().substr(nErrorPos-29,57) << "...\"\n";
                cerr << "|  " << (char)192 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)193 << (char)196 << (char)196 << std::setfill((char)196) << std::setw(32) << (char)217 << endl;
            }
            else if (nErrorPos < 32)
            {
                cerr << "|  " << (char)218 << "Position:" << (char)191 << " \"";
                if (e.GetExpr().length() > 63)
                    cerr << e.GetExpr().substr(0,60) << "...\"" << endl;
                else
                    cerr << e.GetExpr() << "\"" << endl;
                cerr << "|  " << (char)192 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)193 << (char)196 << (char)196 << std::setfill((char)196) << std::setw(nErrorPos+1) << (char)217 << endl;
            }
            else if (nErrorPos > e.GetExpr().length()-32)
            {
                cerr << "|  " << (char)218 << "Position:" << (char)191 << " \"";
                if (e.GetExpr().length() > 63)
                {
                    cerr << _T("...") << e.GetExpr().substr(e.GetExpr().length()-60) << "\"" << endl;
                    cerr << "|  " << (char)192 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)193 << (char)196 << (char)196 << std::setfill((char)196) << std::setw(65-(e.GetExpr().length()-nErrorPos)-2) << (char)217 << endl;
                }
                else
                {
                    mu::console() << e.GetExpr() << _T("\"\n");
                    cerr << "|  " << (char)192 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)193 << (char)196 << (char)196 << std::setfill((char)196) << std::setw(nErrorPos) << (char)217 << endl;
                }
            }

            // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
            if (_script.isValid() && _script.isOpen())
            {
                cerr << LineBreak("|-> Der Fehler wurde im ausgeführten Script nahe der " + toString((int)_script.getCurrentLine()) + ". Zeile gefunden.$Die Ausführung des Scripts wurde sicherheitshalber abgebrochen.", _option) << endl;
                // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                _script.close();
            }
            if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
            {
                /*make_hline();
                cerr << "|-> NUMERE: DEBUGGER" << endl;
                make_hline();*/
                cerr << "|" << endl << "|   " << toUpperCase("Modulinformationen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-24) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printModuleInformations(), _option, false) << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Stacktrace: ") << std::setfill((char)196) << std::setw(_option.getWindow()-16) << (char)196 << endl;
                /*make_hline(-2);
                cerr << "|-> Stacktrace:" << endl;*/
                cerr << LineBreak("|   "+_option._debug.printStackTrace(), _option, false) << endl;
                //make_hline(-2);
                //cerr << "|-> Lokale numerische Variablen:" << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Lokale numerische Variablen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-33) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printLocalVars(), _option, false) << endl;
                //make_hline(-2);
                //cerr << "|-> Lokale Zeichenketten:" << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Lokale Zeichenketten: ") << std::setfill((char)196) << std::setw(_option.getWindow()-26) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printLocalStrings(), _option, false) << endl;
                _option._debug.reset();
            }
            make_hline();

            // --> Alle Variablen zuerst zuruecksetzen! <--
            _procedure.reset(_parser);
            _pData.setFileName("");
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: " << e.GetMsg() << endl;
            if (sCmdCache.length())
                sCmdCache.clear();

        }
        catch (const std::bad_alloc &e)
        {
            _option.setSystemPrintStatus(true);
            /* --> Das ist die schlimmste aller Exceptions: Fehler bei der Speicherallozierung.
             *     Leider gibt es bis dato keine Moeglichkeit, diesen wieder zu beheben, also bleibt
             *     vorerst nichts anderes uebrig, als NumeRe mit terminate() abzuschiessen <--
             */
            cerr << endl;
            make_hline();
            cerr << "|-> EIN KRITISCHER FEHLER IST AUFGETRETEN" << endl;
            make_hline();
            cerr << LineBreak("|-> NumeRe " + sVersion + " verursachte eine kritische Speicherzugriffsverletzung und muss beendet werden.", _option) << endl;
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: KRITISCHE SPEICHERVERLETZUNG" << endl;
            for (int i = 4; i > 0; i--)
            {
                cerr << "\r|-> TERMINATING IN " << i << " sec ...";
                Sleep(1000);
            }
            if (sCmdCache.length())
                sCmdCache.clear();
            throw;
        }
        catch (const std::exception &e)
        {
            _option.setSystemPrintStatus(true);
            // --> Alle anderen Standard-Exceptions <--
            make_hline();
            if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
                    cerr << "|-> NUMERE: DEBUGGER   [INTERNER FEHLER]" << endl;
            else
                cerr << "|-> EIN INTERNER FEHLER IST AUFGETRETEN" << endl;
            make_hline();
            cerr << LineBreak("|-> " + string(e.what()), _option) << endl;
            cerr << LineBreak("|-> Dies ist ein nicht-kritischer Fehler. Sollte sich dieser Fehler (auch nach einem Neustart) wiederholen (lassen), freut der Entwickler sich über eine entsprechende Bugmeldung unter <numere.developer@gmail.com>.", _option) << endl;

            // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
            if (_script.isValid() && _script.isOpen())
            {
                cerr << LineBreak("|-> Der Fehler wurde im ausgeführten Script nahe der " + toString((int)_script.getCurrentLine()) + ". Zeile gefunden.$Die Ausführung des Scripts wurde sicherheitshalber abgebrochen.", _option) << endl;
                // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                _script.close();
            }
            if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
            {
                /*make_hline();
                cerr << "|-> NUMERE: DEBUGGER" << endl;
                make_hline();*/
                cerr << "|" << endl << "|   " << toUpperCase("Modulinformationen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-24) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printModuleInformations(), _option, false) << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Stacktrace: ") << std::setfill((char)196) << std::setw(_option.getWindow()-16) << (char)196 << endl;
                /*make_hline(-2);
                cerr << "|-> Stacktrace:" << endl;*/
                cerr << LineBreak("|   "+_option._debug.printStackTrace(), _option, false) << endl;
                //make_hline(-2);
                //cerr << "|-> Lokale numerische Variablen:" << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Lokale numerische Variablen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-33) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printLocalVars(), _option, false) << endl;
                //make_hline(-2);
                //cerr << "|-> Lokale Zeichenketten:" << endl;
                cerr << "|" << endl << "|   " << toUpperCase("Lokale Zeichenketten: ") << std::setfill((char)196) << std::setw(_option.getWindow()-26) << (char)196 << endl;
                cerr << LineBreak("|   "+_option._debug.printLocalStrings(), _option, false) << endl;
                _option._debug.reset();
            }
            _pData.setFileName("");
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: " << e.what() << endl;
            if (sCmdCache.length())
                sCmdCache.clear();
        }
        catch (errorcode& e)
        {
            _option.setSystemPrintStatus(true);
            make_hline();
            if (e == PROCESS_ABORTED_BY_USER)
            {
                cerr << "|-> PROZESS ABGEBROCHEN" << endl;
                make_hline();
                cerr << LineBreak("|-> Ein Prozess wurde vom Benutzer abgebrochen.$(Während eines Prozesses wurde \"ESC\" gedrückt. Dies bricht die Auswertung ab.)", _option, false) << endl;
                //cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;
                if (oLogFile.is_open())
                    oLogFile << toString(time(0) - tTimeZero, true) << "> HINWEIS: Prozess vom Benutzer abgebrochen" << endl;
                // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
                if (_script.isValid() && _script.isOpen())
                {
                    cerr << LineBreak("|-> Das ausgeführte Script wurde nahe der " + toString((int)_script.getCurrentLine()) + ". Zeile abgebrochen.", _option) << endl;
                    // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                    _script.close();
                }
            }
            else
            {
                if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
                    cerr << "|-> NUMERE: DEBUGGER   [SYNTAXFEHLER]" << endl;
                else
                    cerr << "|-> EIN FEHLER IST AUFGETRETEN" << endl;
                make_hline();
                switch (e)
                {
                    case STRING_ERROR:
                        cerr << LineBreak("|-> Die Zeichenketten konnten nicht korrekt verarbeitet werden.$Womöglich sind eine oder mehrere Zeichenketten nicht zu beiden Seiten mit Anführungszeichen umgeben oder es wurde versucht, einer Zeichenkettenvariable einen numerischen Wert oder einer numerischen Variable eine Zeichenkette zu zu weisen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help string\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zeichenkettenfehler" << endl;
                        break;
                    case PROCEDURE_THROW:
                        if (!sErrorToken.length())
                        {
                            cerr << LineBreak("|-> Eine NumeRe-Prozedur forderte NumeRe auf, den Prozedurablauf unplanmäßig zu beenden.$(Eine Instanz von \"throw\" wurde während der Auswertung gefunden)", _option) << endl;
                            cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                            if (oLogFile.is_open())
                                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Prozedurfehler" << endl;
                        }
                        else
                        {
                            cerr << LineBreak("|-> " + sErrorToken, _option) << endl;
                            if (oLogFile.is_open())
                                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: " << sErrorToken << endl;
                        }
                        break;
                    case LOOP_THROW:
                        if (!sErrorToken.length())
                        {
                            cerr << LineBreak("|-> Eine Schleife oder eine bedingte Verzweigung forderte NumeRe auf, die Auswertung unplanmäßig zu beenden.$(Eine Instanz von \"throw\" wurde während der Auswertung gefunden)", _option) << endl;
                            cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\" und \"help for\"", _option) << endl;
                            if (oLogFile.is_open())
                                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Schleifenfehler" << endl;
                        }
                        else
                        {
                            cerr << LineBreak("|-> " + sErrorToken, _option) << endl;
                            if (oLogFile.is_open())
                                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: " << sErrorToken << endl;
                        }
                        break;
                    case TOO_MANY_ARGS:
                        cerr << LineBreak("|-> Es wurden eine oder mehrere Variablen an eine NumeRe-Prozedur, die keine eigene Variablenliste besitzt, zugewiesen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu viele Argumente" << endl;
                        break;
                    case MISSING_DEFAULT_VALUE:
                        cerr << LineBreak("|-> Es wurde ein leeres Argument an eine NumeRe-Prozedur übergeben, für welches kein Standardwert in der Prozedur deklariert wurde.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlende Argumente" << endl;
                        break;
                    case WRONG_ARG_NAME:
                        cerr << LineBreak("|-> \""+sErrorToken+"\" kann in einer NumeRe-Prozedur nicht als Variablenname verwendet werden.$(\""+sErrorToken+"\" ist ein prozedurspezifisches Kommando)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: \""+sErrorToken+"\" als Variablennamen verwendet" << endl;
                        break;
                    case INVALID_PROCEDURE_NAME:
                        cerr << LineBreak("|-> Ein ungültiger oder leerer Prozedurname wurde gefunden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ungültiger oder leerer Prozedurname" << endl;
                        break;
                    case FILE_NOT_EXIST:
                        cerr << LineBreak("|-> Eine für die gewünschte Aktion erforderliche Datei konnte nicht gefunden oder geöffnet werden. Möglicherweise wurde nur der Pfad zur Datei vergessen oder die Prozedur befindet sich in einem anderen Namensraum. Dateinamen mit Leerzeichen müssen des Weiteren von Anführungzeichen umschlossen sein.$(Prozeduren und Funktionsdefinitionen werden in Dateien abgelegt, die zur korrekten Ausführung nötig sind)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\", \"help define\" und \"help editor\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei existiert nicht" << endl;
                        break;
                    case CANNOT_OPEN_TARGET:
                        cerr << LineBreak("|-> NumeRe konnte die gewünschte Zieldatei nicht öffnen.$(Um Dateien zu verschieben, ist eine gültige Zieldatei vonnöten. Dieser Fehler kann aber auch beim Plotten mit der Option \"otex=FILE\" auftreten.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help move\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Kann Zieldatei nicht öffnen" << endl;
                        break;
                    case CANNOT_OPEN_SOURCE:
                        cerr << LineBreak("|-> NumeRe konnte die zu verschiebende Datei nicht finden oder öffnen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help move\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Kann Quelldatei nicht öffnen" << endl;
                        break;
                    case PROCEDURE_NOT_FOUND:
                        cerr << LineBreak("|-> Die NumeRe-Prozedur konnte nicht in der NumeRe-Prozedurdatei gefunden werden.$(Es wurde eine NumeRe-Prozedurdatei mit passendem Namen gefunden, jedoch enthält diese nicht die gesuchte Prozedurdefinition)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Prozedur konnte nicht gefunden werden" << endl;
                        break;
                    case UNMATCHED_PARENTHESIS:
                        cerr << LineBreak("|-> Klammern im gegebenen Ausdruck passen nicht zusammen.$(Es wurden mehr öffnende als schließende oder mehr schließende als öffnende Klammern gefunden)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Klammerfehler" << endl;
                        break;
                    case PROCEDURE_ERROR:
                        cerr << LineBreak("|-> Eine Prozedur konnte nicht korrekt ausgeführt werden.$(Möglicherweise wurden Teile der Prozedursyntax \"\\$PROZEDURNAME(ARGUMENTE)\" falsch oder unerwartet verwendet)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Eine Prozedur konnte nicht korrekt ausgeführt werden" << endl;
                        break;
                    case INVALID_DATA_ACCESS:
                        cerr << LineBreak("|-> Der Zugriff auf ein Datenobjekt ist fehlgeschlagen.$(Die Datenelemente, die im letzten Ausdruck verwendet wurden, sind möglicherweise nicht verfügbar. Vielleicht enthält die Datentabelle leere Spalten.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datenzugriffsfehler" << endl;
                        break;
                    case NO_DATA_AVAILABLE:
                        cerr << LineBreak("|-> Es wurden keine Daten in den Speicher geladen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Daten im Speicher" << endl;
                        break;
                    case READ_ONLY_DATA:
                        cerr << LineBreak("|-> Die geladenen Daten eines Datenfiles können nicht überschrieben werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Daten sind Schreibgeschützt" << endl;
                        break;
                    case NO_CACHED_DATA:
                        cerr << LineBreak("|-> Es sind keine Daten im gewünschten Cache verfügbar.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Daten im Cache" << endl;
                        break;
                    case NO_MATRIX:
                        cerr << LineBreak("|-> Es können entweder nur Zeilen oder nur Spalten aus einem Datenobjekt extrahiert oder hinein geschrieben werden.$(Möglicherweise wurde versucht, eine Untertabelle aus einem Datenobjekt zu lesen oder hinein zu schreiben)", _option) << endl;
                        cerr << LineBreak("|-> Verwende das \"matop\"-Kommando, um dieses Problem zu lösen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\", \"help cache\" und \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Untertabellen können nicht extrahiert werden" << endl;
                        break;
                    case INVALID_INDEX:
                        cerr << LineBreak("|-> Einer oder mehrere Indices besitzen keinen gültigen Wert, konnten nicht gelesen werden oder befinden sich außerhalb eines Datensatzes.$(\"inf\", \"nan\" oder \"void\" sind nicht als Indexwerte zugelassen, da sie keiner natürlichen Zahl entsprechen. Indices kleiner-gleich 0 sind ebenfalls nicht gültig.)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ungültiger Index" << endl;
                        break;
                    case INVALID_ELEMENT:
                        cerr << LineBreak("|-> Das Element '" + toString(nErrorIndices[0]) + "," + toString(nErrorIndices[1]) + "' konnte in dem gewünschten Datenobjekt nicht gefunden werden.$(Entweder ist der Datensatz kleiner oder das gewünschte Element hat keinen gültigen Wert)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datenelement konnte nicht gefunden werden" << endl;
                        break;
                    case CANNOT_CONTAIN_STRINGS:
                        cerr << LineBreak("|-> Ein Datenobjekt kann als Datensatz keine Zeichenketten beinhalten.$(Datenobjekte fassen nur numerische Werte. Die Tabellenüberschriften können mit \"DATENOBJEKT(#,SPALTE)\" bearbeitet werden. Für komplexere Zeichenketten existiert das \"string()\"-Objekt)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help string\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datenobjekte können keine Zeichenketten enthalten" << endl;
                        break;
                    case FUNCTION_ERROR:
                        cerr << LineBreak("|-> Eine Funktionsdefinition konnte nicht korrekt umgesetzt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktionsdefinitionsfehler" << endl;
                        break;
                    case SEPARATOR_NOT_FOUND:
                        cerr << LineBreak("|-> Das erforderliche Trennzeichen (\",\" oder \":\") konnte nicht gefunden werden.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Trennzeichen konnten nicht gefunden werden" << endl;
                        break;
                    case NO_DIFF_VAR:
                        cerr << LineBreak("|-> Es wurde keine Variable, bezüglich der differenziert werden soll, spezifiziert.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help diff\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Variable zur Differentiation" << endl;
                        break;
                    case NO_DIFF_OPTIONS:
                        cerr << LineBreak("|-> Es wurden keine Optionen übergeben.$(\"diff\" benötigt mindestens die Angabe einer Variable und eines Werts derselbigen)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help diff\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Differentiationsoptionen" << endl;
                        break;
                    case DIFF_VAR_NOT_FOUND:
                        cerr << LineBreak("|-> Die Variable, bezüglich welcher differenziert werden soll, konnte nicht gefunden werden.$(Womöglich wurde sie noch nicht initialisiert)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help diff\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Variable zur Differentiation konnte nicht gefunden werden" << endl;
                        break;
                    case NO_EXTREMA_VAR:
                        cerr << LineBreak("|-> Es wurde keine Variable, bezüglich der extremiert werden soll, angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help extrema\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Variable zur Extremierung" << endl;
                        break;
                    case NO_EXTREMA_OPTIONS:
                        cerr << LineBreak("|-> Es wurden keine Optionen übergeben.$(\"extrema\" benötigt mindestens die Angabe einer Variable und eines Intervalls)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help extrema\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Extremierungsoptionen" << endl;
                        break;
                    case EXTREMA_VAR_NOT_FOUND:
                        cerr << LineBreak("|-> Die Variable, bezüglich welcher extremiert werden soll, konnte nicht gefunden werden.$(Womöglich wurde sie noch nicht initialisiert)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help extrema\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Variable zur Extremierung konnte nicht gefunden werden" << endl;
                        break;
                    case NO_ZEROES_VAR:
                        cerr << LineBreak("|-> Es wurde keine Variable, bezüglich welcher die Nullstellen gesucht werden sollen, angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help zeroes\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Variable zur Nullstellensuche" << endl;
                        break;
                    case NO_ZEROES_OPTIONS:
                        cerr << LineBreak("|-> Es wurden keine Optionen übergeben.$(\"zeroes\" benötigt mindestens die Angabe einer Variable und eines Intervalls)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help zeroes\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Nullstellenoptionen" << endl;
                        break;
                    case ZEROES_VAR_NOT_FOUND:
                        cerr << LineBreak("|-> Die Variable, bezüglich welcher die Nullstellen gesucht werden sollen, konnte nicht gefunden werden.$(Womöglich wurde sie noch nicht initialisiert)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help zeroes\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Variable zur Nullstellensuche wurde nicht gefunden" << endl;
                        break;
                    case NO_EVAL_VAR:
                        cerr << LineBreak("|-> Es wurde keine Variable, bezüglich welcher Punkte berechnet werden sollen, angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help eval\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Punkte-Auswerte-Variable" << endl;
                        break;
                    case NO_EVAL_OPTIONS:
                        cerr << LineBreak("|-> Es wurden keine Optionen übergeben.$(\"eval\" benötigt mindestens die Angabe einer Variable und eines Intervalls)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help eval\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Punkte-Auswerte-Optionen" << endl;
                        break;
                    case EVAL_VAR_NOT_FOUND:
                        cerr << LineBreak("|-> Die Variable, bezüglich welcher Punkte berechnet werden sollen, konnte nicht gefunden werden.$(Womöglich wurde sie noch nicht initialisiert)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help eval\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Variable zum Punkte-Auswerten konnte nicht gefunden werden" << endl;
                        break;
                    case CANNOT_PLOT_STRINGS:
                        cerr << LineBreak("|-> Zeichenketten können nicht geplottet werden.$(Zeichenketten besitzen keine sinnvollen, numerischen Werte)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zeichenketten können nicht geplottet werden" << endl;
                        break;
                    case WRONG_PLOT_INTERVAL_FOR_LOGSCALE:
                        cerr << LineBreak("|-> Wenn eine Achse oder ein Intervall logarithmisch dargestellt werden soll, so können deren Werte nicht im negativen Bereich liegen.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Falsches Plotintervall für Logscale" << endl;
                        break;
                    case TOO_FEW_COLS:
                        cerr << LineBreak("|-> Ein Datenobjekt besitzt zu wenig Spalten für die gewünschte Aktion oder es wurden zu wenig angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plotoptions\" oder \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu wenig Spalten für die gewünschte Aktion" << endl;
                        break;
                    case TOO_FEW_LINES:
                        cerr << LineBreak("|-> Ein Datenobjekt besitzt zu wenig Zeilen für die gewünschte Aktion oder es wurden zu wenig angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fft\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu wenig Zeilen für die gewünschte Aktion" << endl;
                        break;
                    case CANNOT_FIND_DEFINE_OPRT:
                        cerr << LineBreak("|-> Der Definitionsoperator \":=\" konnte in der Funktionsdefinition nicht gefunden werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Definitionsoperator konnte nicht gefunden werden" << endl;
                        break;
                    case FUNCTION_ALREADY_EXISTS:
                        cerr << LineBreak("|-> Unter diesem Namen existiert bereits eine Funktion.$(Um eine Funktionsdefinition zu überschreiben, muss das Kommando \"redefine\" verwendet werden)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktion existiert bereits" << endl;
                        break;
                    case FUNCTION_IS_PREDEFINED:
                        cerr << LineBreak("|-> Die Funktion \"" + sErrorToken + "\" ist eine von NumeRe vordefinierte Funktion und kann nicht umdefiniert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\" und \"help new\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktion ist vordefiniert" << endl;
                        break;
                    case FUNCTION_STRING_IS_COMMAND:
                        cerr << LineBreak("|-> Der Ausdruck \"" + sErrorToken + "\" ist ein geschütztes NumeRe-Kommando und kann nicht als Funktionsname verwendet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktion ist NumeRe-Kommando" << endl;
                        break;
                    case CANNOT_FIND_FUNCTION_ARGS:
                        cerr << LineBreak("|-> Die erforderlichen Funktionsargumente konnten nicht gefunden werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktionsargumente konnten nicht gefunden werden" << endl;
                        break;
                    case NO_NUMBER_AT_POS_1:
                        cerr << LineBreak("|-> Funktionsnamen dürfen nicht mit einer Ziffer beginnen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktionsargumente dürfen nicht mit einer Ziffer beginnen" << endl;
                        break;
                    case FUNCTION_NAMES_MUST_NOT_CONTAIN_SIGN:
                        cerr << LineBreak("|-> Funktionsnamen dürfen das Zeichen \"" + sErrorToken + "\" nicht enthalten.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktionsnamen dürfen \"" << sErrorToken << "\" nicht enthalten" << endl;
                        break;
                    case TOO_MANY_ARGS_FOR_DEFINE:
                        cerr << LineBreak("|-> Selbst definierte Funktionen können nicht mehr als 10 Argumente verarbeiten.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu viele Argumente zur Definition" << endl;
                        break;
                    case INCOMPLETE_VECTOR_SYNTAX:
                        cerr << LineBreak("|-> Die NumeRe-Spaltensyntax wurde nicht korrekt verwendet.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help multiresult\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Spaltensyntax falsch verwendet" << endl;
                        break;
                    case ELLIPSIS_MUST_BE_LAST_ARG:
                        cerr << LineBreak("|-> Der Platzhalter \"...\" darf nur das letzte Argument einer Funktionsdefinition sein.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu viele Elemente der Spaltensyntax" << endl;
                        break;
                    case TOO_MANY_VECTORS:
                        cerr << LineBreak("|-> Ein Ausdruck enthält mehr als 32 Elemente der NumeRe-Spaltensyntax.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help multiresult\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu viele Elemente der Spaltensyntax" << endl;
                        break;
                    case TOO_MANY_FUNCTION_CALLS:
                        cerr << LineBreak("|-> Die aufgerufenen Funktionen sind zu tief verschachtelt oder haben zu einer Endlosschleife geführt.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help define\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zu tief verschachtelte Funktionsdefinition" << endl;
                        break;
                    case CANNOT_SAVE_CACHE:
                        cerr << LineBreak("|-> Es konnte keine automatische Sicherung des Caches angelegt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Automatische Sicherung konnte nicht angelegt werden" << endl;
                        break;
                    case CANNOT_SAVE_FILE:
                        cerr << LineBreak("|-> Die Daten konnten nicht in eine Datei gespeichert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help save\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Speicherfehler" << endl;
                        break;
                    case SCRIPT_NOT_EXIST:
                        cerr << LineBreak("|-> Das Script \"" + sErrorToken + "\" wurde nicht gefunden oder kann nicht gelesen werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help script\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Script existiert nicht" << endl;
                        break;
                    case CANNOT_SORT_DATA:
                        cerr << LineBreak("|-> Die Daten aus \"data()\" konnten nicht sortiert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte data() nicht sortieren" << endl;
                        break;
                    case CANNOT_SORT_CACHE:
                        cerr << LineBreak("|-> Die Daten aus \"cache()\" konnten nicht sortiert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte cache() nicht sortieren" << endl;
                        break;
                    case CANNOT_SMOOTH_CACHE:
                        cerr << LineBreak("|-> Die Daten konnten nicht geglättet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help smooth\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte cache() nicht glätten" << endl;
                        break;
                    case CANNOT_RETOQUE_CACHE:
                        cerr << LineBreak("|-> Die Daten konnten nicht retuschiert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help retoque\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte cache() nicht retuschieren" << endl;
                        break;
                    case CANNOT_RESAMPLE_CACHE:
                        cerr << LineBreak("|-> Resampling konnte nicht erfolgreich abgeschlossen werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help resample\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte nicht erfolgreich resamplen" << endl;
                        break;
                    case CANNOT_MOVE_DATA:
                        cerr << LineBreak("|-> Der gewünschte Datensatz konnte nicht verschoben werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help move\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Konnte den Datensatz nicht verschieben" << endl;
                        break;
                    case CANNOT_MOVE_FILE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" konnte nicht verschoben werden oder existiert nicht.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help move\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei konnte nicht verschoben werden" << endl;
                        break;
                    case CANNOT_REMOVE_FILE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" konnte nicht gelöscht werden oder existiert nicht.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help remove\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei konnte nicht gelöscht werden" << endl;
                        break;
                    case CANNOT_DELETE_ELEMENTS:
                        cerr << LineBreak("|-> Die Elemente konnten nicht gelöscht werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Daten konnten nicht gelöscht werden" << endl;
                        break;
                    case CANNOT_COPY_DATA:
                        cerr << LineBreak("|-> Der Datensatz konnte nicht kopiert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help copy\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Daten konnten nicht kopiert werden" << endl;
                        break;
                    case CANNOT_COPY_FILE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" konnte nicht kopiert werden oder existiert nicht.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help copy\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei konnte nicht kopiert werden" << endl;
                        break;
                    case CANNOT_EXPORT_DATA:
                        cerr << LineBreak("|-> Der Datensatz konnte nicht exportiert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help export\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Daten konnten nicht exportiert werden" << endl;
                        break;
                    case NO_FILENAME:
                        cerr << LineBreak("|-> Ein Kommando erwartete einen Dateinamen, es wurde jedoch keiner angegeben.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlender Dateiname" << endl;
                        break;
                    case DATAFILE_NOT_EXIST:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" existiert nicht oder kann nicht geöffnet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\" und \"help load\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei existiert nicht" << endl;
                        break;
                    case CANNOT_READ_FILE:
                        cerr << LineBreak("|-> Aus der Datei \"" + sErrorToken + "\" kann nicht gelesen werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\", \"help load\" und \"help edit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei kann nicht gelesen werden" << endl;
                        break;
                    case FUNCTION_CANNOT_BE_FITTED:
                        cerr << LineBreak("|-> Die Funktion \"" + sErrorToken + "\" kann nicht an Daten angepasst werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Funktion kann nicht zum Fitten verwendet werden" << endl;
                        break;
                    case NO_FUNCTION_FOR_FIT:
                        cerr << LineBreak("|-> Es wurde keine Funktion zum Anpassen angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Funktion zum Fitten" << endl;
                        break;
                    case NO_PARAMS_FOR_FIT:
                        cerr << LineBreak("|-> Es wurden keine Parameter zum Anpassen angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Parameter zum Fitten" << endl;
                        break;
                   case NO_DATA_FOR_FIT:
                        cerr << LineBreak("|-> Es wurden keine Daten zum Anpassen angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Keine Daten zum Anpassen" << endl;
                        break;
                    case FITFUNC_NOT_CONTAINS:
                        cerr << LineBreak("|-> In der gegebenen Funktion konnte \"" + sErrorToken + "\" nicht gefunden werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\" oder \"help datagrid\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: \"" << sErrorToken << "\" fehlt in der anzupassenden Funktion" << endl;
                        break;
                    case CANNOT_BE_A_FITTING_PARAM:
                        cerr << LineBreak("|-> Die Variable \"" + sErrorToken + "\" kann kein Fit-Parameter sein.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: \"" << sErrorToken << "\" kann kein Fit-Parameter sein" << endl;
                        break;
                    case CANNOT_GENERATE_DIRECTORY:
                        cerr << LineBreak("|-> Der Ordner \"" + sErrorToken + "\" konnte nicht erzeugt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help new\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ordner konnte nicht erzeugt werden" << endl;
                        break;
                    case CANNOT_GENERATE_SCRIPT:
                        cerr << LineBreak("|-> Das Script \"" + sErrorToken + "\" konnte nicht erzeugt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help new\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Script konnte nicht erzeugt werden" << endl;
                        break;
                    case CANNOT_GENERATE_PROCEDURE:
                        cerr << LineBreak("|-> Die Prozedur \"" + sErrorToken + "\" konnte nicht erzeugt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help new\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Prozedur konnte nicht erzeugt werden" << endl;
                        break;
                    case CANNOT_GENERATE_FILE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" konnte nicht erzeugt werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help new\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei konnte nicht erzeugt werden" << endl;
                        break;
                    case IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED:
                        cerr << LineBreak("|-> Die Prozedur \"" + sErrorToken + "\" scheint zu enden, bevor alle Schleifen und/oder Bedingungen geschlossen wurden$(Das Kommando \"endprocedure\" erscheint, bevor ausreichend \"endif\"/\"endfor\"/\"endwhile\" verwendet wurden).$Dies kann zu unvorhergesehenen Fehlern während der Ausführung führen und sollte manuell kontrolliert werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help procedure\", \"help for\", \"help while\" und \"help if\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Schleifenfehler während einer Prozedurdeklaration" << endl;
                        break;
                    case CANNOT_EDIT_FILE_TYPE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" ist weder eine Text- noch eine Bilddatei (oder ein unbekannter Datentyp) und kann nicht in einem Editor geöffnet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help edit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datei kann nicht in einem Editor geöffnet werden" << endl;
                        break;
                    case NO_INTEGRATION_RANGES:
                        cerr << LineBreak("|-> Es wurde kein Integrationsintervall angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help integrate\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlendes Integrationsintervall" << endl;
                        break;
                    case INVALID_INTEGRATION_PRECISION:
                        cerr << LineBreak("|-> Die Präzision der Integration wurde zu fein für das Intervall gewählt.$(NumeRe unterstützt maximal 10^10 Integrations-Schritte)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help integrate\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ungültige Integrationspräzision" << endl;
                        break;
                    case NO_INTEGRATION_FUNCTION:
                        cerr << LineBreak("|-> Es wurde kein oder ein ungültiger zu integrierender Ausdruck angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help integrate\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlender Integrationsausdruck" << endl;
                        break;
                    case INVALID_INTEGRATION_RANGES:
                        cerr << LineBreak("|-> Ein angegebenes Integrationsintervall ist ungültig.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help integrate\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ungültiges Integrationsintervall" << endl;
                        break;
                    case NO_COLS:
                        cerr << LineBreak("|-> Die Spalten, auf die das Kommando ausgeführt werden soll, wurden nicht angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help random\" und \"help hist\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Spalten wurden nicht angegeben" << endl;
                        break;
                    case NO_ROWS:
                        cerr << LineBreak("|-> Die Zahl der zu füllenden Zeilen wurde nicht angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help random\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zeilen wurden nicht angegeben" << endl;
                        break;
                    case UNKNOWN_PATH_TOKEN:
                        cerr << LineBreak("|-> Der Pfadplatzhalter \"" + sErrorToken + "\" ist unbekannt oder wurde falsch geschrieben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help explorer\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Unbekannter Pfadplatzhalter" << endl;
                        break;
                    case INVALID_FILETYPE:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" kann nicht geladen werden, da ihr Dateityp nicht kompatibel ist.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Ungültiger Dateityp" << endl;
                        break;
                    case COL_COUNTS_DOESNT_MATCH:
                        cerr << LineBreak("|-> In dem Datenfile \"" + sErrorToken + "\" kann die Zahl der Spalten nicht korrekt bestimmt werden. Dies kann zum Beispiel durch tabulatorgetrennte Spalten, die selbst Leerzeichen enthalten, verursacht werden. Ersetze die Leerzeichen durch Unterstriche \"_\" und versuche es erneut.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Spaltenzahl konnte nicht bestimmt werden" << endl;
                        break;
                    case CANNOT_EVAL_FOR:
                        cerr << LineBreak("|-> Eine FOR-Schleife konnte nicht ausgewertet werden. Möglicherweise war eine Deklaration falsch oder die Schleifenkontrolle brach die Schleife ab.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help for\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: FOR-Schleife konnte nicht ausgewertet werden" << endl;
                        break;
                    case CANNOT_EVAL_WHILE:
                        cerr << LineBreak("|-> Eine WHILE-Schleife konnte nicht ausgewertet werden. Möglicherweise war eine Deklaration falsch oder die Schleifenkontrolle brach die Schleife ab.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help while\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: WHILE-Schleife konnte nicht ausgewertet werden" << endl;
                        break;
                    case CANNOT_EVAL_IF:
                        cerr << LineBreak("|-> Eine IF-Bedingung konnte nicht ausgewertet werden. Möglicherweise war eine Deklaration falsch oder die Schleifenkontrolle brach eine innenliegende Schleife ab.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help if\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: IF-Bedingung konnte nicht ausgewertet werden" << endl;
                        break;
                    case NO_TARGET:
                        cerr << LineBreak("|-> Es wurde kein Ziel beim Kopieren/Verschieben angegeben.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help copy\" oder \"help move\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Kein Ziel angegeben" << endl;
                        break;
                    case EXTERNAL_PROGRAM_NOT_FOUND:
                        cerr << LineBreak("|-> Das externe Programm \"" + sErrorToken + "\" konnte nicht gefunden oder gestartet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help editor\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Externes Programm konnte nicht gestartet werden" << endl;
                        break;
                    case NUMBER_OF_FUNCTIONS_NOT_MATCHING:
                        cerr << LineBreak("|-> Es wurde eine ungerade Zahl an Funktionen übergeben. Die Optionen \"alphamask\" oder \"colormask\" erfordern jedoch eine gerade Anzahl an Funktionen.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plotoptions\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlende Funktion" << endl;
                        break;
                    case PROCEDURE_WITHOUT_INSTALL_FOUND:
                        cerr << LineBreak("|-> Im Script \"" + _script.getScriptFileName() + "\" wurde eine Prozedurdefinition gefunden, die nicht als Installation deklariert ist.$(Aus Sicherheitsgründen wird eine Prozedurdefinition nicht automatisch übernommen.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help install\", \"help script\" und \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Unmaskierte Prozedurdefinition im Script" << endl;
                        break;
                    case INSTALL_CMD_FOUND:
                        cerr << LineBreak("|-> Es wurde das Kommando \"install SCRIPT\" (oder der Alias \"script -start=SCRIPT install\") in einer Prozedur oder einen Script verwendet.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help install\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: \"install\" in Script oder Prozedur verwendet" << endl;
                        break;
                    case CANNOT_OPEN_LOGFILE:
                        cerr << LineBreak("|-> Die Installations-Protokolldatei \"" + _option.getExePath() + "/install.log\" konnte nicht geöffnet werden.$(Installationen werden protokolliert, um eventuelle Fehlerquellen schneller finden zu können.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help install\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Installationsprotokolldatei konnte nicht geöffnet werden" << endl;
                        break;
                    case CANNOT_OPEN_FITLOG:
                        cerr << LineBreak("|-> Die Anpassungs-Protokolldatei \"" + _option.getSavePath() + "/numerefit.log\" konnte nicht geöffnet werden.$(Anpassungen werden protokolliert, um sie später nachschlagen zu können.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Anpassungs-Protokolldatei konnte nicht geöffnet werden" << endl;
                        break;
                    case PLUGINCMD_ALREADY_EXISTS:
                        cerr << LineBreak("|-> Ein Plugin mit dem Kommando \"" + sErrorToken + "\" existiert bereits von einem anderen Programmierer oder unter einem anderen Namen. Die Deklaration wurde abgelehnt.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plugin\" und \"help install\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Pluginkommando existiert bereits" << endl;
                        break;
                    case PLUGINNAME_ALREADY_EXISTS:
                        cerr << LineBreak("|-> Ein Plugin mit dem Namen \"" + sErrorToken + "\" existiert bereits von einem anderen Programmierer oder ist mit einem anderen Kommando verknüpft. Die Deklaration wurde abgelehnt.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plugin\" und \"help install\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Plugin existiert bereits" << endl;
                        break;
                    case PLUGIN_HAS_NO_CMD:
                        cerr << LineBreak("|-> Das zu installierende Plugin deklariert kein Kommando, mit dem es aufgerufen werden kann.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plugin\" und \"help install\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Plugin deklariert kein Kommando" << endl;
                        break;
                    case PLUGIN_HAS_NO_MAIN:
                        cerr << LineBreak("|-> Das zu installierende Plugin gibt keine Hauptprozedur vor, in die beim Pluginstart gesprungen werden soll.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plugin\", \"help install\" und \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Plugin hat keine Main" << endl;
                        break;
                    case PLUGIN_MAY_NOT_OVERRIDE:
                        cerr << LineBreak("|-> Das Kommando \"" + sErrorToken + "\" ist geschützt und kann nicht von einem Plugin überschrieben werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plugin\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Geschütztes Kommando" << endl;
                        break;
                    case FILETYPE_MAY_NOT_BE_WRITTEN:
                        cerr << LineBreak("|-> Der Dateityp \"" + sErrorToken + "\" kann aus Sicherheitsgründen nicht direkt aus NumeRe heraus bearbeitet werden.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help write\" oder verwende das Kommando \"edit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Dateityp kann nicht bearbeitet werden" << endl;
                        break;
                    case NO_STRING_FOR_WRITING:
                        cerr << LineBreak("|-> Es wurden keine Zeichenketten angeben, die in die gewünschte Datei geschrieben werden können.", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help write\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Fehlende Zeichenketten" << endl;
                        break;
                    case INVALID_HLPIDX:
                        cerr << LineBreak("|-> Das Dokumentationsverzeichnis ist nicht verfügbar. Womöglich konnte die Datei nicht gefunden werden oder sie ist beschädigt.$(Ein aktuelles Dokumentationsverzeichnis kann jeder Vollversion entnommen werden.)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Dokumentationsverzeichnis fehlt" << endl;
                        break;
                    case HLP_FILE_MISSING:
                        cerr << LineBreak("|-> Die Dokumentationsdatei \"" + sErrorToken + "\" konnte nicht gefunden oder kann nicht gelesen werden. Möglicherweise ist sie beschädigt.$(Ein aktuelles Dokumentationsverzeichnis kann jeder Vollversion entnommen werden.)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Dokumentationsdatei konnte nicht gefunden werden" << endl;
                        break;
                    case PLOTDATA_IS_NAN:
                        cerr << LineBreak("|-> Die zu plottenden Funktionswerte oder Datenpunkte besitzen keinen darstellbaren Wert.$(Alle bestimmten Datenpunkte haben den Wert \"nan\" oder \"inf\")", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datenpunkte haben keinen darstellbaren Wert" << endl;
                        break;
                    case OVERFITTING_ERROR:
                        cerr << LineBreak("|-> Die Anzahl der zum Anpassen übergebenen Parameter ist größer als die Zahl der Datenpunkte.$(Wenn mehr Parameter angepasst werden sollen als es Datenpunkte gibt, sind die Ergebnisse willkürlich. Gebe weniger Parameter oder mehr Datenpunkte an)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help fit\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Zahl der Parameter größer als die Zahl der Datenpunkte (Overfitting)" << endl;
                        break;
                    case DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING:
                        cerr << LineBreak("|-> Datenpunkte aus \"cache()\" oder \"data()\" können während eines Plotvorgangs nicht durch mathematische Ausdrücke modifiziert werden.$(Die Modifikation wird aus Gründen der Eindeutigkeit und der Wiederholbarkeit unterdrückt. Möglicherweise wurde auch nur ein \"-\" zu viel verwendet oder es fehlen Kommata zwischen den Ausdrücken.)", _option) << endl;
                        cerr << LineBreak("|-> SIEHE AUCH: \"help plot\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Datenpunkte können während des Plots nicht modifiziert werden" << endl;
                        break;
                    case TOO_LARGE_BINWIDTH:
                        cerr << LineBreak("|-> Die gewünschte Breite der Histogramm-Rubriken ist größer als das gesamte bzw. das darzustellende Intervall des Datensatzes.$(Möglicherweise ist ein Vorzeichen beim Exponenten verloren gegangen. Versuche eine kleinere Binbreite)", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Binbreite größer als Histogrammintervall" << endl;
                        break;
                    case TOO_FEW_ARGS:
                        cerr << LineBreak("|-> Es wurden zu wenig Argumente für das gewünschte Kommando \"" + sErrorToken + "\" übergeben.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help "+toLowerCase(sErrorToken) + "\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zu wenig Argumente für \"" << sErrorToken << "\"" << endl;
                        break;
                    case TOO_FEW_DATAPOINTS:
                        cerr << LineBreak("|-> Die Zahl der Datenpunkte oder der Stützstellen ist zu klein für ein Datengitter.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help datagrid\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zu wenig Datenpunkte für \"datagrid\"" << endl;
                        break;
                    case TOO_LARGE_CACHE:
                        cerr << LineBreak("|-> Mehr als 1 Mio. Elemente kann der Cache derzeit nicht speichern.", _option) << endl;
                        cerr << LineBreak("|-> Versuche, einen Teil des Caches in eine Datei auszulagern und dadurch Speicher freizugeben.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zu viele Elemente für den Cache" << endl;
                        break;
                    case TABLE_DOESNT_EXIST:
                        cerr << LineBreak("|-> Die gewünschte Tabelle existiert nicht, oder es wurde keine spezifiziert.$(Möglicherweise wurde nur die Argumentklammern vergessen)", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help data\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Tabelle existiert nicht / es wurde keine spezifiziert" << endl;
                        break;
                    case STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER:
                        cerr << LineBreak("|-> Zeichenkettenvariablen dürfen nicht mit einer Ziffer beginnen.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help string\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zeichenketten dürfen nicht mit einer Ziffer beginnen" << endl;
                        break;
                    case STRINGVARS_MUSTNT_CONTAIN:
                        cerr << LineBreak("|-> Zeichenkettenvariablen dürfen das Zeichen \"" + sErrorToken + "\" nicht enthalten.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help string\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zeichenketten dürfen \"" << sErrorToken << "\" nicht enthalten" << endl;
                        break;
                    case FILE_IS_EMPTY:
                        cerr << LineBreak("|-> Die Datei \"" + sErrorToken + "\" enthält keine Datenpunkte.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch: \"help data\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Die Datei \"" << sErrorToken << "\" ist leer" << endl;
                        break;
                    case CANNOT_RELOAD_DATA:
                        cerr << LineBreak("|-> Der Datensatz im Speicher wurde aus mehreren Dateien zusammengesetzt oder mittels \"data -paste\" eingefügt. Er kann nicht mittels \"data -reload\" automatisch neu geladen werden.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Zusammengesetzte oder eingefügte Daten können nicht neu geladen werden" << endl;
                        break;
                    case PLOT_ERROR:
                        cerr << LineBreak("|-> Ein Plot kann nicht erzeugt werden. Entweder ist ein Plotstil nicht bekannt, oder die darzustellenden Funktionen/Datensätze konnten nicht identifiziert werden.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Plotfehler" << endl;
                        break;
                    case INVALID_CACHE_NAME:
                        cerr << LineBreak("|-> Der Name eines neuen Cache-Objekts darf nur Groß- und Kleinbuchstaben, Unterstriche und Ziffern enthalten und nicht \"data\" oder \"string\" lauten. Falls er Ziffern enthält, darf er nicht mit einer solchen beginnen.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help new\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Ungültiger Cachename" << endl;
                        break;
                    case CACHE_ALREADY_EXISTS:
                        cerr << LineBreak("|-> Der gewünschte Cache \""+sErrorToken+"\" existiert bereits.$(Verwende ggf. \""+sErrorToken.substr(0,sErrorToken.find('('))+" -swap=CACHE2\" um den Inhalt von \""+sErrorToken.substr(0,sErrorToken.find('('))+"\" mit CACHE2 zu vertauschen.)", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help new\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Cache \""+sErrorToken+"\" existiert bereits" << endl;
                        break;
                    case CACHE_DOESNT_EXIST:
                        cerr << LineBreak("|-> \""+sErrorToken+"\" bezeichnet keinen vorhandenen oder gültigen Cache.", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help new\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: \""+sErrorToken+"\" ist kein Cache" << endl;
                        break;
                    case CACHE_CANNOT_BE_RENAMED:
                        cerr << LineBreak("|-> \"cache()\" Kann nicht umbenannt werden.$(Verwende \"cache -swap=CACHE2\" um den Inhalt von \"cache\" mit \"CACHE2\" zu vertauschen.))", _option) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help new\" und \"help cache\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: \""+sErrorToken+"\" ist kein Cache" << endl;
                        break;
                    case PRIVATE_PROCEDURE_CALLED:
                        cerr << LineBreak("|-> Es wurde versucht, eine private Prozedur des Namensraums " + sErrorToken + " aufzurufen.$(Die Prozedur ist u.U. privat, da sie eine zuvor zu erfüllende Voraussetzung hat.)", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Private Prozedur von außen aufgerufen" << endl;
                        break;
                    case INLINE_PROCEDURE_IS_NOT_INLINE:
                        cerr << LineBreak("|-> Eine als \"inline\" geflagte Prozedur enthält weitere Prozeduren, Verzweigungen oder Schleifen.$(\"inline\"-geflagte Prozeduren dürfen keine weiteren Prozeduren, Schleifen oder Verzweigungen enthalten)", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Als Inline geflagte Prozedur ist nicht inline" << endl;
                        break;
                    case INVALID_INTERVAL:
                        cerr << LineBreak("|-> Ein angegebenes Intervall ist ungültig.$(Entweder ist es fehlerhaft angegeben, oder außerhalb des Datenbereichs.)", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help hist\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Intervall ist ungültig" << endl;
                        break;
                    case NO_MATRIX_FOR_MATOP:
                        cerr << LineBreak("|-> Der angegebene Ausdruck enthält keine Tabellen, die als Matrix interpretiert werden können oder eine angegebene Tabelle existiert nicht.$(Möglicherweise hat sich nur ein Tippfehler in einer Tabellenbezeichnung eingeschlichen.)", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Keine Matrix angegeben" << endl;
                        break;
                    case WRONG_MATRIX_DIMENSIONS_FOR_MATOP:
                        cerr << LineBreak("|-> Die angegebenen Bereiche der angegebenen Tabellen sind im Sinne der Matrixmultiplikation oder -invertierung ungültig.$(Die Zeilen- und Spaltendimensionen stimmen nicht überein)", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Falsche Dimensionen für matop" << endl;
                        break;
                    case MATRIX_IS_NOT_INVERTIBLE:
                        cerr << LineBreak("|-> Die angegebene Matrix kann nicht invertiert werden.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Falsche Dimensionen für matop" << endl;
                        break;
                    case MATRIX_IS_NOT_SYMMETRIC:
                        cerr << LineBreak("|-> Die angegebene Matrix ist nicht symmetrisch.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Matrix ist nicht symmetrisch" << endl;
                        break;
                    case LGS_HAS_NO_SOLUTION:
                        cerr << LineBreak("|-> Das lineare Gleichungssystem hat keine Lösung.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: LGS hat keine Lösung" << endl;
                        break;
                    case LGS_HAS_NO_UNIQUE_SOLUTION:
                        cerr << LineBreak("|-> Das lineare Gleichungssystem hat keine eindeutige Lösung.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help matop\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: LGS hat keine eindeutige Lösung" << endl;
                        break;
                    case NO_INTERVAL_FOR_ODE:
                        cerr << LineBreak("|-> Es wurde kein Integrationsintervall an den ODE-Solver übergeben.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help odesolve\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: ODE-Solver hat kein Integrationsintervall" << endl;
                        break;
                    case NO_EXPRESSION_FOR_ODE:
                        cerr << LineBreak("|-> Es wurde kein Ausdruck an den ODE-Solver übergeben.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help odesolve\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: ODE-Solver hat keinen Ausdruck" << endl;
                        break;
                    case NO_OPTIONS_FOR_ODE:
                        cerr << LineBreak("|-> Es wurde keine Optionen an den ODE-Solver übergeben.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help odesolve\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: ODE-Solver hat keine Optionen" << endl;
                        break;
                    case STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD:
                        cerr << LineBreak("|-> "+sErrorToken+" kann keine Zeichenketten verarbeiten.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help string\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: " << sErrorToken << " kann keine Zeichenketten verarbeiten" << endl;
                        break;
                    case CANNOT_REGULARIZE_CACHE:
                        cerr << LineBreak("|-> Der gewünschte Cache konnte nicht regularisiert werden.", _option, false) << endl;
                        cerr << LineBreak("|-> Siehe auch \"help regularize\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Cache konnte nicht regularisiert werden" << endl;
                        break;
                    /*case PROCESS_ABORTED_BY_USER:
                        cerr << LineBreak("|-> Ein Prozess wurde vom Benutzer abgebrochen.$(Während des Prozesses wurde \"ESC\" gedrückt. Dies bricht die Auswertung ab.)", _option, false) << endl;
                        //cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0) - tTimeZero, true) << "> FEHLER: Prozess vom Benutzer abgebrochen" << endl;
                        break;*/
                    default:
                        cerr << LineBreak("|-> Der Fehler Nr. " + toString(e) + " trat auf und zwang NumeRe, alle laufenden Auswertungen abzubrechen.", _option) << endl;
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: Nr. " << e << endl;
                }
                // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
                if (_script.isValid() && _script.isOpen())
                {
                    cerr << LineBreak("|-> Der Fehler wurde im ausgeführten Script nahe der " + toString((int)_script.getCurrentLine()) + ". Zeile gefunden.$Die Ausführung des Scripts wurde sicherheitshalber abgebrochen.", _option) << endl;
                    // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                    _script.close();
                }
                if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
                {
                    /*make_hline();
                    cerr << "|-> NUMERE: DEBUGGER" << endl;
                    make_hline();*/
                    cerr << "|" << endl << "|   " << toUpperCase("Modulinformationen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-24) << (char)196 << endl;
                    cerr << LineBreak("|   "+_option._debug.printModuleInformations(), _option, false) << endl;
                    cerr << "|" << endl << "|   " << toUpperCase("Stacktrace: ") << std::setfill((char)196) << std::setw(_option.getWindow()-16) << (char)196 << endl;
                    /*make_hline(-2);
                    cerr << "|-> Stacktrace:" << endl;*/
                    cerr << LineBreak("|   "+_option._debug.printStackTrace(), _option, false) << endl;
                    //make_hline(-2);
                    //cerr << "|-> Lokale numerische Variablen:" << endl;
                    cerr << "|" << endl << "|   " << toUpperCase("Lokale numerische Variablen: ") << std::setfill((char)196) << std::setw(_option.getWindow()-33) << (char)196 << endl;
                    cerr << LineBreak("|   "+_option._debug.printLocalVars(), _option, false) << endl;
                    //make_hline(-2);
                    //cerr << "|-> Lokale Zeichenketten:" << endl;
                    cerr << "|" << endl << "|   " << toUpperCase("Lokale Zeichenketten: ") << std::setfill((char)196) << std::setw(_option.getWindow()-26) << (char)196 << endl;
                    cerr << LineBreak("|   "+_option._debug.printLocalStrings(), _option, false) << endl;
                    _option._debug.reset();
                }
            }
            _pData.setFileName("");
            make_hline();
            sErrorToken = "";
            nErrorIndices[0] = -1;
            nErrorIndices[1] = -1;
            if (sCmdCache.length())
                sCmdCache.clear();
        }
        catch (...)
        {
            /* --> Allgemeine Exception abfangen, die nicht durch mu::exception_type oder std::exception
             *     abgedeckt wird <--
             */
            make_hline();
            cerr << "|-> EIN UNBEKANNTER FEHLER IST AUFGETRETEN" << endl;
            make_hline();
            cerr << LineBreak("|-> ENTER drücken, um mit NumeRe fortzufahren ...", _option) << endl;
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> FEHLER: UNKNOWN EXCEPTION" << endl;
            _pData.setFileName("");
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (sCmdCache.length())
                sCmdCache.clear();
        }
    } // while running
    return 0;
}

// --> Var-Factory: Hier werden die physikalischen Adressen der Variablen generiert <--
value_type* parser_AddVariable(const char_type* a_szName, void* a_pUserData)
{
    // --> Wir verwenden ein static-Array (~ globales Array), dessen Adressen fuer die Variablen verwendet werden <--
    static value_type afValBuf[200];
    static int iVal = 0;
    if (iVal >= 199)
        throw mu::ParserError( _T("Keinen Speicherplatz mehr!") );

    if (!bSupressAnswer)
    {
        if (199-iVal < 10)
        {
            cerr << "|-> neue Variable \"" << a_szName << "\" [double]" << endl;
            cerr << "|   (freier Speicher: " << 199-iVal << " Variablen)" << endl;
        }
    }
    afValBuf[iVal] = 0;

    return &afValBuf[iVal++];
}

// --> Zeigt eine Kurzuebersicht an <--
void parser_Help(const Settings& _option)
{
    make_hline();
    cerr << LineBreak("|-> NUMERE: WILLKOMMEN", _option) << endl;
    make_hline();
    cerr << LineBreak("|-> NumeRe ist mittels Kommandos zu bedienen. Die zentralen und wichtigsten Kommandos sind die folgenden:", _option) << endl;
    cerr << LineBreak("|   find BEGRIFFE   - Nach BEGRIFFE suchen", _option, false, 0, 22) << endl;
    cerr << LineBreak("|   help            - Zeigt die Hilfe-Übersicht an", _option, false, 0, 22) << endl;
    cerr << LineBreak("|   help THEMA      - Zeigt eine Hilfe zum THEMA an. Die Hilfe achtet nicht auf Groß- und Kleinschreibung", _option, false, 0, 22) << endl;
    cerr << LineBreak("|   list -OBJEKT    - Listet OBJEKT. OBJEKT kann groß oder klein geschrieben werden", _option, false, 0, 22) << endl;
    //cerr << LineBreak("|   mode            - Wechselt zum Menue-Modus", _option, false, 0, 22) << endl;
    cerr << LineBreak("|   quit            - Beendet NumeRe", _option, false, 0, 22) << endl;
    make_hline();
    return;
}

void parser_FirstStart(const Settings& _option)
{
    doc_FirstStart(_option);
    return;
}

