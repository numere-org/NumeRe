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

#ifndef FILEREVISIONS_HPP
#define FILEREVISIONS_HPP

#include <wx/wx.h>
#include <wx/filename.h>

class FileRevisions
{
    private:
        wxFileName m_revisionPath;
        wxString convertLineEndings(const wxString& content);

    public:
        FileRevisions(const wxString& revisionPath);
        size_t getRevisionCount();
        wxArrayString getRevisionList();
        wxString getRevision(size_t nRevision);
        wxString getRevision(const wxString& revString);
        void restoreRevision(size_t nRevision, const wxString& targetFile);
        void restoreRevision(const wxString& revString, const wxString& targetFile);
        size_t addRevision(const wxString& revisionContent);
        void undoRevision();
};


#endif // FILEREVISIONS_HPP


