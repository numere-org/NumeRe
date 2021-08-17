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

#include "tablecolumn.hpp"
#include "../ui/language.hpp"
#include "../utils/tools.hpp"

extern Language _lang;


/////////////////////////////////////////////////
/// \brief Return the table column's contents as
/// a vector of strings.
///
/// \param idx const VectorIndex&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> TableColumn::getValueAsString(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);
    std::vector<std::string> vVect(idx.size());

    for (size_t i = 0; i < idx.size(); i++)
    {
        vVect[i] = getValueAsString(idx[i]);
    }

    return vVect;
}


/////////////////////////////////////////////////
/// \brief Returns the table column's contents as
/// a vector containing internal strings.
///
/// \param idx const VectorIndex&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> TableColumn::getValueAsInternalString(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);
    std::vector<std::string> vVect(idx.size());

    for (size_t i = 0; i < idx.size(); i++)
    {
        vVect[i] = getValueAsInternalString(idx[i]);
    }

    return vVect;
}


/////////////////////////////////////////////////
/// \brief Return the table column's contents as
/// a vector of numerical types.
///
/// \param idx const VectorIndex&
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> TableColumn::getValue(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);
    std::vector<mu::value_type> vVect(idx.size());

    for (size_t i = 0; i < idx.size(); i++)
    {
        vVect[i] = getValue(idx[i]);
    }

    return vVect;
}


/////////////////////////////////////////////////
/// \brief Simplification wrapper around the
/// indiced copy method to copy the whole column.
///
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* TableColumn::copy() const
{
    return copy(VectorIndex(0, VectorIndex::OPEN_END));
}


/////////////////////////////////////////////////
/// \brief Creates a default column headline for
/// a column, which can be used without an
/// instance of this class.
///
/// \param colNo size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string TableColumn::getDefaultColumnHead(size_t colNo)
{
    return _lang.get("COMMON_COL") + "_" + toString(colNo+1);
}

