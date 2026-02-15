/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "vcsmanager.hpp"
#include "filerevisions.hpp"
#include "../gui/NumeReWindow.h"
#include "datastructures.h"
#include <vector>
#include <string>

std::string replacePathSeparator(const std::string&);

/////////////////////////////////////////////////
/// \brief This method returns the path, where the revisions are stored.
///
/// \param currentFilePath const wxString&
/// \return wxString
///
/// The revision path is located in a subfolder called "/.revisions/",
/// which is hidden and mirrors the files in their revision files. This
/// function will only return a path for files belonging to the current
/// standard folders.
/////////////////////////////////////////////////
wxString VersionControlSystemManager::getRevisionPath(const wxString& currentFilePath)
{
    std::vector<std::string> vDefaultPaths = m_parent->getPathDefs();
    std::string currentPath = replacePathSeparator(wxToUtf8(currentFilePath));

    for (size_t i = LOADPATH; i < vDefaultPaths.size(); i++)
    {
        if (currentPath.substr(0, vDefaultPaths[i].length()) == vDefaultPaths[i])
            return vDefaultPaths[i] + "/.revisions" + currentPath.substr(vDefaultPaths[i].length()) + ".revisions";
    }

    // All other files' revisions are located next to them in the
    // same folder.
    return currentFilePath + ".revisions";
}


/////////////////////////////////////////////////
/// \brief This method returns the file revisions as pointer.
///
/// \param currentFile const wxString&
/// \return FileRevisions*
///
/// The FileRevisions object is created on the heap and the calling
/// function is responsible for cleaning up this allocated block
/// of memory (use std::unique_ptr for simplification). If this file
/// cannot have a revision, because it does not belong to the current
/// standard folders, a nullptr is returned instead.
/////////////////////////////////////////////////
FileRevisions* VersionControlSystemManager::getRevisions(const wxString& currentFile)
{
    wxString revisionPath = getRevisionPath(currentFile);

    if (revisionPath.length())
        return new FileRevisions(revisionPath);

    return nullptr;
}


/////////////////////////////////////////////////
/// \brief This method detects, whether the selected file has revisions.
///
/// \param currentFile const wxString&
/// \return bool
///
/// Use this method for detecting, whether the selected file has any
/// revisions. The method getRevisions() will not work, because the
/// constructor of the FileRevisions class will try to create everything
/// upon construction.
/////////////////////////////////////////////////
bool VersionControlSystemManager::hasRevisions(const wxString& currentFile)
{
    wxString revisionPath = getRevisionPath(currentFile);

    if (revisionPath.length())
        return wxFileExists(revisionPath);

    return false;
}

