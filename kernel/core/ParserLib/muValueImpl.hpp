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

    class NumValue : public BaseValue
    {
        BASE_VALUE_DECL(NumValue, TYPE_NUMERICAL, Numerical, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        BaseValue& operator+=(const BaseValue& other) override;
        BaseValue& operator-=(const BaseValue& other) override;
        BaseValue& operator/=(const BaseValue& other) override;
        BaseValue& operator*=(const BaseValue& other) override;

        BaseValue* pow(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


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

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    class CatValue : public BaseValue
    {
        BASE_VALUE_DECL(CatValue, TYPE_CATEGORY, Category, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    class ArrValue : public BaseValue
    {
        BASE_VALUE_DECL(ArrValue, TYPE_ARRAY, Array, m_val)

        BaseValue* operator+(const BaseValue& other) const override;
        BaseValue* operator-() const override;
        BaseValue* operator-(const BaseValue& other) const override;
        BaseValue* operator/(const BaseValue& other) const override;
        BaseValue* operator*(const BaseValue& other) const override;

        BaseValue& operator+=(const BaseValue& other) override;
        BaseValue& operator-=(const BaseValue& other) override;
        BaseValue& operator/=(const BaseValue& other) override;
        BaseValue& operator*=(const BaseValue& other) override;

        BaseValue* pow(const BaseValue& other) const override;

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;
        bool operator<(const BaseValue& other) const override;

        size_t getBytes() const override;

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };
}

#endif // MUVALUEIMPL_HPP

