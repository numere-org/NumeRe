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

#ifndef PARSERSTANDALONE
#include "../../kernel.hpp"
#endif // PARSERSTANDALONE

#include <json/json.h>
#include <fstream>
#include <filesystem>

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
    /// \param fileName std::string
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::importXml(std::string fileName)
    {
        tinyxml2::XMLDocument doc;

#ifndef PARSERSTANDALONE
        FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
        fileName = _fSys.ValidFileName(fileName, ".xml", false, true);

        if (!fileExists(fileName))
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "loadxml(\"" + fileName + "\")", fileName);
#endif

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
    /// \param fileName std::string
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::importJson(std::string fileName)
    {
#ifndef PARSERSTANDALONE
        FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
        fileName = _fSys.ValidFileName(fileName, ".json", false, true);

        if (!fileExists(fileName))
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "loadjson(\"" + fileName + "\")", fileName);
#endif

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


    //------------------------------------------------------------------------


    File::File()
    { }


    File::File(const File& other)
    {
        if (other.is_open())
            open(other.m_fileName, other.m_openMode);
    }


    File& File::operator=(const File& other)
    {
        if (other.is_open())
            open(other.m_fileName, other.m_openMode);

        return *this;
    }


    size_t File::set_read_pos(size_t p)
    {
        if (!is_open())
            return 0;

        m_stream.seekg(p-1, std::ios_base::beg);
        return p;
    }


    size_t File::get_read_pos() const
    {
        if (!is_open())
            return 0;

        return m_stream.tellg()+std::streamoff(1u);
    }


    size_t File::set_write_pos(size_t p)
    {
        if (!is_open())
            return 0;

        m_stream.seekp(p-1, std::ios_base::beg);
        return p;
    }


    size_t File::get_write_pos() const
    {
        if (!is_open())
            return 0;

        return m_stream.tellp()+std::streamoff(1u);
    }


    size_t File::length() const
    {
        if (!is_open())
            return 0;

        return std::filesystem::file_size(std::filesystem::path(m_fileName));
    }


    bool File::is_open() const
    {
        return m_stream.is_open() && m_stream.good();
    }


    bool File::open(const std::string& sFileName, const std::string& sOpenMode)
    {
        if (m_stream.is_open())
            m_stream.close();

#ifndef PARSERSTANDALONE
        FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
        m_fileName = _fSys.ValidFileName(sFileName, "", false, true);

        if (sOpenMode.find('r') != std::string::npos && !fileExists(m_fileName))
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "object.file.open(\"" + sFileName + "\")", sFileName);
#else
        m_fileName = sFileName;
