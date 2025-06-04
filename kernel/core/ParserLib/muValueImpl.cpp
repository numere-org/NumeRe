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
    /////////////////////////////////////////////////
    /// \brief Construct a neutral value.
    /////////////////////////////////////////////////
    NeutralValue::NeutralValue()
    {
        m_type = TYPE_NEUTRAL;
    }


    /////////////////////////////////////////////////
    /// \brief Copy a neutral value.
    ///
    /// \param other const NeutralValue&
    ///
    /////////////////////////////////////////////////
    NeutralValue::NeutralValue(const NeutralValue& other)
    {
        m_type = TYPE_NEUTRAL;
    }


    /////////////////////////////////////////////////
    /// \brief Assign another value.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NeutralValue::operator=(const BaseValue& other)
    {
        if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH_OOB);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Assign another neutral value.
    ///
    /// \param other const NeutralValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NeutralValue::operator=(const NeutralValue& other)
    {
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Clone this instance.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::clone() const
    {
        return new NeutralValue(*this);
    }


    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::operator+(const BaseValue& other) const
    {
        return other.clone();
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::operator-() const
    {
        return clone();
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::operator-(const BaseValue& other) const
    {
        return -other;
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::operator/(const BaseValue& other) const
    {
        return NumValue(Numerical(0.0)) / other;// NumValue(Numerical(1.0)) / other;
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NeutralValue::operator*(const BaseValue& other) const
    {
        return NumValue(Numerical(0.0)) * other;//other.clone();
    }


    /////////////////////////////////////////////////
    /// \brief A neutral value is never valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NeutralValue::isValid() const
    {
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief A neutral value is always false.
    ///
    /// \return NeutralValue::operator
    ///
    /////////////////////////////////////////////////
    NeutralValue::operator bool() const
    {
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Neutral values always compare false to
    /// other values.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NeutralValue::operator==(const BaseValue& other) const
    {
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Neutral values always compare false to
    /// other values.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NeutralValue::operator<(const BaseValue& other) const
    {
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Neutral values do not have any size.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t NeutralValue::getBytes() const
    {
        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Neutral values are printed as "void*".
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string NeutralValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return "void*";
    }


    /////////////////////////////////////////////////
    /// \brief Neutral values are printed as "void*".
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string NeutralValue::printVal(size_t digits, size_t chrs) const
    {
        return "void*";
    }


    //------------------------------------------------------------------------------


    /////////////////////////////////////////////////
    /// \brief Copy another BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    NumValue::NumValue(const BaseValue& other) : BaseValue()
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
        {
            m_type = other.m_type;
            m_val = static_cast<const NumValue&>(other).m_val;
        }
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }

    /////////////////////////////////////////////////
    /// \brief Assign another BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return NumValue&
    ///
    /////////////////////////////////////////////////
    NumValue& NumValue::operator=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
        {
            m_type = other.m_type;
            m_val = static_cast<const NumValue&>(other).m_val;
        }
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Helper constructor to create an
    /// invalid value.
    ///
    /// \param val double
    /// \param makeInvalid bool
    ///
    /////////////////////////////////////////////////
    NumValue::NumValue(double val, bool makeInvalid)
    {
        if (makeInvalid)
            m_type = TYPE_INVALID;
        else
            m_type = TYPE_NUMERICAL;

        m_val = val;
    }


    /////////////////////////////////////////////////
    /// \brief Addition operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val + static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val + static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator-() const
    {
        return new NumValue(-m_val);
    }


    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator-(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val - static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val - static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) - static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val / static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val / static_cast<const CatValue&>(other).get().val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) / static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
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
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Power operator
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::operator^(const BaseValue& other) const
    {
        return pow(other);
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NumValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            m_val += static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val += static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NumValue::operator-=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            m_val -= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val -= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NumValue::operator/=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            m_val /= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val /= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NumValue::operator*=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            m_val *= static_cast<const NumValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val *= static_cast<const CatValue&>(other).get().val;
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& NumValue::operator^=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            m_val = m_val.pow(static_cast<const NumValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            m_val = m_val.pow(static_cast<const CatValue&>(other).get().val);
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumValue::flipSign()
    {
        m_val.flipSign();
    }


    /////////////////////////////////////////////////
    /// \brief Power function.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* NumValue::pow(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val.pow(static_cast<const NumValue&>(other).m_val));
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.pow(static_cast<const CatValue&>(other).get().val));
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Array(Value(m_val)).pow(static_cast<const ArrValue&>(other).get()));
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Detect, whether the contained value is
    /// valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NumValue::isValid() const
    {
        return m_val.asCF64() == m_val.asCF64();
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value.
    ///
    /// \return NumValue::operator
    ///
    /////////////////////////////////////////////////
    NumValue::operator bool() const
    {
        return bool(m_val);
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NumValue::operator==(const BaseValue& other) const
    {
        return other.m_type == TYPE_NUMERICAL && m_val == static_cast<const NumValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool NumValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_NUMERICAL && other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const NumValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Return the size of the contained value
    /// in bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t NumValue::getBytes() const
    {
        return m_val.getBytes();
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string (possibly adding quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string NumValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return m_val.print(digits);
    }


    /////////////////////////////////////////////////
    /// \brief  Print the contained value into a
    /// std::string (without any additional quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string NumValue::printVal(size_t digits, size_t chrs) const
    {
        return m_val.printVal(digits);
    }


    //------------------------------------------------------------------------------

    // Basic implementation of the StrValue class
    BASE_VALUE_IMPL(StrValue, TYPE_STRING, m_val)

    /////////////////////////////////////////////////
    /// \brief Addition operator -> Concats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_STRING)
            return new StrValue(m_val + static_cast<const StrValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new StrValue(m_val + static_cast<const CatValue&>(other).get().name);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator -> Repeats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL)
            return new StrValue(strRepeat(m_val, static_cast<const NumValue&>(other).get().asI64()));
        else if (other.m_type == TYPE_CATEGORY)
            return new StrValue(strRepeat(m_val, static_cast<const CatValue&>(other).get().val.asI64()));
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) * static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator. Concats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& StrValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_STRING)
            m_val += static_cast<const StrValue&>(other).m_val;
        else if (other.m_type == TYPE_CATEGORY)
            m_val += static_cast<const CatValue&>(other).get().name;
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator. Repeats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& StrValue::operator*=(const BaseValue& other)
    {
        if (other.m_type == TYPE_NUMERICAL)
            m_val = strRepeat(m_val, static_cast<const NumValue&>(other).get().asI64());
        else if (other.m_type == TYPE_CATEGORY)
            m_val = strRepeat(m_val, static_cast<const CatValue&>(other).get().val.asI64());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Detect, whether the contained value is
    /// valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StrValue::isValid() const
    {
        return m_val.length() > 0;
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value.
    ///
    /// \return NumValue::operator
    ///
    /////////////////////////////////////////////////
    StrValue::operator bool() const
    {
        return m_val.length() > 0;
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StrValue::operator==(const BaseValue& other) const
    {
        return other.m_type == TYPE_STRING && m_val == static_cast<const StrValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StrValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_STRING)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const StrValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Return the size of the contained value
    /// in bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StrValue::getBytes() const
    {
        return m_val.length();
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string (possibly adding quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief  Print the contained value into a
    /// std::string (without any additional quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StrValue::printVal(size_t digits, size_t chrs) const
    {
        if (chrs > 0)
            return ellipsize(m_val, chrs);

        return m_val;
    }



    //------------------------------------------------------------------------------

    // Basic implementation of the CatValue class
    BASE_VALUE_IMPL(CatValue, TYPE_CATEGORY, m_val)

    /////////////////////////////////////////////////
    /// \brief Addition operator. Adds or concats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val.val + static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val + static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_STRING)
            return new StrValue(m_val.name + static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) + static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::operator-() const
    {
        return new NumValue(-m_val.val);
    }


    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::operator-(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val.val - static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val - static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) - static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
            return new NumValue(m_val.val / static_cast<const NumValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new NumValue(m_val.val / static_cast<const CatValue&>(other).m_val.val);
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) / static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator. Multiplies or
    /// repeats.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_NUMERICAL || other.m_type == TYPE_INVALID)
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
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        throw ParserError(ecTYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void CatValue::flipSign()
    {
        m_val.val.flipSign();
    }


    /////////////////////////////////////////////////
    /// \brief Detect, whether the contained value is
    /// valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool CatValue::isValid() const
    {
        return m_val.name.length() > 0;
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value.
    ///
    /// \return NumValue::operator
    ///
    /////////////////////////////////////////////////
    CatValue::operator bool() const
    {
        return m_val.name.length() > 0;
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool CatValue::operator==(const BaseValue& other) const
    {
        return other.m_type == TYPE_CATEGORY && m_val == static_cast<const CatValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool CatValue::operator<(const BaseValue& other) const
    {
        if (other.m_type != TYPE_CATEGORY)
            throw ParserError(ecTYPE_MISMATCH);

        return m_val < static_cast<const CatValue&>(other).m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Return the size of the contained value
    /// in bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t CatValue::getBytes() const
    {
        return m_val.val.getBytes();
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string (possibly adding quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string CatValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return toExternalString(m_val.name) + ": " + toString(m_val.val.asI64());
    }


    /////////////////////////////////////////////////
    /// \brief  Print the contained value into a
    /// std::string (without any additional quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string CatValue::printVal(size_t digits, size_t chrs) const
    {
        if (chrs > 0)
            return ellipsize(m_val.name, chrs);

        return m_val.name;
    }



    //------------------------------------------------------------------------------

    // Basic implementation of the ArrValue class
    BASE_VALUE_IMPL(ArrValue, TYPE_ARRAY, m_val)

    /////////////////////////////////////////////////
    /// \brief Addition operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val + static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val + Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator-() const
    {
        return new ArrValue(-m_val);
    }


    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator-(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val - static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val - Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val / static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val / Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator*(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val * static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val * Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Power operator
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::operator^(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val ^ static_cast<const ArrValue&>(other).m_val);
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val * Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& ArrValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_ARRAY)
            m_val += static_cast<const ArrValue&>(other).m_val;
        else
            m_val += Value(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& ArrValue::operator-=(const BaseValue& other)
    {
        if (other.m_type == TYPE_ARRAY)
            m_val -= static_cast<const ArrValue&>(other).m_val;
        else
            m_val -= Value(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& ArrValue::operator/=(const BaseValue& other)
    {
        if (other.m_type == TYPE_ARRAY)
            m_val /= static_cast<const ArrValue&>(other).m_val;
        else
            m_val /= Value(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& ArrValue::operator*=(const BaseValue& other)
    {
        if (other.m_type == TYPE_ARRAY)
            m_val *= static_cast<const ArrValue&>(other).m_val;
        else
            m_val *= Value(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& ArrValue::operator^=(const BaseValue& other)
    {
        if (other.m_type == TYPE_ARRAY)
            m_val ^= static_cast<const ArrValue&>(other).m_val;
        else
            m_val ^= Value(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ArrValue::flipSign()
    {
        m_val.flipSign();
    }


    /////////////////////////////////////////////////
    /// \brief Power function.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::pow(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return new ArrValue(m_val.pow(static_cast<const ArrValue&>(other).m_val));
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();

        return new ArrValue(m_val.pow(Value(other.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Detect, whether the contained value is
    /// valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ArrValue::isValid() const
    {
        return m_val.size() > 0;
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value.
    ///
    /// \return NumValue::operator
    ///
    /////////////////////////////////////////////////
    ArrValue::operator bool() const
    {
        return all(m_val);
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ArrValue::operator==(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return all(m_val == static_cast<const ArrValue&>(other).m_val);

        return all(m_val == Array(Value(other.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ArrValue::operator<(const BaseValue& other) const
    {
        if (other.m_type == TYPE_ARRAY)
            return all(m_val < static_cast<const ArrValue&>(other).m_val);

        return all(m_val < Value(other.clone()));
    }


    /////////////////////////////////////////////////
    /// \brief Return the size of the contained value
    /// in bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t ArrValue::getBytes() const
    {
        return m_val.getBytes();
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string (possibly adding quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string ArrValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return "{" + m_val.printDims() + " " + m_val.getCommonTypeAsString() + "}";
    }


    /////////////////////////////////////////////////
    /// \brief  Print the contained value into a
    /// std::string (without any additional quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string ArrValue::printVal(size_t digits, size_t chrs) const
    {
        return "{" + m_val.printVals(digits, chrs) + "}";
    }







}

