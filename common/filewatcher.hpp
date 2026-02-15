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

#ifndef FILEWATCHER_HPP
#define FILEWATCHER_HPP

#include <wx/fswatcher.h>
#include <wx/filename.h>
#include <wx/wx.h>
#include <vector>
#include <string>
#include <map>

/////////////////////////////////////////////////
/// \brief This class is a specialisation to take
/// care about the internal standard paths.
/////////////////////////////////////////////////
class Filewatcher : public wxFileSystemWatcher
{
    private:
        std::vector<std::string> vDefaultPaths;
        std::map<wxString,wxString> mWatchedFiles;
        bool isDefaultPath(const wxFileName& path);

    public:
        Filewatcher() : wxFileSystemWatcher() {};

        bool SetDefaultPaths(const std::vector<std::string>& vPaths);
        bool Add(const wxFileName& path, int events = wxFSW_EVENT_ALL);
        bool Remove(const wxFileName& path);
};

#endif // FILEWATCHER_HPP

