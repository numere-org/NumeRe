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

mu::value_type parser_Identity(const mu::value_type&);

// Scaling operator callback functions
mu::value_type parser_Mega(const mu::value_type&);
mu::value_type parser_Giga(const mu::value_type&);
mu::value_type parser_Kilo(const mu::value_type&);
mu::value_type parser_Milli(const mu::value_type&);
mu::value_type parser_Micro(const mu::value_type&);
mu::value_type parser_Nano(const mu::value_type&);

// Unit conversion callback functions
mu::value_type parser_ElectronVolt(const mu::value_type&);
mu::value_type parser_Fermi(const mu::value_type&);
mu::value_type parser_Angstroem(const mu::value_type&);
mu::value_type parser_Barn(const mu::value_type&);
mu::value_type parser_Torr(const mu::value_type&);
mu::value_type parser_AstroUnit(const mu::value_type&);
mu::value_type parser_Lightyear(const mu::value_type&);
mu::value_type parser_Parsec(const mu::value_type&);
mu::value_type parser_Mile(const mu::value_type&);
mu::value_type parser_NauticalMile(const mu::value_type&);
mu::value_type parser_Yard(const mu::value_type&);
mu::value_type parser_Foot(const mu::value_type&);
mu::value_type parser_Inch(const mu::value_type&);
mu::value_type parser_Calorie(const mu::value_type&);
mu::value_type parser_PSI(const mu::value_type&);
mu::value_type parser_Knoten(const mu::value_type&);
mu::value_type parser_liter(const mu::value_type&);
mu::value_type parser_kmh(const mu::value_type&);
mu::value_type parser_mph(const mu::value_type&);
mu::value_type parser_Celsius(const mu::value_type&);
mu::value_type parser_Fahrenheit(const mu::value_type&);
mu::value_type parser_Curie(const mu::value_type&);
mu::value_type parser_Gauss(const mu::value_type&);
mu::value_type parser_Poise(const mu::value_type&);
mu::value_type parser_Bar(const mu::value_type&);
mu::value_type parser_Dyn(const mu::value_type&);
mu::value_type parser_Erg(const mu::value_type&);
mu::value_type parser_mol(const mu::value_type&);
mu::value_type parser_gon(const mu::value_type&);
mu::value_type parser_deg(const mu::value_type&);
mu::value_type parser_lb(const mu::value_type&);
mu::value_type parser_oz(const mu::value_type&);
mu::value_type parser_bbarrel(const mu::value_type&);
mu::value_type parser_barrel(const mu::value_type&);
mu::value_type parser_gallon(const mu::value_type&);
mu::value_type parser_pH(const mu::value_type&);
mu::value_type parser_beaufort(const mu::value_type&);
mu::value_type parser_BRT(const mu::value_type&);
mu::value_type parser_PS(const mu::value_type&);
mu::value_type parser_hp(const mu::value_type&);

// Time specific conversions
mu::value_type parser_year(const mu::value_type&);
mu::value_type parser_week(const mu::value_type&);
mu::value_type parser_day(const mu::value_type&);
mu::value_type parser_hour(const mu::value_type&);
mu::value_type parser_minute(const mu::value_type&);

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
    mu::value_type (*m_conv)(const mu::value_type&) = parser_Identity;

    // Conversion function
    mu::value_type operator()(const mu::value_type& val);
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
    mu::value_type operator()(mu::value_type val);
    // Format the unit as string
    std::string formatUnit(UnitConversionMode mode);
};

UnitConversion getUnitConversion(StringView sUnit);
bool canConvert(StringView sUnit);
std::string printUnitConversion(StringView sUnit, UnitConversionMode mode);


#endif // UNITS_HPP

