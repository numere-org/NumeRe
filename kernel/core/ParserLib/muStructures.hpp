/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#ifndef MUSTRUCTURES_HPP
#define MUSTRUCTURES_HPP

#include <vector>
#include <memory>

#include "muTypes.hpp"
#include "muCompositeStructures.hpp"
#include "muApply.hpp"
#include "muValueBase.hpp"
#include "muParserError.h"

namespace mu
{
    // Forward declaration of the Variable class
    class Variable;
    class Array;
    class RefArray;


    /////////////////////////////////////////////////
    /// \brief This class is an abstract value, which
    /// can be filled with a string or a numerical
    /// value (or any other value, which will be
    /// added to this implementation in the future,
    /// e.g. classes).
    /////////////////////////////////////////////////
    class Value : public std::unique_ptr<BaseValue>
    {
        public:
            Value();
            Value(const Value& data);
            //Value(Value&& data);
            Value(BaseValue* other);
            Value(const Numerical& data);
            Value(const Category& data);
            Value(const Array& data);
            Value(bool logical);
            Value(int32_t value);
            Value(uint32_t value);
            Value(int64_t value);
            Value(uint64_t value);
            Value(double value);
            Value(const sys_time_point& value);
            Value(const std::complex<float>& value);
            Value(const std::complex<double>& value, bool autoType = true);
            Value(const std::string& sData);
            Value(const char* sData);
            Value(const DictStruct& dict);
            Value(DataType type);

