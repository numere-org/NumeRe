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
    // 1.2 or "hello" or 2+3i
    class Value
    {
        public:
            Value();
            Value(const Value& data);
            Value(Value&& data);

            Value(const Numerical& data);
            Value(const Category& data);
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

            virtual ~Value();

            Value& operator=(const Value& other);

            DataType getType() const;
            std::string getTypeAsString() const;

            bool isVoid() const;
            bool isValid() const;
            bool isNumerical() const;
            bool isString() const;
            bool isCategory() const;

            std::string& getStr();
            const std::string& getStr() const;

            Numerical& getNum();
            const Numerical& getNum() const;

            Category& getCategory();
            const Category& getCategory() const;

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

    class Variable;


    // {1,2,3,4,...}
    class Array : public std::vector<Value>
    {
        public:
            Array();
            Array(const Array& other);
            Array(Array&& other) = default;

            Array(size_t n, const Value& fillVal = Value());
            Array(const Value& singleton);
            Array(const Variable& var);
            Array(const std::vector<std::complex<double>>& other);
            Array(const std::vector<Numerical>& other);
            Array(const std::vector<std::string>& other);

            Array& operator=(const Array& other);

            std::vector<DataType> getType() const;
            DataType getCommonType() const;
            std::string getCommonTypeAsString() const;
            Numerical::NumericalType getCommonNumericalType() const;
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
            size_t getBytes() const;

            Value& get(size_t i);
            const Value& get(size_t i) const;

            void zerosToVoid();
            bool isCommutative() const;

        private:
            mutable DataType m_commonType;
            static const Value m_default;
    };


    class Variable : public Array
    {
        public:
            Variable();
            Variable(const Value& data);
            Variable(const Array& data);
            Variable(const Variable& data);
            Variable(Array&& data);
            Variable(Variable&& data) = default;

            Variable& operator=(const Value& other);
            Variable& operator=(const Array& other);
            Variable& operator=(const Variable& other);

            void overwrite(const Array& other);
    };


    // {a,b,c,d,...}
    class VarArray : public std::vector<Variable*>
    {
        public:
            VarArray() = default;
            VarArray(Variable* var);
            VarArray(const VarArray& other) = default;
            VarArray(VarArray&& other) = default;
            VarArray& operator=(const VarArray& other) = default;
            Array operator=(const Array& values);
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

