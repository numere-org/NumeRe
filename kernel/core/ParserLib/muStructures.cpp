/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024 Erik Haenel et al.

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

#include "muStructures.hpp"
#include "muParserError.h"
#include "../utils/tools.hpp"
#include "../strings/functionimplementation.hpp" // for string method callees
#include "../maths/functionimplementation.hpp" // for numerical method callees

namespace mu
{
    Value::Value()
    {
        m_type = TYPE_VOID;
        m_data = nullptr;
    }

    Value::Value(const Value& data)
    {
        m_type = data.m_type;

        switch (m_type)
        {
            case TYPE_CATEGORY:
                m_data = new Category;
                *static_cast<Category*>(m_data) = *static_cast<Category*>(data.m_data);
                break;
            case TYPE_NUMERICAL:
                m_data = new Numerical;
                *static_cast<Numerical*>(m_data) = data.getNum();
                break;
            case TYPE_STRING:
                m_data = new std::string;
                *static_cast<std::string*>(m_data) = data.getStr();
                break;
            default:
                m_data = nullptr;
        }
    }

    Value::Value(Value&& data)
    {
        m_type = data.m_type;
        m_data = data.m_data;
        data.m_data = nullptr;
    }

    Value::Value(const Numerical& data)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        *static_cast<Numerical*>(m_data) = data;
    }

    Value::Value(const Category& data)
    {
        m_type = TYPE_CATEGORY;
        m_data = new Category;
        *static_cast<Category*>(m_data) = data;
    }

    Value::Value(bool logical)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(logical);
    }

    Value::Value(int32_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(uint32_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(int64_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(uint64_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(double value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(const sys_time_point& value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(const std::complex<double>& value, bool autoType)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value, autoType ? Numerical::AUTO : Numerical::CF64);
    }

    Value::Value(const std::complex<float>& value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
    }

    Value::Value(const std::string& sData)
    {
        m_type = TYPE_STRING;
        m_data = new std::string;
        *static_cast<std::string*>(m_data) = sData;
    }

    Value::Value(const char* sData)
    {
        m_type = TYPE_STRING;
        m_data = new std::string;
        *static_cast<std::string*>(m_data) = sData;
    }

    Value::~Value()
    {
        clear();
    }

    Value& Value::operator=(const Value& other)
    {
        if (m_data && m_type != other.m_type)
            clear();

        m_type = other.m_type;

        switch (m_type)
        {
            case TYPE_CATEGORY:
                if (!m_data)
                    m_data = new Category;

                *static_cast<Category*>(m_data) = *static_cast<Category*>(other.m_data);
                break;
            case TYPE_NUMERICAL:
                if (!m_data)
                    m_data = new Numerical;

                getNum() = other.getNum();
                break;
            case TYPE_STRING:
                if (!m_data)
                    m_data = new std::string;
                getStr() = other.getStr();
                break;
            default:
                clear();
        }

        return *this;
    }

    DataType Value::getType() const
    {
        return m_type;
    }

    std::string Value::getTypeAsString() const
    {
        switch (m_type)
        {
            case TYPE_CATEGORY:
                return "category";
            case TYPE_NUMERICAL:
                return getNum().getTypeAsString();
            case TYPE_STRING:
                return "string";
        }

        return "void";
    }

    bool Value::isVoid() const
    {
        return m_type == TYPE_VOID;
    }

    bool Value::isValid() const
    {
        return m_type != TYPE_VOID
            && m_data
            && ((isString() && getStr().length()) || (isNumerical() && getNum().asCF64() == getNum().asCF64()));
    }

    bool Value::isNumerical() const
    {
        return (m_type == TYPE_NUMERICAL || m_type == TYPE_CATEGORY) && m_data;
    }

    bool Value::isString() const
    {
        return (m_type == TYPE_STRING || m_type == TYPE_CATEGORY) && m_data;
    }

    bool Value::isCategory() const
    {
        return m_type == TYPE_CATEGORY && m_data;
    }

    std::string& Value::getStr()
    {
        if (!isString())
            throw ParserError(ecTYPE_NO_STR);

        if (m_type == TYPE_CATEGORY)
            return ((Category*)m_data)->name;

        return *(std::string*)m_data;
    }

    const std::string& Value::getStr() const
    {
        if (isVoid())
            return m_defString;

        if (!isString())
            throw ParserError(ecTYPE_NO_STR);

        if (m_type == TYPE_CATEGORY)
            return ((Category*)m_data)->name;

        return *(std::string*)m_data;
    }

    Numerical& Value::getNum()
    {
        if (!isNumerical())
            throw ParserError(ecTYPE_NO_VAL);

        if (m_type == TYPE_CATEGORY)
            return ((Category*)m_data)->val;

        return *(Numerical*)m_data;
    }

    const Numerical& Value::getNum() const
    {
        if (isVoid())
            return m_defVal;

        if (!isNumerical())
            throw ParserError(ecTYPE_NO_VAL);

        if (m_type == TYPE_CATEGORY)
            return ((Category*)m_data)->val;

        return *(Numerical*)m_data;
    }

    Category& Value::getCategory()
    {
        if (!isCategory())
            throw ParserError(ecTYPE_NO_VAL);

        return *(Category*)m_data;
    }

    const Category& Value::getCategory() const
    {
        if (!isCategory())
            throw ParserError(ecTYPE_NO_VAL);

        return *(Category*)m_data;
    }

    std::complex<double> Value::as_cmplx() const
    {
        if (!isNumerical())
            return NAN;

        if (isCategory())
            return ((Category*)m_data)->val.asCF64();

        return static_cast<Numerical*>(m_data)->asCF64();
    }

    Value Value::operator+(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH);

        if (common == TYPE_NUMERICAL)
            return getNum() + other.getNum();
        else if (common == TYPE_STRING)
            return getStr() + other.getStr();

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value Value::operator-() const
    {
        if (m_type == TYPE_NUMERICAL || m_type == TYPE_CATEGORY)
            return -getNum();

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value Value::operator-(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH);

        if (common == TYPE_NUMERICAL)
            return getNum() - other.getNum();

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value Value::operator/(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH);

        if (common == TYPE_NUMERICAL && other.m_type == TYPE_NUMERICAL)
            return getNum() / other.getNum();
        else if (common == TYPE_NUMERICAL)
            return getNum(); // Do not divide by zero

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value Value::operator*(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
        {
            // special allowed cases: str*int and int*str
            if (m_type == TYPE_NUMERICAL && getNum().isInt())
                return mu::Value(strRepeat(other.getStr(), getNum().asUI64()));
            else if (other.m_type == TYPE_NUMERICAL && other.getNum().isInt())
                return mu::Value(strRepeat(getStr(), other.getNum().asUI64()));

            throw ParserError(ecTYPE_MISMATCH);
        }

        if (common == TYPE_NUMERICAL)
            return getNum() * other.getNum();

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value& Value::operator+=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(other);

        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH_OOB);

        if (isCategory())
        {
            if (common == TYPE_STRING)
                return operator=(getStr() + other.getStr());

            return operator=(getNum() + other.getNum());
        }

        if (isNumerical())
            getNum() += other.getNum();
        else if (isString())
            getStr() += other.getStr();
        else
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    Value& Value::operator-=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(-other);

        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH_OOB);

        if (isCategory())
            return operator=(getNum() - other.getNum());

        if (isNumerical())
            getNum() -= other.getNum();
        else
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    Value& Value::operator/=(const Value& other)
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH_OOB);

        if (isCategory())
            return operator=(getNum() / other.getNum());

        if (isNumerical())
            getNum() /= other.getNum();
        else
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    Value& Value::operator*=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(other);

        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
        {
            // special allowed cases: str*=int and int*=str
            if (m_type == TYPE_NUMERICAL && getNum().isInt())
                return operator=(mu::Value(strRepeat(other.getStr(), getNum().asUI64())));
            else if (other.m_type == TYPE_NUMERICAL && other.getNum().isInt())
                return operator=(mu::Value(strRepeat(getStr(), other.getNum().asUI64())));

            throw ParserError(ecTYPE_MISMATCH_OOB);
        }

        if (isCategory())
            return operator=(getNum() * other.getNum());

        if (isNumerical())
            getNum() *= other.getNum();
        else
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    Value Value::pow(const Value& exponent) const
    {
        DataType common = detectCommonType(exponent);

        if (common == TYPE_MIXED)
            throw ParserError(ecTYPE_MISMATCH);

        if (common == TYPE_NUMERICAL)
            return getNum().pow(exponent.getNum());

        throw ParserError(ecTYPE_MISMATCH);
    }

    Value::operator bool() const
    {
        return isValid() && ((isString() && getStr().length()) || (isNumerical() && bool(getNum())));
    }

    Value Value::operator!() const
    {
        return !isValid() || ((isString() && !getStr().length()) || (isNumerical() && !getNum()));
    }

    Value Value::operator==(const Value& other) const
    {
        return m_type == other.m_type
            && ((isCategory() && getCategory() == other.getCategory())
                || (isString() && getStr() == other.getStr())
                || (isNumerical() && getNum() == other.getNum()));
    }

    Value Value::operator!=(const Value& other) const
    {
        return m_type != other.m_type
            || (isCategory() && getCategory() != other.getCategory())
            || (isString() && getStr() != other.getStr())
            || (isNumerical() && getNum() != other.getNum());
    }

    Value Value::operator<(const Value& other) const
    {
        return m_type == other.m_type
            && ((isCategory() && getCategory() < other.getCategory())
                || (isString() && getStr() < other.getStr())
                || (isNumerical() && getNum() < other.getNum()));
    }

    Value Value::operator<=(const Value& other) const
    {
        return operator<(other) || operator==(other);
    }

    Value Value::operator>(const Value& other) const
    {
       return m_type == other.m_type
            && ((isCategory() && getCategory() > other.getCategory())
                || (isString() && getStr() > other.getStr())
                || (isNumerical() && getNum() > other.getNum()));
    }

    Value Value::operator>=(const Value& other) const
    {
        return operator>(other) || operator==(other);
    }

    Value Value::operator&&(const Value& other) const
    {
        return bool(*this) && bool(other);
    }

    Value Value::operator||(const Value& other) const
    {
        return bool(*this) || bool(other);
    }

    std::string Value::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (isCategory())
            return toExternalString(getStr()) + ": " + toString(getNum().asI64());

        if (isString())
        {
            if (chrs > 0)
            {
                if (trunc)
                    return truncString(toExternalString(replaceControlCharacters(getStr())), chrs);

                return ellipsize(toExternalString(replaceControlCharacters(getStr())), chrs);
            }

            return toExternalString(replaceControlCharacters(getStr()));
        }

        if (isNumerical())
            return getNum().print(digits);

        return "void";
    }

    std::string Value::printVal(size_t digits, size_t chrs) const
    {
        if (isString())
        {
            if (chrs > 0)
                return ellipsize(getStr(), chrs);

            return getStr();
        }

        if (isNumerical())
            return getNum().printVal(digits);

        return "void";
    }

    void Value::clear()
    {
        switch (m_type)
        {
            case TYPE_CATEGORY:
                delete static_cast<Category*>(m_data);
                break;
            case TYPE_NUMERICAL:
                delete static_cast<Numerical*>(m_data);
                break;
            case TYPE_STRING:
                delete static_cast<std::string*>(m_data);
                break;
        }

        m_type = TYPE_VOID;
        m_data = nullptr;
    }

    size_t Value::getBytes() const
    {
        switch (m_type)
        {
            case TYPE_STRING:
                return getStr().length();
            case TYPE_CATEGORY:
            case TYPE_NUMERICAL:
                return getNum().getBytes();
        }

        return 0;
    }

    DataType Value::detectCommonType(const Value& other) const
    {
        if ((m_type == TYPE_CATEGORY || m_type == TYPE_NUMERICAL)
            && (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_CATEGORY))
            return TYPE_NUMERICAL;

        if ((m_type == TYPE_CATEGORY || m_type == TYPE_STRING)
            && (other.m_type == TYPE_STRING || other.m_type == TYPE_CATEGORY))
            return TYPE_STRING;

        if (m_type == other.m_type || other.m_type == TYPE_VOID)
            return m_type;

        if (m_type == TYPE_VOID)
            return other.m_type;

        return TYPE_MIXED;
    }




    Array::Array() : std::vector<Value>(), m_commonType(TYPE_VOID)
    { }

    Array::Array(const Array& other) : std::vector<Value>(other), m_commonType(other.m_commonType)
    { }

    Array::Array(size_t n, const Value& fillVal) : std::vector<Value>(n, fillVal), m_commonType(fillVal.getType())
    { }

    Array::Array(const Value& singleton) : std::vector<Value>({singleton}), m_commonType(singleton.getType())
    { }

    Array::Array(const Variable& var) : std::vector<Value>(var), m_commonType(var.m_commonType)
    { }

    Array::Array(const std::vector<std::complex<double>>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }

    Array::Array(const std::vector<Numerical>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }

    Array::Array(const std::vector<std::string>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_STRING)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }

    Array& Array::operator=(const Array& other)
    {
        std::vector<Value>::operator=(other);
        m_commonType = other.m_commonType;

        return *this;
    }

    std::vector<DataType> Array::getType() const
    {
        std::vector<DataType> types;

        for (size_t i = 0; i < size(); i++)
        {
            types.push_back(operator[](i).getType());

            if (m_commonType == TYPE_VOID)
                m_commonType = types.back();

            if (m_commonType != types.back())
                m_commonType = TYPE_MIXED;
        }

        return types;
    }

    DataType Array::getCommonType() const
    {
        if (m_commonType == TYPE_VOID && size())
            getType();

        return m_commonType;
    }

    std::string Array::getCommonTypeAsString() const
    {
        switch (getCommonType())
        {
            case TYPE_CATEGORY:
                return "category";
            case TYPE_NUMERICAL:
            {
                Numerical val = front().getNum();

                for (size_t i = 1; i < size(); i++)
                {
                    val = operator[](i).getNum().getPromotedType(val);
                }

                return val.getTypeAsString();
            }
            case TYPE_STRING:
                return "string";
            case TYPE_MIXED:
                return "record";
        }

        return "void";
    }


    Numerical::NumericalType Array::getCommonNumericalType() const
    {
        Numerical val = front().getNum();

        for (size_t i = 1; i < size(); i++)
        {
            val = operator[](i).getNum().getPromotedType(val);
        }

        return val.getType();
    }


    bool Array::isScalar() const
    {
        return size() == 1u;
    }

    bool Array::isDefault() const
    {
        return !size();
    }

    Array Array::operator+(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) + other.get(i));
        }

        return ret;
    }

    Array Array::operator-() const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(-get(i));
        }

        return ret;
    }

    Array Array::operator-(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) - other.get(i));
        }

        return ret;
    }

    Array Array::operator/(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) / other.get(i));
        }

        return ret;
    }

    Array Array::operator*(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) * other.get(i));
        }

        return ret;
    }

    Array& Array::operator+=(const Array& other)
    {
        if (size() < other.size())
            operator=(operator+(other));
        else
        {
            for (size_t i = 0; i < size(); i++)
            {
                operator[](i) += other.get(i);
            }
        }

        return *this;
    }

    Array& Array::operator-=(const Array& other)
    {
        if (size() < other.size())
            operator=(operator-(other));
        else
        {
            for (size_t i = 0; i < size(); i++)
            {
                operator[](i) -= other.get(i);
            }
        }

        return *this;
    }

    Array& Array::operator/=(const Array& other)
    {
        if (size() < other.size())
            operator=(operator/(other));
        else
        {
            for (size_t i = 0; i < size(); i++)
            {
                operator[](i) /= other.get(i);
            }
        }

        return *this;
    }

    Array& Array::operator*=(const Array& other)
    {
        if (size() < other.size())
            operator=(operator*(other));
        else
        {
            for (size_t i = 0; i < size(); i++)
            {
                operator[](i) *= other.get(i);
            }
        }

        return *this;
    }

    Array Array::pow(const Array& exponent) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), exponent.size()); i++)
        {
            ret.push_back(get(i).pow(exponent.get(i)));
        }

        return ret;
    }

    Array Array::pow(const Numerical& exponent) const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(get(i).pow(exponent));
        }

        return ret;
    }

    Array Array::operator!() const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).operator!());
        }

        return ret;
    }

    Array Array::operator==(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) == other.get(i));
        }

        return ret;
    }

    Array Array::operator!=(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) != other.get(i));
        }

        return ret;
    }

    Array Array::operator<(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) < other.get(i));
        }

        return ret;
    }

    Array Array::operator<=(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) <= other.get(i));
        }

        return ret;
    }

    Array Array::operator>(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) > other.get(i));
        }

        return ret;
    }

    Array Array::operator>=(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) >= other.get(i));
        }

        return ret;
    }

    Array Array::operator&&(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) && other.get(i));
        }

        return ret;
    }

    Array Array::operator||(const Array& other) const
    {
        Array ret;

        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            ret.push_back(get(i) || other.get(i));
        }

        return ret;
    }

    Array Array::call(const std::string& sMethod) const
    {
        if (sMethod == "len")
            return strfnc_strlen(*this);
        else if (sMethod == "first")
            return strfnc_firstch(*this);
        else if (sMethod == "last")
            return strfnc_lastch(*this);
        else if (sMethod == "std")
            return numfnc_Std(this, 1); // Pointer as single-element array
        else if (sMethod == "avg")
            return numfnc_Avg(this, 1);
        else if (sMethod == "prd")
            return numfnc_product(this, 1);
        else if (sMethod == "sum")
            return numfnc_Sum(this, 1);
        else if (sMethod == "min")
            return numfnc_Min(this, 1);
        else if (sMethod == "max")
            return numfnc_Max(this, 1);
        else if (sMethod == "norm")
            return numfnc_Norm(this, 1);
        else if (sMethod == "num")
            return numfnc_Num(this, 1);
        else if (sMethod == "cnt")
            return numfnc_Cnt(this, 1);
        else if (sMethod == "med")
            return numfnc_Med(this, 1);
        else if (sMethod == "and")
            return numfnc_and(this, 1);
        else if (sMethod == "or")
            return numfnc_or(this, 1);
        else if (sMethod == "xor")
            return numfnc_xor(this, 1);
        else if (sMethod == "size")
            return numfnc_Cnt(this, 1);
        else if (sMethod == "maxpos")
            return numfnc_MaxPos(*this);
        else if (sMethod == "minpos")
            return numfnc_MinPos(*this);
        else if (sMethod == "order")
            return numfnc_order(this, 1);

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }

    Array Array::call(const std::string& sMethod, const Array& arg1) const
    {
        if (sMethod == "at")
            return strfnc_char(*this, arg1);
        else if (sMethod == "startsw")
            return strfnc_startswith(*this, arg1);
        else if (sMethod == "endsw")
            return strfnc_endswith(*this, arg1);
        else if (sMethod == "sel")
            return numfnc_getElements(*this, arg1);
        else if (sMethod == "sub")
            return strfnc_substr(*this, arg1, mu::Array());
        else if (sMethod == "splt")
            return strfnc_split(*this, arg1, mu::Array());
        else if (sMethod == "fnd")
            return strfnc_strfnd(arg1, *this, mu::Array());
        else if (sMethod == "rfnd")
            return strfnc_strrfnd(arg1, *this, mu::Array());
        else if (sMethod == "mtch")
            return strfnc_strmatch(arg1, *this, mu::Array());
        else if (sMethod == "rmtch")
            return strfnc_strrmatch(arg1, *this, mu::Array());
        else if (sMethod == "nmtch")
            return strfnc_str_not_match(arg1, *this, mu::Array());
        else if (sMethod == "nrmtch")
            return strfnc_str_not_rmatch(arg1, *this, mu::Array());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }

    Array Array::call(const std::string& sMethod, const Array& arg1, const Array& arg2) const
    {
        if (sMethod == "sub")
            return strfnc_substr(*this, arg1, arg2);
        else if (sMethod == "splt")
            return strfnc_split(*this, arg1, arg2);
        else if (sMethod == "fnd")
            return strfnc_strfnd(arg1, *this, arg2);
        else if (sMethod == "rfnd")
            return strfnc_strrfnd(arg1, *this, arg2);
        else if (sMethod == "mtch")
            return strfnc_strmatch(arg1, *this, arg2);
        else if (sMethod == "rmtch")
            return strfnc_strrmatch(arg1, *this, arg2);
        else if (sMethod == "nmtch")
            return strfnc_str_not_match(arg1, *this, arg2);
        else if (sMethod == "nrmtch")
            return strfnc_str_not_rmatch(arg1, *this, arg2);

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    int64_t Array::getAsScalarInt() const
    {
        return front().getNum().asI64();
    }

    std::vector<std::string> Array::as_str_vector() const
    {
        std::vector<std::string> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).getStr());
        }

        return ret;
    }

    std::vector<std::complex<double>> Array::as_cmplx_vector() const
    {
        std::vector<std::complex<double>> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).as_cmplx());
        }

        return ret;
    }

    std::vector<std::string> Array::to_string() const
    {
        std::vector<std::string> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).print());
        }

        return ret;
    }

    std::string Array::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (isDefault())
            return "DEFVAL";

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += operator[](i).print(digits, chrs, trunc);
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
    }

    std::string Array::printVals(size_t digits, size_t chrs) const
    {
        if (isDefault())
            return "void";

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += operator[](i).printVal(digits, chrs);
        }

        return ret;
    }

    std::string Array::printDims() const
    {
        return toString(size()) + " x 1";
    }

    size_t Array::getBytes() const
    {
        size_t bytes = 0;

        for (size_t i = 0; i < size(); i++)
        {
            bytes += operator[](i).getBytes();
        }

        return bytes;
    }

    Value& Array::get(size_t i)
    {
        if (size() == 1u)
            return front();
        else if (size() <= i)
            throw std::length_error("Element " + ::toString(i) + " is out of bounds.");

        return operator[](i);
    }

    const Value& Array::get(size_t i) const
    {
        if (size() == 1u)
            return front();
        else if (size() <= i)
            return m_default;

        return operator[](i);
    }

    void Array::zerosToVoid()
    {
        if (getCommonType() != TYPE_NUMERICAL)
            return;

        for (size_t i = 0; i < size(); i++)
        {
            if (operator[](i).getNum().asCF64() == 0.0)
                operator[](i).clear();
        }

        m_commonType = TYPE_VOID;
    }

    bool Array::isCommutative() const
    {
        return getCommonType() == TYPE_NUMERICAL;
    }




    Variable::Variable() : Array()
    { }

    Variable::Variable(const Value& data) : Array(data)
    { }

    Variable::Variable(const Array& data) : Array(data)
    { }

    Variable::Variable(const Variable& data) : Array(data)
    { }

    Variable::Variable(Array&& data) : Array(data)
    { }

    Variable& Variable::operator=(const Value& other)
    {
        if (getCommonType() == TYPE_VOID || getCommonType() == other.getType())
        {
            Array::operator=(Array(other));
            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }

    Variable& Variable::operator=(const Array& other)
    {
        if (getCommonType() == TYPE_VOID || (getCommonType() == other.getCommonType() && getCommonType() != TYPE_MIXED))
        {
            Array::operator=(other);
            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }

    Variable& Variable::operator=(const Variable& other)
    {
        if (getCommonType() == TYPE_VOID || getType() == other.getType())
        {
            Array::operator=(other);
            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }

    void Variable::overwrite(const Array& other)
    {
        Array::operator=(other);
    }


    VarArray::VarArray(Variable* var) : std::vector<Variable*>(1, var)
    {
        //
    }

    Array VarArray::operator=(const Array& values)
    {
        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) = values.front();
            }
        }
        else if (size() == 1)
            *front() = values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) = values[i];
            }
        }

        return values;
    }

    Array VarArray::operator+=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) += values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() += values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) += values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }

    Array VarArray::operator-=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) -= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() -= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) -= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }

    Array VarArray::operator*=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) *= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() *= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) *= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }

    Array VarArray::operator/=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) /= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() /= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) /= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }

    Array VarArray::pow(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                Array res = operator[](i)->pow(values.front());
                ret.insert(ret.end(), res.begin(), res.end());
            }
        }
        else if (size() == 1)
            ret = front()->pow(values);
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                Array res = operator[](i)->pow(values[i]);
                ret.insert(ret.end(), res.begin(), res.end());
            }
        }

        return ret;
    }

    VarArray& VarArray::operator=(const std::vector<Array>& arrayList)
    {
        if (arrayList.size() == 1u)
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) = arrayList.front();
            }

            return *this;
        }

        for (size_t i = 0; i < std::min(size(), arrayList.size()); i++)
        {
            *operator[](i) = arrayList[i];
        }

        return *this;
    }

    bool VarArray::operator==(const VarArray& other) const
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); i++)
        {
            if (operator[](i) != other[i])
                return false;
        }

        return true;
    }

    bool VarArray::isNull() const
    {
        return size() == 0u;
    }

    std::string VarArray::print() const
    {
        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.size())
                ret += ", ";

            ret += toHexString((size_t)operator[](i));
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
    }

    Array VarArray::asArray() const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
        }

        return ret;
    }



    const std::string Value::m_defString;
    const Numerical Value::m_defVal;
    const Value Array::m_default;


    bool all(const Array& arr)
    {
        for (size_t i = 0; i < arr.size(); i++)
        {
            if (!arr[i])
                return false;
        }

        return true;
    }

    bool any(const Array& arr)
    {
        for (size_t i = 0; i < arr.size(); i++)
        {
            if (arr[i])
                return true;
        }

        return false;
    }



    Array operator+(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a+v);
        }

        return ret;
    }

    Array operator-(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a-v);
        }

        return ret;
    }

    Array operator*(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a*v);
        }

        return ret;
    }

    Array operator/(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a/v);
        }

        return ret;
    }

    Array operator+(const Value& v, const Array& arr)
    {
        return arr+v;
    }

    Array operator-(const Value& v, const Array& arr)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(v-a);
        }

        return ret;
    }

    Array operator*(const Value& v, const Array& arr)
    {
        return arr*v;
    }

    Array operator/(const Value& v, const Array& arr)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(v/a);
        }

        return ret;
    }


    Array operator+(const Array& arr, double v)
    {
        return arr+Value(v);
    }

    Array operator-(const Array& arr, double v)
    {
        return arr-Value(v);
    }

    Array operator*(const Array& arr, double v)
    {
        return arr*Value(v);
    }

    Array operator/(const Array& arr, double v)
    {
        return arr/Value(v);
    }

    Array operator+(double v, const Array& arr)
    {
        return arr+Value(v);
    }

    Array operator-(double v, const Array& arr)
    {
        return Value(v)-arr;
    }

    Array operator*(double v, const Array& arr)
    {
        return arr*Value(v);
    }

    Array operator/(double v, const Array& arr)
    {
        return Value(v)/arr;
    }
}


