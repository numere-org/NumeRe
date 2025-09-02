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


#ifndef REVISIONDIALOG_HPP
#define REVISIONDIALOG_HPP

#include <wx/wx.h>
#include "../../common/filerevisions.hpp"
#include "../controls/treelistctrl.h"

class NumeReWindow;
class NumeReEditor;

/////////////////////////////////////////////////
/// \brief This class represents the dialog
/// listing the file revisions of the current
/// selected file.
/////////////////////////////////////////////////
class RevisionDialog : public wxDialog
{
    private:
        FileRevisions* revisions;
        wxcode::wxTreeListCtrl* revisionList;
        wxTreeItemId clickedItem;
        NumeReWindow* mainWindow;
        NumeReEditor* editor;
        wxString currentFilePath;
        wxString currentFileName;

        void populateRevisionList();
        void showRevision(const wxString& revString);
        void compareRevisions(const wxString& rev1, const wxString& rev2);

        // Event handling functions
        void OnLeftClick(wxTreeEvent& event);
        void OnRightClick(wxTreeEvent& event);
        void OnItemActivated(wxTreeEvent& event);
        void OnMenuEvent(wxCommandEvent& event);

    public:
        RevisionDialog(wxWindow* parent, FileRevisions* rev, const wxString& fileNameAndPath);

        ~RevisionDialog()
        {
            if (revisions)
                delete revisions;
        }

        DECLARE_EVENT_TABLE();
};



#endif // REVISIONDIALOG_HPP


