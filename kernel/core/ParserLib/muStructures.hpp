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

#include "muTypes.hpp"
#include "muApply.hpp"

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
    class Value
    {
        public:
            Value();
            Value(const Value& data);
            //Value(Value&& data);

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

            virtual ~Value();

            Value& operator=(const Value& other);
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

            Value operator+(const Value& other) const;
            Value operator-() const;
            Value operator-(const Value& other) const;
            Value operator/(const Value& other) const;
            Value operator*(const Value& other) const;
            Value& operator+=(const Value& other);
            Value& operator-=(const Value& other);
            Value& operator/=(const Value& other);
            Value& operator*=(const Value& other);

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
            void* m_data;
            DataType m_type;
            static const std::string m_defString;
            static const Numerical m_defVal;

            DataType detectCommonType(const Value& other) const;
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

            Array& operator=(const Array& other);
            //Array& operator=(Array&& other);

            std::vector<DataType> getType() const;
            DataType getCommonType() const;
            std::string getCommonTypeAsString() const;
            NumericalType getCommonNumericalType() const;
            bool isScalar() const;
            bool isDefault() const;

            Array operator+(const Array& other) const;
            Array operator-() const;
            Array operator-(const Array& other) const;
            Array operator/(const Array& other) const;
            Array operator*(const Array& other) const;
            Array& operator+=(const Array& other);
            Array& operator-=(const Array& other);
            Array& operator/=(const Array& other);
            Array& operator*=(const Array& other);

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

            Value& get(size_t i);
            const Value& get(size_t i) const;

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

