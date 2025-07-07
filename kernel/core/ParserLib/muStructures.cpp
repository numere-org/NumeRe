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
    Value::Value(BaseValue* other)
    {
        if (other && other->m_type == TYPE_ARRAY)
        {
            ArrValue* arrVal = static_cast<ArrValue*>(other);

            if (arrVal->get().size() == 1)
            {
                operator=(arrVal->get().front());
                delete other;
            }
            else
                reset(other);
        }
        else
            reset(other);
    }


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
        if (data.size() == 1)
        {
            if (data.front().get())
                reset(data.front().get()->clone());
        }
        else
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
    /// \brief Construct a Value from a DictStruct
    /// instance.
    ///
    /// \param dict const DictStruct&
    ///
    /////////////////////////////////////////////////
    Value::Value(const DictStruct& dict)
    {
        reset(new DictStructValue(dict));
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Value from an Object
    /// instance.
    ///
    /// \param obj const Object&
    ///
    /////////////////////////////////////////////////
    Value::Value(const Object& obj)
    {
        reset(obj.clone());
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
                reset(new NumValue(NAN, false));
                break;
            case TYPE_CLUSTER:
            case TYPE_INVALID:
                reset(new NumValue(NAN, true));
                break;
            case TYPE_STRING:
                reset(new StrValue);
                break;
            case TYPE_ARRAY:
                reset(new ArrValue);
                break;
            case TYPE_DICTSTRUCT:
                reset(new DictStructValue);
                break;
            case TYPE_VOID:
            case TYPE_OBJECT:
            case TYPE_GENERATOR:
            case TYPE_REFERENCE:
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
            return get()->getType();

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
                return getArray().getCommonTypeAsString();
            case TYPE_DICTSTRUCT:
                return "dictstruct";
            case TYPE_OBJECT:
                if (isVoid())
                    return "object.void";
                return "object." + getObject().getObjectType();
            case TYPE_CLUSTER:
                return "cluster";
            case TYPE_REFERENCE:
                return "reference";
            case TYPE_GENERATOR:
                return "generator";
            case TYPE_VOID:
            case TYPE_INVALID:
                break;
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
        return get() && (get()->getType() == TYPE_NUMERICAL || get()->getType() == TYPE_INVALID || get()->getType() == TYPE_CATEGORY);
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
        return get() && (get()->getType() == TYPE_STRING || get()->getType() == TYPE_CATEGORY);
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
        return get() && get()->getType() == TYPE_CATEGORY;
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
        return get() && get()->getType() == TYPE_ARRAY;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is a
    /// DictStruct.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isDictStruct() const
    {
        return get() && get()->getType() == TYPE_DICTSTRUCT;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is an
    /// Object.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isObject() const
    {
        return get() && get()->getType() == TYPE_OBJECT;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is a
    /// reference.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isRef() const
    {
        return get() && get()->m_type == TYPE_REFERENCE;
    }


    /////////////////////////////////////////////////
    /// \brief True, if the contained value is a
    /// Generator.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Value::isGenerator() const
    {
        return get() && get()->m_type == TYPE_GENERATOR;
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
            else if (isRef() && get()->getType() == TYPE_STRING)
                return static_cast<StrValue&>(getRef().get()).get();
            else if (isRef() && get()->getType() == TYPE_CATEGORY)
                return static_cast<CatValue&>(getRef().get()).get().name;
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
            else if (isRef() && get()->getType() == TYPE_STRING)
                return static_cast<const StrValue&>(getRef().get()).get();
            else if (isRef() && get()->getType() == TYPE_CATEGORY)
                return static_cast<const CatValue&>(getRef().get()).get().name;
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
            else if (isRef() && (get()->getType() == TYPE_NUMERICAL || get()->getType() == TYPE_INVALID))
                return static_cast<NumValue&>(getRef().get()).get();
            else if (isRef() && get()->getType() == TYPE_CATEGORY)
                return static_cast<CatValue&>(getRef().get()).get().val;
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
            else if (isRef() && (get()->getType() == TYPE_NUMERICAL || get()->getType() == TYPE_INVALID))
                return static_cast<const NumValue&>(getRef().get()).get();
            else if (isRef() && get()->getType() == TYPE_CATEGORY)
                return static_cast<const CatValue&>(getRef().get()).get().val;
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
        else if (isRef() && get()->getType() == TYPE_CATEGORY)
            return static_cast<CatValue&>(getRef().get()).get();

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
        else if (isRef() && get()->getType() == TYPE_CATEGORY)
            return static_cast<const CatValue&>(getRef().get()).get();

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
        else if (isRef() && get()->getType() == TYPE_ARRAY)
            return static_cast<ArrValue&>(getRef().get()).get();

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
        else if (isRef() && get()->getType() == TYPE_ARRAY)
            return static_cast<const ArrValue&>(getRef().get()).get();

        throw ParserError(ecTYPE_NO_ARR, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained DictStruct.
    ///
    /// \return DictStruct&
    ///
    /////////////////////////////////////////////////
    DictStruct& Value::getDictStruct()
    {
        if (get() && get()->m_type == TYPE_DICTSTRUCT)
            return static_cast<DictStructValue*>(get())->get();
        else if (isRef() && get()->getType() == TYPE_DICTSTRUCT)
            return static_cast<DictStructValue&>(getRef().get()).get();

        throw ParserError(ecTYPE_NO_DICT, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained DictStruct.
    ///
    /// \return const DictStruct&
    ///
    /////////////////////////////////////////////////
    const DictStruct& Value::getDictStruct() const
    {
        if (get() && get()->m_type == TYPE_DICTSTRUCT)
            return static_cast<const DictStructValue*>(get())->get();
        else if (isRef() && get()->getType() == TYPE_DICTSTRUCT)
            return static_cast<const DictStructValue&>(getRef().get()).get();

        throw ParserError(ecTYPE_NO_DICT, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Object.
    ///
    /// \return Object&
    ///
    /////////////////////////////////////////////////
    Object& Value::getObject()
    {
        if (get() && get()->m_type == TYPE_OBJECT)
            return *static_cast<Object*>(get());
        else if (isRef() && get()->getType() == TYPE_OBJECT)
            return static_cast<Object&>(getRef().get());

        throw ParserError(ecTYPE_NO_OBJ, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained Object.
    ///
    /// \return const Object&
    ///
    /////////////////////////////////////////////////
    const Object& Value::getObject() const
    {
        if (get() && get()->m_type == TYPE_OBJECT)
            return *static_cast<const Object*>(get());
        else if (isRef() && get()->getType() == TYPE_OBJECT)
            return static_cast<const Object&>(getRef().get());

        throw ParserError(ecTYPE_NO_OBJ, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained RefValue.
    ///
    /// \return RefValue&
    ///
    /////////////////////////////////////////////////
    RefValue& Value::getRef()
    {
        if (get() && get()->m_type == TYPE_REFERENCE)
            return *static_cast<RefValue*>(get());

        throw ParserError(ecTYPE_NO_REF, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained RefValue.
    ///
    /// \return const RefValue&
    ///
    /////////////////////////////////////////////////
    const RefValue& Value::getRef() const
    {
        if (get() && get()->m_type == TYPE_REFERENCE)
            return *static_cast<const RefValue*>(get());

        throw ParserError(ecTYPE_NO_REF, getTypeAsString());
    }


    /////////////////////////////////////////////////
    /// \brief Get the contained GeneratorValue.
    ///
    /// \return const GeneratorValue&
    ///
    /////////////////////////////////////////////////
    const GeneratorValue& Value::getGenerator() const
    {
        if (get() && get()->m_type == TYPE_GENERATOR)
            return *static_cast<const GeneratorValue*>(get());

        throw ParserError(ecTYPE_NO_REF, getTypeAsString());
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
        if (!get() || !isNumerical())
            return NAN;

        return getNum().asCF64();
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
    /// \brief Does this value object have the
    /// requested method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition Value::isMethod(const std::string& sMethod, size_t argc) const
    {
        if (get())
            return get()->isMethod(sMethod, argc);

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::call(const std::string& sMethod) const
    {
        if (!get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->call(sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::call(const std::string& sMethod, const Value& arg1) const
    {
        if (!get() || !arg1.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->call(sMethod, *arg1.get());
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::call(const std::string& sMethod, const Value& arg1, const Value& arg2) const
    {
        if (!get() || !arg1.get() || !arg2.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->call(sMethod, *arg1.get(), *arg2.get());
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \param arg3 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3) const
    {
        if (!get() || !arg1.get() || !arg2.get() || !arg2.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->call(sMethod, *arg1.get(), *arg2.get(), *arg3.get());
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \param arg3 const Value&
    /// \param arg4 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::call(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4) const
    {
        if (!get() || !arg1.get() || !arg2.get() || !arg3.get() || !arg4.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->call(sMethod, *arg1.get(), *arg2.get(), *arg3.get(), *arg4.get());
    }


    /////////////////////////////////////////////////
    /// \brief Does this value object have the
    /// requested applying method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition Value::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        if (get())
            return get()->isApplyingMethod(sMethod, argc);

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::apply(const std::string& sMethod)
    {
        if (!get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->apply(sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::apply(const std::string& sMethod, const Value& arg1)
    {
        if (!get() || !arg1.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->apply(sMethod, *arg1.get());
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::apply(const std::string& sMethod, const Value& arg1, const Value& arg2)
    {
        if (!get() || !arg1.get() || !arg2.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->apply(sMethod, *arg1.get(), *arg2.get());
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \param arg3 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3)
    {
        if (!get() || !arg1.get() || !arg2.get() || !arg2.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->apply(sMethod, *arg1.get(), *arg2.get(), *arg3.get());
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Value&
    /// \param arg2 const Value&
    /// \param arg3 const Value&
    /// \param arg4 const Value&
    /// \return Value
    ///
    /////////////////////////////////////////////////
    Value Value::apply(const std::string& sMethod, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4)
    {
        if (!get() || !arg1.get() || !arg2.get() || !arg3.get() || !arg4.get())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        return get()->apply(sMethod, *arg1.get(), *arg2.get(), *arg3.get(), *arg4.get());
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
    /// string for printing on the terminal. Will
    /// append quotation marks to string values, if
    /// necessary, and can truncate the value to a
    /// number of characters. Structures and arrays
    /// are shortened in this variant.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Value::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        if (get())
            return get()->printEmbedded(digits, chrs, trunc);

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
    Array::Array() : std::vector<Value>(), m_commonType(TYPE_VOID), m_isConst(true)
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
    Array::Array(size_t n, const Value& fillVal) : std::vector<Value>(n, fillVal), m_commonType(fillVal.getType()), m_isConst(true)
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
    Array::Array(const Variable& var) : std::vector<Value>(var), m_commonType(var.m_commonType), m_isConst(true)
    { }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of std::complex values.
    ///
    /// \param other const std::vector<std::complex<double>>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<std::complex<double>>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
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
    Array::Array(const std::vector<double>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
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
    Array::Array(const std::vector<size_t>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = Numerical(other[i]);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a std::vector
    /// of int values.
    ///
    /// \param other const std::vector<int>&
    ///
    /////////////////////////////////////////////////
    Array::Array(const std::vector<int>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
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
    Array::Array(const std::vector<int64_t>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
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
    Array::Array(const std::vector<Numerical>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_NUMERICAL), m_isConst(true)
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
    Array::Array(const std::vector<std::string>& other) : std::vector<Value>(other.size()), m_commonType(TYPE_STRING), m_isConst(true)
    {
        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }
    }


    /////////////////////////////////////////////////
    /// \brief Construct an Array from a
    /// GeneratorValue.
    ///
    /// \param generator GeneratorValue*
    ///
    /////////////////////////////////////////////////
    Array::Array(GeneratorValue* generator) : std::vector<Value>(1u), m_commonType(TYPE_GENERATOR_CONSTRUCTOR), m_isConst(true)
    {
        first().reset(generator);
    }


    /////////////////////////////////////////////////
    /// \brief Get the (general) data types of every
    /// contained Value.
    ///
    /// \param common& DataType
    /// \return std::vector<DataType>
    ///
    /////////////////////////////////////////////////
    std::vector<DataType> Array::getType(DataType& common) const
    {
        std::vector<DataType> types;

        if (count() == 1 && first().isRef() && first().isArray())
            return first().getArray().getType(common);

        size_t elems = count();
        types.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            types.push_back(operator[](i).getType());

            if (common == TYPE_VOID
                || ((common == TYPE_NEUTRAL || common == TYPE_INVALID) && types.back() != TYPE_VOID))
                common = types.back();

            if (types.back() != TYPE_VOID
                && types.back() != TYPE_NEUTRAL
                && types.back() != TYPE_INVALID
                && common != types.back())
                common = TYPE_CLUSTER;
        }

        if (common == TYPE_NEUTRAL || common == TYPE_INVALID)
            common = TYPE_NUMERICAL;

        if (common == TYPE_ARRAY)
            common = TYPE_CLUSTER;

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
            getType(m_commonType);

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
        if (count() == 1 && first().isRef() && first().isArray())
            return first().getArray().getCommonTypeAsString();

        switch (getCommonType())
        {
            case TYPE_CATEGORY:
                return "category";
            case TYPE_NUMERICAL:
            {
                if (empty())
                    return "value.void";

                TypeInfo info = first().getNum().getInfo();

                for (size_t i = 1; i < size(); i++)
                {
                    info.promote(get(i).getNum().getInfo());
                }

                return info.printType();
            }
            case TYPE_STRING:
                return "string";
            case TYPE_CLUSTER:
                return "cluster";
            case TYPE_ARRAY:
                return "cluster*";
            case TYPE_DICTSTRUCT:
                return "dictstruct";
            case TYPE_OBJECT:
            {
                if (empty())
                    return "object.void";

                std::string sType = first().getObject().getObjectType();

                for (size_t i = 1; i < size(); i++)
                {
                    const Object& obj = get(i).getObject();

                    if (sType == "void")
                        sType = obj.getObjectType();
                    else if (sType != obj.getObjectType())
                    {
                        sType = "*";
                        break;
                    }
                }

                return "object." + sType;
            }
            case TYPE_REFERENCE:
                return "reference";
            case TYPE_GENERATOR:
                return "generator";
            case TYPE_VOID:
            case TYPE_NEUTRAL:
            case TYPE_INVALID:
                break;
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
        TypeInfo info = get(0).getNum().getInfo();

        for (size_t i = 1; i < size(); i++)
        {
            info.promote(get(i).getNum().getInfo());
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
    /// \brief Determine, whether this Array is void
    /// as a whole.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Array::isVoid() const
    {
        if (isDefault())
            return true;

        std::vector<DataType> types = getType(m_commonType);

        return std::find_if(types.begin(), types.end(), [](DataType type){return type != TYPE_VOID;}) == types.end();
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
            ret.emplace_back(get(i).operator!());
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
    /// \brief Return this Array with any generators
    /// expanded.
    ///
    /// \return Array
    ///
    /// \warning That might not fit into memory!
    /////////////////////////////////////////////////
    Array Array::expandGenerators() const
    {
        Array expanded;
        size_t elems = size();
        expanded.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            expanded.push_back(get(i));
        }

        return expanded;
    }


    /////////////////////////////////////////////////
    /// \brief Determine, whether the passed string
    /// corresponds to a method.
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition Array::isMethod(const std::string& sMethod, size_t argc) const
    {
        static const MethodSet methods({{"std", 0}, {"avg", 0}, {"prd", 0}, {"sum", 0}, {"min", 0}, {"max", 0}, {"norm", 0},
                                        {"num", 0}, {"cnt", 0}, {"med", 0}, {"and", 0}, {"or", 0}, {"xor", 0}, {"size", 0},
                                        {"maxpos", 0}, {"minpos", 0}, {"exc", 0}, {"skw", 0}, {"stderr", 0}, {"rms", 0},
                                        {"unwrap", 0}, {"sel", -1}, {"delegate", 1}, {"delegate", -2},
                                        {"order", 0}, {"order", -1}});

        auto iter = methods.find(MethodDefinition(sMethod, argc));

        if (iter != methods.end())
            return *iter;

        return front().isMethod(sMethod, argc);
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
        if (size() == 1 && first().isArray())
            return first().getArray().call(sMethod);

        if (sMethod == "std")
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
            return order();
        else if (sMethod == "unwrap")
            return unWrap();
        else if (front().isMethod(sMethod, 0))
        {
            Array ret;
            ret.reserve(size());

            for (size_t i = 0; i < size(); i++)
            {
                ret.emplace_back(get(i).call(sMethod));
            }

            return ret;
        }

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
        if (size() == 1 && first().isArray())
            return first().getArray().call(sMethod, arg1);

        if (sMethod == "sel")
            return numfnc_getElements(*this, arg1);
        else if (sMethod == "delegate" && arg1.getCommonType() == TYPE_STRING)
        {
            Array ret;
            size_t elems = size();
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(arg1.front().getStr()));
            }

            return ret;
        }
        else if (sMethod == "order")
            return order(arg1);

        MethodDefinition def = front().isMethod(sMethod, 1);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().call(sMethod, arg1);

            Array ret;
            size_t elems = std::max(size(), arg1.size());
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(sMethod, arg1.get(i)));
            }

            return ret;
        }

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
        if (size() == 1 && first().isArray())
            return first().getArray().call(sMethod, arg1, arg2);

        if (sMethod == "delegate" && arg1.getCommonType() == TYPE_STRING)
        {
            Array ret;
            size_t elems = size();
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(arg1.front().getStr(), arg2));
            }

            return ret;
        }

        MethodDefinition def = front().isMethod(sMethod, 2);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().call(sMethod, arg1, arg2);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size()});
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(sMethod, arg1.get(i), arg2.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \param arg3 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3) const
    {
        MethodDefinition def = front().isMethod(sMethod, 3);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().call(sMethod, arg1, arg2, arg3);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size(), arg3.size()});
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(sMethod, arg1.get(i), arg2.get(i), arg3.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \param arg3 const Array&
    /// \param arg4 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::call(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4) const
    {
        MethodDefinition def = front().isMethod(sMethod, 4);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().call(sMethod, arg1, arg2, arg3, arg4);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size(), arg3.size(), arg4.size()});
            ret.reserve(elems);

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).call(sMethod, arg1.get(i), arg2.get(i), arg3.get(i), arg4.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Determine, whether the passed string
    /// corresponds to an applying method.
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    MethodDefinition Array::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        const static MethodSet methods = {{"sel", -1}, {"rem", -1}, {"ins", -1}, {"ins", -2}};

        auto iter = methods.find(MethodDefinition(sMethod, argc));

        if (iter != methods.end())
            return *iter;

        return front().isApplyingMethod(sMethod, argc);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::apply(const std::string& sMethod)
    {
        if (isConst())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        if (front().isApplyingMethod(sMethod, 0))
        {
            Array ret;
            ret.reserve(size());
            ret.makeMutable();

            for (size_t i = 0; i < size(); i++)
            {
                ret.emplace_back(get(i).apply(sMethod));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::apply(const std::string& sMethod, const Array& arg1)
    {
        if (isConst())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        if (sMethod == "sel")
        {
            if (size() == 1 && first().isArray())
                return first().getArray().makeMutable().apply(sMethod, arg1);

            Array ret;
            size_t elems = arg1.size();
            ret.reserve(elems);
            ret.makeMutable();

            for (size_t i = 0; i < elems; i++)
            {
                int64_t n = arg1.get(i).getNum().asI64();

                if (n > 0 && (size_t)n <= size())
                    ret.emplace_back(new RefValue(&get(n-1)));
            }

            return ret;
        }
        else if (sMethod == "rem" && arg1.getCommonType() == TYPE_NUMERICAL)
            return deleteItems(arg1);
        else if (sMethod == "ins" && arg1.getCommonType() == TYPE_NUMERICAL)
            return insertItems(arg1);

        MethodDefinition def = front().isApplyingMethod(sMethod, 1);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().apply(sMethod, arg1);

            Array ret;
            size_t elems = std::max(size(), arg1.size());
            ret.reserve(elems);
            ret.makeMutable();

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).apply(sMethod, arg1.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::apply(const std::string& sMethod, const Array& arg1, const Array& arg2)
    {
        if (isConst())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        if (sMethod == "ins" && arg1.getCommonType() == TYPE_NUMERICAL)
            return insertItems(arg1, arg2);

        MethodDefinition def = front().isApplyingMethod(sMethod, 2);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().apply(sMethod, arg1, arg2);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size()});
            ret.reserve(elems);
            ret.makeMutable();

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).apply(sMethod, arg1.get(i), arg2.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \param arg3 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3)
    {
        if (isConst())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        MethodDefinition def = front().isApplyingMethod(sMethod, 3);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().apply(sMethod, arg1, arg2, arg3);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size(), arg3.size()});
            ret.reserve(elems);
            ret.makeMutable();

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).apply(sMethod, arg1.get(i), arg2.get(i), arg3.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const Array&
    /// \param arg2 const Array&
    /// \param arg3 const Array&
    /// \param arg4 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::apply(const std::string& sMethod, const Array& arg1, const Array& arg2, const Array& arg3, const Array& arg4)
    {
        if (isConst())
            throw ParserError(ecMETHOD_ERROR, sMethod);

        MethodDefinition def = front().isApplyingMethod(sMethod, 4);

        if (def)
        {
            if (size() == 1 && def.receiveArray())
                return front().apply(sMethod, arg1, arg2, arg3, arg4);

            Array ret;
            size_t elems = std::max({size(), arg1.size(), arg2.size(), arg3.size(), arg4.size()});
            ret.reserve(elems);
            ret.makeMutable();

            for (size_t i = 0; i < elems; i++)
            {
                ret.emplace_back(get(i).apply(sMethod, arg1.get(i), arg2.get(i), arg3.get(i), arg4.get(i)));
            }

            return ret;
        }

        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Get references to individual elements.
    ///
    /// \param idx const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::index(const Array& idx)
    {
        Array ret;
        size_t elems = idx.size();
        ret.reserve(std::min(size(), elems));
        ret.makeMutable();
        DataType common = getCommonType();

        for (size_t i = 0; i < elems; i++)
        {
            int64_t p = idx.get(i).getNum().asI64();

            if (p > 0 && p <= (int64_t)size())
                ret.emplace_back(new RefValue(&get(p-1)));
            else if (p > 0 && idx.m_commonType == TYPE_GENERATOR && idx.size() == UINT64_MAX)
                return ret;
            else if (p > 0)
                ret.emplace_back(Value(common));
            else
                throw ParserError(ecTYPE_MISMATCH_OOB);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Get constant references to individual
    /// elements.
    ///
    /// \param idx const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::index(const Array& idx) const
    {
        Array ret;
        size_t elems = idx.size();
        ret.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            int64_t p = idx.get(i).getNum().asI64();

            if (p > 0 && p <= (int64_t)size())
                ret.emplace_back(get(p-1));
            else if (p > 0)
                ret.emplace_back(Value(getCommonType()));
            else
                throw ParserError(ecTYPE_MISMATCH_OOB);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Insert the selected elements.
    ///
    /// \param idx const Array&
    /// \param vals const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::insertItems(const Array& idx, const Array& vals)
    {
        Array ret;
        size_t elems = idx.size();
        ret.resize(elems);
        DataType commonType = getCommonType();

        // Ordering the accessing index is reasonable
        Array order = idx.order();

        // Insert the elements
        for (int i = elems-1; i >= 0; i--)
        {
            int64_t item = order.get(i).getNum().asI64();
            int64_t p = idx.get(item-1).getNum().asI64();

            // Construct and insert the element
            if (p > 0 && p <= (int64_t)size())
            {
                if (vals.isDefault())
                    ret.get(item-1) = Value(commonType);
                else
                    ret.get(item-1) = vals.get(item-1);

                insert(begin()+p-1, ret.get(item-1));
            }
        }

        if (m_commonType != TYPE_CLUSTER)
            m_commonType = TYPE_VOID;

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Delete the selected elements.
    ///
    /// \param idx const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::deleteItems(const Array& idx)
    {
        Array ret;
        size_t elems = idx.size();
        ret.reserve(elems);

        // Ordering the accessing index is reasonable
        Array order = idx.order();

        // Move the elements to the returning array
        for (size_t i = 0; i < elems; i++)
        {
            int64_t p = idx.get(i).getNum().asI64();

            if (p > 0 && p <= (int64_t)size())
                ret.emplace_back(get(p-1).release());
        }

        // Erase the remaining stubs
        for (int i = elems-1; i >= 0; i--)
        {
            int64_t item = order.get(i).getNum().asI64();
            int64_t p = idx.get(item-1).getNum().asI64();

            if (p > 0 && p <= (int64_t)size())
                erase(begin()+p-1);
        }

        if (m_commonType != TYPE_CLUSTER)
            m_commonType = TYPE_VOID;

        // If only one element is remaining, unwrap it automatically
        if (count() == 1 && first().isArray() && !first().isRef())
        {
            // Assigning a child of this array is amazingly complex
            // Release the pointer first, move the contents and delete
            // it afterwards
            ArrValue* arr = static_cast<ArrValue*>(first().release());
            operator=(std::move(arr->get()));
            delete arr;
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Get the order of this array.
    ///
    /// \param opts const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array Array::order(const Array& opts) const
    {
        if (!size())
            return mu::Value(false);

        bool desc = false;
        bool ignoreCase = false;

        if (!opts.isDefault())
        {
            for (size_t i = 0; i < opts.size(); i++)
            {
                if (opts.get(i) == "desc")
                    desc = true;

                if (opts.get(i) == "ignorecase")
                    ignoreCase = true;
            }
        }

        mu::Array index;
        index.reserve(size());

        for (size_t i = 1; i <= size(); i++)
        {
            index.push_back(i);
        }

        auto sorter = [=, this](const mu::Value& v1, const mu::Value& v2)
            {
                int64_t p1 = desc ? v2.getNum().asI64()-1 : v1.getNum().asI64()-1;
                int64_t p2 = desc ? v1.getNum().asI64()-1 : v2.getNum().asI64()-1;

                if (!get(p1).isValid())
                    return false;

                if (ignoreCase && get(p1).isString() && get(p2).isString())
                    return toLowerCase(get(p1).getStr()) < toLowerCase(get(p2).getStr());

                return bool(get(p1) < get(p2)) || !get(p2).isValid();
            };
        std::sort(index.begin(), index.end(), sorter);

        return index;
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
        size_t elems = size();
        ret.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            ret.push_back(get(i).getStr());
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
        size_t elems = size();
        ret.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            ret.push_back(get(i).as_cmplx());
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
        size_t elems = size();
        ret.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
            ret.push_back(get(i).print());
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
        //if (size() == 1 && front().isRef() && front().isArray())
        //    return front().getArray().print(digits, chrs, trunc);

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            if (size() > 1)
                ret += get(i).printEmbedded(digits, chrs, trunc);
            else
                ret += get(i).print(digits, chrs, trunc);
        }

        if (size() > 1)
            return "{" + ret + "}";

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Print the contents as if they are
    /// embedded.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Array::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        if (isDefault())
#ifdef PARSERSTANDALONE
            return "default";
#else
            return "void";
#endif
        if (size() == 1 && first().isArray())
            return first().getArray().printEmbedded(digits, chrs, trunc);

        return "{" + printDims() + " " + getCommonTypeAsString() + "}";
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

        //if (size() == 1 && front().isRef() && front().isArray())
        //    return front().getArray().printVals(digits, chrs);

        std::string ret;

        for (size_t i = 0; i < size(); i++)
        {
            if (ret.length())
                ret += ", ";

            ret += get(i).printVal(digits, chrs);
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
        //if (size() == 1 && front().isRef() && front().isArray())
        //    return front().getArray().printDims();

        if (!size())
            return "0 x 0";

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
            std::string sVal = get(i).printVal();

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
        //if (size() == 1 && front().isRef() && front().isArray())
        //    return front().getArray().printOverview(digits, chrs, maxElems, alwaysBraces);

        // Return an empty brace pair, if no data is
        // available
        if (!size())
            return alwaysBraces ? "{}" : "void";

        // If only one value is available, omit the brace
        if (size() == 1 && !alwaysBraces)
            return print(digits, chrs, false);

        std::string sVector;

        if (m_commonType == TYPE_GENERATOR)
        {
            for (size_t i = 0; i < count(); i++)
            {
                if (sVector.size())
                sVector += ", ";

                sVector += operator[](i).print(digits, chrs < std::string::npos ? chrs/4 : std::string::npos, false);

                // Insert the ellipsis in the middle. The additional -1 is to
                // handle the zero-based indices
                if (i == maxElems / 2 - 1 && count() > maxElems)
                {
                    sVector += ", ...";
                    i = size()-maxElems / 2 - 1;
                }
            }
        }
        else
        {
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

        for (size_t i = 0; i < count(); i++)
        {
            bytes += operator[](i).getBytes();
        }

        return bytes;
    }


    /////////////////////////////////////////////////
    /// \brief Custom size function. Will default to
    /// the parent size function except when this
    /// array contains a reference to another array.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Array::size() const
    {
        size_t vectSize = count();

        if (vectSize == 1 && first().isRef() && first().isArray())
            return first().getArray().size();

        if (m_commonType == TYPE_GENERATOR)
        {
            size_t totalSize = 0;

            for (size_t i = 0; i < vectSize; i++)
            {
                if (operator[](i).isGenerator())
                {
                    size_t genSize = operator[](i).getGenerator().size();

                    if (genSize == UINT64_MAX)
                        return genSize;

                    totalSize += genSize;
                }
                else
                    totalSize++;
            }

            return totalSize;
        }

        return vectSize;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of elements stored in
    /// the array. Does not correspond to its size.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Array::count() const
    {
        return std::vector<Value>::size();
    }


    /////////////////////////////////////////////////
    /// \brief The the first element stored in the
    /// array. Does not correspond to calling front().
    ///
    /// \return Value&
    ///
    /////////////////////////////////////////////////
    Value& Array::first()
    {
        return std::vector<Value>::front();
    }


    /////////////////////////////////////////////////
    /// \brief The the first element stored in the
    /// array. Does not correspond to calling front().
    ///
    /// \return const Value&
    ///
    /////////////////////////////////////////////////
    const Value& Array::first() const
    {
        return std::vector<Value>::front();
    }


    /////////////////////////////////////////////////
    /// \brief Generate the i-th value and return a
    /// reference to its buffered value.
    ///
    /// \param i size_t
    /// \return Value&
    ///
    /////////////////////////////////////////////////
    Value& Array::getGenerated(size_t i)
    {
        if (i >= size())
            throw ParserError(ecTYPE_MISMATCH_OOB);

        size_t vectSize = count();

        for (size_t n = 0; n < vectSize; n++)
        {
            Value& val = operator[](n);

            if (val.isGenerator())
            {
                if (val.getGenerator().size() <= i)
                    i -= val.getGenerator().size();
                else
                {
                    m_buffer.push_back(val.getGenerator().at(i));

                    if (m_buffer.size() > 10)
                        m_buffer.pop_front();

                    return m_buffer.back();
                }
            }
            else if (i)
                i--;
            else if (!i)
                return val;
        }

        throw ParserError(ecTYPE_MISMATCH_OOB);
    }


    /////////////////////////////////////////////////
    /// \brief Generate the i-th value and return a
    /// reference to its buffered value.
    ///
    /// \param i size_t
    /// \return const Value&
    ///
    /////////////////////////////////////////////////
    const Value& Array::getGenerated(size_t i) const
    {
        if (i >= size())
            throw ParserError(ecTYPE_MISMATCH_OOB);

        size_t vectSize = count();

        for (size_t n = 0; n < vectSize; n++)
        {
            const Value& val = operator[](n);

            if (val.isGenerator())
            {
                if (val.getGenerator().size() <= i)
                    i -= val.getGenerator().size();
                else
                {
                    m_buffer.push_back(val.getGenerator().at(i));

                    if (m_buffer.size() > 10)
                        m_buffer.pop_front();

                    return m_buffer.back();
                }
            }
            else if (i)
                i--;
            else if (!i)
                return val;
        }

        throw ParserError(ecTYPE_MISMATCH_OOB);
    }


    /////////////////////////////////////////////////
    /// \brief Set a value at some place in the array.
    ///
    /// \param i size_t
    /// \param v const Value&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Array::set(size_t i, const Value& v)
    {
        DataType common = getCommonType();

        if (i >= size())
            resize(i+1, Value(NAN));

        get(i) = v;
        dereference();

        if (common != v.getType() && common != TYPE_CLUSTER)
            m_commonType = TYPE_VOID;
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
            if (get(i).getNum().asCF64() == 0.0)
                get(i).clear();
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
    /// \brief Remove all instances of RefValue in
    /// this array and replace them by clones of
    /// their referenced-to values.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Array::dereference() const
    {
        for (size_t i = 0; i < count(); i++)
        {
            Value& v = const_cast<Value&>(operator[](i));

            if (v.isRef())
                v.reset(v.getRef().get().clone());

            if (v.isArray())
                v.getArray().dereference();
        }
    }







    /////////////////////////////////////////////////
    /// \brief Construct an empty Variable.
    /////////////////////////////////////////////////
    Variable::Variable() : Array()
    {
        makeMutable();
    }


    /////////////////////////////////////////////////
    /// \brief Construct an empty Variable with a
    /// dedicated data type.
    ///
    /// \param type DataType
    ///
    /////////////////////////////////////////////////
    Variable::Variable(DataType type) : Array()
    {
        // Pre-initialize default variable types
        if (type == TYPE_NUMERICAL)
            emplace_back(0.0);
        else if (type == TYPE_STRING)
            emplace_back("");

        makeMutable();
        m_commonType = type;
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Variable from a Value.
    ///
    /// \param data const Value&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Value& data) : Array(data)
    {
        makeMutable();
        dereference();
    }


    /////////////////////////////////////////////////
    /// \brief Construct a Variable from an Array.
    ///
    /// \param data const Array&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Array& data) : Array(data)
    {
        makeMutable();
        dereference();
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param data const Variable&
    ///
    /////////////////////////////////////////////////
    Variable::Variable(const Variable& data) : Array(data)
    {
        makeMutable();
    }


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

        DataType common = getCommonType();

        if (common == TYPE_VOID
            || common == TYPE_CLUSTER
            || (common == TYPE_OBJECT && !size())
            || common == other.getType())
        {
            Array::operator=(Array(other));
            makeMutable();
            dereference();

            if (common == TYPE_CLUSTER)
                m_commonType = TYPE_CLUSTER;

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
        DataType common = getCommonType();

        if (common == TYPE_VOID
            || common == TYPE_CLUSTER
            || (common == TYPE_OBJECT && !size())
            || common == other.getCommonType())
        {
            Array::operator=(other);
            makeMutable();

            if (common == TYPE_CLUSTER)
                m_commonType = TYPE_CLUSTER;

            return *this;
        }

        throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Perform an indexed assign.
    ///
    /// \param idx const Array&
    /// \param vals const Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Variable::indexedAssign(const Array& idx, const Array& vals)
    {
        DataType common = getCommonType();

        if (common == TYPE_VOID
            || common == TYPE_CLUSTER
            || (common == TYPE_OBJECT && !size())
            || common == vals.getCommonType()
            || (common == TYPE_NUMERICAL && vals.getCommonType() == TYPE_GENERATOR))
        {
            vals.dereference();
            size_t elems = idx.size();
            size_t valSize = vals.size();

            // Assign the whole array directly
            if (elems == 1)
            {
                set(idx.get(0).getNum().asI64()-1, vals);
                return;
            }

            // Assign individual elements
            for (size_t i = 0; i < elems; i++)
            {
                // Ensure that we have enough values
                if (valSize > 1 && valSize <= i)
                    break;

                int64_t p = idx.get(i).getNum().asI64()-1;

                // A scalar value is only written until the last
                // already available value (or exceeds once for the first
                // element)
                if (elems == UINT64_MAX && valSize == 1 && i && p >= (int64_t)size())
                    break;

                set(p, vals.get(i));
            }
        }
        else
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
        other.dereference();
        assign(other);
        makeMutable();
    }




    /////////////////////////////////////////////////
    /// \brief Construct a VarArray from a single
    /// Variable.
    ///
    /// \param var Variable*
    ///
    /////////////////////////////////////////////////
    VarArray::VarArray(Variable* var) : VarArray()
    {
        if (var)
            push_back(var);
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
            if (!arr.get(i))
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
            if (arr.get(i))
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(arr.get(i)+v);
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(arr.get(i)-v);
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(arr.get(i)*v);
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(arr.get(i)/v);
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(v+arr.get(i));
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(v-arr.get(i));
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

        for (size_t i = 0; i < arr.size(); i++)
        {
            ret.push_back(v/arr.get(i));
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


