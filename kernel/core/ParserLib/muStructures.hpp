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
#include "muApply.hpp"
#include "muValueBase.hpp"
#include "muParserError.h"

#define VALUE20

namespace mu
{
    // Forward declaration of the Variable class
    class Variable;
    class Array;


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
            Value(DataType type);

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
            //Value& operator=(Value&& other);

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

            Value operator+(const Value& other) const
            {
                // Special case for optimizer
                if (!other.get())
                    return *this;

                if (get() && other.get())
                    return *get() + *other.get();

                throw ParserError(ecTYPE_MISMATCH);
            }
            Value operator-() const
            {
                if (get())
                    return -(*get());

                throw ParserError(ecTYPE_MISMATCH);
            }
            Value operator-(const Value& other) const
            {
                if (get() && other.get())
                    return *get() - *other.get();

                throw ParserError(ecTYPE_MISMATCH);
            }
            Value operator/(const Value& other) const
            {
                if (get() && other.get())
                    return *get() / *other.get();

                throw ParserError(ecTYPE_MISMATCH);
            }
            Value operator*(const Value& other) const
            {
                // Special case for optimizer
                if (!other.get())
                    return Value();

                if (get() && other.get())
                    return *get() * *other.get();

                throw ParserError(ecTYPE_MISMATCH);
            }
            Value& operator+=(const Value& other)
            {
                if (!get())
                    reset(other->clone());
                else if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->m_type, other->m_type))
                        return operator=(*this + other);

                    *get() += *other.get();
                }

                return *this;
            }
            Value& operator-=(const Value& other)
            {
                if (!get())
                    return operator=(-other);

                if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->m_type, other->m_type))
                        return operator=(*this - other);

                    *get() -= *other.get();
                }

                return *this;
            }
            Value& operator/=(const Value& other)
            {
                if (get() && other.get())
                {
                    if (nonRecursiveOps(get()->m_type, other->m_type))
                        return operator=(*this / other);

                    *get() /= *other.get();
                }

                return *this;
            }
            Value& operator*=(const Value& other)
            {
                if (get() && other.get())
                {
                    DataType thisType = get()->m_type;
                    DataType otherType = other->m_type;

                    if (nonRecursiveOps(thisType, otherType)
                        || (thisType == TYPE_NUMERICAL && otherType == TYPE_STRING))
                        return operator=(*this * other);

                    *get() *= *other.get();
                }

                return *this;
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

            std::string print(size_t digits = 0, size_t chrs = 0, bool trunc = false) const;
            std::string printVal(size_t digits = 0, size_t chrs = 0) const;
            void clear();
            size_t getBytes() const;

        protected:
            static const Numerical m_defVal;
            static const std::string m_defString;
    };


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
            //Array(Array&& other) = default;

            Array(size_t n, const Value& fillVal = Value());
            Array(const Value& singleton);
            Array(const Variable& var);
            Array(const std::vector<std::complex<double>>& other);
            Array(const std::vector<double>& other);
            Array(const std::vector<size_t>& other);
            Array(const std::vector<int64_t>& other);
            Array(const std::vector<Numerical>& other);
            Array(const std::vector<std::string>& other);
            Array(const Array& fst, const Array& lst);
            Array(const Array& fst, const Array& inc, const Array& lst);

            Array& operator=(const Array& other)
            {
                if (other.size() == 1)
                {
                    //if (other.front().isArray())
                    //    return operator=(other.front().getArray());

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

                return *this;
            }
            //Array& operator=(Array&& other);

            std::vector<DataType> getType() const;
            DataType getCommonType() const;
            std::string getCommonTypeAsString() const;
            NumericalType getCommonNumericalType() const;
            bool isScalar() const;
            bool isDefault() const;

            Array operator+(const Array& other) const
            {
                Array ret;

                for (size_t i = 0; i < std::max(size(), other.size()); i++)
                {
                    ret.push_back(get(i) + other.get(i));
                }

                return ret;
            }
            Array operator-() const
            {
                Array ret;

                for (size_t i = 0; i < size(); i++)
                {
                    ret.push_back(-get(i));
                }

                return ret;
            }
            Array operator-(const Array& other) const
            {
                Array ret;

                for (size_t i = 0; i < std::max(size(), other.size()); i++)
                {
                    ret.push_back(get(i) - other.get(i));
                }

                return ret;
            }
            Array operator/(const Array& other) const
            {
                Array ret;

                for (size_t i = 0; i < std::max(size(), other.size()); i++)
                {
                    ret.push_back(get(i) / other.get(i));
                }

                return ret;
            }
            Array operator*(const Array& other) const
            {
                Array ret;

                for (size_t i = 0; i < std::max(size(), other.size()); i++)
                {
                    ret.push_back(get(i) * other.get(i));
                }

                return ret;
            }
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

            Array call(const std::string& sMethod) const;
            Array call(const std::string& sMethod, const Array& arg1) const;
            Array call(const std::string& sMethod, const Array& arg1, const Array& arg2) const;

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

            Value& get(size_t i)
            {
                if (size() == 1u)
                    return front();
                else if (size() <= i)
                    throw std::length_error("Element " + std::to_string(i) + " is out of bounds.");

                return operator[](i);
            }
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
            Variable& operator=(const Array& other);
            Variable& operator=(const Variable& other);

            void overwrite(const Array& other);

            /*void powN(int N, Array& res) const
            {
                res = *this;

                for (int n = 1; n < N; n++)
                {
                    res *= *this;
                }
            }

            void mul(const Array& fact, const Array& add, Array& res) const
            {
                res = *this;
                res *= fact;

                if (!add.isDefault())
                    res += add;
            }

            void revMul(const Array& fact, const Array& add, Array& res) const
            {
                if (!add.isDefault())
                {
                    res = add;
                    res += *this * fact;
                }
                else
                {
                    res = *this;
                    res *= fact;
                }
            }

            void div(const Array& fact, const Array& add, Array& res) const
            {
                res = fact;
                res /= *this;

                if (!add.isDefault())
                    res += add;
            }*/
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
            const Array& operator=(const Array& values);
            Array operator+=(const Array& values);
            Array operator-=(const Array& values);
            Array operator*=(const Array& values);
            Array operator/=(const Array& values);
            Array pow(const Array& values);
            VarArray& operator=(const std::vector<Array>& arrayList);

            bool operator==(const VarArray& other) const;

            bool isNull() const;
            std::string print() const;
            Array asArray() const;
    };


    class StackBuffer : protected Array
    {
        private:
            Variable* m_var;

        public:
            StackBuffer() : Array(), m_var(nullptr) {}

            void use(Variable* var)
            {
                m_var = var;
            }

            const Array& get() const
            {
                if (m_var)
                    return *m_var;

                return *this;
            }

            StackBuffer& operator=(const StackBuffer& other) = default; // ???
            StackBuffer& operator=(const Array& other)
            {
                Array::operator=(other);
                m_var = nullptr;
                return *this;
            }

            StackBuffer& operator+=(const StackBuffer& other)
            {
                if (m_var)
                    Array::operator=(*m_var + other.get());
                else
                    Array::operator+=(other.get());

                m_var = nullptr;
                return *this;
            }

            StackBuffer& operator-=(const StackBuffer& other)
            {
                if (m_var)
                    Array::operator=(*m_var - other.get());
                else
                    Array::operator-=(other.get());

                m_var = nullptr;
                return *this;
            }

            StackBuffer& operator*=(const StackBuffer& other)
            {
                if (m_var)
                    Array::operator=(*m_var * other.get());
                else
                    Array::operator*=(other.get());

                m_var = nullptr;
                return *this;
            }

            StackBuffer& operator/=(const StackBuffer& other)
            {
                if (m_var)
                    Array::operator=(*m_var / other.get());
                else
                    Array::operator/=(other.get());

                m_var = nullptr;
                return *this;
            }

            Array operator<(const StackBuffer& other) const
            {
                return get() < other.get();
            }

            Array operator<=(const StackBuffer& other) const
            {
                return get() <= other.get();
            }

            Array operator>(const StackBuffer& other) const
            {
                return get() > other.get();
            }

            Array operator>=(const StackBuffer& other) const
            {
                return get() >= other.get();
            }

            Array operator!=(const StackBuffer& other) const
            {
                return get() != other.get();
            }

            Array operator==(const StackBuffer& other) const
            {
                return get() == other.get();
            }

            Array operator&&(const StackBuffer& other) const
            {
                return get() && other.get();
            }

            Array operator||(const StackBuffer& other) const
            {
                return get() || other.get();
            }

            Array pow(const StackBuffer& other) const
            {
                return get().pow(other.get());
            }


            void varPowN(const Variable& var, int N)
            {
                Array::operator=(var);

                for (int n = 1; n < N; n++)
                {
                    Array::operator*=(var);
                }
            }

            void varMul(const Array& fact, const Variable& var, const Array& add)
            {
                Array::operator=(var);
                Array::operator*=(fact);

                if (!add.isDefault())
                    Array::operator+=(add);
            }

            void revVarMul(const Array& fact, const Variable& var, const Array& add)
            {
                if (!add.isDefault())
                {
                    Array::operator=(add);
                    Array::operator+=(var * fact);
                }
                else
                {
                    Array::operator=(var);
                    Array::operator*=(fact);
                }
            }

            void divVar(const Array& fact, const Variable& var, const Array& add)
            {
                Array::operator=(fact);
                Array::operator/=(var);

                if (!add.isDefault())
                    Array::operator+=(add);
            }
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

