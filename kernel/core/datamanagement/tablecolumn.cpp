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
/// \brief Sets a string vector at the specified
/// indices.
///
/// \param idx const VectorIndex&
/// \param vValue const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void TableColumn::setValue(const VectorIndex& idx, const std::vector<std::string>& vValue)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= vValue.size())
            break;

        setValue(idx[i], vValue[i]);
    }
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
void TableColumn::setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= vValue.size())
            break;

        setValue(idx[i], vValue[i]);
    }
}


/////////////////////////////////////////////////
/// \brief Sets a plain numerical array at the
/// specified indices.
///
/// \param idx const VectorIndex&
/// \param _dData mu::value_type*
/// \param _nNum size_t
/// \return void
///
/////////////////////////////////////////////////
void TableColumn::setValue(const VectorIndex& idx, mu::value_type* _dData, size_t _nNum)
{
    for (size_t i = 0; i < idx.size(); i++)
    {
        if (i >= _nNum)
            break;

        setValue(idx[i], _dData[i]);
    }
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
/// \brief Shrink the column by removing all
/// invalid elements from the end.
///
/// \return void
///
/////////////////////////////////////////////////
void TableColumn::shrink()
{
    for (int i = size()-1; i >= 0; i--)
    {
        if (isValid(i))
        {
            resize(i+1);
            return;
        }
    }

    // If the code reaches this point, it is either empty
    // or full of invalid values
    resize(0);
}


/////////////////////////////////////////////////
/// \brief Return the number of actual filled
/// elements in this column, which can be
/// different from the actual size of the column.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t TableColumn::getNumFilledElements() const
{
    for (size_t i = size(); i > 0; i--)
    {
        if (isValid(i-1))
            return i;
    }

    return 0;
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


/////////////////////////////////////////////////
/// \brief Converts the passed ColumnType value
/// to a string representation.
///
/// \param type TableColumn::ColumnType
/// \return std::string
///
/////////////////////////////////////////////////
std::string TableColumn::typeToString(TableColumn::ColumnType type)
{
    switch (type)
    {
    case TYPE_NONE:
        return "none";
    case TYPE_VALUE_I8:
        return "value.i8";
    case TYPE_VALUE_UI8:
        return "value.ui8";
    case TYPE_VALUE_I16:
        return "value.i16";
    case TYPE_VALUE_UI16:
        return "value.ui16";
    case TYPE_VALUE_I32:
        return "value.i32";
    case TYPE_VALUE_UI32:
        return "value.ui32";
    case TYPE_VALUE_I64:
        return "value.i64";
    case TYPE_VALUE_UI64:
        return "value.ui64";
    case TYPE_VALUE_F32:
        return "value.f32";
    case TYPE_VALUE_F64:
        return "value.f64";
    case TYPE_VALUE_CF32:
        return "value.cf32";
    case TYPE_VALUE:
        return "value";
    case TYPE_STRING:
        return "string";
    case TYPE_DATETIME:
        return "datetime";
    case TYPE_LOGICAL:
        return "logical";
    case TYPE_CATEGORICAL:
        return "category";
    default:
        return "unknown";
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Converts the passed string
/// representation to a ColumnType value.
///
/// \param sType const std::string&
/// \return TableColumn::ColumnType
///
/////////////////////////////////////////////////
TableColumn::ColumnType TableColumn::stringToType(const std::string& sType)
{
    if (sType == "value" || sType == "value.cf64")
        return TYPE_VALUE;
    else if (sType == "value.cf32")
        return TYPE_VALUE_CF32;
    else if (sType == "value.f64")
        return TYPE_VALUE_F64;
    else if (sType == "value.f32")
        return TYPE_VALUE_F32;
    else if (sType == "value.i8")
        return TYPE_VALUE_I8;
    else if (sType == "value.ui8")
        return TYPE_VALUE_UI8;
    else if (sType == "value.i16")
        return TYPE_VALUE_I16;
    else if (sType == "value.ui16")
        return TYPE_VALUE_UI16;
    else if (sType == "value.i32")
        return TYPE_VALUE_I32;
    else if (sType == "value.ui32")
        return TYPE_VALUE_UI32;
    else if (sType == "value.i64")
        return TYPE_VALUE_I64;
    else if (sType == "value.ui64")
        return TYPE_VALUE_UI64;
    else if (sType == "string")
        return TYPE_STRING;
    else if (sType == "datetime")
        return TYPE_DATETIME;
    else if (sType == "logical")
        return TYPE_LOGICAL;
    else if (sType == "category")
        return TYPE_CATEGORICAL;
    else if (sType == "auto")
        return TYPE_MIXED;

    return TYPE_NONE;
}


/////////////////////////////////////////////////
/// \brief Returns a list of all available column
/// types as strings.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> TableColumn::getTypesAsString()
{
    return {"string", "datetime", "logical", "category",
        "value", "value.cf32", "value.f64", "value.f32",
        "value.i64", "value.ui64", "value.i32", "value.ui32",
        "value.i16", "value.ui16", "value.i8", "value.ui8"};
}


/////////////////////////////////////////////////
/// \brief Return, whether the passed type
/// represents a value type.
///
/// \param type TableColumn::ColumnType
/// \return bool
///
/////////////////////////////////////////////////
bool TableColumn::isValueType(TableColumn::ColumnType type)
{
    return type > TableColumn::VALUELIKE && type < TableColumn::VALUE_LAST;
}

