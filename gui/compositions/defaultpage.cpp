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

#include "defaultpage.hpp"
#include "../IconManager.h"
#include "../../common/datastructures.h"
#include "../../common/recentfilemanager.hpp"
#include "../guilang.hpp"
#include "../../network/githubapi.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../kernel/versioninformation.hpp"

#include <wx/stdpaths.h>

#include "../NumeReWindow.h"

std::string getUserDisplayName(bool informal);

extern double g_pixelScale;

BEGIN_EVENT_TABLE(DefaultPage, GroupPanel)
	EVT_BUTTON(ID_MENU_NEW_ASK, DefaultPage::OnCommandEvent)
	EVT_BUTTON(ID_MENU_OPEN_SOURCE_LOCAL, DefaultPage::OnCommandEvent)
	EVT_BUTTON(ID_MENU_OPTIONS, DefaultPage::OnCommandEvent)
	EVT_BUTTON(ID_MENU_HELP, DefaultPage::OnCommandEvent)
	EVT_LIST_ITEM_ACTIVATED(-1, DefaultPage::OnListItemActivate)
	//EVT_SIZE(DefaultPage::OnSizeEvent)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Static helper function to load the
/// template file contents from file.
///
/// \param fileName const wxString&
/// \param searchPaths const wxArrayString&
/// \return wxString
///
/////////////////////////////////////////////////
static wxString getFileContents(const wxString& fileName, const wxArrayString& searchPaths)
{
    for (size_t i = 0; i < searchPaths.size(); i++)
    {
        if (wxFileExists(searchPaths[i] + "\\" + fileName))
        {
            wxFile file(searchPaths[i] + "\\" + fileName);

            if (!file.IsOpened())
                return wxEmptyString;

            ssize_t lng = file.Length();

            if (lng <= 0 || (wxFileOffset)lng != file.Length())
                return wxEmptyString;

            wxString fileContents;
            file.ReadAll(&fileContents, wxConvAuto(wxFONTENCODING_UTF8));

            return fileContents;
        }
    }

    return wxEmptyString;
}


/////////////////////////////////////////////////
/// \brief Resize the columns to fill the whole
/// list view.
///
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::resizeColumns()
{
    if (m_recentFiles->GetItemCount() > 0)
    {
        m_recentFiles->SetColumnWidth(0, wxLIST_AUTOSIZE);
        m_recentFiles->SetColumnWidth(1, wxLIST_AUTOSIZE);
        m_recentFiles->SetColumnWidth(2, wxLIST_AUTOSIZE);

        int col1 = m_recentFiles->GetColumnWidth(1)*1.2;
        int col2 = m_recentFiles->GetColumnWidth(2)*1.2;
        int col0 = m_recentFiles->GetSize().x-25;

        if (col0 > 250+col1+col2)
            col0 -= col1 + col2;
        else
            col0 = 250;

        m_recentFiles->SetColumnWidth(0, col0);
        m_recentFiles->SetColumnWidth(1, col1);
        m_recentFiles->SetColumnWidth(2, col2);
    }
    else
    {
        int col1 = 120;
        int col2 = 120;
        int col0 = m_recentFiles->GetSize().x-25;

        if (col0 > 250+col1+col2)
            col0 -= col1 + col2;
        else
            col0 = 250;

        m_recentFiles->SetColumnWidth(0, col0);
        m_recentFiles->SetColumnWidth(1, col1);
        m_recentFiles->SetColumnWidth(2, col2);
    }
}


/////////////////////////////////////////////////
/// \brief Construct the default page.
///
/// \param parent wxWindow*
/// \param searchPaths const wxArrayString&
/// \param showReleaseNotes bool
///
/////////////////////////////////////////////////
DefaultPage::DefaultPage(wxWindow* parent, const wxArrayString& searchPaths, bool showReleaseNotes) : GroupPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, false), m_icons(nullptr), m_rightBox(nullptr)
{
    SetBackgroundColour(*wxWHITE);

    AddSpacer(50, getMainSizer());

    if (!showReleaseNotes)
        getMainSizer()->AddStretchSpacer(1);

    wxBoxSizer* vGroup = createGroup(wxVERTICAL, getMainSizer(), 3);

    if (!showReleaseNotes)
        getMainSizer()->AddStretchSpacer(1);

    AddSpacer(50, getMainSizer());

    std::string sUserName = getUserDisplayName(true);

    wxStaticText* text = AddStaticText(this, vGroup, _guilang.get("GUI_STATUSBAR_WELCOMEPAGE_FILETYPE")
                                       + (sUserName.length() ? ", " + sUserName + "!" : std::string()));
    wxFont font = text->GetFont();
    font.SetPointSize(24);
    text->SetFont(font);

    wxBoxSizer* columnGroup = createGroup(wxHORIZONTAL, vGroup, 1);
    wxBoxSizer* contentGroup = createGroup(wxVERTICAL, columnGroup, 1);

    wxBoxSizer* controlGroup = createGroup(wxHORIZONTAL, contentGroup);
    wxBoxSizer* buttonGroup = createGroup(wxVERTICAL, controlGroup);

    CreateButton(this, buttonGroup, _guilang.get("GUI_MENU_NEWFILE"), ID_MENU_NEW_ASK);
    CreateButton(this, buttonGroup, _guilang.get("GUI_MENU_OPENFILE"), ID_MENU_OPEN_SOURCE_LOCAL);
    CreateButton(this, buttonGroup, _guilang.get("GUI_MENU_OPTIONS"), ID_MENU_OPTIONS);
    CreateButton(this, buttonGroup, _guilang.get("GUI_MENU_HELP"), ID_MENU_HELP);

    m_recentFiles = CreateListView(this, controlGroup, wxLC_REPORT, wxDefaultSize, wxID_ANY, 1);
    m_recentFiles->AppendColumn(_guilang.get("GUI_DEFAULTPAGE_NAME"), wxLIST_FORMAT_LEFT, 250);
    m_recentFiles->AppendColumn(_guilang.get("GUI_DEFAULTPAGE_TYPE"), wxLIST_FORMAT_LEFT, 120);
    m_recentFiles->AppendColumn(_guilang.get("GUI_DEFAULTPAGE_LAST_USED"), wxLIST_FORMAT_LEFT, 120);

    if (showReleaseNotes)
    {
        AddSpacer(25, columnGroup);
        m_rightBox = CreateTextInput(this, columnGroup, wxEmptyString, "###Loading ...",
                                     wxTE_AUTO_URL | wxTE_RICH2 | wxTE_MULTILINE | wxTE_READONLY,
                                     wxID_ANY, wxDefaultSize, wxALIGN_CENTER_VERTICAL, 1);
    }

    AddSpacer(25, contentGroup);

    m_bottomBox = CreateTextInput(this, contentGroup, wxEmptyString, getFileContents("tmpl_defaultpage.nlng", searchPaths),
                                  wxTE_AUTO_URL | wxTE_RICH2 | wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE | (g_pixelScale == 1.0 ? wxTE_NO_VSCROLL : 0),
                                  wxID_ANY, wxSize(-1, -1), wxALIGN_CENTER_VERTICAL, 1);

    AddSpacer(25, vGroup);
}


