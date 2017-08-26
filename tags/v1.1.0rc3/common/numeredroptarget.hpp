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

#ifndef NUMEREDROPTARGET_HPP
#define NUMEREDROPTARGET_HPP

#include <wx/wx.h>
#include <wx/dnd.h>

#if wxUSE_DRAG_AND_DROP


class NumeReDropTarget : public wxDropTarget
{
    public:
        enum parentType {NUMEREWINDOW, EDITOR, FILETREE, CONSOLE};
        enum fileType {NOEXTENSION, TEXTFILE, BINARYFILE, EXECUTABLE, NOTSUPPORTED};

        NumeReDropTarget(wxWindow* topwindow, wxWindow* owner, parentType type);

        wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult defaultDragResult);
        wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult defaultDragResult);
        fileType getFileType(const wxString& filename);

    private:
        wxWindow* m_owner;
        wxWindow* m_topWindow;
        parentType m_type;
};




#endif //wxUSE_DRAG_AND_DROP

#endif

