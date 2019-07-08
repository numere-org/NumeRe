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


#ifndef VCSMANAGER_HPP
#define VCSMANAGER_HPP

#include <wx/wx.h>

class NumeReWindow;
class FileRevisions;

class VersionControlSystemManager
{
    private:
        NumeReWindow* m_parent;
        wxString getRevisionPath(const wxString& currentFilePath);

    public:
        VersionControlSystemManager(NumeReWindow* parent) : m_parent(parent) {}

        FileRevisions* getRevisions(const wxString& currentFile);
        bool hasRevisions(const wxString& currentFile);
};

#endif // VCSMANAGER_HPP

