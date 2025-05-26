/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#ifndef MUVALUEBASE_HPP
#define MUVALUEBASE_HPP

#include "muTypes.hpp"


namespace mu
{
    class BaseValue
    {
        public:
            DataType m_type;

            virtual ~BaseValue() {}
            virtual BaseValue& operator=(const BaseValue& other) = 0;
            virtual BaseValue* clone() const = 0;

            virtual BaseValue* operator+(const BaseValue& other) const;
            virtual BaseValue* operator-() const;
            virtual BaseValue* operator-(const BaseValue& other) const;
            virtual BaseValue* operator/(const BaseValue& other) const;
            virtual BaseValue* operator*(const BaseValue& other) const;
            virtual BaseValue* operator^(const BaseValue& other) const;

            virtual BaseValue& operator+=(const BaseValue& other);
            virtual BaseValue& operator-=(const BaseValue& other);
            virtual BaseValue& operator/=(const BaseValue& other);
            virtual BaseValue& operator*=(const BaseValue& other);
            virtual BaseValue& operator^=(const BaseValue& other);

            virtual BaseValue* pow(const BaseValue& other) const;

            virtual bool isValid() const = 0;

            virtual operator bool() const;
            virtual bool operator!() const;
            virtual bool operator==(const BaseValue& other) const;
            virtual bool operator!=(const BaseValue& other) const;
            virtual bool operator<(const BaseValue& other) const;
            virtual bool operator<=(const BaseValue& other) const;
            virtual bool operator>(const BaseValue& other) const;
            virtual bool operator>=(const BaseValue& other) const;

            virtual size_t getBytes() const = 0;

            virtual bool isMethod(const std::string& sMethod) const;
            virtual BaseValue* call(const std::string& sMethod) const;
            virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const;
            virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const;

            virtual std::string print(size_t digits, size_t chrs, bool trunc) const = 0;
            virtual std::string printVal(size_t digits, size_t chrs) const = 0;
    };

    inline bool nonRecursiveOps(DataType lhs, DataType rhs)
    {
        return lhs == TYPE_CATEGORY || (rhs == TYPE_ARRAY && lhs != rhs);
    }
}


#define BASE_VALUE_DECL(CLASS, ID, TYPE, ATTR)                \
private:                                                      \
    TYPE ATTR;                                                \
public:                                                       \
    CLASS() : BaseValue(), ATTR()                             \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const TYPE& val) : BaseValue(), ATTR(val)           \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const CLASS& other): BaseValue()                    \
    {                                                         \
        m_type = ID;                                          \
        ATTR = other.ATTR;                                    \
    }                                                         \
    CLASS(CLASS&& other) = default;                           \
    CLASS(const BaseValue& other);                            \
    CLASS& operator=(const BaseValue& other) override;        \
    CLASS& operator=(const TYPE& val)                         \
    {                                                         \
        ATTR = val;                                           \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(const CLASS& other)                      \
    {                                                         \
        ATTR = other.ATTR;                                    \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(CLASS&& other) = default;                \
    BaseValue* clone() const override                         \
    {                                                         \
        return new CLASS(*this);                              \
    }                                                         \
    TYPE& get()                                               \
    {                                                         \
        return ATTR;                                          \
    }                                                         \
    const TYPE& get() const                                   \
    {                                                         \
        return ATTR;                                          \
    }

#define BASE_VALUE_IMPL(CLASS, ID, ATTR)                      \
CLASS::CLASS(const BaseValue& other) : BaseValue()            \
{                                                             \
    m_type = ID;                                              \
    if (other.m_type == ID)                                   \
        ATTR = static_cast<const CLASS&>(other).ATTR;         \
    else                                                      \
        throw ParserError(ecASSIGNED_TYPE_MISMATCH);          \
}                                                             \
CLASS& CLASS::operator=(const BaseValue& other)               \
{                                                             \
    if (other.m_type == ID)                                   \
        ATTR = static_cast<const CLASS&>(other).ATTR;         \
    else                                                      \
        throw ParserError(ecASSIGNED_TYPE_MISMATCH);          \
    return *this;                                             \
}

#endif // MUVALUEBASE_HPP


