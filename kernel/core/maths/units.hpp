/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#ifndef UNITS_HPP
#define UNITS_HPP

#include "../ParserLib/muParser.h"
#include "../structures.hpp"

mu::Array numfnc_Identity(const mu::Array&);

// Scaling operator callback functions
mu::Array unit_Mega(const mu::Array&);
mu::Array unit_Giga(const mu::Array&);
mu::Array unit_Kilo(const mu::Array&);
mu::Array unit_Milli(const mu::Array&);
mu::Array unit_Micro(const mu::Array&);
mu::Array unit_Nano(const mu::Array&);

// Unit conversion callback functions
mu::Array unit_ElectronVolt(const mu::Array&);
mu::Array unit_Fermi(const mu::Array&);
mu::Array unit_Angstroem(const mu::Array&);
mu::Array unit_Barn(const mu::Array&);
mu::Array unit_Torr(const mu::Array&);
mu::Array unit_AstroUnit(const mu::Array&);
mu::Array unit_Lightyear(const mu::Array&);
mu::Array unit_Parsec(const mu::Array&);
mu::Array unit_Mile(const mu::Array&);
mu::Array unit_NauticalMile(const mu::Array&);
mu::Array unit_Yard(const mu::Array&);
mu::Array unit_Foot(const mu::Array&);
mu::Array unit_Inch(const mu::Array&);
mu::Array unit_Calorie(const mu::Array&);
mu::Array unit_PSI(const mu::Array&);
mu::Array unit_Knoten(const mu::Array&);
mu::Array unit_liter(const mu::Array&);
mu::Array unit_kmh(const mu::Array&);
mu::Array unit_mph(const mu::Array&);
mu::Array unit_Celsius(const mu::Array&);
mu::Array unit_Fahrenheit(const mu::Array&);
mu::Array unit_Curie(const mu::Array&);
mu::Array unit_Gauss(const mu::Array&);
mu::Array unit_Poise(const mu::Array&);
mu::Array unit_Bar(const mu::Array&);
mu::Array unit_Dyn(const mu::Array&);
mu::Array unit_Erg(const mu::Array&);
mu::Array unit_mol(const mu::Array&);
mu::Array unit_gon(const mu::Array&);
mu::Array unit_deg(const mu::Array&);
mu::Array unit_lb(const mu::Array&);
mu::Array unit_oz(const mu::Array&);
mu::Array unit_bbarrel(const mu::Array&);
mu::Array unit_barrel(const mu::Array&);
mu::Array unit_gallon(const mu::Array&);
mu::Array unit_pH(const mu::Array&);
mu::Array unit_beaufort(const mu::Array&);
mu::Array unit_BRT(const mu::Array&);
mu::Array unit_PS(const mu::Array&);
mu::Array unit_hp(const mu::Array&);

// Time specific conversions
mu::Array unit_year(const mu::Array&);
mu::Array unit_week(const mu::Array&);
mu::Array unit_day(const mu::Array&);
mu::Array unit_hour(const mu::Array&);
mu::Array unit_minute(const mu::Array&);

// Conversion structure and helper
enum UnitConversionMode
{
    MODE_DIRECT,
    MODE_BASESI,
    MODE_SIMPLIFY
};

/////////////////////////////////////////////////
/// \brief Functor structure to perform a single
/// unit conversion with the detected conversion
/// parameters.
/////////////////////////////////////////////////
struct UnitFunction
{
    std::string m_unit; // Unit charseq
    double m_exp = 1.0; // Final unit exponent (only for formatting)
    double m_sourceExp = 1.0; // Part inherited from original unit
    double m_scale = 1.0; // Scalefactor
    bool m_nonlinear = false;
    mu::Array (*m_conv)(const mu::Array&) = numfnc_Identity;

    // Conversion function
    mu::Array operator()(const mu::Array& val);
};

/////////////////////////////////////////////////
/// \brief Functor structure to perform a unit
/// conversion with the detected conversion
/// parameters.
/////////////////////////////////////////////////
struct UnitConversion
{
    std::vector<UnitFunction> m_convs;

    // Conversion function
    mu::Array operator()(mu::Array val);
    // Format the unit as string
    std::string formatUnit(UnitConversionMode mode);
};

UnitConversion getUnitConversion(StringView sUnit);
bool canConvert(StringView sUnit);
std::string printUnitConversion(StringView sUnit, UnitConversionMode mode);


#endif // UNITS_HPP

