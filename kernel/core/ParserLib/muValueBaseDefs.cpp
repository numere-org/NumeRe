/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#include "muValueBaseDefs.hpp"
#include "muValueBase.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Construct from a BaseValue instance.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValueRef::BaseValueRef(const BaseValue& other)
    {
        m_ref.reset(other.clone());
    }


    /////////////////////////////////////////////////
    /// \brief Construct froom another BaseValueRef
    /// instance.
    ///
    /// \param other const BaseValueRef&
    ///
    /////////////////////////////////////////////////
    BaseValueRef::BaseValueRef(const BaseValueRef& other)
    {
        m_ref.reset(other.m_ref->clone());
    }


    /////////////////////////////////////////////////
    /// \brief Move-constructor.
    ///
    /// \param other BaseValueRef&&
    ///
    /////////////////////////////////////////////////
    BaseValueRef::BaseValueRef(BaseValueRef&& other)
    {
        m_ref.reset(other.m_ref.release());
    }


    /////////////////////////////////////////////////
    /// \brief Copy assignment overload for a
    /// BaseValue instance.
    ///
    /// \param other const BaseValue&
    /// \return BaseValueRef&
    ///
    /////////////////////////////////////////////////
    BaseValueRef& BaseValueRef::operator=(const BaseValue& other)
    {
        m_ref.reset(other.clone());
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Copy assignment overload for a
    /// BaseValueRef instance.
    ///
    /// \param other const BaseValueRef&
    /// \return BaseValueRef&
    ///
    /////////////////////////////////////////////////
    BaseValueRef& BaseValueRef::operator=(const BaseValueRef& other)
    {
        m_ref.reset(other.m_ref->clone());
        return *this;
    }

    /////////////////////////////////////////////////
    /// \brief Move-assignment overload.
    ///
    /// \param other BaseValueRef&&
    /// \return BaseValueRef&
    ///
    /////////////////////////////////////////////////
    BaseValueRef& BaseValueRef::operator=(BaseValueRef&& other)
    {
        m_ref.reset(other.m_ref.release());
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Equality operator overload for a
    /// BaseValue.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValueRef::operator==(const BaseValue& other) const
    {
        return m_ref && (*m_ref) == other;
    }


    /////////////////////////////////////////////////
    /// \brief Equality operator overload for a
    /// BaseValueRef instance.
    ///
    /// \param other const BaseValueRef&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValueRef::operator==(const BaseValueRef& other) const
    {
        return m_ref && other.m_ref && (*m_ref) == (*other.m_ref);
    }


    /////////////////////////////////////////////////
    /// \brief Less-than relation for a BaseValue
    /// instance.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValueRef::operator<(const BaseValue& other) const
    {
        return m_ref && (*m_ref) < other;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than relation for a BaseValueRef
    /// instance.
    ///
    /// \param other const BaseValueRef&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValueRef::operator<(const BaseValueRef& other) const
    {
        return m_ref && other.m_ref && (*m_ref) < (*other.m_ref);
    }
}

