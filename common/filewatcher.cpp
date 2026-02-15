/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#include "filewatcher.hpp"
#include "../gui/stringconv.hpp"
std::string replacePathSeparator(const std::string&);


/////////////////////////////////////////////////
/// \brief Determine if a file is in one of the
/// default paths.
///
/// \param path const wxFileName&
/// \return bool
///
/////////////////////////////////////////////////
bool Filewatcher::isDefaultPath(const wxFileName& path)
{
    std::string sPath = replacePathSeparator(wxToUtf8(path.GetPath()));

    for (size_t i = 2; i < vDefaultPaths.size(); i++)
    {
        if (sPath.find(vDefaultPaths[i]) != std::string::npos)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Set the default paths and load their
/// trees.
///
/// \param vPaths const std::vector<std::string>&
/// \return bool
///
/////////////////////////////////////////////////
bool Filewatcher::SetDefaultPaths(const std::vector<std::string>& vPaths)
{
    vDefaultPaths = vPaths;

    if (GetWatchedPathsCount())
        RemoveAll();

    for (size_t i = 2; i < vDefaultPaths.size(); i++)
        AddTree(wxFileName(wxFromUtf8(vDefaultPaths[i]) + "/"), wxFSW_EVENT_ALL, "*");

    for (auto iter = mWatchedFiles.begin(); iter != mWatchedFiles.end(); ++iter)
    {
        wxFileSystemWatcher::Add(wxFileName(iter->second + "/"));
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Add a path to the watched paths.
///
/// \param path const wxFileName&
/// \param events int
/// \return bool
///
/////////////////////////////////////////////////
bool Filewatcher::Add(const wxFileName& path, int events)
{
    if (isDefaultPath(path))
        return false;

    if (mWatchedFiles.find(path.GetFullPath()) != mWatchedFiles.end())
        return false;

    for (auto iter = mWatchedFiles.begin(); iter != mWatchedFiles.end(); ++iter)
    {
        if (iter->second == path.GetPath())
        {
            mWatchedFiles[path.GetFullPath()] = path.GetPath();
            return true;
        }
    }

    mWatchedFiles[path.GetFullPath()] = path.GetPath();
    return wxFileSystemWatcher::Add(wxFileName(path.GetPath() + "/"), events);
}


/////////////////////////////////////////////////
/// \brief Remove a path from the watched paths.
///
/// \param path const wxFileName&
/// \return bool
///
/////////////////////////////////////////////////
bool Filewatcher::Remove(const wxFileName& path)
{
    if (isDefaultPath(path))
        return false;

    if (mWatchedFiles.find(path.GetFullPath()) == mWatchedFiles.end())
        return false;

    auto iter_found = mWatchedFiles.find(path.GetFullPath());

    for (auto iter = mWatchedFiles.begin(); iter != mWatchedFiles.end(); ++iter)
    {
        if (iter == iter_found)
            continue;

        if (iter->second == path.GetPath())
        {
            mWatchedFiles.erase(path.GetFullPath());
            return true;
        }
    }

    mWatchedFiles.erase(path.GetFullPath());
    return wxFileSystemWatcher::Remove(wxFileName(path.GetPath() + "/"));
}

