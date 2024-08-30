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




/////////////////////////////////////////////////
/// \brief Static helper function used by some of
/// the conversion member functions to define the
/// common type of all the values in a column.
///
/// \param vVals const std::vector<std::string>&
/// \return ConvertibleType
///
/////////////////////////////////////////////////
static ConvertibleType detectCommonType(const std::vector<std::string>& vVals, int &numFormat)
{
    ConvertibleType convType = CONVTYPE_NONE;
    NumberFormatsVoter voter;

    // Determine first, if a conversion is possible
    for (size_t i = 0; i < vVals.size(); i++)
    {
        if (!vVals[i].length())
            continue;

        if (convType == CONVTYPE_NONE)
        {
            // No type was set, try to auto-detect the type
            if (isConvertible(vVals[i], CONVTYPE_VALUE, &voter))
                convType = CONVTYPE_VALUE;
            else if (isConvertible(vVals[i], CONVTYPE_LOGICAL))
                convType = CONVTYPE_LOGICAL;
            else if (isConvertible(vVals[i], CONVTYPE_DATE_TIME))
                convType = CONVTYPE_DATE_TIME;
            else
                break; // category is not used, bc. that's isomorph to string
        }
        else if (convType == CONVTYPE_LOGICAL && isConvertible(vVals[i], CONVTYPE_VALUE, &voter))
            convType = CONVTYPE_VALUE;
        else if (!isConvertible(vVals[i], convType, &voter))
        {
            // The current value is not convertible to the set or auto-detected
            // column type. Use a fall-back or simply abort
            if (convType == CONVTYPE_LOGICAL && isConvertible(vVals[i], CONVTYPE_VALUE, &voter))
            {
                // CONVTYPE_VALUE is more general than logical,
                // therefore we use this as common type
                convType = CONVTYPE_VALUE;
            }
            else if (convType == CONVTYPE_VALUE && isConvertible(vVals[i], CONVTYPE_LOGICAL))
            {
                // Do nothing here, that's already fine, as CONVTYPE_VALUE
                // is more general
            }
            else
            {
                convType = CONVTYPE_NONE;
                break;
            }
        }
    }

    if(convType == CONVTYPE_VALUE) {
        numFormat = voter.getFormat();
        if(numFormat & NUM_INVALID)
            return CONVTYPE_NONE;
    }

    return convType;
}


