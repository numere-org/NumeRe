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

#include "student_t.hpp"
#include <boost/math/distributions/students_t.hpp>

/////////////////////////////////////////////////
/// \brief Calculate the student_t value for the
/// selected degrees of freedoms and the desired
/// confidence interval.
///
/// \param nFreedoms int
/// \param dConfidenceInterval double
/// \return double
/// \remark Extracted due to the fact that the
/// inclusion of the boost header grows a file by
/// about 400 kB and we need this function at two
/// different locations.
/////////////////////////////////////////////////
double student_t(int nFreedoms, double dConfidenceInterval)
{
    boost::math::students_t dist(nFreedoms-1);
    return boost::math::quantile(boost::math::complement(dist, (1.0-dConfidenceInterval)/2.0));
}

