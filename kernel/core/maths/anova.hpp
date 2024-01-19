/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#ifndef ANOVA_HPP
#define ANOVA_HPP

#include <string>
#include "../datamanagement/tablecolumn.hpp"

/////////////////////////////////////////////////
/// \brief Contains the relevant results of the
/// ANOVA F test.
/////////////////////////////////////////////////
struct AnovaResult
{
    std::string prefix;
    mu::value_type m_FRatio;
    mu::value_type m_significanceVal;
    mu::value_type m_significance;
    bool m_isSignificant;
    size_t m_numCategories;
};

#endif
