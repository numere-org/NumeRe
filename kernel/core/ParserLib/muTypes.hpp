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
#include <chrono>


// We want to avoid including the whole datetime-tools here, because PostGreSQL defines date as long,
// which results in problems
using sys_time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Defines the general data type, which
    /// can be embedded into a mu::Value instance.
    /////////////////////////////////////////////////
    enum DataType
    {
        TYPE_VOID,
        TYPE_NUMERICAL,
        TYPE_STRING,
        TYPE_CATEGORY,
        TYPE_ARRAY,
        TYPE_DICTSTRUCT,
        TYPE_OBJECT,
        TYPE_GENERATOR,
        TYPE_NEUTRAL,
        TYPE_INVALID,
        TYPE_REFERENCE,
        TYPE_MIXED
    };


    /////////////////////////////////////////////////
    /// \brief Defines the specialisation of the
    /// value represented within a TYPE_NUMERICAL
    /// instantiated mu::Value.
    ///
    /// \remark Be careful during adaption of these
    /// values bc. of internal calculations.
    /////////////////////////////////////////////////
    enum NumericalType
    {
        LOGICAL,
        UI8,
        UI16,
        UI32,
        UI64,
        I8,
        I16,
        I32,
        I64,
        F32,
        F64,
        DURATION,
        DATETIME,
        CF32,
        CF64,
        AUTO
    };

    std::string getTypeAsString(DataType type);


    /////////////////////////////////////////////////
    /// \brief This structure separates a
    /// NumericalType value into its number of bits
    /// and the type-specific flags. Can be used to
    /// determine type promotion within the numerical
    /// regime.
    /////////////////////////////////////////////////
    struct TypeInfo
    {
        enum TypeFlags
        {
            TYPE_LOGICAL = 0x0,
            TYPE_UINT = 0x1,
            TYPE_INT = 0x2,
            TYPE_FLOAT = 0x4,
            TYPE_DURATION = 0x8,
            TYPE_DATETIME = 0x10,
            TYPE_COMPLEX = 0x20
        };

        uint8_t m_flags;
        uint8_t m_bits;

        TypeInfo() = default;
        TypeInfo(NumericalType type);
        void promote(const TypeInfo& other);
        NumericalType getPromotedType(const TypeInfo& other) const;
        NumericalType asType() const;
        std::string printType() const;
    };


    /////////////////////////////////////////////////
    /// \brief This structure abstrahizes all
    /// available numerical types, i.e. each
    /// NumericalType specialisation can be used
    /// identical without the need of knowing the
    /// specialisation.
    /////////////////////////////////////////////////
    struct Numerical
    {
        private:
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

            InternalType getConversion(NumericalType promotion) const;
            void resultPromote();

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

            static Numerical autoType(const std::complex<double>& data, NumericalType hint = AUTO);

            int64_t asI64() const;
            uint64_t asUI64() const;
            double asF64() const;
            std::complex<double> asCF64() const;

            Numerical operator+(const Numerical& other) const;
            Numerical operator-() const;
            Numerical operator-(const Numerical& other) const;
            Numerical operator/(const Numerical& other) const;
            Numerical operator*(const Numerical& other) const;
            Numerical operator^(const Numerical& other) const;

            Numerical& operator+=(const Numerical& other);
            Numerical& operator-=(const Numerical& other);
            Numerical& operator/=(const Numerical& other);
            Numerical& operator*=(const Numerical& other);
            Numerical& operator^=(const Numerical& other);

            void flipSign();

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
            TypeInfo getInfo() const;
            std::string getTypeAsString() const;
            std::string print(size_t digits = 0) const;
            std::string printVal(size_t digits = 0) const;
            size_t getBytes() const;
            bool isInt() const;
    };


    /////////////////////////////////////////////////
    /// \brief This structure represents a
    /// categorical datatype, i.e. a combination of
    /// the string representation together with the
    /// associated ID.
    /////////////////////////////////////////////////
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

