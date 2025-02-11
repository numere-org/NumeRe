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

#include "recentfilemanager.hpp"
#include "../externals/tinyxml2/tinyxml2.h"
#include <fstream>


/////////////////////////////////////////////////
/// \brief Construct a RecentFile instance.
///
/// \param newFile const wxString&
///
/////////////////////////////////////////////////
RecentFile::RecentFile(const wxString& newFile) : name(newFile), opened(_time64(nullptr))
{
    name.Replace("\\", "/");
}


/////////////////////////////////////////////////
/// \brief Equality operator.
///
/// \param other const RecentFile&
/// \return bool
///
/////////////////////////////////////////////////
bool RecentFile::operator==(const RecentFile& other) const
{
    return name == other.name;
}





/////////////////////////////////////////////////
/// \brief Push a new opened file to the list.
///
/// \param newFile const wxString&
/// \return void
///
/////////////////////////////////////////////////
void RecentFilesManager::push(const wxString& newFile)
{
    RecentFile file(newFile);
    auto iter = std::find(m_fileList.begin(), m_fileList.end(), file);

    if (iter != m_fileList.end())
        m_fileList.erase(iter);

    m_fileList.push_back(file);

    if (m_fileList.size() > 20)
        m_fileList.erase(m_fileList.begin());
}


/////////////////////////////////////////////////
/// \brief Return the size of the stored
/// elements.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t RecentFilesManager::size() const
{
    return m_fileList.size();
}


/////////////////////////////////////////////////
/// \brief Access the i-th element.
///
/// \param i size_t
/// \return const RecentFile&
///
/////////////////////////////////////////////////
const RecentFile& RecentFilesManager::operator[](size_t i) const
{
    return m_fileList[i];
}


/////////////////////////////////////////////////
/// \brief Import the list from the indicated
/// file.
///
/// \param fileName const wxString&
/// \return void
///
/////////////////////////////////////////////////
void RecentFilesManager::importList(const wxString& fileName)
{
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(fileName.ToStdString().c_str()) != tinyxml2::XML_SUCCESS)
        return;

    const tinyxml2::XMLElement* files = doc.RootElement();

    if (files->IntAttribute("version", 1) != 1)
        return;

    const tinyxml2::XMLElement* file = files->FirstChildElement("file");

    while (file)
    {
        RecentFile recent(file->GetText());
        recent.opened = file->Int64Attribute("opened", 0);

        m_fileList.push_back(recent);

        file = file->NextSiblingElement();
    }
}


/////////////////////////////////////////////////
/// \brief Export the list to the indicated file.
///
/// \param fileName const wxString&
/// \return void
///
/////////////////////////////////////////////////
void RecentFilesManager::exportList(const wxString& fileName)
{
    if (!m_fileList.size())
        return;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* files = doc.NewElement("recent");
    doc.InsertEndChild(files);
    files->SetAttribute("version", 1);

    for (size_t i = 0; i < m_fileList.size(); i++)
    {
        tinyxml2::XMLElement* file = doc.NewElement("file");

        file->SetText(m_fileList[i].name.ToStdString().c_str());
        file->SetAttribute("opened", (int64_t)m_fileList[i].opened);

        files->InsertEndChild(file);
    }

    doc.SaveFile(fileName.ToStdString().c_str());
}


