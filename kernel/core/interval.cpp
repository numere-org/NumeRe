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


#include "interval.hpp"
#include "../kernel.hpp"
#include "utils/tools.hpp"

// Prototype from parser_functions.hpp
size_t findVariableInExpression(const std::string& sExpr, const std::string& sVarName);


/////////////////////////////////////////////////
/// \brief Assign a interval object to this
/// instance.
///
/// \param ivl const Interval&
/// \return void
///
/////////////////////////////////////////////////
void Interval::assign(const Interval& ivl)
{
    name = ivl.name;
    m_sDefinition = ivl.m_sDefinition;
    m_vInterval = ivl.m_vInterval;
}


/////////////////////////////////////////////////
/// \brief Return the nth sample of the interval
/// using the desired number of samples.
///
/// \param n size_t
/// \param nSamples size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
double Interval::getSample(size_t n, size_t nSamples) const
{
    // Too few samples are not reasonable
    if (nSamples < 2)
        throw SyntaxError(SyntaxError::INVALID_INDEX, m_sDefinition, SyntaxError::invalid_position, m_sDefinition);

    // Depending on the number of elements in the internal
    // interval, we'll interpret the interval access different
    if (m_vInterval.size() == 2)
        // Usual interval access using the two values as start
        // and end of the interval
        return m_vInterval.front() + (m_vInterval.back() - m_vInterval.front()) / (double)(nSamples-1) * n;
    else if (m_vInterval.size() == nSamples)
    {
        // In this case we have more than two values in
        // the internal array and the selected number of
        // samples corresponds to the array length - we
        // simply return the corresponding array entries
        if (m_vInterval.size() > n)
            return m_vInterval[n];
        else
            return m_vInterval.back();
    }
    else
    {
        // In this case we have more than two values in
        // the internal array but the selected number of
        // samples is different from the array length.
        // We use linear interpolation between the array
        // values to handle this
        double val = (m_vInterval.size()-1) / (double)(nSamples-1) * n;
        size_t nIdx = (size_t)val;
        val -= nIdx;

        if (nIdx < m_vInterval.size()+1)
            return m_vInterval[nIdx] + val * (m_vInterval[nIdx+1] - m_vInterval[nIdx]);
        else if (nIdx < m_vInterval.size())
            return m_vInterval[nIdx] + val * (m_vInterval[nIdx] - m_vInterval[nIdx-1]);
        else
            return m_vInterval.back() + (val + nIdx - m_vInterval.size() + 1) * (m_vInterval.back() - m_vInterval[m_vInterval.size()-2]);
    }
}


/////////////////////////////////////////////////
/// \brief Construct an Interval object from an
/// interval definition string.
///
/// \param sDef const std::string&
/// \note Expects a plain interval definition
/// string without brackets and variable names,
/// e.g. "a:b"
/////////////////////////////////////////////////
Interval::Interval(const std::string& sDef) : Interval()
{
    reset(sDef);
}


/////////////////////////////////////////////////
/// \brief Constructor from the two interval
/// boundaries.
///
/// \param dFront mu::value_type
/// \param dBack mu::value_type
///
/////////////////////////////////////////////////
Interval::Interval(mu::value_type dFront, mu::value_type dBack) : Interval()
{
    reset(dFront, dBack);
}


/////////////////////////////////////////////////
/// \brief Create a copy of an Interval object.
///
/// \param ivl const Interval&
///
/////////////////////////////////////////////////
Interval::Interval(const Interval& ivl)
{
    assign(ivl);
}


/////////////////////////////////////////////////
/// \brief Assignment operator overload.
///
/// \param ivl const Interval&
/// \return Interval&
///
/////////////////////////////////////////////////
Interval& Interval::operator=(const Interval& ivl)
{
    assign(ivl);
    return *this;
}


