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
    BaseValue* BaseValue::operator+(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue* BaseValue::operator-() const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue* BaseValue::operator-(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue* BaseValue::operator/(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue* BaseValue::operator*(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue& BaseValue::operator+=(const BaseValue& other)
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue& BaseValue::operator-=(const BaseValue& other)
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue& BaseValue::operator/=(const BaseValue& other)
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    BaseValue& BaseValue::operator*=(const BaseValue& other)
    {
        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue* BaseValue::pow(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }

    BaseValue::operator bool() const
    {
        return false;
    }
    bool BaseValue::operator!() const
    {
        return !bool(*this);
    }
    bool BaseValue::operator==(const BaseValue& other) const
    {
        throw m_type == other.m_type;
    }
    bool BaseValue::operator!=(const BaseValue& other) const
    {
        return !operator==(other);
    }
    bool BaseValue::operator<(const BaseValue& other) const
    {
        throw ParserError(ecTYPE_MISMATCH);
    }
    bool BaseValue::operator<=(const BaseValue& other) const
    {
        return operator<(other) || operator==(other);
    }
    bool BaseValue::operator>(const BaseValue& other) const
    {
        return !operator<(other) && !operator==(other);
    }
    bool BaseValue::operator>=(const BaseValue& other) const
    {
        return !operator<(other);
    }

    bool BaseValue::isMethod(const std::string& sMethod) const
    {
        return false;
    }
    BaseValue* BaseValue::call(const std::string& sMethod) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }
    BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }
    BaseValue* BaseValue::call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }
}

