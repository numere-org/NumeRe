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
    /// \brief Defines a single method combined with
    /// (one of) the possible argument counts.
    /////////////////////////////////////////////////
    struct MethodDefinition
    {
        std::string name;
        int argc; // positive for scalars, negative for arrays

        MethodDefinition(const std::string& _name = "", int _argc = 0) : name(_name), argc(_argc)
        { }

        bool operator==(const MethodDefinition& def) const
        {
            return name == def.name && std::abs(argc) == std::abs(def.argc);
        }

        bool operator<(const MethodDefinition& def) const
        {
            return name < def.name || (name == def.name && std::abs(argc) < std::abs(def.argc));
        }

        operator bool() const
        {
            return name.length();
        }

        bool receiveArray() const
        {
            return argc < 0;
        }
    };

    using MethodSet = std::set<MethodDefinition>;


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

            virtual MethodDefinition isMethod(const std::string& sMethod, size_t argc) const;
            virtual BaseValue* call(const std::string& sMethod) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const;
            virtual BaseValue* call(const std::string& sMethod,
                                    const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const;

            virtual MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const;
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
    /// \brief This class is an abstract base for any
    /// advanced object.
    /////////////////////////////////////////////////
    class Object : public BaseValue
    {
        protected:
            std::string m_objectType;
            MethodSet m_methods;
            MethodSet m_applyingMethods;

            void declareMethod(const MethodDefinition& def);
            void declareApplyingMethod(const MethodDefinition& def);

        public:
            Object(const std::string& objectType = "void");
            Object(const Object& other);
            std::string getObjectType() const;

            bool operator==(const BaseValue& other) const;

            MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
            MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const override;

            std::string printEmbedded(size_t digits, size_t chrs, bool trunc) const override;

            // virtual BaseValue& operator=(const BaseValue& other)
            // virtual BaseValue* clone() const = 0;
            // virtual bool isValid() const = 0;
            // virtual size_t getBytes() const = 0;
            // virtual std::string print(size_t digits, size_t chrs, bool trunc) const
            // virtual std::string printVal(size_t digits, size_t chrs) const
    };


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

            MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const override;
            BaseValue* call(const std::string& sMethod,
                            const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const override;

            MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const override;
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
    /// \brief This class is a Generator for Array
    /// expansion.
    /////////////////////////////////////////////////
    class GeneratorValue : public BaseValue
    {
        private:
            std::complex<double> m_firstVal;
            std::complex<double> m_step;
            std::complex<double> m_lastVal;
            size_t m_size;

            /////////////////////////////////////////////////
            /// \brief Determines, whether the passed step is
            /// still in valid range and therefore can be
            /// done to expand the vector.
            ///
            /// \param current const std::complex<double>&
            /// \param last const std::complex<double>&
            /// \param d const std::complex<double>&
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool stepIsStillPossible(const std::complex<double>& current, const std::complex<double>& last, const std::complex<double>& d) const
            {
                std::complex<double> fact(d.real() >= 0.0 ? 1.0 : -1.0, d.imag() >= 0.0 ? 1.0 : -1.0);

                return (current.real() * fact.real()) <= (last.real() * fact.real())
                    && (current.imag() * fact.imag()) <= (last.imag() * fact.imag());
            }

            /////////////////////////////////////////////////
            /// \brief Calculate the size of the zipped Array.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateSize()
            {
                if (std::isinf(m_lastVal.real()) || std::isinf(m_lastVal.imag()))
                {
                    m_size = -1;
                    return;
                }
                else if (std::isnan(m_firstVal.real()) || std::isnan(m_firstVal.imag())
                         || std::isnan(m_lastVal.real()) || std::isnan(m_lastVal.imag()))
                {
                    m_size = 0;
                    return;
                }

                std::complex<double> current = m_firstVal;
                m_size = 1;

                // As long as the next step is possible, add the increment
                while (stepIsStillPossible(current+m_step, m_lastVal+1e-10*m_step, m_step))
                {
                    m_size++;
                    current += m_step;
                }
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Default constructor.
            /////////////////////////////////////////////////
            GeneratorValue() : BaseValue(), m_firstVal(NAN), m_step(NAN), m_lastVal(NAN), m_size(0)
            {
                m_type = TYPE_GENERATOR;
            }

            /////////////////////////////////////////////////
            /// \brief Construct a Generator from the first
            /// and the last value of the zipped Array.
            ///
            /// \param fstVal const Numerical&
            /// \param lstVal const Numerical&
            ///
            /////////////////////////////////////////////////
            GeneratorValue(const Numerical& fstVal, const Numerical& lstVal)
                : BaseValue(), m_firstVal(fstVal.asCF64()), m_lastVal(lstVal.asCF64()), m_size(0)
            {
                m_type = TYPE_GENERATOR;
                m_step = m_lastVal - m_firstVal;
                m_step.real(m_step.real() > 0.0 ? 1.0 : (m_step.real() < 0.0 ? -1.0 : 0.0));
                m_step.imag(m_step.imag() > 0.0 ? 1.0 : (m_step.imag() < 0.0 ? -1.0 : 0.0));
                calculateSize();
            }

            /////////////////////////////////////////////////
            /// \brief Construct a Generator from the first,
            /// the last and the increment value of the
            /// zipped Array.
            ///
            /// \param fstVal const Numerical&
            /// \param increment const Numerical&
            /// \param lstVal const Numerical&
            ///
            /////////////////////////////////////////////////
            GeneratorValue(const Numerical& fstVal, const Numerical& increment, const Numerical& lstVal)
                : BaseValue(), m_firstVal(fstVal.asCF64()), m_step(increment.asCF64()), m_lastVal(lstVal.asCF64()), m_size(0)
            {
                m_type = TYPE_GENERATOR;
                calculateSize();
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            ///
            /// \param other const GeneratorValue&
            ///
            /////////////////////////////////////////////////
            GeneratorValue(const GeneratorValue& other) : BaseValue()
            {
                m_type = other.m_type;
                m_firstVal = other.m_firstVal;
                m_step = other.m_step;
                m_lastVal = other.m_lastVal;
                m_size = other.m_size;
            }

            GeneratorValue(GeneratorValue&& other) = default;
            GeneratorValue(const BaseValue& other);

            /////////////////////////////////////////////////
            /// \brief Assign another Generator.
            ///
            /// \param other const GeneratorValue&
            /// \return GeneratorValue&
            ///
            /////////////////////////////////////////////////
            GeneratorValue& operator=(const GeneratorValue& other)
            {
                m_firstVal = other.m_firstVal;
                m_step = other.m_step;
                m_lastVal = other.m_lastVal;
                m_size = other.m_size;
                return *this;
            }

            GeneratorValue& operator=(GeneratorValue&& other) = default;
            GeneratorValue& operator=(const BaseValue& other);

            /////////////////////////////////////////////////
            /// \brief Clone this instance.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new GeneratorValue(*this);
            }

            /////////////////////////////////////////////////
            /// \brief Flip the sign of the generator result.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void flipSign() override
            {
                m_firstVal *= -1.0;
                m_lastVal *= -1.0;
                m_step *= -1.0;
            }

            /////////////////////////////////////////////////
            /// \brief Get the i-th value of the zipped Array.
            ///
            /// \param i size_t
            /// \return Numerical
            ///
            /////////////////////////////////////////////////
            Numerical at(size_t i) const
            {
                if (i < m_size)
                    return Numerical::autoType(m_firstVal + double(i)*m_step);

                return NAN;
            }

            /////////////////////////////////////////////////
            /// \brief Return the first value of the
            /// Generator.
            ///
            /// \return Numerical
            ///
            /////////////////////////////////////////////////
            Numerical first() const
            {
                return Numerical::autoType(m_firstVal);
            }

            /////////////////////////////////////////////////
            /// \brief Return the last value of the Generator.
            ///
            /// \return Numerical
            ///
            /////////////////////////////////////////////////
            Numerical last() const
            {
                return Numerical::autoType(m_lastVal);
            }

            /////////////////////////////////////////////////
            /// \brief Return the step value of the Generator.
            ///
            /// \return Numerical
            ///
            /////////////////////////////////////////////////
            Numerical step() const
            {
                return Numerical::autoType(m_step);
            }

            /////////////////////////////////////////////////
            /// \brief Is this Generator valid?
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isValid() const override
            {
                return m_size;
            }

            /////////////////////////////////////////////////
            /// \brief Return the necessary storage amount
            /// for this Generator.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t getBytes() const override
            {
                return (m_size > 0) * 48;
            }

            /////////////////////////////////////////////////
            /// \brief Return the size of the zipped Array.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t size() const
            {
                return m_size;
            }

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


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
        return lhs == TYPE_CATEGORY || lhs == TYPE_REFERENCE || lhs == TYPE_GENERATOR || (rhs == TYPE_ARRAY && lhs != rhs);
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


