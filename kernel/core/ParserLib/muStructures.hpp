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
#include <list>
#include <numeric>

#include "muTypes.hpp"
#include "muCompositeStructures.hpp"
#include "muApply.hpp"
#include "muValueBase.hpp"
#include "muParserError.h"

#ifdef PARSERSTANDALONE
void log(const std::string& message);
#endif

namespace mu
{
    // Forward declaration of the Variable class
    class Variable;
    class Array;
    class MultiArgFuncParams;

    using DimSizes = std::vector<size_t>; ///< An alias for the dimension sizes within an Array
    using IndexTuple = std::vector<size_t>; ///< An alias for a multidimensional index


    /////////////////////////////////////////////////
    /// \brief Simple wrapper to calculate the number
    /// of elements in an Array based upon its
    /// DimSizes.
    ///
    /// \param sizes const DimSizes&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    inline size_t getNumElements(const DimSizes& sizes)
    {
        return std::accumulate(sizes.begin(), sizes.end(), 1ull, std::multiplies<size_t>());
    }


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
            Value(const Object& obj);
            Value(DataType type);

            /////////////////////////////////////////////////
            /// \brief Assign a Value.
            ///
            /// \param other const Value&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& assign(const Value& other)
            {
                if (!other.get())
                    reset(nullptr);
                else if (isRef())
                    getRef() = *other.get();
                else if (!get() || (get()->m_type != other->m_type))
                    reset(other->clone());
                else if (isObject() && other.isObject() && getObject().getObjectType() != other.getObject().getObjectType())
                    reset(other->clone());
                else if (this != &other)
                    *get() = *other.get();

                return *this;
            }

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
                else if (isObject() && other.isObject() && getObject().getObjectType() != other.getObject().getObjectType())
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
            bool isDictStruct() const;
            bool isObject() const;
            bool isRef() const;
            bool isGenerator() const;

            std::string& getStr();
            const std::string& getStr() const;
            std::string getPath(char separator = '/') const;

            Numerical& getNum();
            const Numerical& getNum() const;

            Category& getCategory();
            const Category& getCategory() const;

            Array& getArray();
            const Array& getArray() const;

            DictStruct& getDictStruct();
            const DictStruct& getDictStruct() const;

            Object& getObject();
            const Object& getObject() const;

            RefValue& getRef();
            const RefValue& getRef() const;

            const GeneratorValue& getGenerator() const;

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
                    if (nonRecursiveOps(get()->m_type, other->getType()))
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
                    if (nonRecursiveOps(get()->m_type, other->getType()))
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
                    DataType thisType = get()->m_type;
                    DataType otherType = other->getType();

                    if (nonRecursiveOps(thisType, otherType) || thisType == TYPE_STRING)
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
                    DataType thisType = get()->m_type;
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
                    if (nonRecursiveOps(get()->m_type, other->getType()))
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

            MethodDefinition isMethod(const std::string& sMethod, size_t argc) const;