/////////////////////////////////////////////////
/// \brief Simple helper for formatting the
/// optional unit as part of a string.
///
/// \param sUnit const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatUnit(const std::string& sUnit)
{
    if (sUnit.length())
        return " " + sUnit;

    return "";
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
        return toCmdString(m_data[elem]);

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks or unit).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string DateTimeColumn::getValueAsInternalString(size_t elem) const
{
    if (elem < m_data.size() && !mu::isnan(m_data[elem]))
        return toString(to_timePoint(m_data[elem]), 0);

    return "nan";
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
    return "\"" + getValueAsInternalString(elem) + formatUnit(m_sUnit) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string DateTimeColumn::getValueAsStringLiteral(size_t elem) const
{
    return getValueAsInternalString(elem) + formatUnit(m_sUnit);
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> DateTimeColumn::getValue(size_t elem) const
{
    if (elem < m_data.size())
        return m_data[elem];

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// mu::Value type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value DateTimeColumn::get(size_t elem) const
{
    if (elem < m_data.size())
        return to_timePoint(m_data[elem]);

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single mu::Value.
///
/// \param elem size_t
/// \param val const mu::Value&
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::set(size_t elem, const mu::Value& val)
{
    if (val.isNumerical())
        setValue(elem, val.getNum().asCF64());
    else
        setValue(elem, val.getStr());
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
        throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue, _lang.get("ERR_NR_3603_INCONVERTIBLE_STRING"));
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void DateTimeColumn::setValue(size_t elem, const std::complex<double>& vValue)
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
    col->assignMetaData(this);

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
        assignMetaData(column);
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
    if (elem >= (int)m_data.size() || elem < 0 || std::isnan(m_data[elem]))
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
        //case TableColumn::TYPE_NONE: // Disabled for correct autoconversion
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
        case TableColumn::TYPE_DATETIME:
            return this;
        default:
        {
            if (TableColumn::isValueType(type))
                return convertNumericType(type, this);

            return nullptr;
        }
    }

    col->assignMetaData(this);
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
    return getValueAsInternalString(elem) + formatUnit(m_sUnit);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks or unit).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string LogicalColumn::getValueAsInternalString(size_t elem) const
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
/// \brief Returns the contents as parser
/// string (i.e. without quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string LogicalColumn::getValueAsStringLiteral(size_t elem) const
{
    return getValueAsString(elem);
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// numerical type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> LogicalColumn::getValue(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != LOGICAL_NAN)
        return m_data[elem] ? 1.0 : 0.0;

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// mu::Value type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value LogicalColumn::get(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != LOGICAL_NAN)
        return m_data[elem] != 0.0;

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single mu::Value.
///
/// \param elem size_t
/// \param val const mu::Value&
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::set(size_t elem, const mu::Value& val)
{
    if (val.isNumerical())
        setValue(elem, val.getNum().asCF64());
    else
        setValue(elem, val.getStr());
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
        throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue, _lang.get("ERR_NR_3603_INCONVERTIBLE_STRING"));
}


/////////////////////////////////////////////////
/// \brief Set a single numerical value.
///
/// \param elem size_t
/// \param vValue const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void LogicalColumn::setValue(size_t elem, const std::complex<double>& vValue)
{
    if (elem >= m_data.size() && mu::isnan(vValue))
        return;

    if (elem >= m_data.size())
        m_data.resize(elem+1, LOGICAL_NAN);

    m_data[elem] = mu::isnan(vValue) ? LOGICAL_NAN : (vValue != 0.0 ? LOGICAL_TRUE : LOGICAL_FALSE);
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
    col->assignMetaData(this);

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
void LogicalColumn::assign(const TableColumn* column)
{
    if (column->m_type == TableColumn::TYPE_LOGICAL)
    {
        assignMetaData(column);
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
    if (elem >= (int)m_data.size() || elem < 0 || m_data[elem] == LOGICAL_NAN)
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
        //case TableColumn::TYPE_NONE: // Disabled for correct autoconversion
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
        case TableColumn::TYPE_LOGICAL:
            return this;
        default:
        {
            if (!TableColumn::isValueType(type))
                return nullptr;

            col = createValueTypeColumn(type, m_data.size());

            for (size_t i = 0; i < m_data.size(); i++)
            {
                if (m_data[i] != LOGICAL_NAN)
                    col->setValue(i, m_data[i] == LOGICAL_TRUE ? 1.0 : 0.0);
            }
        }
    }

    col->assignMetaData(this);
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
    return "\"" + getValueAsInternalString(elem) + "\"" + formatUnit(m_sUnit);
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks or unit).
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
    return "\"" + getValueAsInternalString(elem) + formatUnit(m_sUnit) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringColumn::getValueAsStringLiteral(size_t elem) const
{
    return toExternalString(getValueAsInternalString(elem)) + formatUnit(m_sUnit);
}


/////////////////////////////////////////////////
/// \brief Returns always NaN, because this
/// conversion is not possible.
///
/// \param elem size_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> StringColumn::getValue(size_t elem) const
{
    return NAN;
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// mu::Value type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value StringColumn::get(size_t elem) const
{
    if (elem < m_data.size())
        return m_data[elem];

    return "";
}


/////////////////////////////////////////////////
/// \brief Set a single mu::Value.
///
/// \param elem size_t
/// \param val const mu::Value&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::set(size_t elem, const mu::Value& val)
{
    if (val.isNumerical())
        setValue(elem, val.getNum().asCF64());
    else
        setValue(elem, val.getStr());
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
/// \param vValue const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void StringColumn::setValue(size_t elem, const std::complex<double>& vValue)
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
    col->assignMetaData(this);

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
        assignMetaData(column);
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
    if (elem >= (int)m_data.size() || elem < 0 || !m_data[elem].length())
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
        bytes += val.capacity() * sizeof(char);

    return bytes + m_sHeadLine.capacity() * sizeof(char);
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

    // Determine first, if a conversion is possible
    int numFormat = 0;
    ConvertibleType convType = detectCommonType(m_data, numFormat);

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
            if (!m_data.size())
                convType = CONVTYPE_DATE_TIME;
            else if (convType != CONVTYPE_DATE_TIME)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            if (!m_data.size())
                convType = CONVTYPE_LOGICAL;
            else if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_STRING:
            return this;
        case TableColumn::TYPE_CATEGORICAL:
        {
            col = new CategoricalColumn(m_data.size());
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), m_data);
            col->assignMetaData(this);
            return col;
        }
        default:
        {
            if (TableColumn::isValueType(type))
            {
                if (!m_data.size())
                    convType = CONVTYPE_VALUE;
                else if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                    return nullptr;
            }
            else
                return nullptr;
        }
    }

    if (convType == CONVTYPE_DATE_TIME)
        col = new DateTimeColumn(m_data.size());
    else if (convType == CONVTYPE_VALUE)
        col = createValueTypeColumn(TableColumn::isValueType(type) ? type : TableColumn::TYPE_VALUE,
                                    m_data.size());
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
            strChangeNumberFormat(strval, numFormat);
            col->setValue(i, !isConvertible(strval, CONVTYPE_VALUE)
                             ? StrToLogical(strval)
                             : StrToCmplx(strval));
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

    col->assignMetaData(this);
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
    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
        return toString(m_data[elem]+1);

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as an internal
/// string (i.e. without quotation marks or unit).
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
    return "\"" + getValueAsInternalString(elem) + formatUnit(m_sUnit) + "\"";
}


/////////////////////////////////////////////////
/// \brief Returns the contents as parser
/// string (i.e. with quotation marks).
///
/// \param elem size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string CategoricalColumn::getValueAsStringLiteral(size_t elem) const
{
    return getValueAsInternalString(elem) + formatUnit(m_sUnit);
}


/////////////////////////////////////////////////
/// \brief Returns always NaN, because this
/// conversion is not possible.
///
/// \param elem size_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> CategoricalColumn::getValue(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
        return m_data[elem]+1;

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Returns the selected value as a
/// mu::Value type or an invalid value, if it
/// does not exist.
///
/// \param elem size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value CategoricalColumn::get(size_t elem) const
{
    if (elem < m_data.size() && m_data[elem] != CATEGORICAL_NAN)
        return mu::Category(m_data[elem]+1, m_categories[m_data[elem]]);

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Set a single mu::Value.
///
/// \param elem size_t
/// \param val const mu::Value&
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::set(size_t elem, const mu::Value& val)
{
    if (val.isNumerical())
        setValue(elem, val.getNum().asCF64());
    else
        setValue(elem, val.getStr());
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

    if (!sValue.length())
    {
        m_data[elem] = CATEGORICAL_NAN;
        return;
    }

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
/// \param vValue const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::setValue(size_t elem, const std::complex<double>& vValue)
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
    col->assignMetaData(this);

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
        assignMetaData(column);
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
    if (elem >= (int)m_data.size() || elem < 0 || m_data[elem] == CATEGORICAL_NAN)
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
        bytes += val.capacity() * sizeof(char);

    return size() * sizeof(int) + bytes + m_sHeadLine.capacity() * sizeof(char);
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

    // Determine first, if a conversion is possible
    int NumFormat = 0;
    ConvertibleType convType = detectCommonType(m_categories, NumFormat);

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
            if (!m_data.size())
                convType = CONVTYPE_DATE_TIME;
            else if (convType != CONVTYPE_DATE_TIME)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            if (!m_data.size())
                convType = CONVTYPE_LOGICAL;
            else if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                return nullptr;

            break;
        }
        case TableColumn::TYPE_STRING:
        {
            col = new StringColumn(m_data.size());
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), TableColumn::getValueAsInternalString(VectorIndex(0, VectorIndex::OPEN_END)));
            col->assignMetaData(this);
            return col;
        }
        case TableColumn::TYPE_CATEGORICAL:
            return this;
        default:
        {
            if (TableColumn::isValueType(type))
            {
                if (!m_data.size())
                    convType = CONVTYPE_VALUE;
                else if (convType != CONVTYPE_VALUE && convType != CONVTYPE_LOGICAL)
                    return nullptr;
            }
            else
                return nullptr;
        }
    }

    if (convType == CONVTYPE_DATE_TIME)
        col = new DateTimeColumn(m_data.size());
    else if (convType == CONVTYPE_VALUE)
        col = createValueTypeColumn(TableColumn::isValueType(type) ? type : TableColumn::TYPE_VALUE,
                                    m_data.size());
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
            strChangeNumberFormat(strval, NumFormat);
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

    col->assignMetaData(this);
    return col;
}


