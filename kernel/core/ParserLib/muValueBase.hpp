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

#ifndef MUVALUEBASE_HPP
#define MUVALUEBASE_HPP

#include "muTypes.hpp"
#include <memory>
#include <set>


namespace mu
{
    /////////////////////////////////////////////////
    /// \brief This class is an abstract base class
    /// with some default implementations (i.e.
    /// throws errors) to signal missing
    /// implementations. It dispatches all operations
    /// to derived child classes, if they override
    /// the corresponding base operations.
    /////////////////////////////////////////////////
    class BaseValue
    {
        public:
            DataType m_type;

            virtual ~BaseValue() {}

            /////////////////////////////////////////////////
            /// \brief Get the contained data type.
            ///
            /// \return DataType
            ///
            /////////////////////////////////////////////////
            virtual DataType getType() const
            {
                return m_type;
            }

            /////////////////////////////////////////////////
            /// \brief Assignment operator. Will be
            /// auto-implemented by the macro below.
            ///
            /// \param other const BaseValue&
            /// \return BaseValue&
            ///
            /////////////////////////////////////////////////
            virtual BaseValue& operator=(const BaseValue& other) = 0;

            /////////////////////////////////////////////////
            /// \brief Clone this instance. Will be
            /// auto-implemented by the macro below.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            virtual BaseValue* clone() const = 0;

            virtual BaseValue* operator+(const BaseValue& other) const;
            virtual BaseValue* operator-() const;
            virtual BaseValue* operator-(const BaseValue& other) const;
            virtual BaseValue* operator/(const BaseValue& other) const;
            virtual BaseValue* operator*(const BaseValue& other) const;
            virtual BaseValue* operator^(const BaseValue& other) const;

            virtual BaseValue& operator+=(const BaseValue& other);
            virtual BaseValue& operator-=(const BaseValue& other);
            virtual BaseValue& operator/=(const BaseValue& other);
            virtual BaseValue& operator*=(const BaseValue& other);
            virtual BaseValue& operator^=(const BaseValue& other);

            virtual void flipSign();

            virtual BaseValue* pow(const BaseValue& other) const;

            /////////////////////////////////////////////////
            /// \brief Determine, whether this instance
            /// contains a valid value. That's typically used
            /// for detecting NaN and such.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            virtual bool isValid() const = 0;

            virtual operator bool() const;
            virtual bool operator!() const;
            virtual bool operator==(const BaseValue& other) const;
            virtual bool operator!=(const BaseValue& other) const;
            virtual bool operator<(const BaseValue& other) const;
            virtual bool operator<=(const BaseValue& other) const;
            virtual bool operator>(const BaseValue& other) const;
            virtual bool operator>=(const BaseValue& other) const;

            /////////////////////////////////////////////////
            /// \brief Get the size of the contained value
            /// in bytes.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            virtual size_t getBytes() const = 0;

            virtual bool isMethod(const std::string& sMethod, size_t argc) const;
            virtual BaseValue* call(const std::string& sMethod) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const;

            virtual bool isApplyingMethod(const std::string& sMethod, size_t argc) const;
            virtual BaseValue* apply(const std::string& sMethod);
            virtual BaseValue* apply(const std::string& sMethod,
                                    const BaseValue& arg1);
            virtual BaseValue* apply(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2);
            virtual BaseValue* apply(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3);
            virtual BaseValue* apply(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4);

            virtual std::string printEmbedded(size_t digits, size_t chrs, bool trunc) const;

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
            virtual std::string print(size_t digits, size_t chrs, bool trunc) const = 0;

            /////////////////////////////////////////////////
            /// \brief Print the contained value into a
            /// std::string (without any additional quotation
            /// marks).
            ///
            /// \param digits size_t
            /// \param chrs size_t
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string printVal(size_t digits, size_t chrs) const = 0;
    };

    using BaseValuePtr = std::unique_ptr<BaseValue>;

    /////////////////////////////////////////////////
    /// \brief This class represents a reference to
    /// another value. The referenced value is only
    /// borrowed and not managed.
    /////////////////////////////////////////////////
    class RefValue : public BaseValue
    {
        private:
            BaseValuePtr* m_ptr;

        public:
            /////////////////////////////////////////////////
            /// \brief Construct an empty reference.
            /////////////////////////////////////////////////
            RefValue() : BaseValue(), m_ptr(nullptr)
            {
                m_type = TYPE_REFERENCE;
            }