            Value call(const std::string& sMethod) const;
            Value call(const std::string& sMethod, const Value& arg1) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3) const;
            Value call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4) const;
            Value call(const std::string& sMethod, const std::vector<Value>& args) const;

            MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const;

            Value apply(const std::string& sMethod);
            Value apply(const std::string& sMethod, const Value& arg1);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3);
            Value apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4);
            Value apply(const std::string& sMethod, const std::vector<Value>& args);

            std::string print(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printEmbedded(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
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

    Array matrixAdd(const Array& curr, const Array& other);
    Array matrixSub(const Array& curr, const Array& other);
    Array matrixMul(const Array& curr, const Array& other);
    Array matrixDiv(const Array& curr, const Array& other);
    Array matrixPow(const Array& curr, const Array& other);

    void matrixSelfAdd(Array& curr, const Array& other);
    void matrixSelfSub(Array& curr, const Array& other);
    void matrixSelfMul(Array& curr, const Array& other);
    void matrixSelfDiv(Array& curr, const Array& other);
    void matrixSelfPow(Array& curr, const Array& other);

    Array matrixEq(const Array& curr, const Array& other);
    Array matrixNeq(const Array& curr, const Array& other);
    Array matrixLess(const Array& curr, const Array& other);
    Array matrixLessEq(const Array& curr, const Array& other);
    Array matrixGreater(const Array& curr, const Array& other);
    Array matrixGreaterEq(const Array& curr, const Array& other);
    Array matrixAnd(const Array& curr, const Array& other);
    Array matrixOr(const Array& curr, const Array& other);




    /////////////////////////////////////////////////
    /// \brief This class handles the scalar-vector
    /// interactions and is the general datatype
    /// within the parser. It is an extended
    /// std::vector with mu::Value as underlying
    /// template parameter and a bunch of customized
    /// operators.
    /////////////////////////////////////////////////
    class Array : public std::vector<Value>
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
            Array(const std::vector<int>& other);
            Array(const std::vector<int64_t>& other);
            Array(const std::vector<Numerical>& other);
            Array(const std::vector<std::string>& other);
            Array(GeneratorValue* generator);

            /////////////////////////////////////////////////
            /// \brief Assign an Array.
            ///
            /// \param other const Array&
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& assign(const Array& other)
            {
                if (other.count() == 1 && other.m_commonType != TYPE_GENERATOR)
                {
                    if (other.first().isArray()) // was front()
                        return assign(other.first().getArray()); // was front()

                    if (size() && first().isRef()) // was front()
                    {
                        size_t elems = size();
                        for (size_t i = 0; i < elems; i++)
                        {
                            get(i).assign(other.get(i));
                        }

                        m_dimSizes = other.m_dimSizes;
                        m_commonType = other.m_commonType;
                        return *this;
                    }

                    if (size() != 1)
                        resize(1);

                    first().assign(other.first()); // was front()
                }
                else if (count() == 1 && first().isRef()) // was front()
                {
                    // Insert a complete array into a single reference
                    first().assign(other); // was front()
                    m_commonType = TYPE_ARRAY;
                    m_dimSizes = other.m_dimSizes;
                    return *this;
                }
                else
                {
                    resize(other.size());

                    for (size_t i = 0; i < size(); i++)
                    {
                        get(i).assign(other.get(i));
                    }
                }

                if (other.m_commonType == TYPE_GENERATOR)
                    m_commonType = TYPE_NUMERICAL;
                else
                    m_commonType = other.m_commonType;

                m_dimSizes = other.m_dimSizes;
                m_isConst = other.m_isConst;

                return *this;
            }

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
                    if (other.first().isArray())
                        return operator=(other.first().getArray());

                    if (size() != 1)
                        resize(1);

                    first() = other.first();
                }
                else if (other.m_commonType == TYPE_GENERATOR)
                {
                    size_t elems = other.count();
                    resize(elems);

                    for (size_t i = 0; i < elems; i++)
                    {
                        operator[](i) = other[i];
                    }
                }
                else
                {
                    size_t elems = other.size();
                    resize(elems);

                    for (size_t i = 0; i < elems; i++)
                    {
                        get(i) = other.get(i);
                    }
                }

                m_dimSizes = other.m_dimSizes;
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
                if (other.count() == 1 && other.first().isArray() && !other.first().isRef()) // was front()
                {
                    Array& fst = other.first().getArray(); // was front()
                    std::swap(_M_impl._M_start, fst._M_impl._M_start);
                    std::swap(_M_impl._M_finish, fst._M_impl._M_finish);
                    std::swap(_M_impl._M_end_of_storage, fst._M_impl._M_end_of_storage);
                    std::swap(m_commonType, fst.m_commonType);
                    std::swap(m_isConst, fst.m_isConst);
                    std::swap(m_dimSizes, fst.m_dimSizes);
                }
                else
                {
                    std::swap(_M_impl._M_start, other._M_impl._M_start);
                    std::swap(_M_impl._M_finish, other._M_impl._M_finish);
                    std::swap(_M_impl._M_end_of_storage, other._M_impl._M_end_of_storage);
                    std::swap(m_commonType, other.m_commonType);
                    std::swap(m_isConst, other.m_isConst);
                    std::swap(m_dimSizes, other.m_dimSizes);
                }

                return *this;
            }

            std::vector<DataType> getType(DataType& common) const;
            DataType getCommonType() const;
            std::string getCommonTypeAsString() const;
            NumericalType getCommonNumericalType() const;
            bool isScalar() const;
            bool isScalarDim(size_t dim) const
            {
                if (dim < m_dimSizes.size())
                    return m_dimSizes[dim] == 1ull;

                return (dim == 0 && size() == 1ull) || (dim > 0 && dim >= m_dimSizes.size());
            }
            bool isDefault() const;
            bool isVoid() const;

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
            /// \brief Return true, it this Array represents
            /// a matrix.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isMatrix() const
            {
                return m_dimSizes.size() > 1;
            }

            /////////////////////////////////////////////////
            /// \brief Return true, if this Array represents
            /// a row or column vector.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isVector() const
            {
                return m_dimSizes.size() == 1ull || (m_dimSizes.size() == 2 && (m_dimSizes[0] == 1ull || m_dimSizes[1] == 1ull));
            }

            /////////////////////////////////////////////////
            /// \brief Return true, if this Array is actually
            /// dimensionless and represents a plain array or
            /// a list.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isPlainArray() const
            {
                return size() > 1 && !m_dimSizes.size();
            }

            /////////////////////////////////////////////////
            /// \brief Make this instance mutable.
            ///
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& makeMutable()
            {
                m_isConst = false;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Make this instance const.
            ///
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& makeConst()
            {
                m_isConst = true;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Make this instance a generator.
            ///
            /// \return Array&
            ///
            /////////////////////////////////////////////////
            Array& makeGenerator()
            {
                m_commonType = TYPE_GENERATOR;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Copy the dimensions of the other array.
            ///
            /// \param other const Array&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void copyDims(const Array& other)
            {
                m_dimSizes = other.m_dimSizes;
                reserve(other.size());
            }

            /////////////////////////////////////////////////
            /// \brief Get this array's dimensions.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t getDims() const
            {
                if (!m_dimSizes.size())
                    return 1;

                return m_dimSizes.size();
            }

            /////////////////////////////////////////////////
            /// \brief Set the dimensions to a desired count.
            /// Note that this will only create the
            /// dimensions, but there are only one element
            /// per dimension with exception of the first
            /// dimension.
            ///
            /// \param dims size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setDims(size_t dims)
            {
                if (!dims)
                    return;

                m_dimSizes.resize(dims, 1ull);
                m_dimSizes.front() = size();
            }

            /////////////////////////////////////////////////
            /// \brief Get the dimension sizes of this array.
            ///
            /// \return DimSizes
            ///
            /////////////////////////////////////////////////
            DimSizes getDimSizes() const
            {
                if (!size())
                    return DimSizes({0, 0});

                if (m_dimSizes.size() < 2)
                    return DimSizes({size(), 1ull});

                return m_dimSizes;
            }

            /////////////////////////////////////////////////
            /// \brief Set the dimension sizes of this array.
            ///
            /// \param dimSizes const DimSizes&
            ///
            /////////////////////////////////////////////////
            void setDimSizes(const DimSizes& dimSizes)
            {
                m_dimSizes = dimSizes;

                if (m_dimSizes.size())
                    reserve(getNumElements(m_dimSizes));
            }

            /////////////////////////////////////////////////
            /// \brief Return the number of rows in this
            /// instance.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t rows() const
            {
                if (!m_dimSizes.size())
                    return size();

                return m_dimSizes[0];
            }

            /////////////////////////////////////////////////
            /// \brief Return the number of columns in this
            /// instance
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t cols() const
            {
                if (m_dimSizes.size() < 2ull)
                    return 1ull;

                return m_dimSizes[1];
            }

            /////////////////////////////////////////////////
            /// \brief Return the number of layers in this
            /// instance.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t layers() const
            {
                if (m_dimSizes.size() < 3ull)
                    return 1ull;

                return m_dimSizes[2];
            }

            /////////////////////////////////////////////////
            /// \brief Get the size of the corresponding
            /// dimension.
            ///
            /// \param dim int
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t getSize(int dim) const
            {
                if (dim < 0 || (dim == 0 && !m_dimSizes.size()))
                    return size();

                if (m_dimSizes.size() <= (size_t)dim)
                    return 1ull;

                return m_dimSizes[dim];
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
                if (isMatrix() || other.isMatrix())
                    return matrixAdd(*this, other);

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
                ret.m_dimSizes = m_dimSizes;

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
                if (isMatrix() || other.isMatrix())
                    return matrixSub(*this, other);

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
                if (isMatrix() || other.isMatrix())
                    return matrixDiv(*this, other);

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
                if (isMatrix() || other.isMatrix())
                    return matrixMul(*this, other);

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
                if (isMatrix() || other.isMatrix())
                    return matrixPow(*this, other);

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
                if (size() < other.size() || m_commonType == TYPE_GENERATOR)
                    operator=(operator+(other));
                else if (isMatrix() || other.isMatrix())
                {
                    matrixSelfAdd(*this, other);
                    m_commonType = TYPE_VOID;
                }
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        get(i) += other.get(i);
                    }

                    // A recursive operation may change the common type
                    m_commonType = TYPE_VOID;
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
                if (size() < other.size() || m_commonType == TYPE_GENERATOR)
                    operator=(operator-(other));
                else if (isMatrix() || other.isMatrix())
                {
                    matrixSelfSub(*this, other);
                    m_commonType = TYPE_VOID;
                }
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                       get(i) -= other.get(i);
                    }

                    // A recursive operation may change the common type
                    m_commonType = TYPE_VOID;
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
                if (size() < other.size() || m_commonType == TYPE_GENERATOR)
                    operator=(operator/(other));
                else if (isMatrix() || other.isMatrix())
                {
                    matrixSelfDiv(*this, other);
                    m_commonType = TYPE_VOID;
                }
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        get(i) /= other.get(i);
                    }

                    // A recursive operation may change the common type
                    m_commonType = TYPE_VOID;
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
                if (size() < other.size() || m_commonType == TYPE_GENERATOR)
                    operator=(operator*(other));
                else if (isMatrix() || other.isMatrix())
                {
                    matrixSelfMul(*this, other);
                    m_commonType = TYPE_VOID;
                }
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        get(i) *= other.get(i);
                    }

                    // A recursive operation may change the common type
                    m_commonType = TYPE_VOID;
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
                if (size() < other.size() || m_commonType == TYPE_GENERATOR)
                    operator=(operator^(other));
                else if (isMatrix() || other.isMatrix())
                {
                    matrixSelfPow(*this, other);
                    m_commonType = TYPE_VOID;
                }
                else
                {
                    for (size_t i = 0; i < size(); i++)
                    {
                        get(i) ^= other.get(i);
                    }

                    // A recursive operation may change the common type
                    m_commonType = TYPE_VOID;
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
                for (size_t i = 0; i < count(); i++)
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
            Array expandGenerators() const;

            MethodDefinition isMethod(const std::string& sMethod, size_t argc) const;

            Array call(const std::string& sMethod) const;
            Array call(const std::string& sMethod, const Array& arg1) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4) const;
            Array call(const std::string& sMethod, const MultiArgFuncParams& args) const;

            MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const;

            Array apply(const std::string& sMethod);
            Array apply(const std::string& sMethod, const Array& arg1);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3);
            Array apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4);
            Array apply(const std::string& sMethod, const MultiArgFuncParams& args);

            Array index(const Array& idx);
            Array index(const Array& idx) const;

            Array insertItems(const Array& idx, const Array& vals = Array());
            Array deleteItems(const Array& idx);

            Array order(const Array& opts = Array()) const;

            int64_t getAsScalarInt() const;

            std::vector<std::string> as_str_vector() const;
            std::vector<std::complex<double>> as_cmplx_vector() const;
            std::vector<int64_t> as_int_vector() const;
            std::vector<std::string> to_string() const;
            std::string print(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printEmbedded(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printVals(size_t digits = 0, size_t chrs = 0) const;
            std::string printDims() const;
            std::string printJoined(const std::string& sSep = "", bool keepEmpty = false) const;
            std::string printOverview(size_t digits = 0, size_t chrs = 0, size_t maxElems = 5, bool alwaysBraces = false) const;
            size_t getBytes() const;
            size_t size() const;
            size_t count() const;
            Value& first();
            const Value& first() const;

            /////////////////////////////////////////////////
            /// \brief Get the i-th element.
            ///
            /// \param i size_t
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& get(size_t i)
            {
                size_t vectSize = count();

                if (m_commonType == TYPE_GENERATOR)
                    return getGenerated(i);
                else if (vectSize == 1u)
                {
                    if (first().isRef() && first().isArray())
                        return first().getArray().get(i);

                    return operator[](0);
                }
                else if (vectSize <= i)
                    throw std::out_of_range("Element " + std::to_string(i) + " is out of bounds.");

                return operator[](i);
            }

            /////////////////////////////////////////////////
            /// \brief Access the element (i,j)
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& get(size_t i, size_t j)
            {
                if (j > 0 && m_dimSizes.size() != 2)
                    throw std::length_error("Dimension mismatch.");

                if (!m_dimSizes.size())
                    return get(i);

                if (i >= m_dimSizes[0] || j >= m_dimSizes[1])
                    throw std::out_of_range("Element (" + std::to_string(i) + "," + std::to_string(j) + ") is out of bounds.");

                return get(i + j*m_dimSizes[0]);
            }

            /////////////////////////////////////////////////
            /// \brief Access the element
            /// (idx[0], ..., idx[n-1])
            ///
            /// \param idx const IndexTuple&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& get(const IndexTuple& idx)
            {
                size_t minDim = std::min(m_dimSizes.size(), idx.size());

                for (size_t n = std::max(minDim, 1ull); n < idx.size(); n++)
                {
                    if (idx[n])
                        throw std::length_error("Dimension mismatch.");
                }

                if (idx[0] >= m_dimSizes[0])
                {
                    std::string dims;

                    for (size_t m = 0; m < idx.size(); m++)
                    {
                        if (dims.size())
                            dims += ",";

                        dims += std::to_string(idx[m]);
                    }

                    throw std::out_of_range("Element (" + dims + ") is out of bounds.");
                }

                size_t linIdx = idx[0];

                for (size_t n = 1; n < idx.size(); n++)
                {
                    if (idx[n])
                    {
                        if (idx[n] >= m_dimSizes[n])
                        {
                            std::string dims;

                            for (size_t m = 0; m < idx.size(); m++)
                            {
                                if (dims.size())
                                    dims += ",";

                                dims += std::to_string(idx[m]);
                            }

                            throw std::out_of_range("Element (" + dims + ") is out of bounds.");
                        }

                        linIdx += idx[n] * std::accumulate(m_dimSizes.begin(), m_dimSizes.begin()+n, 1ull, std::multiplies<size_t>());
                    }
                }

                return get(linIdx);
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
                size_t vectSize = count();

                if (m_commonType == TYPE_GENERATOR)
                    return getGenerated(i);
                else if (vectSize == 1u)
                {
                    if (first().isRef() && first().isArray())
                        return first().getArray().get(i);

                    return operator[](0);
                }
                else if (vectSize <= i)
                    return m_default;

                return operator[](i);
            }

            /////////////////////////////////////////////////
            /// \brief Get the element (i,j)
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            const Value& get(size_t i, size_t j) const
            {
                if (m_dimSizes.size() < 2 && j)
                    return m_default;

                if (m_dimSizes.size() < 2)
                    return get(i);

                if (i >= m_dimSizes[0] || j >= m_dimSizes[1])
                    return m_default;

                return get(i + j*m_dimSizes[0]);
            }

            /////////////////////////////////////////////////
            /// \brief Get the element
            /// (idx[0], ..., idx[n-1])
            ///
            /// \param idx const IndexTuple&
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            const Value& get(const IndexTuple& idx) const
            {
                size_t minDim = std::min(m_dimSizes.size(), idx.size());

                for (size_t n = std::max(minDim, 1ull); n < idx.size(); n++)
                {
                    if (idx[n])
                        return m_default;
                }

                if (minDim < 2)
                    return get(idx.front());

                if (idx[0] >= m_dimSizes[0])
                    return m_default;

                size_t linIdx = idx[0];

                for (size_t n = 1; n < minDim; n++)
                {
                    if (idx[n] >= m_dimSizes[n])
                        return m_default;

                    linIdx += idx[n] * std::accumulate(m_dimSizes.begin(), m_dimSizes.begin()+n, 1ull, std::multiplies<size_t>());
                }

                return get(linIdx);
            }

            /////////////////////////////////////////////////
            /// \brief Get a ref to the first element.
            ///
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& front()
            {
                if (empty())
                    throw std::length_error("Array is empty.");

                return get(0);
            }

            /////////////////////////////////////////////////
            /// \brief Get a const ref to the first element.
            ///
            /// \return const Value&
            ///
            /////////////////////////////////////////////////
            const Value& front() const
            {
                if (empty())
                    throw std::length_error("Array is empty.");

                return get(0);
            }

            /////////////////////////////////////////////////
            /// \brief Get a ref to the last element.
            ///
            /// \return Value&
            ///
            /////////////////////////////////////////////////
            Value& back()
            {
                if (empty())
                    throw std::length_error("Array is empty.");

                return get(size()-1);
            }

            /////////////////////////////////////////////////
            /// \brief Get a const ref to the last element.
            ///
            /// \return const Value&
            ///
            /////////////////////////////////////////////////
            const Value& back() const
            {
                if (empty())
                    throw std::length_error("Array is empty.");

                return get(size()-1);
            }

            Value& getGenerated(size_t i);
            const Value& getGenerated(size_t i) const;
            void set(size_t i, const Value& v);

            void zerosToVoid();
            bool isCommutative() const;
            bool isParallelizable() const;
            void dereference() const;

        protected:
            mutable DataType m_commonType;
            static const Value m_default;
            bool m_isConst;
            mutable std::list<Value> m_buffer;
            mutable DimSizes m_dimSizes;

            /////////////////////////////////////////////////
            /// \brief Check, whether the index is inside of
            /// the dimension sizes.
            ///
            /// \param idx const IndexTuple&
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isInside(const IndexTuple& idx) const
            {
                for (size_t i = 0; i < idx.size(); i++)
                {
                    if ((m_dimSizes.size() > i && m_dimSizes[i] <= idx[i]) || (m_dimSizes.size() <= i && idx[i]))
                        return false;
                }

                return true;
            }

            void nthIndex(Array& ret, const Array& idx, IndexTuple& source, IndexTuple& target, size_t currentDim);
            void nthIndex(Array& ret, const Array& idx, IndexTuple& source, IndexTuple& target, size_t currentDim) const;
            Array ndIndex(const Array& idx);
            Array ndIndex(const Array& idx) const;
    };




    /////////////////////////////////////////////////
    /// \brief This class creates a facet on top of
    /// an existing Array to provide multidimensional
    /// access to this element supporting the actual
    /// dimensions and also different dimension sizes.
    /////////////////////////////////////////////////
    class MatrixView
    {
        public:
            DimSizes m_dimSizes;

            /////////////////////////////////////////////////
            /// \brief Create a MatrixView instance.
            ///
            /// \param arr const Array&
            ///
            /////////////////////////////////////////////////
            MatrixView(const Array& arr) : m_array(arr)
            {
                setDimSizes(m_array.getDimSizes());
            }

            /////////////////////////////////////////////////
            /// \brief Set new DimSizes from another
            /// MatrixView instance.
            ///
            /// \param view const MatrixView&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setDimSizes(const MatrixView& view)
            {
                m_dimSizes = view.m_dimSizes;
                prepareDimFacts();
            }

            /////////////////////////////////////////////////
            /// \brief Set new DimSizes.
            ///
            /// \param dims const DimSizes&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setDimSizes(const DimSizes& dims)
            {
                m_dimSizes = dims;
                prepareDimFacts();
            }

            /////////////////////////////////////////////////
            /// \brief Merge the passed DimSizes with the
            /// current ones.
            ///
            /// \param dims const DimSizes&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void mergeDimSizes(const DimSizes& dims)
            {
                merge(dims);
                prepareDimFacts();
            }

            /////////////////////////////////////////////////
            /// \brief Merge the DimSizes from the passed
            /// Array with the current ones.
            ///
            /// \param arr const Array&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void mergeDimSizes(const Array& arr)
            {
                merge(arr.getDimSizes());
                prepareDimFacts();
            }

            /////////////////////////////////////////////////
            /// \brief Merge the DimSizes from the passed
            /// MatrixView instance with the current ones.
            ///
            /// \param view const MatrixView&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void mergeDimSizes(const MatrixView& view)
            {
                merge(view.m_dimSizes);
                prepareDimFacts();
            }

            /////////////////////////////////////////////////
            /// \brief Prepare a new Array using this and
            /// another MatrixView instances' DimSizes.
            ///
            /// \param view1 MatrixView&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array prepare(MatrixView& view1)
            {
                mergeDimSizes(view1);
                view1.setDimSizes(m_dimSizes);

                Array ret;

                if (m_dimSizes.size() == 2
                    && !m_array.isMatrix()
                    && !view1.m_array.isMatrix())
                    ret.reserve(getNumElements(m_dimSizes));
                else
                    ret.setDimSizes(m_dimSizes);

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Prepare a new Array using this and
            /// other MatrixView instances' DimSizes.
            ///
            /// \param view1 MatrixView&
            /// \param view2 MatrixView&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array prepare(MatrixView& view1, MatrixView& view2)
            {
                merge(view1.m_dimSizes);
                merge(view2.m_dimSizes);
                prepareDimFacts();
                view1.setDimSizes(m_dimSizes);
                view2.setDimSizes(m_dimSizes);

                Array ret;

                if (m_dimSizes.size() == 2
                    && !m_array.isMatrix()
                    && !view1.m_array.isMatrix()
                    && !view2.m_array.isMatrix())
                    ret.reserve(getNumElements(m_dimSizes));
                else
                    ret.setDimSizes(m_dimSizes);

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Prepare a new Array using this and
            /// other MatrixView instances' DimSizes.
            ///
            /// \param view1 MatrixView&
            /// \param view2 MatrixView&
            /// \param view3 MatrixView&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array prepare(MatrixView& view1, MatrixView& view2, MatrixView& view3)
            {
                merge(view1.m_dimSizes);
                merge(view2.m_dimSizes);
                merge(view3.m_dimSizes);
                prepareDimFacts();
                view1.setDimSizes(m_dimSizes);
                view2.setDimSizes(m_dimSizes);
                view3.setDimSizes(m_dimSizes);

                Array ret;

                if (m_dimSizes.size() == 2
                    && !m_array.isMatrix()
                    && !view1.m_array.isMatrix()
                    && !view2.m_array.isMatrix()
                    && !view3.m_array.isMatrix())
                    ret.reserve(getNumElements(m_dimSizes));
                else
                    ret.setDimSizes(m_dimSizes);

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Prepare a new Array using this and
            /// other MatrixView instances' DimSizes.
            ///
            /// \param view1 MatrixView&
            /// \param view2 MatrixView&
            /// \param view3 MatrixView&
            /// \param view4 MatrixView&
            /// \return Array
            ///
            /////////////////////////////////////////////////
            Array prepare(MatrixView& view1, MatrixView& view2, MatrixView& view3, MatrixView& view4)
            {
                merge(view1.m_dimSizes);
                merge(view2.m_dimSizes);
                merge(view3.m_dimSizes);
                merge(view4.m_dimSizes);
                prepareDimFacts();
                view1.setDimSizes(m_dimSizes);
                view2.setDimSizes(m_dimSizes);
                view3.setDimSizes(m_dimSizes);
                view4.setDimSizes(m_dimSizes);

                Array ret;

                if (m_dimSizes.size() == 2
                    && !m_array.isMatrix()
                    && !view1.m_array.isMatrix()
                    && !view2.m_array.isMatrix()
                    && !view3.m_array.isMatrix()
                    && !view4.m_array.isMatrix())
                    ret.reserve(getNumElements(m_dimSizes));
                else
                    ret.setDimSizes(m_dimSizes);

                return ret;
            }

            /////////////////////////////////////////////////
            /// \brief Determine the virtual size of the
            /// contained Array.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t size() const
            {
                return getNumElements(m_dimSizes);
            }

            /////////////////////////////////////////////////
            /// \brief Get the index tuple for the i-th
            /// element addressed in linear fashion.
            ///
            /// \param i size_t
            /// \return IndexTuple
            ///
            /////////////////////////////////////////////////
            IndexTuple index(size_t i) const
            {
                if (m_dimFacts.size() != m_dimSizes.size())
                    prepareDimFacts();

                IndexTuple idx(m_dimSizes.size(), 0u);

                if (m_array.isScalar())
                    return idx;

                for (int n = idx.size()-1; n > 0; n--)
                {
                    if (i >= m_dimFacts[n])
                    {
                        if (!m_array.isScalarDim(n))
                            idx[n] = i / m_dimFacts[n];

                        i = i % m_dimFacts[n];
                    }
                }

                if (!m_array.isScalarDim(0))
                    idx.front() = i;

                return idx;
            }

            /////////////////////////////////////////////////
            /// \brief Get the i-th element in a linear
            /// indexing fashion.
            ///
            /// \param i size_t
            /// \return const Value&
            ///
            /////////////////////////////////////////////////
            const Value& get(size_t i) const
            {
                if (m_array.isScalar())
                    return m_array.front();

                return m_array.get(index(i));
            }

        private:
            mutable std::vector<size_t> m_dimFacts;
            const Array& m_array;

            /////////////////////////////////////////////////
            /// \brief Prepare the dimension factors for fast
            /// indexing access.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void prepareDimFacts() const
            {
                m_dimFacts.resize(m_dimSizes.size(), 1ull);

                for (size_t n = 1; n < m_dimFacts.size(); n++)
                {
                    m_dimFacts[n] = std::accumulate(m_dimSizes.begin(), m_dimSizes.begin()+n, 1ull, std::multiplies<size_t>());
                }
            }

            /////////////////////////////////////////////////
            /// \brief Merge other DimSizes with the internal
            /// ones creating a max of all dimensions.
            ///
            /// \param dims const DimSizes&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void merge(const DimSizes& dims)
            {
                if (dims.size() > m_dimSizes.size())
                    m_dimSizes.resize(dims.size());

                for (size_t i = 0; i < dims.size(); i++)
                {
                    m_dimSizes[i] = std::max(m_dimSizes[i], dims[i]);
                }
            }
    };




    /////////////////////////////////////////////////
    /// \brief This class is an iterator for
    /// multidimensional structures. It provides each
    /// new index tuple as an IndexTuple.
    /////////////////////////////////////////////////
    class IndexIterator
    {
        private:
            DimSizes m_dimSizes;
            IndexTuple m_current;

        public:
            /////////////////////////////////////////////////
            /// \brief Construct an IndexIterator object.
            ///
            /// \param dimSizes const DimSizes&
            ///
            /////////////////////////////////////////////////
            IndexIterator(const DimSizes& dimSizes) : m_dimSizes(dimSizes)
            {
                m_current.resize(m_dimSizes.size(), 0ull);
            }

            /////////////////////////////////////////////////
            /// \brief Get const access to the internal
            /// IndexTuple instance.
            ///
            /// \return const IndexTuple&
            ///
            /////////////////////////////////////////////////
            const IndexTuple& index() const
            {
                return m_current;
            }

            /////////////////////////////////////////////////
            /// \brief Calculate the next IndexTuple. Returns
            /// true, it this was possible, false otherwise.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool next()
            {
                if (!more())
                    return false;

                m_current.front()++;

                if (m_current.front() == m_dimSizes.front())
                {
                    m_current.front() = 0;

                    for (size_t i = 1; i < m_current.size(); i++)
                    {
                        m_current[i]++;

                        if (m_current[i] == m_dimSizes[i])
                            m_current[i] = 0;
                        else
                            break;
                    }
                }

                return true;
            }

            /////////////////////////////////////////////////
            /// \brief Determine, whether further IndexTuples
            /// can be calculated. IndexIterator::next()
            /// calls this automatically.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool more() const
            {
                for (size_t i = 0; i < m_current.size(); i++)
                {
                    if (m_current[i]+1 < m_dimSizes[i])
                        return true;
                }

                return false;
            }
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
            Variable(DataType type);
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
                if (this == &other)
                    return *this;

                DataType common = getCommonType();

                if (accepts(other))
                {
                    // It might be possible that this Array is the
                    // data source for references in the passed array.
                    // Therefore, we assign the data to a buffer,
                    // dereference it and move the result later.
                    other.dereference();
                    assign(other);
                    makeMutable();

                    if (common == TYPE_CLUSTER)
                        m_commonType = TYPE_CLUSTER;

                    return *this;
                }

                throw ParserError(ecASSIGNED_TYPE_MISMATCH);
            }

            Variable& operator=(const Variable& other);

            void indexedAssign(const Array& idx, const Array& vals);
            void overwrite(const Array& other);
            bool accepts(const Array& other) const;

        protected:
            void ndAssign(const Array& idx, const Array& vals);
            void nthIndexAssign(const Array& idx, IndexTuple& target, const DimSizes& elemCount, int currentDim, const MatrixView& vals, size_t& currentElem);
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
                        *operator[](i) = values.get(i);
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

