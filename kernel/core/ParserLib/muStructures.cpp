/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024 Erik Haenel et al.

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

#include "muStructures.hpp"
#include "muParserError.h"
#include "../utils/tools.hpp"
#include "../utils/timer.hpp"
#include "muHelpers.hpp"
#include "../strings/functionimplementation.hpp" // for string method callees
#include "../maths/functionimplementation.hpp" // for numerical method callees

#include "muValueImpl.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Construct an empty Value instance.
    /////////////////////////////////////////////////
    Value::Value() : std::unique_ptr<BaseValue>()
    { }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param data const Value&
    ///
    /////////////////////////////////////////////////
    Value::Value(const Value& data)
    {
        if (data.get())
            reset(data->clone());
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a BaseValue
    /// pointer (can be a nullptr).
    ///
    /// \param other BaseValue*
    ///
    /////////////////////////////////////////////////
    Value::Value(BaseValue* other) : std::unique_ptr<BaseValue>(other)
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from an Numerical.
    ///
    /// \param data const Numerical&
    ///
    /////////////////////////////////////////////////
    Value::Value(const Numerical& data)
    {
        reset(new NumValue(data));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a Category.
    ///
    /// \param data const Category&
    ///
    /////////////////////////////////////////////////
    Value::Value(const Category& data)
    {
        reset(new CatValue(data));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from an Array.
    ///
    /// \param data const Array&
    ///
    /////////////////////////////////////////////////
    Value::Value(const Array& data)
    {
        reset(new ArrValue(data));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a logical value.
    ///
    /// \param logical bool
    ///
    /////////////////////////////////////////////////
    Value::Value(bool logical) : Value(Numerical(logical))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from an int32_t.
    ///
    /// \param value int32_t
    ///
    /////////////////////////////////////////////////
    Value::Value(int32_t value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a uint32_t.
    ///
    /// \param value uint32_t
    ///
    /////////////////////////////////////////////////
    Value::Value(uint32_t value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from an int64_t.
    ///
    /// \param value int64_t
    ///
    /////////////////////////////////////////////////
    Value::Value(int64_t value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a uint64_t.
    ///
    /// \param value uint64_t
    ///
    /////////////////////////////////////////////////
    Value::Value(uint64_t value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a double.
    ///
    /// \param value double
    ///
    /////////////////////////////////////////////////
    Value::Value(double value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a time point.
    ///
    /// \param value const sys_time_point&
    ///
    /////////////////////////////////////////////////
    Value::Value(const sys_time_point& value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a float variant
    /// of a std::complex.
    ///
    /// \param value const std::complex<float>&
    ///
    /////////////////////////////////////////////////
    Value::Value(const std::complex<float>& value) : Value(Numerical(value))
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a std::complex
    /// allowing for autotyping.
    ///
    /// \param value const std::complex<double>&
    /// \param autoType bool
    ///
    /////////////////////////////////////////////////
    Value::Value(const std::complex<double>& value, bool autoType)
    {
        reset(new NumValue(Numerical(value, autoType ? AUTO : CF64)));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a std::string.
    ///
    /// \param sData const std::string&
    ///
    /////////////////////////////////////////////////
    Value::Value(const std::string& sData)
    {
        reset(new StrValue(sData));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a const char*.
    ///
    /// \param sData const char*
    ///
    /////////////////////////////////////////////////
    Value::Value(const char* sData)
    {
        reset(new StrValue(sData));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from a data type.
    ///
    /// \param type DataType
    ///
    /////////////////////////////////////////////////
    Value::Value(DataType type)
    {
        switch (type)
        {
            case TYPE_CATEGORY:
               reset(new CatValue);
                break;
            case TYPE_NEUTRAL:
                reset(new NeutralValue);
                break;
            case TYPE_NUMERICAL:
                reset(new NumValue);
                break;
            case TYPE_INVALID:
                reset(new NumValue(NAN, true));
                break;
            case TYPE_STRING:
                reset(new StrValue);
                break;
            case TYPE_ARRAY:
                reset(new ArrValue);
                break;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Return the contained general type.
    ///
    /// \return DataType
    ///
    /////////////////////////////////////////////////
    DataType Value::getType() const
    {
        if (get())
            return get()->m_type;

        return TYPE_VOID;
    }


    /////////////////////////////////////////////////
    /// \brief Return the contained type as a
    /// std::string.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Value::getTypeAsString() const
    {
        switch (getType())
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
                return getNum().getTypeAsString();
            case TYPE_STRING:
                return "string";
            case TYPE_ARRAY:
                return "cluster";
        }

        return "void";
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is empty.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isVoid() const
    {
        return !get() || get()->m_type == TYPE_NEUTRAL || get()->m_type == TYPE_INVALID;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isValid() const
    {
        return get() && get()->isValid();
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value can be
    /// interpreted as a numerical value.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isNumerical() const
    {
        return get() && (get()->m_type == TYPE_NUMERICAL || get()->m_type == TYPE_INVALID || get()->m_type == TYPE_CATEGORY);
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value can be
    /// interpreted as a string.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isString() const
    {
        return get() && (get()->m_type == TYPE_STRING || get()->m_type == TYPE_CATEGORY);
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is a
    /// category.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isCategory() const
    {
        return get() && get()->m_type == TYPE_CATEGORY;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is an
    /// Array.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isArray() const
    {
        return get() && get()->m_type == TYPE_ARRAY;
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained string.
    ///
    /// \return std::string&
    ///
    /////////////////////////////////////////////////
    std::string& Value::getStr()
    {
        if (get())
        {
            if (get()->m_type == TYPE_STRING)
                return static_cast<StrValue*>(get())->get();
            else if (get()->m_type == TYPE_CATEGORY)
                return static_cast<CatValue*>(get())->get().name;
        }

        throw ParserError(ecTYPE_NO_STR, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained string.
    ///
    /// \return const std::string&
    ///
    /////////////////////////////////////////////////
    const std::string& Value::getStr() const
    {
        if (get())
        {
            if (get()->m_type == TYPE_STRING)
                return static_cast<const StrValue*>(get())->get();
            else if (get()->m_type == TYPE_CATEGORY)
                return static_cast<const CatValue*>(get())->get().name;
        }
        else
            return m_defString;

        throw ParserError(ecTYPE_NO_STR, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Numerical.
    ///
    /// \return Numerical&
    ///
    /////////////////////////////////////////////////
    Numerical& Value::getNum()
    {
        if (get())
        {
            if (get()->m_type == TYPE_NUMERICAL || get()->m_type == TYPE_INVALID)
                return static_cast<NumValue*>(get())->get();
            else if (get()->m_type == TYPE_CATEGORY)
                return static_cast<CatValue*>(get())->get().val;
        }

        throw ParserError(ecTYPE_NO_VAL, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Numerical.
    ///
    /// \return const Numerical&
    ///
    /////////////////////////////////////////////////
    const Numerical& Value::getNum() const
    {
        if (get())
        {
            if (get()->m_type == TYPE_NUMERICAL || get()->m_type == TYPE_INVALID)
                return static_cast<const NumValue*>(get())->get();
            else if (get()->m_type == TYPE_CATEGORY)
                return static_cast<const CatValue*>(get())->get().val;
        }
        else
            return m_defVal;

        throw ParserError(ecTYPE_NO_VAL, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Category.
    ///
    /// \return Category&
    ///
    /////////////////////////////////////////////////
    Category& Value::getCategory()
    {
        if (get() && get()->m_type == TYPE_CATEGORY)
            return static_cast<CatValue*>(get())->get();

        throw ParserError(ecTYPE_NO_CAT, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Category.
    ///
    /// \return const Category&
    ///
    /////////////////////////////////////////////////
    const Category& Value::getCategory() const
    {
        if (get() && get()->m_type == TYPE_CATEGORY)
            return static_cast<const CatValue*>(get())->get();

        throw ParserError(ecTYPE_NO_CAT, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Array.
    ///
    /// \return Array&
    ///
    /////////////////////////////////////////////////
    Array& Value::getArray()
    {
        if (get() && get()->m_type == TYPE_ARRAY)
            return static_cast<ArrValue*>(get())->get();

        throw ParserError(ecTYPE_NO_ARR, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Array.
    ///
    /// \return const Array&
    ///
    /////////////////////////////////////////////////
    const Array& Value::getArray() const
    {
        if (get() && get()->m_type == TYPE_ARRAY)
            return static_cast<const ArrValue*>(get())->get();

        throw ParserError(ecTYPE_NO_ARR, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Convert the contained type to a
    /// complex in a fail-safe mode (returning NAN if
    /// a conversion is not possible).
    ///
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Value::as_cmplx() const
    {
        if (get())
        {
            if (get()->m_type == TYPE_NUMERICAL || get()->m_type == TYPE_INVALID)
                return static_cast<const NumValue*>(get())->get().asCF64();
            else if (get()->m_type == TYPE_CATEGORY)
                return static_cast<const CatValue*>(get())->get().val.asCF64();
        }

        return NAN;
    }


    /////////////////////////////////////////////////
    /// \brief Optimized power-function.
    ///
    /// \param exponent const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::pow(const Value& exponent) const
    {
        if (get() && exponent.get())
            return get()->pow(*exponent.get());

        throw ParserError(ecTYPE_MISMATCH, getTypeAsString() + " ^ " + exponent.getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Interpret the contained value as a
    /// bool.
    ///
    /// \return Value::operator
    ///
    /////////////////////////////////////////////////
    Value::operator bool() const
    {
        return get() && bool(*get());
    }


    /////////////////////////////////////////////////
    /// \brief Logical not.
    ///
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator!() const
    {
        return get() && !*get();
    }


    /////////////////////////////////////////////////
    /// \brief Equal operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator==(const Value& other) const
    {
        if (get() && other.get())
            return *get() == *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Not-equal operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator!=(const Value& other) const
    {
        if (get() && other.get())
            return *get() != *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator<(const Value& other) const
    {
        if (get() && other.get())
            return *get() < *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Less-or-equal operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator<=(const Value& other) const
    {
        if (get() && other.get())
            return *get() <= *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Greater-than operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator>(const Value& other) const
    {
        if (get() && other.get())
            return *get() > *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Greator-or-equal operator.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator>=(const Value& other) const
    {
        if (get() && other.get())
            return *get() >= *other.get();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Logical and.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator&&(const Value& other) const
    {
        return bool(*this) && bool(other);
    }


    /////////////////////////////////////////////////
    /// \brief Logical or.
    ///
    /// \param other const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::operator||(const Value& other) const
    {
        return bool(*this) || bool(other);
    }


    /////////////////////////////////////////////////
    /// \brief Convert the contained value into a
    /// string for printing on the terminal. Will
    /// append quotation marks to string values, if
    /// necessary, and can truncate the value to a
    /// number of characters.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Value::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (get())
            return get()->print(digits, chrs, trunc);

#ifdef PARSERSTANDALONE
        return "0x0";
#else
        return "void";
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Convert the contained value into a
    /// string for printing on the terminal. Won't
    /// append quotation marks to string values and
    /// can truncate the value to a number of
    /// characters.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Value::printVal(size_t digits, size_t chrs) const
    {
        if (get())
            return get()->printVal(digits, chrs);

        return "void";
    }


    /////////////////////////////////////////////////
    /// \brief Clear the contained value and free the
    /// associated memory space.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Value::clear()
    {
        reset(nullptr);
    }


    /////////////////////////////////////////////////
    /// \brief Return the number of acquired bytes
    /// for this value.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Value::getBytes() const
    {
        if (get())
            return get()->getBytes();

        return 0;
    }











    /////////////////////////////////////////////////
    /// \brief Construct an empty Array.
    /////////////////////////////////////////////////
    Array::Array() : std::vector<Value>(), m_commonType(TYPE_VOID)
    { }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param other const Array&
    ///
    /////////////////////////////////////////////////
    Array::Array(const Array& other) : Array()
    {
        operator=(other);
    }


    /////////////////////////////////////////////////
    /// \brief Fill constructor.
    ///
    /// \param n size_t
    /// \param fillVal const Value&
    ///
    /////////////////////////////////////////////////
    Array::Array(size_t n, const Value& fillVal) : std::vector<Value>(n, fillVal), m_commonType(fillVal.getType())
    { }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a single
    /// scalar Value.
    ///
    /// \param scalar const Value&
    ///
    /////////////////////////////////////////////////
    Array::Array(const Value& scalar) : Array()
    {
        if (scalar.isArray())
            operator=(scalar.getArray());
        else
        {
            push_back(scalar);
            m_commonType = scalar.getType();
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a Variable.
    ///
    /// \param var const Variable&
    ///
    /////////////////////////////////////////////////
    Array::Array(const Variable& var) : std::vector<Value>(var), m_commonType(var.m_commonType)
    { }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of std::complex values.
    ///
    /// \param other const std::vector<std::complex<double>>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<std::complex<double>>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = Numerical::autoType(other[i]);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of double values.
    ///
    /// \param other const std::vector<double>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<double>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = Numerical(other[i]);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of size_t values.
    ///
    /// \param other const std::vector<size_t>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<size_t>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = Numerical(other[i]);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of int64_t values.
    ///
    /// \param other const std::vector<int64_t>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<int64_t>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = Numerical(other[i]);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of Numericals.
    ///
    /// \param other const std::vector<Numerical>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<Numerical>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of std::strings.
    ///
    /// \param other const std::vector<std::string>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<std::string>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_STRING)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }


    /////////////////////////////////////////////////
    /// \brief Make this array a 2-val generator
    /// array.
    ///
    /// \param fst const Array&
    /// \param lst const Array&
    ///
    /////////////////////////////////////////////////
    Array::Array(const Array& fst, const Array& lst) : std::vector<Value>({fst,lst}), m_commonType(TYPE_GENERATOR)
    { }


    /////////////////////////////////////////////////
    /// \brief Make this array a 3-val generator
    /// array.
    ///
    /// \param fst const Array&
    /// \param inc const Array&
    /// \param lst const Array&
    ///
    /////////////////////////////////////////////////
    Array::Array(const Array& fst, const Array& inc, const Array& lst) : std::vector<Value>({fst,inc,lst}), m_commonType(TYPE_GENERATOR)
    { }


    /////////////////////////////////////////////////
    /// \brief Get the (general) data types of every
    /// contained Value.
    ///
    /// \return std::vector<DataType>
    ///
    /////////////////////////////////////////////////
    std::vector<DataType> Array::getType() const
    {
        std::vector<DataType> types;
        size_t elems = size();
        types.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            types.push_back(operator[](i).getType());

            if (m_commonType == TYPE_VOID
                || ((m_commonType == TYPE_NEUTRAL || m_commonType == TYPE_INVALID) && types.back() != TYPE_VOID))
                m_commonType = types.back();

            if (types.back() != TYPE_VOID
                && types.back() != TYPE_NEUTRAL
                && types.back() != TYPE_INVALID
                && m_commonType != types.back())
                m_commonType = TYPE_MIXED;
        }

        if (m_commonType == TYPE_NEUTRAL || m_commonType == TYPE_INVALID)
            m_commonType = TYPE_NUMERICAL;

        return types;
    }


    /////////////////////////////////////////////////
    /// \brief Get the common (general) data type
    /// representing all contained Values best.
    ///
    /// \return DataType
    ///
    /////////////////////////////////////////////////
    DataType Array::getCommonType() const
    {
        if (m_commonType == TYPE_VOID && size())
            getType();

        return m_commonType;
    }


    /////////////////////////////////////////////////
    /// \brief Get the common (general) data type
    /// representing all contained Values best as a
    /// printable std::string.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::getCommonTypeAsString() const
    {
        switch (getCommonType())
        {
            case TYPE_CATEGORY:
                return "category";
            case TYPE_NUMERICAL:
            {
                TypeInfo info = front().getNum().getInfo();

                for (size_t i = 1; i < size(); i++)
                {
                    info.promote(operator[](i).getNum().getInfo());
                }

                return info.printType();
            }
            case TYPE_STRING:
                return "string";
            case TYPE_MIXED:
                return "cluster";
        }

        return "void";
    }


    /////////////////////////////////////////////////
    /// \brief Get the common (promoted)
    /// NumericalType of all contained Values
    /// assuming that all Values are Numericals.
    ///
    /// \return NumericalType
    ///
    /////////////////////////////////////////////////
    NumericalType Array::getCommonNumericalType() const
    {
        TypeInfo info = front().getNum().getInfo();

        for (size_t i = 1; i < size(); i++)
        {
            info.promote(operator[](i).getNum().getInfo());
        }

        return info.asType();
    }


    /////////////////////////////////////////////////
    /// \brief True, if only a single value is
    /// contained within this Array.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Array::isScalar() const
    {
        return size() == 1u;
    }


    /////////////////////////////////////////////////
    /// \brief True, if this Array represents a
    /// default value (an empty array in this
    /// implementation).
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Array::isDefault() const
    {
        return !size();
    }


    /////////////////////////////////////////////////
    /// \brief Optimized power function.
    ///
    /// \param exponent const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::pow(const Array& exponent) const
    {
        Array ret;
        size_t elements = std::max(size(), exponent.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i).pow(exponent.get(i)));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Optimized power function.
    ///
    /// \param exponent const Numerical&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::pow(const Numerical& exponent) const
    {
        Array ret;
        size_t elements = size();
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i).pow(exponent));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Logical not.
    ///
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator!() const
    {
        Array ret;
        size_t elements = size();
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(operator[](i).operator!());
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Equal operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator==(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) == other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Not-equal operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator!=(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) != other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator<(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) < other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Less-or-equal operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator<=(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) <= other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Greater-than operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator>(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) > other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Greater-or-equal operator.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator>=(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) >= other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Logical and.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator&&(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) && other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Logical or.
    ///
    /// \param other const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::operator||(const Array& other) const
    {
        Array ret;
        size_t elements = std::max(size(), other.size());
        ret.reserve(elements);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(get(i) || other.get(i));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Returns an unwrapped version of this
    /// array, i.e. all sub-array are rolled out.
    ///
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::unWrap() const
    {
        Array ret;
        ret.reserve(size());

        for (size_t i = 0; i < size(); i++)
        {
            const Value& v = get(i);

            if (v.isArray())
            {
                Array unWrapped = v.getArray().unWrap();
                ret.insert(ret.end(), unWrapped.begin(), unWrapped.end());
            }
            else
                ret.push_back(v);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::call(const std::string& sMethod) const
    {
        if (sMethod == "len")
            return strfnc_strlen(*this);
        else if (sMethod == "first")
            return strfnc_firstch(*this);
        else if (sMethod == "last")
            return strfnc_lastch(*this);
        else if (sMethod == "std")
            return numfnc_Std(this); // Pointer as single-element array
        else if (sMethod == "avg")
            return numfnc_Avg(this);
        else if (sMethod == "prd")
            return numfnc_product(this);
        else if (sMethod == "sum")
            return numfnc_Sum(this);
        else if (sMethod == "min")
            return numfnc_Min(this);
        else if (sMethod == "max")
            return numfnc_Max(this);
        else if (sMethod == "norm")
            return numfnc_Norm(this);
        else if (sMethod == "num")
            return numfnc_Num(this);
        else if (sMethod == "cnt")
            return numfnc_Cnt(this);
        else if (sMethod == "med")
            return numfnc_Med(this);
        else if (sMethod == "and")
            return numfnc_and(this);
        else if (sMethod == "or")
            return numfnc_or(this);
        else if (sMethod == "xor")
            return numfnc_xor(this);
        else if (sMethod == "size")
            return numfnc_Cnt(this);
        else if (sMethod == "maxpos")
            return numfnc_MaxPos(*this);
        else if (sMethod == "minpos")
            return numfnc_MinPos(*this);
        else if (sMethod == "exc")
            return numfnc_Exc(this);
        else if (sMethod == "skw")
            return numfnc_Skew(this);
        else if (sMethod == "stderr")
            return numfnc_StdErr(this);
        else if (sMethod == "rms")
            return numfnc_Rms(this);
        else if (sMethod == "order")
            return numfnc_order(this);
        else if (sMethod == "unwrap")
            return unWrap();

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::call(const std::string& sMethod, const Array& arg1) const
    {
        if (sMethod == "at")
            return strfnc_char(*this, arg1);
        else if (sMethod == "startsw")
            return strfnc_startswith(*this, arg1);
        else if (sMethod == "endsw")
            return strfnc_endswith(*this, arg1);
        else if (sMethod == "sel")
            return numfnc_getElements(*this, arg1);
        else if (sMethod == "sub")
            return strfnc_substr(*this, arg1, mu::Array());
        else if (sMethod == "splt")
            return strfnc_split(*this, arg1, mu::Array());
        else if (sMethod == "fnd")
            return strfnc_strfnd(arg1, *this, mu::Array());
        else if (sMethod == "rfnd")
            return strfnc_strrfnd(arg1, *this, mu::Array());
        else if (sMethod == "mtch")
            return strfnc_strmatch(arg1, *this, mu::Array());
        else if (sMethod == "rmtch")
            return strfnc_strrmatch(arg1, *this, mu::Array());
        else if (sMethod == "nmtch")
            return strfnc_str_not_match(arg1, *this, mu::Array());
        else if (sMethod == "nrmtch")
            return strfnc_str_not_rmatch(arg1, *this, mu::Array());

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::call(const std::string& sMethod, const Array& arg1, const Array& arg2) const
    {
        if (sMethod == "sub")
            return strfnc_substr(*this, arg1, arg2);
        else if (sMethod == "splt")
            return strfnc_split(*this, arg1, arg2);
        else if (sMethod == "fnd")
            return strfnc_strfnd(arg1, *this, arg2);
        else if (sMethod == "rfnd")
            return strfnc_strrfnd(arg1, *this, arg2);
        else if (sMethod == "mtch")
            return strfnc_strmatch(arg1, *this, arg2);
        else if (sMethod == "rmtch")
            return strfnc_strrmatch(arg1, *this, arg2);
        else if (sMethod == "nmtch")
            return strfnc_str_not_match(arg1, *this, arg2);
        else if (sMethod == "nrmtch")
            return strfnc_str_not_rmatch(arg1, *this, arg2);

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Interpret the Array as a scalar
    /// (ignoring all other values) and return the
    /// first value as an int64_t.
    ///
    /// \return int64_t
    ///
    /////////////////////////////////////////////////
    int64_t Array::getAsScalarInt() const
    {
        return front().getNum().asI64();
    }


    /////////////////////////////////////////////////
    /// \brief Convert the whole Array into a
    /// std::vector full of std::strings assuming
    /// that only strings are contained within this
    /// Array.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
    std::vector<std::string> Array::as_str_vector() const
    {
        std::vector<std::string> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).getStr());
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Convert the whole Array into a
    /// std::vector full of std::complex assuming
    /// that only Numericals are contained within
    /// this Array.
    ///
    /// \return std::vector<std::complex<double>>
    ///
    /////////////////////////////////////////////////
    std::vector<std::complex<double>> Array::as_cmplx_vector() const
    {
        std::vector<std::complex<double>> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).as_cmplx());
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Convert the whole Array into a
    /// std::vector full of std::strings by printing
    /// every value.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
    std::vector<std::string> Array::to_string() const
    {
        std::vector<std::string> ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.push_back(operator[](i).print());
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Print all contained Values into a
    /// single std::string (possibly adding quotation
    /// marks).
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (isDefault())
#ifdef PARSERSTANDALONE
            return "default";
#else
            return "void";
#endif

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += operator[](i).print(digits, chrs, trunc);
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Print all contained Values into a
    /// single std::string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::printVals(size_t digits, size_t chrs) const
    {
        if (isDefault())
            return "void";

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += operator[](i).printVal(digits, chrs);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Print the dimensions of this Array.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::printDims() const
    {
        return toString(size()) + " x 1";
    }


    /////////////////////////////////////////////////
    /// \brief Print all values using a strjoin
    /// logic with a customizable separator.
    ///
    /// \param sSep const std::string&
    /// \param keepEmpty bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::printJoined(const std::string& sSep, bool keepEmpty) const
    {
        std::string sRet;

        for (size_t i = 0; i < size(); i++)
        {
            std::string sVal = operator[](i).printVal();

            if ((sRet.length() && sVal.length())
                || (i && keepEmpty))
                sRet += sSep;

            sRet += sVal;
        }

        return sRet;
    }


    /////////////////////////////////////////////////
    /// \brief Print an overview of the contained
    /// elements, i.e. the first and the last few
    /// elements in the array.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param maxElems size_t
    /// \param alwaysBraces bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::printOverview(size_t digits, size_t chrs, size_t maxElems, bool alwaysBraces) const
    {
        // Return an empty brace pair, if no data is
        // available
        if (!size())
            return alwaysBraces ? "{}" : "void";

        // If only one value is available, omit the brace
        if (size() == 1 && !alwaysBraces)
            return print(digits, chrs, false);

        std::string sVector;

        // Append the contained data depending on its type but
        // restrict the number to maximal five values (use the first
        // and the last ones) and insert an ellipsis in the middle
        for (size_t i = 0; i < size(); i++)
        {
            if (sVector.size())
                sVector += ", ";

            sVector += get(i).print(digits, chrs < std::string::npos ? chrs/4 : std::string::npos, false);

            // Insert the ellipsis in the middle. The additional -1 is to
            // handle the zero-based indices
            if (i == maxElems / 2 - 1 && size() > maxElems)
            {
                sVector += ", ...";
                i = size()-maxElems / 2 - 1;
            }
        }

        return "{" + sVector + "}";
    }


    /////////////////////////////////////////////////
    /// \brief Return the number of acquired bytes
    /// for this Array.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Array::getBytes() const
    {
        size_t bytes = 0;

        for (size_t i = 0; i < size(); i++)
        {
            bytes += operator[](i).getBytes();
        }

        return bytes;
    }


    /////////////////////////////////////////////////
    /// \brief Helper method to convert all contained
    /// numerical types to void values (needed for
    /// some parser optimizations).
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Array::zerosToVoid()
    {
        if (getCommonType() != TYPE_NUMERICAL)
            return;

        bool canBeDefaulted = true;

        for (size_t i = 0; i < size(); i++)
        {
            if (operator[](i).getNum().asCF64() == 0.0)
                operator[](i).clear();
            else
                canBeDefaulted = false;
        }

        if (canBeDefaulted)
            clear();

        m_commonType = TYPE_VOID;
    }


    /////////////////////////////////////////////////
    /// \brief Helper method returning true, if the
    /// underlying data type is commutative, i.e.
    /// A+B == B+A (which is not true for strings,
    /// for example).
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Array::isCommutative() const
    {
        return getCommonType() == TYPE_NUMERICAL;
    }







    /////////////////////////////////////////////////
    /// \brief Construct an empty Variable.
    /////////////////////////////////////////////////
    Variable::Variable() : Array()
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Variable from a Value.
    ///
    /// \param data const Value&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Value& data) : Array(data)
    { }


    /////////////////////////////////////////////////
    /// \brief Construct a Variable from an Array.
    ///
    /// \param data const Array&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Array& data) : Array(data)
    { }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param data const Variable&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Variable& data) : Array(data)
    { }


    /////////////////////////////////////////////////
    /// \brief Move constructor.
    ///
    /// \param data Array&&
    ///
    /////////////////////////////////////////////////
    /*Variable::Variable(Array&& data) : Array(data)
    { }*/


    /////////////////////////////////////////////////
    /// \brief Assign a Value checking for type
    /// compatibility.
    ///
    /// \param other const Value&
    /// \return Variable&
    ///
    /////////////////////////////////////////////////
    Variable& Variable::operator=(const Value& other)
    {
        if (other.getType() == TYPE_ARRAY)
            return Variable::operator=(other.getArray());

        if (getCommonType() == TYPE_VOID || getCommonType() == other.getType())
        {
            Array::operator=(Array(other));
            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign a Variable checking for type
    /// compatibility.
    ///
    /// \param other const Variable&
    /// \return Variable&
    ///
    /////////////////////////////////////////////////
    Variable& Variable::operator=(const Variable& other)
    {
        if (getCommonType() == TYPE_VOID || getType() == other.getType())
        {
            Array::operator=(other);
            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Force-overwrite the contained values
    /// of this Array with another Array.
    ///
    /// \param other const Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Variable::overwrite(const Array& other)
    {
        Array::operator=(other);
    }






    /////////////////////////////////////////////////
    /// \brief Construct a VarArray from a single
    /// Variable.
    ///
    /// \param var Variable*
    ///
    /////////////////////////////////////////////////
    VarArray::VarArray(Variable* var) : std::vector<Variable*>(1, var)
    {
        //
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign the values of an Array.
    ///
    /// \param values const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::operator+=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) += values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() += values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) += values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign the values of an Array.
    ///
    /// \param values const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::operator-=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) -= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() -= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) -= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign the values of an Array.
    ///
    /// \param values const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::operator*=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) *= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() *= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) *= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign the values of an Array.
    ///
    /// \param values const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::operator/=(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) /= values.front();
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }
        else if (size() == 1)
            ret = *front() /= values;
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                *operator[](i) /= values[i];
                ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
            }
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Optimized power function.
    ///
    /// \param values const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::pow(const Array& values)
    {
        Array ret;

        if (values.isScalar())
        {
            for (size_t i = 0; i < size(); i++)
            {
                Array res = operator[](i)->pow(values.front());
                ret.insert(ret.end(), res.begin(), res.end());
            }
        }
        else if (size() == 1)
            ret = front()->pow(values);
        else
        {
            for (size_t i = 0; i < std::min(size(), values.size()); i++)
            {
                Array res = operator[](i)->pow(values[i]);
                ret.insert(ret.end(), res.begin(), res.end());
            }
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Assign a std::vector of Arrays.
    ///
    /// \param arrayList const std::vector<Array>&
    /// \return VarArray&
    ///
    /////////////////////////////////////////////////
    VarArray& VarArray::operator=(const std::vector<Array>& arrayList)
    {
        if (arrayList.size() == 1u)
        {
            for (size_t i = 0; i < size(); i++)
            {
                *operator[](i) = arrayList.front();
            }

            return *this;
        }

        for (size_t i = 0; i < std::min(size(), arrayList.size()); i++)
        {
            *operator[](i) = arrayList[i];
        }

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Equal operator, checking the equality
    /// of pointers.
    ///
    /// \param other const VarArray&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool VarArray::operator==(const VarArray& other) const
    {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); i++)
        {
            if (operator[](i) != other[i])
                return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Return true, if this VarArray does not
    /// contain any Variables.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool VarArray::isNull() const
    {
        return size() == 0u;
    }


    /////////////////////////////////////////////////
    /// \brief Print the pointer addresses in
    /// hexadecimal numbers.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string VarArray::print() const
    {
        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.size())
                ret += ", ";

            ret += toHexString((size_t)operator[](i));
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Convert this VarArray as a single
    /// Array.
    ///
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array VarArray::asArray() const
    {
        Array ret;

        for (size_t i = 0; i < size(); i++)
        {
            ret.insert(ret.end(), operator[](i)->begin(), operator[](i)->end());
        }

        return ret;
    }





    // Instantiation of the static member variables
    const std::string Value::m_defString;
    const Numerical Value::m_defVal;
    const Value Array::m_default(TYPE_NEUTRAL);





    /////////////////////////////////////////////////
    /// \brief Check if all Values in the Array
    /// evaluate to true (vector-and).
    ///
    /// \param arr const Array&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool all(const Array& arr)
    {
        if (!arr.size())
            return false;

        for (size_t i = 0; i < arr.size(); i++)
        {
            if (!arr[i])
                return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Check if at least one Value in the
    /// Array evaluates to true (vector-or).
    ///
    /// \param arr const Array&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool any(const Array& arr)
    {
        for (size_t i = 0; i < arr.size(); i++)
        {
            if (arr[i])
                return true;
        }

        return false;
    }




    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param arr const Array&
    /// \param v const Value&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator+(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a+v);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param arr const Array&
    /// \param v const Value&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator-(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a-v);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply operator.
    ///
    /// \param arr const Array&
    /// \param v const Value&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator*(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a*v);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Divide operator.
    ///
    /// \param arr const Array&
    /// \param v const Value&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator/(const Array& arr, const Value& v)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(a/v);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param v const Value&
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator+(const Value& v, const Array& arr)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(v+a);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param v const Value&
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator-(const Value& v, const Array& arr)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(v-a);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply operator.
    ///
    /// \param v const Value&
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator*(const Value& v, const Array& arr)
    {
        return arr*v;
    }


    /////////////////////////////////////////////////
    /// \brief Divide operator.
    ///
    /// \param v const Value&
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator/(const Value& v, const Array& arr)
    {
        Array ret;

        for (const auto& a : arr)
        {
            ret.push_back(v/a);
        }

        return ret;
    }




    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param arr const Array&
    /// \param v double
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator+(const Array& arr, double v)
    {
        return arr+Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param arr const Array&
    /// \param v double
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator-(const Array& arr, double v)
    {
        return arr-Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Multiply operator.
    ///
    /// \param arr const Array&
    /// \param v double
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator*(const Array& arr, double v)
    {
        return arr*Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Divide operator.
    ///
    /// \param arr const Array&
    /// \param v double
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator/(const Array& arr, double v)
    {
        return arr/Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Add operator.
    ///
    /// \param v double
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator+(double v, const Array& arr)
    {
        return arr+Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Subtract operator.
    ///
    /// \param v double
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator-(double v, const Array& arr)
    {
        return Value(v)-arr;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply operator.
    ///
    /// \param v double
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator*(double v, const Array& arr)
    {
        return arr*Value(v);
    }


    /////////////////////////////////////////////////
    /// \brief Divide operator.
    ///
    /// \param v double
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array operator/(double v, const Array& arr)
    {
        return Value(v)/arr;
    }
}


