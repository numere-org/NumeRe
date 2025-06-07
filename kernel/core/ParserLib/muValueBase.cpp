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

#include "muValueBase.hpp"
#include "muParserError.h"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Addition operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator+(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator-() const
    {
        throw ParserError(ecNOT_IMPLEMENTED, "-" + getTypeAsString(m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator-(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator/(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator*(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Power operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator^(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator+=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator-=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator/=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator*=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator^=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void BaseValue::flipSign()
    {
        throw ParserError(ecNOT_IMPLEMENTED, "-" + getTypeAsString(m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Power function.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::pow(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value. Has to be overridden, if comparisons
    /// shall work.
    ///
    /// \return BaseValue::operator
    ///
    /////////////////////////////////////////////////
    BaseValue::operator bool() const
    {
        return false;
    }

    /////////////////////////////////////////////////
    /// \brief Operator NOT.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator!() const
    {
        return !bool(*this);
    }

    /////////////////////////////////////////////////
    /// \brief Equal comparison operator. Has to be
    /// overridden, if comparisons shall work.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator==(const BaseValue& other) const
    {
        throw m_type == other.m_type;
    }

    /////////////////////////////////////////////////
    /// \brief Not-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator!=(const BaseValue& other) const
    {
        return !operator==(other);
    }

    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator. Has to
    /// be overridden, if comparisons shall work.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator<(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " < " + getTypeAsString(other.m_type));
    }

    /////////////////////////////////////////////////
    /// \brief Less-or-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator<=(const BaseValue& other) const
    {
        return operator<(other) || operator==(other);
    }

    /////////////////////////////////////////////////
    /// \brief Greater-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator>(const BaseValue& other) const
    {
        return !operator<(other) && !operator==(other);
    }

    /////////////////////////////////////////////////
    /// \brief Greater-or-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator>=(const BaseValue& other) const
    {
        return !operator<(other);
    }

    /////////////////////////////////////////////////
    /// \brief Method to detect, whether a method
    /// with the passed name is implemented in this
    /// instance. Can also be used to detect possible
    /// fields.
    ///
    /// \param sMethod const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::isMethod(const std::string& sMethod) const
    {
        return false;
    }

    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }

    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }

    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }
}