/////////////////////////////////////////////////
/// \brief Parenthesis operator overload.
///
/// \param n size_t
/// \param nSamples size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::operator()(size_t n, size_t nSamples) const
{
    return getSample(n, nSamples);
}


/////////////////////////////////////////////////
/// \brief Return a sample in logarithmic scale.
///
/// \param n size_t
/// \param nSamples size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::log(size_t n, size_t nSamples) const
{
    return std::pow(10.0, std::log10(front()) + (double)n * (std::log10(back()) - std::log10(front())) / (double)(nSamples - 1));
}


/////////////////////////////////////////////////
/// \brief Return the first element in the
/// interval.
///
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::front() const
{
    return m_vInterval.front();
}


/////////////////////////////////////////////////
/// \brief Return the last element in the
/// interval.
///
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::back() const
{
    return m_vInterval.back();
}


/////////////////////////////////////////////////
/// \brief Return the componentwise minimal
/// element in the interval.
///
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::cmin() const
{
    return *std::min_element(m_vInterval.begin(), m_vInterval.end());
}


/////////////////////////////////////////////////
/// \brief Return the componentwise maximal
/// element in the interval.
///
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Interval::cmax() const
{
    return *std::max_element(m_vInterval.begin(), m_vInterval.end());
}


/////////////////////////////////////////////////
/// \brief Return the minimal element in the
/// interval.
///
/// \return double
///
/////////////////////////////////////////////////
double Interval::min() const
{
    return *std::min_element(m_vInterval.begin(), m_vInterval.end());
}


/////////////////////////////////////////////////
/// \brief Return the maximal element in the
/// interval.
///
/// \return double
///
/////////////////////////////////////////////////
double Interval::max() const
{
    return *std::max_element(m_vInterval.begin(), m_vInterval.end());
}


/////////////////////////////////////////////////
/// \brief Return the real inteval range of this
/// interval.
///
/// \return double
///
/////////////////////////////////////////////////
double Interval::range() const
{
    return max() - min();
}


/////////////////////////////////////////////////
/// \brief Calculates the middle point of the
/// interval
///
/// \return double
///
/////////////////////////////////////////////////
double Interval::middle() const
{
    return 0.5 * (max() + min());
}


/////////////////////////////////////////////////
/// \brief Checks, whether the passed value is
/// part of this interval.
///
/// \param val mu::value_type
/// \return bool
///
/////////////////////////////////////////////////
bool Interval::isInside(mu::value_type val) const
{
    return (m_vInterval.size() == 2 && val.real() >= min() && val.real() <= max())
        || std::find(m_vInterval.begin(), m_vInterval.end(), val.real()) != m_vInterval.end();
}


/////////////////////////////////////////////////
/// \brief Check, whether a variable is part of
/// the interval definition string to detect a
/// possible dependency.
///
/// \param sVarName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Interval::contains(const std::string& sVarName) const
{
    return findVariableInExpression(m_sDefinition, sVarName) != std::string::npos;
}


/////////////////////////////////////////////////
/// \brief Returns the number of internal
/// samples. Will return zero, if the internal
/// samples are only interval start and end point.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t Interval::getSamples() const
{
    if (m_vInterval.size() == 2)
        return 0u;

    return m_vInterval.size();
}