            /////////////////////////////////////////////////
            /// \brief Construct a reference to another value.
            ///
            /// \param val BaseValuePtr*
            ///
            /////////////////////////////////////////////////
            RefValue(BaseValuePtr* val) : BaseValue(), m_ptr(val)
            {
                m_type = TYPE_REFERENCE;
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor
            ///
            /// \return other const RefValue&
            ///
            /////////////////////////////////////////////////
            RefValue(const RefValue& other): BaseValue()
            {
                m_type = other.m_type;
                m_ptr = other.m_ptr;
            }

            RefValue(RefValue&& other) = default;
            RefValue(const BaseValue& other);
            RefValue& operator=(const BaseValue& other) override;

            /////////////////////////////////////////////////
            /// \brief Assign another value.
            ///
            /// \param val BaseValuePtr*
            /// \return RefValue&
            ///
            /////////////////////////////////////////////////
            RefValue& operator=(BaseValuePtr* val)
            {
                m_ptr = val;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign another reference.
            ///
            /// \param other const RefValue&
            /// \return RefValue&
            ///
            /////////////////////////////////////////////////
            RefValue& operator=(const RefValue& other)
            {
                m_ptr = other.m_ptr;
                return *this;
            }

            RefValue& operator=(RefValue&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Return the embedded data type.
            ///
            /// \return DataType
            ///
            /////////////////////////////////////////////////
            DataType getType() const override
            {
                return !isNull() ? get().getType() : TYPE_REFERENCE;
            }

            /////////////////////////////////////////////////
            /// \brief Clone this reference.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new RefValue(*this);
            }

            BaseValue& get();
            const BaseValue& get() const;

            BaseValue* operator+(const BaseValue& other) const override;
            BaseValue* operator-() const override;
            BaseValue* operator-(const BaseValue& other) const override;
            BaseValue* operator/(const BaseValue& other) const override;
            BaseValue* operator*(const BaseValue& other) const override;
            BaseValue* operator^(const BaseValue& other) const override;

            BaseValue& operator+=(const BaseValue& other) override;
            BaseValue& operator-=(const BaseValue& other) override;
            BaseValue& operator/=(const BaseValue& other) override;
            BaseValue& operator*=(const BaseValue& other) override;
            BaseValue& operator^=(const BaseValue& other) override;

            void flipSign() override;

            BaseValue* pow(const BaseValue& other) const override;

            bool isNull() const;
            bool isValid() const override;

            operator bool() const override;
            bool operator==(const BaseValue& other) const override;
            bool operator<(const BaseValue& other) const override;

            size_t getBytes() const override;

            bool isMethod(const std::string& sMethod, size_t argc) const override;
            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const override;

            bool isApplyingMethod(const std::string& sMethod, size_t argc) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod,
                             const BaseValue& arg1) override;
            BaseValue* apply(const std::string& sMethod,
                             const BaseValue& arg1, const BaseValue& arg2) override;
            BaseValue* apply(const std::string& sMethod,
                             const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) override;
            BaseValue* apply(const std::string& sMethod,
                             const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief Defines a single method combined with
    /// (one of) the possible argument counts.
    /////////////////////////////////////////////////
    struct MethodDefinition
    {
        std::string name;
        size_t argc;

        MethodDefinition(const std::string& _name, size_t _argc) : name(_name), argc(_argc)
        { }

        bool operator==(const MethodDefinition& def) const
        {
            return name == def.name && argc == def.argc;
        }

        bool operator<(const MethodDefinition& def) const
        {
            return name < def.name || (name == def.name && argc < def.argc);
        }
    };


    using MethodSet = std::set<MethodDefinition>;


    /////////////////////////////////////////////////
    /// \brief This is a simple function to detect,
    /// which combinations of operands cannot make
    /// use of the faster recursive operations.
    ///
    /// \param lhs DataType
    /// \param rhs DataType
    /// \return bool
    ///
    /////////////////////////////////////////////////
    inline bool nonRecursiveOps(DataType lhs, DataType rhs)
    {
        return lhs == TYPE_CATEGORY || lhs == TYPE_REFERENCE || (rhs == TYPE_ARRAY && lhs != rhs);
    }
}


#define BASE_VALUE_DECL(CLASS, ID, TYPE, ATTR)                \
private:                                                      \
    TYPE ATTR;                                                \
public:                                                       \
    CLASS() : BaseValue(), ATTR()                             \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const TYPE& val) : BaseValue(), ATTR(val)           \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const CLASS& other): BaseValue()                    \
    {                                                         \
        m_type = other.m_type;                                \
        ATTR = other.ATTR;                                    \
    }                                                         \
    CLASS(CLASS&& other) = default;                           \
    CLASS(const BaseValue& other);                            \
    CLASS& operator=(const BaseValue& other) override;        \
    CLASS& operator=(const TYPE& val)                         \
    {                                                         \
        ATTR = val;                                           \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(const CLASS& other)                      \
    {                                                         \
        ATTR = other.ATTR;                                    \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(CLASS&& other) = default;                \
    BaseValue* clone() const override                         \
    {                                                         \
        return new CLASS(*this);                              \
    }                                                         \
    TYPE& get()                                               \
    {                                                         \
        return ATTR;                                          \
    }                                                         \
    const TYPE& get() const                                   \
    {                                                         \
        return ATTR;                                          \
    }

#define BASE_VALUE_IMPL(CLASS, ID, ATTR)                        \
CLASS::CLASS(const BaseValue& other) : BaseValue()              \
{                                                               \
    m_type = ID;                                                \
    if (other.m_type == ID)                                     \
        ATTR = static_cast<const CLASS&>(other).ATTR;           \
    else if (other.m_type == TYPE_REFERENCE && static_cast<const RefValue&>(other).get().m_type == ID)  \
        ATTR = static_cast<const CLASS&>(static_cast<const RefValue&>(other).get()).ATTR;               \
    else                                                        \
        throw ParserError(ecASSIGNED_TYPE_MISMATCH);            \
}                                                               \
CLASS& CLASS::operator=(const BaseValue& other)                 \
{                                                               \
    if (other.m_type == ID)                                     \
        ATTR = static_cast<const CLASS&>(other).ATTR;           \
    else if (other.m_type == TYPE_REFERENCE)                    \
        return operator=(static_cast<const RefValue&>(other).get());  \
    else                                                        \
        throw ParserError(ecASSIGNED_TYPE_MISMATCH);            \
    return *this;                                               \
}



#endif // MUVALUEBASE_HPP


