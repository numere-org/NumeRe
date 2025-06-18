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

namespace mu
{
    class BaseValue;

    /////////////////////////////////////////////////
    /// \brief This class is a combination of a
    /// dictionary and a dynamic structure, hence the
    /// name "DictStruct". It can be used in both
    /// ways.
    /////////////////////////////////////////////////
    class DictStruct
    {
        private:
            std::map<std::string, std::unique_ptr<BaseValue>> m_fields;

        public:
            DictStruct();
            DictStruct(const DictStruct& other);
            DictStruct(DictStruct&& other);
            DictStruct(const std::map<std::string, std::unique_ptr<BaseValue>>& dictDef);

            DictStruct& operator=(const DictStruct& other);
            DictStruct& operator=(DictStruct&& other);
            DictStruct& operator=(const std::map<std::string, std::unique_ptr<BaseValue>>& dictDef);

            size_t size() const;
            bool isField(const std::string& fieldName) const;
            std::vector<std::string> getFields() const;
            BaseValue* read(const std::string& fieldName);
            const BaseValue* read(const std::string& fieldName) const;
            BaseValue* write(const std::string& fieldName, const BaseValue& value);
            BaseValue* remove(const std::string& fieldName);
            BaseValue* clear();
    };
}

#endif // MUCOMPOSITESTRUCTURES_HPP

