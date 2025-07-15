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
#include <memory>
#include <fstream>

namespace mu
{
    class BaseValue;
    using BaseValuePtr = std::unique_ptr<BaseValue>;
    using DictStructMap = std::map<std::string, BaseValuePtr>;

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
            std::vector<std::string> getFields() const;
            BaseValuePtr* read(const std::string& fieldName);
            const BaseValue* read(const std::string& fieldName) const;
            BaseValuePtr* write(const std::string& fieldName, const BaseValue& value);
            bool addKey(const std::string& fieldName);
            BaseValue* remove(const std::string& fieldName);
            size_t clear();
            bool importXml(std::string fileName);
            bool importJson(std::string fileName);
            std::string encodeJson() const;
            bool decodeJson(const std::string& jsonString);
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

