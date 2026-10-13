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

#include <memory>

#ifndef MUVALUEBASEDEFS_HPP
#define MUVALUEBASEDEFS_HPP

namespace mu
{
    class BaseValue;
    using BaseValuePtr = std::unique_ptr<BaseValue>;

    /////////////////////////////////////////////////
    /// \brief This struct wraps a BaseValuePtr into
    /// a value-lile structure.
    /////////////////////////////////////////////////
    struct BaseValueRef
    {
        BaseValuePtr m_ref;

        /////////////////////////////////////////////////
        /// \brief Default constructor.
        /////////////////////////////////////////////////
        BaseValueRef() : m_ref(nullptr)
        { }

        BaseValueRef(const BaseValue& other);
        BaseValueRef(const BaseValueRef& other);
        BaseValueRef(BaseValueRef&& other);
        BaseValueRef& operator=(const BaseValue& other);
        BaseValueRef& operator=(const BaseValueRef& other);
        BaseValueRef& operator=(BaseValueRef&& other);

        bool operator==(const BaseValue& other) const;
        bool operator==(const BaseValueRef& other) const;
        bool operator<(const BaseValue& other) const;
        bool operator<(const BaseValueRef& other) const;

        /////////////////////////////////////////////////
        /// \brief Return true, if the contained pointer
        /// is null.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool is_null() const
        {
            return !m_ref;
        }

        /////////////////////////////////////////////////
        /// \brief Dereference operator overload.
        ///
        /// \return BaseValue&
        ///
        /////////////////////////////////////////////////
        BaseValue& operator*()
        {
            return *m_ref;
        }

        /////////////////////////////////////////////////
        /// \brief Dereference operator overload.
        ///
        /// \return const BaseValue&
        ///
        /////////////////////////////////////////////////
        const BaseValue& operator*() const
        {
            return *m_ref;
        }
    };
}



#endif // MUVALUEBASEDEFS_HPP
