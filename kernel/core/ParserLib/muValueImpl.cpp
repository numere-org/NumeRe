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

#include "muValueImpl.hpp"
#include "muParserError.h"
#include "../utils/stringtools.hpp"

namespace mu
{
    NeutralValue::NeutralValue()
    {
        m_type = TYPE_INVALID;
    }
    NeutralValue::NeutralValue(const NeutralValue& other)
    {
        m_type = TYPE_INVALID;
    }
    BaseValue& NeutralValue::operator=(const BaseValue& other)
    {
        if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH_OOB);

        return *this;
    }
    BaseValue& NeutralValue::operator=(const NeutralValue& other)
    {
        return *this;
    }
    BaseValue* NeutralValue::clone() const
    {
        return new NeutralValue(*this);
    }

    BaseValue* NeutralValue::operator+(const BaseValue& other) const
    {
        return other.clone();
    }
    BaseValue* NeutralValue::operator-() const
    {
        return clone();
    }
    BaseValue* NeutralValue::operator-(const BaseValue& other) const
    {
        return -other;
    }
    BaseValue* NeutralValue::operator/(const BaseValue& other) const
    {
        return NumValue(Numerical(0.0)) / other;// NumValue(Numerical(1.0)) / other;
    }
    BaseValue* NeutralValue::operator*(const BaseValue& other) const
    {
        return NumValue(Numerical(0.0)) * other;//other.clone();
    }

    bool NeutralValue::isValid() const
    {
        return false;
    }

    NeutralValue::operator bool() const
    {
        return false;
    }

    bool NeutralValue::operator==(const BaseValue& other) const
    {
        return false;
    }

    bool NeutralValue::operator<(const BaseValue& other) const
    {
        return false;
    }

    size_t NeutralValue::getBytes() const
    {
        return 0;
    }

    std::string NeutralValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return "void*";
    }
    std::string NeutralValue::printVal(size_t digits, size_t chrs) const
    {
        return "void*";
    }


    // X' OP= ARR
    // CAT OP= X

    // NUM *= STR

    BASE_VALUE_IMPL(NumValue, TYPE_NUMERICAL, m_val)

    BaseValue* NumValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val + static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val + static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* NumValue::operator-() const
    {
        return new NumValue(-m_val);
    }

    BaseValue* NumValue::operator-(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val - static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val - static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) - static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* NumValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val / static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val / static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) / static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* NumValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val * static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
        {
            if (m_val.isInt() && m_val.asI64() == 1)
                return other.clone();

            return new NumValue(m_val * static_cast<const CatValue&>(other).get().val);
        }
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) * static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_STRING)
            return new StrValue(strRepeat(static_cast<const StrValue&>(other).get(), m_val.asI64()));
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue& NumValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val += static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val += static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    BaseValue& NumValue::operator-=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val -= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val -= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    BaseValue& NumValue::operator/=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val /= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val /= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    BaseValue& NumValue::operator*=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val *= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val *= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    BaseValue* NumValue::pow(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.pow(static_cast<const NumValue&>(other).m_val));
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.pow(static_cast<const CatValue&>(other).get().val));
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Array(Value(m_val)).pow(static_cast<const ArrValue&>(other).get()));
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    bool NumValue::isValid() const
    {
        return m_val.asCF64() == m_val.asCF64();
    }

    NumValue::operator bool() const
    {
        return bool(m_val);
    }

    bool NumValue::operator==(const BaseValue& other) const
    {
        return other.m_type == TYPE_NUMERICAL && m_val == static_cast<const NumValue&>(other).m_val;
    }

    bool NumValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_NUMERICAL)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const NumValue&>(other).m_val;
    }

    size_t NumValue::getBytes() const
    {
        return m_val.getBytes();
    }

    std::string NumValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return m_val.print(digits);
    }

    std::string NumValue::printVal(size_t digits, size_t chrs) const
    {
        return m_val.printVal(digits);
    }





    BASE_VALUE_IMPL(StrValue, TYPE_STRING, m_val)

    BaseValue* StrValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_STRING)
            return new StrValue(m_val + static_cast<const StrValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new StrValue(m_val + static_cast<const CatValue&>(other).get().name);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* StrValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new StrValue(strRepeat(m_val, static_cast<const NumValue&>(other).get().asI64()));
        else if (other.m_type == TYPE_CATEGORY)
            return new StrValue(strRepeat(m_val, static_cast<const CatValue&>(other).get().val.asI64()));
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) * static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue& StrValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_STRING)
            m_val += static_cast<const StrValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val += static_cast<const CatValue&>(other).get().name;
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    BaseValue& StrValue::operator*=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val = strRepeat(m_val, static_cast<const NumValue&>(other).get().asI64());
        else if (other.m_type == TYPE_CATEGORY)
            m_val = strRepeat(m_val, static_cast<const CatValue&>(other).get().val.asI64());
        else if (other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }

    bool StrValue::isValid() const
    {
        return m_val.length() > 0;
    }

    StrValue::operator bool() const
    {
        return m_val.length() > 0;
    }

    bool StrValue::operator==(const BaseValue& other) const
    {
        return other.m_type == TYPE_STRING && m_val == static_cast<const StrValue&>(other).m_val;
    }

    bool StrValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_STRING)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const StrValue&>(other).m_val;
    }

    size_t StrValue::getBytes() const
    {
        return m_val.length();
    }

    std::string StrValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (chrs > 0)
        {
            if (trunc)
                return truncString(toExternalString(replaceControlCharacters(m_val)), chrs);

            return ellipsize(toExternalString(replaceControlCharacters(m_val)), chrs);
        }

        return toExternalString(replaceControlCharacters(m_val));
    }

    std::string StrValue::printVal(size_t digits, size_t chrs) const
    {
        if (chrs > 0)
            return ellipsize(m_val, chrs);

        return m_val;
    }












    BASE_VALUE_IMPL(CatValue, TYPE_CATEGORY, m_val)

    BaseValue* CatValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.val + static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val + static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_STRING)
            return new StrValue(m_val.name + static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* CatValue::operator-() const
    {
        return new NumValue(-m_val.val);
    }

    BaseValue* CatValue::operator-(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.val - static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val - static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) - static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* CatValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.val / static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val / static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) / static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* CatValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
        {
            const Numerical& n = static_cast<const NumValue&>(other).get();

            if (n.isInt() && n.asI64() == 1)
                return clone();

            return new NumValue(m_val.val * n);
        }
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val * static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_STRING)
            return new StrValue(strRepeat(static_cast<const StrValue&>(other).get(), m_val.val.asI64()));
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) * static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_INVALID)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }

    bool CatValue::isValid() const
    {
        return m_val.name.length() > 0;
    }

    CatValue::operator bool() const
    {
        return m_val.name.length() > 0;
    }

    bool CatValue::operator==(const BaseValue& other) const
    {
        return other == TYPE_CATEGORY && m_val == static_cast<const CatValue&>(other).m_val;
    }

    bool CatValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_CATEGORY)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const CatValue&>(other).m_val;
    }

    size_t CatValue::getBytes() const
    {
        return m_val.val.getBytes();
    }

    std::string CatValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return toExternalString(m_val.name) + ": " + toString(m_val.val.asI64());
    }

    std::string CatValue::printVal(size_t digits, size_t chrs) const
    {
        if (chrs > 0)
            return ellipsize(m_val.name, chrs);

        return m_val.name;
    }








    BASE_VALUE_IMPL(ArrValue, TYPE_ARRAY, m_val)

    BaseValue* ArrValue::operator+(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return new ArrValue(m_val + static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_INVALID)
            return clone();

        return new ArrValue(m_val + Value(other.clone()));
    }

    BaseValue* ArrValue::operator-() const
    {
        return new ArrValue(-m_val);
    }

    BaseValue* ArrValue::operator-(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return new ArrValue(m_val - static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_INVALID)
            return clone();

        return new ArrValue(m_val - Value(other.clone()));
    }

    BaseValue* ArrValue::operator/(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return new ArrValue(m_val / static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_INVALID)
            return clone();

        return new ArrValue(m_val / Value(other.clone()));
    }

    BaseValue* ArrValue::operator*(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return new ArrValue(m_val * static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_INVALID)
            return clone();

        return new ArrValue(m_val * Value(other.clone()));
    }

    BaseValue& ArrValue::operator+=(const BaseValue& other)
    {
        if (other == TYPE_ARRAY)
            m_val += static_cast<const ArrValue&>(other).m_val;
        else
            m_val += Value(other.clone());

        return *this;
    }

    BaseValue& ArrValue::operator-=(const BaseValue& other)
    {
        if (other == TYPE_ARRAY)
            m_val -= static_cast<const ArrValue&>(other).m_val;
        else
            m_val -= Value(other.clone());

        return *this;
    }

    BaseValue& ArrValue::operator/=(const BaseValue& other)
    {
        if (other == TYPE_ARRAY)
            m_val /= static_cast<const ArrValue&>(other).m_val;
        else
            m_val /= Value(other.clone());

        return *this;
    }

    BaseValue& ArrValue::operator*=(const BaseValue& other)
    {
        if (other == TYPE_ARRAY)
            m_val *= static_cast<const ArrValue&>(other).m_val;
        else
            m_val *= Value(other.clone());

        return *this;
    }

    BaseValue* ArrValue::pow(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return new ArrValue(m_val.pow(static_cast<const ArrValue&>(other).m_val));
        else if (other.m_type == TYPE_INVALID)
            return clone();

        return new ArrValue(m_val.pow(Value(other.clone())));
    }

    bool ArrValue::isValid() const
    {
        return m_val.size() > 0;
    }

    ArrValue::operator bool() const
    {
        return all(m_val);
    }

    bool ArrValue::operator==(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return all(m_val == static_cast<const ArrValue&>(other).m_val);

        return all(m_val == Array(Value(other.clone())));
    }

    bool ArrValue::operator<(const BaseValue& other) const
    {
        if (other == TYPE_ARRAY)
            return all(m_val < static_cast<const ArrValue&>(other).m_val);

        return all(m_val < Value(other.clone()));
    }

    size_t ArrValue::getBytes() const
    {
        return m_val.getBytes();
    }

    std::string ArrValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return "{" + m_val.printDims() + " " + m_val.print(digits, chrs, trunc) + "}";
    }

    std::string ArrValue::printVal(size_t digits, size_t chrs) const
    {
        return "{" + m_val.printVals(digits, chrs) + "}";
    }







}

