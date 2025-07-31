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

#include <set>

// Reasonable other data types:
// - Duration (as extension of date-time)
// - FilePath (as extension of string w/ operator/())

// Reasonable additional methods:
// - .year, ..., .microsec (for date-time and duration)
// - .drive, .path, .name, .ext (for FilePath)

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
            throw ParserError(ecTYPE_MISMATCH_OOB, getTypeAsString(m_type) + " = " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
        {
            const RefValue& refVal = static_cast<const RefValue&>(other);

            if (refVal.get().m_type == TYPE_NUMERICAL || refVal.get().m_type == TYPE_INVALID)
            {
                m_type = refVal.get().m_type;
                m_val = static_cast<const NumValue&>(refVal.get()).m_val;
            }
            else
                throw ParserError(ecASSIGNED_TYPE_MISMATCH);
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator-(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator/(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator-=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator/=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator^=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return pow(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
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
        if (other.m_type == TYPE_REFERENCE)
            return operator==(static_cast<const RefValue&>(other).get());

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
        if (other.m_type == TYPE_REFERENCE)
            return operator<(static_cast<const RefValue&>(other).get());

        if (other.m_type != TYPE_NUMERICAL && other.m_type != TYPE_INVALID)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " < " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Division operator -> Path operator
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_STRING)
            return new PathValue(Path(m_val) / static_cast<const StrValue&>(other).m_val);
        else if (other.m_type == TYPE_CATEGORY)
            return new PathValue(Path(m_val) / static_cast<const CatValue&>(other).get().name);
        else if (other.m_type == TYPE_OBJECT && static_cast<const Object&>(other).getObjectType() == "path")
            return new PathValue(Path(m_val) / static_cast<const PathValue&>(other).get());
        else if (other.m_type == TYPE_ARRAY)
            return new ArrValue(Value(m_val) / static_cast<const ArrValue&>(other).get());
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();
        else if (other.m_type == TYPE_REFERENCE)
            return operator/(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));

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
        if (other.m_type == TYPE_REFERENCE)
            return operator==(static_cast<const RefValue&>(other).get());

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
        if (other.m_type == TYPE_REFERENCE)
            return operator<(static_cast<const RefValue&>(other).get());

        if (other.m_type != TYPE_STRING)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " < " + getTypeAsString(other.m_type));

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
    /// \brief Does the passed string correspond to a
    /// method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition StrValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"len", 0}, {"first", 0}, {"last", 0},
                                        {"at", 1}, {"startsw", 1}, {"endsw", 1},
                                        {"sub", 1}, {"sub", 2}, {"splt", 1}, {"splt", 2},
                                        {"fnd", 1}, {"fnd", 2}, {"rfnd", 1}, {"rfnd", 2},
                                        {"mtch", 1}, {"mtch", 2}, {"rmtch", 1}, {"rmtch", 2},
                                        {"nmtch", 1}, {"nmtch", 2}, {"nrmtch", 1}, {"nrmtch", 2}});

        auto iter = methods.find(MethodDefinition(sMethod, argc));

        if (iter != methods.end())
            return *iter;

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::call(const std::string& sMethod) const
    {
        if (sMethod == "len")
            return new NumValue(m_val.length());
        else if (sMethod == "first")
            return new StrValue(std::string(1, m_val.front()));
        else if (sMethod == "last")
            return new StrValue(std::string(1, m_val.back()));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::call(const std::string& sMethod, const BaseValue& arg1) const
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return call(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "at" && arg1.m_type == TYPE_NUMERICAL)
        {
            int64_t p = static_cast<const NumValue&>(arg1).get().asI64();

            if (p <= 1)
                return new StrValue(m_val.substr(0, 1));
            else if (p >= (int64_t)m_val.length())
                return new StrValue(m_val.substr(m_val.length()-1, 1));
            else
                return new StrValue(m_val.substr(p-1, 1));
        }
        else if (sMethod == "startsw" && arg1.m_type == TYPE_STRING)
        {
            if (!arg1)
                return new NumValue(false);

            return new NumValue(m_val.starts_with(static_cast<const StrValue&>(arg1).get()));
        }
        else if (sMethod == "endsw" && arg1.m_type == TYPE_STRING)
        {
            if (!arg1)
                return new NumValue(false);

            return new NumValue(m_val.ends_with(static_cast<const StrValue&>(arg1).get()));
        }
        else if (sMethod == "sub" && arg1.m_type == TYPE_NUMERICAL)
            return new StrValue(substr_impl(m_val, static_cast<const NumValue&>(arg1).get().asI64()-1));
        else if (sMethod == "splt" && arg1.m_type == TYPE_STRING)
            return new ArrValue(split_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "fnd" && arg1.m_type == TYPE_STRING)
            return new NumValue(strfnd_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "rfnd" && arg1.m_type == TYPE_STRING)
            return new NumValue(strrfnd_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "mtch" && arg1.m_type == TYPE_STRING)
            return new NumValue(strmatch_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "rmtch" && arg1.m_type == TYPE_STRING)
            return new NumValue(strrmatch_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "nmtch" && arg1.m_type == TYPE_STRING)
            return new NumValue(str_not_match_impl(m_val, static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "nrmtch" && arg1.m_type == TYPE_STRING)
            return new NumValue(str_not_rmatch_impl(m_val, static_cast<const StrValue&>(arg1).get()));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StrValue::call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return call(sMethod,
                        arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                        arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        if (sMethod == "sub" && arg1.m_type == TYPE_NUMERICAL && arg2.m_type == TYPE_NUMERICAL)
            return new StrValue(substr_impl(m_val, static_cast<const NumValue&>(arg1).get().asI64()-1,
                                            static_cast<const NumValue&>(arg2).get().asI64()));
        else if (sMethod == "splt" && arg1.m_type == TYPE_STRING)
            return new ArrValue(split_impl(m_val, static_cast<const StrValue&>(arg1).get(), bool(arg2)));
        else if (sMethod == "fnd" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(strfnd_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                            static_cast<const NumValue&>(arg2).get().asI64()-1));
        else if (sMethod == "rfnd" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(strrfnd_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                             static_cast<const NumValue&>(arg2).get().asI64()-1));
        else if (sMethod == "mtch" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(strmatch_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                              static_cast<const NumValue&>(arg2).get().asI64()-1));
        else if (sMethod == "rmtch" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(strrmatch_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                               static_cast<const NumValue&>(arg2).get().asI64()-1));
        else if (sMethod == "nmtch" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(str_not_match_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                                   static_cast<const NumValue&>(arg2).get().asI64()-1));
        else if (sMethod == "nrmtch" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return new NumValue(str_not_rmatch_impl(m_val, static_cast<const StrValue&>(arg1).get(),
                                                    static_cast<const NumValue&>(arg2).get().asI64()-1));

        throw ParserError(ecMETHOD_ERROR, sMethod);
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator-(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator/(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
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
        if (other.m_type == TYPE_REFERENCE)
            return operator==(static_cast<const RefValue&>(other).get());

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
        if (other.m_type == TYPE_REFERENCE)
            return operator<(static_cast<const RefValue&>(other).get());

        if (other.m_type != TYPE_CATEGORY)
            throw ParserError(ecTYPE_MISMATCH, getTypeAsString(m_type) + " < " + getTypeAsString(other.m_type));

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
    /// \brief Is the passed string a method for this
    /// instance?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition CatValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        if (argc == 0 && (sMethod == "key" || sMethod == "val"))
            return MethodDefinition(sMethod);

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call a zero-argument method on this
    /// instance.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* CatValue::call(const std::string& sMethod) const
    {
        if (sMethod == "key")
            return new StrValue(m_val.name);
        else if (sMethod == "val")
            return new NumValue(m_val.val);

        throw ParserError(ecMETHOD_ERROR, sMethod);
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator-(static_cast<const RefValue&>(other).get());

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator/(static_cast<const RefValue&>(other).get());

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*(static_cast<const RefValue&>(other).get());

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
        else if (other.m_type == TYPE_REFERENCE)
            return operator^(static_cast<const RefValue&>(other).get());

        return new ArrValue(m_val ^ Value(other.clone()));
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator+=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator-=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator/=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator*=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return operator^=(static_cast<const RefValue&>(other).get());
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
        else if (other.m_type == TYPE_REFERENCE)
            return pow(static_cast<const RefValue&>(other).get());

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
        if (other.m_type == TYPE_REFERENCE)
            return operator==(static_cast<const RefValue&>(other).get());

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
        if (other.m_type == TYPE_REFERENCE)
            return operator<(static_cast<const RefValue&>(other).get());

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
    /// \brief Does the passed string correspond to a
    /// method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition ArrValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        return MethodDefinition(sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::call(const std::string& sMethod) const
    {
        return new ArrValue(m_val.call(sMethod));
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::call(const std::string& sMethod,
                              const BaseValue& arg1) const
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return call(sMethod, static_cast<const RefValue&>(arg1).get());

        return new ArrValue(m_val.call(sMethod, Value(arg1.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return call(sMethod,
                        arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                        arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        return new ArrValue(m_val.call(sMethod, Value(arg1.clone()), Value(arg2.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE)
            return call(sMethod,
                        arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                        arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                        arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3);

        return new ArrValue(m_val.call(sMethod, Value(arg1.clone()), Value(arg2.clone()), Value(arg3.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE || arg4.m_type == TYPE_REFERENCE)
            return call(sMethod,
                        arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                        arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                        arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3,
                        arg4.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg4).get() : arg4);

        return new ArrValue(m_val.call(sMethod, Value(arg1.clone()), Value(arg2.clone()), Value(arg3.clone()), Value(arg4.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to
    /// an applying method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition ArrValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        return m_val.isApplyingMethod(sMethod, argc);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::apply(const std::string& sMethod)
    {
        return new ArrValue(m_val.apply(sMethod));
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::apply(const std::string& sMethod,
                               const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        return new ArrValue(m_val.apply(sMethod, Value(arg1.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        return new ArrValue(m_val.apply(sMethod, Value(arg1.clone()), Value(arg2.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                         arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3);

        return new ArrValue(m_val.apply(sMethod, Value(arg1.clone()), Value(arg2.clone()), Value(arg3.clone())));
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* ArrValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE || arg4.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                         arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3,
                         arg4.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg4).get() : arg4);

        return new ArrValue(m_val.apply(sMethod, Value(arg1.clone()), Value(arg2.clone()), Value(arg3.clone()), Value(arg4.clone())));
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
        return m_val.print(digits, chrs, trunc);
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string (possibly adding quotation
    /// marks). Will shorten the representation.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string ArrValue::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        return m_val.printEmbedded();
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


    //------------------------------------------------------------------------------

    // Basic implementation of the DictStructValue class
    BASE_VALUE_IMPL(DictStructValue, TYPE_DICTSTRUCT, m_val)


    /////////////////////////////////////////////////
    /// \brief Is this DictStruct instance valid?
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStructValue::isValid() const
    {
        return m_val.size();
    }


    /////////////////////////////////////////////////
    /// \brief Convert to boolean.
    ///
    /// \return DictStructValue::operator
    ///
    /////////////////////////////////////////////////
    DictStructValue::operator bool() const
    {
        std::vector<std::string> fieldNames = m_val.getFields();

        for (const auto& field : fieldNames)
        {
            const BaseValue* f = m_val.read(field);

            if (!f || !bool(*f))
                return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Equality comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStructValue::operator==(const BaseValue& other) const
    {
        if (other.m_type == TYPE_REFERENCE)
            return operator==(static_cast<const RefValue&>(other).get());

        if (other.m_type != TYPE_DICTSTRUCT)
            return false;

        const DictStruct& otherDict = static_cast<const DictStructValue&>(other).get();
        std::vector<std::string> fieldNames = m_val.getFields();

        if (fieldNames != otherDict.getFields())
            return false;

        for (const auto& field : fieldNames)
        {
            const BaseValue* v = m_val.read(field);
            const BaseValue* otherV = otherDict.read(field);

            if ((!v xor !otherV) || *v != *otherV)
                return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Return the byte size of this
    /// DictStruct instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t DictStructValue::getBytes() const
    {
        size_t s = 0;
        std::vector<std::string> fieldNames = m_val.getFields();

        for (const auto& field : fieldNames)
        {
            s += field.length();
            const BaseValue* f = m_val.read(field);

            if (f)
                s += f->getBytes();
        }

        return s;
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to a
    /// method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition DictStructValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"keys", 0}, {"values", 0}, {"encodejson", 0}, {"at", 1}, {"len", 0}});

        if (m_val.isField(sMethod) && argc == 0)
            return MethodDefinition(sMethod);

        auto iter = methods.find(MethodDefinition(sMethod, argc));

        if (iter != methods.end())
            return *iter;

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no argument.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStructValue::call(const std::string& sMethod) const
    {
        if (sMethod == "keys")
            return new ArrValue(m_val.getFields());
        else if (sMethod == "values")
        {
            std::vector<std::string> fieldNames = m_val.getFields();
            Array vals;
            vals.reserve(fieldNames.size());

            for (const std::string& field : fieldNames)
            {
                const BaseValue* val = m_val.read(field);
                vals.emplace_back(val ? val->clone() : nullptr);
            }

            return new ArrValue(vals);
        }
        else if (sMethod == "encodejson")
            return new StrValue(m_val.encodeJson());
        else if (sMethod == "len")
            return new NumValue(Numerical(m_val.size()));
        else if (m_val.isField(sMethod))
        {
            const BaseValue* v = m_val.read(sMethod);
            return v ? v->clone() : nullptr;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStructValue::call(const std::string& sMethod, const BaseValue& arg1) const
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return call(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "at" && arg1.m_type == TYPE_STRING && m_val.isField(static_cast<const StrValue&>(arg1).get()))
        {
            const BaseValue* v = m_val.read(static_cast<const StrValue&>(arg1).get());
            return v ? v->clone() : nullptr;
        }
        else if (sMethod == "at"
                 && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path"
                 && m_val.isField(static_cast<const PathValue&>(arg1).get()))
        {
            const BaseValue* v = m_val.read(static_cast<const PathValue&>(arg1).get());
            return v ? v->clone() : nullptr;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to
    /// an applying method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition DictStructValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"clear", 0}, {"removekey", 1}, {"loadxml", 1}, {"loadjson", 1}, {"decodejson", 1}, {"write", -2}, {"insertkey", -1}, {"insertkey", -2}});

        if (m_val.isField(sMethod) && argc <= 1)
            return MethodDefinition(sMethod, -argc);

        auto iter = methods.find(MethodDefinition(sMethod, argc));

        if (iter != methods.end())
            return *iter;

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStructValue::apply(const std::string& sMethod)
    {
        if (sMethod == "clear")
            return new NumValue(Numerical(m_val.clear()));

        if (m_val.isField(sMethod))
            return new RefValue(m_val.read(sMethod));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStructValue::apply(const std::string& sMethod, const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "removekey" && arg1.m_type == TYPE_STRING && m_val.isField(static_cast<const StrValue&>(arg1).get()))
            return m_val.remove(static_cast<const StrValue&>(arg1).get());

        if (sMethod == "removekey"
            && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path"
            && m_val.isField(static_cast<const PathValue&>(arg1).get()))
            return m_val.remove(static_cast<const PathValue&>(arg1).get());

        if (sMethod == "loadxml" && arg1.m_type == TYPE_STRING)
            return new NumValue(Numerical(m_val.importXml(static_cast<const StrValue&>(arg1).get())));

        if (sMethod == "loadxml" && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path")
            return new NumValue(Numerical(m_val.importXml(static_cast<const PathValue&>(arg1).get().to_string('/'))));

        if (sMethod == "loadjson" && arg1.m_type == TYPE_STRING)
            return new NumValue(Numerical(m_val.importJson(static_cast<const StrValue&>(arg1).get())));

        if (sMethod == "loadjson" && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path")
            return new NumValue(Numerical(m_val.importJson(static_cast<const PathValue&>(arg1).get().to_string('/'))));

        if (sMethod == "decodejson" && arg1.m_type == TYPE_STRING)
            return new NumValue(Numerical(m_val.decodeJson(static_cast<const StrValue&>(arg1).get())));

        if (sMethod == "insertkey" && arg1.m_type == TYPE_STRING)
            return new NumValue(m_val.addKey(static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "insertkey"
                 && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path"
                 && m_val.isField(static_cast<const PathValue&>(arg1).get()))
            return new NumValue(m_val.addKey(static_cast<const PathValue&>(arg1).get()));
        else if (sMethod == "insertkey" && arg1.m_type == TYPE_ARRAY)
        {
            const Array& arr1 = static_cast<const ArrValue&>(arg1).get();
            size_t elems = arr1.size();
            Array ret;
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(new NumValue(m_val.addKey(arr1.get(i).getStr())));
            }

            return new ArrValue(ret);
        }

        if (m_val.isField(sMethod))
            return new RefValue(m_val.write(sMethod, arg1));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStructValue::apply(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        if (sMethod == "write" && arg1.m_type == TYPE_STRING && m_val.isField(static_cast<const StrValue&>(arg1).get()))
            return new RefValue(m_val.write(static_cast<const StrValue&>(arg1).get(), arg2));
        else if (sMethod == "write"
                 && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path"
                 && m_val.isField(static_cast<const PathValue&>(arg1).get()))
            return new RefValue(m_val.write(static_cast<const PathValue&>(arg1).get(), arg2));
        else if (sMethod == "write" && arg1.m_type == TYPE_ARRAY)
        {
            const Array& arr1 = static_cast<const ArrValue&>(arg1).get();
            size_t elems;
            Array ret;

            if (arg2.m_type == TYPE_ARRAY)
            {
                const Array& arr2 = static_cast<const ArrValue&>(arg2).get();
                elems = std::max(arr1.size(), arr2.size());

                ret.reserve(elems);

                for (size_t i = 0; i < elems; i++)
                {
                    if (!m_val.isField(arr1.get(i).getStr()))
                        throw ParserError(ecMETHOD_ERROR, sMethod);

                    ret.emplace_back(new RefValue(m_val.write(arr1.get(i).getStr(), *arr2.get(i).get())));
                }
            }
            else
            {
                elems = arr1.size();
                ret.reserve(elems);

                for (size_t i = 0; i < elems; i++)
                {
                    if (!m_val.isField(arr1.get(i).getStr()))
                        throw ParserError(ecMETHOD_ERROR, sMethod);

                    ret.emplace_back(new RefValue(m_val.write(arr1.get(i).getStr(), arg2)));
                }
            }

            return new ArrValue(ret);
        }

        if (sMethod == "insertkey" && arg1.m_type == TYPE_STRING)
            return new RefValue(m_val.write(static_cast<const StrValue&>(arg1).get(), arg2));
        else if (sMethod == "insertkey"
                 && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path")
            return new RefValue(m_val.write(static_cast<const PathValue&>(arg1).get(), arg2));
        else if (sMethod == "insertkey" && arg1.m_type == TYPE_ARRAY)
        {
            const Array& arr1 = static_cast<const ArrValue&>(arg1).get();
            size_t elems;
            Array ret;

            if (arg2.m_type == TYPE_ARRAY)
            {
                const Array& arr2 = static_cast<const ArrValue&>(arg2).get();
                elems = std::max(arr1.size(), arr2.size());

                ret.reserve(elems);

                for (size_t i = 0; i < elems; i++)
                {
                    ret.emplace_back(new RefValue(m_val.write(arr1.get(i).getStr(), *arr2.get(i).get())));
                }
            }
            else
            {
                elems = arr1.size();
                ret.reserve(elems);

                for (size_t i = 0; i < elems; i++)
                {
                    ret.emplace_back(new RefValue(m_val.write(arr1.get(i).getStr(), arg2)));
                }
            }

            return new ArrValue(ret);
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string
    /// adding possible quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string DictStructValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        std::vector<std::string> fieldNames = m_val.getFields();
        std::string sPrinted;

        for (const auto& field : fieldNames)
        {
            if (sPrinted.length())
                sPrinted += ", ";

            sPrinted += "." + field + ": ";
            const BaseValue* v = m_val.read(field);

            if (v)
                sPrinted += v->printEmbedded(digits, chrs, trunc);
            else
                sPrinted += "void";
        }

        return "[" + sPrinted + "]";
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance as if it was
    /// embedded.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string DictStructValue::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        return "{1 x 1 dictstruct}";
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string DictStructValue::printVal(size_t digits, size_t chrs) const
    {
        std::vector<std::string> fieldNames = m_val.getFields();
        std::string sPrinted;

        for (const auto& field : fieldNames)
        {
            if (sPrinted.length())
                sPrinted += ", ";

            sPrinted += "." + field + ": ";
            const BaseValue* v = m_val.read(field);

            if (v)
                sPrinted += v->printVal(digits, chrs);
            else
                sPrinted += "void";
        }

        return "[" + sPrinted + "]";
    }


    //------------------------------------------------------------------------------

    /////////////////////////////////////////////////
    /// \brief PathValue constructor.
    /////////////////////////////////////////////////
    PathValue::PathValue() : Object("path")
    {
        declareMethod(MethodDefinition("root", 0));
        declareMethod(MethodDefinition("leaf", 0));
        declareMethod(MethodDefinition("depth", 0));
        declareMethod(MethodDefinition("trunk", 1));
        declareMethod(MethodDefinition("branch", 1));
        declareMethod(MethodDefinition("revbranch", 1));
        declareMethod(MethodDefinition("format", 1));
        declareMethod(MethodDefinition("at", 1));
        declareMethod(MethodDefinition("sub", 1));
        declareMethod(MethodDefinition("sub", 2));

        declareApplyingMethod(MethodDefinition("pop", 0));
        declareApplyingMethod(MethodDefinition("clean", 0));
        declareApplyingMethod(MethodDefinition("clear", 0));
        declareApplyingMethod(MethodDefinition("remove", 1));
        declareApplyingMethod(MethodDefinition("remove", 2));
        declareApplyingMethod(MethodDefinition("insert", 2));
        declareApplyingMethod(MethodDefinition("write", 2));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a PathValue from another
    /// BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    PathValue::PathValue(const BaseValue& other) : PathValue()
    {
        if (Object::operator==(other))
            m_val = static_cast<const PathValue&>(other).m_val;
        else if (other.m_type == TYPE_REFERENCE && Object::operator==(static_cast<const RefValue&>(other).get()))
            m_val = static_cast<const PathValue&>(static_cast<const RefValue&>(other).get()).m_val;
        else if (other.m_type == TYPE_STRING)
            m_val = Path(static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_REFERENCE && static_cast<const RefValue&>(other).get().m_type == TYPE_STRING)
            m_val = Path(static_cast<const StrValue&>(static_cast<const RefValue&>(other).get()).get());
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign another BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return PathValue&
    ///
    /////////////////////////////////////////////////
    PathValue& PathValue::operator=(const BaseValue& other)
    {
        if (Object::operator==(other))
            m_val = static_cast<const PathValue&>(other).m_val;
        else if (other.m_type == TYPE_REFERENCE && Object::operator==(static_cast<const RefValue&>(other).get()))
            m_val = static_cast<const PathValue&>(static_cast<const RefValue&>(other).get()).m_val;
        else if (other.m_type == TYPE_STRING)
            m_val = Path(static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_REFERENCE && static_cast<const RefValue&>(other).get().m_type == TYPE_STRING)
            m_val = Path(static_cast<const StrValue&>(static_cast<const RefValue&>(other).get()).get());
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Is this path instance valid?
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool PathValue::isValid() const
    {
        return m_val.depth();
    }


    /////////////////////////////////////////////////
    /// \brief Return the byte size of this instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t PathValue::getBytes() const
    {
        size_t bytes = 0;

        for (size_t i = 0; i < m_val.depth(); i++)
        {
            bytes += m_val[i].length();
        }

        return bytes;
    }


    /////////////////////////////////////////////////
    /// \brief Leaf concatenation operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::operator+(const BaseValue& other) const
    {
        if (other.m_type == TYPE_STRING)
            return new PathValue(m_val + static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new PathValue(m_val + static_cast<const CatValue&>(other).get().name);
        else if (other.m_type == TYPE_ARRAY)
        {
            const Array& otherArr = static_cast<const ArrValue&>(other).get();
            Array ret;
            ret.reserve(otherArr.size());

            for (size_t i = 0; i < otherArr.size(); i++)
            {
                ret.emplace_back(new PathValue(m_val + otherArr.get(i).getStr()));
            }

            return new ArrValue(ret);
        }
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, "object.path + " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Path combination operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::operator/(const BaseValue& other) const
    {
        if (other.m_type == TYPE_STRING)
            return new PathValue(m_val / static_cast<const StrValue&>(other).get());
        else if (other.m_type == TYPE_CATEGORY)
            return new PathValue(m_val / static_cast<const CatValue&>(other).get().name);
        else if (Object::operator==(other))
            return new PathValue(m_val / static_cast<const PathValue&>(other).get());
        else if (other.m_type == TYPE_ARRAY)
        {
            const Array& otherArr = static_cast<const ArrValue&>(other).get();
            Array ret;
            ret.reserve(otherArr.size());

            for (size_t i = 0; i < otherArr.size(); i++)
            {
                if (otherArr.get(i).isObject() && otherArr.get(i).getObject().getObjectType() == "path")
                    ret.emplace_back(new PathValue(m_val / static_cast<const PathValue&>(otherArr.get(i).getObject()).get()));
                else
                    ret.emplace_back(new PathValue(m_val / otherArr.get(i).getStr()));
            }

            return new ArrValue(ret);
        }
        else if (other.m_type == TYPE_NEUTRAL)
            return clone();
        else if (other.m_type == TYPE_REFERENCE)
            return operator+(static_cast<const RefValue&>(other).get());

        throw ParserError(ecTYPE_MISMATCH, "object.path / " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Concat this leaf with another string.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& PathValue::operator+=(const BaseValue& other)
    {
        if (other.m_type == TYPE_STRING)
            m_val += static_cast<const StrValue&>(other).get();
        else if (other.m_type == TYPE_CATEGORY)
            m_val += static_cast<const CatValue&>(other).get().name;
        else if (other.m_type == TYPE_REFERENCE)
            return operator+=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, "object.path + " + getTypeAsString(other.m_type));

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Append a path to this path instance.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& PathValue::operator/=(const BaseValue& other)
    {
        if (other.m_type == TYPE_STRING)
            m_val /= static_cast<const StrValue&>(other).get();
        else if (other.m_type == TYPE_CATEGORY)
            m_val /= static_cast<const CatValue&>(other).get().name;
        else if (Object::operator==(other))
            m_val /= static_cast<const PathValue&>(other).get();
        else if (other.m_type == TYPE_REFERENCE)
            return operator/=(static_cast<const RefValue&>(other).get());
        else if (other.m_type != TYPE_NEUTRAL)
            throw ParserError(ecTYPE_MISMATCH, "object.path / " + getTypeAsString(other.m_type));

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool PathValue::operator==(const BaseValue& other) const
    {
        return Object::operator==(other) && static_cast<const PathValue&>(other).m_val == m_val;
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::call(const std::string& sMethod) const
    {
        if (sMethod == "root")
            return new StrValue(m_val.root());
        else if (sMethod == "leaf")
            return new StrValue(m_val.leaf());
        else if (sMethod == "depth")
            return new NumValue(m_val.depth());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::call(const std::string& sMethod, const BaseValue& arg1) const
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return call(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "trunk" && Object::operator==(arg1))
            return new PathValue(m_val.getTrunkPart(static_cast<const PathValue&>(arg1).get()));
        else if (sMethod == "branch" && Object::operator==(arg1))
            return new PathValue(m_val.getBranchPart(static_cast<const PathValue&>(arg1).get()));
        else if (sMethod == "revbranch" && Object::operator==(arg1))
            return new PathValue(m_val.getRevBranchPart(static_cast<const PathValue&>(arg1).get()));
        else if (sMethod == "format" && arg1.m_type == TYPE_STRING)
            return new StrValue(m_val.to_string(static_cast<const StrValue&>(arg1).get().front()));
        else if (sMethod == "at" && arg1.m_type == TYPE_NUMERICAL)
            return new StrValue(m_val.at(static_cast<const NumValue&>(arg1).get().asUI64()-1));
        else if (sMethod == "sub" && arg1.m_type == TYPE_NUMERICAL)
            return new PathValue(m_val.getSegment(static_cast<const NumValue&>(arg1).get().asUI64()-1));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return call(sMethod,
                        arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                        arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        if (sMethod == "sub" && arg1.m_type == TYPE_NUMERICAL && arg2.m_type == TYPE_NUMERICAL)
            return new PathValue(m_val.getSegment(static_cast<const NumValue&>(arg1).get().asUI64()-1,
                                                  static_cast<const NumValue&>(arg2).get().asUI64()));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::apply(const std::string& sMethod)
    {
        if (sMethod == "clean")
        {
            m_val.clean();
            return new NumValue(true);
        }
        else if (sMethod == "pop")
            return new StrValue(m_val.pop());
        else if (sMethod == "clear")
            return new NumValue(m_val.clear());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::apply(const std::string& sMethod, const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "remove" && arg1.m_type == TYPE_NUMERICAL)
            return new StrValue(m_val.remove(static_cast<const NumValue&>(arg1).get().asUI64()-1));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


   /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* PathValue::apply(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        if (sMethod == "write" && arg1.m_type == TYPE_NUMERICAL && arg2.m_type == TYPE_STRING)
            return new NumValue(m_val.write(static_cast<const NumValue&>(arg1).get().asUI64()-1, static_cast<const StrValue&>(arg2).get()));
        else if (sMethod == "insert" && arg1.m_type == TYPE_NUMERICAL && arg2.m_type == TYPE_STRING)
            return new NumValue(m_val.insert(static_cast<const NumValue&>(arg1).get().asUI64()-1, static_cast<const StrValue&>(arg2).get()));
        else if (sMethod == "insert" && arg1.m_type == TYPE_NUMERICAL && Object::operator==(arg2))
            return new NumValue(m_val.insert(static_cast<const NumValue&>(arg1).get().asUI64()-1, static_cast<const PathValue&>(arg2).get()));
        else if (sMethod == "remove" && arg1.m_type == TYPE_NUMERICAL && arg2.m_type == TYPE_NUMERICAL)
            return new PathValue(m_val.remove(static_cast<const NumValue&>(arg1).get().asUI64()-1,
                                              static_cast<const NumValue&>(arg2).get().asUI64()));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print this path instance into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string PathValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (m_val.depth())
        {
            std::string sSerialized;

            for (size_t i = 0; i < m_val.depth(); i++)
            {
                if (sSerialized.length())
                    sSerialized += " / ";

                sSerialized += "\"" + m_val[i] + "\"";
            }

            return "{" + sSerialized + "}";
        }

        return "{void}";
    }


    /////////////////////////////////////////////////
    /// \brief Print this path instance into a string
    /// without additional quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string PathValue::printVal(size_t digits, size_t chrs) const
    {
        if (m_val.depth())
        {
            std::string sSerialized;

            for (size_t i = 0; i < m_val.depth(); i++)
            {
                if (sSerialized.length())
                    sSerialized += "/";

                sSerialized += m_val[i];
            }

            return "{" + sSerialized + "}";
        }

        return "{void}";
    }


    //------------------------------------------------------------------------------

    /////////////////////////////////////////////////
    /// \brief FileValue constructor.
    /////////////////////////////////////////////////
    FileValue::FileValue() : Object("file")
    {
        declareMethod(MethodDefinition("isopen", 0));
        declareMethod(MethodDefinition("wpos", 0));
        declareMethod(MethodDefinition("rpos", 0));
        declareMethod(MethodDefinition("len", 0));
        declareMethod(MethodDefinition("fname", 0));
        declareMethod(MethodDefinition("mode", 0));

        declareApplyingMethod(MethodDefinition("readline", 0));
        declareApplyingMethod(MethodDefinition("close", 0));
        declareApplyingMethod(MethodDefinition("flush", 0));
        declareApplyingMethod(MethodDefinition("wpos", 1));
        declareApplyingMethod(MethodDefinition("rpos", 1));
        declareApplyingMethod(MethodDefinition("read", 1));
        declareApplyingMethod(MethodDefinition("read", 2));
        declareApplyingMethod(MethodDefinition("write", -1));
        declareApplyingMethod(MethodDefinition("write", -2));
        declareApplyingMethod(MethodDefinition("open", 1));
        declareApplyingMethod(MethodDefinition("open", 2));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a FileValue from another
    /// BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    FileValue::FileValue(const BaseValue& other) : FileValue()
    {
        if (operator==(other))
            m_val = static_cast<const FileValue&>(other).m_val;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_val = static_cast<const FileValue&>(static_cast<const RefValue&>(other).get()).m_val;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign another BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return FileValue&
    ///
    /////////////////////////////////////////////////
    FileValue& FileValue::operator=(const BaseValue& other)
    {
        if (operator==(other))
            m_val = static_cast<const FileValue&>(other).m_val;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_val = static_cast<const FileValue&>(static_cast<const RefValue&>(other).get()).m_val;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether this instance is valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool FileValue::isValid() const
    {
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Cast to bool.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    FileValue::operator bool() const
    {
        return m_val.is_open();
    }


    /////////////////////////////////////////////////
    /// \brief Return the bytes size of this
    /// instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t FileValue::getBytes() const
    {
        return m_val.length();
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* FileValue::call(const std::string& sMethod) const
    {
        if (sMethod == "isopen")
            return new NumValue(m_val.is_open());
        else if (sMethod == "wpos")
            return new NumValue(m_val.get_write_pos());
        else if (sMethod == "rpos")
            return new NumValue(m_val.get_read_pos());
        else if (sMethod == "len")
            return new NumValue(m_val.length());
        else if (sMethod == "fname")
            return new StrValue(m_val.getFileName());
        else if (sMethod == "mode")
            return new StrValue(m_val.getOpenMode());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* FileValue::apply(const std::string& sMethod)
    {
        if (sMethod == "readline")
            return new StrValue(m_val.read_line());
        else if (sMethod == "close")
            return new NumValue(m_val.close());
        else if (sMethod == "flush")
            return new NumValue(m_val.flush());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* FileValue::apply(const std::string& sMethod, const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "wpos" && arg1.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.set_write_pos(static_cast<const NumValue&>(arg1).get().asUI64()));
        else if (sMethod == "rpos" && arg1.m_type == TYPE_NUMERICAL)
            return new NumValue(m_val.set_read_pos(static_cast<const NumValue&>(arg1).get().asUI64()));
        else if (sMethod == "open" && arg1.m_type == TYPE_STRING)
            return new NumValue(m_val.open(static_cast<const StrValue&>(arg1).get()));
        else if (sMethod == "open" && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path")
            return new NumValue(m_val.open(static_cast<const PathValue&>(arg1).get()));
        else if (sMethod == "read" && arg1.m_type == TYPE_STRING)
            return m_val.read(static_cast<const StrValue&>(arg1).get());
        else if (sMethod == "write")
            return new NumValue(m_val.write(arg1));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* FileValue::apply(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return apply(sMethod,
                         arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                         arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        if (sMethod == "open" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_STRING)
            return new NumValue(m_val.open(static_cast<const StrValue&>(arg1).get(), static_cast<const StrValue&>(arg2).get()));
        else if (sMethod == "open"
                 && arg1.m_type == TYPE_OBJECT && static_cast<const Object&>(arg1).getObjectType() == "path"
                 && arg2.m_type == TYPE_STRING)
            return new NumValue(m_val.open(static_cast<const PathValue&>(arg1).get(), static_cast<const StrValue&>(arg2).get()));
        else if (sMethod == "read" && arg1.m_type == TYPE_STRING && arg2.m_type == TYPE_NUMERICAL)
            return m_val.read(static_cast<const StrValue&>(arg1).get(), static_cast<const NumValue&>(arg2).get().asUI64());
        else if (sMethod == "write" && arg2.m_type == TYPE_STRING)
            return new NumValue(m_val.write(arg1, static_cast<const StrValue&>(arg2).get()));

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance to a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return "{.isopen: " + toString(m_val.is_open()) + ", .len: " + toString(m_val.length())
            + ", .fname: \"" + m_val.getFileName() + "\", .mode: \"" + m_val.getOpenMode() + "\"}";
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance to a string
    /// without additional quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileValue::printVal(size_t digits, size_t chrs) const
    {
        return "{.isopen: " + toString(m_val.is_open()) + ", .len: " + toString(m_val.length())
            + ", .fname: " + m_val.getFileName() + ", .mode: " + m_val.getOpenMode() + "}";
    }



    //------------------------------------------------------------------------------

    /////////////////////////////////////////////////
    /// \brief StackValue constructor.
    /////////////////////////////////////////////////
    StackValue::StackValue() : Object("stack")
    {
        declareMethod(MethodDefinition("top", 0));
        declareMethod(MethodDefinition("len", 0));
        declareMethod(MethodDefinition("values", 0));

        declareApplyingMethod(MethodDefinition("pop", 0));
        declareApplyingMethod(MethodDefinition("clear", 0));
        declareApplyingMethod(MethodDefinition("push", -1));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a StackValue from another
    /// BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    StackValue::StackValue(const BaseValue& other) : StackValue()
    {
        if (operator==(other))
            m_stack = static_cast<const StackValue&>(other).m_stack;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_stack = static_cast<const StackValue&>(static_cast<const RefValue&>(other).get()).m_stack;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign another BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return StackValue&
    ///
    /////////////////////////////////////////////////
    StackValue& StackValue::operator=(const BaseValue& other)
    {
        if (operator==(other))
            m_stack = static_cast<const StackValue&>(other).m_stack;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_stack = static_cast<const StackValue&>(static_cast<const RefValue&>(other).get()).m_stack;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether this instance is valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StackValue::isValid() const
    {
        return m_stack.size();
    }


    /////////////////////////////////////////////////
    /// \brief Cast to bool.
    ///
    /// \return StackValue::operator
    ///
    /////////////////////////////////////////////////
    StackValue::operator bool() const
    {
        return m_stack.size();
    }


    /////////////////////////////////////////////////
    /// \brief Return the byte size of this instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t StackValue::getBytes() const
    {
        size_t s = 0;

        for (size_t i = 0; i < m_stack.size(); i++)
        {
            s += m_stack[i].getBytes();
        }

        return s;
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StackValue::call(const std::string& sMethod) const
    {
        if (sMethod == "top")
        {
            if (m_stack.size())
                return m_stack.back()->clone();

            return nullptr;
        }
        else if (sMethod == "values")
        {
            if (!m_stack.size())
                return nullptr;

            Array vals;
            vals.reserve(m_stack.size());

            for (int i = m_stack.size()-1; i >= 0; i--)
            {
                vals.emplace_back(m_stack[i]);
            }

            return new ArrValue(vals);
        }
        else if (sMethod == "len")
            return new NumValue(m_stack.size());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StackValue::apply(const std::string& sMethod)
    {
        if (sMethod == "pop")
        {
            if (m_stack.size())
            {
                BaseValue* val = m_stack.back().release();
                m_stack.pop_back();
                return val;
            }

            return nullptr;
        }
        else if (sMethod == "clear")
        {
            if (m_stack.size())
            {
                m_stack.clear();
                return new NumValue(true);
            }

            return new NumValue(false);
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* StackValue::apply(const std::string& sMethod, const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "push")
        {
            m_stack.emplace_back(arg1.clone());
            return new RefValue(&m_stack.back());
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StackValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        std::string top = "void";

        if (m_stack.size())
            top = m_stack.back().printEmbedded(digits, chrs, trunc);

        return "{.len: " + toString(m_stack.size()) + ", .top: " + top +"}";
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string
    /// without additional quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StackValue::printVal(size_t digits, size_t chrs) const
    {
        std::string top = "void";

        if (m_stack.size())
            top = m_stack.back().printVal(digits, chrs);

        return "{.len: " + toString(m_stack.size()) + ", .top: " + top +"}";
    }



    //------------------------------------------------------------------------------

    /////////////////////////////////////////////////
    /// \brief QueueValue constructor.
    /////////////////////////////////////////////////
    QueueValue::QueueValue() : Object("queue")
    {
        declareMethod(MethodDefinition("front", 0));
        declareMethod(MethodDefinition("back", 0));
        declareMethod(MethodDefinition("len", 0));
        declareMethod(MethodDefinition("values", 0));

        declareApplyingMethod(MethodDefinition("clear", 0));
        declareApplyingMethod(MethodDefinition("pop", 0));
        declareApplyingMethod(MethodDefinition("popback", 0));
        declareApplyingMethod(MethodDefinition("push", -1));
        declareApplyingMethod(MethodDefinition("pushfront", -1));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a QueueValue from another
    /// BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    QueueValue::QueueValue(const BaseValue& other) : QueueValue()
    {
        if (operator==(other))
            m_queue = static_cast<const QueueValue&>(other).m_queue;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_queue = static_cast<const QueueValue&>(static_cast<const RefValue&>(other).get()).m_queue;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign another BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return QueueValue&
    ///
    /////////////////////////////////////////////////
    QueueValue& QueueValue::operator=(const BaseValue& other)
    {
        if (operator==(other))
            m_queue = static_cast<const QueueValue&>(other).m_queue;
        else if (other.m_type == TYPE_REFERENCE && operator==(static_cast<const RefValue&>(other).get()))
            m_queue = static_cast<const QueueValue&>(static_cast<const RefValue&>(other).get()).m_queue;
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether this instance is valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool QueueValue::isValid() const
    {
        return m_queue.size();
    }


    /////////////////////////////////////////////////
    /// \brief Cast to bool.
    ///
    /// \return QueueValue::operator
    ///
    /////////////////////////////////////////////////
    QueueValue::operator bool() const
    {
        return m_queue.size();
    }


    /////////////////////////////////////////////////
    /// \brief Return the byte size of this instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t QueueValue::getBytes() const
    {
        size_t s = 0;

        for (size_t i = 0; i < m_queue.size(); i++)
        {
            s += m_queue[i].getBytes();
        }

        return s;
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* QueueValue::call(const std::string& sMethod) const
    {
        if (sMethod == "len")
            return new NumValue(m_queue.size());
        else if (sMethod == "front")
        {
            if (m_queue.size())
                return m_queue.front()->clone();

            return nullptr;
        }
        else if (sMethod == "back")
        {
            if (m_queue.size())
                return m_queue.back()->clone();

            return nullptr;
        }
        else if (sMethod == "values")
        {
            if (!m_queue.size())
                return nullptr;

            Array vals;
            vals.reserve(m_queue.size());

            for (size_t i = 0; i < m_queue.size();  i++)
            {
                vals.emplace_back(m_queue[i]);
            }

            return new ArrValue(vals);
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* QueueValue::apply(const std::string& sMethod)
    {
        if (sMethod == "pop")
        {
            if (m_queue.size())
            {
                BaseValue* val = m_queue.front().release();
                m_queue.pop_front();
                return val;
            }

            return nullptr;
        }
        else if (sMethod == "popback")
        {
            if (m_queue.size())
            {
                BaseValue* val = m_queue.back().release();
                m_queue.pop_back();
                return val;
            }

            return nullptr;
        }
        else if (sMethod == "clear")
        {
            if (m_queue.size())
            {
                m_queue.clear();
                return new NumValue(true);
            }

            return new NumValue(false);
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* QueueValue::apply(const std::string& sMethod, const BaseValue& arg1) // push push_front
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return apply(sMethod, static_cast<const RefValue&>(arg1).get());

        if (sMethod == "push")
        {
            m_queue.emplace_back(arg1.clone());
            return new RefValue(&m_queue.back());
        }
        else if (sMethod == "pushfront")
        {
            m_queue.emplace_front(arg1.clone());
            return new RefValue(&m_queue.front());
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string QueueValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        std::string sFront = "void";
        std::string sBack = "void";

        if (m_queue.size())
        {
            sFront = m_queue.front().printEmbedded(digits, chrs, trunc);
            sBack = m_queue.back().printEmbedded(digits, chrs, trunc);
        }

        return "{.len: " + toString(m_queue.size()) + ", .front: " + sFront + ", .back: " + sBack + "}";
    }


    /////////////////////////////////////////////////
    /// \brief Print this instance into a string
    /// without additional quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string QueueValue::printVal(size_t digits, size_t chrs) const
    {
        std::string sFront = "void";
        std::string sBack = "void";

        if (m_queue.size())
        {
            sFront = m_queue.front().printVal(digits, chrs);
            sBack = m_queue.back().printVal(digits, chrs);
        }

        return "{.len: " + toString(m_queue.size()) + ", .front: " + sFront + ", .back: " + sBack + "}";
    }
}

