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
#include "../utils/tools.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Custom implementation for the complex
    /// multiplication operator with a scalar
    /// optimization.
    ///
    /// \param __x const std::complex<double>&
    /// \param __y const std::complex<double>&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    inline std::complex<double> operator*(const std::complex<double>& __x, const std::complex<double>& __y)
    {
        if (__x.imag() == 0.0)
            return std::complex<double>(__y.real()*__x.real(), __y.imag()*__x.real());
        else if (__y.imag() == 0.0)
            return std::complex<double>(__x.real()*__y.real(), __x.imag()*__y.real());

        std::complex<double> __r = __x;
        __r *= __y;
        return __r;
    }


    /////////////////////////////////////////////////
    /// \brief Custom implementation for the complex
    /// division operator with a scalar optimization.
    ///
    /// \param __x const std::complex<double>&
    /// \param __y const std::complex<double>&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    inline std::complex<double> operator/(const std::complex<double>& __x, const std::complex<double>& __y)
    {
        if (__y.imag() == 0.0)
            return std::complex<double>(__x.real() / __y.real(), __x.imag() / __y.real());

        std::complex<double> __r = __x;
        __r /= __y;
        return __r;
    }




    Numerical::Numerical(double data) : val(data)
    { }

    Numerical::Numerical(const std::complex<double>& data) : val(data)
    { }

    Numerical Numerical::operator+(const Numerical& other) const
    {
        return val + other.val;
    }

    Numerical Numerical::operator-() const
    {
        return -val;
    }

    Numerical Numerical::operator-(const Numerical& other) const
    {
        return val - other.val;
    }

    Numerical Numerical::operator/(const Numerical& other) const
    {
        return val / other.val;
    }

    Numerical Numerical::operator*(const Numerical& other) const
    {
        return val * other.val;
    }

    Numerical& Numerical::operator+=(const Numerical& other)
    {
        val += other.val;
        return *this;
    }

    Numerical& Numerical::operator-=(const Numerical& other)
    {
        val -= other.val;
        return *this;
    }

    Numerical& Numerical::operator/=(const Numerical& other)
    {
        val /= other.val;
        return *this;
    }

    Numerical& Numerical::operator*=(const Numerical& other)
    {
        val *= other.val;
        return *this;
    }




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

    Value::Value(bool logical)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = logical;
    }

    Value::Value(int value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(unsigned int value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(size_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(int64_t value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(double value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(const std::complex<double>& value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical;
        static_cast<Numerical*>(m_data)->val = value;
    }

    Value::Value(const std::string& sData)
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
        if (isValid() && m_type != other.m_type)
            clear();

        m_type = other.m_type;

        switch (m_type)
        {
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
            case TYPE_NUMERICAL:
            {
                if (getNum().val.imag() != 0.0)
                    return "complex";

                return "double";
            }
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
            && ((isString() && getStr().length()) || (isNumerical() && !isnan(getNum().val)));
    }

    bool Value::isNumerical() const
    {
        return m_type == TYPE_NUMERICAL && m_data;
    }

    bool Value::isString() const
    {
        return m_type == TYPE_STRING && m_data;
    }

    std::string& Value::getStr()
    {
        if (!isString())
            throw std::runtime_error("Value does not contain a string type.");

        return *(std::string*)m_data;
    }

    const std::string& Value::getStr() const
    {
        if (isVoid())
            return m_defString;

        if (!isString())
            throw std::runtime_error("Value does not contain a string type.");

        return *(std::string*)m_data;
    }

    Numerical& Value::getNum()
    {
        if (!isNumerical())
            throw std::runtime_error("Value does not contain a numerical type.");

        return *(Numerical*)m_data;
    }

    const Numerical& Value::getNum() const
    {
        if (isVoid())
            return m_defVal;

        if (!isNumerical())
            throw std::runtime_error("Value does not contain a numerical type.");

        return *(Numerical*)m_data;
    }

    Value Value::operator+(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw std::runtime_error("Value types do not match.");

        if (common == TYPE_NUMERICAL)
            return getNum() + other.getNum();
        else if (common == TYPE_STRING)
            return getStr() + other.getStr();

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value Value::operator-() const
    {
        if (m_type == TYPE_NUMERICAL)
            return -getNum();

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value Value::operator-(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw std::runtime_error("Value types do not match.");

        if (common == TYPE_NUMERICAL)
            return getNum() - other.getNum();

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value Value::operator/(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
            throw std::runtime_error("Value types do not match.");

        if (common == TYPE_NUMERICAL && other.m_type == TYPE_NUMERICAL)
            return getNum() / other.getNum();
        else if (common == TYPE_NUMERICAL)
            return getNum(); // Do not divide by zero

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value Value::operator*(const Value& other) const
    {
        DataType common = detectCommonType(other);

        if (common == TYPE_MIXED)
        {
            // special allowed cases: str*int and int*str
            if (m_type == TYPE_NUMERICAL && isInt(getNum().val))
                return mu::Value(strRepeat(other.getStr(), intCast(getNum().val)));
            else if (other.m_type == TYPE_NUMERICAL && isInt(other.getNum().val))
                return mu::Value(strRepeat(getStr(), intCast(other.getNum().val)));

            throw std::runtime_error("Value types do not match.");
        }

        if (common == TYPE_NUMERICAL)
            return getNum() * other.getNum();

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value& Value::operator+=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(other);

        if (m_type != other.m_type)
            throw std::runtime_error("Value types do not match or index out of bounds.");

        if (isNumerical())
            getNum() += other.getNum();
        else if (isString())
            getStr() += other.getStr();
        else
            throw std::runtime_error("Value type invalid or operation not supported.");

        return *this;
    }

    Value& Value::operator-=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(-other);

        if (m_type != other.m_type)
            throw std::runtime_error("Value types do not match or index out of bounds.");

        if (isNumerical())
            getNum() -= other.getNum();
        else
            throw std::runtime_error("Value type invalid or operation not supported.");

        return *this;
    }

    Value& Value::operator/=(const Value& other)
    {
        if (m_type != other.m_type)
            throw std::runtime_error("Value types do not match or index out of bounds.");

        if (isNumerical())
            getNum() /= other.getNum();
        else
            throw std::runtime_error("Value type invalid or operation not supported.");

        return *this;
    }

    Value& Value::operator*=(const Value& other)
    {
        if (m_type == TYPE_VOID)
            return operator=(other);

        if (m_type != other.m_type)
        {
            // special allowed cases: str*=int and int*=str
            if (m_type == TYPE_NUMERICAL && isInt(getNum().val))
                return operator=(mu::Value(strRepeat(other.getStr(), intCast(getNum().val))));
            else if (other.m_type == TYPE_NUMERICAL && isInt(other.getNum().val))
                return operator=(mu::Value(strRepeat(getStr(), intCast(other.getNum().val))));

            throw std::runtime_error("Value types do not match or index out of bounds.");
        }

        if (isNumerical())
            getNum() *= other.getNum();
        else
            throw std::runtime_error("Value type invalid or operation not supported.");

        return *this;
    }

    Value Value::pow(const Value& exponent) const
    {
        DataType common = detectCommonType(exponent);

        if (common == TYPE_MIXED)
            throw std::runtime_error("Value types do not match.");

        if (common == TYPE_NUMERICAL)
        {
            if (isInt(exponent.getNum().val))
                return Numerical(intPower(getNum().val, intCast(exponent.getNum().val)));

            return Numerical(std::pow(getNum().val, exponent.getNum().val));
        }

        throw std::runtime_error("Value type invalid or operation not supported.");
    }

    Value::operator bool() const
    {
        return isValid() && ((isString() && getStr().length()) || (isNumerical() && getNum().val != 0.0));
    }

    Value Value::operator!() const
    {
        return !bool(*this);
    }

    Value Value::operator==(const Value& other) const
    {
        return m_type == other.m_type
            && ((isString() && getStr() == other.getStr()) || (isNumerical() && getNum().val == other.getNum().val));
    }

    Value Value::operator!=(const Value& other) const
    {
        return !operator==(other);
    }

    Value Value::operator<(const Value& other) const
    {
        return m_type == other.m_type
            && ((isString() && getStr() < other.getStr()) || (isNumerical() && getNum().val.real() < other.getNum().val.real()));
    }

    Value Value::operator<=(const Value& other) const
    {
        return operator<(other) || operator==(other);
    }

    Value Value::operator>(const Value& other) const
    {
        return !operator<=(other);
    }

    Value Value::operator>=(const Value& other) const
    {
        return !operator<(other);
    }

    Value Value::operator&&(const Value& other) const
    {
        return bool(*this) && bool(other);
    }

    Value Value::operator||(const Value& other) const
    {
        return bool(*this) || bool(other);
    }

    std::string Value::print() const
    {
        if (isNumerical())
            return ::toString(getNum().val, 7);

        if (isString())
            return toExternalString(getStr());

        return "void";
    }

    void Value::clear()
    {
        switch (m_type)
        {
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

    DataType Value::detectCommonType(const Value& other) const
    {
        if (m_type == other.m_type || other.m_type == TYPE_VOID)
            return m_type;

        if (m_type == TYPE_VOID)
            return other.m_type;

        return TYPE_MIXED;
    }




    /*
    Variable::Variable() : Value()
    { }

    Variable::Variable(const Value& data) : Value(data)
    { }

    Variable::Variable(const Variable& data) : Value(data)
    { }

    Variable::Variable(Value&& data) : Value(data)
    { }

    Variable::Variable(Variable&& data) : Value(data)
    { }

    Variable::~Variable()
    {
        clear();
    }

    Variable& Variable::operator=(const Value& other)
    {
        if (!isValid() || m_type == other.getType())
        {
            switch (other.getType())
            {
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
            }

            return *this;
        }

        throw std::runtime_error("Cannot assign a different type to an already initialized variable.");
    }


    Variable& Variable::operator=(const Variable& other)
    {
        if (!isValid() || m_type == other.getType())
        {
            switch (other.getType())
            {
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
            }

            return *this;
        }

        throw std::runtime_error("Cannot assign a different type to an already initialized variable.");
    }




    VarArray& VarArray::operator=(const Array& values)
    {
        if (values.isSingleton())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) = values.front();
            }

            return *this;
        }

        for (size_t i = 0; i < std::min(size(), values.size()); i++)
        {
            *operator[](i) = values[i];
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

        return "{" + ret + "}";
    }*/




    Array::Array() : std::vector<Value>(), m_commonType(TYPE_VOID)
    { }

    Array::Array(const Array& other) : std::vector<Value>(other), m_commonType(other.m_commonType)
    { }

    Array::Array(const Value& singleton) : std::vector<Value>({singleton}), m_commonType(singleton.getType())
    { }

    Array::Array(const Variable& var) : std::vector<Value>(var), m_commonType(var.m_commonType)
    { }

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

    bool Array::isScalar() const
    {
        return size() == 1u;
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

    Array::operator bool() const
    {
        bool ret = true;

        for (size_t i = 0; i < size(); i++)
        {
            ret = ret && bool(operator[](i));
        }

        return ret;
    }

    Array Array::operator!() const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(!operator[](i));
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

    std::vector<std::string> Array::to_string() const
    {
        std::vector<std::string> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).print());
        }

        return ret;
    }

    std::string Array::print() const
    {
        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += operator[](i).print();
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
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
#warning FIXME (numere#1#07/04/24): void-initialized fails for most operations. Is that reasonable?
            return m_default;

        return operator[](i);
    }

    void Array::zerosToVoid()
    {
        if (getCommonType() != TYPE_NUMERICAL)
            return;

        for (size_t i = 0; i < size(); i++)
        {
            if (operator[](i).getNum().val == 0.0)
                operator[](i).clear();
        }

        m_commonType = TYPE_VOID;
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

        throw std::runtime_error("Cannot assign a different type to an already initialized variable.");
    }

    Variable& Variable::operator=(const Array& other)
    {
        if (getCommonType() == TYPE_VOID || (getCommonType() == other.getCommonType() && getCommonType() != TYPE_MIXED))
        {
            Array::operator=(other);
            return *this;
        }

        throw std::runtime_error("Cannot assign a different type to an already initialized variable.");
    }

    Variable& Variable::operator=(const Variable& other)
    {
        if (getCommonType() == TYPE_VOID || getType() == other.getType())
        {
            Array::operator=(other);
            return *this;
        }

        throw std::runtime_error("Cannot assign a different type to an already initialized variable.");
    }

    void Variable::overwrite(const Array& other)
    {
        Array::operator=(other);
    }



    VarArray& VarArray::operator=(const Array& values)
    {
        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) = values.front();
            }

            return *this;
        }

        for (size_t i = 0; i < std::min(size(), values.size()); i++)
        {
            *operator[](i) = values[i];
        }

        return *this;
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

        return "{" + ret + "}";
    }



    const std::string Value::m_defString;
    const Numerical Value::m_defVal;
    const Value Array::m_default;


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



    Array apply(std::complex<double>(*func)(const std::complex<double>&), const Array& a)
    {
        Array ret;

        for (const auto& val : a)
        {
            if (val.isNumerical())
                ret.push_back(Numerical(func(val.getNum().val)));
            else
                ret.push_back(Numerical(NAN));
        }

        return ret;
    }

    Array apply(Value(*func)(const Value&), const Array& a)
    {
        Array ret;

        for (const auto& val : a)
        {
            ret.push_back(func(val));
        }

        return ret;
    }

    Array apply(Value(*func)(const Value&, const Value&), const Array& a1, const Array& a2)
    {
        Array ret;

        for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i)));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2)
    {
        Array ret;

        for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
        {
            if (a1.get(i).isNumerical()
                && a2.get(i).isNumerical())
                ret.push_back(Numerical(func(a1.get(i).getNum().val,
                                             a2.get(i).getNum().val)));
            else
                ret.push_back(Numerical(NAN));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
        {
            if (a1.get(i).isNumerical()
                && a2.get(i).isNumerical()
                && a3.get(i).isNumerical())
                ret.push_back(Numerical(func(a1.get(i).getNum().val,
                                             a2.get(i).getNum().val,
                                             a3.get(i).getNum().val)));
            else
                ret.push_back(Numerical(NAN));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size()}); i++)
        {
            if (a1.get(i).isNumerical()
                && a2.get(i).isNumerical()
                && a3.get(i).isNumerical()
                && a4.get(i).isNumerical())
                ret.push_back(Numerical(func(a1.get(i).getNum().val,
                                             a2.get(i).getNum().val,
                                             a3.get(i).getNum().val,
                                             a4.get(i).getNum().val)));
            else
                ret.push_back(Numerical(NAN));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}); i++)
        {
            if (a1.get(i).isNumerical()
                && a2.get(i).isNumerical()
                && a3.get(i).isNumerical()
                && a4.get(i).isNumerical()
                && a5.get(i).isNumerical())
                ret.push_back(Numerical(func(a1.get(i).getNum().val,
                                             a2.get(i).getNum().val,
                                             a3.get(i).getNum().val,
                                             a4.get(i).getNum().val,
                                             a5.get(i).getNum().val)));
            else
                ret.push_back(Numerical(NAN));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>*, int), const Array* arrs, int elems)
    {
        size_t nCount = 0;

        for (int i = 0; i < elems; i++)
        {
            nCount = std::max(nCount, arrs[i].size());
        }

        mu::Array res;

        for (size_t i = 0; i < nCount; i++)
        {
            std::vector<std::complex<double>> vVals;

            for (int e = 0; e < elems; e++)
            {
                vVals.push_back(arrs[e].get(i).getNum().val);
            }

            res.push_back(Numerical(func(&vVals[0], elems)));
        }

        return res;
    }

    Array apply(Value(*func)(const Value*, int), const Array* arrs, int elems)
    {
        size_t nCount = 0;

        for (int i = 0; i < elems; i++)
        {
            nCount = std::max(nCount, arrs[i].size());
        }

        mu::Array res;

        for (size_t i = 0; i < nCount; i++)
        {
            std::vector<Value> vVals;

            for (int e = 0; e < elems; e++)
            {
                vVals.push_back(arrs[e].get(i));
            }

            res.push_back(func(&vVals[0], elems));
        }

        return res;
    }

}



void test()
{
    std::vector<mu::Numerical> v1({1,2,3,4,5});
    std::vector<mu::Numerical> v2({2,3,4,5,6});
    std::vector<std::string> s1({"2","3","4","5","6"});
    std::vector<std::string> s2({"2","3","4","5","6"});

    mu::Array a1(v1);
    mu::Array a2(v2);
    mu::Array res = a1+a2;

    mu::Array sa1(s1);
    mu::Array sa2(s2);
    mu::Array sres = sa1 + sa2;

    mu::Array dyn;
    dyn.push_back(1);
    dyn.push_back("2");
    dyn.push_back(3);
    dyn.push_back("4");

    mu::Array dynres = dyn + dyn;

    mu::Variable var(v1.front());
    var = v2.front();
    mu::Variable var2(var);
    var2 = v1.front();
    var2 = var;
}


