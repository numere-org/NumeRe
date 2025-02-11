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

#ifndef RECENTFILEMANAGER_HPP
#define RECENTFILEMANAGER_HPP

#include <vector>
#include <ctime>
#include <wx/string.h>

/////////////////////////////////////////////////
/// \brief This structure represents a single
/// file together with its last opened time-point.
/////////////////////////////////////////////////
struct RecentFile
{
    wxString name;
    __time64_t opened;

    RecentFile(const wxString& newFile);
    bool operator==(const RecentFile& other) const;
};

/////////////////////////////////////////////////
/// \brief Implements an object to store the
/// recent used files in the order, in which they
/// have been added to the manager, moving
/// existing entries to the top.
/////////////////////////////////////////////////
class RecentFilesManager
{
    private:
        std::vector<RecentFile> m_fileList;

    public:
        void push(const wxString& newFile);
        size_t size() const;
        const RecentFile& operator[](size_t i) const;

        void importList(const wxString& fileName);
        void exportList(const wxString& fileName);
};

#endif // RECENTFILEMANAGER_HPP

