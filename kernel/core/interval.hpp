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
        enum Bounds
        {
            INCLUDE_LOWER = 1,
            INCLUDE_UPPER = 2,
            INCLUDE_BOTH = 3
        };

        std::string name;

        Interval() : m_sDefinition(), m_vInterval({NAN,NAN}), name() {}
        Interval(const std::string& sDef);
        Interval(std::complex<double> dFront, std::complex<double> dBack);
        Interval(const Interval& ivl);

        Interval& operator=(const Interval& ivl);
        std::complex<double> operator()(size_t n, size_t nSamples = 100) const;
        std::complex<double> log(size_t n, size_t nSamples = 100) const;

        std::complex<double> front() const;
        std::complex<double> back() const;

        std::complex<double> cmin() const;
        std::complex<double> cmax() const;

        double min() const;
        double max() const;
        double range() const;
        double middle() const;

        bool isInside(std::complex<double> val, int bnds = INCLUDE_BOTH) const;
        bool contains(const std::string& sVarName) const;
        size_t getSamples() const;

        void refresh();
        void reset(const std::string& sDef);
        void reset(std::complex<double> dFront, std::complex<double> dBack);

        void expand(double perc, double dMin = -INFINITY);
        Interval combine(const Interval& _ivl) const;
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
    IntervalSet() = default;

    IntervalSet& operator=(const IntervalSet& ivSet);
    Interval& operator[](size_t n);
    const Interval& operator[](size_t n) const;

    bool hasDependentIntervals() const;
    size_t size() const;
    std::vector<std::complex<double>> convert();
    void setNames(const std::vector<std::string>& vNames);
};


/////////////////////////////////////////////////
/// \brief Structure for the horizontal and
/// vertical lines in plots.
/////////////////////////////////////////////////
struct Line
{
    std::string sDesc;
    std::string sStyle;
    double dPos;

    Line() : sDesc(""), sStyle("k;2"), dPos(0.0) {}
};


/////////////////////////////////////////////////
/// \brief Structure for the axes in plots.
/////////////////////////////////////////////////
struct Axis
{
    std::string sLabel;
    std::string sStyle;
    Interval ivl;
};


/////////////////////////////////////////////////
/// \brief Structure for describing time axes in
/// plots.
/////////////////////////////////////////////////
struct TimeAxis
{
    std::string sTimeFormat;
    bool use;

    TimeAxis() : sTimeFormat(""), use(false) {}

    void activate(const std::string& sFormat = "")
    {
        use = true;
        sTimeFormat = sFormat;

        if (!sTimeFormat.length())
            return;

        if (sTimeFormat.find("YYYY") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("YYYY"), 4, "%Y");

        if (sTimeFormat.find("YY") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("YY"), 2, "%y");

        if (sTimeFormat.find("MM") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("MM"), 2, "%m");

        if (sTimeFormat.find("DD") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("DD"), 2, "%d");

        if (sTimeFormat.find("HH") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("HH"), 2, "%H");

        if (sTimeFormat.find("hh") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("hh"), 2, "%H");

        if (sTimeFormat.find("mm") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("mm"), 2, "%M");

        if (sTimeFormat.find("ss") != std::string::npos)
            sTimeFormat.replace(sTimeFormat.find("ss"), 2, "%S");
    }

    void deactivate()
    {
        use = false;
        sTimeFormat.clear();
    }
};



#endif // INTERVAL_HPP


