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

#include "muTypes.hpp"
#include "../utils/tools.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Lookup table for determining the
    /// promoted type of the operation.
    /////////////////////////////////////////////////
    static constexpr NumericalType PROMOTIONTABLEADDSUB[] = {
//		 LOGICAL,  UI8,	     UI16,	   UI32,	 UI64,	   I8,	     I16,	   I32,	     I64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64
/*LOG*/	 I8,	   UI8,	     UI16,	   UI32,	 UI64,	   I8,	     I16,	   I32,	     I64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64,
/*UI8*/	 UI8,	   UI8, 	 UI16,	   UI32,	 UI64, 	   I8, 	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI16*/ UI16,	   UI16, 	 UI16,	   UI32,	 UI64, 	   I16,	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI32*/ UI32,	   UI32, 	 UI32,	   UI32,	 UI64, 	   I32,	     I32,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI64*/ UI64,	   UI64, 	 UI64,	   UI64,	 UI64, 	   I64,	     I64,	   I64,	     I64, 	   F64,      F64,      DURATION, DATETIME, CF64, CF64,
/*I8*/	 I8,	   I8, 	     I16,	   I32,	     I64, 	   I8, 	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I16*/	 I16,	   I16, 	 I16,	   I32,	     I64, 	   I16,	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I32*/	 I32,	   I32, 	 I32,	   I32,	     I64, 	   I32,	     I32,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I64*/	 I64,	   I64, 	 I64,	   I64,	     I64, 	   I64,	     I64,	   I64,	     I64, 	   F64,      F64,      DURATION, DATETIME, CF64, CF64,
/*F32*/	 F32,	   F32,	     F32,	   F32,	     F64,	   F32,	     F32,	   F32,	     F64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64,
/*F64*/	 F64,	   F64, 	 F64,	   F64,	     F64,	   F64,	     F64,	   F64,	     F64,	   F64,	     F64,      DURATION, DATETIME, CF64, CF64,
/*DUR*/  DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DATETIME, CF64, CF64,
/*DTM*/  DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DURATION, CF64, CF64,
/*CF32*/ CF32,	   CF32,	 CF32,	   CF32,	 CF64,	   CF32,     CF32,	   CF32,	 CF64,	   CF32,	 CF64,	   CF64,     CF64,     CF32, CF64,
/*CF64*/ CF64,	   CF64,	 CF64,	   CF64,	 CF64,	   CF64,     CF64,	   CF64,	 CF64,	   CF64,	 CF64,	   CF64,     CF64,     CF64, CF64};


    /////////////////////////////////////////////////
    /// \brief Lookup table for determining the
    /// promoted type of the operation.
    /////////////////////////////////////////////////
    static constexpr NumericalType PROMOTIONTABLEMULDIV[] = {
//		 LOGICAL,  UI8,	     UI16,	   UI32,	 UI64,	   I8,	     I16,	   I32,	     I64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64
/*LOG*/	 I8,	   UI8,	     UI16,	   UI32,	 UI64,	   I8,	     I16,	   I32,	     I64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64,
/*UI8*/	 UI8,	   UI8, 	 UI16,	   UI32,	 UI64, 	   I8, 	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI16*/ UI16,	   UI16, 	 UI16,	   UI32,	 UI64, 	   I16,	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI32*/ UI32,	   UI32, 	 UI32,	   UI32,	 UI64, 	   I32,	     I32,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*UI64*/ UI64,	   UI64, 	 UI64,	   UI64,	 UI64, 	   I64,	     I64,	   I64,	     I64, 	   F64,      F64,      DURATION, DATETIME, CF64, CF64,
/*I8*/	 I8,	   I8, 	     I16,	   I32,	     I64, 	   I8, 	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I16*/	 I16,	   I16, 	 I16,	   I32,	     I64, 	   I16,	     I16,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I32*/	 I32,	   I32, 	 I32,	   I32,	     I64, 	   I32,	     I32,	   I32,	     I64, 	   F32,      F64,      DURATION, DATETIME, CF32, CF64,
/*I64*/	 I64,	   I64, 	 I64,	   I64,	     I64, 	   I64,	     I64,	   I64,	     I64, 	   F64,      F64,      DURATION, DATETIME, CF64, CF64,
/*F32*/	 F32,	   F32,	     F32,	   F32,	     F64,	   F32,	     F32,	   F32,	     F64,	   F32,	     F64,      DURATION, DATETIME, CF32, CF64,
/*F64*/	 F64,	   F64, 	 F64,	   F64,	     F64,	   F64,	     F64,	   F64,	     F64,	   F64,	     F64,      DURATION, DATETIME, CF64, CF64,
/*DUR*/  DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, DURATION, F64,      F64,      CF64, CF64,
/*DTM*/  DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, DATETIME, F64,      F64,      CF64, CF64,
/*CF32*/ CF32,	   CF32,	 CF32,	   CF32,	 CF64,	   CF32,     CF32,	   CF32,	 CF64,	   CF32,	 CF64,	   CF64,     CF64,     CF32, CF64,
/*CF64*/ CF64,	   CF64,	 CF64,	   CF64,	 CF64,	   CF64,     CF64,	   CF64,	 CF64,	   CF64,	 CF64,	   CF64,     CF64,     CF64, CF64};


    /////////////////////////////////////////////////
    /// \brief Return the type as a std::string.
    ///
    /// \param type DataType
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string getTypeAsString(DataType type)
    {
        switch (type)
        {
            case TYPE_CATEGORY:
                return "category";
            case TYPE_NEUTRAL:
#ifdef PARSERSTANDALONE
                return "neutral";
#else
                return "void";
#endif
            case TYPE_NUMERICAL:
                return "value";
            case TYPE_STRING:
                return "string";
            case TYPE_ARRAY:
            case TYPE_CLUSTER:
                return "cluster";
            case TYPE_REFERENCE:
                return "reference";
            case TYPE_DICTSTRUCT:
                return "dictstruct";
            case TYPE_OBJECT:
                return "object";
        }

        return "void";
    }


    /////////////////////////////////////////////////
    /// \brief Custom implementation for the complex
    /// multiplication operator with a scalar
    /// optimization.
    ///
    /// \param __x const std::complex<double>&
    /// \param __y const std::complex<double>&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    inline std::complex<double> operator*(const std::complex<double>& __x, const std::complex<double>& __y)
    {
        if (__x.imag() == 0.0)
            return std::complex<double>(__y.real()*__x.real(), __y.imag()*__x.real());
        else if (__y.imag() == 0.0)
            return std::complex<double>(__x.real()*__y.real(), __x.imag()*__y.real());

        std::complex<double> __r = __x;
        __r *= __y;
        return __r;
    }


    /////////////////////////////////////////////////
    /// \brief Custom implementation for the complex
    /// division operator with a scalar optimization.
    ///
    /// \param __x const std::complex<double>&
    /// \param __y const std::complex<double>&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    inline std::complex<double> operator/(const std::complex<double>& __x, const std::complex<double>& __y)
    {
        if (__y.imag() == 0.0)
            return std::complex<double>(__x.real() / __y.real(), __x.imag() / __y.real());

        std::complex<double> __r = __x;
        __r /= __y;
        return __r;
    }


    /////////////////////////////////////////////////
    /// \brief Provides a faster way to calculate the
    /// promotion of two different types than using
    /// TypeInfo.
    ///
    /// \param fst NumericalType
    /// \param scnd NumericalType
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    static NumericalType fastPromoteAddSub(NumericalType fst, NumericalType scnd)
    {
        return PROMOTIONTABLEADDSUB[fst * AUTO + scnd];
    }



    /////////////////////////////////////////////////
    /// \brief Provides a faster way to calculate the
    /// promotion of two different types than using
    /// TypeInfo.
    ///
    /// \param fst NumericalType
    /// \param scnd NumericalType
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    static NumericalType fastPromoteMulDiv(NumericalType fst, NumericalType scnd)
    {
        return PROMOTIONTABLEMULDIV[fst * AUTO + scnd];
    }



    /////////////////////////////////////////////////
    /// \brief Construct a TypeInfo from a
    /// NumericalType value.
    ///
    /// \param type NumericalType
    ///
    /////////////////////////////////////////////////
    TypeInfo::TypeInfo(NumericalType type)
    {
        if (type == LOGICAL)
        {
            m_bits = 8;
            m_flags = TYPE_LOGICAL;
        }
        else if (type == DURATION)
        {
            m_bits = 64;
            m_flags = TYPE_FLOAT | TYPE_DURATION;
        }
        else if (type == DATETIME)
        {
            m_bits = 64;
            m_flags = TYPE_FLOAT | TYPE_DATETIME;
        }
        else if (type <= UI64)
        {
            m_flags = TYPE_UINT;
            m_bits = 8 * (0x8 >> (UI64 - type));
        }
        else if (type <= I64)
        {
            m_flags = TYPE_INT;
            m_bits = 8 * (0x8 >> (I64 - type));
        }
        else if (type <= F64)
        {
            m_flags = TYPE_FLOAT;
            m_bits = type == F64 ? 64 : 32;
        }
        else
        {
            m_flags = TYPE_FLOAT | TYPE_COMPLEX;
            m_bits = type == CF64 ? 64 : 32;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Promote this instance using the
    /// information from the passed TypeInfo instance.
    ///
    /// \param other const TypeInfo&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TypeInfo::promote(const TypeInfo& other)
    {
        m_bits = std::max(m_bits, other.m_bits);
        m_flags = std::max(m_flags, other.m_flags);
    }


    /////////////////////////////////////////////////
    /// \brief Calculate the promotion of this
    /// TypeInfo instance with the passed one and
    /// return it as a NumericalType value.
    ///
    /// \param other const TypeInfo&
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    NumericalType TypeInfo::getPromotedType(const TypeInfo& other) const
    {
        uint8_t bits = std::max(m_bits, other.m_bits);
        uint8_t flags = std::max(m_flags, other.m_flags);

        if (flags & TYPE_COMPLEX)
            return bits == 32 ? CF32 : CF64;

        if (flags & TYPE_DURATION)
            return DURATION;

        if (flags & TYPE_DATETIME)
            return DATETIME;

        if (flags & TYPE_FLOAT)
            return bits == 32 ? F32 : F64;

        if (flags & TYPE_INT)
            return NumericalType(I8 + std::log2(bits)-3);

        if (flags & TYPE_UINT)
            return NumericalType(UI8 + std::log2(bits)-3);

        return LOGICAL;
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance into a
    /// NumericalType value.
    ///
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    NumericalType TypeInfo::asType() const
    {
        if (m_flags & TYPE_COMPLEX)
            return m_bits == 32 ? CF32 : CF64;

        if (m_flags & TYPE_DURATION)
            return DURATION;

        if (m_flags & TYPE_DATETIME)
            return DATETIME;

        if (m_flags & TYPE_FLOAT)
            return m_bits == 32 ? F32 : F64;

        if (m_flags & TYPE_INT)
            return NumericalType(I8 + std::log2(m_bits)-3);

        if (m_flags & TYPE_UINT)
            return NumericalType(UI8 + std::log2(m_bits)-3);

        return LOGICAL;
    }


    /////////////////////////////////////////////////
    /// \brief Format this TypeInfo into a
    /// std::string for printing on the terminal.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TypeInfo::printType() const
    {
        switch (asType())
        {
            case LOGICAL:
                return "logical";
            case I8:
                return "value.i8";
            case I16:
                return "value.i16";
            case I32:
                return "value.i32";
            case I64:
                return "value.i64";
            case UI8:
                return "value.ui8";
            case UI16:
                return "value.ui16";
            case UI32:
                return "value.ui32";
            case UI64:
                return "value.ui64";
            case DURATION:
                return "duration";
            case DATETIME:
                return "datetime";
            case F32:
                return "value.f32";
            case F64:
                return "value.f64";
            case CF32:
                return "value.cf32";
            case AUTO:
            case CF64:
                return "value.cf64";
            case INVALID:
                return "void";
        }

        return "value.cf64";
    }





    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from an int8_t.
    ///
    /// \param data int8_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(int8_t data)
    {
        writeInt(data, I8);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a uint8_t.
    ///
    /// \param data uint8_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(uint8_t data)
    {
        writeUint(data, UI8);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from an int16_t.
    ///
    /// \param data int16_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(int16_t data)
    {
        writeInt(data, I16);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a uint16_t.
    ///
    /// \param data uint16_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(uint16_t data)
    {
        writeUint(data, UI16);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from an int32_t.
    ///
    /// \param data int32_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(int32_t data)
    {
        writeInt(data, I32);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a uint32_t.
    ///
    /// \param data uint32_t
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(uint32_t data)
    {
        writeUint(data, UI32);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from an int64_t.
    /// The additional parameter allows for defining
    /// the target type if different than int64_t.
    ///
    /// \param data int64_t
    /// \param type NumericalType
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(int64_t data, NumericalType type)
    {
        writeInt(data, type);
        resultPromote();
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a uint64_t.
    /// The additional parameter allows for defining
    /// the target type if different than uint64_t.
    ///
    /// \param data uint64_t
    /// \param type NumericalType
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(uint64_t data, NumericalType type)
    {
        writeUint(data, type);
        resultPromote();
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a float.
    ///
    /// \param data float
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(float data)
    {
        writeFloat(data, F32);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a double.
    ///
    /// \param data double
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(double data)
    {
        writeFloat(data, F64);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a bool.
    ///
    /// \param data bool
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(bool data)
    {
        writeUint(data, LOGICAL);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a
    /// std::complex in the float variant.
    ///
    /// \param data const std::complex<float>&
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(const std::complex<float>& data)
    {
        writeComplex(data, CF32);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a
    /// std::complex in the double variant. The
    /// additional parameter allows for defining the
    /// target type if different than std::complex.
    ///
    /// \param data const std::complex<double>&
    /// \param type NumericalType
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(const std::complex<double>& data, NumericalType type)
    {
        if (type == AUTO)
        {
            if (data.imag() == 0.0)
                writeFloat(data.real(), F64);
            else
                writeComplex(data, CF64);
        }
        else if (type == CF32 || type == CF64)
            writeComplex(data, type);
        else if (type <= UI64)
            writeUint(intCast(data.real()), type);
        else if (type <= I64)
            writeInt(intCast(data.real()), type);
        else
            writeFloat(data.real(), type);
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Numerical from a time
    /// point.
    ///
    /// \param time const sys_time_point&
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(const sys_time_point& time)
    {
        writeFloat(to_double(time), DATETIME);
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor with a special case
    /// for complex values.
    ///
    /// \param num const Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(const Numerical& num)
    {
        if (num.m_type == CF32 || num.m_type == CF64)
            writeComplex(num.asCF64(), num.m_type);
        else
        {
            m_storage = num.m_storage;
            m_type = num.m_type;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor with a special case
    /// for complex values.
    ///
    /// \param num Numerical&&
    ///
    /////////////////////////////////////////////////
    Numerical::Numerical(Numerical&& num)
    {
        m_storage = num.m_storage;
        m_type = num.m_type;

        if (num.m_type == CF32 || num.m_type == CF64)
            num.m_type = INVALID;
    }


    /////////////////////////////////////////////////
    /// \brief Copy-assignment operator.
    ///
    /// \param num const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator=(const Numerical& num)
    {
        clear();

        if (num.m_type == CF32 || num.m_type == CF64)
            writeComplex(num.asCF64(), num.m_type);
        else
        {
            m_storage = num.m_storage;
            m_type = num.m_type;
        }

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Move-assignment operator.
    ///
    /// \param num Numerical&&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator=(Numerical&& num)
    {
        clear();

        m_storage = num.m_storage;
        m_type = num.m_type;

        if (num.m_type == CF32 || num.m_type == CF64)
            num.m_type = INVALID;

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Destructor.
    /////////////////////////////////////////////////
    Numerical::~Numerical()
    {
        clear();
    }


    /////////////////////////////////////////////////
    /// \brief This member function clears the
    /// internal contents.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Numerical::clear()
    {
        if (m_type == CF32 || m_type == CF64)
            delete reinterpret_cast<std::complex<double>*>(m_storage);

        m_storage = 0;
        m_type = INVALID;
    }


    /////////////////////////////////////////////////
    /// \brief Create a Numerical instance by
    /// determining the underlying type automatically.
    /// The second parameter can indicate, whether
    /// the target type should be a float although it
    /// would fit into an int.
    ///
    /// \param data const std::complex<double>&
    /// \param hint NumericalType
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::autoType(const std::complex<double>& data, NumericalType hint)
    {
        if (data.imag() == 0.0 && std::rint(data.real()) == data.real() && hint != F64)
        {
            /*if (data.real() >= 0)
            {
                if (data.real() <= UINT8_MAX)
                    return Numerical((uint8_t)data.real());

                if (data.real() <= UINT16_MAX)
                    return Numerical((uint16_t)data.real());

                if (data.real() <= UINT32_MAX)
                    return Numerical((uint32_t)data.real());

                return Numerical((uint64_t)data.real());
            }*/

            if (data.real() >= INT8_MIN && data.real() <= INT8_MAX)
                return Numerical((int8_t)data.real());

            if (data.real() >= INT16_MIN && data.real() <= INT16_MAX)
                return Numerical((int16_t)data.real());

            if (data.real() >= INT32_MIN && data.real() <= INT32_MAX)
                return Numerical((int32_t)data.real());

            if (data.real() >= INT64_MIN && data.real() <= INT64_MAX)
                return Numerical((int64_t)data.real());

            if (data.real() <= UINT64_MAX)
                return Numerical((uint64_t)data.real());
        }

        if (data.imag() == 0.0)
            return Numerical(data.real());

        return Numerical(data);
    }


    /////////////////////////////////////////////////
    /// \brief Determine the internal conversion from
    /// the promoted type.
    ///
    /// \param promotion NumericalType
    /// \return Numerical::InternalType
    ///
    /////////////////////////////////////////////////
    Numerical::InternalType Numerical::getConversion(NumericalType promotion) const
    {
        static constexpr Numerical::InternalType CONVERSIONTABLE[] = {
            Numerical::UINT, Numerical::UINT, Numerical::UINT, Numerical::UINT, Numerical::UINT,
            Numerical::INT, Numerical::INT, Numerical::INT, Numerical::INT,
            Numerical::COMPLEX, Numerical::COMPLEX, Numerical::COMPLEX, Numerical::COMPLEX, Numerical::COMPLEX, Numerical::COMPLEX};

        return CONVERSIONTABLE[promotion];
    }


    /////////////////////////////////////////////////
    /// \brief Combining function to promote a type
    /// based upon its value.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Numerical::resultPromote()
    {
        if (m_type >= I8 && m_type <= I32)
        {
            int64_t i64 = asI64();

            if (m_type == I8 && i64 >= INT8_MIN && i64 <= INT8_MAX)
                m_type = I8;
            else if (m_type <= I16 && i64 >= INT16_MIN && i64 <= INT16_MAX)
                m_type = I16;
            else if (m_type <= I32 && i64 >= INT32_MIN && i64 <= INT32_MAX)
                m_type = I32;
            else
                m_type = I64;
        }
        else if (m_type >= UI8 && m_type <= UI32)
        {
            uint64_t ui64 = asUI64();

            if (m_type == UI8 && ui64 <= UINT8_MAX)
                m_type = UI8;
            else if (m_type <= UI16 && ui64 <= UINT16_MAX)
                m_type = UI16;
            else if (m_type <= UI32 && ui64 <= UINT32_MAX)
                m_type = UI32;
            else
                m_type = UI64;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Return the represented value as an
    /// int64_t.
    ///
    /// \return int64_t
    ///
    /////////////////////////////////////////////////
    int64_t Numerical::asI64() const
    {
        if (m_type <= UI64)
            return m_storage;

        if (m_type <= I64)
            return *reinterpret_cast<const int64_t*>(&m_storage);

        if (m_type <= DATETIME)
            return intCast(*reinterpret_cast<const double*>(&m_storage));

        return intCast(*reinterpret_cast<const std::complex<double>*>(m_storage));
    }


    /////////////////////////////////////////////////
    /// \brief Return the represented value as a
    /// uint64_t.
    ///
    /// \return uint64_t
    ///
    /////////////////////////////////////////////////
    uint64_t Numerical::asUI64() const
    {
        if (m_type <= UI64)
            return m_storage;

        if (m_type <= I64)
            return (uint64_t)*reinterpret_cast<const int64_t*>(&m_storage);

        if (m_type <= DATETIME)
            return (uint64_t)intCast(*reinterpret_cast<const double*>(&m_storage));

        return (uint64_t)intCast(*reinterpret_cast<const std::complex<double>*>(m_storage));
    }


    /////////////////////////////////////////////////
    /// \brief Return the represented value as a
    /// double.
    ///
    /// \return double
    ///
    /////////////////////////////////////////////////
    double Numerical::asF64() const
    {
        if (m_type <= UI64)
            return m_storage;

        if (m_type <= I64)
            return *reinterpret_cast<const int64_t*>(&m_storage);

        if (m_type <= DATETIME)
            return *reinterpret_cast<const double*>(&m_storage);

        return reinterpret_cast<const std::complex<double>*>(m_storage)->real();
    }


    /////////////////////////////////////////////////
    /// \brief Return the represented value as a
    /// std::complex.
    ///
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Numerical::asCF64() const
    {
        if (m_type <= UI64)
            return m_storage;

        if (m_type <= I64)
            return *reinterpret_cast<const int64_t*>(&m_storage);

        if (m_type <= DATETIME)
            return *reinterpret_cast<const double*>(&m_storage);

        return *reinterpret_cast<const std::complex<double>*>(m_storage);
    }


    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator+(const Numerical& other) const
    {
        NumericalType promotion = fastPromoteAddSub(m_type, other.m_type);

        if (promotion == LOGICAL && asI64() && other.asI64())
            return Numerical(2LL, I8);

        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() + other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() + other.asUI64(), promotion);

        return Numerical(asCF64() + other.asCF64(), promotion);
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator-() const
    {
        if (m_type <= UI64)
            return Numerical(-asUI64(), (NumericalType)(m_type-I64));

        if (m_type <= I64)
            return Numerical(-asI64(), m_type);

        return Numerical(-asCF64(), m_type);
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator-(const Numerical& other) const
    {
        NumericalType promotion = fastPromoteAddSub(m_type, other.m_type);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() - other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() - other.asUI64(), promotion);

        return Numerical(asCF64() - other.asCF64(), promotion);
    }


    /////////////////////////////////////////////////
    /// \brief Divide operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator/(const Numerical& other) const
    {
        if (m_type <= UI64)
            return autoType(asUI64() / other.asCF64());

        if (m_type <= I64)
            return autoType(asI64() / other.asCF64());

        NumericalType promotion = fastPromoteMulDiv(m_type, other.m_type);

        if ((promotion == DURATION || promotion == DATETIME) && other.m_type != DURATION && other.m_type != DATETIME)
            return Numerical(asCF64() / other.asCF64(), promotion);

        return autoType(asCF64() / other.asCF64());
    }


    /////////////////////////////////////////////////
    /// \brief Multiply operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator*(const Numerical& other) const
    {
        NumericalType promotion = fastPromoteMulDiv(m_type, other.m_type);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            return Numerical(asI64() * other.asI64(), promotion);

        if (conversion == Numerical::UINT)
            return Numerical(asUI64() * other.asUI64(), promotion);

        return Numerical(asCF64() * other.asCF64(), promotion);
    }


    /////////////////////////////////////////////////
    /// \brief Power operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::operator^(const Numerical& other) const
    {
        return pow(other);
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator+=(const Numerical& other)
    {
        NumericalType promotion = fastPromoteAddSub(m_type, other.m_type);

        if (promotion == LOGICAL && asI64() && other.asI64())
        {
            writeInt(2, I8);
            return *this;
        }

        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            writeInt(asI64() + other.asI64(), promotion);
        else if (conversion == Numerical::UINT)
            writeUint(asUI64() + other.asUI64(), promotion);
        else if (promotion <= DATETIME)
            writeFloat(asF64() + other.asF64(), promotion);
        else
            writeComplex(asCF64() + other.asCF64(), promotion);

        resultPromote();
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator-=(const Numerical& other)
    {
        NumericalType promotion = fastPromoteAddSub(m_type, other.m_type);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            writeInt(asI64() - other.asI64(), promotion);
        else if (conversion == Numerical::UINT)
            writeUint(asUI64() - other.asUI64(), promotion);
        else if (promotion <= DATETIME)
            writeFloat(asF64() - other.asF64(), promotion);
        else
            writeComplex(asCF64() - other.asCF64(), promotion);

        resultPromote();
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator/=(const Numerical& other)
    {
        *this = operator/(other);
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator*=(const Numerical& other)
    {
        NumericalType promotion = fastPromoteMulDiv(m_type, other.m_type);
        Numerical::InternalType conversion = getConversion(promotion);

        if (conversion == Numerical::INT)
            writeInt(asI64() * other.asI64(), promotion);
        else if (conversion == Numerical::UINT)
            writeUint(asUI64() * other.asUI64(), promotion);
        else if (promotion <= DATETIME)
            writeFloat(asF64() * other.asF64(), promotion);
        else
            writeComplex(asCF64() * other.asCF64(), promotion);

        resultPromote();
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const Numerical&
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Numerical::operator^=(const Numerical& other)
    {
        operator=(pow(other));
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Numerical::flipSign()
    {
        if (m_type == LOGICAL)
            writeInt(-asUI64(), I8);
        else if (m_type <= UI64)
            writeInt(-asUI64(), NumericalType(m_type + I8));
        else if (m_type <= I64)
            writeInt(-asI64(), m_type);
        else if (m_type <= DURATION)
            writeFloat(-asF64(), m_type);
        else
            writeComplex(-asCF64(), m_type);
    }


    /////////////////////////////////////////////////
    /// \brief Optimized power function.
    ///
    /// \param exponent const Numerical&
    /// \return Numerical
    ///
    /////////////////////////////////////////////////
    Numerical Numerical::pow(const Numerical& exponent) const
    {
        if (exponent.isInt())
            return Numerical::autoType(intPower(asCF64(), exponent.asI64()), (m_type > LOGICAL && m_type <= I64) ? I64 : F64);

        return Numerical(std::pow(asCF64(), exponent.asCF64()), AUTO);
    }


    /////////////////////////////////////////////////
    /// \brief Represent the value as a boolean.
    ///
    /// \return Numerical::operator
    ///
    /////////////////////////////////////////////////
    Numerical::operator bool() const
    {
        if (m_type <= UI64)
            return asUI64() != 0;

        if (m_type <= I64)
            return asI64() != 0;

        return asCF64() != 0.0;
    }


    /////////////////////////////////////////////////
    /// \brief Logical not.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator!() const
    {
        return !bool(*this);
    }


    /////////////////////////////////////////////////
    /// \brief Equal operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator==(const Numerical& other) const
    {
        Numerical::InternalType conversion = getConversion(fastPromoteAddSub(m_type, other.m_type));

        if (conversion == Numerical::INT)
            return asI64() == other.asI64();

        if (conversion == Numerical::UINT)
            return asUI64() == other.asUI64();

        return asCF64() == other.asCF64();
    }


    /////////////////////////////////////////////////
    /// \brief Not-equal operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator!=(const Numerical& other) const
    {
        return !operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Less-than operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator<(const Numerical& other) const
    {
        Numerical::InternalType conversion = getConversion(fastPromoteAddSub(m_type, other.m_type));

        if (conversion == Numerical::INT)
            return asI64() < other.asI64();

        if (conversion == Numerical::UINT)
            return asUI64() < other.asUI64();

        return asF64() < other.asF64();
    }


    /////////////////////////////////////////////////
    /// \brief Less-or-equal operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator<=(const Numerical& other) const
    {
        return operator<(other) || operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-than operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator>(const Numerical& other) const
    {
        return !operator<=(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-or-equal operator.
    ///
    /// \param other const Numerical&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::operator>=(const Numerical& other) const
    {
        return !operator<(other);
    }


    /////////////////////////////////////////////////
    /// \brief Get the NumeericalType of the
    /// contained value.
    ///
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    NumericalType Numerical::getType() const
    {
        return m_type;
    }


    /////////////////////////////////////////////////
    /// \brief Get the TypeInfo corresponding to the
    /// contained value.
    ///
    /// \return TypeInfo
    ///
    /////////////////////////////////////////////////
    TypeInfo Numerical::getInfo() const
    {
        return TypeInfo(m_type);
    }


    /////////////////////////////////////////////////
    /// \brief Convert the NumericalType information
    /// into a std::string.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Numerical::getTypeAsString() const
    {
        return getInfo().printType();
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string.
    ///
    /// \param digits size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Numerical::print(size_t digits) const
    {
        if (m_type == INVALID)
            return "void";

        if (m_type == LOGICAL)
            return toString((bool)asUI64());

        if (m_type == DURATION && !std::isnan(asF64()))
            return formatDuration(asF64());

        if (m_type == DATETIME && !std::isnan(asF64()))
            return toString(to_timePoint(asF64()), 0);

        if (m_type <= UI64)
            return toString(asUI64());

        if (m_type <= I64)
            return toString(asI64());

        if (m_type <= DATETIME)
            return toString(asF64(), digits > 0 ? digits : 7);

        return toString(asCF64(), (asCF64().imag() != 0.0 ? 2 : 1) * (digits > 0 ? digits : 7));
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained value into a
    /// std::string considering some integer
    /// optimisations.
    ///
    /// \param digits size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Numerical::printVal(size_t digits) const
    {
        if (m_type == INVALID)
            return "void";

        if (m_type == LOGICAL)
            return toString((bool)asUI64());

        if (m_type == DURATION && !std::isnan(asF64()))
            return formatDuration(asF64());

        if (m_type == DATETIME && !std::isnan(asF64()))
            return toString(to_timePoint(asF64()), 0);

        if (m_type <= UI64)
            return toString(asUI64());

        if (m_type <= I64)
            return toString(asI64());

        if (m_type <= DATETIME)
            return toString(asF64(), digits > 0 ? digits : 7);

        // Is one of the components zero, then try to find an
        // integer optimisation
        std::complex<double> cf64 = asCF64();

        if (cf64.imag() == 0.0)
        {
            if (fabs(rint(cf64.real()) - cf64.real()) < 1e-14 && fabs(cf64.real()) >= 1.0)
                return toString(intCast(cf64.real()));
        }
        else if (cf64.real() == 0.0)
        {
            if (fabs(rint(cf64.imag()) - cf64.imag()) < 1e-14 && fabs(cf64.imag()) >= 1.0)
                return toString(intCast(cf64.imag())) + "i";
        }

        // Otherwise do not optimize due to the fact that the
        // precision will get halved in this case
        return toString(cf64, (cf64.imag() != 0.0 ? 2 : 1) * (digits > 0 ? digits : 7));
    }


    /////////////////////////////////////////////////
    /// \brief Return the number of acquired bytes
    /// for this instance.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Numerical::getBytes() const
    {
        TypeInfo info(m_type);
        return info.m_flags & TypeInfo::TYPE_COMPLEX ? info.m_bits / 4 : info.m_bits / 8;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value can
    /// safely be interpreted as an integer.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Numerical::isInt() const
    {
        return (m_type > LOGICAL && m_type <= I64) || ::isInt(asCF64());
    }






    /////////////////////////////////////////////////
    /// \brief Equal operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator==(const Category& other) const
    {
        return val == other.val && name == other.name;
    }


    /////////////////////////////////////////////////
    /// \brief Not-equal operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator!=(const Category& other) const
    {
        return !operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Less-than operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator<(const Category& other) const
    {
        return val < other.val;
    }


    /////////////////////////////////////////////////
    /// \brief Less-or-equal operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator<=(const Category& other) const
    {
        return operator<(other) || operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-than operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator>(const Category& other) const
    {
        return !operator<=(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-or-equal operator.
    ///
    /// \param other const Category&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Category::operator>=(const Category& other) const
    {
        return !operator<(other);
    }

}

