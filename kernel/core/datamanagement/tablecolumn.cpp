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
#include "../utils/tools.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"

namespace mu
{
    // Forward declaration
    bool isnan(mu::value_type);
}


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
/// \brief Shrink the column by removing all
/// invalid elements from the end.
///
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::shrink()
{
    for (int i = m_data.size()-1; i >= 0; i--)
    {
        if (!mu::isnan(m_data[i]))
        {
            m_data.erase(m_data.begin()+i+1, m_data.end());
            break;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a string
/// or a default value, if it does not exist.
///
/// \param elem int
/// \return std::string
///
/////////////////////////////////////////////////
std::string ValueColumn::getValueAsString(int elem) const
{
    if (elem >= 0 && elem < m_data.size())
        return "\"" + toString(m_data[elem], NumeReKernel::getInstance()->getSettings().getPrecision()) + "\"";

    return "\"nan\"";
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem int
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type ValueColumn::getValue(int elem) const
{
    if (elem >= 0 && elem < m_data.size())
        return m_data[elem];

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Sets a string vector at the specified
/// indices.
///
/// \throws SyntaxError, because this assignment
/// is not possible.
///
/// \param idx const VectorIndex&
/// \param vValue const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::setValue(const VectorIndex& idx, const std::vector<std::string>& vValue)
{
    throw SyntaxError(SyntaxError::STRING_ERROR, "", "");
}


/////////////////////////////////////////////////
/// \brief Sets a numerical vector at the
/// specified indices.
///
/// \param idx const VectorIndex&
/// \param vValue const std::vector<mu::value_type>&
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= vValue.size())
            break;

        if (idx[i] > m_data.size())
            m_data.resize(idx[i]+1);

        m_data[idx[i]] = vValue[i];
    }
}


/////////////////////////////////////////////////
/// \brief Sets a plain numerical array at the
/// specified indices.
///
/// \param idx const VectorIndex&
/// \param _dData mu::value_type*
/// \param _nNum unsigned int
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= _nNum)
            break;

        if (idx[i] > m_data.size())
            m_data.resize(idx[i]+1);

        m_data[idx[i]] = _dData[i];
    }
}


/////////////////////////////////////////////////
/// \brief Creates a copy of the selected part of
/// this column. Can be used for simple
/// extraction into a new table.
///
/// \param idx const VectorIndex&
/// \return ValueColumn*
///
/////////////////////////////////////////////////
ValueColumn* ValueColumn::copy(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);

    ValueColumn* col = new ValueColumn(idx.size());

    for (size_t i = 0; i < idx.size(); i++)
    {
        col->m_data[i] = m_data[idx[i]];
    }

    return col;
}


/////////////////////////////////////////////////
/// \brief Assign another TableColumn's contents
/// to this table column.
///
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_VALUE)
        m_data = static_cast<const ValueColumn*>(column)->m_data;
    else
        throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, "", ""); // TODO
}


/////////////////////////////////////////////////
/// \brief Delete the specified elements.
///
/// \note Will trigger the shrinking algorithm.
///
/// \param idx const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::deleteElements(const VectorIndex& idx)
{
    idx.setOpenEndIndex(size()-1);

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < m_data.size())
            m_data[idx[i]] = NAN;
    }

    shrink();
}






/////////////////////////////////////////////////
/// \brief Shrink the column by removing all
/// invalid elements from the end.
///
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::shrink()
{
    for (int i = m_data.size()-1; i >= 0; i--)
    {
        if (m_data[i].length())
        {
            m_data.erase(m_data.begin()+i+1, m_data.end());
            break;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the selected value or an empty
/// string, if the value does not exist.
///
/// \param elem int
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringColumn::getValueAsString(int elem) const
{
    if (elem >= 0 && elem < m_data.size())
        return "\"" + m_data[elem] + "\"";

    return "\"\"";
}


/////////////////////////////////////////////////
/// \brief Returns always NaN, because this
/// conversion is not possible.
///
/// \param elem int
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type StringColumn::getValue(int elem) const
{
    return NAN;
}


/////////////////////////////////////////////////
/// \brief Assigns the strings from the passed
/// vector at the selected positions.
///
/// \param idx const VectorIndex&
/// \param vValue const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(const VectorIndex& idx, const std::vector<std::string>& vValue)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= vValue.size())
            break;

        if (idx[i] > m_data.size())
            m_data.resize(idx[i]+1);

        m_data[idx[i]] = vValue[i];
    }
}


/////////////////////////////////////////////////
/// \brief Assigns the numerical values of the
/// passed vector at the selected positions by
/// converting the values to strings.
///
/// \param idx const VectorIndex&
/// \param vValue const std::vector<mu::value_type>&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= vValue.size())
            break;

        if (idx[i] > m_data.size())
            m_data.resize(idx[i]+1);

        m_data[idx[i]] = toString(vValue[i], NumeReKernel::getInstance()->getSettings().getPrecision());
    }
}


/////////////////////////////////////////////////
/// \brief Assigns the numerical values of the
/// passed array at the selected positions by
/// converting the values to strings.
///
/// \param idx const VectorIndex&
/// \param _dData mu::value_type*
/// \param _nNum unsigned int
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= _nNum)
            break;

        if (idx[i] > m_data.size())
            m_data.resize(idx[i]+1);

        m_data[idx[i]] = toString(_dData[i], NumeReKernel::getInstance()->getSettings().getPrecision());
    }
}


/////////////////////////////////////////////////
/// \brief Creates a copy of the selected part of
/// this column. Can be used for simple
/// extraction into a new table.
///
/// \param idx const VectorIndex&
/// \return StringColumn*
///
/////////////////////////////////////////////////
StringColumn* StringColumn::copy(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);

    StringColumn* col = new StringColumn(idx.size());

    for (size_t i = 0; i < idx.size(); i++)
    {
        col->m_data[i] = m_data[idx[i]];
    }

    return col;
}


/////////////////////////////////////////////////
/// \brief Assign another TableColumn's contents
/// to this table column.
///
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_STRING)
        m_data = static_cast<const StringColumn*>(column)->m_data;
    else
        throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, "", ""); // TODO
}


/////////////////////////////////////////////////
/// \brief Delete the specified elements.
///
/// \note Will trigger the shrinking algorithm.
///
/// \param idx const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::deleteElements(const VectorIndex& idx)
{
    idx.setOpenEndIndex(size()-1);

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < m_data.size())
            m_data[idx[i]].clear();
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Calculates the number of bytes
/// occupied by this column.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t StringColumn::getBytes() const
{
    size_t bytes = 0;

    for (const auto& val : m_data)
        bytes += val.length() * sizeof(char);

    return bytes;
}











