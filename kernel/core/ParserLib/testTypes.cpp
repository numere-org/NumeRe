/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#include "testTypes.hpp"

#ifndef TESTTYPES_HEADERONLY
BaseValue* BaseValue::operator+(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue* BaseValue::operator-() const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue* BaseValue::operator-(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue* BaseValue::operator/(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue* BaseValue::operator*(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}

BaseValue& BaseValue::operator+=(const BaseValue& other)
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue& BaseValue::operator-=(const BaseValue& other)
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue& BaseValue::operator/=(const BaseValue& other)
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
BaseValue& BaseValue::operator*=(const BaseValue& other)
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}

BaseValue& BaseValue::pow(const BaseValue& other)
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}

bool BaseValue::operator!() const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
bool BaseValue::operator==(const BaseValue& other) const
{
    throw m_type == other.m_type;
}
bool BaseValue::operator!=(const BaseValue& other) const
{
    throw m_type != other.m_type;
}
bool BaseValue::operator<(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
bool BaseValue::operator<=(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
bool BaseValue::operator>(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
bool BaseValue::operator>=(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}

bool BaseValue::operator&&(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
bool BaseValue::operator||(const BaseValue& other) const
{
    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}

bool BaseValue::isMethod(const std::string& sMethod) const
{
    return false;
}
BaseValue* BaseValue::call(const std::string& sMethod) const
{
    throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
}
BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1) const
{
    throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
}
BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
{
    throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
}



BaseValue& CategoryValue::operator+=(const BaseValue& other)
{
    if (other.m_type == mu::TYPE_CATEGORY)
        m_val.name += static_cast<const CategoryValue&>(other).m_val.name;
    else
        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    return *this;
}
BaseValue* CategoryValue::operator+(const BaseValue& other) const
{
    if (other.m_type == mu::TYPE_CATEGORY)
        return new CategoryValue(mu::Category(m_val.val, m_val.name + static_cast<const CategoryValue&>(other).m_val.name));

    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}


BaseValue& NumericValue::operator+=(const BaseValue& other)
{
    if (other.m_type == mu::TYPE_NUMERICAL)
        m_val += static_cast<const NumericValue&>(other).m_val;
    else if (other.m_type == mu::TYPE_CATEGORY)
        m_val += static_cast<const CategoryValue&>(other).get().val;
    else
        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    return *this;
}
BaseValue* NumericValue::operator+(const BaseValue& other) const
{
    if (other.m_type == mu::TYPE_NUMERICAL)
        return new NumericValue(m_val + static_cast<const NumericValue&>(other).m_val);
    else if (other.m_type == mu::TYPE_CATEGORY)
        return new NumericValue(m_val + static_cast<const CategoryValue&>(other).get().val);

    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}



BaseValue& StringValue::operator+=(const BaseValue& other)
{
    if (other.m_type == mu::TYPE_STRING)
        m_val += static_cast<const StringValue&>(other).m_val;
    else if (other.m_type == mu::TYPE_CATEGORY)
        m_val += static_cast<const CategoryValue&>(other).get().name;
    else
        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    return *this;
}
BaseValue* StringValue::operator+(const BaseValue& other) const
{
    if (other.m_type == mu::TYPE_STRING)
        return new StringValue(m_val + static_cast<const StringValue&>(other).m_val);
    else if (other.m_type == mu::TYPE_CATEGORY)
        return new StringValue(m_val + static_cast<const CategoryValue&>(other).get().name);

    throw mu::ParserError(mu::ecTYPE_MISMATCH);
}
#endif


Value::Value() : std::unique_ptr<BaseValue>() {}
Value::Value(BaseValue* other) : std::unique_ptr<BaseValue>(other) {}
Value::Value(const mu::Numerical& val) : Value()
{
    reset(new NumericValue(val));
}
Value::Value(bool logical) : Value()
{
    reset(new NumericValue(logical));
}
Value::Value(const std::string& val) : Value()
{
    reset(new StringValue(val));
}
Value::Value(const char* val) : Value()
{
    reset(new StringValue(val));
}
Value::Value(const Value& other)
{
    if (other)
        reset(other->clone());
}

arr::arr() : std::vector<mu::Value>(), m_commonType(mu::TYPE_VOID)
{
    //
}

arr::arr(const arr& other) : arr()
{
    resize(other.size());

    for (size_t i = 0; i < other.size(); i++)
    {
        operator[](i).reset(other[i]->clone());
    }
}

#ifndef STRUCTURES_HEADERONLY
Value& Value::operator=(const Value& other)
{
    if (!other.get())
        reset(nullptr);
    else if (!get() || (get()->m_type != other->m_type))
        reset(other->clone());
    else if (this != &other)
        *get() = *other.get();

    return *this;
}
Value& Value::operator+=(const Value& other)
{
    if (get() && other)
        *get() += *other;

    return *this;
}


arr& arr::operator=(const arr& other)
{
    if (other.size() == 1)
    {
        if (size() != 1)
            resize(1);

        front() = other.front();
    }
    else
    {
        resize(other.size());

        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }

    m_commonType = other.m_commonType;
    return *this;
}
arr& arr::operator+=(const arr& other)
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
arr arr::operator+(const arr& other) const
{
    arr res;
    res.resize(std::max(size(), other.size()));

    for (size_t i = 0; i < res.size(); i++)
    {
        res[i].reset(*operator[](i).get() + *other[i].get());
    }

    return res;
}
const mu::Value& arr::get(size_t i) const
{
    if (size() == 1u)
        return front();
    else if (size() <= i)
        return m_default;

    return operator[](i);
}

#endif // TESTTYPES_HEADERONLY

Var& Var::operator=(const arr& other)
{
    arr::operator=(arr(other));
    return *this;
}

const mu::Value arr::m_default(mu::TYPE_INVALID);
