/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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


#ifndef INTERVAL_HPP
#define INTERVAL_HPP

#include <vector>
#include <string>

#include "ParserLib/muParserDef.h"

/////////////////////////////////////////////////
/// \brief This class represents a single
/// interval in code providing reading access
/// functionality.
/////////////////////////////////////////////////
class Interval
{
    private:
        std::string m_sDefinition;
        std::vector<double> m_vInterval;

        void assign(const Interval& ivl);
        double getSample(size_t n, size_t nSamples) const;

    public:
        std::string name;

        Interval() : m_sDefinition(), m_vInterval(), name() {}
        Interval(const std::string& sDef);
        Interval(mu::value_type dFront, mu::value_type dBack);
        Interval(const Interval& ivl);

        Interval& operator=(const Interval& ivl);
        mu::value_type operator()(size_t n, size_t nSamples = 100) const;
        mu::value_type log(size_t n, size_t nSamples = 100) const;

        mu::value_type front() const;
        mu::value_type back() const;

        mu::value_type min() const;
        mu::value_type max() const;

        bool isInside(mu::value_type val) const;
        bool contains(const std::string& sVarName) const;
        size_t getSamples() const;

        void refresh();
};


/////////////////////////////////////////////////
/// \brief This class represents a set of
/// intervals used together for calculations and
/// simulations.
/////////////////////////////////////////////////
struct IntervalSet
{
    std::vector<Interval> intervals;

    IntervalSet(const std::string& sIntervalString);
    IntervalSet(const IntervalSet& ivSet);

    IntervalSet& operator=(const IntervalSet& ivSet);
    Interval& operator[](size_t n);

    bool hasDependentIntervals() const;
    size_t size() const;
    std::vector<mu::value_type> convert();
};

#endif // INTERVAL_HPP


