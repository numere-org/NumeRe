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
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StrValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"len", 0}, {"first", 0}, {"last", 0},
                                        {"at", 1}, {"startsw", 1}, {"endsw", 1},
                                        {"sub", 1}, {"sub", 2}, {"splt", 1}, {"splt", 2},
                                        {"fnd", 1}, {"fnd", 2}, {"rfnd", 1}, {"rfnd", 2},
                                        {"mtch", 1}, {"mtch", 2}, {"rmtch", 1}, {"rmtch", 2},
                                        {"nmtch", 1}, {"nmtch", 2}, {"nrmtch", 1}, {"nrmtch", 2}});

        return methods.contains(MethodDefinition(sMethod, argc));
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
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool CatValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        return argc == 0 && (sMethod == "key" || sMethod == "val");
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
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ArrValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        return true;
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
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ArrValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
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
        return "{" + m_val.print(digits, chrs, trunc) + "}";
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
        std::vector<std::string> fieldNames = m_val.getFields();

        for (const auto& field : fieldNames)
        {
            const BaseValue* f = m_val.read(field);

            if (!f || !f->isValid())
                return false;
        }

        return true;
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
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStructValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"keys", 0}, {"values", 0}, {"at", 1}, {"len", 1}});
        return (m_val.isField(sMethod) && argc == 0) || methods.contains(MethodDefinition(sMethod, argc));
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

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to
    /// an applying method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStructValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"clr", 0}, {"rem", 1}, {"loadxml", 1}, {"loadjson", 1}, {"wrt", 2}, {"ins", 2}});
        return (m_val.isField(sMethod) && argc <= 1) || methods.contains(MethodDefinition(sMethod, argc));
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
        if (sMethod == "clr")
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

        if (sMethod == "rem" && arg1.m_type == TYPE_STRING && m_val.isField(static_cast<const StrValue&>(arg1).get()))
            return m_val.remove(static_cast<const StrValue&>(arg1).get());

        if (sMethod == "loadxml" && arg1.m_type == TYPE_STRING)
            return new NumValue(Numerical(m_val.importXml(static_cast<const StrValue&>(arg1).get())));

        if (sMethod == "loadjson" && arg1.m_type == TYPE_STRING)
            return new NumValue(Numerical(m_val.importJson(static_cast<const StrValue&>(arg1).get())));

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

        if (sMethod == "wrt" && arg1.m_type == TYPE_STRING && m_val.isField(static_cast<const StrValue&>(arg1).get()))
            return new RefValue(m_val.write(static_cast<const StrValue&>(arg1).get(), arg2));

        if (sMethod == "ins" && arg1.m_type == TYPE_STRING)
            return new RefValue(m_val.write(static_cast<const StrValue&>(arg1).get(), arg2));

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




}

