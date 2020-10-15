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

#include "revisiondialog.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../NumeReWindow.h"

BEGIN_EVENT_TABLE(RevisionDialog, wxDialog)
    EVT_TREE_ITEM_RIGHT_CLICK(-1, RevisionDialog::OnRightClick)
    EVT_TREE_ITEM_ACTIVATED(-1, RevisionDialog::OnItemActivated)
    EVT_MENU_RANGE(ID_REVISIONDIALOG_SHOW, ID_REVISIONDIALOG_REFRESH, RevisionDialog::OnMenuEvent)
END_EVENT_TABLE()

extern Language _guilang;


/////////////////////////////////////////////////
/// \brief Constructor. Creates the window and populates the tree list ctrl.
///
/// \param parent wxWindow*
/// \param rev FileRevisions*
/// \param currentFileName const wxString&
///
/////////////////////////////////////////////////
RevisionDialog::RevisionDialog(wxWindow* parent, FileRevisions* rev, const wxString& currentFileName)
    : wxDialog(parent, wxID_ANY, _guilang.get("GUI_DLG_REVISIONDIALOG_TITLE"), wxDefaultPosition, wxSize(750, 500), wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX), revisions(rev), currentFile(currentFileName)
{
    mainWindow = static_cast<NumeReWindow*>(parent);

    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    revisionList = new wxcode::wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_EXTENDED | wxTR_MULTIPLE);
    revisionList->AddColumn(_guilang.get("GUI_DLG_REVISIONDIALOG_REV"), 150);
    revisionList->AddColumn(_guilang.get("GUI_DLG_REVISIONDIALOG_DATE"), 150);
    revisionList->AddColumn(_guilang.get("GUI_DLG_REVISIONDIALOG_COMMENT"), 450);
    revisionList->AddRoot(currentFile);

    populateRevisionList();

    vsizer->Add(revisionList, 1, wxEXPAND | wxALL, 5);
    vsizer->Add(CreateButtonSizer(wxOK), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    SetSizer(vsizer);
}


/////////////////////////////////////////////////
/// \brief This method pupulates the tree list ctrl.
///
/// \return void
///
/// Depending on the the type of the revision, the line colour
/// is selected. Tags are additionally indented and printed in
/// italics.
/////////////////////////////////////////////////
void RevisionDialog::populateRevisionList()
{
    wxArrayString revList = revisions->getRevisionList();
    wxString currentRev = revisions->getCurrentRevision();
    wxTreeItemId currentItem;

    // Handle each revision independently
    for (size_t i = 0; i < revList.size(); i++)
    {
        // Is it a tag?
        if (revList[i].substr(0, 3) == "tag")
        {
            // A tag is appended to the previous revision
            wxTreeItemId currentTagItem = revisionList->AppendItem(currentItem, revList[i].substr(0, revList[i].find('\t')));
            revisionList->SetItemText(currentTagItem, 1, revList[i].substr(revList[i].find('\t')+1, revList[i].find('\t', revList[i].find('\t')+1) - revList[i].find('\t')-1));
            revisionList->SetItemText(currentTagItem, 2, revList[i].substr(revList[i].rfind('\t')+1));
            revisionList->SetItemFont(currentTagItem, revisionList->GetFont().MakeItalic());
            revisionList->SetItemTextColour(currentTagItem, wxColour(0, 0, 192));
        }
        else
        {
            // Create a new revision in the tree
            currentItem = revisionList->AppendItem(revisionList->GetRootItem(), revList[i].substr(0, revList[i].find('\t')));
            revisionList->SetItemText(currentItem, 1, revList[i].substr(revList[i].find('\t')+1, revList[i].find('\t', revList[i].find('\t')+1) - revList[i].find('\t')-1));
            revisionList->SetItemText(currentItem, 2, revList[i].substr(revList[i].rfind('\t')+1));

            if (revisionList->GetItemText(currentItem, 2).substr(0, 5) == "MOVE:")
                revisionList->SetItemTextColour(currentItem, wxColour(128, 0, 0));

            if (revisionList->GetItemText(currentItem, 2).substr(0, 7) == "RENAME:")
                revisionList->SetItemTextColour(currentItem, wxColour(0, 128, 0));

            // Do not display the "DIFF" comment identifier
            if (revisionList->GetItemText(currentItem, 2).substr(0, 4) == "DIFF")
                revisionList->SetItemText(currentItem, 2, "");

            if (revisionList->GetItemText(currentItem, 0) == currentRev)
                revisionList->SetItemBold(currentItem, true);
        }
    }

    revisionList->ExpandAll(revisionList->GetRootItem());
}


/////////////////////////////////////////////////
/// \brief This method displays the selected revision in the editor.
///
/// \param revString const wxString&
/// \return void
///
/// The method gets the revision from the associated FileRevisions
/// object and passes it to the main window method for displaying a
/// revision.
/////////////////////////////////////////////////
void RevisionDialog::showRevision(const wxString& revString)
{
    wxString revision = revisions->getRevision(revString);
    mainWindow->ShowRevision(revString + "-" + currentFile, revision);
}


