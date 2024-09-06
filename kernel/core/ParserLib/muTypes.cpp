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



    TypeInfo::TypeInfo(NumericalType type)
    {
        if (type == LOGICAL)
        {
            m_bits = 8;
            m_flags = TYPE_LOGICAL;
        }
        else if (type == DATETIME)
        {
            m_bits = 64;
            m_flags = TYPE_FLOAT | TYPE_DATETIME;
        }
        else if (type <= I64)
        {
            m_flags = TYPE_INT;
            m_bits = 8 * (0x8 >> (I64 - type));
        }
        else if (type <= UI64)
        {
            m_flags = TYPE_INT | TYPE_UINT;
            m_bits = 8 * (0x8 >> (UI64 - type));
        }
        else if (type <= F64)
        {
            m_flags = TYPE_FLOAT;
            m_bits = type == F64 ? 64 : 32;
        }
        else
        {
            m_flags = TYPE_FLOAT | TYPE_COMPLEX;
            m_bits = type == CF64 ? 64 : 32;
        }
    }

    void TypeInfo::promote(const TypeInfo& other)
    {
        m_bits = std::max(m_bits, other.m_bits);
        m_flags = std::max(m_flags, other.m_flags);
    }


    NumericalType TypeInfo::getPromotedType(const TypeInfo& other) const
    {
        uint8_t bits = std::max(m_bits, other.m_bits);
        uint8_t flags = std::max(m_flags, other.m_flags);

        if (flags & TYPE_COMPLEX)
            return bits == 32 ? CF32 : CF64;

        if (flags & TYPE_DATETIME)
            return DATETIME;

        if (flags & TYPE_FLOAT)
            return bits == 32 ? F32 : F64;

        if (flags & TYPE_INT)
            return NumericalType(I8 + std::log2(bits)-3);

        if (flags & TYPE_UINT)
            return NumericalType(UI8 + std::log2(bits)-3);

        return LOGICAL;
    }


    NumericalType TypeInfo::asType() const
    {
        if (m_flags & TYPE_COMPLEX)
            return m_bits == 32 ? CF32 : CF64;

        if (m_flags & TYPE_DATETIME)
            return DATETIME;

        if (m_flags & TYPE_FLOAT)
            return m_bits == 32 ? F32 : F64;

        if (m_flags & TYPE_INT)
            return NumericalType(I8 + std::log2(m_bits)-3);

        if (m_flags & TYPE_UINT)
            return NumericalType(UI8 + std::log2(m_bits)-3);

        return LOGICAL;
    }


    std::string TypeInfo::printType() const
    {
        switch (asType())
        {
            case LOGICAL:
                return "logical";
            case I8:
                return "value.i8";
            case I16:
                return "value.i16";
            case I32:
                return "value.i32";
            case I64:
                return "value.i64";
            case UI8:
                return "value.ui8";
            case UI16:
                return "value.ui16";
            case UI32:
                return "value.ui32";
            case UI64:
                return "value.ui64";
            case DATETIME:
                return "datetime";
            case F32:
                return "value.f32";
            case F64:
                return "value.f64";
            case CF32:
                return "value.cf32";
        }

        return "value.cf64";
    }



    Numerical::Numerical(int8_t data) : m_type(I8)
    {
        i64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(uint8_t data) : m_type(UI8)
    {
        ui64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(int16_t data) : m_type(I16)
    {
        i64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(uint16_t data) : m_type(UI16)
    {
        ui64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(int32_t data) : m_type(I32)
    {
        i64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(uint32_t data) : m_type(UI32)
    {
        ui64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(int64_t data, NumericalType type) : m_type(type)
    {
        i64 = data;
        m_info = TypeInfo(m_type);

        if (m_type != I64)
        {
            while ((i64 < -(1LL << (m_info.m_bits-1)) || i64 > (1LL << (m_info.m_bits-1))-1) && m_info.m_bits != 64)
                m_info.m_bits *= 2;

            m_type = m_info.asType();
        }
    }

    Numerical::Numerical(uint64_t data, NumericalType type) : m_type(type)
    {
        ui64 = data;
        m_info = TypeInfo(m_type);

        if (m_type != UI64)
        {
            while (ui64 > (1ULL << m_info.m_bits)-1 && m_info.m_bits != 64)
                m_info.m_bits *= 2;

            m_type = m_info.asType();
        }
    }

    Numerical::Numerical(float data) : m_type(F32)
    {
        cf64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(double data) : m_type(F64)
    {
        cf64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(bool data) : m_type(LOGICAL)
    {
        i64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(const std::complex<float>& data) : m_type(CF32)
    {
        cf64 = data;
        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(const std::complex<double>& data, NumericalType type)
    {
        cf64 = data;

        if (type == AUTO)
        {
            if (data.imag() == 0.0)
                m_type = F64;
            else
                m_type = CF64;
        }
        else
            m_type = type;

        m_info = TypeInfo(m_type);
    }

    Numerical::Numerical(const sys_time_point& time) : m_type(DATETIME)
    {
        cf64 = to_double(time);
        m_info = TypeInfo(m_type);
    }

    Numerical Numerical::autoType(const std::complex<double>& data, NumericalType hint)
    {
        if (data.imag() == 0.0 && std::rint(data.real()) == data.real() && hint != F64)
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

    Numerical::InternalType Numerical::getConversion(NumericalType promotion) const
    {
        if (promotion <= I64)
            return Numerical::INT;

        if (promotion <= UI64)
            return Numerical::UINT;

        return Numerical::COMPLEX;
    }

    int64_t Numerical::asI64() const
    {
        if (m_type <= I64)
            return i64;

        if (m_type <= UI64)
            return ui64;

        return intCast(cf64);
    }

    uint64_t Numerical::asUI64() const
    {
        if (m_type <= I64)
            return (uint64_t)i64;

        if (m_type <= UI64)
            return ui64;

        return (uint64_t)intCast(cf64);
    }

    double Numerical::asF64() const
    {
        if (m_type <= I64)
            return i64;

        if (m_type <= UI64)
            return ui64;

        return cf64.real();
    }

    std::complex<double> Numerical::asCF64() const
    {
        if (m_type <= I64)
            return i64;

        if (m_type <= UI64)
            return ui64;

        return cf64;
    }

    Numerical Numerical::operator+(const Numerical& other) const
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() + other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() + other.asUI64(), promotion);

        return Numerical(asCF64() + other.asCF64(), promotion);
    }

    Numerical Numerical::operator-() const
    {
        if (m_type <= I64)
            return Numerical(-i64, m_type);

        if (m_type <= UI64)
            return Numerical(-ui64, (NumericalType)(m_type-I64));

        return Numerical(-cf64, m_type);
    }

    Numerical Numerical::operator-(const Numerical& other) const
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() - other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() - other.asUI64(), promotion);

        return Numerical(asCF64() - other.asCF64(), promotion);
    }

    Numerical Numerical::operator/(const Numerical& other) const
    {
        if (m_type <= I64)
            return autoType(i64 / other.asCF64());

        if (m_type <= UI64)
            return autoType(ui64 / other.asCF64());

        return autoType(cf64 / other.asCF64());
    }

    Numerical Numerical::operator*(const Numerical& other) const
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() * other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() * other.asUI64(), promotion);

        return Numerical(asCF64() * other.asCF64(), promotion);
    }

    Numerical& Numerical::operator+=(const Numerical& other)
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() + other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() + other.asUI64();
        else
            cf64 = asCF64() + other.asCF64();

        m_type = promotion;
        m_info = TypeInfo(m_type);
        return *this;
    }

    Numerical& Numerical::operator-=(const Numerical& other)
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() - other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() - other.asUI64();
        else
            cf64 = asCF64() - other.asCF64();

        m_type = promotion;
        m_info = TypeInfo(m_type);
        return *this;
    }

    Numerical& Numerical::operator/=(const Numerical& other)
    {
        *this = operator/(other);
        return *this;
    }

    Numerical& Numerical::operator*=(const Numerical& other)
    {
        NumericalType promotion = m_info.getPromotedType(other.m_info);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            i64 = asI64() * other.asI64();
        else if (conversion == Numerical::UINT)
            ui64 = asUI64() * other.asUI64();
        else
            cf64 = asCF64() * other.asCF64();

        m_type = promotion;
        m_info = TypeInfo(m_type);
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
        if (m_type <= I64)
            return i64 != 0;

        if (m_type <= UI64)
            return ui64 != 0;

        return cf64 != 0.0;
    }

    bool Numerical::operator!() const
    {
        return !bool(*this);
    }

    bool Numerical::operator==(const Numerical& other) const
    {
        Numerical::InternalType conversion = getConversion(m_info.getPromotedType(other.m_info));

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
        Numerical::InternalType conversion = getConversion(m_info.getPromotedType(other.m_info));

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

    NumericalType Numerical::getType() const
    {
        return m_type;
    }

    TypeInfo Numerical::getInfo() const
    {
        return m_info;
    }

    std::string Numerical::getTypeAsString() const
    {
        return m_info.printType();
    }

    std::string Numerical::print(size_t digits) const
    {
        if (m_type == LOGICAL)
            return toString((bool)i64);

        if (m_type == DATETIME && !isnan(cf64))
            return toString(to_timePoint(cf64.real()), 0);

        if (m_type <= I64)
            return toString(i64);

        if (m_type <= UI64)
            return toString(ui64);

        return toString(cf64, digits > 0 ? digits : 7);
    }

    std::string Numerical::printVal(size_t digits) const
    {
        if (m_type == LOGICAL)
            return toString((bool)i64);

        if (m_type == DATETIME && !isnan(cf64))
            return toString(to_timePoint(cf64.real()), 0);

        if (m_type <= I64)
            return toString(i64);

        if (m_type <= UI64)
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
        return m_info.m_flags & TypeInfo::TYPE_COMPLEX ? m_info.m_bits / 4 : m_info.m_bits / 8;
    }

    bool Numerical::isInt() const
    {
        return m_info.m_flags & TypeInfo::TYPE_INT || ::isInt(asCF64());
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

