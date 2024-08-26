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




    Numerical::Numerical(int8_t data) : m_type(Numerical::I8)
    {
        i64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(uint8_t data) : m_type(Numerical::UI8)
    {
        ui64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(int16_t data) : m_type(Numerical::I16)
    {
        i64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(uint16_t data) : m_type(Numerical::UI16)
    {
        ui64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(int32_t data) : m_type(Numerical::I32)
    {
        i64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(uint32_t data) : m_type(Numerical::UI32)
    {
        ui64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(int64_t data, Numerical::NumericalType type) : m_type(type)
    {
        i64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(uint64_t data, Numerical::NumericalType type) : m_type(type)
    {
        ui64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(float data) : m_type(Numerical::F32)
    {
        cf64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(double data) : m_type(Numerical::F64)
    {
        cf64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(bool data) : m_type(Numerical::LOGICAL)
    {
        i64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(const std::complex<float>& data) : m_type(Numerical::CF32)
    {
        cf64 = data;
        getTypeInfo();
    }

    Numerical::Numerical(const std::complex<double>& data, Numerical::NumericalType type) : m_type(type)
    {
        cf64 = data;

        if (type == Numerical::CF64 && data.imag() == 0.0)
            m_type = Numerical::F64;

        getTypeInfo();
    }

    Numerical::Numerical(const sys_time_point& time) : m_type(Numerical::DATETIME)
    {
        cf64 = to_double(time);
        getTypeInfo();
    }

    Numerical Numerical::autoType(const std::complex<double>& data)
    {
        if (::isInt(data))
        {
            /*if (data.real() >= 0)
            {
                if (data.real() <= UINT8_MAX)
                    return Numerical((uint8_t)data.real());

                if (data.real() <= UINT16_MAX)
                    return Numerical((uint16_t)data.real());

                if (data.real() <= UINT32_MAX)
                    return Numerical((uint32_t)data.real());

                return Numerical((uint64_t)data.real());
            }*/

            if (data.real() >= INT8_MIN && data.real() <= INT8_MAX)
                return Numerical((int8_t)data.real());

            if (data.real() >= INT16_MIN && data.real() <= INT16_MAX)
                return Numerical((int16_t)data.real());

            if (data.real() >= INT32_MIN && data.real() <= INT32_MAX)
                return Numerical((int32_t)data.real());

            return Numerical((int64_t)data.real());
        }

        if (data.imag() == 0.0)
            return Numerical(data.real());

        return Numerical(data);
    }

    void Numerical::getTypeInfo()
    {
        if (m_type == Numerical::LOGICAL)
        {
            m_bits = 8;
            m_flags = Numerical::TYPE_LOGICAL;
        }
        else if (m_type == Numerical::DATETIME)
        {
            m_bits = 64;
            m_flags = Numerical::TYPE_FLOAT | Numerical::TYPE_DATETIME;
        }
        else if (m_type <= Numerical::I64)
        {
            m_flags = Numerical::TYPE_INT;
            m_bits = 8 * (0x8 >> (Numerical::I64 - m_type));
        }
        else if (m_type <= Numerical::UI64)
        {
            m_flags = Numerical::TYPE_INT | Numerical::TYPE_UINT;
            m_bits = 8 * (0x8 >> (Numerical::UI64 - m_type));
        }
        else if (m_type <= Numerical::F64)
        {
            m_flags = Numerical::TYPE_FLOAT;
            m_bits = m_type == Numerical::F64 ? 64 : 32;
        }
        else
        {
            if (cf64.imag() != 0.0)
                m_flags = Numerical::TYPE_FLOAT | Numerical::TYPE_COMPLEX;
            else
                m_flags = Numerical::TYPE_FLOAT;

            m_bits = m_type == Numerical::CF64 ? 64 : 32;
        }
    }

    Numerical::NumericalType Numerical::getPromotion(const Numerical& other) const
    {
        uint8_t bits = std::max(m_bits, other.m_bits);
        uint8_t flags = std::max(m_flags, other.m_flags);

        if (flags & Numerical::TYPE_COMPLEX)
            return bits == 32 ? Numerical::CF32 : Numerical::CF64;

        if (flags & Numerical::TYPE_DATETIME)
            return Numerical::DATETIME;

        if (flags & Numerical::TYPE_FLOAT)
            return bits == 32 ? Numerical::F32 : Numerical::F64;

        if (flags & Numerical::TYPE_UINT)
            return Numerical::NumericalType(Numerical::UI8 + (bits-8)/8);

        if (flags & Numerical::TYPE_INT)
            return Numerical::NumericalType(Numerical::I8 + (bits-8)/8);

        return Numerical::LOGICAL;
    }

    Numerical::InternalType Numerical::getConversion(Numerical::NumericalType promotion) const
    {
        if (promotion <= Numerical::I64)
            return Numerical::INT;

        if (promotion <= Numerical::UI64)
            return Numerical::UINT;

        return Numerical::COMPLEX;
    }

    int64_t Numerical::asI64() const
    {
        if (m_type <= Numerical::I64)
            return i64;

        if (m_type <= Numerical::UI64)
            return ui64;

        return intCast(cf64);
    }

    uint64_t Numerical::asUI64() const
    {
        if (m_type <= Numerical::I64)
            return (uint64_t)i64;

        if (m_type <= Numerical::UI64)
            return ui64;

        return (uint64_t)intCast(cf64);
    }

    double Numerical::asF64() const
    {
        if (m_type <= Numerical::I64)
            return i64;

        if (m_type <= Numerical::UI64)
            return ui64;

        return cf64.real();
    }

    std::complex<double> Numerical::asCF64() const
    {
        if (m_type <= Numerical::I64)
            return i64;

        if (m_type <= Numerical::UI64)
            return ui64;

        return cf64;
    }

    Numerical Numerical::operator+(const Numerical& other) const
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() + other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() + other.asUI64(), promotion);

        return Numerical(asCF64() + other.asCF64(), promotion);
    }

    Numerical Numerical::operator-() const
    {
        if (m_type <= Numerical::I64)
            return Numerical(-i64, m_type);

        if (m_type <= Numerical::UI64)
            return Numerical(-ui64, (Numerical::NumericalType)(m_type-Numerical::I64));

        return Numerical(-cf64, m_type);
    }

    Numerical Numerical::operator-(const Numerical& other) const
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() - other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() - other.asUI64(), promotion);

        return Numerical(asCF64() - other.asCF64(), promotion);
    }

    Numerical Numerical::operator/(const Numerical& other) const
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() / other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() / other.asUI64(), promotion);

        return Numerical(asCF64() / other.asCF64(), promotion);
    }

    Numerical Numerical::operator*(const Numerical& other) const
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() * other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() * other.asUI64(), promotion);

        return Numerical(asCF64() * other.asCF64(), promotion);
    }

    Numerical& Numerical::operator+=(const Numerical& other)
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() + other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() + other.asUI64();
        else
            cf64 = asCF64() + other.asCF64();

        m_type = promotion;
        getTypeInfo();
        return *this;
    }

    Numerical& Numerical::operator-=(const Numerical& other)
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() - other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() - other.asUI64();
        else
            cf64 = asCF64() - other.asCF64();

        m_type = promotion;
        getTypeInfo();
        return *this;
    }

    Numerical& Numerical::operator/=(const Numerical& other)
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() / other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() / other.asUI64();
        else
            cf64 = asCF64() / other.asCF64();

        m_type = promotion;
        getTypeInfo();
        return *this;
    }

    Numerical& Numerical::operator*=(const Numerical& other)
    {
        Numerical::NumericalType promotion = getPromotion(other);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() * other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() * other.asUI64();
        else
            cf64 = asCF64() * other.asCF64();

        m_type = promotion;
        getTypeInfo();
        return *this;
    }

    Numerical Numerical::pow(const Numerical& exponent) const
    {
        Numerical::NumericalType promotion = getPromotion(exponent);

        if (exponent.isInt())
            return Numerical(intPower(asCF64(), exponent.asI64()), promotion);

        return Numerical(std::pow(asCF64(), exponent.asCF64()), promotion);
    }

    Numerical::operator bool() const
    {
        if (m_type <= Numerical::I64)
            return i64 != 0;

        if (m_type <= Numerical::UI64)
            return ui64 != 0;

        return cf64 != 0.0;
    }

    bool Numerical::operator!() const
    {
        return !bool(*this);
    }

    bool Numerical::operator==(const Numerical& other) const
    {
        Numerical::InternalType conversion = getConversion(getPromotion(other));

        if (conversion == Numerical::INT)
            return asI64() == other.asI64();

        if (conversion == Numerical::UINT)
            return asUI64() == other.asUI64();

        return asCF64() == other.asCF64();
    }

    bool Numerical::operator!=(const Numerical& other) const
    {
        return !operator==(other);
    }

    bool Numerical::operator<(const Numerical& other) const
    {
        Numerical::InternalType conversion = getConversion(getPromotion(other));

        if (conversion == Numerical::INT)
            return asI64() < other.asI64();

        if (conversion == Numerical::UINT)
            return asUI64() < other.asUI64();

        return asF64() < other.asF64();
    }

    bool Numerical::operator<=(const Numerical& other) const
    {
        return operator<(other) || operator==(other);
    }

    bool Numerical::operator>(const Numerical& other) const
    {
        return !operator<=(other);
    }

    bool Numerical::operator>=(const Numerical& other) const
    {
        return !operator<(other);
    }

    Numerical::NumericalType Numerical::getType() const
    {
        return m_type;
    }

    std::string Numerical::getTypeAsString() const
    {
        switch (m_type)
        {
            case Numerical::LOGICAL:
                return "logical";
            case Numerical::I8:
                return "value.i8";
            case Numerical::I16:
                return "value.i16";
            case Numerical::I32:
                return "value.i32";
            case Numerical::I64:
                return "value.i64";
            case Numerical::UI8:
                return "value.ui8";
            case Numerical::UI16:
                return "value.ui16";
            case Numerical::UI32:
                return "value.ui32";
            case Numerical::UI64:
                return "value.ui64";
            case Numerical::DATETIME:
                return "datetime";
            case Numerical::F32:
                return "value.f32";
            case Numerical::F64:
                return "value.f64";
            case Numerical::CF32:
                return "value.cf32";
        }

        return "value";
    }

    std::string Numerical::print(size_t digits) const
    {
        if (m_type == Numerical::LOGICAL)
            return toString((bool)i64);

        if (m_type == Numerical::DATETIME && !isnan(cf64))
            return toString(to_timePoint(cf64.real()), 0);

        if (m_type <= Numerical::I64)
            return toString(i64);

        if (m_type <= Numerical::UI64)
            return toString(ui64);

        return toString(cf64, digits > 0 ? digits : 7);
    }

    std::string Numerical::printVal(size_t digits) const
    {
        if (m_type == Numerical::LOGICAL)
            return toString((bool)i64);

        if (m_type == Numerical::DATETIME && !isnan(cf64))
            return toString(to_timePoint(cf64.real()), 0);

        if (m_type <= Numerical::I64)
            return toString(i64);

        if (m_type <= Numerical::UI64)
            return toString(ui64);

        // Is one of the components zero, then try to find an
        // integer optimisation
        if (cf64.imag() == 0.0)
        {
            if (fabs(rint(cf64.real()) - cf64.real()) < 1e-14 && fabs(cf64.real()) >= 1.0)
                return toString(intCast(cf64.real()));
        }
        else if (cf64.real() == 0.0)
        {
            if (fabs(rint(cf64.imag()) - cf64.imag()) < 1e-14 && fabs(cf64.imag()) >= 1.0)
                return toString(intCast(cf64.imag())) + "i";
        }

        // Otherwise do not optimize due to the fact that the
        // precision will get halved in this case
        return toString(cf64, digits > 0 ? digits : 7);
    }

    size_t Numerical::getBytes() const
    {
        return m_flags & Numerical::TYPE_COMPLEX ? m_bits / 4 : m_bits / 8;
    }

    bool Numerical::isInt() const
    {
        return m_flags & Numerical::TYPE_INT || ::isInt(asCF64());
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

    Value::Value(const std::complex<double>& value)
    {
        m_type = TYPE_NUMERICAL;
        m_data = new Numerical(value);
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
            throw ParserError(ecTYPE_NO_STR);

        if (m_type == TYPE_CATEGORY)
            return ((Category*)m_data)->val;

        return *(Numerical*)m_data;
    }

    std::complex<double> Value::as_cmplx() const
    {
        if (!isNumerical())
            return NAN;

        if (m_type == TYPE_CATEGORY)
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

        if (m_type == TYPE_CATEGORY)
            return operator=(getNum() + other.getNum());

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

        if (m_type == TYPE_CATEGORY)
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

        if (m_type == TYPE_CATEGORY)
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

        if (m_type == TYPE_CATEGORY)
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
            && ((isString() && getStr() == other.getStr()) || (isNumerical() && getNum() == other.getNum()));
    }

    Value Value::operator!=(const Value& other) const
    {
        return m_type != other.m_type
            || ((isString() && getStr() != other.getStr()) || (isNumerical() && getNum() != other.getNum()));
    }

    Value Value::operator<(const Value& other) const
    {
        return m_type == other.m_type
            && ((isString() && getStr() < other.getStr()) || (isNumerical() && getNum() < other.getNum()));
    }

    Value Value::operator<=(const Value& other) const
    {
        return operator<(other) || operator==(other);
    }

    Value Value::operator>(const Value& other) const
    {
       return m_type == other.m_type
            && ((isString() && getStr() > other.getStr()) || (isNumerical() && getNum() > other.getNum()));
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
        if (m_type == TYPE_CATEGORY)
            return toExternalString(getStr()) + ": " + toString(getNum().asI64());

        if (isString())
        {
            if (chrs > 0)
            {
                if (trunc)
                    return truncString(toExternalString(getStr()), chrs);

                return ellipsize(toExternalString(getStr()), chrs);
            }

            return toExternalString(getStr());
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
            case TYPE_CATEGORY:
            case TYPE_STRING:
                return getStr().length();
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
                return front().getNum().getTypeAsString();
            case TYPE_STRING:
                return "string";
            case TYPE_MIXED:
                return "cluster";
        }

        return "void";
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
        return intCast(front().getNum().asI64());
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



    Array apply(std::complex<double>(*func)(const std::complex<double>&), const Array& a)
    {
        Array ret;

        for (const auto& val : a)
        {
            ret.push_back(Numerical(func(val.getNum().asCF64())));
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

    Array apply(std::string(*func)(const std::string&), const Array& a)
    {
        Array ret;

        for (const auto& val : a)
        {
            ret.push_back(func(val.getStr()));
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
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64())));
        }

        return ret;
    }



    Array apply(Value(*func)(const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i)));
        }

        return ret;
    }

    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64())));
        }

        return ret;
    }



    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64(),
                                         a4.get(i).getNum().asCF64())));
        }

        return ret;
    }

    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i), a4.get(i)));
        }

        return ret;
    }



    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64(),
                                         a4.get(i).getNum().asCF64(),
                                         a5.get(i).getNum().asCF64())));
        }

        return ret;
    }

    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        Array ret;

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i), a4.get(i), a5.get(i)));
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
                vVals.push_back(arrs[e].get(i).getNum().asCF64());
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


