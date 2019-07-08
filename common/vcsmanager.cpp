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

wxString VersionControlSystemManager::getRevisionPath(const wxString& currentFilePath)
{
    std::vector<std::string> vDefaultPaths = m_parent->getPathDefs();
    std::string currentPath = replacePathSeparator(currentFilePath.ToStdString());

    for (size_t i = LOADPATH; i < vDefaultPaths.size(); i++)
    {
        if (currentPath.substr(0, vDefaultPaths[i].length()) == vDefaultPaths[i])
            return vDefaultPaths[i] + "/.revisions" + currentPath.substr(vDefaultPaths[i].length()) + ".revisions";
    }

    return "";
}

FileRevisions* VersionControlSystemManager::getRevisions(const wxString& currentFile)
{
    wxString revisionPath = getRevisionPath(currentFile);

    if (revisionPath.length())
        return new FileRevisions(revisionPath);

    return nullptr;
}


bool VersionControlSystemManager::hasRevisions(const wxString& currentFile)
{
    wxString revisionPath = getRevisionPath(currentFile);

    if (revisionPath.length())
        return wxFileExists(revisionPath);

    return false;
}

