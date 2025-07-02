/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#include "muValueBase.hpp"
#include "muParserError.h"
#include "../utils/stringtools.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Addition operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator+(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator-() const
    {
        throw ParserError(ecNOT_IMPLEMENTED, "-" + getTypeAsString(m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator-(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator/(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator*(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Power operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::operator^(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator+=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " + " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator-=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " - " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator/=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " / " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator*=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " * " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& BaseValue::operator^=(const BaseValue& other)
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void BaseValue::flipSign()
    {
        throw ParserError(ecNOT_IMPLEMENTED, "-" + getTypeAsString(m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Power function.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::pow(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " ^ " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Convert this instance to a logical
    /// value. Has to be overridden, if comparisons
    /// shall work.
    ///
    /// \return BaseValue::operator
    ///
    /////////////////////////////////////////////////
    BaseValue::operator bool() const
    {
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Operator NOT.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator!() const
    {
        return !bool(*this);
    }


    /////////////////////////////////////////////////
    /// \brief Equal comparison operator. Has to be
    /// overridden, if comparisons shall work.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator==(const BaseValue& other) const
    {
        throw m_type == other.m_type;
    }


    /////////////////////////////////////////////////
    /// \brief Not-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator!=(const BaseValue& other) const
    {
        return !operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Less-than comparison operator. Has to
    /// be overridden, if comparisons shall work.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator<(const BaseValue& other) const
    {
        throw ParserError(ecNOT_IMPLEMENTED, getTypeAsString(m_type) + " < " + getTypeAsString(other.m_type));
    }


    /////////////////////////////////////////////////
    /// \brief Less-or-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator<=(const BaseValue& other) const
    {
        return operator<(other) || operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-than comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator>(const BaseValue& other) const
    {
        return !operator<(other) && !operator==(other);
    }


    /////////////////////////////////////////////////
    /// \brief Greater-or-equal comparison operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool BaseValue::operator>=(const BaseValue& other) const
    {
        return !operator<(other);
    }


    /////////////////////////////////////////////////
    /// \brief Method to detect, whether a method
    /// with the passed name is implemented in this
    /// instance. Can also be used to detect possible
    /// fields.
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition BaseValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod,
                               const BaseValue& arg1) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call an instance specific method with
    /// four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* BaseValue::call(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Method to detect, whether an applying
    /// (non-const) method with the passed name is
    /// implemented in this instance. Can also be
    /// used to detect possible fields.
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition BaseValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Apply an instance specific method with
    /// zero arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /// \note Must return this instance if
    /// implemented.
    /////////////////////////////////////////////////
    BaseValue* BaseValue::apply(const std::string& sMethod)
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply an instance specific method with
    /// one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /// \note Must return this instance if
    /// implemented.
    /////////////////////////////////////////////////
    BaseValue* BaseValue::apply(const std::string& sMethod,
                               const BaseValue& arg1)
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply an instance specific method with
    /// two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /// \note Must return this instance if
    /// implemented.
    /////////////////////////////////////////////////
    BaseValue* BaseValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2)
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply an instance specific method with
    /// three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /// \note Must return this instance if
    /// implemented.
    /////////////////////////////////////////////////
    BaseValue* BaseValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3)
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply an instance specific method with
    /// four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /// \note Must return this instance if
    /// implemented.
    /////////////////////////////////////////////////
    BaseValue* BaseValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4)
    {
        throw ParserError(ecMETHOD_ERROR, sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Print as if embedded into another
    /// structure. Defaults to the standard printing
    /// function.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string BaseValue::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        return print(digits, chrs, trunc);
    }


    //------------------------------------------------------------------------------


    /////////////////////////////////////////////////
    /// \brief Declare a method of this class
    ///
    /// \param def const MethodDefinition&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Object::declareMethod(const MethodDefinition& def)
    {
        m_methods.insert(def);
    }


    /////////////////////////////////////////////////
    /// \brief Declare an applying method of this
    /// class
    ///
    /// \param def const MethodDefinition&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Object::declareApplyingMethod(const MethodDefinition& def)
    {
        m_applyingMethods.insert(def);
    }


    /////////////////////////////////////////////////
    /// \brief Constructor for an abstract object.
    ///
    /// \param objectType const std::string&
    ///
    /////////////////////////////////////////////////
    Object::Object(const std::string& objectType) : m_objectType(objectType)
    {
        m_type = TYPE_OBJECT;
    }


    /////////////////////////////////////////////////
    /// \brief Copy an instance.
    ///
    /// \param other const Object&
    ///
    /////////////////////////////////////////////////
    Object::Object(const Object& other)
    {
        m_type = TYPE_OBJECT;
        m_objectType = other.m_objectType;
        m_methods = other.m_methods;
        m_applyingMethods = other.m_applyingMethods;
    }


    /////////////////////////////////////////////////
    /// \brief Return the object type of this object
    /// instance.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Object::getObjectType() const
    {
        return m_objectType;
    }


    /////////////////////////////////////////////////
    /// \brief Equality operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Object::operator==(const BaseValue& other) const
    {
        if (m_type == other.m_type)
            return m_objectType == static_cast<const Object&>(other).m_objectType;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Is the passed method a method of this
    /// class?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition Object::isMethod(const std::string& sMethod, size_t argc) const
    {
        auto iter = m_methods.find(MethodDefinition(sMethod, argc));

        if (iter != m_methods.end())
            return *iter;

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Is the passed method an applying
    /// method of this class?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition Object::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        auto iter = m_applyingMethods.find(MethodDefinition(sMethod, argc));

        if (iter != m_applyingMethods.end())
            return *iter;

        return MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Print this object as if it is embedded
    /// into an arry.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Object::printEmbedded(size_t digits, size_t chrs, bool trunc) const
    {
        return "{1 x 1 object." + m_objectType + "}";
    }


    //------------------------------------------------------------------------------


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    RefValue::RefValue(const BaseValue& other) : BaseValue()
    {
        m_type = TYPE_REFERENCE;

        if (other.m_type == TYPE_REFERENCE)
            m_ptr = static_cast<const RefValue&>(other).m_ptr;
    }


    /////////////////////////////////////////////////
    /// \brief Assign a value to the referanced value.
    ///
    /// \param other const BaseValue&
    /// \return RefValue&
    ///
    /////////////////////////////////////////////////
    RefValue& RefValue::operator=(const BaseValue& other)
    {
        if (other.m_type == TYPE_REFERENCE && !isNull() && !static_cast<const RefValue&>(other).isNull())
            m_ptr->reset(static_cast<const RefValue&>(other).get().clone());
        else if (other.m_type == TYPE_REFERENCE && isNull())
            m_ptr = static_cast<const RefValue&>(other).m_ptr;
        else
            m_ptr->reset(other.clone());

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Get a reference to the embedded value.
    ///
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::get()
    {
        if (isNull())
            throw ParserError(ecDEREFERENCE_VOID);

        return *m_ptr->get();
    }


    /////////////////////////////////////////////////
    /// \brief Get a const reference to the embedded
    /// value.
    ///
    /// \return const BaseValue&
    ///
    /////////////////////////////////////////////////
    const BaseValue& RefValue::get() const
    {
        if (isNull())
            throw ParserError(ecDEREFERENCE_VOID);

        return *m_ptr->get();
    }


    /////////////////////////////////////////////////
    /// \brief Addition operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator+(const BaseValue& other) const
    {
        return get() + other;
    }


    /////////////////////////////////////////////////
    /// \brief Unary minus operator.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator-() const
    {
        return -get();
    }


    /////////////////////////////////////////////////
    /// \brief Subtraction operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator-(const BaseValue& other) const
    {
        return get() - other;
    }


    /////////////////////////////////////////////////
    /// \brief Division operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator/(const BaseValue& other) const
    {
        return get() / other;
    }


    /////////////////////////////////////////////////
    /// \brief Multiplication operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator*(const BaseValue& other) const
    {
        return get() * other;
    }

    /////////////////////////////////////////////////
    /// \brief Power operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::operator^(const BaseValue& other) const
    {
        return get() ^ other;
    }


    /////////////////////////////////////////////////
    /// \brief Add-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::operator+=(const BaseValue& other)
    {
        get() += other;
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Subtract-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::operator-=(const BaseValue& other)
    {
        get() -= other;
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Divide-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::operator/=(const BaseValue& other)
    {
        get() /= other;
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Multiply-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::operator*=(const BaseValue& other)
    {
        get() *= other;
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Power-assign operator.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue&
    ///
    /////////////////////////////////////////////////
    BaseValue& RefValue::operator^=(const BaseValue& other)
    {
        get() ^= other;
        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Flip the sign bit as fast as possible.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void RefValue::flipSign()
    {
        if (!isNull())
            get().flipSign();
    }


    /////////////////////////////////////////////////
    /// \brief Power function.
    ///
    /// \param other const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::pow(const BaseValue& other) const
    {
        return !isNull() ? get().pow(other) : nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Return, whether the reference points
    /// to null.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool RefValue::isNull() const
    {
        return !m_ptr || !m_ptr->get();
    }


    /////////////////////////////////////////////////
    /// \brief Is this instance valid?
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool RefValue::isValid() const
    {
        return !isNull() && m_ptr->get()->isValid();
    }


    /////////////////////////////////////////////////
    /// \brief Cast to bool.
    ///
    /// \return RefValue::operator
    ///
    /////////////////////////////////////////////////
    RefValue::operator bool() const
    {
        return !isNull() && bool(get());
    }


    /////////////////////////////////////////////////
    /// \brief Operator-equal
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool RefValue::operator==(const BaseValue& other) const
    {
        return !isNull() ? get() == other : false;
    }


    /////////////////////////////////////////////////
    /// \brief Less-than operator.
    ///
    /// \param other const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool RefValue::operator<(const BaseValue& other) const
    {
        return get() < other;
    }


    /////////////////////////////////////////////////
    /// \brief Return the bytesize of the contained
    /// type.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t RefValue::getBytes() const
    {
        return !isNull() ? get().getBytes() : 0u;
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to a
    /// method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition RefValue::isMethod(const std::string& sMethod, size_t argc) const
    {
        return !isNull() ? get().isMethod(sMethod, argc) : MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::call(const std::string& sMethod) const
    {
        return get().call(sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::call(const std::string& sMethod,
                              const BaseValue& arg1) const
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return get().call(sMethod, static_cast<const RefValue&>(arg1).get());

        return get().call(sMethod, arg1);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return get().call(sMethod,
                               arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                               arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        return get().call(sMethod, arg1, arg2);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE)
            return get().call(sMethod,
                               arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                               arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                               arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3);

        return get().call(sMethod, arg1, arg2, arg3);
    }


    /////////////////////////////////////////////////
    /// \brief Call a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::call(const std::string& sMethod,
                              const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE || arg4.m_type == TYPE_REFERENCE)
            return get().call(sMethod,
                               arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                               arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                               arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3,
                               arg4.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg4).get() : arg4);

        return get().call(sMethod, arg1, arg2, arg3, arg4);
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to
    /// an applying method?
    ///
    /// \param sMethod const std::string&
    /// \param argc size_t
    /// \return MethodDefinition
    ///
    /////////////////////////////////////////////////
    MethodDefinition RefValue::isApplyingMethod(const std::string& sMethod, size_t argc) const
    {
        return !isNull() ? get().isApplyingMethod(sMethod, argc) : MethodDefinition();
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with no arguments.
    ///
    /// \param sMethod const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::apply(const std::string& sMethod)
    {
        return get().apply(sMethod);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with one argument.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::apply(const std::string& sMethod,
                               const BaseValue& arg1)
    {
        if (arg1.m_type == TYPE_REFERENCE)
            return get().apply(sMethod, static_cast<const RefValue&>(arg1).get());

        return get().apply(sMethod, arg1);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with two arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE)
            return get().apply(sMethod,
                                arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                                arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2);

        return get().apply(sMethod, arg1, arg2);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with three arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE)
            return get().apply(sMethod,
                                arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                                arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                                arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3);

        return get().apply(sMethod, arg1, arg2, arg3);
    }


    /////////////////////////////////////////////////
    /// \brief Apply a method with four arguments.
    ///
    /// \param sMethod const std::string&
    /// \param arg1 const BaseValue&
    /// \param arg2 const BaseValue&
    /// \param arg3 const BaseValue&
    /// \param arg4 const BaseValue&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* RefValue::apply(const std::string& sMethod,
                               const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4)
    {
        if (arg1.m_type == TYPE_REFERENCE || arg2.m_type == TYPE_REFERENCE || arg3.m_type == TYPE_REFERENCE || arg4.m_type == TYPE_REFERENCE)
            return get().apply(sMethod,
                                arg1.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg1).get() : arg1,
                                arg2.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg2).get() : arg2,
                                arg3.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg3).get() : arg3,
                                arg4.m_type == TYPE_REFERENCE ? static_cast<const RefValue&>(arg4).get() : arg4);

        return get().apply(sMethod, arg1, arg2, arg3, arg4);
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained type as string
    /// including additional quotation marks.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string RefValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        return !isNull() ? get().print(digits, chrs, trunc) : "void";
    }


    /////////////////////////////////////////////////
    /// \brief Print the contained type as string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string RefValue::printVal(size_t digits, size_t chrs) const
    {
        return !isNull() ? get().printVal(digits, chrs) : "void";
    }


    //------------------------------------------------------------------------------


    /////////////////////////////////////////////////
    /// \brief Create a generator from a BaseValue.
    ///
    /// \param other const BaseValue&
    ///
    /////////////////////////////////////////////////
    GeneratorValue::GeneratorValue(const BaseValue& other)
    {
        m_type = TYPE_GENERATOR;

        if (other.m_type == TYPE_GENERATOR)
        {
            const GeneratorValue& generator = static_cast<const GeneratorValue&>(other);
            m_firstVal = generator.m_firstVal;
            m_lastVal = generator.m_lastVal;
            m_step = generator.m_step;
            m_size = generator.m_size;
        }
        else if (other.m_type == TYPE_REFERENCE && static_cast<const RefValue&>(other).get().m_type == TYPE_GENERATOR)
        {
            const GeneratorValue& generator = static_cast<const GeneratorValue&>(static_cast<const RefValue&>(other).get());
            m_firstVal = generator.m_firstVal;
            m_lastVal = generator.m_lastVal;
            m_step = generator.m_step;
            m_size = generator.m_size;
        }
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);
    }


    /////////////////////////////////////////////////
    /// \brief Assign a BaseValue.
    ///
    /// \param other const BaseValue&
    /// \return GeneratorValue&
    ///
    /////////////////////////////////////////////////
    GeneratorValue& GeneratorValue::operator=(const BaseValue& other)
    {
        if (other.m_type == TYPE_GENERATOR)
        {
            const GeneratorValue& generator = static_cast<const GeneratorValue&>(other);
            m_firstVal = generator.m_firstVal;
            m_lastVal = generator.m_lastVal;
            m_step = generator.m_step;
            m_size = generator.m_size;
        }
        else if (other.m_type == TYPE_REFERENCE && static_cast<const RefValue&>(other).get().m_type == TYPE_GENERATOR)
            return operator=(static_cast<const RefValue&>(other).get());
        else
            throw ParserError(ecASSIGNED_TYPE_MISMATCH);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Format this Generator into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \param trunc bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string GeneratorValue::print(size_t digits, size_t chrs, bool trunc) const
    {
        if (!digits)
            digits = 3;

        return "{" + toString(m_firstVal, digits) + ":" + toString(m_step, digits) + ":" + toString(m_lastVal, digits) + "}";
    }


    /////////////////////////////////////////////////
    /// \brief Format this Generator into a string.
    ///
    /// \param digits size_t
    /// \param chrs size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string GeneratorValue::printVal(size_t digits, size_t chrs) const
    {
        if (!digits)
            digits = 3;

        return "{" + toString(m_firstVal, digits) + ":" + toString(m_step, digits) + ":" + toString(m_lastVal, digits) + "}";
    }
}

