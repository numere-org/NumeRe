/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#include "muTypes.hpp"
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

    Numerical::Numerical(const std::complex<double>& data, Numerical::NumericalType type)
    {
        cf64 = data;

        if (type == Numerical::AUTO)
        {
            if (data.imag() == 0.0)
                m_type = Numerical::F64;
            else
                m_type = Numerical::CF64;
        }
        else
            m_type = type;

        getTypeInfo();
    }

    Numerical::Numerical(const sys_time_point& time) : m_type(Numerical::DATETIME)
    {
        cf64 = to_double(time);
        getTypeInfo();
    }

    Numerical Numerical::autoType(const std::complex<double>& data)
    {
        if (data.imag() == 0.0 && std::rint(data.real()) == data.real())
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

        if (flags & Numerical::TYPE_INT)
            return Numerical::NumericalType(Numerical::I8 + std::log2(bits)-3);

        if (flags & Numerical::TYPE_UINT)
            return Numerical::NumericalType(Numerical::UI8 + std::log2(bits)-3);

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
        if (m_type <= Numerical::I64)
            return autoType(i64 / other.asCF64());

        if (m_type <= Numerical::UI64)
            return autoType(ui64 / other.asCF64());

        return autoType(cf64 / other.asCF64());
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
        *this = operator/(other);
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
        if (exponent.isInt())
            return Numerical::autoType(intPower(asCF64(), exponent.asI64()));

        return Numerical::autoType(std::pow(asCF64(), exponent.asCF64()));
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

    Numerical Numerical::getPromotedType(const Numerical& other) const
    {
        Numerical::NumericalType promotion = getPromotion(other);

        return Numerical(asCF64().imag() != 0.0 ? asCF64() : other.asCF64(), promotion);
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

        return "value.cf64";
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


    bool Category::operator==(const Category& other) const
    {
        return val == other.val && name == other.name;
    }

    bool Category::operator!=(const Category& other) const
    {
        return !operator==(other);
    }

    bool Category::operator<(const Category& other) const
    {
        return val < other.val || name < other.name;
    }

    bool Category::operator<=(const Category& other) const
    {
        return operator<(other) || operator==(other);
    }

    bool Category::operator>(const Category& other) const
    {
        return !operator<=(other);
    }

    bool Category::operator>=(const Category& other) const
    {
        return !operator<(other);
    }

}

