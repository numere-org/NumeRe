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

mu::Array parser_Identity(const mu::Array&);

// Scaling operator callback functions
mu::Array parser_Mega(const mu::Array&);
mu::Array parser_Giga(const mu::Array&);
mu::Array parser_Kilo(const mu::Array&);
mu::Array parser_Milli(const mu::Array&);
mu::Array parser_Micro(const mu::Array&);
mu::Array parser_Nano(const mu::Array&);

// Unit conversion callback functions
mu::Array parser_ElectronVolt(const mu::Array&);
mu::Array parser_Fermi(const mu::Array&);
mu::Array parser_Angstroem(const mu::Array&);
mu::Array parser_Barn(const mu::Array&);
mu::Array parser_Torr(const mu::Array&);
mu::Array parser_AstroUnit(const mu::Array&);
mu::Array parser_Lightyear(const mu::Array&);
mu::Array parser_Parsec(const mu::Array&);
mu::Array parser_Mile(const mu::Array&);
mu::Array parser_NauticalMile(const mu::Array&);
mu::Array parser_Yard(const mu::Array&);
mu::Array parser_Foot(const mu::Array&);
mu::Array parser_Inch(const mu::Array&);
mu::Array parser_Calorie(const mu::Array&);
mu::Array parser_PSI(const mu::Array&);
mu::Array parser_Knoten(const mu::Array&);
mu::Array parser_liter(const mu::Array&);
mu::Array parser_kmh(const mu::Array&);
mu::Array parser_mph(const mu::Array&);
mu::Array parser_Celsius(const mu::Array&);
mu::Array parser_Fahrenheit(const mu::Array&);
mu::Array parser_Curie(const mu::Array&);
mu::Array parser_Gauss(const mu::Array&);
mu::Array parser_Poise(const mu::Array&);
mu::Array parser_Bar(const mu::Array&);
mu::Array parser_Dyn(const mu::Array&);
mu::Array parser_Erg(const mu::Array&);
mu::Array parser_mol(const mu::Array&);
mu::Array parser_gon(const mu::Array&);
mu::Array parser_deg(const mu::Array&);
mu::Array parser_lb(const mu::Array&);
mu::Array parser_oz(const mu::Array&);
mu::Array parser_bbarrel(const mu::Array&);
mu::Array parser_barrel(const mu::Array&);
mu::Array parser_gallon(const mu::Array&);
mu::Array parser_pH(const mu::Array&);
mu::Array parser_beaufort(const mu::Array&);
mu::Array parser_BRT(const mu::Array&);
mu::Array parser_PS(const mu::Array&);
mu::Array parser_hp(const mu::Array&);

// Time specific conversions
mu::Array parser_year(const mu::Array&);
mu::Array parser_week(const mu::Array&);
mu::Array parser_day(const mu::Array&);
mu::Array parser_hour(const mu::Array&);
mu::Array parser_minute(const mu::Array&);

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
    mu::Array (*m_conv)(const mu::Array&) = parser_Identity;

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

