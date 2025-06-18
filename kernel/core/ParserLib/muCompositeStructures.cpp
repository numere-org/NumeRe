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
#include "muValueImpl.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Construct an empty DictStruct.
    /////////////////////////////////////////////////
    DictStruct::DictStruct()
    { }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param other const DictStruct&
    ///
    /////////////////////////////////////////////////
    DictStruct::DictStruct(const DictStruct& other)
    {
        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor.
    ///
    /// \param other DictStruct&&
    ///
    /////////////////////////////////////////////////
    DictStruct::DictStruct(DictStruct&& other)
    {
        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first].reset(iter.second.release());
        }
    }


    /////////////////////////////////////////////////
    /// \brief Create a DictStruct instance from a
    /// prepared std::map.
    ///
    /// \param dictDef const std::map<std::string, std::unique_ptr<BaseValue>>&
    ///
    /////////////////////////////////////////////////
    DictStruct::DictStruct(const std::map<std::string, std::unique_ptr<BaseValue>>& dictDef)
    {
        for (auto& iter : dictDef)
        {
            m_fields[iter.first];

            if (iter.second)
                m_fields[iter.first].reset(iter.second->clone());
        }
    }


    /////////////////////////////////////////////////
    /// \brief Assign another DictStruct instance.
    ///
    /// \param other const DictStruct&
    /// \return DictStruct&
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Move-assign another DictStruct
    /// instance.
    ///
    /// \param other DictStruct&&
    /// \return DictStruct&
    ///
    /////////////////////////////////////////////////
    DictStruct& DictStruct::operator=(DictStruct&& other)
    {
        m_fields.clear();

        for (auto& iter : other.m_fields)
        {
            m_fields[iter.first].reset(iter.second.release());
        }

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Assign a prepared std::map.
    ///
    /// \param dictDef const std::map<std::string,std::unique_ptr<BaseValue>>&
    /// \return DictStruct&
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Return the number of fields stored in
    /// this structure.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t DictStruct::size() const
    {
        return m_fields.size();
    }


    /////////////////////////////////////////////////
    /// \brief Does the passed string correspond to a
    /// field of this instance?
    ///
    /// \param fieldName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::isField(const std::string& fieldName) const
    {
        return m_fields.find(fieldName) != m_fields.end();
    }


    /////////////////////////////////////////////////
    /// \brief Return a vector of all field names.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Read the value at the passed field
    /// name.
    ///
    /// \param fieldName const std::string&
    /// \return BaseValue*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValue* DictStruct::read(const std::string& fieldName)
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        return iter->second.get();
    }


    /////////////////////////////////////////////////
    /// \brief Read the value at the passed field
    /// name.
    ///
    /// \param fieldName const std::string&
    /// \return const BaseValue*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    const BaseValue* DictStruct::read(const std::string& fieldName) const
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        return iter->second.get();
    }


    /////////////////////////////////////////////////
    /// \brief Write a new value to the passed field.
    ///
    /// \param fieldName const std::string&
    /// \param value const BaseValue&
    /// \return BaseValue*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValue* DictStruct::write(const std::string& fieldName, const BaseValue& value)
    {
        BaseValue* val = value.clone();
        m_fields[fieldName].reset(val);
        return val;
    }


    /////////////////////////////////////////////////
    /// \brief Remove a single element.
    ///
    /// \param fieldName const std::string&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStruct::remove(const std::string& fieldName)
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        BaseValue* val = iter->second.release();
        m_fields.erase(iter);
        return val;
    }

    /////////////////////////////////////////////////
    /// \brief Remove all elements.
    ///
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStruct::clear()
    {
        size_t count = m_fields.size();
        m_fields.clear();
        return new NumValue(Numerical(count));
    }
}