/////////////////////////////////////////////////
/// \brief Referesh the internal interval
/// representation (e.g. after a dependency has
/// been updated).
///
/// \return void
///
/////////////////////////////////////////////////
void Interval::refresh()
{
    if (!m_sDefinition.length())
        return;

    auto indices = getAllIndices(m_sDefinition);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Examine the number of indices. If only one value then it
    // might be a singleton or a data object as interval
    if (indices.size() == 1)
    {
        // Get the data elements
        if (_data.containsTablesOrClusters(indices.front()))
            getDataElements(indices.front(), _parser, _data, NumeReKernel::getInstance()->getSettings());

        // Parse the index
        _parser.SetExpr(indices.front());
        mu::value_type* v;
        int nResults;

        // Get the return values
        v = _parser.Eval(nResults);

        // Assign the values depending on their
        // number
        if (nResults == 1)
            m_vInterval.assign(2, v[0].real());
        else
        {
            m_vInterval.clear();

            for (int i = 0; i < nResults; i++)
                m_vInterval.push_back(v[i].real());
        }
    }
    else
    {
        m_vInterval.clear();

        // Parse every index
        for (size_t i = 0; i < indices.size(); i++)
        {
            // Get possible data elements
            if (_data.containsTablesOrClusters(indices[i]))
                getDataElements(indices[i], _parser, _data, NumeReKernel::getInstance()->getSettings());

            _parser.SetExpr(indices[i]);
            m_vInterval.push_back(_parser.Eval().real());
        }
    }
}


/////////////////////////////////////////////////
/// \brief Reset the interval with a new
/// definition.
///
/// \param sDef const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Interval::reset(const std::string& sDef)
{
    m_sDefinition = sDef;
    refresh();
}


/////////////////////////////////////////////////
/// \brief Reset the interval with new boundaries.
///
/// \param dFront mu::value_type
/// \param dBack mu::value_type
/// \return void
///
/////////////////////////////////////////////////
void Interval::reset(mu::value_type dFront, mu::value_type dBack)
{
    m_vInterval.assign({dFront.real(), dBack.real()});
}


/////////////////////////////////////////////////
/// \brief Expand the interval by the
/// corresponding percentage. The expansion is
/// equally distributed to both ends keeping the
/// minimal value.
///
/// \param perc double
/// \param dMin double
/// \return void
///
/////////////////////////////////////////////////
void Interval::expand(double perc, double dMin)
{
    double r = range();
    double mi = min();
    double ma = max();

    if (r == 0.0 || r < 1e-4 * mi)
        r = fabs(ma);

    mi -= r * (perc - 1.0) * 0.5;
    ma += r * (perc - 1.0) * 0.5;

    if (mi <= dMin)
        mi = dMin + r * (perc - 1.0) * 0.5;
}


/////////////////////////////////////////////////
/// \brief Returns the (continous) interval,
/// which contains this and the passed interval.
///
/// \param _ivl const Interval&
/// \return Interval
///
/////////////////////////////////////////////////
Interval Interval::combine(const Interval& _ivl) const
{
    if (isnan(m_vInterval.first()))
        return _ivl;
    else if (isnan(_ivl.m_vInterval.first()))
        return *this;

    return Interval(::min(this->min(), _ivl.min()), ::max(this->max(), _ivl.max()));
}





