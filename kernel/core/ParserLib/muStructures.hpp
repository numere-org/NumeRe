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

#include <string>
#include <complex>
#include <vector>

namespace mu
{
    enum DataType
    {
        TYPE_VOID,
        TYPE_NUMERICAL,
        TYPE_STRING,
        TYPE_MIXED
    };


    // Abstraction of numerical values
    struct Numerical
    {
        std::complex<double> val;

        Numerical(double data);
        Numerical(const std::complex<double>& data = 0.0);

        Numerical operator+(const Numerical& other) const;
        Numerical operator-() const;
        Numerical operator-(const Numerical& other) const;
        Numerical operator/(const Numerical& other) const;
        Numerical operator*(const Numerical& other) const;

        Numerical& operator+=(const Numerical& other);
        Numerical& operator-=(const Numerical& other);
        Numerical& operator/=(const Numerical& other);
        Numerical& operator*=(const Numerical& other);

        int64_t asInt() const;
    };


    // 1.2 or "hello" or 2+3i
    class Value
    {
        public:
            Value();
            Value(const Value& data);
            Value(Value&& data);

            Value(const Numerical& data);
            Value(bool logical);
            Value(int value);
            Value(unsigned int value);
            Value(size_t value);
            Value(int64_t value);
            Value(double value);
            Value(const std::complex<double>& value);
            Value(const std::string& sData);

            virtual ~Value();

            Value& operator=(const Value& other);

            DataType getType() const;
            std::string getTypeAsString() const;

            bool isVoid() const;
            bool isValid() const;
            bool isNumerical() const;
            bool isString() const;

            std::string& getStr();
            const std::string& getStr() const;

            Numerical& getNum();
            const Numerical& getNum() const;

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

            std::string print() const;
            void clear();

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

            Array(const Value& singleton);
            Array(const Variable& var);
            Array(const std::vector<Numerical>& other);
            Array(const std::vector<std::string>& other);

            Array& operator=(const Array& other);

            std::vector<DataType> getType() const;
            DataType getCommonType() const;
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

            operator bool() const;

            Array operator!() const;
            Array operator==(const Array& other) const;
            Array operator!=(const Array& other) const;
            Array operator<(const Array& other) const;
            Array operator<=(const Array& other) const;
            Array operator>(const Array& other) const;
            Array operator>=(const Array& other) const;

            Array operator&&(const Array& other) const;
            Array operator||(const Array& other) const;

            std::vector<std::string> to_string() const;
            std::string print() const;

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
            VarArray& operator=(const std::vector<Array>& arrayList);

            bool operator==(const VarArray& other) const;

            bool isNull() const;
            std::string print() const;
            Array asArray() const;
    };

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




    Array apply(std::complex<double>(*)(const std::complex<double>&),
                const Array& a);
    Array apply(Value(*)(const Value&),
                const Array& a);
    Array apply(std::string(*)(const std::string&),
                const Array& a);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2);
    Array apply(Value(*)(const Value&, const Value&),
                const Array& a1, const Array& a2);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3);
    Array apply(Value(*)(const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4);
    Array apply(Value(*)(const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5);
    Array apply(Value(*)(const Value&, const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5);

    Array apply(std::complex<double>(*)(const std::complex<double>*, int),
                const Array* arrs, int elems);
    Array apply(Value(*)(const Value*, int),
                const Array* arrs, int elems);

}

#endif // MUSTRUCTURES_HPP

