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

#ifndef MUVALUEIMPL_HPP
#define MUVALUEIMPL_HPP

#include "muValueBase.hpp"
#include "muStructures.hpp"
#include "muCompositeStructures.hpp"

namespace mu
{
    /*
    class VoidValue : public BaseValue
    {
        public:
        VoidValue();
        VoidValue(const VoidValue& other);
        BaseValue& operator=(const BaseValue& other) override;
        BaseValue& operator=(const VoidValue& other) override;
        BaseValue* clone() const override;

        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };
    */

    /////////////////////////////////////////////////
    /// \brief This implements the "neutral operand"
    /// i.e. the "do-nothing" value, similar to a
    /// neutral value in addition and multiplication.
    /////////////////////////////////////////////////
    class NeutralValue : public BaseValue
    {
        public:
        NeutralValue();
        NeutralValue(const NeutralValue& other);
        BaseValue& operator=(const BaseValue& other) override;
        BaseValue& operator=(const NeutralValue& other);
        BaseValue* clone() const override;

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class is a numerical value.
    /////////////////////////////////////////////////
    class NumValue : public BaseValue
    {
        BASE_VALUE_DECL(NumValue, TYPE_NUMERICAL, Numerical, m_val)

        NumValue(double val, bool makeInvalid);

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;
        BaseValue* operator^(const BaseValue& other) const override;

        BaseValue& operator+=(const BaseValue& other) override;
        BaseValue& operator-=(const BaseValue& other) override;
        BaseValue& operator/=(const BaseValue& other) override;
        BaseValue& operator*=(const BaseValue& other) override;
        BaseValue& operator^=(const BaseValue& other) override;

        void flipSign() override;

        BaseValue* pow(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class is a string value.
    /////////////////////////////////////////////////
    class StrValue : public BaseValue
    {
        BASE_VALUE_DECL(StrValue, TYPE_STRING, std::string, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        BaseValue& operator+=(const BaseValue& other) override;
        BaseValue& operator*=(const BaseValue& other) override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        virtual bool isMethod(const std::string& sMethod) const override;
        virtual BaseValue* call(const std::string& sMethod) const override;
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const override;
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class is a categorical value.
    /////////////////////////////////////////////////
    class CatValue : public BaseValue
    {
        BASE_VALUE_DECL(CatValue, TYPE_CATEGORY, Category, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        void flipSign() override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class wraps an array into a
    /// value.
    /////////////////////////////////////////////////
    class ArrValue : public BaseValue
    {
        BASE_VALUE_DECL(ArrValue, TYPE_ARRAY, Array, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;
        BaseValue* operator^(const BaseValue& other) const override;

        BaseValue& operator+=(const BaseValue& other) override;
        BaseValue& operator-=(const BaseValue& other) override;
        BaseValue& operator/=(const BaseValue& other) override;
        BaseValue& operator*=(const BaseValue& other) override;
        BaseValue& operator^=(const BaseValue& other) override;

        void flipSign() override;

        BaseValue* pow(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        virtual bool isMethod(const std::string& sMethod) const override;
        virtual BaseValue* call(const std::string& sMethod) const override;
        virtual BaseValue* call(const std::string& sMethod,
                                const BaseValue& arg1) const override;
        virtual BaseValue* call(const std::string& sMethod,
                                const BaseValue& arg1, const BaseValue& arg2) const override;
        virtual BaseValue* call(const std::string& sMethod,
                                const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const override;
        virtual BaseValue* call(const std::string& sMethod,
                                const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const override;

        virtual bool isApplyingMethod(const std::string& sMethod) const override;
        virtual BaseValue* apply(const std::string& sMethod) override;
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1) override;
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1, const BaseValue& arg2) override;
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) override;
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4);

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    class DictStructValue : public BaseValue
    {
        BASE_VALUE_DECL(DictStructValue, TYPE_DICTSTRUCT, DictStruct, m_val)

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;

        size_t getBytes() const override;

        virtual bool isMethod(const std::string& sMethod) const override;
        virtual BaseValue* call(const std::string& sMethod) const override; // .FIELD, .fields
        virtual BaseValue* call(const std::string& sMethod,
                                const BaseValue& arg1) const override; // .get("FIELD")

        virtual bool isApplyingMethod(const std::string& sMethod) const override;
        virtual BaseValue* apply(const std::string& sMethod) override; // .FIELD
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1) override;  // .FIELD(val)
        virtual BaseValue* apply(const std::string& sMethod,
                                 const BaseValue& arg1, const BaseValue& arg2) override; // .wrt("FIELD", val)


        virtual std::string print(size_t digits, size_t chrs, bool trunc) const override;
        virtual std::string printEmbedded(size_t digits, size_t chrs, bool trunc) const override;
        virtual std::string printVal(size_t digits, size_t chrs) const override;
    };

}

#endif // MUVALUEIMPL_HPP

