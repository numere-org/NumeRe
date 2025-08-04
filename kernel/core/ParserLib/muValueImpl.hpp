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
        BaseValue* operator/(const BaseValue& other) const override;
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


    /////////////////////////////////////////////////
    /// \brief This class wraps a Path instance into
    /// a value.
    /////////////////////////////////////////////////
    class PathValue : public Object
    {
        private:
            Path m_val;

        public:
            PathValue();

            /////////////////////////////////////////////////
            /// \brief Create an instance from a Path.
            /////////////////////////////////////////////////
            PathValue(const Path& path) : PathValue()
            {
                m_val = path;
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            /////////////////////////////////////////////////
            PathValue(const PathValue& other) : PathValue()
            {
                m_val = other.m_val;
            }

            PathValue(PathValue&& other) = default;
            PathValue(const BaseValue& other);
            PathValue& operator=(const BaseValue& other) override;

            /////////////////////////////////////////////////
            /// \brief Assign a Path instance.
            ///
            /// \param val const Path&
            /// \return PathValue&
            ///
            /////////////////////////////////////////////////
            PathValue& operator=(const Path& val)
            {
                m_val = val;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign a PathValue instance.
            ///
            /// \param other const PathValue&
            /// \return PathValue&
            ///
            /////////////////////////////////////////////////
            PathValue& operator=(const PathValue& other)
            {
                m_val = other.m_val;
                return *this;
            }

            PathValue& operator=(PathValue&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Clone this instance.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new PathValue(*this);
            }

            /////////////////////////////////////////////////
            /// \brief Get a reference to the internal Path.
            ///
            /// \return Path&
            ///
            /////////////////////////////////////////////////
            Path& get()
            {
                return m_val;
            }

            /////////////////////////////////////////////////
            /// \brief Get a const reference to the internal
            /// Path.
            ///
            /// \return const Path&
            ///
            /////////////////////////////////////////////////
            const Path& get() const
            {
                return m_val;
            }

            bool isValid() const override;
            size_t getBytes() const override;

            BaseValue* operator+(const BaseValue& other) const override;
            BaseValue* operator/(const BaseValue& other) const override;
            BaseValue& operator+=(const BaseValue& other) override;
            BaseValue& operator/=(const BaseValue& other) override;
            bool operator==(const BaseValue& other) const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const override;
            BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class wraps a file instance into
    /// an object.
    /////////////////////////////////////////////////
    class FileValue : public Object
    {
        private:
            File m_val;

        public:
            FileValue();

            /////////////////////////////////////////////////
            /// \brief Create from a file instance.
            /////////////////////////////////////////////////
            FileValue(const File& file) : FileValue()
            {
                m_val = file;
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            /////////////////////////////////////////////////
            FileValue(const FileValue& other) : FileValue()
            {
                m_val = other.m_val;
            }

            FileValue(FileValue&& other) = default;
            FileValue(const BaseValue& other);
            FileValue& operator=(const BaseValue& other) override;

            /////////////////////////////////////////////////
            /// \brief Assign a file instance.
            ///
            /// \param val const File&
            /// \return FileValue&
            ///
            /////////////////////////////////////////////////
            FileValue& operator=(const File& val)
            {
                m_val = val;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign another FileValue instance.
            ///
            /// \param other const FileValue&
            /// \return FileValue&
            ///
            /////////////////////////////////////////////////
            FileValue& operator=(const FileValue& other)
            {
                m_val = other.m_val;
                return *this;
            }

            FileValue& operator=(FileValue&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Clone this instance.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new FileValue(*this);
            }

            /////////////////////////////////////////////////
            /// \brief Get a reference to the file instance.
            ///
            /// \return File&
            ///
            /////////////////////////////////////////////////
            File& get()
            {
                return m_val;
            }

            /////////////////////////////////////////////////
            /// \brief Get a const reference to the file
            /// instance.
            ///
            /// \return const File&
            ///
            /////////////////////////////////////////////////
            const File& get() const
            {
                return m_val;
            }

            bool isValid() const override;
            operator bool() const override;
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

    /////////////////////////////////////////////////
    /// \brief This class wraps a stack into an
    /// object.
    /////////////////////////////////////////////////
    class StackValue : public Object
    {
        private:
            Stack m_stack;

        public:
            StackValue();

            /////////////////////////////////////////////////
            /// \brief Create from a stack instance.
            /////////////////////////////////////////////////
            StackValue(const Stack& stck) : StackValue()
            {
                m_stack = stck;
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            /////////////////////////////////////////////////
            StackValue(const StackValue& other) : StackValue()
            {
                m_stack = other.m_stack;
            }

            StackValue(StackValue&& other) = default;
            StackValue(const BaseValue& other);
            StackValue& operator=(const BaseValue& other) override;

            /////////////////////////////////////////////////
            /// \brief Assign a stack instance.
            ///
            /// \param stck const Stack&
            /// \return StackValue&
            ///
            /////////////////////////////////////////////////
            StackValue& operator=(const Stack& stck)
            {
                m_stack = stck;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign another StackValue instance.
            ///
            /// \param other const StackValue&
            /// \return StackValue&
            ///
            /////////////////////////////////////////////////
            StackValue& operator=(const StackValue& other)
            {
                m_stack = other.m_stack;
                return *this;
            }

            StackValue& operator=(StackValue&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Clone this instance.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new StackValue(*this);
            }

            /////////////////////////////////////////////////
            /// \brief Get a reference to the Stack instance.
            ///
            /// \return Stack&
            ///
            /////////////////////////////////////////////////
            Stack& get()
            {
                return m_stack;
            }

            /////////////////////////////////////////////////
            /// \brief Get a const reference to the Stack
            /// instance.
            ///
            /// \return const Stack&
            ///
            /////////////////////////////////////////////////
            const Stack& get() const
            {
                return m_stack;
            }

            bool isValid() const override;
            operator bool() const override;
            size_t getBytes() const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };


    /////////////////////////////////////////////////
    /// \brief This class wraps a queue into an
    /// object.
    /////////////////////////////////////////////////
    class QueueValue : public Object
    {
        private:
            Queue m_queue;

        public:
            QueueValue();

            /////////////////////////////////////////////////
            /// \brief Create from a queue instance.
            /////////////////////////////////////////////////
            QueueValue(const Queue& que) : QueueValue()
            {
                m_queue = que;
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            /////////////////////////////////////////////////
            QueueValue(const QueueValue& other) : QueueValue()
            {
                m_queue = other.m_queue;
            }

            QueueValue(QueueValue&& other) = default;
            QueueValue(const BaseValue& other);
            QueueValue& operator=(const BaseValue& other) override;

            /////////////////////////////////////////////////
            /// \brief Assign a Queue instance.
            ///
            /// \param que const Queue&
            /// \return QueueValue&
            ///
            /////////////////////////////////////////////////
            QueueValue& operator=(const Queue& que)
            {
                m_queue = que;
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Assign another QueueValue instance.
            ///
            /// \param other const QueueValue&
            /// \return QueueValue&
            ///
            /////////////////////////////////////////////////
            QueueValue& operator=(const QueueValue& other)
            {
                m_queue = other.m_queue;
                return *this;
            }

            QueueValue& operator=(QueueValue&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Clone this instance.
            ///
            /// \return BaseValue*
            ///
            /////////////////////////////////////////////////
            BaseValue* clone() const override
            {
                return new QueueValue(*this);
            }

            /////////////////////////////////////////////////
            /// \brief Get a reference to the Queue instance.
            ///
            /// \return Queue&
            ///
            /////////////////////////////////////////////////
            Queue& get()
            {
                return m_queue;
            }

            /////////////////////////////////////////////////
            /// \brief Get a const reference to the Queue
            /// instance.
            ///
            /// \return const Queue&
            ///
            /////////////////////////////////////////////////
            const Queue& get() const
            {
                return m_queue;
            }

            bool isValid() const override;
            operator bool() const override;
            size_t getBytes() const override;

            BaseValue* call(const std::string& sMethod) const override;
            BaseValue* apply(const std::string& sMethod) override;
            BaseValue* apply(const std::string& sMethod, const BaseValue& arg1) override;

            std::string print(size_t digits, size_t chrs, bool trunc) const override;
            std::string printVal(size_t digits, size_t chrs) const override;
    };

}

#endif // MUVALUEIMPL_HPP

