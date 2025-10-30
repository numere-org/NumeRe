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
#include <sstream>

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Create an empty Path instance.
    /////////////////////////////////////////////////
    Path::Path()
    { }


    /////////////////////////////////////////////////
    /// \brief Create a Path instance from a
    /// string-encoded path and the corresponding
    /// separators (default to path separators).
    ///
    /// \param sPath const std::string&
    /// \param sSeparators const std::string&
    ///
    /////////////////////////////////////////////////
    Path::Path(const std::string& sPath, const std::string& sSeparators)
    {
        size_t p = 0;
        size_t sep = 0;

        // Jump over UNC network host markers
        if ((sSeparators == "/\\" || sSeparators == "\\/")
            && (sPath.starts_with("//") || sPath.starts_with("\\\\")))
        {
            sep = sPath.find_first_of(sSeparators, 2);
            m_pathElements.push_back(sPath.substr(0, sep));

            if (sep == std::string::npos)
            {
                clean();
                return;
            }

            p = sep+1;

            // Double path separators. Does not work with relative namespaces
            while (sSeparators.find(sPath[p]) != std::string::npos && p < sPath.length())
                p++;
        }

        while ((sep = sPath.find_first_of(sSeparators, p)) != std::string::npos)
        {
            m_pathElements.push_back(sPath.substr(p, sep-p));
            p = sep+1;

            // Double path separators. Does not work with relative namespaces
            while (sSeparators.find(sPath[p]) != std::string::npos && p < sPath.length())
                p++;
        }

        m_pathElements.push_back(sPath.substr(p));
        clean();
    }


    /////////////////////////////////////////////////
    /// \brief Create a Path instance from an already
    /// tokenized set of path elements.
    ///
    /// \param pathTokens const std::vector<std::string>&
    ///
    /////////////////////////////////////////////////
    Path::Path(const std::vector<std::string>& pathTokens)
    {
        m_pathElements = pathTokens;
        clean();
    }


    /////////////////////////////////////////////////
    /// \brief Compare two Path instances.
    ///
    /// \param other const Path&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Path::operator==(const Path& other) const
    {
        return m_pathElements == other.m_pathElements;
    }


    /////////////////////////////////////////////////
    /// \brief Append another path element to this
    /// instance.
    ///
    /// \param element const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Path::append(const std::string& element)
    {
        m_pathElements.push_back(element);
    }


    /////////////////////////////////////////////////
    /// \brief Append another Path instance to this
    /// instance.
    ///
    /// \param other const Path&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Path::append(const Path& other)
    {
        m_pathElements.insert(m_pathElements.end(), other.m_pathElements.begin(), other.m_pathElements.end());
    }


    /////////////////////////////////////////////////
    /// \brief Append some string to the current leaf
    /// element of this instance or create a new leaf
    /// element, if this Path instance is empty.
    ///
    /// \param element const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Path::cat(const std::string& element)
    {
        if (m_pathElements.size())
            m_pathElements.back() += element;
        else
            m_pathElements.push_back(element);
    }


    /////////////////////////////////////////////////
    /// \brief Return the current leaf value and
    /// remove it from the Path instance.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::pop()
    {
        std::string sPopped;

        if (m_pathElements.size())
        {
            sPopped = m_pathElements.back();
            m_pathElements.pop_back();
        }

        return sPopped;
    }


    /////////////////////////////////////////////////
    /// \brief Return the i-th path element.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::at(size_t i) const
    {
        if (i < m_pathElements.size())
            return m_pathElements[i];

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Remove the i-th element and return its
    /// value.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::remove(size_t i)
    {
        if (i >= m_pathElements.size())
            return "";

        std::string element = m_pathElements[i];
        m_pathElements.erase(m_pathElements.begin()+i);
        return element;
    }


    /////////////////////////////////////////////////
    /// \brief Remove a path segment and return it.
    ///
    /// \param i size_t
    /// \param depth size_t
    /// \return Path
    ///
    /////////////////////////////////////////////////
    Path Path::remove(size_t i, size_t depth)
    {
        if (i >= m_pathElements.size())
            return Path();

        Path path = getSegment(i, depth);
        m_pathElements.erase(m_pathElements.begin()+i, m_pathElements.begin()+i+depth);
        return path;
    }


    /////////////////////////////////////////////////
    /// \brief Write the string to the i-th path
    /// element.
    ///
    /// \param i size_t
    /// \param element const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Path::write(size_t i, const std::string& element)
    {
        if (i < m_pathElements.size())
        {
            m_pathElements[i] = element;
            return true;
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Insert a path element into the path.
    ///
    /// \param i size_t
    /// \param element const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Path::insert(size_t i, const std::string& element)
    {
        if (i > m_pathElements.size())
            return false;

        m_pathElements.insert(m_pathElements.begin()+i, element);
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Insert a complete path into this path.
    ///
    /// \param i size_t
    /// \param path const Path&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Path::insert(size_t i, const Path& path)
    {
        if (i > m_pathElements.size())
            return false;

        m_pathElements.insert(m_pathElements.begin()+i, path.m_pathElements.begin(), path.m_pathElements.end());
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Clean this Path instance by removing
    /// internal relative paths.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Path::clean()
    {
        if (m_pathElements.size() < 2)
            return;

        auto iter = m_pathElements.begin()+1;

        while (iter != m_pathElements.end())
        {
            if (*iter == ".")
                iter = m_pathElements.erase(iter);
            else if (*iter == ".." && *(iter-1) != "..")
                iter = m_pathElements.erase(iter-1, iter+1);
            else
                iter++;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Clear the object's contents and return
    /// true, if anything was cleared.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Path::clear()
    {
        if (m_pathElements.size())
        {
            m_pathElements.clear();
            return true;
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Return the current path depth (i.e.
    /// the number of path elements)
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Path::depth() const
    {
        return m_pathElements.size();
    }


    /////////////////////////////////////////////////
    /// \brief Access the i-th path element.
    ///
    /// \param i size_t
    /// \return std::string&
    ///
    /////////////////////////////////////////////////
    std::string& Path::operator[](size_t i)
    {
        return m_pathElements[i];
    }


    /////////////////////////////////////////////////
    /// \brief Const access to the i-th path element.
    ///
    /// \param i size_t
    /// \return const std::string&
    ///
    /////////////////////////////////////////////////
    const std::string& Path::operator[](size_t i) const
    {
        return m_pathElements[i];
    }


    /////////////////////////////////////////////////
    /// \brief Return the i-th path element as XPath
    /// element with an optional selector.
    ///
    /// \param i size_t
    /// \return xPathElement
    ///
    /////////////////////////////////////////////////
    xPathElement Path::getXPathElement(size_t i) const
    {
        std::string element = m_pathElements[i];
        int p = 0;

        if (element.find('[') != std::string::npos && element.ends_with(']'))
        {
            std::string sSelector = element.substr(element.find('['));
            sSelector.pop_back();
            sSelector.erase(0, 1);
            element.erase(element.find('['));

            if (sSelector.starts_with("last()"))
            {
                p = -1;
                sSelector.erase(0, 6);
            }

            if (sSelector.length())
                p += std::stoi(sSelector);

            // Because last()-x has to be equal to -x-1 and
            // x shall be equal to x-1, we only adapt positive
            // values
            if (p > 0)
                p--;
        }

        return xPathElement{element, p};
    }


    /////////////////////////////////////////////////
    /// \brief Get the first path element in this
    /// instance, which is likely to be the drive.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::root() const
    {
        if (m_pathElements.size())
            return m_pathElements.front();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Get the last path element in this
    /// instance, which is likely to be the filename.
    /// If the path ended with a separator, the leaf
    /// will return an empty string.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::leaf() const
    {
        if (m_pathElements.size())
            return m_pathElements.back();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Get the common trunk of the passed
    /// instance and this instance (i.e. the common
    /// part of the paths).
    ///
    /// \param other const Path&
    /// \return Path
    ///
    /////////////////////////////////////////////////
    Path Path::getTrunkPart(const Path& other) const
    {
        if (!m_pathElements.size() || !other.m_pathElements.size())
            return Path();

        auto iter = m_pathElements.begin();
        auto otherIter = other.m_pathElements.begin();

        while (iter != m_pathElements.end()
               && otherIter != other.m_pathElements.end()
               && *iter == *otherIter)
        {
            ++iter;
            ++otherIter;
        }

        Path path;
        path.m_pathElements.assign(m_pathElements.begin(), iter);
        return path;
    }


    /////////////////////////////////////////////////
    /// \brief Get the branch part of this instance
    /// with respect to the passed trunk. If the
    /// trunk is not found completely in this
    /// instance, only the common part is considered
    /// as trunk and the remaining elements belong to
    /// this branch.
    ///
    /// \param trunk const Path&
    /// \return Path
    ///
    /////////////////////////////////////////////////
    Path Path::getBranchPart(const Path& trunk) const
    {
        if (!m_pathElements.size() || !trunk.m_pathElements.size())
            return Path();

        auto iter = m_pathElements.begin();
        auto trunkIter = trunk.m_pathElements.begin();

        while (iter != m_pathElements.end()
               && trunkIter != trunk.m_pathElements.end()
               && *iter == *trunkIter)
        {
            ++iter;
            ++trunkIter;
        }

        Path path;
        path.m_pathElements.assign(iter, m_pathElements.end());
        return path;
    }


    /////////////////////////////////////////////////
    /// \brief Get the reverse branch part of this
    /// instance with respect to the passed other
    /// path, i.e. the unique trunk if the branches
    /// are equal. If the other path is not found
    /// completely in this instance, only the common
    /// part is considered as common branch and the
    /// remaining elements belong to the returned
    /// trunk branch.
    ///
    /// \param other const Path&
    /// \return Path
    ///
    /////////////////////////////////////////////////
    Path Path::getRevBranchPart(const Path& other) const
    {
        if (!m_pathElements.size() || !other.m_pathElements.size())
            return Path();

        auto iter = m_pathElements.rbegin();
        auto otherIter = other.m_pathElements.rbegin();

        while (iter != m_pathElements.rend()
               && otherIter != other.m_pathElements.rend()
               && *iter == *otherIter)
        {
            ++iter;
            ++otherIter;
        }

        Path path;
        path.m_pathElements.assign(m_pathElements.begin(), iter.base());
        return path;
    }


    /////////////////////////////////////////////////
    /// \brief Get a segment of the stored path.
    ///
    /// \param start size_t
    /// \param depth size_t
    /// \return Path
    ///
    /////////////////////////////////////////////////
    Path Path::getSegment(size_t start, size_t depth) const
    {
        Path path;

        if (start >= m_pathElements.size())
            return path;

        depth = std::min(m_pathElements.size()-start, depth);
        path.m_pathElements.assign(m_pathElements.begin()+start, m_pathElements.begin()+start+depth);
        return path;
    }


    /////////////////////////////////////////////////
    /// \brief Serialize this instance into a string.
    ///
    /// \param separator char
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Path::to_string(char separator) const
    {
        std::string sFormatted;

        for (const std::string& sElement : m_pathElements)
        {
            if (sFormatted.length())
                sFormatted += separator;

            sFormatted += sElement;
        }

        return sFormatted;
    }





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
    /// \brief Does the passed Path correspond to a
    /// field of this instance?
    ///
    /// \param xPath const Path&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::isField(const Path& xPath) const
    {
        if (!xPath.depth())
            throw std::out_of_range("Path is empty.");

        const BaseValue* element = nullptr;

        for (size_t i = 0; i < xPath.depth(); i++)
        {
            xPathElement pathElement = xPath.getXPathElement(i);

            if (!i)
                element = read(pathElement.name);
            else
            {
                if (element->m_type != TYPE_DICTSTRUCT || !static_cast<const DictStructValue*>(element)->get().isField(pathElement.name))
                    return false;

                element = static_cast<const DictStructValue*>(element)->get().read(pathElement.name);
            }

            if (!element)
                return false;

            // First and last element are identical in the scalar case and do not need an
            // array. All others do
            if (pathElement.needArray() && element->m_type != TYPE_ARRAY)
                return false;

            // If the element is an array, select the correct element
            if (element->m_type == TYPE_ARRAY)
            {
                const Array& arr = static_cast<const ArrValue*>(element)->get();

                if (pathElement.minSize() > arr.size())
                    return false;

                element = arr.get(pathElement.pos(arr.size())).get();

                if (!element && i+1 < xPath.depth())
                    return false;
            }
        }

        return true;
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
    /// \brief Read the value at the passed Path.
    ///
    /// \param xPath const Path&
    /// \return BaseValuePtr*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValuePtr* DictStruct::read(const Path& xPath)
    {
        if (!xPath.depth())
            throw std::out_of_range("Path is empty.");

        BaseValuePtr* element = nullptr;

        for (size_t i = 0; i < xPath.depth(); i++)
        {
            xPathElement pathElement = xPath.getXPathElement(i);

            if (!i)
                element = read(pathElement.name);
            else
            {
                if (element->get()->m_type != TYPE_DICTSTRUCT)
                    throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");

                element = static_cast<DictStructValue*>(element->get())->get().read(pathElement.name);
            }

            if (!element->get())
                throw std::out_of_range("Field " + xPath.getSegment(0, i+1).to_string('.') + " does not exist.");

            // First and last element are identical in the scalar case and do not need an
            // array. All others do
            if (pathElement.needArray() && element->get()->m_type != TYPE_ARRAY)
                throw std::runtime_error("Field " + xPath.getSegment(0, i+1).to_string('.') + " is not a cluster.");

            // If the element is an array, select the correct element
            if (element->get()->m_type == TYPE_ARRAY)
            {
                Array& arr = static_cast<ArrValue*>(element->get())->get();

                if (pathElement.minSize() > arr.size())
                    throw std::out_of_range("Field " + xPath.getSegment(0, i+1).to_string('.') + " has not a sufficient number of elements.");

                element = &arr.get(pathElement.pos(arr.size()));

                if (!element->get() && i+1 < xPath.depth())
                    throw std::out_of_range("Selected element " + xPath[i] + " at " + xPath.getSegment(0, i+1).to_string('.') + " is empty.");
            }
        }

        return element;
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
    /// \brief Read the value at the passed path.
    ///
    /// \param xPath const Path&
    /// \return const BaseValue*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    const BaseValue* DictStruct::read(const Path& xPath) const
    {
        if (!xPath.depth())
            throw std::out_of_range("Path is empty.");

        // To avoid code duplication, we're just casting the
        // const avay and use the non-const implementation
        return const_cast<DictStruct*>(this)->read(xPath)->get();
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
        BaseValue* val = value.dereferencedClone();
        m_fields[fieldName].reset(val);
        return &m_fields[fieldName];
    }


    /////////////////////////////////////////////////
    /// \brief Write a new value to the passed path.
    ///
    /// \param xPath const Path&
    /// \param value const BaseValue&
    /// \return BaseValuePtr*
    /// \note The returned pointer may only be
    /// borrowed.
    /////////////////////////////////////////////////
    BaseValuePtr* DictStruct::write(const Path& xPath, const BaseValue& value)
    {
        if (!xPath.depth())
            throw std::out_of_range("Path is empty.");

        BaseValuePtr* element = nullptr;

        for (size_t i = 0; i < xPath.depth(); i++)
        {
            xPathElement pathElement = xPath.getXPathElement(i);

            if (!i)
            {
                addKey(pathElement.name);
                element = read(pathElement.name);
            }
            else
            {
                DictStruct& dict = static_cast<DictStructValue*>(element->get())->get();
                dict.addKey(pathElement.name);
                element = dict.read(pathElement.name);
            }

            if (pathElement.index != 0)
            {
                // Create, if missing
                if (!element->get())
                    element->reset(new ArrValue());

                // Ensure correct typeness
                if (element->get() && element->get()->m_type != TYPE_DICTSTRUCT && element->get()->m_type != TYPE_ARRAY)
                    throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " has an invalid type.");
                else if (element->get()->m_type == TYPE_ARRAY)
                {
                    // If we have an array, we want to have the correct element as a dictstruct
                    Array& arr = static_cast<ArrValue*>(element->get())->get();

                    if (arr.size() < pathElement.minSize())
                        arr.resize(pathElement.minSize());

                    element = &arr.get(pathElement.pos(arr.size()));

                    if (xPath.depth() > i+1)
                    {
                        if (!element->get())
                            element->reset(new DictStructValue());
                        else if (element->get()->m_type != TYPE_DICTSTRUCT)
                            throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");
                    }
                    else
                    {
                        BaseValue* val = value.dereferencedClone();
                        element->reset(val);
                        return element;
                    }
                }
                else if (element->get()->m_type == TYPE_DICTSTRUCT)
                {
                    BaseValue* dict = element->release();
                    Array arr{Value(dict)};
                    arr.resize(pathElement.minSize());

                    if (xPath.depth() > i+1)
                        arr.get(pathElement.pos(arr.size())).reset(new DictStructValue());

                    element->reset(new ArrValue(arr));
                    element = &(static_cast<ArrValue*>(element->get())->get().get(pathElement.pos(arr.size())));
                }
            }
            else if (xPath.depth() > i+1)
            {
                // Create, if missing
                if (!element->get())
                    element->reset(new DictStructValue());

                // Ensure correct typeness
                if (element->get() && element->get()->m_type != TYPE_DICTSTRUCT && element->get()->m_type != TYPE_ARRAY)
                    throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " has an invalid type.");
                else if (element->get()->m_type == TYPE_ARRAY)
                {
                    // If we have an array, we want to have the first element as a dictstruct
                    element = &(static_cast<ArrValue*>(element->get())->get().get(0));

                    if (!element->get())
                        element->reset(new DictStructValue());
                    else if (element->get()->m_type != TYPE_DICTSTRUCT)
                        throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");
                }
            }
            else
            {
                BaseValue* val = value.dereferencedClone();
                element->reset(val);
                return element;
            }
        }

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Add a key with the selected name. Does
    /// not modify already values located with this
    /// key.
    ///
    /// \param fieldName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::addKey(const std::string& fieldName)
    {
        if (m_fields.find(fieldName) == m_fields.end())
        {
            m_fields[fieldName];
            return true;
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Add a key with the selected path. Does
    /// not modify already values located with this
    /// key.
    ///
    /// \param xPath const Path&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::addKey(const Path& xPath)
    {
        if (!xPath.depth())
            return false;

        BaseValuePtr* element = nullptr;

        for (size_t i = 0; i < xPath.depth(); i++)
        {
            xPathElement pathElement = xPath.getXPathElement(i);

            if (!i)
            {
                addKey(pathElement.name);
                element = read(pathElement.name);
            }
            else
            {
                DictStruct& dict = static_cast<DictStructValue*>(element->get())->get();
                dict.addKey(pathElement.name);
                element = dict.read(pathElement.name);
            }

            if (pathElement.index != 0)
            {
                // Create, if missing
                if (!element->get())
                    element->reset(new ArrValue());

                // Ensure correct typeness
                if (element->get() && element->get()->m_type != TYPE_DICTSTRUCT && element->get()->m_type != TYPE_ARRAY)
                    throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " has an invalid type.");
                else if (element->get()->m_type == TYPE_ARRAY)
                {
                    // If we have an array, we want to have the correct element as a dictstruct
                    Array& arr = static_cast<ArrValue*>(element->get())->get();

                    if (arr.size() < pathElement.minSize())
                        arr.resize(pathElement.minSize());

                    element = &arr.get(pathElement.pos(arr.size()));

                    if (xPath.depth() > i+1)
                    {
                        if (!element->get())
                            element->reset(new DictStructValue());
                        else if (element->get()->m_type != TYPE_DICTSTRUCT)
                            throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");
                    }
                }
                else if (element->get()->m_type == TYPE_DICTSTRUCT)
                {
                    BaseValue* dict = element->release();
                    Array arr{Value(dict)};
                    arr.resize(pathElement.minSize());

                    if (xPath.depth() > i+1)
                        arr.get(pathElement.pos(arr.size())).reset(new DictStructValue());

                    element->reset(new ArrValue(arr));
                    element = &(static_cast<ArrValue*>(element->get())->get().get(pathElement.pos(arr.size())));
                }
            }
            else if (xPath.depth() > i+1)
            {
                // Create, if missing
                if (!element->get())
                    element->reset(new DictStructValue());

                // Ensure correct typeness
                if (element->get() && element->get()->m_type != TYPE_DICTSTRUCT && element->get()->m_type != TYPE_ARRAY)
                    throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " has an invalid type.");
                else if (element->get()->m_type == TYPE_ARRAY)
                {
                    // If we have an array, we want to have the first element as a dictstruct
                    element = &(static_cast<ArrValue*>(element->get())->get().get(0));

                    if (!element->get())
                        element->reset(new DictStructValue());
                    else if (element->get()->m_type != TYPE_DICTSTRUCT)
                        throw std::runtime_error("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");
                }
            }
        }

        return true;
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
    /// \brief Remove a single element located at the
    /// selected path.
    ///
    /// \param xPath const Path&
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* DictStruct::remove(const Path& xPath)
    {
        if (!xPath.depth())
            throw std::out_of_range("Path is empty.");

        if (xPath.depth() == 1)
            return remove(xPath.root());

        BaseValuePtr* element = nullptr;

        for (size_t i = 0; i < xPath.depth()-1; i++)
        {
            xPathElement pathElement = xPath.getXPathElement(i);

            if (!i)
                element = read(pathElement.name);
            else
            {
                if (element->get()->m_type != TYPE_DICTSTRUCT)
                    throw std::out_of_range("Field " + xPath.getSegment(0, i).to_string('.') + " is not a dictstruct instance.");

                element = static_cast<DictStructValue*>(element->get())->get().read(pathElement.name);
            }

            if (!element->get())
                throw std::out_of_range("Field " + xPath.getSegment(0, i+1).to_string('.') + " does not exist.");

            // First and last element are identical in the scalar case and do not need an
            // array. All others do
            if (pathElement.needArray() && element->get()->m_type != TYPE_ARRAY)
                throw std::out_of_range("Field " + xPath.getSegment(0, i+1).to_string('.') + " is not a cluster.");

            // If the element is an array, select the correct element
            if (element->get()->m_type == TYPE_ARRAY)
            {
                Array& arr = static_cast<ArrValue*>(element->get())->get();

                if (pathElement.minSize() > arr.size())
                    throw std::out_of_range("Field " + xPath.getSegment(0, i+1).to_string('.') + " has not a sufficient number of elements.");

                element = &arr.get(pathElement.pos(arr.size()));

                if (!element->get() && i+1 < xPath.depth())
                    throw std::out_of_range("Selected element " + xPath[i] + " at " + xPath.getSegment(0, i+1).to_string('.') + " is empty.");
            }
        }

        if (element->get()->m_type != TYPE_DICTSTRUCT)
            throw std::out_of_range("Field " + xPath.to_string('.') + " is not a dictstruct instance.");

        return static_cast<DictStructValue*>(element->get())->get().remove(xPath.leaf());
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
    /// \param child const tinyxml2::XMLNode*
    /// \return Array
    ///
    /////////////////////////////////////////////////
    static Array importXmlChildren(const tinyxml2::XMLNode* child)
    {
        Array siblings;
//        siblings.makeMutable();

        while (child)
        {
            const tinyxml2::XMLElement* element = child->ToElement();
            const tinyxml2::XMLText* text = child->ToText();
            DictStructMap node;

            if (element)
            {
                const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
                DictStructMap attributes;
                node["name"].reset(new StrValue(element->Name()));

                while (attr)
                {
                    attributes[attr->Name()].reset(new StrValue(std::string(attr->Value())));
                    attr = attr->Next();
                }

                if (!attributes.empty())
                    node["attrs"].reset(new DictStructValue(attributes));
            }

            if (text)
                node["text"].reset(new StrValue(text->Value()));

            Array children = importXmlChildren(child->FirstChild());

            if (children.size() == 1)
            {
                DictStruct& dict = children.front().getDictStruct();

                // If it is a single text leaf, just copy it to the element
                if (dict.size() == 1 && dict.isField("text"))
                    node["text"].reset(dict.remove("text"));
                else
                    node["nodes"].reset(children.front().release());
            }
            else if (children.size() > 1)
                node["nodes"].reset(new ArrValue(children));

            if (node.size())
                siblings.emplace_back(DictStruct(node));

            child = child->NextSibling();
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
        Array nodes = importXmlChildren(doc.FirstChild());

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
                    dict[member].reset(new NumValue(Numerical(json[member].asInt64())));
                    break;
                case Json::uintValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asUInt64())));
                    break;
                case Json::realValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asDouble())));
                    break;
                case Json::stringValue:
                {
#ifndef PARSERSTANDALONE
                    if (isConvertible(json[member].asString(), CONVTYPE_DATE_TIME))
                        dict[member].reset(new NumValue(Numerical(StrToTime(json[member].asString()))));
                    else
#endif
                        dict[member].reset(new StrValue(json[member].asString()));

                    break;
                }
                case Json::booleanValue:
                    dict[member].reset(new NumValue(Numerical(json[member].asBool())));
                    break;
                case Json::arrayValue:
                    dict[member].reset(new ArrValue(importJsonArray(json[member])));
                    break;
                case Json::objectValue:
                    dict[member].reset(new DictStructValue(DictStruct(importJsonObject(json[member]))));
                    break;
                case Json::nullValue:
                    dict[member];
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
//        arr.makeMutable();

        for (Json::ArrayIndex i = 0; i < json.size(); i++)
        {
            switch (json[i].type())
            {
                case Json::intValue:
                    arr.emplace_back(json[i].asInt64());
                    break;
                case Json::uintValue:
                    arr.emplace_back(json[i].asUInt64());
                    break;
                case Json::realValue:
                    arr.emplace_back(json[i].asDouble());
                    break;
                case Json::stringValue:
                {
#ifndef PARSERSTANDALONE
                    if (isConvertible(json[i].asString(), CONVTYPE_DATE_TIME))
                        arr.emplace_back(StrToTime(json[i].asString()));
                    else
#endif
                        arr.emplace_back(json[i].asString());

                    break;
                }
                case Json::booleanValue:
                    arr.emplace_back(json[i].asBool());
                    break;
                case Json::arrayValue:
                    arr.emplace_back(importJsonArray(json[i]));
                    break;
                case Json::objectValue:
                    arr.emplace_back(DictStruct(importJsonObject(json[i])));
                    break;
                case Json::nullValue:
                    arr.emplace_back(Value());
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

    // Forward declaration due to interdependence of the two
    // encoder functions for JSON
    static std::string encodeJson(const DictStruct& dict);

    /////////////////////////////////////////////////
    /// \brief Encode an Array into a JSON-formatted
    /// string.
    ///
    /// \param arr const Array&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    static std::string encodeJson(const Array& arr)
    {
        std::string sJsonString;

        for (size_t i = 0; i < arr.size(); i++)
        {
            if (sJsonString.length())
                sJsonString += ", ";

            const Value& val = arr.get(i);

            if (val.isArray())
                sJsonString += encodeJson(val.getArray());
            else if (val.isDictStruct())
                sJsonString += encodeJson(val.getDictStruct());
            else if (val.isNumerical() && (val.getNum().getType() == DATETIME || val.getNum().getType() == DURATION))
                sJsonString += "\"" + val.print() + "\"";
            else
                sJsonString += val.print();
        }

        return "[" + sJsonString + "]";
    }


    /////////////////////////////////////////////////
    /// \brief Encode a DictStruct into a
    /// JSON-formatted string.
    ///
    /// \param dict const DictStruct&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    static std::string encodeJson(const DictStruct& dict)
    {
        std::vector<std::string> fields = dict.getFields();
        std::string sJsonString;

        for (const std::string& field : fields)
        {
            if (sJsonString.length())
                sJsonString += ", ";

            const BaseValue* val = dict.read(field);

            if (val->m_type == TYPE_ARRAY)
                sJsonString += "\"" + field + "\": " + encodeJson(static_cast<const ArrValue*>(val)->get());
            else if (val->m_type == TYPE_DICTSTRUCT)
                sJsonString += "\"" + field + "\": " + encodeJson(static_cast<const DictStructValue*>(val)->get());
            else if (val->m_type == TYPE_NUMERICAL
                     && (static_cast<const NumValue*>(val)->get().getType() == DATETIME
                         || static_cast<const NumValue*>(val)->get().getType() == DURATION))
                sJsonString += "\"" + field + "\": \"" + val->print(0, 0, false) + "\"";
            else
                sJsonString += "\"" + field + "\": " + val->print(0, 0, false);
        }

        return "{" + sJsonString + "}";
    }


    /////////////////////////////////////////////////
    /// \brief Encode the contents as a
    /// string-encoded json.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string DictStruct::encodeJson() const
    {
        if (!size())
            return "";

        std::vector<std::string> fields = getFields();

        if (fields.size() == 1 && fields.front() == "DOM")
        {
            // Ignore that
            const BaseValue* val = read("DOM");

            if (val->m_type == TYPE_ARRAY)
                return mu::encodeJson(static_cast<const ArrValue*>(val)->get());

            return mu::encodeJson(static_cast<const DictStructValue*>(val)->get());
        }

        // Create an object
        return mu::encodeJson(*this);
    }


    /////////////////////////////////////////////////
    /// \brief Decode the string-encoded JSON.
    ///
    /// \param jsonString const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::decodeJson(const std::string& jsonString)
    {
        std::istringstream jsonStream(jsonString);

        Json::Value root;
        jsonStream >> root;

        m_fields.clear();

        if (root.isArray())
            m_fields["DOM"].reset(new ArrValue(importJsonArray(root)));
        else
            m_fields["DOM"].reset(new DictStructValue(importJsonObject(root)));

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether this level of the
    /// DictStruct instance has XML structure.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool DictStruct::hasXmlStructure() const
    {
        if (m_fields.empty())
            return false;

        for (const auto& iter : m_fields)
        {
            if (iter.first != "name" && iter.first != "attrs" && iter.first != "nodes" && iter.first != "text")
                return false;
        }

        return true;
    }


    //------------------------------------------------------------------------

    /////////////////////////////////////////////////
    /// \brief Empty default constructor.
    /////////////////////////////////////////////////
    File::File()
    { }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param other const File&
    ///
    /////////////////////////////////////////////////
    File::File(const File& other)
    {
        if (other.is_open())
            open(other.m_fileName, other.m_openMode);
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor. Closes the
    /// moved-from file and re-opens the stream in
    /// this instance.
    ///
    /// \param other File&&
    ///
    /////////////////////////////////////////////////
    File::File(File&& other)
    {
        if (other.is_open())
        {
            other.m_stream.close();
            open(other.m_fileName, other.m_openMode);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Copy assignment operator.
    ///
    /// \param other const File&
    /// \return File&
    ///
    /////////////////////////////////////////////////
    File& File::operator=(const File& other)
    {
        close();

        if (other.is_open())
            open(other.m_fileName, other.m_openMode);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Move assignment operator.
    ///
    /// \param other File&&
    /// \return File&
    ///
    /////////////////////////////////////////////////
    File& File::operator=(File&& other)
    {
        close();

        if (other.is_open())
        {
            other.m_stream.close();
            open(other.m_fileName, other.m_openMode);
        }

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief Set the reading position of the
    /// underlying stream.
    ///
    /// \param p size_t
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t File::set_read_pos(size_t p)
    {
        if (!is_open())
            return 0;

        m_stream.seekg(p-1, std::ios_base::beg);
        return p;
    }


    /////////////////////////////////////////////////
    /// \brief Get the reading position of the
    /// underlying stream.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t File::get_read_pos() const
    {
        if (!is_open())
            return 0;

        return m_stream.tellg()+std::streamoff(1u);
    }


    /////////////////////////////////////////////////
    /// \brief Set the writing position of the
    /// underlying stream.
    ///
    /// \param p size_t
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t File::set_write_pos(size_t p)
    {
        if (!is_open())
            return 0;

        m_stream.seekp(p-1, std::ios_base::beg);
        return p;
    }


    /////////////////////////////////////////////////
    /// \brief Get the writing position of the
    /// underlying stream.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t File::get_write_pos() const
    {
        if (!is_open())
            return 0;

        return m_stream.tellp()+std::streamoff(1u);
    }


    /////////////////////////////////////////////////
    /// \brief Return the file length in characters
    /// or bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t File::length() const
    {
        if (!is_open())
            return 0;

        return std::filesystem::file_size(std::filesystem::path(m_fileName));
    }


    /////////////////////////////////////////////////
    /// \brief Determine, whether the underlying
    /// filestream is open.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool File::is_open() const
    {
        return m_stream.is_open() && m_stream.good();
    }


    /////////////////////////////////////////////////
    /// \brief Overload to make use of the Path class.
    ///
    /// \param path const Path&
    /// \param sOpenMode const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool File::open(const Path& path, const std::string& sOpenMode)
    {
        return open(path.to_string('/'), sOpenMode);
    }


    /////////////////////////////////////////////////
    /// \brief Open the underlying filestream for the
    /// selected file name using the open mode. The
    /// supported open modes resemble the C interface:
    /// "r", "r+", "w", "w+", "a", "a+" and "b" for
    /// binary mode.
    ///
    /// \param sFileName const std::string&
    /// \param sOpenMode const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
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

        int mode = 0;

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

        m_stream.open(m_fileName, (std::ios_base::openmode)mode);

        if (!m_stream)
        {
            close();
            return false;
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Close the stream if it is open.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Flush the stream's buffer and write to
    /// file.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool File::flush()
    {
        if (m_stream.is_open())
        {
            m_stream.flush();
            return true;
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Return the filename of the currently
    /// opened file.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string File::getFileName() const
    {
        if (!is_open())
            return "";

        return m_fileName;
    }


    /////////////////////////////////////////////////
    /// \brief Get the open mode of the currently
    /// opened file.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string File::getOpenMode() const
    {
        if (!is_open())
            return "";

        return m_openMode;
    }


    /////////////////////////////////////////////////
    /// \brief Template function to read an array of
    /// objects of type T from the stream in binary
    /// mode.
    ///
    /// \param stream std::fstream&
    /// \param n size_t&
    /// \return std::unique_ptr<T[]>
    ///
    /////////////////////////////////////////////////
    template<class T>
    static std::unique_ptr<T[]> readBytes(std::fstream& stream, size_t& n)
    {
        std::unique_ptr<T[]> data(new T[n]);
        stream.read((char*)data.get(), sizeof(T)*n);
        n = stream.gcount() / sizeof(T);

        return data;
    }


    /////////////////////////////////////////////////
    /// \brief Template function to convert an array
    /// of objects of type T into a mu::Array
    /// instance.
    ///
    /// \param data const std::unique_ptr<T[]>&
    /// \param n size_t
    /// \return ArrValue*
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Read something from the stream in
    /// binary mode. The object type is string
    /// encoded. Allows for reading of whole arrays
    /// at once.
    ///
    /// \param stream std::fstream&
    /// \param type const std::string&
    /// \param n size_t
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Template function to read an array of
    /// objects of type T from the stream in text
    /// mode.
    ///
    /// \param stream std::fstream&
    /// \param n size_t&
    /// \return std::unique_ptr<T[]>
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Read something from the stream in
    /// text mode. The object type is string encoded.
    /// Allows for reading of whole arrays at once.
    ///
    /// \param stream std::fstream&
    /// \param type const std::string&
    /// \param n size_t
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
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
#ifndef PARSERSTANDALONE
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
#endif
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


    /////////////////////////////////////////////////
    /// \brief Read n objects of the selected type
    /// from the file.
    ///
    /// \param type const std::string&
    /// \param n size_t
    /// \return BaseValue*
    ///
    /////////////////////////////////////////////////
    BaseValue* File::read(const std::string& type, size_t n)
    {
        if (!is_open() || m_openMode.find_first_of("r+") == std::string::npos)
            return nullptr;

        if (m_openMode.find("b") != std::string::npos)
            return readBinary(m_stream, type, n);

        return readText(m_stream, type, n);
    }


    /////////////////////////////////////////////////
    /// \brief Read a full line in text mode.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string File::read_line()
    {
        if (!is_open() || m_openMode.find_first_of("r+") == std::string::npos)
            return "";

        std::string sLine;
        std::getline(m_stream, sLine);
        return sLine;
    }


    /////////////////////////////////////////////////
    /// \brief Template function to write an object
    /// of type T in the file stream using binary
    /// mode.
    ///
    /// \param stream std::fstream&
    /// \param val const T&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    template<class T>
    static bool writeBinaryImpl(std::fstream& stream, const T& val)
    {
        stream.write((char*)&val, sizeof(T));
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Template specialisation for strings.
    ///
    /// \param stream std::fstream&
    /// \param val const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    template<>
    bool writeBinaryImpl(std::fstream& stream, const std::string& val)
    {
        stream.write(val.c_str(), val.length());
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Write the passed value to the stream
    /// in binary mode.
    ///
    /// \param stream std::fstream&
    /// \param val const BaseValue&
    /// \return bool
    ///
    /////////////////////////////////////////////////
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

            return true;
        }
        else if (val.m_type == TYPE_GENERATOR)
        {
            const GeneratorValue& gen = static_cast<const GeneratorValue&>(val);

            for (size_t i = 0; i < gen.size(); i++)
            {
                if (!writeBinary(stream, NumValue(gen.at(i))))
                    return false;
            }

            return true;
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
                case DURATION:
                case DATETIME:
                case AUTO:
                    break;
            }
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Write something to the stream. The
    /// separator is only used for cases, when the
    /// value contains an array and the file is
    /// opened in text mode.
    ///
    /// \param val const BaseValue&
    /// \param sSeparator const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
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
        else if (val.m_type == TYPE_GENERATOR)
        {
            const GeneratorValue& gen = static_cast<const GeneratorValue&>(val);

            for (size_t i = 0; i < gen.size(); i++)
            {
                m_stream << gen.at(i).printVal();

                if (i+1 < gen.size() && sSeparator.length())
                    m_stream << sSeparator;
            }
        }
        else
            m_stream << val.printVal(0, 0);

        return true;
    }
}

