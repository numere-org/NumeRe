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

#include "tablecolumnimpl.hpp"
#include "../utils/tools.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief Returns the selected value as a string
/// or a default value, if it does not exist.
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string ValueColumn::getValueAsString(size_t elem) const
{
    if (elem < m_data.size())
        return toString(m_data[elem], NumeReKernel::getInstance()->getSettings().getPrecision());

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string ValueColumn::getValueAsInternalString(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string ValueColumn::getValueAsParserString(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type ValueColumn::getValue(size_t elem) const
{
    if (elem < m_data.size())
        return m_data[elem];

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single string value.
///
/// \param elem size_t
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::setValue(size_t elem, const std::string& sValue)
{
    if (isConvertible(sValue, CONVTYPE_VALUE))
        setValue(elem, StrToCmplx(toInternalString(sValue)));
    else
        throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue);
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::setValue(size_t elem, const mu::value_type& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, NAN);

    m_data[elem] = vValue;
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
    col->m_sHeadLine = m_sHeadLine;

    for (size_t i = 0; i < idx.size(); i++)
    {
        col->m_data[i] = getValue(idx[i]);
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
    {
        m_sHeadLine = column->m_sHeadLine;
        m_data = static_cast<const ValueColumn*>(column)->m_data;
    }
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
}


/////////////////////////////////////////////////
/// \brief Insert the contents of the passed
/// column at the specified positions.
///
/// \param idx const VectorIndex&
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::insert(const VectorIndex& idx, const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_VALUE)
        TableColumn::setValue(idx, static_cast<const ValueColumn*>(column)->m_data);
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
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

    // Shortcut, if everything shall be deleted
    if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
    {
        m_data.clear();
        return;
    }

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
            m_data[idx[i]] = NAN;
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Inserts as many as the selected
/// elements at the desired position, if the
/// column is already larger than the starting
/// position.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::insertElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.insert(m_data.begin()+pos, elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Appends the number of elements.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::appendElements(size_t elem)
{
    m_data.insert(m_data.end(), elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Removes the selected number of
/// elements from the column and moving all
/// following items forward.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::removeElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
}


/////////////////////////////////////////////////
/// \brief Resizes the internal array.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void ValueColumn::resize(size_t elem)
{
    if (!elem)
        m_data.clear();
    else
        m_data.resize(elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Returns 0, if both elements are equal,
/// -1 if element i is smaller than element j and
/// 1 otherwise.
///
/// \param i int
/// \param j int
/// \param unused bool
/// \return int
///
/////////////////////////////////////////////////
int ValueColumn::compare(int i, int j, bool unused) const
{
    if ((int)m_data.size() <= std::max(i, j))
        return 0;

    if (m_data[i] == m_data[j])
        return 0;
    else if (m_data[i].real() < m_data[j].real())
        return -1;

    return 1;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected element
/// is a valid value.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool ValueColumn::isValid(int elem) const
{
    if (elem >= (int)m_data.size() || mu::isnan(m_data[elem]))
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Interprets the value as a boolean.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool ValueColumn::asBool(int elem) const
{
    if (elem < 0 || elem >= (int)m_data.size())
        return false;

    return m_data[elem] != 0.0;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of this column
/// converted to the new column type. Might even
/// return itself.
///
/// \param type ColumnType
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* ValueColumn::convert(ColumnType type)
{
    TableColumn* col = nullptr;

    switch (type)
    {
        case TableColumn::TYPE_NONE:
        case TableColumn::TYPE_STRING:
        {
            col = new StringColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, toString(m_data[i], NumeReKernel::getInstance()->getSettings().getPrecision()));
            }

            break;
        }
        case TableColumn::TYPE_CATEGORICAL:
        {
            col = new CategoricalColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, toString(m_data[i], NumeReKernel::getInstance()->getSettings().getPrecision()));
            }

            break;
        }
        case TableColumn::TYPE_DATETIME:
        {
            col = new DateTimeColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, m_data[i]);
            }

            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            col = new LogicalColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, m_data[i]);
            }

            break;
        }
        case TableColumn::TYPE_VALUE:
            return this;
        default:
            return nullptr;
    }

    col->m_sHeadLine = m_sHeadLine;
    return col;
}







/////////////////////////////////////////////////
/// \brief Returns the selected value as a string
/// or a default value, if it does not exist.
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string DateTimeColumn::getValueAsString(size_t elem) const
{
    if (elem < m_data.size() && !mu::isnan(m_data[elem]))
        return toString(to_timePoint(m_data[elem]), 0);

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string DateTimeColumn::getValueAsInternalString(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string DateTimeColumn::getValueAsParserString(size_t elem) const
{
    return "\"" + getValueAsString(elem) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type DateTimeColumn::getValue(size_t elem) const
{
    if (elem < m_data.size())
        return m_data[elem];

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single string value.
///
/// \param elem size_t
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::setValue(size_t elem, const std::string& sValue)
{
    if (isConvertible(sValue, CONVTYPE_DATE_TIME))
        setValue(elem, to_double(StrToTime(toInternalString(sValue))));
    else
        throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue);
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::setValue(size_t elem, const mu::value_type& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, NAN);

    m_data[elem] = vValue.real();
}


/////////////////////////////////////////////////
/// \brief Creates a copy of the selected part of
/// this column. Can be used for simple
/// extraction into a new table.
///
/// \param idx const VectorIndex&
/// \return DateTimeColumn*
///
/////////////////////////////////////////////////
DateTimeColumn* DateTimeColumn::copy(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);

    DateTimeColumn* col = new DateTimeColumn(idx.size());
    col->m_sHeadLine = m_sHeadLine;

    for (size_t i = 0; i < idx.size(); i++)
    {
        col->m_data[i] = getValue(idx[i]).real();
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
void DateTimeColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_DATETIME || column->m_type == TableColumn::TYPE_VALUE)
    {
        m_sHeadLine = column->m_sHeadLine;
        m_data.clear();

        for (const auto& val : static_cast<const DateTimeColumn*>(column)->m_data)
            m_data.push_back(val);
    }
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
}


/////////////////////////////////////////////////
/// \brief Insert the contents of the passed
/// column at the specified positions.
///
/// \param idx const VectorIndex&
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::insert(const VectorIndex& idx, const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_DATETIME || column->m_type == TableColumn::TYPE_VALUE)
        TableColumn::setValue(idx, column->getValue(VectorIndex(0, VectorIndex::OPEN_END)));
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
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
void DateTimeColumn::deleteElements(const VectorIndex& idx)
{
    idx.setOpenEndIndex(size()-1);

    // Shortcut, if everything shall be deleted
    if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
    {
        m_data.clear();
        return;
    }

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
            m_data[idx[i]] = NAN;
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Inserts as many as the selected
/// elements at the desired position, if the
/// column is already larger than the starting
/// position.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::insertElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.insert(m_data.begin()+pos, elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Appends the number of elements.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::appendElements(size_t elem)
{
    m_data.insert(m_data.end(), elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Removes the selected number of
/// elements from the column and moving all
/// following items forward.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::removeElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
}


/////////////////////////////////////////////////
/// \brief Resizes the internal array.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::resize(size_t elem)
{
    if (!elem)
        m_data.clear();
    else
        m_data.resize(elem, NAN);
}


/////////////////////////////////////////////////
/// \brief Returns 0, if both elements are equal,
/// -1 if element i is smaller than element j and
/// 1 otherwise.
///
/// \param i int
/// \param j int
/// \param unused bool
/// \return int
///
/////////////////////////////////////////////////
int DateTimeColumn::compare(int i, int j, bool unused) const
{
    if ((int)m_data.size() <= std::max(i, j))
        return 0;

    if (m_data[i] == m_data[j])
        return 0;
    else if (m_data[i] < m_data[j])
        return -1;

    return 1;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected element
/// is a valid value.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool DateTimeColumn::isValid(int elem) const
{
    if (elem >= (int)m_data.size() || std::isnan(m_data[elem]))
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Interprets the value as a boolean.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool DateTimeColumn::asBool(int elem) const
{
    if (elem < 0 || elem >= (int)m_data.size())
        return false;

    return m_data[elem] != 0.0;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of this column
/// converted to the new column type. Might even
/// return itself.
///
/// \param type ColumnType
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* DateTimeColumn::convert(ColumnType type)
{
    TableColumn* col = nullptr;

    switch (type)
    {
        case TableColumn::TYPE_NONE:
        case TableColumn::TYPE_STRING:
        {
            col = new StringColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, toString(to_timePoint(m_data[i]), 0));
            }

            break;
        }
        case TableColumn::TYPE_CATEGORICAL:
        {
            col = new CategoricalColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, toString(to_timePoint(m_data[i]), 0));
            }

            break;
        }
        case TableColumn::TYPE_VALUE:
        {
            col = new ValueColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (!mu::isnan(m_data[i]))
                    col->setValue(i, m_data[i]);
            }

            break;
        }
        case TableColumn::TYPE_DATETIME:
            return this;
        default:
            return nullptr;
    }

    col->m_sHeadLine = m_sHeadLine;
    return col;
}








/////////////////////////////////////////////////
/// \brief Returns the selected value as a string
/// or a default value, if it does not exist.
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string LogicalColumn::getValueAsString(size_t elem) const
{
    if (elem < m_data.size())
    {
        if (m_data[elem] == LOGICAL_FALSE)
            return "false";
        else if (m_data[elem] == LOGICAL_TRUE)
            return "true";
    }

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string LogicalColumn::getValueAsInternalString(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string LogicalColumn::getValueAsParserString(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type LogicalColumn::getValue(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != LOGICAL_NAN)
        return m_data[elem] ? 1.0 : 0.0;

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single string value.
///
/// \param elem size_t
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::setValue(size_t elem, const std::string& sValue)
{
    if (isConvertible(sValue, CONVTYPE_LOGICAL))
        setValue(elem, StrToLogical(toInternalString(sValue)));
    else
        throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue);
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::setValue(size_t elem, const mu::value_type& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, LOGICAL_NAN);

    m_data[elem] = vValue != 0.0 ? LOGICAL_TRUE : LOGICAL_FALSE;
}


/////////////////////////////////////////////////
/// \brief Creates a copy of the selected part of
/// this column. Can be used for simple
/// extraction into a new table.
///
/// \param idx const VectorIndex&
/// \return LogicalColumn*
///
/////////////////////////////////////////////////
LogicalColumn* LogicalColumn::copy(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);

    LogicalColumn* col = new LogicalColumn(idx.size());
    col->m_sHeadLine = m_sHeadLine;

    for (size_t i = 0; i < idx.size(); i++)
    {
        col->m_data[i] = m_data[i];
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
void LogicalColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_LOGICAL)
    {
        m_sHeadLine = column->m_sHeadLine;
        m_data = static_cast<const LogicalColumn*>(column)->m_data;
    }
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
}


/////////////////////////////////////////////////
/// \brief Insert the contents of the passed
/// column at the specified positions.
///
/// \param idx const VectorIndex&
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::insert(const VectorIndex& idx, const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_LOGICAL)
        TableColumn::setValue(idx, column->getValue(VectorIndex(0, VectorIndex::OPEN_END)));
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
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
void LogicalColumn::deleteElements(const VectorIndex& idx)
{
    idx.setOpenEndIndex(size()-1);

    // Shortcut, if everything shall be deleted
    if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
    {
        m_data.clear();
        return;
    }

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
            m_data[idx[i]] = LOGICAL_NAN;
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Inserts as many as the selected
/// elements at the desired position, if the
/// column is already larger than the starting
/// position.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::insertElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.insert(m_data.begin()+pos, elem, LOGICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Appends the number of elements.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::appendElements(size_t elem)
{
    m_data.insert(m_data.end(), elem, LOGICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Removes the selected number of
/// elements from the column and moving all
/// following items forward.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::removeElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
}


/////////////////////////////////////////////////
/// \brief Resizes the internal array.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::resize(size_t elem)
{
    if (!elem)
        m_data.clear();
    else
        m_data.resize(elem, LOGICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Returns 0, if both elements are equal,
/// -1 if element i is smaller than element j and
/// 1 otherwise.
///
/// \param i int
/// \param j int
/// \param unused bool
/// \return int
///
/////////////////////////////////////////////////
int LogicalColumn::compare(int i, int j, bool unused) const
{
    if ((int)m_data.size() <= std::max(i, j))
        return 0;

    if (m_data[i] == m_data[j])
        return 0;
    else if (m_data[i] < m_data[j])
        return -1;

    return 1;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected element
/// is a valid value.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool LogicalColumn::isValid(int elem) const
{
    if (elem >= (int)m_data.size() || m_data[elem] == LOGICAL_NAN)
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Interprets the value as a boolean.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool LogicalColumn::asBool(int elem) const
{
    if (elem < 0 || elem >= (int)m_data.size())
        return false;

    return m_data[elem] == LOGICAL_TRUE;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of this column
/// converted to the new column type. Might even
/// return itself.
///
/// \param type ColumnType
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* LogicalColumn::convert(ColumnType type)
{
    TableColumn* col = nullptr;

    switch (type)
    {
        case TableColumn::TYPE_NONE:
        case TableColumn::TYPE_STRING:
        {
            col = new StringColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (m_data[i] != LOGICAL_NAN)
                    col->setValue(i, m_data[i] == LOGICAL_TRUE ? "true" : "false");
            }

            break;
        }
        case TableColumn::TYPE_CATEGORICAL:
        {
            col = new CategoricalColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (m_data[i] != LOGICAL_NAN)
                    col->setValue(i, m_data[i] == LOGICAL_TRUE ? "true" : "false");
            }

            break;
        }
        case TableColumn::TYPE_VALUE:
        {
            col = new ValueColumn(m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (m_data[i] != LOGICAL_NAN)
                    col->setValue(i, m_data[i] == LOGICAL_TRUE ? 1.0 : 0.0);
            }

            break;
        }
        case TableColumn::TYPE_LOGICAL:
            return this;
        default:
            return nullptr;
    }

    col->m_sHeadLine = m_sHeadLine;
    return col;
}







/////////////////////////////////////////////////
/// \brief Returns the selected value or an empty
/// string, if the value does not exist.
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringColumn::getValueAsString(size_t elem) const
{
    return toExternalString(getValueAsInternalString(elem));
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringColumn::getValueAsInternalString(size_t elem) const
{
    if (elem < m_data.size())
        return m_data[elem];

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringColumn::getValueAsParserString(size_t elem) const
{
    return "\"" + getValueAsInternalString(elem) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns always NaN, because this
/// conversion is not possible.
///
/// \param elem size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type StringColumn::getValue(size_t elem) const
{
    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single string value.
///
/// \param elem size_t
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(size_t elem, const std::string& sValue)
{
    if (elem >= m_data.size() && !sValue.length())
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1);

    m_data[elem] = sValue;
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(size_t elem, const mu::value_type& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1);

    m_data[elem] = toString(vValue, NumeReKernel::getInstance()->getSettings().getPrecision());
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
    col->m_sHeadLine = m_sHeadLine;

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
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
    {
        m_sHeadLine = column->m_sHeadLine;
        m_data = static_cast<const StringColumn*>(column)->m_data;
    }
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
}


/////////////////////////////////////////////////
/// \brief Insert the contents of the passed
/// column at the specified positions.
///
/// \param idx const VectorIndex&
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::insert(const VectorIndex& idx, const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_STRING)
        TableColumn::setValue(idx, static_cast<const StringColumn*>(column)->m_data);
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
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

    // Shortcut, if everything shall be deleted
    if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
    {
        m_data.clear();
        return;
    }

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
            m_data[idx[i]].clear();
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Inserts as many as the selected
/// elements at the desired position, if the
/// column is already larger than the starting
/// position.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::insertElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.insert(m_data.begin()+pos, elem, "");
}


/////////////////////////////////////////////////
/// \brief Appends the number of elements.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::appendElements(size_t elem)
{
    m_data.insert(m_data.end(), elem, "");
}


/////////////////////////////////////////////////
/// \brief Removes the selected number of
/// elements from the column and moving all
/// following items forward.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::removeElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
}


/////////////////////////////////////////////////
/// \brief Resizes the internal array.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::resize(size_t elem)
{
    if (!elem)
        m_data.clear();
    else
        m_data.resize(elem, "");
}


/////////////////////////////////////////////////
/// \brief Returns 0, if both elements are equal,
/// -1 if element i is smaller than element j and
/// 1 otherwise.
///
/// \param i int
/// \param j int
/// \param caseinsensitive bool
/// \return int
///
/////////////////////////////////////////////////
int StringColumn::compare(int i, int j, bool caseinsensitive) const
{
    if ((int)m_data.size() <= std::max(i, j))
        return 0;

    if (caseinsensitive)
    {
        if (toLowerCase(m_data[i]) == toLowerCase(m_data[j]))
            return 0;
        else if (toLowerCase(m_data[i]) < toLowerCase(m_data[j]))
            return -1;
    }
    else
    {
        if (m_data[i] == m_data[j])
            return 0;
        else if (m_data[i] < m_data[j])
            return -1;
    }

    return 1;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected element
/// is a valid value.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool StringColumn::isValid(int elem) const
{
    if (elem >= (int)m_data.size() || !m_data[elem].length())
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Interprets the value as a boolean.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool StringColumn::asBool(int elem) const
{
    if (elem < 0 || elem >= (int)m_data.size())
        return false;

    return m_data[elem].length() != 0;
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

    return bytes + m_sHeadLine.length() * sizeof(char);
}


/////////////////////////////////////////////////
/// \brief Returns the contents of this column
/// converted to the new column type. Might even
/// return itself.
///
/// \param type ColumnType
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* StringColumn::convert(ColumnType type)
{
    TableColumn* col = nullptr;

    ConvertibleType convType = CONVTYPE_NONE;

    // Determine first, if a conversion is possible
    for (size_t i = 0; i < m_data.size(); i++)
    {
        if (!m_data[i].length())
            continue;

        if (convType == CONVTYPE_NONE)
        {
            if (isConvertible(m_data[i], CONVTYPE_VALUE))
                convType = CONVTYPE_VALUE;
            else if (isConvertible(m_data[i], CONVTYPE_LOGICAL))
                convType = CONVTYPE_LOGICAL;
            else if (isConvertible(m_data[i], CONVTYPE_DATE_TIME))
                convType = CONVTYPE_DATE_TIME;
            else
                break;
        }
        else if (!isConvertible(m_data[i], convType))
        {
            if (convType == CONVTYPE_VALUE && isConvertible(m_data[i], CONVTYPE_LOGICAL))
                convType = CONVTYPE_LOGICAL;
            else
            {
                convType = CONVTYPE_NONE;
                break;
            }
        }
    }

    switch (type)
    {
        case TableColumn::TYPE_NONE:
        {
            if (convType == CONVTYPE_NONE)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_DATETIME:
        {
            if (convType != CONVTYPE_DATE_TIME)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_VALUE:
        {
            if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_STRING:
            return this;
        case TableColumn::TYPE_CATEGORICAL:
        {
            col = new CategoricalColumn(m_data.size());
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), m_data);
            col->m_sHeadLine = m_sHeadLine;
            return col;
        }
        default:
            return nullptr;
    }

    if (convType == CONVTYPE_DATE_TIME)
        col = new DateTimeColumn(m_data.size());
    else if (convType == CONVTYPE_VALUE)
        col = new ValueColumn(m_data.size());
    else if (convType == CONVTYPE_LOGICAL)
        col = new LogicalColumn(m_data.size());

    for (size_t i = 0; i < m_data.size(); i++)
    {
        if (!m_data[i].length() || toLowerCase(m_data[i]) == "nan" || m_data[i] == "---")
            col->setValue(i, NAN);
        else if (toLowerCase(m_data[i]) == "inf")
            col->setValue(i, INFINITY);
        else if (toLowerCase(m_data[i]) == "-inf")
            col->setValue(i, -INFINITY);
        else if (convType == CONVTYPE_VALUE)
        {
            std::string strval = m_data[i];
            replaceAll(strval, ",", ".");
            col->setValue(i, StrToCmplx(strval));
        }
        else if (convType == CONVTYPE_LOGICAL)
        {
            std::string strval = m_data[i];
            replaceAll(strval, ",", ".");
            col->setValue(i, StrToLogical(strval));
        }
        else if (convType == CONVTYPE_DATE_TIME)
        {
            col->setValue(i, to_double(StrToTime(m_data[i])));
        }
    }

    col->m_sHeadLine = m_sHeadLine;
    return col;
}





/////////////////////////////////////////////////
/// \brief Returns the selected value or an empty
/// string, if the value does not exist.
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string CategoricalColumn::getValueAsString(size_t elem) const
{
    return getValueAsInternalString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string CategoricalColumn::getValueAsInternalString(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
        return m_categories[m_data[elem]];

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string CategoricalColumn::getValueAsParserString(size_t elem) const
{
//    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
//        return toString(m_data[elem]+1);
//
//    return "nan";
    return "\"" + getValueAsInternalString(elem) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns always NaN, because this
/// conversion is not possible.
///
/// \param elem size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type CategoricalColumn::getValue(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
        return m_data[elem]+1;

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single string value.
///
/// \param elem size_t
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::setValue(size_t elem, const std::string& sValue)
{
    if (elem >= m_data.size() && !sValue.length())
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, CATEGORICAL_NAN);

    auto iter = std::find(m_categories.begin(), m_categories.end(), sValue);

    if (iter != m_categories.end())
        m_data[elem] = iter - m_categories.begin();
    else
    {
        m_categories.push_back(sValue);
        m_data[elem] = m_categories.size()-1;
    }
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::setValue(size_t elem, const mu::value_type& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, CATEGORICAL_NAN);

    if (isInt(vValue) && (size_t)intCast(vValue) <= m_categories.size())
        m_data[elem] = intCast(vValue)-1;
    else if (mu::isnan(vValue))
        m_data[elem] = CATEGORICAL_NAN;
    else
        setValue(elem, toString(vValue, NumeReKernel::getInstance()->getSettings().getPrecision()));
}


/////////////////////////////////////////////////
/// \brief Creates a copy of the selected part of
/// this column. Can be used for simple
/// extraction into a new table.
///
/// \param idx const VectorIndex&
/// \return CategoricalColumn*
///
/////////////////////////////////////////////////
CategoricalColumn* CategoricalColumn::copy(const VectorIndex& idx) const
{
    idx.setOpenEndIndex(size()-1);

    CategoricalColumn* col = new CategoricalColumn(idx.size());
    col->m_sHeadLine = m_sHeadLine;

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
            col->m_data[i] = m_data[idx[i]];

        col->m_categories = m_categories;
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
void CategoricalColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_CATEGORICAL)
    {
        m_sHeadLine = column->m_sHeadLine;
        m_data = static_cast<const CategoricalColumn*>(column)->m_data;
        m_categories = static_cast<const CategoricalColumn*>(column)->m_categories;
    }
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
}


/////////////////////////////////////////////////
/// \brief Insert the contents of the passed
/// column at the specified positions.
///
/// \param idx const VectorIndex&
/// \param column const TableColumn*
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::insert(const VectorIndex& idx, const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_CATEGORICAL)
        TableColumn::setValue(idx, column->getValueAsInternalString(VectorIndex(0, VectorIndex::OPEN_END)));
    else
        throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
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
void CategoricalColumn::deleteElements(const VectorIndex& idx)
{
    idx.setOpenEndIndex(size()-1);

    // Shortcut, if everything shall be deleted
    if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
    {
        m_data.clear();
        m_categories.clear();
        return;
    }

    for (size_t i = 0; i < idx.size(); i++)
    {
        if (idx[i] >= 0 && idx[i] < (int)m_data.size())
        {
            m_data[idx[i]] = CATEGORICAL_NAN;
        }
    }

    shrink();
}


/////////////////////////////////////////////////
/// \brief Inserts as many as the selected
/// elements at the desired position, if the
/// column is already larger than the starting
/// position.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::insertElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.insert(m_data.begin()+pos, elem, CATEGORICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Appends the number of elements.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::appendElements(size_t elem)
{
    m_data.insert(m_data.end(), elem, CATEGORICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Removes the selected number of
/// elements from the column and moving all
/// following items forward.
///
/// \param pos size_t
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::removeElements(size_t pos, size_t elem)
{
    if (pos < m_data.size())
        m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
}


/////////////////////////////////////////////////
/// \brief Resizes the internal array.
///
/// \param elem size_t
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::resize(size_t elem)
{
    if (!elem)
        m_data.clear();
    else
        m_data.resize(elem, CATEGORICAL_NAN);
}


/////////////////////////////////////////////////
/// \brief Returns 0, if both elements are equal,
/// -1 if element i is smaller than element j and
/// 1 otherwise.
///
/// \param i int
/// \param j int
/// \param caseinsensitive bool
/// \return int
///
/////////////////////////////////////////////////
int CategoricalColumn::compare(int i, int j, bool caseinsensitive) const
{
    if ((int)m_data.size() <= std::max(i, j))
        return 0;

    if (caseinsensitive)
    {
        if (toLowerCase(m_categories[m_data[i]]) == toLowerCase(m_categories[m_data[j]]))
            return 0;
        else if (toLowerCase(m_categories[m_data[i]]) < toLowerCase(m_categories[m_data[j]]))
            return -1;
    }
    else
    {
        if (m_categories[m_data[i]] == m_categories[m_data[j]])
            return 0;
        else if (m_categories[m_data[i]] < m_categories[m_data[j]])
            return -1;
    }

    return 1;
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected element
/// is a valid value.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool CategoricalColumn::isValid(int elem) const
{
    if (elem >= (int)m_data.size() || m_data[elem] == CATEGORICAL_NAN)
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Interprets the value as a boolean.
///
/// \param elem int
/// \return bool
///
/////////////////////////////////////////////////
bool CategoricalColumn::asBool(int elem) const
{
    if (elem < 0 || elem >= (int)m_data.size())
        return false;

    return m_data[elem] != CATEGORICAL_NAN && m_categories[m_data[elem]].length() != 0;
}


/////////////////////////////////////////////////
/// \brief Calculates the number of bytes
/// occupied by this column.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t CategoricalColumn::getBytes() const
{
    size_t bytes = 0;

    for (const auto& val : m_categories)
        bytes += val.length() * sizeof(char);

    return size() * sizeof(int) + bytes + m_sHeadLine.length() * sizeof(char);
}


/////////////////////////////////////////////////
/// \brief Returns the contents of this column
/// converted to the new column type. Might even
/// return itself.
///
/// \param type ColumnType
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* CategoricalColumn::convert(ColumnType type)
{
    TableColumn* col = nullptr;

    ConvertibleType convType = CONVTYPE_NONE;

    // Determine first, if a conversion is possible
    for (size_t i = 0; i < m_categories.size(); i++)
    {
        if (!m_categories[i].length())
            continue;

        if (convType == CONVTYPE_NONE)
        {
            if (isConvertible(m_categories[i], CONVTYPE_VALUE))
                convType = CONVTYPE_VALUE;
            else if (isConvertible(m_categories[i], CONVTYPE_LOGICAL))
                convType = CONVTYPE_LOGICAL;
            else if (isConvertible(m_categories[i], CONVTYPE_DATE_TIME))
                convType = CONVTYPE_DATE_TIME;
            else
                break;
        }
        else if (!isConvertible(m_categories[i], convType))
        {
            if (convType == CONVTYPE_VALUE && isConvertible(m_categories[i], CONVTYPE_LOGICAL))
                convType = CONVTYPE_LOGICAL;
            else
            {
                convType = CONVTYPE_NONE;
                break;
            }
        }
    }

    switch (type)
    {
        case TableColumn::TYPE_NONE:
        {
            if (convType == CONVTYPE_NONE)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_DATETIME:
        {
            if (convType != CONVTYPE_DATE_TIME)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_VALUE:
        {
            if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_STRING:
        {
            col = new StringColumn(m_data.size());
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), TableColumn::getValueAsInternalString(VectorIndex(0, VectorIndex::OPEN_END)));
            col->m_sHeadLine = m_sHeadLine;
            return col;
        }
        case TableColumn::TYPE_CATEGORICAL:
            return this;
        default:
            return nullptr;
    }

    if (convType == CONVTYPE_DATE_TIME)
        col = new DateTimeColumn(m_data.size());
    else if (convType == CONVTYPE_VALUE)
        col = new ValueColumn(m_data.size());
    else if (convType == CONVTYPE_LOGICAL)
        col = new LogicalColumn(m_data.size());

    for (size_t i = 0; i < m_data.size(); i++)
    {
        if (m_data[i] == CATEGORICAL_NAN || toLowerCase(m_categories[m_data[i]]) == "nan" || m_categories[m_data[i]] == "---")
            col->setValue(i, NAN);
        else if (toLowerCase(m_categories[m_data[i]]) == "inf")
            col->setValue(i, INFINITY);
        else if (toLowerCase(m_categories[m_data[i]]) == "-inf")
            col->setValue(i, -INFINITY);
        else if (convType == CONVTYPE_VALUE)
        {
            std::string strval = m_categories[m_data[i]];
            replaceAll(strval, ",", ".");
            col->setValue(i, StrToCmplx(strval));
        }
        else if (convType == CONVTYPE_LOGICAL)
        {
            std::string strval = m_categories[m_data[i]];
            replaceAll(strval, ",", ".");
            col->setValue(i, StrToLogical(strval));
        }
        else if (convType == CONVTYPE_DATE_TIME)
        {
            col->setValue(i, to_double(StrToTime(m_categories[m_data[i]])));
        }
    }

    col->m_sHeadLine = m_sHeadLine;
    return col;
}





/////////////////////////////////////////////////
/// \brief Tries to convert a column if the
/// column does not contain any data (with the
/// exception of the header).
///
/// \param col TblColPtr&
/// \param colNo size_t
/// \param type TableColumn::ColumnType
/// \return void
///
/////////////////////////////////////////////////
void convert_if_empty(TblColPtr& col, size_t colNo, TableColumn::ColumnType type)
{
    if (!col || (col->m_type != type && !col->size()))
    {
        std::string sHead = TableColumn::getDefaultColumnHead(colNo);

        if (col)
            sHead = col->m_sHeadLine;

        switch (type)
        {
            case TableColumn::TYPE_STRING:
                col.reset(new StringColumn);
                break;
            case TableColumn::TYPE_VALUE:
                col.reset(new ValueColumn);
                break;
            case TableColumn::TYPE_DATETIME:
                col.reset(new DateTimeColumn);
                break;
            case TableColumn::TYPE_LOGICAL:
                col.reset(new LogicalColumn);
                break;
            case TableColumn::TYPE_CATEGORICAL:
                col.reset(new CategoricalColumn);
                break;
            default:
                return;
        }

        col->m_sHeadLine = sHead;
    }
}


/////////////////////////////////////////////////
/// \brief Tries to convert a column into the
/// selected column, if possible.
///
/// \param col TblColPtr&
/// \param colNo size_t
/// \param type TableColumn::ColumnType
/// \return void
///
/////////////////////////////////////////////////
void convert_if_needed(TblColPtr& col, size_t colNo, TableColumn::ColumnType type)
{
    if (!col || (!col->size() && col->m_type != type))
    {
        convert_if_empty(col, colNo, type);
        return;
    }

    if (col->m_type == type
        || (type == TableColumn::TYPE_CATEGORICAL && col->m_type == TableColumn::TYPE_STRING)
        || (type == TableColumn::TYPE_STRING && col->m_type == TableColumn::TYPE_CATEGORICAL))
        return;

    TableColumn* convertedCol = col->convert(type);

    if (convertedCol && convertedCol != col.get())
        col.reset(convertedCol);
}


/////////////////////////////////////////////////
/// \brief This function deletes the contents of
/// a column, if necessary, and creates a new
/// column with the correct type.
///
/// \param col TblColPtr&
/// \param colNo size_t
/// \param type TableColumn::ColumnType
/// \return void
///
/////////////////////////////////////////////////
void convert_for_overwrite(TblColPtr& col, size_t colNo, TableColumn::ColumnType type)
{
    if (!col || (!col->size() && col->m_type != type))
    {
        convert_if_empty(col, colNo, type);
        return;
    }

    if (col->m_type == type)
        return;

    std::string sHeadLine = col->m_sHeadLine;

    if (!sHeadLine.length())
        sHeadLine = TableColumn::getDefaultColumnHead(colNo);

    switch (type)
    {
        case TableColumn::TYPE_STRING:
        {
            col.reset(new StringColumn);
            col->m_sHeadLine = sHeadLine;
            break;
        }
        case TableColumn::TYPE_VALUE:
        {
            col.reset(new ValueColumn);
            col->m_sHeadLine = sHeadLine;
            break;
        }
        case TableColumn::TYPE_DATETIME:
        {
            col.reset(new DateTimeColumn);
            col->m_sHeadLine = sHeadLine;
            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            col.reset(new LogicalColumn);
            col->m_sHeadLine = sHeadLine;
            break;
        }
        case TableColumn::TYPE_CATEGORICAL:
        {
            col.reset(new CategoricalColumn);
            col->m_sHeadLine = sHeadLine;
            break;
        }
        default:
            return;
    }
}