/////////////////////////////////////////////////
/// \brief Replaces the internal categories with
/// new categories.
///
/// \param vCategories const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void CategoricalColumn::setCategories(const std::vector<std::string>& vCategories)
{
    // If the number of new categories is higher
    // than the previous one: extend the necessary space
    if (vCategories.size() > m_categories.size())
        m_categories.resize(vCategories.size());

    // Now replace only the new ones. But consider the
    // case that the user might only want to reorder the categories
    for (size_t i = 0; i < vCategories.size(); i++)
    {
        // Try to find the new category in the current list
        auto iter = std::find(m_categories.begin(), m_categories.end(), vCategories[i]);

        // If the category was found and is different to the current
        // one: swap them. Otherwise simply overwrite
        if (iter != m_categories.end() && iter-m_categories.begin() != (int)i)
        {
            std::swap(m_categories[i], *iter);

            // We also have to swap the IDs
            for (int& val : m_data)
            {
                if (val == i)
                    val = iter - m_categories.begin();
                else if (val == iter - m_categories.begin())
                    val = i;
            }
        }
        else
            m_categories[i] = vCategories[i];
    }
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
        std::string sUnit;

        if (col)
        {
            sHead = col->m_sHeadLine;
            sUnit = col->m_sUnit;
        }

        switch (type)
        {
            case TableColumn::TYPE_STRING:
                col.reset(new StringColumn);
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
            {
                if (!TableColumn::isValueType(type))
                    return;

                col.reset(createValueTypeColumn(type));
            }
        }

        col->m_sHeadLine = sHead;
        col->m_sUnit = sUnit;
    }
}


