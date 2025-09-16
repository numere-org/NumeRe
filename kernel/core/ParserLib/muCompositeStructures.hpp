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

#ifndef MUCOMPOSITESTRUCTURES_HPP
#define MUCOMPOSITESTRUCTURES_HPP

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <fstream>

namespace mu
{
    class BaseValue;
    using BaseValuePtr = std::unique_ptr<BaseValue>;
    using DictStructMap = std::map<std::string, BaseValuePtr>;

    struct xPathElement
    {
        std::string name;
        int index;

        size_t minSize() const
        {
            return index < 0 ? -index : index+1;
        }

        size_t pos(size_t s) const
        {
            return index < 0 ? s+index : index;
        }

        bool needArray() const
        {
            return index > 0 || index < -1;
        }
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a generic path,
    /// which can either be a file path, or a network
    /// path, or a package path, or any other
    /// combination of strings and separators.
    /////////////////////////////////////////////////
    class Path
    {
        private:
            std::vector<std::string> m_pathElements;

        public:
            Path();
            Path(const std::string& sPath, const std::string& sSeparators = "/\\");
            Path(const std::vector<std::string>& pathTokens);
            Path(const Path& other) = default;
            Path(Path&& other) = default;

            Path& operator=(const Path& other) = default;
            Path& operator=(Path&& other) = default;

            /////////////////////////////////////////////////
            /// \brief Append another path element to the
            /// returned Path instance.
            ///
            /// \param element const std::string&
            /// \return Path
            ///
            /////////////////////////////////////////////////
            Path operator/(const std::string& element) const
            {
                return Path(*this) /= element;
            }

            /////////////////////////////////////////////////
            /// \brief Append a whole Path instance to the
            /// returned Path instance.
            ///
            /// \param other const Path&
            /// \return Path
            ///
            /////////////////////////////////////////////////
            Path operator/(const Path& other) const
            {
                return Path(*this) /= other;
            }

            /////////////////////////////////////////////////
            /// \brief Append some string to the current leaf
            /// element of the returned Path instance or
            /// create a new leaf element, if the Path
            /// instance is empty.
            ///
            /// \param element const std::string&
            /// \return Path
            ///
            /////////////////////////////////////////////////
            Path operator+(const std::string& element) const
            {
                return Path(*this) += element;
            }

            /////////////////////////////////////////////////
            /// \brief Append another path element to this
            /// instance.
            ///
            /// \param element const std::string&
            /// \return Path&
            ///
            /////////////////////////////////////////////////
            Path& operator/=(const std::string& element)
            {
                append(element);
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Append a whole Path instance to this
            /// instance.
            ///
            /// \param other const Path&
            /// \return Path&
            ///
            /////////////////////////////////////////////////
            Path& operator/=(const Path& other)
            {
                append(other);
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief Append some string to the current leaf
            /// element of this instance or create a new leaf
            /// element, if this Path instance is empty.
            ///
            /// \param element const std::string&
            /// \return Path&
            ///
            /////////////////////////////////////////////////
            Path& operator+=(const std::string& element)
            {
                cat(element);
                return *this;
            }

            bool operator==(const Path& other) const;

            void append(const std::string& element);
            void append(const Path& other);
            void cat(const std::string& element);
            std::string pop();
            std::string at(size_t i) const;
            std::string remove(size_t i);
            Path remove(size_t i, size_t depth);
            bool write(size_t i, const std::string& element);
            bool insert(size_t i, const std::string& element);
            bool insert(size_t i, const Path& path);

            void clean();
            bool clear();

            size_t depth() const;
            std::string& operator[](size_t i);
            const std::string& operator[](size_t i) const;
            xPathElement getXPathElement(size_t i) const;
            std::string root() const;
            std::string leaf() const;

            Path getTrunkPart(const Path& other) const;
            Path getBranchPart(const Path& trunk) const;
            Path getRevBranchPart(const Path& other) const;
            Path getSegment(size_t start = 0, size_t depth = std::string::npos) const;
            std::string to_string(char separator) const;
    };


    /////////////////////////////////////////////////
    /// \brief This class is a combination of a
    /// dictionary and a dynamic structure, hence the
    /// name "DictStruct". It can be used in both
    /// ways.
    /////////////////////////////////////////////////
    class DictStruct
    {
        private:
            DictStructMap m_fields;

        public:
            DictStruct();
            DictStruct(const DictStruct& other);
            DictStruct(DictStruct&& other);
            DictStruct(const DictStructMap& dictDef);

            DictStruct& operator=(const DictStruct& other);
            DictStruct& operator=(DictStruct&& other);
            DictStruct& operator=(const DictStructMap& dictDef);

            size_t size() const;
            bool isField(const std::string& fieldName) const;
            bool isField(const Path& xPath) const;
            std::vector<std::string> getFields() const;
            BaseValuePtr* read(const std::string& fieldName);
            BaseValuePtr* read(const Path& xPath);
            const BaseValue* read(const std::string& fieldName) const;
            const BaseValue* read(const Path& xPath) const;
            BaseValuePtr* write(const std::string& fieldName, const BaseValue& value);
            BaseValuePtr* write(const Path& xPath, const BaseValue& value);
            bool addKey(const std::string& fieldName);
            bool addKey(const Path& xPath);
            BaseValue* remove(const std::string& fieldName);
            BaseValue* remove(const Path& xPath);
            size_t clear();
            bool importXml(std::string fileName);
            bool importJson(std::string fileName);
            std::string encodeJson() const;
            bool decodeJson(const std::string& jsonString);
            bool hasXmlStructure() const;
    };


    /////////////////////////////////////////////////
    /// \brief This class allows for arbitrary file
    /// accesses using test and binary modes.
    /////////////////////////////////////////////////
    class File
    {
        private:
            mutable std::fstream m_stream;
            std::string m_fileName;
            std::string m_openMode;

        public:
            File();
            File(const File& other);
            File(File&& other);

            File& operator=(const File& other);
            File& operator=(File&& other);

            size_t set_read_pos(size_t p);
            size_t set_write_pos(size_t p);
            size_t get_read_pos() const;
            size_t get_write_pos() const;
            size_t length() const;
            bool is_open() const;
            bool open(const Path& path, const std::string& sOpenMode = "r");
            bool open(const std::string& sFileName, const std::string& sOpenMode = "r");
            bool close();
            bool flush();

            std::string getFileName() const;
            std::string getOpenMode() const;

            BaseValue* read(const std::string& type, size_t n = 1);
            std::string read_line();
            bool write(const BaseValue& val, const std::string& sSeparator = "");
    };
}

#endif // MUCOMPOSITESTRUCTURES_HPP

