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

#include <deque>

#include "muValueBase.hpp"
#include "muStructures.hpp"
#include "muCompositeStructures.hpp"

namespace mu
{
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

        MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* call(const std::string& sMethod) const override;
        BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const override;
        BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const override;

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

        MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* call(const std::string& sMethod) const override;

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

        MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* call(const std::string& sMethod) const override;
        BaseValue* call(const std::string& sMethod,
                        const BaseValue& arg1) const override;
        BaseValue* call(const std::string& sMethod,
                        const BaseValue& arg1, const BaseValue& arg2) const override;
        BaseValue* call(const std::string& sMethod,
                        const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) const override;
        BaseValue* call(const std::string& sMethod,
                        const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4) const override;

        MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* apply(const std::string& sMethod) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1, const BaseValue& arg2) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1, const BaseValue& arg2, const BaseValue& arg3, const BaseValue& arg4);

        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printEmbedded(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class wraps an DictStruct
    /// instance into a value.
    /////////////////////////////////////////////////
    class DictStructValue : public BaseValue
    {
        BASE_VALUE_DECL(DictStructValue, TYPE_DICTSTRUCT, DictStruct, m_val)

        bool isValid() const override;

        operator bool() const override;
        bool operator==(const BaseValue& other) const override;

        size_t getBytes() const override;

        MethodDefinition isMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* call(const std::string& sMethod) const override;
        BaseValue* call(const std::string& sMethod,
                        const BaseValue& arg1) const override;

        MethodDefinition isApplyingMethod(const std::string& sMethod, size_t argc) const override;
        BaseValue* apply(const std::string& sMethod) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1) override;
        BaseValue* apply(const std::string& sMethod,
                         const BaseValue& arg1, const BaseValue& arg2) override;


        std::string print(size_t digits, size_t chrs, bool trunc) const override;
        std::string printEmbedded(size_t digits, size_t chrs, bool trunc) const override;
        std::string printVal(size_t digits, size_t chrs) const override;
    };


    class FileValue : public Object
    {
        private:
            File m_val;

        public:
            FileValue();

            FileValue(const File& file) : FileValue()
            {
                m_val = file;
            }

            FileValue(const FileValue& other) : FileValue()
            {
                m_val = other.m_val;
            }

            FileValue(FileValue&& other) = default;
            FileValue(const BaseValue& other);
            FileValue& operator=(const BaseValue& other) override;

            FileValue& operator=(const File& val)
            {
                m_val = val;
                return *this;
            }

            FileValue& operator=(const FileValue& other)
            {
                m_val = other.m_val;
                return *this;
            }

            FileValue& operator=(FileValue&& other) = default;

            BaseValue* clone() const override
            {
                return new FileValue(*this);
            }

            File& get()
            {
                return m_val;
            }

            const File& get() const
            {
                return m_val;
            }

            bool isValid() const override;
            size_t getBytes() const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


    using Stack = std::deque<Value>;
    using Queue = std::deque<Value>;

    class StackValue : public Object
    {
        private:
            Stack m_stack;

        public:
            StackValue();

            StackValue(const Stack& stck) : StackValue()
            {
                m_stack = stck;
            }

            StackValue(const StackValue& other) : StackValue()
            {
                m_stack = other.m_stack;
            }

            StackValue(StackValue&& other) = default;
            StackValue(const BaseValue& other);
            StackValue& operator=(const BaseValue& other) override;

            StackValue& operator=(const Stack& stck)
            {
                m_stack = stck;
                return *this;
            }

            StackValue& operator=(const StackValue& other)
            {
                m_stack = other.m_stack;
                return *this;
            }

            StackValue& operator=(StackValue&& other) = default;

            BaseValue* clone() const override
            {
                return new StackValue(*this);
            }

            Stack& get()
            {
                return m_stack;
            }

            const Stack& get() const
            {
                return m_stack;
            }

            bool isValid() const override;
            size_t getBytes() const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


    class QueueValue : public Object
    {
        private:
            Queue m_queue;

        public:
            QueueValue();

            QueueValue(const Queue& que) : QueueValue()
            {
                m_queue = que;
            }

            QueueValue(const QueueValue& other) : QueueValue()
            {
                m_queue = other.m_queue;
            }

            QueueValue(QueueValue&& other) = default;
            QueueValue(const BaseValue& other);
            QueueValue& operator=(const BaseValue& other) override;

            QueueValue& operator=(const Queue& que)
            {
                m_queue = que;
                return *this;
            }

            QueueValue& operator=(const QueueValue& other)
            {
                m_queue = other.m_queue;
                return *this;
            }

            QueueValue& operator=(QueueValue&& other) = default;

            BaseValue* clone() const override
            {
                return new QueueValue(*this);
            }

            Queue& get()
            {
                return m_queue;
            }

            const Queue& get() const
            {
                return m_queue;
            }

            bool isValid() const override;
            size_t getBytes() const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };

}

#endif // MUVALUEIMPL_HPP