#endif
        m_openMode = sOpenMode;

        std::ios_base::openmode mode;

        if (m_openMode.find('b') != std::string::npos)
            mode |= std::ios_base::binary;

        if (m_openMode.find("w+") != std::string::npos)
            mode |= std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
        else if (m_openMode.find('w') != std::string::npos)
            mode |= std::ios_base::out | std::ios_base::trunc;

        if (m_openMode.find("r+") != std::string::npos)
            mode |= std::ios_base::in | std::ios_base::out;
        else if (m_openMode.find('r') != std::string::npos)
            mode |= std::ios_base::in;

        if (m_openMode.find("a+") != std::string::npos)
            mode |= std::ios_base::in | std::ios_base::out | std::ios_base::app;
        else if (m_openMode.find('a') != std::string::npos)
            mode |= std::ios_base::out | std::ios_base::app;

        m_stream.open(m_fileName, mode);

        if (!m_stream)
        {
            close();
            return false;
        }

        return true;
    }


    bool File::close()
    {
        if (m_stream.is_open())
        {
            m_stream.close();
            m_fileName.clear();
            m_openMode.clear();
        }

        return true;
    }


    bool File::flush()
    {
        if (m_stream.is_open())
        {
            m_stream.flush();
            return true;
        }

        return false;
    }


    std::string File::getFileName() const
    {
        if (!is_open())
            return "";

        return m_fileName;
    }


    std::string File::getOpenMode() const
    {
        if (!is_open())
            return "";

        return m_openMode;
    }


    template<class T>
    static std::unique_ptr<T[]> readBytes(std::fstream& stream, size_t& n)
    {
        std::unique_ptr<T[]> data(new T[n]);
        stream.read((char*)data.get(), sizeof(T)*n);
        n = stream.gcount() / sizeof(T);

        return data;
    }


    template<class T>
    static ArrValue* convertToArray(const std::unique_ptr<T[]>& data, size_t n)
    {
        Array arr;

        for (size_t i = 0; i < n; i++)
        {
            arr.emplace_back(data[i]);
        }

        return new ArrValue(arr);
    }


    static BaseValue* readBinary(std::fstream& stream, const std::string& type, size_t n)
    {
        size_t numTypes = n;

        if (type == "value.i8")
        {
            auto data = readBytes<int8_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui8")
        {
            auto data = readBytes<uint8_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i16")
        {
            auto data = readBytes<int16_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui16")
        {
            auto data = readBytes<uint16_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i32")
        {
            auto data = readBytes<int32_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui32")
        {
            auto data = readBytes<uint32_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i64")
        {
            auto data = readBytes<int64_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui64")
        {
            auto data = readBytes<uint64_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.f32")
        {
            auto data = readBytes<float>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.f64")
        {
            auto data = readBytes<double>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.cf32")
        {
            auto data = readBytes<std::complex<float>>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.cf64")
        {
            auto data = readBytes<std::complex<double>>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "char" || type == "string")
        {
            auto data = readBytes<char>(stream, numTypes);
            std::string sStr;

            for (size_t i = 0; i < numTypes; i++)
            {
                sStr += data[i];
            }

            return new StrValue(sStr);
        }

        return nullptr;
    }


    template<class T>
    static std::unique_ptr<T[]> readSegments(std::fstream& stream, size_t& n)
    {
        std::unique_ptr<T[]> data(new T[n]);

        for (size_t i = 0; i < n; i++)
        {
            stream >> data[i];

            if (!stream)
            {
                n = i+1;
                return data;
            }
        }

        return data;
    }


    static BaseValue* readText(std::fstream& stream, const std::string& type, size_t n)
    {
        size_t numTypes = n;

        if (type == "value.i8")
        {
            auto data = readSegments<int8_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui8")
        {
            auto data = readSegments<uint8_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i16")
        {
            auto data = readSegments<int16_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui16")
        {
            auto data = readSegments<uint16_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i32")
        {
            auto data = readSegments<int32_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui32")
        {
            auto data = readSegments<uint32_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.i64")
        {
            auto data = readSegments<int64_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.ui64")
        {
            auto data = readSegments<uint64_t>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.f32")
        {
            auto data = readSegments<float>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.f64")
        {
            auto data = readSegments<double>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new NumValue(data[0]);
        }
        else if (type == "value.cf32")
        {
            auto data = readSegments<std::string>(stream, numTypes);

            if (numTypes > 1)
            {
                Array arr;

                for (size_t i = 0; i < numTypes; i++)
                {
                    arr.emplace_back(std::complex<float>(StrToCmplx(data[i])));
                }

                return new ArrValue(arr);
            }
            else
                return new NumValue(std::complex<float>(StrToCmplx(data[0])));
        }
        else if (type == "value.cf64")
        {
            auto data = readSegments<std::string>(stream, numTypes);

            if (numTypes > 1)
            {
                Array arr;

                for (size_t i = 0; i < numTypes; i++)
                {
                    arr.emplace_back(StrToCmplx(data[i]));
                }

                return new ArrValue(arr);
            }
            else
                return new NumValue(StrToCmplx(data[0]));
        }
        else if (type == "string")
        {
            auto data = readSegments<std::string>(stream, numTypes);

            if (numTypes > 1)
                return convertToArray(data, numTypes);
            else
                return new StrValue(data[0]);
        }
        else if (type == "char")
        {
            auto data = readSegments<char>(stream, numTypes);

            if (numTypes > 1)
            {
                Array arr;

                for (size_t i = 0; i < numTypes; i++)
                {
                    arr.emplace_back(std::string(1u, data[i]));
                }

                return new ArrValue(arr);
            }
            else
                return new StrValue(std::string(1u, data[0]));
        }

        return nullptr;
    }


    BaseValue* File::read(const std::string& type, size_t n)
    {
        if (!is_open() || m_openMode.find_first_of("r+") == std::string::npos)
            return nullptr;

        if (m_openMode.find("b") != std::string::npos)
            return readBinary(m_stream, type, n);

        return readText(m_stream, type, n);
    }


    std::string File::read_line()
    {
        if (!is_open() || m_openMode.find_first_of("r+b") == std::string::npos)
            return nullptr;

        std::string sLine;
        std::getline(m_stream, sLine);
        return sLine;
    }


    template<class T>
    static bool writeBinaryImpl(std::fstream& stream, const T& val)
    {
        stream.write((char*)&val, sizeof(T));
        return true;
    }

    template<>
    bool writeBinaryImpl(std::fstream& stream, const std::string& val)
    {
        stream.write(val.c_str(), val.length());
        return true;
    }


    static bool writeBinary(std::fstream& stream, const BaseValue& val)
    {
        if (val.m_type == TYPE_ARRAY)
        {
            const Array& arr = static_cast<const ArrValue&>(val).get();

            for (size_t i = 0; i < arr.size(); i++)
            {
                if (!writeBinary(stream, *arr.get(i).get()))
                    return false;
            }
        }
        else if (val.m_type == TYPE_CATEGORY)
        {
            const Category& cat = static_cast<const CatValue&>(val).get();
            NumValue value(cat.val);

            return writeBinaryImpl(stream, cat.name) && writeBinary(stream, value);
        }
        else if (val.m_type == TYPE_DICTSTRUCT)
        {
            const DictStruct& dict = static_cast<const DictStructValue&>(val).get();

            std::vector<std::string> fields = dict.getFields();

            for (const std::string& field : fields)
            {
                if (!writeBinaryImpl(stream, field) || !writeBinary(stream, *dict.read(field)))
                    return false;
            }

            return true;
        }
        else if (val.m_type == TYPE_STRING)
        {
            const std::string& sStr = static_cast<const StrValue&>(val).get();
            return writeBinaryImpl(stream, sStr);
        }
        else if (val.m_type == TYPE_NUMERICAL)
        {
            const Numerical& num = static_cast<const NumValue&>(val).get();

            switch (num.getType())
            {
                case I8:
                {
                    int8_t val = num.asI64();
                    return writeBinaryImpl(stream, val);
                }
                case LOGICAL:
                case UI8:
                {
                    uint8_t val = num.asUI64();
                    return writeBinaryImpl(stream, val);
                }
                case I16:
                {
                    int16_t val = num.asI64();
                    return writeBinaryImpl(stream, val);
                }
                case UI16:
                {
                    uint16_t val = num.asUI64();
                    return writeBinaryImpl(stream, val);
                }
                case I32:
                {
                    int32_t val = num.asI64();
                    return writeBinaryImpl(stream, val);
                }
                case UI32:
                {
                    uint32_t val = num.asUI64();
                    return writeBinaryImpl(stream, val);
                }
                case I64:
                    return writeBinaryImpl(stream, num.asI64());
                case UI64:
                    return writeBinaryImpl(stream, num.asUI64());
                case F32:
                {
                    float val = num.asF64();
                    return writeBinaryImpl(stream, val);
                }
                case F64:
                    return writeBinaryImpl(stream, num.asF64());
                case CF32:
                {
                    std::complex<float> val;
                    val.real(num.asCF64().real());
                    val.imag(num.asCF64().imag());
                    return writeBinaryImpl(stream, val);
                }
                case CF64:
                    return writeBinaryImpl(stream, num.asCF64());
            }
        }

        return false;
    }


    bool File::write(const BaseValue& val, const std::string& sSeparator)
    {
        if (!is_open() || m_openMode.find_first_of("aw+") == std::string::npos)
            return false;

        if (val.m_type == TYPE_REFERENCE)
            return write(static_cast<const RefValue&>(val).get());

        if (m_openMode.find("b") != std::string::npos)
            return writeBinary(m_stream, val);

        if (val.m_type == TYPE_ARRAY)
        {
            const Array& arr = static_cast<const ArrValue&>(val).get();

            for (size_t i = 0; i < arr.size(); i++)
            {
                m_stream << arr.get(i).printVal();

                if (i+1 < arr.size() && sSeparator.length())
                    m_stream << sSeparator;
            }
        }
        else
            m_stream << val.printVal(0, 0);

        return true;
    }
}