/////////////////////////////////////////////////
/// \brief Construct an interval set from the
/// corresponding interval definition string.
/// Supports explicit ("x=a:b") as well as
/// implicit assignments ("[a:b,...]").
///
/// \param sIntervalString const std::string&
///
/////////////////////////////////////////////////
IntervalSet::IntervalSet(const std::string& sIntervalString)
{
    if (findParameter(sIntervalString, "x", '='))
    {
        intervals.push_back(Interval(getArgAtPos(sIntervalString, findParameter(sIntervalString, "x", '=') + 1)));
        intervals.back().name = "x";
    }

    if (findParameter(sIntervalString, "y", '='))
    {
        intervals.push_back(Interval(getArgAtPos(sIntervalString, findParameter(sIntervalString, "y", '=') + 1)));
        intervals.back().name = "y";
    }

    if (findParameter(sIntervalString, "z", '='))
    {
        intervals.push_back(Interval(getArgAtPos(sIntervalString, findParameter(sIntervalString, "z", '=') + 1)));
        intervals.back().name = "z";
    }

    if (findParameter(sIntervalString, "t", '='))
    {
        intervals.push_back(Interval(getArgAtPos(sIntervalString, findParameter(sIntervalString, "t", '=') + 1)));
        intervals.back().name = "t";
    }

    // Read the interval syntax
	if (sIntervalString.find('[') != std::string::npos && getMatchingParenthesis(sIntervalString.substr(sIntervalString.find('['))) != std::string::npos)
	{
		size_t nPos = 0;
		size_t nMatchingParens = 0;
		size_t nPrevChar = 0;

		// Find the correct interval bracket
		do
		{
			nPos = sIntervalString.find('[', nPos);

			if (nPos == std::string::npos || (nMatchingParens = getMatchingParenthesis(sIntervalString.substr(nPos))) == std::string::npos)
				break;

            nMatchingParens += nPos;
			nPrevChar = sIntervalString.find_last_not_of(" [", nPos);
			nPos++;
		}
		while (isInQuotes(sIntervalString, nPos) || (nPrevChar != std::string::npos && sIntervalString[nPrevChar] == '='));

		// If an interval bracket was found
		if (nPos != std::string::npos && nMatchingParens != std::string::npos)
		{
			std::string sRanges = sIntervalString.substr(nPos, nMatchingParens - nPos);

            // Split the whole argument list
            auto args = getAllArguments(sRanges);
            static std::string sNames[4] = {"x", "y", "z", "t"};

			for (size_t i = 0; i < args.size(); i++)
            {
                intervals.push_back(Interval(args[i]));

                if (i < 4)
                    intervals.back().name = sNames[i];
            }
		}
	}
}


/////////////////////////////////////////////////
/// \brief Create a copy of an interval set.
///
/// \param ivSet const IntervalSet&
///
/////////////////////////////////////////////////
IntervalSet::IntervalSet(const IntervalSet& ivSet)
{
    intervals = ivSet.intervals;
}


/////////////////////////////////////////////////
/// \brief Assignment operator overload.
///
/// \param ivSet const IntervalSet&
/// \return IntervalSet&
///
/////////////////////////////////////////////////
IntervalSet& IntervalSet::operator=(const IntervalSet& ivSet)
{
    intervals = ivSet.intervals;
    return *this;
}


/////////////////////////////////////////////////
/// \brief Access operator overload.
///
/// \param n size_t
/// \return Interval&
///
/////////////////////////////////////////////////
Interval& IntervalSet::operator[](size_t n)
{
    if (n < intervals.size())
        return intervals[n];

    return intervals.back();
}


/////////////////////////////////////////////////
/// \brief Access operator const overload.
///
/// \param n size_t
/// \return const Interval&
///
/////////////////////////////////////////////////
const Interval& IntervalSet::operator[](size_t n) const
{
    if (n < intervals.size())
        return intervals[n];

    return intervals.back();
}


/////////////////////////////////////////////////
/// \brief Detect, whether there are intervals,
/// which depend on each other.
///
/// \return bool
///
/////////////////////////////////////////////////
bool IntervalSet::hasDependentIntervals() const
{
    for (size_t i = 0; i < intervals.size(); i++)
    {
        for (size_t j = 0; j < intervals.size(); j++)
        {
            if (i != j && intervals[j].contains(intervals[i].name))
                return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Return the number of intervals.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t IntervalSet::size() const
{
    return intervals.size();
}


/////////////////////////////////////////////////
/// \brief Convert the new interval data types to
/// the old plain mu::value_type values.
///
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> IntervalSet::convert()
{
    std::vector<mu::value_type> vIntervals;

    for (const Interval& ivl : intervals)
    {
        vIntervals.push_back(ivl.front());
        vIntervals.push_back(ivl.back());
    }

    return vIntervals;
}


/////////////////////////////////////////////////
/// \brief Set the interval names.
///
/// \param vNames const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void IntervalSet::setNames(const std::vector<std::string>& vNames)
{
    for (size_t i = 0; i < vNames.size(); i++)
    {
        if (intervals.size() <= i)
            break;

        intervals[i].name = vNames[i];
    }
}


