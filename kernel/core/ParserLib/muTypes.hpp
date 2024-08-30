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

#ifndef MUTYPES_HPP
#define MUTYPES_HPP

#include <complex>
#include <string>

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
            CF64,
            AUTO
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
            Numerical(const std::complex<double>& data = 0.0, NumericalType type = AUTO);
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

            Numerical getPromotedType(const Numerical& other) const;

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

        bool operator==(const Category& other) const;
        bool operator!=(const Category& other) const;
        bool operator<(const Category& other) const;
        bool operator<=(const Category& other) const;
        bool operator>(const Category& other) const;
        bool operator>=(const Category& other) const;
    };
}

#endif // MUTYPES_HPP

