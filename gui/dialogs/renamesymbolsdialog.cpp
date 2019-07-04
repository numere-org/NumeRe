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

#include "renamesymbolsdialog.hpp"
#include "../compositions/grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;

std::string toString(int);

// Constructor
RenameSymbolsDialog::RenameSymbolsDialog(wxWindow* parent, const vector<wxString>& vChangeLog, wxWindowID id, const wxString& title, const wxString& defaultval) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER | wxSTAY_ON_TOP)
{
    m_replaceInComments = nullptr;
    m_replaceInWholeFile = nullptr;
    m_replaceBeforeCursor = nullptr;
    m_replaceAfterCursor = nullptr;
    m_replaceName = nullptr;

    // Create the vertical sizer for this dialog
    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vSizer);

    // Create a GroupPanel and add it to the
    // vertical sizer of the current dialog
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxSize(350, -1));
    vSizer->Add(panel, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // Create the text input control
    m_replaceName = panel->CreateTextInput(panel, panel->getVerticalSizer(), _guilang.get("GUI_DLG_RENAMESYMBOLS_QUESTION"), defaultval);

    panel->AddSpacer();

    // Create the checkbox group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_DLG_RENAMESYMBOLS_SETTINGS"));

    m_replaceInComments = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACECOMMENTS"));
    m_replaceInComments->SetValue(false);

    m_replaceInWholeFile = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEWHOLEFILE"));
    m_replaceInWholeFile->SetValue(false);

    m_replaceBeforeCursor = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEBEFOREPOS"));
    m_replaceBeforeCursor->SetValue(true);

    m_replaceAfterCursor = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_DLG_RENAMESYMBOLS_REPLACEAFTERPOS"));
    m_replaceAfterCursor->SetValue(true);

    // Greate the changes log group
    group = panel->createGroup(_guilang.get("GUI_DLG_RENAMESYMBOLS_CHANGELOG"));

    // Create the changes log list view and
    // create the necessary columns
    wxListView* listview = panel->CreateListView(group->GetStaticBox(), group, wxLC_REPORT | wxLC_VRULES);
    listview->AppendColumn("ID");
    listview->AppendColumn(_guilang.get("GUI_DLG_RENAMESYMBOLS_CHANGELOG_OLDNAME"));
    listview->AppendColumn(_guilang.get("GUI_DLG_RENAMESYMBOLS_CHANGELOG_NEWNAME"));

    // Fill the changes log with the supplied
    // changes log from the editor
    fillChangesLog(listview, vChangeLog);

    // Create a standard button sizer
    vSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    GetSizer()->SetSizeHints(this);

    this->Centre();

    // Focus the text input control and select
    // its contents to allow overtyping
    m_replaceName->SetFocus();
    m_replaceName->SelectAll();
}

// This private member function fills the changes log
// in the current dialog with the passed changes log
// information supplied by the editor
void RenameSymbolsDialog::fillChangesLog(wxListView* listView, const std::vector<wxString>& vChangeLog)
{
    wxString sCurrentItem;

    // Gp through the supplied changes log and decode it
    for (int i = vChangeLog.size()-1; i >= 0 ; i--)
    {
        sCurrentItem = vChangeLog[i];

        listView->InsertItem((vChangeLog.size()-1) - i, toString(i+1));
        listView->SetItem((vChangeLog.size()-1) - i, 1, sCurrentItem.substr(0, sCurrentItem.find('\t')));
        listView->SetItem((vChangeLog.size()-1) - i, 2, sCurrentItem.substr(sCurrentItem.find('\t')+1));
    }

    // Create an empty changes log, if necessary and
    // resize the columns
    if (!vChangeLog.size())
    {
        listView->InsertItem(0, "---");
        listView->SetItem(0, 1, "---");
        listView->SetItem(0, 2, "---");
        listView->SetItemTextColour(0, wxColour(128, 128, 128));
        listView->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
        listView->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
        listView->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);

    }
    else
    {
        listView->SetColumnWidth(0, wxLIST_AUTOSIZE);
        listView->SetColumnWidth(1, wxLIST_AUTOSIZE);
        listView->SetColumnWidth(2, wxLIST_AUTOSIZE);
    }
}

