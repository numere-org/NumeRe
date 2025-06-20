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
#include "../../../externals/tinyxml2/tinyxml2.h"
#include <json/json.h>
#include <fstream>

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
    /// \param dictDef const DictStructMap&
    ///
    /////////////////////////////////////////////////
    DictStruct::DictStruct(const DictStructMap& dictDef)
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
    /// \param dictDef const DictStructMap&
    /// \return DictStruct&
    ///
    /////////////////////////////////////////////////
    DictStruct& DictStruct::operator=(const DictStructMap& dictDef)
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
    /// \return BaseValuePtr*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValuePtr* DictStruct::read(const std::string& fieldName)
    {
        auto iter = m_fields.find(fieldName);

        if (iter == m_fields.end())
            throw std::out_of_range("Field " + fieldName + " does not exist.");

        return &iter->second;
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
    /// \return BaseValuePtr*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValuePtr* DictStruct::write(const std::string& fieldName, const BaseValue& value)
    {
        BaseValue* val = value.clone();
        m_fields[fieldName].reset(val);
        return &m_fields[fieldName];
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
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t DictStruct::clear()
    {
        size_t count = m_fields.size();
        m_fields.clear();
        return count;
    }


    /////////////////////////////////////////////////
    /// \brief Import the current children of the
    /// parent node.
    ///
    /// \param child tinyxml2::XMLElement*
    /// \return Array
    ///
    /////////////////////////////////////////////////
    static Array importXmlChildren(tinyxml2::XMLElement* child)
    {
        Array siblings;
        siblings.makeMutable();

        while (child)
        {
            const tinyxml2::XMLAttribute* attr = child->FirstAttribute();
            DictStructMap attributes;
            DictStructMap node;
            node["name"].reset(new StrValue(child->Name()));

            while (attr)
            {
                attributes[attr->Name()].reset(new StrValue(std::string(attr->Value())));
                attr = attr->Next();
            }

            if (!attributes.empty())
                node["attrs"].reset(new DictStructValue(attributes));

            Array children = importXmlChildren(child->FirstChildElement());

            if (children.size() == 1)
                node["nodes"].reset(children.front().release());
            else if (children.size() > 1)
                node["nodes"].reset(new ArrValue(children));

            siblings.emplace_back(DictStruct(node));
            child = child->NextSiblingElement();
        }

        return siblings;
    }


    /////////////////////////////////////////////////
    /// \brief Import the contents of an XML file
    /// into this DictStruct instance.
    ///
    /// \param fileName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::importXml(const std::string& fileName)
    {
        tinyxml2::XMLDocument doc;

        if (doc.LoadFile(fileName.c_str()) != tinyxml2::XML_SUCCESS)
            return false;

        m_fields.clear();
        Array nodes = importXmlChildren(doc.FirstChildElement());

        if (nodes.size() == 1)
            m_fields["DOM"].reset(nodes.front().release());
        else
            m_fields["DOM"].reset(new ArrValue(nodes));

        return true;
    }


    // Forward declaration due to interdependence of the two
    // importer functions for JSON
    static Array importJsonArray(const Json::Value& json);


    /////////////////////////////////////////////////
    /// \brief Import an object structure from a JSON
    /// file.
    ///
    /// \param json const Json::Value&
    /// \return DictStructMap
    ///
    /////////////////////////////////////////////////
    static DictStructMap importJsonObject(const Json::Value& json)
    {
        Json::Value::Members members = json.getMemberNames();
        DictStructMap dict;

        for (const std::string& member : members)
        {
            switch (json[member].type())
            {
                case Json::intValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asInt())));
                    break;
                case Json::uintValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asUInt())));
                    break;
                case Json::realValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asDouble())));
                    break;
                case Json::stringValue:
                    dict[member].reset(new StrValue(json[member].asString()));
                    break;
                case Json::booleanValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asBool())));
                    break;
                case Json::arrayValue:
                    dict[member].reset(new ArrValue(importJsonArray(json[member])));
                    break;
                case Json::objectValue:
                    dict[member].reset(new DictStructValue(DictStruct(importJsonObject(json[member]))));
                    break;
            }
        }

        return dict;
    }


    /////////////////////////////////////////////////
    /// \brief Import an array structure from a JSON
    /// file.
    ///
    /// \param json const Json::Value&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    static Array importJsonArray(const Json::Value& json)
    {
        Array arr;
        arr.makeMutable();

        for (Json::ArrayIndex i = 0; i < json.size(); i++)
        {
            switch (json[i].type())
            {
                case Json::intValue:
                    arr.emplace_back(json[i].asInt());
                    break;
                case Json::uintValue:
                    arr.emplace_back(json[i].asUInt());
                    break;
                case Json::realValue:
                    arr.emplace_back(json[i].asDouble());
                    break;
                case Json::stringValue:
                    arr.emplace_back(json[i].asString());
                    break;
                case Json::booleanValue:
                    arr.emplace_back(json[i].asBool());
                    break;
                case Json::arrayValue:
                    arr.emplace_back(importJsonArray(json[i]));
                    break;
                case Json::objectValue:
                    arr.emplace_back(DictStruct(importJsonObject(json[i])));
                    break;
            }
        }

        return arr;
    }


    /////////////////////////////////////////////////
    /// \brief Import the contents of a JSON file
    /// into this DictStruct instance.
    ///
    /// \param fileName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::importJson(const std::string& fileName)
    {
        std::ifstream jsonFile(fileName);

        if (!jsonFile.good())
            return false;

        Json::Value root;
        jsonFile >> root;

        m_fields.clear();

        if (root.isArray())
            m_fields["DOM"].reset(new ArrValue(importJsonArray(root)));
        else
            m_fields["DOM"].reset(new DictStructValue(importJsonObject(root)));

        return true;
    }
}

