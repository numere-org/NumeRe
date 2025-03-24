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

#include "units.hpp"
#include "../utils/stringtools.hpp"
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief Identity function. Used for ignoring
/// functions and parameters in special cases.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array numfnc_Identity(const mu::Array& v)
{
    return v;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1M.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Mega(const mu::Array& a_fVal)
{
    return a_fVal * 1e6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1m.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Milli(const mu::Array& a_fVal)
{
    return a_fVal * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1G.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Giga(const mu::Array& a_fVal)
{
    return a_fVal * 1e9;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1k.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Kilo(const mu::Array& a_fVal)
{
    return a_fVal * 1e3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mu.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Micro(const mu::Array& a_fVal)
{
    return a_fVal * 1e-6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1n.
///
/// \param a_fVal const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Nano(const mu::Array& a_fVal)
{
    return a_fVal * 1e-9;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1eV.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_ElectronVolt(const mu::Array& v)
{
    return v * 1.60217657e-19;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1fm.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Fermi(const mu::Array& v)
{
    return v * 1e-15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1A.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Angstroem(const mu::Array& v)
{
    return v * 1e-10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bn.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Barn(const mu::Array& v)
{
    return v * 1e-28;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Torr/1mmhg.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Torr(const mu::Array& v)
{
    return v * 101325.0/760.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1AU.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_AstroUnit(const mu::Array& v)
{
    return v * 149597870700.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ly.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Lightyear(const mu::Array& v)
{
    return v * 9460730472580800.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1pc.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Parsec(const mu::Array& v)
{
    return v * 30856775777948584.2;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mi.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Mile(const mu::Array& v)
{
    return v * 1609.344;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1NM.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_NauticalMile(const mu::Array& v)
{
    return v * 1852.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1yd.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Yard(const mu::Array& v)
{
    return v * 0.9144;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1ft.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Foot(const mu::Array& v)
{
    return v * 0.3048;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1in.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Inch(const mu::Array& v)
{
    return v * 0.0254;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1cal.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Calorie(const mu::Array& v)
{
    return v * 4.1868;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1psi.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_PSI(const mu::Array& v)
{
    return v * 6894.75729;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kn.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Knoten(const mu::Array& v)
{
    return v * 463.0 / 900.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1l.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_liter(const mu::Array& v)
{
    return v * 1e-3;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1kmh.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_kmh(const mu::Array& v)
{
    return v / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mph.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_mph(const mu::Array& v)
{
    return v * 1.609334 / 3.6;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°C.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Celsius(const mu::Array& v)
{
    return v + 273.15;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1°F.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Fahrenheit(const mu::Array& v)
{
    return (v + 459.67) * 5.0 / 9.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1C.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Curie(const mu::Array& v)
{
    return v * 3.7e10;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Gs.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Gauss(const mu::Array& v)
{
    return v * 1e-4;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1Ps.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Poise(const mu::Array& v)
{
    return v * 1e-1;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bar.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Bar(const mu::Array& v)
{
    return v * 1e5;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1dyn.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Dyn(const mu::Array& v)
{
    return v * 1e-5;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1erg.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_Erg(const mu::Array& v)
{
    return v * 1e-7;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1mol.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_mol(const mu::Array& v)
{
    return v * 6.022140857E23;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1gon.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_gon(const mu::Array& v)
{
    return v * M_PI / 200.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1gon.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_deg(const mu::Array& v)
{
    return v * M_PI / 180.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1lb.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_lb(const mu::Array& v)
{
    return v * 0.45359237;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1oz.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_oz(const mu::Array& v)
{
    return unit_lb(v/16.0);
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bbl.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_bbarrel(const mu::Array& v)
{
    return v * 0.158987294928;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bl.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_barrel(const mu::Array& v)
{
    return v * 0.1192405;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1gal.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_gallon(const mu::Array& v)
{
    return unit_barrel(v/31.5);
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1pH.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_pH(const mu::Array& v)
{
    return mu::Array(mu::Value(10.0)).pow(-v);
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1bft.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_beaufort(const mu::Array& v)
{
    return 0.836 * v.pow(mu::Value(1.5));
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1BRT.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_BRT(const mu::Array& v)
{
    return v * 100.0 * unit_Foot(mu::Value(1.0)).pow(mu::Numerical(3));
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1PS.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_PS(const mu::Array& v)
{
    return v * 735.49875;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1hp.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_hp(const mu::Array& v)
{
    return v * 76.0402249 * 9.80665;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1year.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_year(const mu::Array& v)
{
    return v * 365.25*unit_day(mu::Value(1.0));
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1week.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_week(const mu::Array& v)
{
    return v * 7.0*unit_day(mu::Value(1.0));
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1day.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_day(const mu::Array& v)
{
    return v * 24.0*3600.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1day.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_hour(const mu::Array& v)
{
    return v * 3600.0;
}


/////////////////////////////////////////////////
/// \brief Conversion function for 1minute.
///
/// \param v const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_minute(const mu::Array& v)
{
    return v * 60.0;
}



///////////////////////
// Helpers/Structures
///////////////////////


/////////////////////////////////////////////////
/// \brief Reduce the fractions within the unit
/// term by aggregating same fractions to the
/// leftmost unit occurence.
///
/// \param unitFuncs std::vector<UnitFunction>&
/// \return void
///
/////////////////////////////////////////////////
static void reduceFractions(std::vector<UnitFunction>& unitFuncs)
{
    for (size_t i = 0; i < unitFuncs.size()-1; i++)
    {
        // Do not perform this for already reduced fractions
        if (unitFuncs[i].m_exp == 0.0 || !unitFuncs[i].m_unit.length())
            continue;

        for (size_t j = i+1; j < unitFuncs.size(); j++)
        {
            // Find the same unit
            if (unitFuncs[i].m_unit == unitFuncs[j].m_unit)
            {
                // Aggregate everything in the first occurence
                // and remove the found unit
                unitFuncs[i].m_exp += unitFuncs[j].m_exp;
                unitFuncs[j].m_exp = 0.0;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Get the representation of a single
/// SI-compatible unit in SI base units.
///
/// \param unit UnitFunction
/// \return std::vector<UnitFunction>
///
/////////////////////////////////////////////////
static std::vector<UnitFunction> getSiBaseUnit(UnitFunction unit)
{
    StringView sUnit = unit.m_unit;

    if (sUnit == "N") // kg*m/s^2
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "J") // kg*m^2/s^2
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "W") // kg*m^2/s^3
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-3*unit.m_exp}}};
    }

    if (sUnit == "V") // kg*m^2/(A*s^3)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"A"}, .m_exp{-unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-3*unit.m_exp}}};
    }

    if (sUnit == "Pa") // kg/(m*s^2)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{-unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "T") // kg/(A*s^2)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"A"}, .m_exp{-unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "Wb") // kg*m^2/(A*s^2)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"A"}, .m_exp{-unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "H") // kg*m^2/(A^2*s^2)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"A"}, .m_exp{-2*unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "Ohm") // kg*m^2/(A^2*s^3)
    {
        unit.m_unit = "kg";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"A"}, .m_exp{-2*unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-3*unit.m_exp}}};
    }

    if (sUnit == "S") // kg*m^2/(A^2*s^3)
    {
        unit.m_unit = "kg";
        unit.m_exp *= -1;
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}},
                UnitFunction{.m_unit{"A"}, .m_exp{-2*unit.m_exp}},
                UnitFunction{.m_unit{"s"}, .m_exp{-3*unit.m_exp}}};
    }

    if (sUnit == "F") // A^2*s^4/(kg*m^2)
    {
        double e = unit.m_exp;
        unit.m_unit = "A";
        unit.m_exp *= 2;
        return {unit,
                UnitFunction{.m_unit{"s"}, .m_exp{4*e}},
                UnitFunction{.m_unit{"kg"}, .m_exp{-e}},
                UnitFunction{.m_unit{"m"}, .m_exp{-2*e}}};
    }

    if (sUnit == "C") // A*s
    {
        unit.m_unit = "A";
        return {unit,
                UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}}};
    }

    if (sUnit == "kat") // mol/s
    {
        unit.m_unit = "mol";
        return {unit,
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}}};
    }

    if (sUnit == "lx") // cd/m^2
    {
        unit.m_unit = "cd";
        return {unit,
                UnitFunction{.m_unit{"m"}, .m_exp{-2*unit.m_exp}}};
    }

    if (sUnit == "lm")
    {
        unit.m_unit = "cd";
        return {unit};
    }

    if (sUnit == "Gy" || sUnit == "Sv") // m^2/s^2
    {
        double e = unit.m_exp;
        unit.m_unit = "m";
        unit.m_exp *= 2;
        return {unit,
                UnitFunction{.m_unit{"s"}, .m_exp{-2*e}}};
    }

    if (sUnit == "Hz" || sUnit == "Bq") // 1/s
    {
        unit.m_unit = "s";
        unit.m_exp *= -1;
        return {unit};
    }

    return {unit};
}


/////////////////////////////////////////////////
/// \brief Expand the combination of
/// SI-compatible units into their respective SI
/// base units.
///
/// \param unitFuncs std::vector<UnitFunction>&
/// \return void
///
/////////////////////////////////////////////////
static void expandToBaseSiUnits(std::vector<UnitFunction>& unitFuncs)
{
    for (size_t i = 0; i < unitFuncs.size(); i++)
    {
        // Get the expansion
        std::vector<UnitFunction> expandedConvs = getSiBaseUnit(unitFuncs[i]);

        // Remove/replace the expanded unit
        auto iter = unitFuncs.erase(unitFuncs.begin()+i);
        unitFuncs.insert(iter, expandedConvs.begin(), expandedConvs.end());

        // Advance, if the returned expansion is more than one term
        i += expandedConvs.size()-1;
    }
}


/////////////////////////////////////////////////
/// \brief Create a one-valued factor written as
/// the new unit divided through its
/// representation in SI base units. This factor
/// can be appended to the unit composition to
/// replace existing units with a new unit using
/// reduceFractions().
///
/// \param unit const UnitFunction&
/// \return std::vector<UnitFunction>
///
/////////////////////////////////////////////////
static std::vector<UnitFunction> getFactor(const UnitFunction& unit)
{
    // Get the divisor in SI base units
    std::vector<UnitFunction> factor = getSiBaseUnit(UnitFunction{.m_unit{unit.m_unit}, .m_exp{-unit.m_exp}});

    // Prepend the new unit to complete the one-valued fraction
    factor.insert(factor.begin(), unit);

    return factor;
}


/////////////////////////////////////////////////
/// \brief Calculate the penalty value of the
/// current unit combination.
///
/// \param unitFuncs const std::vector<UnitFunction>&
/// \return double
///
/////////////////////////////////////////////////
static double calculatePenalty(const std::vector<UnitFunction>& unitFuncs)
{
    double penalty = 0;

    for (const auto& ufnc : unitFuncs)
    {
        // Penalty composes out of the number of units added to their
        // respective absolute-valued exponents times 0.1 (if positive)
        // or 0.2 (if negative). This weighting enforces that less
        // different units is better than more and that positive
        // exponents are better than negative ones, although the focus
        // lies on the number of different units.
        if (ufnc.m_unit.length() && ufnc.m_exp)
            penalty += 1 + (ufnc.m_exp < 0 ? -0.2 * ufnc.m_exp : 0.1 * ufnc.m_exp);
    }

    return penalty;
}


/////////////////////////////////////////////////
/// \brief Test the simplification if the passed
/// unit is used for replacing parts of the unit
/// composition. Returns the calculated penalty.
///
/// \param unitFuncs std::vector<UnitFunction>
/// \param unit const UnitFunction&
/// \return double
///
/////////////////////////////////////////////////
static double testSimplification(std::vector<UnitFunction> unitFuncs, const UnitFunction& unit)
{
    // Get the replacement factor and append it to the unit composition
    auto factor = getFactor(unit);
    unitFuncs.insert(unitFuncs.end(), factor.begin(), factor.end());

    // Apply the replacement by reducing the fraction
    reduceFractions(unitFuncs);

    // Return the calculated penalty
    return calculatePenalty(unitFuncs);
}


/////////////////////////////////////////////////
/// \brief Simplify the unit combination by
/// searching for SI-compatible units effective
/// reducing the amount of different units within
/// the combination.
///
/// \param unitFuncs std::vector<UnitFunction>&
/// \return void
///
/////////////////////////////////////////////////
static void simplifyUnits(std::vector<UnitFunction>& unitFuncs)
{
    // 0. Reduce first
    reduceFractions(unitFuncs);

    // Variables for the base units
    double exp_kg = 0.0;
    double exp_A = 0.0;
    double exp_cd = 0.0;

    // Optimisation variables
    double basePenalty = 0;
    double currentBestPenalty = 0;
    double candidatePenalty = 0;
    double candidateExp = 0;
    std::string candidateUnit;

    // Fill a map with kg-specific compatible units
    static std::map<std::string, int> kg_units = {{"N", 1},
                                                  {"J", 1},
                                                  {"W", 1},
                                                  {"V", 1},
                                                  {"Pa", 1},
                                                  {"T", 1},
                                                  {"Wb", 1},
                                                  {"H", 1},
                                                  {"Ohm", 1},
                                                  {"F", -1}};

    // Loop is left by returning
    while (true)
    {
        basePenalty = 0;

        // 1. Find main unit in expression and the corresponding exponent (kg, A, cd) to find the
        //    the exponent of the replacing summary unit
        for (const auto& ufnc : unitFuncs)
        {
            if (ufnc.m_unit == "kg" && ufnc.m_exp)
                exp_kg = ufnc.m_exp;
            else if (ufnc.m_unit == "A" && ufnc.m_exp)
                exp_A = ufnc.m_exp;
            else if (ufnc.m_unit == "cd" && ufnc.m_exp)
                exp_cd = ufnc.m_exp;
        }

        // Calculate the penalty for the current unit combination
        basePenalty = calculatePenalty(unitFuncs);
        currentBestPenalty = basePenalty;

        // 2. Apply the inverse unit definition of all possible unit definitions to the
        //    expression and reduce the fractions
        // 3. Determine a penalty for every result: Number of (remaining) units + a contribution
        //    of their exponents u + (sign(exp) < 0 ? 0.2*abs(exp) : 0.1*exp)
        // N (kg), J (kg), W (kg), V (kg), Pa (kg), T (kg), Wb (kg), H (kg), Ohm (kg), F (kg^-1),
        if (exp_kg)
        {
            for (auto iter : kg_units)
            {
                candidatePenalty = testSimplification(unitFuncs, UnitFunction{.m_unit{iter.first}, .m_exp{iter.second*exp_kg}});

                if (candidatePenalty < currentBestPenalty)
                {
                    currentBestPenalty = candidatePenalty;
                    candidateUnit = iter.first;
                    candidateExp = iter.second*exp_kg;
                }
            }
        }

        // lx (cd)
        if (exp_cd)
        {
            candidatePenalty = testSimplification(unitFuncs, UnitFunction{.m_unit{"lx"}, .m_exp{exp_cd}});

            if (candidatePenalty < currentBestPenalty)
            {
                currentBestPenalty = candidatePenalty;
                candidateUnit = "lx";
                candidateExp = exp_cd;
            }
        }

        // F (A^2), C (A)
        if (exp_A)
        {
            candidatePenalty = testSimplification(unitFuncs, UnitFunction{.m_unit{"C"}, .m_exp{exp_A}});

            if (candidatePenalty < currentBestPenalty)
            {
                currentBestPenalty = candidatePenalty;
                candidateUnit = "C";
                candidateExp = exp_A;
            }

            if (std::abs(exp_A / 2 - std::rint(exp_A / 2)) < 1e-2)
            {
                candidatePenalty = testSimplification(unitFuncs, UnitFunction{.m_unit{"F"}, .m_exp{exp_A/2}});

                if (candidatePenalty < currentBestPenalty)
                {
                    currentBestPenalty = candidatePenalty;
                    candidateUnit = "F";
                    candidateExp = exp_A / 2;
                }
            }
        }

        //NumeReKernel::print("basePenalty="+toString(basePenalty, 7)+", currentBestPenalty=" + toString(currentBestPenalty, 7));

        // 4. Compare it with the original penalty: if higher/equal abort, otherwise repeat to find further factors
        if (currentBestPenalty >= basePenalty)
            return;

        //NumeReKernel::print("Unit: " + candidateUnit + "(" + toString(candidateExp, 5) + ")");

        // Add the best candidate to the combination of units
        auto factor = getFactor(UnitFunction{.m_unit{candidateUnit}, .m_exp{candidateExp}});
        unitFuncs.insert(unitFuncs.end(), factor.begin(), factor.end());

        // Apply the operation by reducing the fractions
        reduceFractions(unitFuncs);
    }
}



/////////////////////////////////////////////////
/// \brief A single unit in a set of units.
/////////////////////////////////////////////////
struct Unit
{
    StringView m_unit;
    double m_exp = 1.0;
};



///////////////////////
// SingleUnitConversion
///////////////////////

/////////////////////////////////////////////////
/// \brief Convert a value using the conversion
/// configuration.
///
/// This operator qualifies this structure as a
/// functor.
///
/// \param val const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array UnitFunction::operator()(const mu::Array& val)
{
    // °C and °F are affine transforms and do not follow the
    // generic convention
    if (m_nonlinear)
    {
        if (m_sourceExp < 0.0)
            return 1.0 / m_conv(m_scale * (1.0/val).pow(mu::Value(1/std::abs(m_sourceExp)))).pow(mu::Value(std::abs(m_sourceExp)));

        return m_conv(m_scale * val.pow(mu::Value(1/std::abs(m_sourceExp)))).pow(mu::Value(std::abs(m_sourceExp)));
    }

    if (m_sourceExp < 0.0)
        return val / m_conv(mu::Value(m_scale)).pow(mu::Value(std::abs(m_sourceExp)));

    return val * m_conv(mu::Value(m_scale)).pow(mu::Value(m_sourceExp));
}



///////////////////////
// UnitConversion
///////////////////////

/////////////////////////////////////////////////
/// \brief Convert a value using the conversion
/// configuration.
///
/// This operator qualifies this structure as a
/// functor.
///
/// \param val mu::Array
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array UnitConversion::operator()(mu::Array val)
{
    for (auto& conv : m_convs)
    {
        val = conv(val);
    }

    return m_finalScale * val;
}


/////////////////////////////////////////////////
/// \brief Format the embedded units into a
/// single long string.
///
/// \param mode UnitConversionMode
/// \return std::string
///
/////////////////////////////////////////////////
std::string UnitConversion::formatUnit(UnitConversionMode mode)
{
    // Evaluate the modes
    if (mode == MODE_BASESI || mode == MODE_SIMPLIFY)
        expandToBaseSiUnits(m_convs);

    // Reduce not necessary fractions or simplify
    // the expression
    if (mode == MODE_SIMPLIFY)
        simplifyUnits(m_convs);
    else
        reduceFractions(m_convs);

    // Sort to move negative exponents to end and have the
    // individual units sorted alphabetically
    // (purpose of the complicated-looking lambda)
    std::sort(m_convs.begin(), m_convs.end(), [](const UnitFunction& uf1, const UnitFunction& uf2)
              {
                  if (uf1.m_exp*uf2.m_exp <= 0)
                      return uf1.m_exp > uf2.m_exp; // negative exponents to the end
                  return uf1.m_unit < uf2.m_unit; // alphabetically, if same exponents
              });

    std::string sUnit;

    // Compose the final unit string
    for (const auto& conv : m_convs)
    {
        // Do not use units with zero-value exponents
        if (conv.m_exp == 0.0)
            continue;

        if (sUnit.length())
        {
            sUnit += " ";
            sUnit += conv.m_unit;
        }
        else
        {
            if (m_metricPrefix.length() && conv.m_unit == "kg")
                sUnit = "g";
            else
                sUnit = conv.m_unit;
        }


        // Append a exponent, if necessary
        if (conv.m_exp != 1.0)
            sUnit += "^" + toString(conv.m_exp, 7);
    }

    return m_metricPrefix + sUnit;
}




/////////////////////////////////////////////////
/// \brief Tokenize the passed unit combination
/// into single contributions including their
/// respective exponent.
///
/// \param sUnit StringView
/// \return std::vector<SingleUnit>
///
/////////////////////////////////////////////////
static std::vector<Unit> tokenizeUnit(StringView sUnit)
{
    std::vector<StringView> vTokens;

    // Tokenize. Misses the functionality to detect and handle parentheses
    while (sUnit.length())
    {
        size_t nOperator = sUnit.find_first_of(" *^/");
        size_t nParens = sUnit.find('(');

        // Jump over parentheses
        if (nOperator != std::string::npos
            && nOperator > nParens
            && getMatchingParenthesis(sUnit.subview(nParens)) != std::string::npos)
            nOperator = sUnit.find_first_of(" *^/", getMatchingParenthesis(sUnit.subview(nParens))+nParens+1);

        Unit unit;

        if (nOperator)
            vTokens.push_back(sUnit.subview(0, nOperator));

        if (nOperator == std::string::npos)
            break;

        if (sUnit[nOperator] == '^')
        {
            size_t nExponentEnd = sUnit.find_first_not_of("0123456789.-+", nOperator+1);

            if (nExponentEnd == std::string::npos)
            {
                vTokens.push_back(sUnit.subview(nOperator));
                break;
            }

            vTokens.push_back(sUnit.subview(nOperator, nExponentEnd-nOperator));
            sUnit.trim_front(nExponentEnd);
        }
        else
        {
            // Only save the division operator. Multiplications are obvious
            if (sUnit[nOperator] == '/')
                vTokens.push_back(sUnit.subview(nOperator, 1));

            sUnit.trim_front(nOperator+1);
        }
    }

    std::vector<Unit> vSingleUnits;

    // Reorganize by converting divisions into negative exponents
    // and putting everything into dedicated structures
    for (size_t i = 0; i < vTokens.size(); i++)
    {
        if (vTokens[i] == "/" && vTokens.size() > i+1)
        {
            // Handle divisions
            if (vTokens.size() > i+2 && vTokens[i+2].front() == '^')
            {
                vSingleUnits.push_back(Unit{.m_unit{vTokens[i+1]}, .m_exp{-1*StrToDb(vTokens[i+2].subview(1).to_string())}});
                i += 2;
            }
            else
            {
                vSingleUnits.push_back(Unit{.m_unit{vTokens[i+1]}, .m_exp{-1.0}});
                i++;
            }
        }
        else if (vTokens.size() > i+1 && vTokens[i+1].front() == '^')
        {
            // Handle simple exponents
            vSingleUnits.push_back(Unit{.m_unit{vTokens[i]}, .m_exp{StrToDb(vTokens[i+1].subview(1).to_string())}});
            i++;
        }
        else
            vSingleUnits.push_back(Unit{.m_unit{vTokens[i]}});
    }

    // Resolve parentheses
    for (size_t i = 0; i < vSingleUnits.size(); i++)
    {
        vSingleUnits[i].m_unit.strip();

        if (vSingleUnits[i].m_unit.front() == '(' && vSingleUnits[i].m_unit.back() == ')')
        {
            auto resolved = tokenizeUnit(vSingleUnits[i].m_unit.subview(1, vSingleUnits[i].m_unit.length()-2));

            for (auto& res : resolved)
            {
                res.m_exp *= vSingleUnits[i].m_exp;
            }

            auto iter = vSingleUnits.erase(vSingleUnits.begin() + i);
            vSingleUnits.insert(iter, resolved.begin(), resolved.end());
        }
    }

    // TODO: this is just for visual inspection
//    std::string sSerialized;
//
//    for (Unit tok : vSingleUnits)
//    {
//        if (sSerialized.length())
//            sSerialized += ",";
//
//        sSerialized += tok.m_unit.to_string();
//
//        if (tok.m_exp != 1.0)
//            sSerialized += "(" + toString(tok.m_exp, 7) + ")";
//    }
//
//    NumeReKernel::print("{" + sSerialized + "}");

    return vSingleUnits;
}


/////////////////////////////////////////////////
/// \brief Detect the applied SI scaling for the
/// current unit and removes the corresponding
/// prefix. Detects all common prefixes from
/// femto- up to Exa-.
///
/// \param sUnit StringView&
/// \return double
///
/////////////////////////////////////////////////
static double detectSiScaling(StringView& sUnit)
{
    if (sUnit.length() > 1 && (std::isalpha(sUnit[1]) || sUnit[1] == '°'))
    {
        switch (sUnit.front())
        {
            case 'E':
                sUnit.trim_front(1);
                return 1e18;
            case 'P':
                sUnit.trim_front(1);
                return 1e15;
            case 'T':
                sUnit.trim_front(1);
                return 1e12;
            case 'G':
                sUnit.trim_front(1);
                return 1e9;
            case 'M':
                sUnit.trim_front(1);
                return 1e6;
            case 'k':
                sUnit.trim_front(1);
                return 1e3;
            case 'h':
                sUnit.trim_front(1);
                return 1e2;
            case 'd':
            {
                if (sUnit.length() > 2 && sUnit.starts_with("da") && (std::isalpha(sUnit[2]) || sUnit[2] == '°'))
                {
                    sUnit.trim_front(2);
                    return 1e1;
                }

                sUnit.trim_front(1);
                return 1e-1;
            }
            case 'c':
                sUnit.trim_front(1);
                return 1e-2;
            case 'm':
            {
                if (sUnit.length() > 2 && sUnit.starts_with("mu") && (std::isalpha(sUnit[2]) || sUnit[2] == '°'))
                {
                    sUnit.trim_front(2);
                    return 1e-6;
                }

                sUnit.trim_front(1);
                return 1e-3;
            }
            case 'µ':
            case 'u': // e.g. um instead if µm
                sUnit.trim_front(1);
                return 1e-6;
            case 'n':
                sUnit.trim_front(1);
                return 1e-9;
            case 'p':
                sUnit.trim_front(1);
                return 1e-12;
            case 'f':
                sUnit.trim_front(1);
                return 1e-15;
            case 'a':
                sUnit.trim_front(1);
                return 1e-18;
        }
    }

    return 1.0;
}


/////////////////////////////////////////////////
/// \brief Detect and return the unit conversion
/// for a single unit-exponent combination. Might
/// return multiple conversions, because a single
/// unit might be composed out of multiple single
/// units (e.g. N = kg * m / s^2). This is to
/// allow the latter reduction of the compose
/// fraction.
///
/// \param unit Unit
/// \return std::vector<UnitFunction>
///
/////////////////////////////////////////////////
static std::vector<UnitFunction> getUnitFunction(Unit unit)
{
    StringView sUnit = unit.m_unit;

    if (sUnit == "kmh" || sUnit == "kph")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{1e3}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "gph")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_gallon}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "mpg")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Mile}},
                UnitFunction{.m_unit{"m"}, .m_exp{-3*unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_gallon}}};

    if (sUnit == "Gs")
        return {UnitFunction{.m_unit{"T"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Gauss}}};

    if (sUnit == "kn")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_NauticalMile}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "mile" || sUnit == "mi")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Mile}}};

    if (sUnit == "NM")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_NauticalMile}}};

    if (sUnit == "mph")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Mile}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "Torr" || sUnit == "mmHg")
        return {UnitFunction{.m_unit{"Pa"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Torr}}};

    if (sUnit == "ft")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Foot}}};

    if (sUnit == "in" || sUnit == "\"")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Inch}}};

    if (sUnit == "mil" || sUnit == "thou")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{1e-3}, .m_conv{unit_Inch}}};

    if (sUnit == "yd")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Yard}}};

    if (sUnit == "psi")
        return {UnitFunction{.m_unit{"Pa"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_PSI}}};

    if (sUnit == "lb")
        return {UnitFunction{.m_unit{"kg"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_lb}}};

    if (sUnit == "oz")
        return {UnitFunction{.m_unit{"kg"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_oz}}};

    if (sUnit == "bbl")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_bbarrel}}};

    if (sUnit == "bl")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_barrel}}};

    if (sUnit == "gal")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_gallon}}};

    if (sUnit == "hp")
        return {UnitFunction{.m_unit{"W"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_hp}}};

    if (sUnit == "PS")
        return {UnitFunction{.m_unit{"W"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_PS}}};

    if (sUnit == "BRT" || sUnit == "RT")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_BRT}}};

    // Time specifics
    if (sUnit == "y")
        return {UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_year}}};

    if (sUnit == "wk")
        return {UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_week}}};

    if (sUnit == "d")
        return {UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_day}}};

    if (sUnit == "h")
        return {UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "min")
        return {UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_minute}}};


    // Duplicated code intended to avoid those units to be mixed
    // up with the SI scaling factors
    if (sUnit == "pc")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_Parsec}}};

    if (sUnit == "Pa")
        return {UnitFunction{.m_unit{"Pa"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}}};

    if (sUnit == "mol")
        return {UnitFunction{.m_unit{"mol"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}}};

    if (sUnit == "Gal")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{1e-2}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}, .m_sourceExp{-unit.m_exp}}};

    if (sUnit == "°C" || sUnit == "degC")
        return {UnitFunction{.m_unit{"K"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_nonlinear{true}, .m_conv{unit_Celsius}}};

    if (sUnit == "°F" || sUnit == "degF")
        return {UnitFunction{.m_unit{"K"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_nonlinear{true}, .m_conv{unit_Fahrenheit}}};

    if (sUnit == "°" || sUnit == "deg")
        return {UnitFunction{.m_unit{"rad"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_deg}}};

    if (sUnit == "pH")
        return {UnitFunction{.m_unit{"mol"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_nonlinear{true}, .m_conv{unit_pH}},
                UnitFunction{.m_unit{"m"}, .m_exp{-3*unit.m_exp}, .m_sourceExp{-unit.m_exp}, .m_conv{unit_liter}}};

    if (sUnit == "bft")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_nonlinear{true}, .m_conv{unit_beaufort}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}}};



    // Try to detect additional scaling factors
    double scale = detectSiScaling(sUnit);

    if (sUnit == "°C" || sUnit == "degC")
        return {UnitFunction{.m_unit{"K"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_nonlinear{true}, .m_conv{unit_Celsius}}};

    if (sUnit == "°F" || sUnit == "degF")
        return {UnitFunction{.m_unit{"K"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_nonlinear{true}, .m_conv{unit_Fahrenheit}}};

    if (sUnit == "°" || sUnit == "deg")
        return {UnitFunction{.m_unit{"rad"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_deg}}};

    if (sUnit == "eV")
        return {UnitFunction{.m_unit{"J"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_ElectronVolt}}};

    if (sUnit == "dyn")
        return {UnitFunction{.m_unit{"N"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Dyn}}};

    if (sUnit == "erg")
        return {UnitFunction{.m_unit{"J"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Erg}}};

    if (sUnit == "b")
        return {UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Barn}}};

    if (sUnit == "cal")
        return {UnitFunction{.m_unit{"J"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Calorie}}};

    if (sUnit == "AU")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_AstroUnit}}};

    if (sUnit == "Ci")
        return {UnitFunction{.m_unit{"Bq"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Curie}}};

    if (sUnit == "l" || sUnit == "L" || sUnit == "ltr")
        return {UnitFunction{.m_unit{"m"}, .m_exp{3*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_liter}}};

    if (sUnit == "ly")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Lightyear}}};

    if (sUnit == "P")
        return {UnitFunction{.m_unit{"Pa"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Poise}},
                UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}}};

    if (sUnit == "St")
        return {UnitFunction{.m_unit{"m"}, .m_exp{2*unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{1e-4*scale}},
                UnitFunction{.m_unit{"s"}, .m_exp{-unit.m_exp}, .m_sourceExp{-unit.m_exp}}};

    if (sUnit == "Gal")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{1e-2*scale}},
                UnitFunction{.m_unit{"s"}, .m_exp{-2*unit.m_exp}, .m_sourceExp{-unit.m_exp}}};

    if (sUnit == "Nm")
        return {UnitFunction{.m_unit{"N"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}},
                UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}}};

    if (sUnit == "VA")
        return {UnitFunction{.m_unit{"V"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}},
                UnitFunction{.m_unit{"A"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}}};

    if (sUnit == "Wh")
        return {UnitFunction{.m_unit{"W"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}},
                UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "Ah")
        return {UnitFunction{.m_unit{"A"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}},
                UnitFunction{.m_unit{"s"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_conv{unit_hour}}};

    if (sUnit == "bar")
        return {UnitFunction{.m_unit{"Pa"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Bar}}};

    if (sUnit == "pc")
        return {UnitFunction{.m_unit{"m"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Parsec}}};

    if (sUnit == "t")
        return {UnitFunction{.m_unit{"kg"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Kilo}}};

    if (sUnit == "g")
        return {UnitFunction{.m_unit{"kg"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_Milli}}};

    if (sUnit == "gon")
        return {UnitFunction{.m_unit{"rad"}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}, .m_conv{unit_gon}}};

    return {UnitFunction{.m_unit{sUnit.to_string()}, .m_exp{unit.m_exp}, .m_sourceExp{unit.m_exp}, .m_scale{scale}}};
}


