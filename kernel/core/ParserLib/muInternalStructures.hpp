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

#ifndef MUINTERNALSTRUCTURES_HPP
#define MUINTERNALSTRUCTURES_HPP

#include "muStructures.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief This class represents a generalized
    /// calculation stack item, which can take other
    /// arrays as references, if they are not
    /// modified. If a stack item referencing another
    /// array is modified, the referenced data is
    /// copied to the internal buffer first (copy-on-write).
    /////////////////////////////////////////////////
    class StackItem : protected Array
    {
        private:
            Array* m_alias;
            bool m_isIndex;

            /////////////////////////////////////////////////
            /// \brief Can the calculation be optimized by
            /// swapping the operands?
            ///
            /// \param other const Array&
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool canOptimize(const Array& other)
            {
                return other.size() > m_alias->size() && other.isCommutative() && m_alias->isCommutative();
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Default constructor. Creates an empty
            /// internal array and sets the reference to a
            /// nullptr.
            /////////////////////////////////////////////////
            StackItem() : Array(), m_alias(nullptr), m_isIndex(false) {}

            /////////////////////////////////////////////////
            /// \brief Copy constructor
            ///
            /// \param other const StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem(const StackItem& other) : StackItem()
            {
                if (other.m_alias)
                    m_alias = other.m_alias;
                else
                    Array::operator=(other);

                m_isIndex = false;
            }

            /////////////////////////////////////////////////
            /// \brief MOve constructor
            ///
            /// \param other StackItem&&
            ///
            /////////////////////////////////////////////////
            StackItem(StackItem&& other) : StackItem()
            {
                if (other.m_alias)
                    m_alias = other.m_alias;
                else
                    Array::operator=(std::move(other));

                m_isIndex = other.m_isIndex;
            }

            /////////////////////////////////////////////////
            /// \brief Reference another Array instance
            /// within this StackItem instance.
            ///
            /// \param var Array*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void aliasOf(Array* var)
            {
                m_alias = var;
            }

            /////////////////////////////////////////////////
            /// \brief Check, whether this StackItem
            /// references the passed array.
            ///
            /// \param var const Array*
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isAliasOf(const Array* var)
            {
                return m_alias == var;
            }

            /////////////////////////////////////////////////
            /// \brief Mark this stack item as an index.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void makeIndex()
            {
                m_isIndex = true;
            }

            /////////////////////////////////////////////////
            /// \brief Is this stack item an index?
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isIndex() const
            {
                return m_isIndex;
            }

            /////////////////////////////////////////////////
            /// \brief Get a constant reference to the
            /// contained array. Can be the internal buffer
            /// or the referenced one.
            ///
            /// \return const Array&
            ///
            /////////////////////////////////////////////////
            const Array& get() const
            {
                if (m_alias)
                    return *m_alias;

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Get a non-const reference to the
            /// contained array. Can be the internal buffer
            /// or the referenced one.
            ///
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& getMutable()
            {
                if (m_alias)
                    return *m_alias;

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign another StackItem instance.
            /// This assignment operator does only reset the
            /// assigned-to instance but does not copy
            /// anything. It's not intended for active use
            /// but for keeping the syntax itself straight.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator=(const StackItem& other)
            {
                m_alias = nullptr;
                m_isIndex = false;

                if (other.m_alias)
                    m_alias = other.m_alias;
                else
                    Array::operator=(other);

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Move-assign another StackItem instance.
            ///
            /// \param other StackItem&&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator=(StackItem&& other)
            {
                m_alias = nullptr;

                if (other.m_alias)
                    m_alias = other.m_alias;
                else
                    Array::operator=(std::move(other));

                m_isIndex = other.m_isIndex;

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign an Array instance to the
            /// internal buffer. Will automatically reset any
            /// possible reference to another Array instance
            /// (m_alias is set to a nullptr).
            ///
            /// \param other const Array&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator=(const Array& other)
            {
                Array::operator=(other);
                m_alias = nullptr;
                m_isIndex = false;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Move an Array instance into the
            /// internal buffer. Will automatically reset any
            /// possible reference to another Array instance
            /// (m_alias is set to a nullptr).
            ///
            /// \param other Array&&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator=(Array&& other)
            {
                Array::operator=(std::move(other));
                m_alias = nullptr;
                m_isIndex = false;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Add-assign another StackItem instance.
            /// Will take care about used references and
            /// possible optimisation by swapping the order
            /// of operands.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator+=(const StackItem& other)
            {
                if (m_alias)
                {
                    if (canOptimize(other.get()))
                    {
                        const Array* buf = m_alias;
                        operator=(other.get());
                        Array::operator+=(*buf);
                        return *this;
                    }

                    operator=(*m_alias);
                }

                Array::operator+=(other.get());
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Subtract-assign another StackItem
            /// instance. Will take care about used
            /// references and possible optimisation by
            /// swapping the order of operands.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator-=(const StackItem& other)
            {
                if (m_alias)
                {
                    if (canOptimize(other.get()))
                    {
                        const Array* buf = m_alias;
                        operator=(other.get());
                        flipSign();
                        Array::operator+=(*buf);
                        return *this;
                    }

                    operator=(*m_alias);
                }

                Array::operator-=(other.get());
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Multiply-assign another StackItem
            /// instance. Will take care about used
            /// references and possible optimisation by
            /// swapping the order of operands.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator*=(const StackItem& other)
            {
                if (m_alias)
                {
                    if (canOptimize(other.get()))
                    {
                        const Array* buf = m_alias;
                        operator=(other.get());
                        Array::operator*=(*buf);
                        return *this;
                    }

                    operator=(*m_alias);
                }

                Array::operator*=(other.get());
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Divide-assign another StackItem
            /// instance. Will take care about used
            /// references and possible optimisation by
            /// swapping the order of operands.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator/=(const StackItem& other)
            {
                if (m_alias)
                    operator=(*m_alias);

                Array::operator/=(other.get());
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Power-assign another StackItem
            /// instance. Will take care about used
            /// references and possible optimisation by
            /// swapping the order of operands.
            ///
            /// \param other const StackItem&
            /// \return StackItem&
            ///
            /////////////////////////////////////////////////
            StackItem& operator^=(const StackItem& other)
            {
                if (m_alias)
                    operator=(*m_alias);

                Array::operator^=(other.get());
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Less-than comparison operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator<(const StackItem& other) const
            {
                return get() < other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Less-than-or-equal-to comparsion
            /// operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator<=(const StackItem& other) const
            {
                return get() <= other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Greater-than comparison operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator>(const StackItem& other) const
            {
                return get() > other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Greater-than-or-equal-to comparison
            /// operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator>=(const StackItem& other) const
            {
                return get() >= other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Not-equals comparison operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator!=(const StackItem& other) const
            {
                return get() != other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Equals comparison operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator==(const StackItem& other) const
            {
                return get() == other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Logical and operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator&&(const StackItem& other) const
            {
                return get() && other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Logical or operator.
            ///
            /// \param other const StackItem&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator||(const StackItem& other) const
            {
                return get() || other.get();
            }

            /////////////////////////////////////////////////
            /// \brief Implementation for the cmVARPOW* byte
            /// codes.
            ///
            /// \param var const Variable&
            /// \param N int
            /// \return void
            ///
            /////////////////////////////////////////////////
            void varPowN(const Variable& var, int N)
            {
                if (N < 1)
                {
                    operator=(var.pow(Numerical(N)));
                    return;
                }

                operator=(var);

                for (int n = 1; n < N; n++)
                {
                    Array::operator*=(var);
                }
            }

            /////////////////////////////////////////////////
            /// \brief Implementaton for the cmVARMUL byte
            /// code.
            ///
            /// \param fact const Array&
            /// \param var const Variable&
            /// \param add const Array&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void varMul(const Array& fact, const Variable& var, const Array& add)
            {
                operator=(var);
                Array::operator*=(fact);

                if (!add.isDefault())
                    Array::operator+=(add);
            }

            /////////////////////////////////////////////////
            /// \brief Implementation for the cmREVVARMUL
            /// byte code (a byte code for non-commutative
            /// values like strings).
            ///
            /// \param fact const Array&
            /// \param var const Variable&
            /// \param add const Array&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void revVarMul(const Array& fact, const Variable& var, const Array& add)
            {
                if (!add.isDefault())
                {
                    operator=(add);
                    Array::operator+=(var * fact);
                }
                else
                {
                    operator=(var);
                    Array::operator*=(fact);
                }
            }

            /////////////////////////////////////////////////
            /// \brief Implementation for the cmDIVVAR byte
            /// code.
            ///
            /// \param fact const Array&
            /// \param var const Variable&
            /// \param add const Array&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void divVar(const Array& fact, const Variable& var, const Array& add)
            {
                operator=(fact);
                Array::operator/=(var);

                if (!add.isDefault())
                    Array::operator+=(add);
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class provides an abstract way of
    /// piping an arbitrary amount of parameters into
    /// a prepared function without caring, wether
    /// the data is in a plain Array array or in a
    /// StackItem array.
    /// \note Accesses are unchecked. Data is only
    /// borrowed, not managed.
    /////////////////////////////////////////////////
    class MultiArgFuncParams
    {
        private:
            size_t m_len;
            const StackItem* m_stack;
            const Array* m_arr;

        public:
            MultiArgFuncParams() : m_len(0), m_stack(nullptr), m_arr(nullptr)
            { }

            MultiArgFuncParams(const Array* arr, size_t len = 1) : m_len(len), m_stack(nullptr), m_arr(arr)
            { }

            MultiArgFuncParams(const StackItem* stck, size_t len) : m_len(len), m_stack(stck), m_arr(nullptr)
            { }

            /////////////////////////////////////////////////
            /// \brief Get the number of provided parameters
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t count() const
            {
                return m_len;
            }

            /////////////////////////////////////////////////
            /// \brief Get access to the i-th parameter.
            ///
            /// \param i size_t
            /// \return const Array&
            ///
            /////////////////////////////////////////////////
            const Array& operator[](size_t i) const
            {
                if (i >= m_len)
                    throw std::out_of_range("MultiArgFuncParams instance provides less parameters than requested.");

                if (m_stack)
                    return m_stack[i].get();
                else if (m_arr)
                    return m_arr[i];

                throw std::runtime_error("MultiArgFuncParams instance is empty.");
            }
    };


    /////////////////////////////////////////////////
    /// \brief Make a vector from an array of
    /// StackItem instances.
    ///
    /// \param beg const StackItem*
    /// \param elems int
    /// \return std::vector<Array>
    ///
    /////////////////////////////////////////////////
    inline std::vector<Array> make_vector(const StackItem* beg, int elems)
    {
        std::vector<Array> ret(elems);

        for (size_t i = 0; i < ret.size(); i++)
        {
            ret[i] = beg[i].get();
        }

        return ret;
    }
}

#endif // MUINTERNALSTRUCTURES_HPP