            /////////////////////////////////////////////////
            /// \brief Assign a Value.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator=(const Value& other)
            {
                if (!other.get())
                    reset(nullptr);
                else if (!get() || (get()->m_type != other->m_type))
                    reset(other->clone());
                else if (this != &other)
                    *get() = *other.get();

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Move-Assign a Value.
            ///
            /// \param other Value&&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            //Value& operator=(Value&& other)
            //{
            //    reset(other.release());
            //    return *this;
            //}

            DataType getType() const;
            std::string getTypeAsString() const;

            bool isVoid() const;
            bool isValid() const;
            bool isNumerical() const;
            bool isString() const;
            bool isCategory() const;
            bool isArray() const;

            std::string& getStr();
            const std::string& getStr() const;

            Numerical& getNum();
            const Numerical& getNum() const;

            Category& getCategory();
            const Category& getCategory() const;

            Array& getArray();
            const Array& getArray() const;

            std::complex<double> as_cmplx() const;

            /////////////////////////////////////////////////
            /// \brief Add operator.
            ///
            /// \param other const Value&
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator+(const Value& other) const
            {
                // Special case for optimizer
                if (!other.get())
                    return *this;

                if (get() && other.get())
                    return *get() + *other.get();

                throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " + " + other.getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Unary minus operator.
            ///
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator-() const
            {
                if (get())
                    return -(*get());

                throw ParserError(ecTYPE_MISMATCH, "-" + getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Minus operator.
            ///
            /// \param other const Value&
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator-(const Value& other) const
            {
                if (get() && other.get())
                    return *get() - *other.get();

                throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " - " + other.getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Division operator.
            ///
            /// \param other const Value&
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator/(const Value& other) const
            {
                if (get() && other.get())
                    return *get() / *other.get();

                throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " / " + other.getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Multiplication operator.
            ///
            /// \param other const Value&
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator*(const Value& other) const
            {
                // Special case for optimizer
                if (!other.get())
                    return Value();

                if (get() && other.get())
                    return *get() * *other.get();

                throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " * " + other.getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Power operator.
            ///
            /// \param other const Value&
            /// \return Value
            ///
            /////////////////////////////////////////////////
            Value operator^(const Value& other) const
            {
                if (get() && other.get())
                    return *get() ^ *other.get();

                throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " ^ " + other.getTypeAsString());
            }

            /////////////////////////////////////////////////
            /// \brief Add-assign operator.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator+=(const Value& other)
            {
                if (!get())
                    reset(other->clone());
                else if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->getType(), other->getType()))
                        return operator=(*this + other);

                    *get() += *other.get();
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Subtract-assign operator.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator-=(const Value& other)
            {
                if (!get())
                    return operator=(-other);

                if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->getType(), other->getType()))
                        return operator=(*this - other);

                    *get() -= *other.get();
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Divide-assign operator.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator/=(const Value& other)
            {
                if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->getType(), other->getType()))
                        return operator=(*this / other);

                    *get() /= *other.get();
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Multiply-assign operator.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator*=(const Value& other)
            {
                if (get() && other.get())
                {
                    DataType thisType = get()->getType();
                    DataType otherType = other->getType();

                    if (nonRecursiveOps(thisType, otherType)
                        || (thisType == TYPE_NUMERICAL && otherType == TYPE_STRING))
                        return operator=(*this * other);

                    *get() *= *other.get();
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Power-assign operator.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& operator^=(const Value& other)
            {
                if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->getType(), other->getType()))
                        return operator=(*this ^ other);

                    *get() ^= *other.get();
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Flip the sign bit (if any) as fast as
            /// possible.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void flipSign()
            {
                if (get())
                    get()->flipSign();
            }

            Value pow(const Value& exponent) const;

            operator bool() const;

            Value operator!() const;
            Value operator==(const Value& other) const;
            Value operator!=(const Value& other) const;
            Value operator<(const Value& other) const;
            Value operator<=(const Value& other) const;
            Value operator>(const Value& other) const;
            Value operator>=(const Value& other) const;

            Value operator&&(const Value& other) const;
            Value operator||(const Value& other) const;

            bool isMethod(const std::string& sMethod) const;

            Value call(const std::string& sMethod) const;
            Value call(const std::string& sMethod, const Value& arg1) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4) const;

            bool isApplyingMethod(const std::string& sMethod) const;

            Value apply(const std::string& sMethod);
            Value apply(const std::string& sMethod, const Value& arg1);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4);

            std::string print(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printVal(size_t digits = 0, size_t chrs = 0) const;
            void clear();
            size_t getBytes() const;

        protected:
            static const Numerical m_defVal;
            static const std::string m_defString;
    };


    /*
    class BaseArray {...}
    class Array : public BaseArray, public std::vector<Value> {...}
    class NumArray : public BaseArray, public std::vector<Numerical> {...}
    class StrArray : public BaseArray, public std::vector<string> {...}
    */

    /////////////////////////////////////////////////
    /// \brief This class handles the scalar-vector
    /// interactions and is the general datatype
    /// within the parser. It is an extended
    /// std::vector with mu::Value as underlying
    /// template parameter and a bunch of customized
    /// operators.
    /////////////////////////////////////////////////
    class Array : public std::vector<Value> // Potential: have dedicated NumArray and StrArray variants or convert Value in an abstract class
    {
        public:
            Array();
            Array(const Array& other);
            Array(Array&& other) = default;

            Array(size_t n, const Value& fillVal = Value());
            Array(const Value& scalar);
            Array(const Variable& var);
            Array(const std::vector<std::complex<double>>& other);
            Array(const std::vector<double>& other);
            Array(const std::vector<size_t>& other);
            Array(const std::vector<int64_t>& other);
            Array(const std::vector<Numerical>& other);
            Array(const std::vector<std::string>& other);
            Array(const Array& fst, const Array& lst);
            Array(const Array& fst, const Array& inc, const Array& lst);

            /////////////////////////////////////////////////
            /// \brief Assign an Array.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator=(const Array& other)
            {
                if (other.size() == 1)
                {
                    if (other.front().isArray())
                        return operator=(other.front().getArray());

                    if (size() != 1)
                        resize(1);

                    front() = other.front();
                }
                else
                {
                    resize(other.size());

                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) = other[i];
                    }
                }

                m_commonType = other.m_commonType;
                m_isConst = other.m_isConst;

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Move an Array.
            ///
            /// \param other Array&&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator=(Array&& other)
            {
                if (other.size() == 1 && other.front().isArray())
                {
                    Array& fst = other.front().getArray();
                    std::swap(_M_impl._M_start, fst._M_impl._M_start);
                    std::swap(_M_impl._M_finish, fst._M_impl._M_finish);
                    std::swap(_M_impl._M_end_of_storage, fst._M_impl._M_end_of_storage);
                    std::swap(m_commonType, fst.m_commonType);
                    std::swap(m_isConst, fst.m_isConst);
                }
                else
                {
                    std::swap(_M_impl._M_start, other._M_impl._M_start);
                    std::swap(_M_impl._M_finish, other._M_impl._M_finish);
                    std::swap(_M_impl._M_end_of_storage, other._M_impl._M_end_of_storage);
                    std::swap(m_commonType, other.m_commonType);
                    std::swap(m_isConst, other.m_isConst);
                }

                return *this;
            }

            std::vector<DataType> getType() const;
            DataType getCommonType() const;
            std::string getCommonTypeAsString() const;
            NumericalType getCommonNumericalType() const;
            bool isScalar() const;
            bool isDefault() const;

            /////////////////////////////////////////////////
            /// \brief Is this a constant instance?
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isConst() const
            {
                return m_isConst;
            }

            /////////////////////////////////////////////////
            /// \brief Add operator.
            ///
            /// \param other const Array&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator+(const Array& other) const
            {
                Array ret;
                size_t elements = std::max(size(), other.size());
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(get(i) + other.get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Unary minus operator.
            ///
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator-() const
            {
                Array ret;
                size_t elements = size();
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(-get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Subtract operator.
            ///
            /// \param other const Array&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator-(const Array& other) const
            {
                Array ret;

                size_t elements = std::max(size(), other.size());
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(get(i) - other.get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Divide operator.
            ///
            /// \param other const Array&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator/(const Array& other) const
            {
                Array ret;
                size_t elements = std::max(size(), other.size());
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(get(i) / other.get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Multiply operator.
            ///
            /// \param other const Array&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array operator*(const Array& other) const
            {
                Array ret;
                size_t elements = std::max(size(), other.size());
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(get(i) * other.get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Power operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array operator^(const Array& other) const
            {
                Array ret;
                size_t elements = std::max(size(), other.size());
                ret.reserve(elements);

                for (size_t i = 0; i < elements; i++)
                {
                    ret.emplace_back(get(i) ^ other.get(i));
                }

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Add-assign operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator+=(const Array& other)
            {
                if (size() < other.size())
                    operator=(operator+(other));
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) += other.get(i);
                    }
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Subtract-assign operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator-=(const Array& other)
            {
                if (size() < other.size())
                    operator=(operator-(other));
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) -= other.get(i);
                    }
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Divide-assign operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator/=(const Array& other)
            {
                if (size() < other.size())
                    operator=(operator/(other));
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) /= other.get(i);
                    }
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Multiply-assign operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator*=(const Array& other)
            {
                if (size() < other.size())
                    operator=(operator*(other));
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) *= other.get(i);
                    }
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Power-assign operator.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& operator^=(const Array& other)
            {
                if (size() < other.size())
                    operator=(operator^(other));
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        operator[](i) ^= other.get(i);
                    }
                }

                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Flip the sign bit (if any) as fast as
            /// possible.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void flipSign()
            {
                for (size_t i = 0; i < size(); i++)
                {
                    operator[](i).flipSign();
                }
            }

            Array pow(const Array& exponent) const;
            Array pow(const Numerical& exponent) const;

            Array operator!() const;
            Array operator==(const Array& other) const;
            Array operator!=(const Array& other) const;
            Array operator<(const Array& other) const;
            Array operator<=(const Array& other) const;
            Array operator>(const Array& other) const;
            Array operator>=(const Array& other) const;

            Array operator&&(const Array& other) const;
            Array operator||(const Array& other) const;

            Array unWrap() const;

            bool isMethod(const std::string& sMethod) const;

            Array call(const std::string& sMethod) const;
            Array call(const std::string& sMethod, const Array& arg1) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4) const;

            bool isApplyingMethod(const std::string& sMethod) const;

            Array apply(const std::string& sMethod);
            Array apply(const std::string& sMethod, const Array& arg1);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4);

            int64_t getAsScalarInt() const;

            std::vector<std::string> as_str_vector() const;
            std::vector<std::complex<double>> as_cmplx_vector() const;
            std::vector<std::string> to_string() const;
            std::string print(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printVals(size_t digits = 0, size_t chrs = 0) const;
            std::string printDims() const;
            std::string printJoined(const std::string& sSep = "", bool keepEmpty = false) const;
            std::string printOverview(size_t digits = 0, size_t chrs = 0, size_t maxElems = 5, bool alwaysBraces = false) const;
            size_t getBytes() const;

            /////////////////////////////////////////////////
            /// \brief Get the i-th element.
            ///
            /// \param i size_t
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& get(size_t i)
            {
                if (size() == 1u)
                    return front();
                else if (size() <= i)
                    throw std::length_error("Element " + std::to_string(i) + " is out of bounds.");

                return operator[](i);
            }

            /////////////////////////////////////////////////
            /// \brief Get the i-th element.
            ///
            /// \param i size_t
            /// \return const Value&
            ///
            /////////////////////////////////////////////////
            const Value& get(size_t i) const
            {
                if (size() == 1u)
                    return front();
                else if (size() <= i)
                    return m_default;

                return operator[](i);
            }

            void zerosToVoid();
            bool isCommutative() const;

        protected:
            mutable DataType m_commonType;
            static const Value m_default;
            bool m_isConst;
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a variable which
    /// is an extension to mu::Array, where
    /// overwriting has been restricted to keeping
    /// the general data type.
    /////////////////////////////////////////////////
    class Variable : public Array
    {
        public:
            Variable();
            Variable(const Value& data);
            Variable(const Array& data);
            Variable(const Variable& data);
            //Variable(Array&& data);
            //Variable(Variable&& data) = default;

            Variable& operator=(const Value& other);

            /////////////////////////////////////////////////
            /// \brief Assign an Array checking for type
            /// compatibility.
            ///
            /// \param other const Array&
            /// \return Variable&
            ///
            /////////////////////////////////////////////////
            Variable& operator=(const Array& other)
            {
                if (getCommonType() == TYPE_VOID || (getCommonType() == other.getCommonType() && getCommonType() != TYPE_MIXED))
                {
#warning FIXME (numere#1#06/17/25): Variables must not contain references
                    Array::operator=(other);
                    m_isConst = false;
                    return *this;
                }

                throw ParserError(ecASSIGNED_TYPE_MISMATCH);
            }

            Variable& operator=(const Variable& other);

            void overwrite(const Array& other);
    };


    /////////////////////////////////////////////////
    /// \brief This class is a handler for an array
    /// of variables, which can be used within result
    /// assignment, where the results are distributed
    /// accordingly. This class is only intended for
    /// parser-internal use.
    /////////////////////////////////////////////////
    class VarArray : public std::vector<Variable*>
    {
        public:
            VarArray() = default;
            VarArray(Variable* var);
            VarArray(const VarArray& other) = default;
            //VarArray(VarArray&& other) = default;
            VarArray& operator=(const VarArray& other) = default;

            /////////////////////////////////////////////////
            /// \brief Assign the values of an Array.
            ///
            /// \param values const Array&
            /// \return const Array&
            ///
            /////////////////////////////////////////////////
            const Array& operator=(const Array& values)
            {
                if (size() == 1)
                    *front() = values;
                else if (values.isScalar())
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        *operator[](i) = values;
                    }
                }
                else
                {
                    for (size_t i = 0; i < std::min(size(), values.size()); i++)
                    {
                        *operator[](i) = values[i];
                    }
                }

                return values;
            }

            Array operator+=(const Array& values);
            Array operator-=(const Array& values);
            Array operator*=(const Array& values);
            Array operator/=(const Array& values);
            Array pow(const Array& values);
            VarArray& operator=(const std::vector<Array>& arrayList);

            bool operator==(const VarArray& other) const;

            bool isNull() const;

            /////////////////////////////////////////////////
            /// \brief Return wether this VarArray is in fact
            /// a scalar (i.e. a single variable).
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isScalar() const
            {
                return size() == 1u;
            }

            std::string print() const;
            Array asArray() const;
    };


    bool all(const Array& arr);
    bool any(const Array& arr);

    Array operator+(const Array& arr, const Value& v);
    Array operator-(const Array& arr, const Value& v);
    Array operator*(const Array& arr, const Value& v);
    Array operator/(const Array& arr, const Value& v);

    Array operator+(const Value& v, const Array& arr);
    Array operator-(const Value& v, const Array& arr);
    Array operator*(const Value& v, const Array& arr);
    Array operator/(const Value& v, const Array& arr);

    Array operator+(const Array& arr, double v);
    Array operator-(const Array& arr, double v);
    Array operator*(const Array& arr, double v);
    Array operator/(const Array& arr, double v);

    Array operator+(double v, const Array& arr);
    Array operator-(double v, const Array& arr);
    Array operator*(double v, const Array& arr);
    Array operator/(double v, const Array& arr);
}

#endif // MUSTRUCTURES_HPP

