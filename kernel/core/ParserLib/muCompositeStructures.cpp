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


#include "muCompositeStructures.hpp"
#include "muValueBase.hpp"

namespace mu
{
    DictStruct::DictStruct()
    { }

    DictStruct::DictStruct(const DictStruct& other)
    {
        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }
    }

    DictStruct::DictStruct(DictStruct&& other)
    {
        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first].reset(iter.second.release());
        }
    }

    DictStruct::DictStruct(const std::map<std::string, std::unique_ptr<BaseValue>>& dictDef)
    {
        for (auto& iter : dictDef)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }
    }

    DictStruct& DictStruct::operator=(const DictStruct& other)
    {
        m_fields.clear();

        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }

        return *this;
    }

    DictStruct& DictStruct::operator=(DictStruct&& other)
    {
        m_fields.clear();

        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first].reset(iter.second.release());
        }

        return *this;
    }

    DictStruct& DictStruct::operator=(const std::map<std::string, std::unique_ptr<BaseValue>>& dictDef)
    {
        m_fields.clear();

        for (auto& iter : dictDef)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }

        return *this;
    }

    bool DictStruct::isField(const std::string& fieldName) const
    {
        return m_fields.find(fieldName) != m_fields.end();
    }

    std::vector<std::string> DictStruct::getFields() const
    {
        std::vector<std::string> fields;
        fields.reserve(m_fields.size());

        for (const auto& iter : m_fields)
        {
            fields.push_back(iter.first);
        }

        return fields;
    }

    BaseValue* DictStruct::read(const std::string& fieldName)
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        return iter->second.get();
    }

    const BaseValue* DictStruct::read(const std::string& fieldName) const
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        return iter->second.get();
    }

    BaseValue* DictStruct::write(const std::string& fieldName, const BaseValue& value)
    {
        BaseValue* val = value.clone();
        m_fields[fieldName].reset(val);
        return val;
    }
}