/////////////////////////////////////////////////
/// \brief Tries to convert a column into the
/// selected column, if possible.
///
/// \param col TblColPtr&
/// \param colNo size_t
/// \param type TableColumn::ColumnType
/// \param convertSimilarTypes bool
/// \return bool
///
/////////////////////////////////////////////////
bool convert_if_needed(TblColPtr& col, size_t colNo, TableColumn::ColumnType type, bool convertSimilarTypes)
{
    if (!col || (!col->size() && col->m_type != type))
    {
        convert_if_empty(col, colNo, type);
        return true;
    }

    if (col->m_type == type)
        return true;

    bool isSimilar = (type == TableColumn::TYPE_CATEGORICAL && col->m_type == TableColumn::TYPE_STRING)
                      || (type == TableColumn::TYPE_STRING && col->m_type == TableColumn::TYPE_CATEGORICAL)
                      || (TableColumn::isValueType(type) && TableColumn::isValueType(col->m_type));

    if (!convertSimilarTypes && isSimilar)
        return true;

    TableColumn* convertedCol = col->convert(type);

    if (!convertedCol)
        return false;

    if (convertedCol != col.get())
        col.reset(convertedCol);

    return true;
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
    std::string sUnit = col->m_sUnit;

    if (!sHeadLine.length())
        sHeadLine = TableColumn::getDefaultColumnHead(colNo);

    switch (type)
    {
        case TableColumn::TYPE_STRING:
        {
            col.reset(new StringColumn);
            col->m_sHeadLine = sHeadLine;
            col->m_sUnit = sUnit;
            break;
        }
        case TableColumn::TYPE_DATETIME:
        {
            col.reset(new DateTimeColumn);
            col->m_sHeadLine = sHeadLine;
            col->m_sUnit = sUnit;
            break;
        }
        case TableColumn::TYPE_LOGICAL:
        {
            col.reset(new LogicalColumn);
            col->m_sHeadLine = sHeadLine;
            col->m_sUnit = sUnit;
            break;
        }
        case TableColumn::TYPE_CATEGORICAL:
        {
            col.reset(new CategoricalColumn);
            col->m_sHeadLine = sHeadLine;
            col->m_sUnit = sUnit;
            break;
        }
        default:
        {
            if (TableColumn::isValueType(type))
            {
                col.reset(createValueTypeColumn(type));
                col->m_sHeadLine = sHeadLine;
                col->m_sUnit = sUnit;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Converts the types of a mu::Value
/// instance into a single
/// TableColumn::ColumnType type.
///
/// \param val const mu::Value&
/// \return TableColumn::ColumnType
///
/////////////////////////////////////////////////
TableColumn::ColumnType to_column_type(const mu::Value& val)
{
    if (val.isCategory())
        return TableColumn::TYPE_CATEGORICAL;
    else if (val.isString())
        return TableColumn::TYPE_STRING;
    else if (val.isNumerical())
    {
            g_logger.info(val.getNum().getTypeAsString());
        switch (val.getNum().getType())
        {
            case mu::Numerical::LOGICAL:
                return TableColumn::TYPE_LOGICAL;
            case mu::Numerical::I8:
                return TableColumn::TYPE_VALUE_I8;
            case mu::Numerical::I16:
                return TableColumn::TYPE_VALUE_I16;
            case mu::Numerical::I32:
                return TableColumn::TYPE_VALUE_I32;
            case mu::Numerical::I64:
                return TableColumn::TYPE_VALUE_I64;
            case mu::Numerical::UI8:
                return TableColumn::TYPE_VALUE_UI8;
            case mu::Numerical::UI16:
                return TableColumn::TYPE_VALUE_UI16;
            case mu::Numerical::UI32:
                return TableColumn::TYPE_VALUE_UI32;
            case mu::Numerical::UI64:
                return TableColumn::TYPE_VALUE_UI64;
            case mu::Numerical::DATETIME:
                return TableColumn::TYPE_DATETIME;
            case mu::Numerical::F32:
                return TableColumn::TYPE_VALUE_F32;
            case mu::Numerical::F64:
                return TableColumn::TYPE_VALUE_F64;
            case mu::Numerical::CF32:
                return TableColumn::TYPE_VALUE_CF32;
            case mu::Numerical::CF64:
                return TableColumn::TYPE_VALUE_CF64;
        }
    }

    return TableColumn::TYPE_NONE;
}


/////////////////////////////////////////////////
/// \brief Converts the types of a mu::Array
/// instance into a single
/// TableColumn::ColumnType type.
///
/// \param arr const mu::Array&
/// \return TableColumn::ColumnType
///
/////////////////////////////////////////////////
TableColumn::ColumnType to_column_type(const mu::Array& arr)
{
    if (arr.getCommonType() == mu::TYPE_CATEGORY)
        return TableColumn::TYPE_CATEGORICAL;
    else if (arr.getCommonType() == mu::TYPE_STRING
             || arr.getCommonType() == mu::TYPE_MIXED)
        return TableColumn::TYPE_STRING;
    else if (arr.getCommonType() == TYPE_NUMERICAL)
    {
        switch (arr.getCommonNumericalType())
        {
            case mu::Numerical::LOGICAL:
                return TableColumn::TYPE_LOGICAL;
            case mu::Numerical::I8:
                return TableColumn::TYPE_VALUE_I8;
            case mu::Numerical::I16:
                return TableColumn::TYPE_VALUE_I16;
            case mu::Numerical::I32:
                return TableColumn::TYPE_VALUE_I32;
            case mu::Numerical::I64:
                return TableColumn::TYPE_VALUE_I64;
            case mu::Numerical::UI8:
                return TableColumn::TYPE_VALUE_UI8;
            case mu::Numerical::UI16:
                return TableColumn::TYPE_VALUE_UI16;
            case mu::Numerical::UI32:
                return TableColumn::TYPE_VALUE_UI32;
            case mu::Numerical::UI64:
                return TableColumn::TYPE_VALUE_UI64;
            case mu::Numerical::DATETIME:
                return TableColumn::TYPE_DATETIME;
            case mu::Numerical::F32:
                return TableColumn::TYPE_VALUE_F32;
            case mu::Numerical::F64:
                return TableColumn::TYPE_VALUE_F64;
            case mu::Numerical::CF32:
                return TableColumn::TYPE_VALUE_CF32;
            case mu::Numerical::CF64:
                return TableColumn::TYPE_VALUE_CF64;
        }
    }

    return TableColumn::TYPE_NONE;
}


/////////////////////////////////////////////////
/// \brief Create a value column with the passed
/// value type (if it is a value type).
///
/// \param type TableColumn::ColumnType
/// \param nSize size_t
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* createValueTypeColumn(TableColumn::ColumnType type, size_t nSize)
{
    switch (type)
    {
        case TableColumn::TYPE_VALUE_I8:
            return new I8ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_UI8:
            return new UI8ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_I16:
            return new I16ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_UI16:
            return new UI16ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_I32:
            return new I32ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_UI32:
            return new UI32ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_I64:
            return new I64ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_UI64:
            return new UI64ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_F32:
            return new F32ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_F64:
            return new F64ValueColumn(nSize);
        case TableColumn::TYPE_VALUE_CF32:
            return new CF32ValueColumn(nSize);
        //case TableColumn::TYPE_VALUE_CF64: // Reminder that this might change
        case TableColumn::TYPE_VALUE:
            return new ValueColumn(nSize);
    }

    return nullptr;
}


/////////////////////////////////////////////////
/// \brief This function returns the contents of
/// an existing value column (type) as a new
/// value column type, if the target type is an
/// existing value column type.
///
/// \param type TableColumn::ColumnType
/// \param current TableColumn*
/// \return TableColumn*
///
/////////////////////////////////////////////////
TableColumn* convertNumericType(TableColumn::ColumnType type, TableColumn* current)
{
    size_t nNumElements = current->getNumFilledElements();
    TableColumn* col = createValueTypeColumn(type, nNumElements);

    if (col)
    {
        col->setValue(VectorIndex(0, nNumElements-1), current->getValue(VectorIndex(0, nNumElements-1)));
        col->assignMetaData(current);
    }

    return col;
}

