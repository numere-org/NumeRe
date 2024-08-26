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

#include "../utils/datetimetools.hpp"

namespace mu
{
    enum DataType
    {
        TYPE_VOID,
        TYPE_NUMERICAL,
        TYPE_STRING,
        TYPE_CATEGORY,
        TYPE_MIXED
    };


    // Abstraction of numerical values
    struct Numerical
    {
        // Careful during adaption of these values bc. of internal calculations
        enum NumericalType
        {
            LOGICAL,
            I8,
            I16,
            I32,
            I64,
            UI8,
            UI16,
            UI32,
            UI64,
            DATETIME,
            F32,
            F64,
            CF32,
            CF64
        };

        private:
            enum TypeFlags
            {
                TYPE_LOGICAL = 0x0,
                TYPE_INT = 0x1,
                TYPE_UINT = 0x2,
                TYPE_FLOAT = 0x4,
                TYPE_DATETIME = 0x8,
                TYPE_COMPLEX = 0x10
            };

            enum InternalType
            {
                INT,
                UINT,
                COMPLEX
            };

            union
            {
                int64_t i64;
                uint64_t ui64;
                std::complex<double> cf64;
            };

            NumericalType m_type;
            uint8_t m_bits;
            uint8_t m_flags;

            void getTypeInfo();
            NumericalType getPromotion(const Numerical& other) const;
            InternalType getConversion(NumericalType promotion) const;

        public:
            Numerical(int8_t data);
            Numerical(uint8_t data);
            Numerical(int16_t data);
            Numerical(uint16_t data);
            Numerical(int32_t data);
            Numerical(uint32_t data);
            Numerical(int64_t data, NumericalType type = I64);
            Numerical(uint64_t data, NumericalType type = UI64);
            Numerical(float data);
            Numerical(double data);
            Numerical(bool data);
            Numerical(const std::complex<float>& data);
            Numerical(const std::complex<double>& data = 0.0, NumericalType type = CF64);
            Numerical(const sys_time_point& data);

            static Numerical autoType(const std::complex<double>& data);

            int64_t asI64() const;
            uint64_t asUI64() const;
            double asF64() const;
            std::complex<double> asCF64() const;

            Numerical operator+(const Numerical& other) const;
            Numerical operator-() const;
            Numerical operator-(const Numerical& other) const;
            Numerical operator/(const Numerical& other) const;
            Numerical operator*(const Numerical& other) const;

            Numerical& operator+=(const Numerical& other);
            Numerical& operator-=(const Numerical& other);
            Numerical& operator/=(const Numerical& other);
            Numerical& operator*=(const Numerical& other);

            Numerical pow(const Numerical& exponent) const;

            operator bool() const;

            bool operator!() const;
            bool operator==(const Numerical& other) const;
            bool operator!=(const Numerical& other) const;
            bool operator<(const Numerical& other) const;
            bool operator<=(const Numerical& other) const;
            bool operator>(const Numerical& other) const;
            bool operator>=(const Numerical& other) const;

            NumericalType getType() const;
            std::string getTypeAsString() const;
            std::string print(size_t digits = 0) const;
            std::string printVal(size_t digits = 0) const;
            size_t getBytes() const;
            bool isInt() const;
    };


    struct Category
    {
        Numerical val;
        std::string name;
    };


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
            Value(const std::complex<double>& value);
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

            std::string& getStr();
            const std::string& getStr() const;

            Numerical& getNum();
            const Numerical& getNum() const;

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