/////////////////////////////////////////////////
/// \brief Load the latest release from the
/// GitHub API and update the right box with the
/// release notes, if the box exists.
///
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::loadLatestRelease()
{
    if (!m_rightBox)
        return;

    Json::Value releases = GitHub::getReleases("https://api.github.com/repos/numere-org/NumeRe",
                                               wxStandardPaths::Get().GetExecutablePath().ToStdString());

    if (releases.empty())
    {
        m_rightBox->SetMarkupText("###No internet connection.");
        return;
    }

    wxString headline;

    if (versionToFloat(releases[0]["tag_name"].asString()) > getFloatingPointVersion())
        headline = "#==New: Highlighted changes in " + releases[0]["name"].asString() + "==";
    else
        headline = "#Highlighted changes in the currently installed version " + releases[0]["name"].asString();

    wxString body = releases[0]["body"].asString();
    body.erase(0, body.find('\n', body.find('#')));
    body.erase(body.rfind("[!["));

    m_rightBox->SetMarkupText(headline + body);
    m_rightBox->ShowPosition(0);
}


/////////////////////////////////////////////////
/// \brief Add the icon manager to this instance.
///
/// \param icons IconManager*
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::setIconManager(IconManager* icons)
{
    m_icons = icons;

    if (m_icons)
        m_recentFiles->SetImageList(m_icons->GetImageList(), wxIMAGE_LIST_SMALL);
}


/////////////////////////////////////////////////
/// \brief Display the recent opened files on the
/// default page.
///
/// \param recentFiles const RecentFilesManager&
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::setRecentFiles(const RecentFilesManager& recentFiles)
{
    size_t nCount = 0;

    for (int i = recentFiles.size()-1; i >= 0 && nCount < 20; i--, nCount++)
    {
        wxFileName fn(recentFiles[i].name);
        wxString sFileType = ("COMMON_FILETYPE_" + fn.GetExt().MakeUpper());

        sFileType = _guilang.get(sFileType.ToStdString());

        if (sFileType.StartsWith("COMMON_FILETYPE_"))
            sFileType = _guilang.get("COMMON_FILETYPE_UNKNOWN", fn.GetExt().MakeUpper());

        if (m_icons)
            m_recentFiles->InsertItem(nCount, recentFiles[i].name, m_icons->GetIconIndex(fn.GetExt()));
        else
            m_recentFiles->InsertItem(nCount, recentFiles[i].name);

        m_recentFiles->SetItem(nCount, 1, sFileType);
        m_recentFiles->SetItem(nCount, 2, toString(recentFiles[i].opened, GET_WITH_TEXT));
    }

    resizeColumns();
    m_recentFiles->Bind(wxEVT_SIZE, DefaultPage::OnSizeEvent, this);
}


/////////////////////////////////////////////////
/// \brief Convert a button event into a menu
/// event.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::OnCommandEvent(wxCommandEvent& event)
{
    event.SetEventType(wxEVT_MENU);
    wxGetTopLevelParent(this)->GetEventHandler()->ProcessEvent(event);
}


/////////////////////////////////////////////////
/// \brief Event handler for double-clicking one
/// of the recent files.
///
/// \param event wxListEvent&
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::OnListItemActivate(wxListEvent& event)
{
    static_cast<NumeReWindow*>(wxGetTopLevelParent(this))->OpenSourceFile(wxArrayString(1, event.GetText()));
}


/////////////////////////////////////////////////
/// \brief Automatically resize the list columns.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void DefaultPage::OnSizeEvent(wxSizeEvent& event)
{
    m_recentFiles->Freeze();
    resizeColumns();
    m_recentFiles->Thaw();
}