/////////////////////////////////////////////////
/// \brief Get the corresponding SI unit and the
/// necessary conversion function for the passed
/// unit. Returns the original unit and an
/// identity function, if no conversion has been
/// found.
///
/// \param sUnit StringView
/// \param mode UnitConversionMode
/// \return UnitConversion
///
/////////////////////////////////////////////////
UnitConversion getUnitConversion(StringView sUnit, UnitConversionMode mode)
{
    // Tokenize into single units
    std::vector<Unit> vUnits = tokenizeUnit(sUnit);

    UnitConversion converter;

    for (auto unit : vUnits)
    {
        // Ignore simple numbers as units
        if (unit.m_unit == "1")
            continue;

        // Get the necessary conversion functions and the
        // new units
        std::vector<UnitFunction> convs = getUnitFunction(unit);
        converter.m_convs.insert(converter.m_convs.end(), convs.begin(), convs.end());
    }

    if (mode == MODE_SIMPLIFY)
    {
        for (const auto& conv : converter.m_convs)
        {
            if (conv.m_nonlinear)
                return converter;
        }

        mu::Value res = converter(mu::Value(1.00001)).front();
        converter.m_finalScale = res.getNum().asF64();
        double remainingScale = converter.m_finalScale;

        for (auto& conv : converter.m_convs)
        {
            remainingScale /= intPower(conv.m_scale, conv.m_sourceExp);

            conv.m_scale = 1.0;
        }

        std::vector<UnitFunction> convs = converter.m_convs;

        // Evaluate the modes
        expandToBaseSiUnits(convs);

        // Reduce not necessary fractions or simplify
        // the expression
        simplifyUnits(convs);

        // Sort to move negative exponents to end and have the
        // individual units sorted alphabetically
        // (purpose of the complicated-looking lambda)
        std::sort(convs.begin(), convs.end(), [](const UnitFunction& uf1, const UnitFunction& uf2)
                  {
                      if (uf1.m_exp*uf2.m_exp <= 0)
                          return uf1.m_exp > uf2.m_exp; // negative exponents to the end
                      return uf1.m_unit < uf2.m_unit; // alphabetically, if same exponents
                  });

        double exp = 1.0;
        double unitScale = 1.0;

        for (const auto& c : convs)
        {
            if (c.m_exp)
            {
                exp = c.m_exp;

                if (c.m_unit == "kg")
                    unitScale = std::pow(1000.0, exp);

                break;
            }
        }

        auto fact = [](int p, double exp, double unitScale) {return std::pow(10.0, p*std::abs(exp))/unitScale;};

        // Reduce the final scale factor
        if (converter.m_finalScale >= fact(18, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "E" : "a";
            converter.m_finalScale /= fact(18, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(15, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "P" : "f";
            converter.m_finalScale /= fact(15, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(12, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "T" : "p";
            converter.m_finalScale /= fact(12, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(9, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "G" : "n";
            converter.m_finalScale /= fact(9, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(6, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "M" : "µ";
            converter.m_finalScale /= fact(6, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(4, exp, unitScale)) // The prefixes for k and m will be used for a slightly larger regime
        {
            converter.m_metricPrefix = exp > 0 ? "k" : "m";
            converter.m_finalScale /= fact(3, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(-1, exp, unitScale)) // We do also accept down to 0.1 here
        {
            // Just do nothing
        }
        else if (converter.m_finalScale >= fact(-4, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "m" : "k";
            converter.m_finalScale /= fact(-3, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(-6, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "µ" : "M";
            converter.m_finalScale /= fact(-6, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(-9, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "n" : "G";
            converter.m_finalScale /= fact(-9, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(-12, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "p" : "T";
            converter.m_finalScale /= fact(-12, exp, unitScale);
        }
        else if (converter.m_finalScale >= fact(-15, exp, unitScale))
        {
            converter.m_metricPrefix = exp > 0 ? "f" : "P";
            converter.m_finalScale /= fact(-15, exp, unitScale);
        }
        else
        {
            converter.m_metricPrefix = exp > 0 ? "a" : "E";
            converter.m_finalScale /= fact(-18, exp, unitScale);
        }

        converter.m_finalScale /= remainingScale;
    }

    return converter;
}


/////////////////////////////////////////////////
/// \brief Simple wrapper for testing, whether a
/// unit has a known conversion.
///
/// \param sUnit StringView
/// \return bool
///
/////////////////////////////////////////////////
bool canConvert(StringView sUnit)
{
    UnitConversion converter = getUnitConversion(sUnit, MODE_DIRECT);

    return converter.m_convs.size() > 1 || converter.m_convs.front().m_conv != numfnc_Identity;
}


/////////////////////////////////////////////////
/// \brief Print the determined unit conversion
/// based upon the value of 1.
///
/// \param sUnit StringView
/// \param mode UnitConversionMode
/// \return std::string
///
/////////////////////////////////////////////////
std::string printUnitConversion(StringView sUnit, UnitConversionMode mode)
{
    if (!sUnit.length())
        return "";

    UnitConversion convert = getUnitConversion(sUnit, mode);

    return "1 " + sUnit + " = " + convert(mu::Value(1.0)).front().print(7) + " " + convert.formatUnit(mode);
}


/////////////////////////////////////////////////
/// \brief Provides and implements a unit
/// conversion function.
///
/// \param vals const mu::Array&
/// \param units const mu::Array&
/// \param methods const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array unit_conversion(const mu::Array& vals, const mu::Array& units, const mu::Array& methods)
{
    // Simple setup
    if (units.size() == 1 && methods.size() <= 1)
    {
        UnitConversionMode mode = MODE_DIRECT;

        if (!methods.isDefault() && methods.get(0) == mu::Value("base"))
            mode = MODE_BASESI;
        else if (!methods.isDefault() && methods.get(0) == mu::Value("simplify"))
            mode = MODE_SIMPLIFY;

        UnitConversion convert = getUnitConversion(units.get(0).getStr(), mode);
        mu::Array converted;

        if (vals.size() == 1)
            converted.push_back(convert(vals).front());
        else
            converted.push_back(convert(vals));

        converted.push_back(convert.formatUnit(mode));

        return converted;
    }

    mu::Array converted;
    mu::Array convVals;
    mu::Array convUnits;

    // Individual units or methods
    for (size_t i = 0; i < std::max({vals.size(), units.size(), methods.size()}); i++)
    {
        UnitConversionMode mode = MODE_DIRECT;

        if (!methods.isDefault() && methods.get(i) == mu::Value("base"))
            mode = MODE_BASESI;
        else if (!methods.isDefault() && methods.get(i) == mu::Value("simplify"))
            mode = MODE_SIMPLIFY;

        UnitConversion convert = getUnitConversion(units.get(i).getStr(), mode);
        convVals.push_back(convert(vals.get(i)).front());
        convUnits.push_back(convert.formatUnit(mode));
    }

    converted.push_back(convVals);
    converted.push_back(convUnits);

    return converted;
}