/////////////////////////////////////////////////
/// \brief This method compares two defined
/// revisions and opens them as a diff file in the
/// editor.
///
/// \param rev1 const wxString&
/// \param rev2 const wxString&
/// \return void
///
/////////////////////////////////////////////////
void RevisionDialog::compareRevisions(const wxString& rev1, const wxString& rev2)
{
    mainWindow->ShowRevision(rev1 + "-" + rev2 + "-" + currentFile + ".diff", revisions->compareRevisions(rev1, rev2));
}


/////////////////////////////////////////////////
/// \brief This method displays the context menu containing the actions.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void RevisionDialog::OnRightClick(wxTreeEvent& event)
{
    clickedItem = event.GetItem();

    // do nothing, if the parent node is not the
    // table root node
    if (event.GetItem() == revisionList->GetRootItem())
        return;

    // Create the menu
    wxMenu popUpmenu;

    // Append commons
    popUpmenu.Append(ID_REVISIONDIALOG_SHOW, _guilang.get("GUI_DLG_REVISIONDIALOG_SHOW"));
    popUpmenu.AppendSeparator();
    wxArrayTreeItemIds selection;

    if (revisionList->GetSelections(selection) >= 2)
    {
        popUpmenu.Append(ID_REVISIONDIALOG_COMPARE, _guilang.get("GUI_DLG_REVISIONDIALOG_COMPARE", revisionList->GetItemText(selection[0]).ToStdString(), revisionList->GetItemText(selection[1]).ToStdString()));
        popUpmenu.AppendSeparator();
    }

    popUpmenu.Append(ID_REVISIONDIALOG_TAG, _guilang.get("GUI_DLG_REVISIONDIALOG_TAG"));
    popUpmenu.Append(ID_REVISIONDIALOG_RESTORE, _guilang.get("GUI_DLG_REVISIONDIALOG_RESTORE"));
    popUpmenu.Append(ID_REVISIONDIALOG_REFRESH, _guilang.get("GUI_DLG_REVISIONDIALOG_REFRESH"));

    // Display the menu
    PopupMenu(&popUpmenu, event.GetPoint());

}


/////////////////////////////////////////////////
/// \brief This method displays the double-clicked revision.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void RevisionDialog::OnItemActivated(wxTreeEvent& event)
{
    wxString revID = revisionList->GetItemText(event.GetItem());
    showRevision(revID);
}


/////////////////////////////////////////////////
/// \brief This method handles the menu events emitted from the context menu.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void RevisionDialog::OnMenuEvent(wxCommandEvent& event)
{
    wxString revID;

    // Ensure that the tree item id is valid and that
    // the user did not click on the root item
    if (clickedItem.IsOk() && clickedItem != revisionList->GetRootItem())
        revID = revisionList->GetItemText(clickedItem);
    else if (event.GetId() != ID_REVISIONDIALOG_REFRESH)
        return;

    switch (event.GetId())
    {
        case ID_REVISIONDIALOG_SHOW:
            // Show the right-clicked revision
            showRevision(revID);
            break;
        case ID_REVISIONDIALOG_COMPARE:
            {
                wxArrayTreeItemIds selectedIds;

                if (revisionList->GetSelections(selectedIds) >= 2)
                    compareRevisions(revisionList->GetItemText(selectedIds[0]), revisionList->GetItemText(selectedIds[1]));

                break;
            }
        case ID_REVISIONDIALOG_TAG:
            {
                // Tag the right-clicked revision. Display
                // a text entry dialog to ask for the
                // comment
                wxTextEntryDialog textdialog(this, _guilang.get("GUI_DLG_REVISIONDIALOG_PROVIDETAGCOMMENT"), _guilang.get("GUI_DLG_REVISIONDIALOG_PROVIDETAGCOMMENT_TITLE"), wxEmptyString, wxCENTER | wxOK | wxCANCEL);
                int ret = textdialog.ShowModal();

                if (ret == wxID_OK)
                {
                    revisions->tagRevision(revID, textdialog.GetValue());
                    revisionList->DeleteChildren(revisionList->GetRootItem());
                    populateRevisionList();
                }

                break;
            }
        case ID_REVISIONDIALOG_RESTORE:
            {
                // Restore the right-clicked revision. Display
                // a file dialog to ask for the target file.
                wxFileDialog filedialog(this, _guilang.get("GUI_DLG_REVISIONDIALOG_RESTOREFILE"), wxGetCwd(), currentFile, wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                int ret = filedialog.ShowModal();

                if (ret == wxID_OK)
                    revisions->restoreRevision(revID, filedialog.GetPath());

                break;
            }
        case ID_REVISIONDIALOG_REFRESH:
            {
                // Refresh the contents of the revision list
                revisionList->DeleteChildren(revisionList->GetRootItem());
                populateRevisionList();

                break;
            }
    }
}

