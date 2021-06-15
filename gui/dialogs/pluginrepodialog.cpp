/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#include "pluginrepodialog.hpp"
#include "../IconManager.h"
#include "../compositions/grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../common/datastructures.h"
#include "../../common/http.h"
#include "../controls/searchctrl.hpp"

#include <fstream>
#include <vector>
#include <algorithm>


#define WINDOWWIDTH 1000
#define CTRLWIDTH WINDOWWIDTH-50
#define WINDOWHEIGHT 600



/////////////////////////////////////////////////
/// \brief This class specializes the generic
/// search control to interact with the package
/// list of the package repository browser.
/////////////////////////////////////////////////
class PackageListSearchCtrl : public SearchCtrl
{
    private:
        wxcode::wxTreeListCtrl* m_associatedCtrl;

    protected:
        // Interaction functions with the wxTreeCtrl
        virtual bool selectItem(const wxString& value) override;
        wxTreeItemId findItem(const wxString& value, wxTreeItemId node);
        virtual wxArrayString getCandidates(const wxString& enteredText) override;
        wxArrayString getChildCandidates(const wxString& enteredText, wxTreeItemId node);

    public:
        PackageListSearchCtrl(wxWindow* parent, wxWindowID id, const wxString& hint = wxEmptyString, const wxString& calltip = wxEmptyString, wxcode::wxTreeListCtrl* associatedCtrl = nullptr) : SearchCtrl(parent, id, wxEmptyString), m_associatedCtrl(associatedCtrl)
        {
            // Provide a neat hint to the user, what he
            // may expect from this control
            SetHint(hint);
            wxArrayInt sizes;
            sizes.Add(450, 1);
            sizes.Add(200, 1);
            sizes.Add(CTRLWIDTH-650, 1);

            popUp->SetCallTips(calltip);
            popUp->SetColSizes(sizes);
        }

};


/////////////////////////////////////////////////
/// \brief This method searches and selects the
/// item with the passed label in the associated
/// tree.
///
/// \param value const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageListSearchCtrl::selectItem(const wxString& value)
{
    // Ensure that a tree as associated
    if (m_associatedCtrl)
    {
        // Get the root node and the first child
        wxTreeItemIdValue cookie;
        wxTreeItemId root = m_associatedCtrl->GetRootItem();
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(root, cookie);

        // If the child exists, try to find the passed string
        // in the labels of all childs
        if (child.IsOk())
        {
            wxTreeItemId match = findItem(value[0] == ' ' ? value.substr(1).Lower() : value.Lower(), child);

            // If a label was found, ensure it is visible
            // and select it
            if (match.IsOk())
            {
                m_associatedCtrl->EnsureVisible(match);
                m_associatedCtrl->SelectItem(match);

                return true;
            }
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This method searches for the tree
/// item, whose label corresponds to the passed
/// string.
///
/// \param value const wxString&
/// \param node wxTreeItemId
/// \return wxTreeItemId
///
/////////////////////////////////////////////////
wxTreeItemId PackageListSearchCtrl::findItem(const wxString& value, wxTreeItemId node)
{
    // Go through all siblings
    do
    {
        // Return the current node, if it
        // corresponds to the passed string
        if (m_associatedCtrl->GetItemText(node).Lower() == value)
            return node;

        // Search the first child
        wxTreeItemIdValue cookie;
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(node, cookie);

        // If the child exists, try to find the
        // passed string in its or its siblings
        // labels
        if (child.IsOk())
        {
            wxTreeItemId match = findItem(value, child);

            // Return the id, if it exists
            if (match.IsOk())
                return match;
        }
    }
    while ((node = m_associatedCtrl->GetNextSibling(node)).IsOk());

    // Return an invalid tree item id, if
    // nothing had been found
    return wxTreeItemId();
}


/////////////////////////////////////////////////
/// \brief This method returns an array of
/// strings containing possible candidates for
/// the passed search string.
///
/// \param enteredText const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString PackageListSearchCtrl::getCandidates(const wxString& enteredText)
{
    // Ensure that a tree control was associated
    if (m_associatedCtrl)
    {
        // Find root node and its child
        wxTreeItemIdValue cookie;
        wxTreeItemId root = m_associatedCtrl->GetRootItem();
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(root, cookie);

        // If the child exists, get all labels of its
        // siblings and their childs, which are candidates
        // for the passed string
        if (child.IsOk())
            return getChildCandidates(enteredText.Lower(), child);
    }

    // Return an empty string otherwise
    return wxArrayString();
}


/////////////////////////////////////////////////
/// \brief This method returns an array of
/// strings containing possible candiates for the
/// passed search string, which correspond to the
/// current tree node, its siblings and its
/// childs.
///
/// \param enteredText const wxString&
/// \param node wxTreeItemId
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString PackageListSearchCtrl::getChildCandidates(const wxString& enteredText, wxTreeItemId node)
{
    wxArrayString stringArray;
    wxArrayString enteredWords = wxStringTokenize(enteredText);

    // Go through all siblings
    do
    {
        size_t nCount = 0;
        // Append the current label, if it contains the
        // searched string
        for (size_t i = 0; i < enteredWords.size(); i++)
        {
            if ((m_associatedCtrl->GetItemBold(node) && m_associatedCtrl->GetItemText(node).Lower().find(enteredWords[i]) != std::string::npos)
                || m_associatedCtrl->GetItemText(node, 1).Lower().find(enteredWords[i]) != std::string::npos
                || m_associatedCtrl->GetItemText(node, 2).Lower().find(enteredWords[i]) != std::string::npos)
                nCount++;
        }

        wxString match;

        // Only add the headline if we had found at least
        // half of the number of keywords
        // (otherwise the list will get longer and longer)
        if (nCount && nCount >= std::rint(2*enteredWords.size() / 3.0))
            match = m_associatedCtrl->GetItemText(node) + "~" + m_associatedCtrl->GetItemText(node, 1) + "~" + m_associatedCtrl->GetItemText(node, 2);

        // Find the first child of the current node
        wxTreeItemIdValue cookie;
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(node, cookie);

        // If the child exists, find the candidates in
        // its siblings and childs
        if (child.IsOk())
        {
            wxArrayString childArray = getChildCandidates(enteredText, child);

            // If the child array reports a size
            // we'll add the parent line here
            if (childArray.size())
                match = m_associatedCtrl->GetItemText(node) + "~" + m_associatedCtrl->GetItemText(node, 1) + "~" + m_associatedCtrl->GetItemText(node, 2);
        }

        if (match.length())
            stringArray.Add(match);
    }
    while ((node = m_associatedCtrl->GetNextSibling(node)).IsOk());

    return stringArray;
}







#define REPO_LOCATION "http://svn.code.sf.net/p/numere/plugins/repository/"
#define REPO_URL "Repository URL"
#define DEPENDENCIES "Dependencies"

#define PACKAGCOLUMN 0
#define REPOCOLUMN 1
#define INSTALLEDCOLUMN 2

#define INSTALLEDCOLOUR wxColour(220,255,220)
#define UPDATECOLOUR wxColour(220,220,255)

extern Language _guilang;

BEGIN_EVENT_TABLE(PackageRepoBrowser, ViewerFrame)
    EVT_BUTTON(ID_REPODLG_INSTALL, PackageRepoBrowser::OnInstall)
    EVT_BUTTON(ID_REPODLG_UNINSTALL, PackageRepoBrowser::OnUninstall)
    EVT_TREE_SEL_CHANGED(-1, PackageRepoBrowser::OnItemSelect)
    EVT_THREAD(-1, PackageRepoBrowser::OnThreadUpdate)
    EVT_CLOSE(PackageRepoBrowser::OnClose)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Static helper function to extract the
/// value of an install info tag.
///
/// \param sTaggedString const std::string&
/// \param sTag const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getTagValue(const std::string& sTaggedString, const std::string& sTag)
{
    int nTag = findParameter(sTaggedString, sTag, '=');

    if (nTag)
        return getArgAtPos(sTaggedString, nTag+sTag.length());

    return "";
}


/////////////////////////////////////////////////
/// \brief PackageRepositoryBrowser constructor.
/// Starts the file loading task.
///
/// \param parent wxWindow*
/// \param terminal NumeReTerminal*
/// \param icons IconManager*
///
/////////////////////////////////////////////////
PackageRepoBrowser::PackageRepoBrowser(wxWindow* parent, NumeReTerminal* terminal, IconManager* icons) : ViewerFrame(parent, PACKAGE_REPO_BROWSER_TITLE)
{
    SetSize(WINDOWWIDTH, WINDOWHEIGHT);
    m_terminal = terminal;
    m_icons = icons;
    m_scriptPath = m_terminal->getPathSettings()[SCRIPTPATH];

    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    m_listCtrl = panel->CreateWxcTreeListCtrl(panel, panel->getMainSizer(), wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_EXTENDED | wxTR_HIDE_ROOT);
    m_listCtrl->SetImageList(m_icons->GetImageList());

    m_listCtrl->AddColumn("Packages", 300);
    m_listCtrl->AddColumn("Repository", 500);
    m_listCtrl->AddColumn("Installed", 150);

    PackageListSearchCtrl* treesearch = new PackageListSearchCtrl(panel, wxID_ANY, "Search packages ...", wxEmptyString, m_listCtrl);
    panel->getMainSizer()->Prepend(treesearch, 0, wxALL | wxEXPAND, 5);

    m_statusText = panel->AddStaticText(panel, panel->getMainSizer(), "", wxID_ANY, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
    m_progress = panel->CreateGauge(panel, panel->getMainSizer(), wxGA_HORIZONTAL);
    m_filesLoaded = false;

    wxBoxSizer* hsizer = panel->createGroup(wxHORIZONTAL, panel->getMainSizer());

    m_installButton = panel->CreateButton(panel, hsizer, "Install/Update", ID_REPODLG_INSTALL);
    m_uninstallButton = panel->CreateButton(panel, hsizer, "Uninstall", ID_REPODLG_UNINSTALL);

    m_installButton->Disable();
    m_uninstallButton->Disable();

    StartThread(TASK_LOADREPO);
}


/////////////////////////////////////////////////
/// \brief Detect installed packages by loading
/// the list of installed and comparing them to
/// the loaded contents from the repository.
///
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::DetectInstalledPackages()
{
    std::vector<std::string> vInstalled = m_terminal->getInstalledPackages();

    wxTreeItemIdValue cookie;
    wxTreeItemId item = m_listCtrl->GetFirstChild(m_listCtrl->GetRootItem(), cookie);

    while (item.IsOk())
    {
        size_t packagenamelength = m_listCtrl->GetItemText(item).length();
        m_listCtrl->SetItemBackgroundColour(item, *wxWHITE);
        m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "");

        // Find already installed versions
        for (size_t i = 0; i < vInstalled.size(); i++)
        {
            if (vInstalled[i].substr(0, packagenamelength+1) == m_listCtrl->GetItemText(item) + "\t")
            {
                // Colourize the line backgrounds correspondingly,
                // if a match had been found
                if (m_listCtrl->GetItemText(item, REPOCOLUMN) > "v" + vInstalled[i].substr(packagenamelength+1))
                {
                    m_listCtrl->SetItemBackgroundColour(item, UPDATECOLOUR);
                    m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "v" + vInstalled[i].substr(packagenamelength+1) + " (Updateable)");
                }
                else
                {
                    m_listCtrl->SetItemBackgroundColour(item, INSTALLEDCOLOUR);
                    m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "v" + vInstalled[i].substr(packagenamelength+1));
                }

                break;
            }
        }

        item = m_listCtrl->GetNextSibling(item);
    }

}


/////////////////////////////////////////////////
/// \brief Start a new thread with the passed
/// task.
///
/// \param task ThreadTask
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::StartThread(ThreadTask task)
{
    if (!GetThread() || !GetThread()->IsRunning())
    {
        m_task = task;

        if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR)
            return;

        if (GetThread()->Run() != wxTHREAD_NO_ERROR)
            return;
    }
}


/////////////////////////////////////////////////
/// \brief Secondary thread worker function.
///
/// \return wxThread::ExitCode
///
/////////////////////////////////////////////////
wxThread::ExitCode PackageRepoBrowser::Entry()
{
    try
    {
        if (m_task == TASK_LOADREPO)
        {
            m_statusText->SetLabel("Status: Fetching Package list ...");
            m_progress->Pulse();
            m_listCtrl->AddRoot("ROOT");
            std::vector<std::string> vRepoContents = getRepoList(REPO_LOCATION);

            m_statusText->SetLabel("Status: Reading Package information ...");
            m_progress->SetRange(vRepoContents.size());
            m_progress->SetValue(1);

            for (size_t i = 0; i < vRepoContents.size(); i++)
            {
                // Test, whether the user wants to close the window
                if (GetThread()->TestDestroy())
                {
                    m_task = TASK_NONE;
                    return (wxThread::ExitCode)0;
                }

                populatePackageList(vRepoContents[i]);
                m_progress->SetValue(i+1);
            }

            m_statusText->SetLabel("Status: Detecting installations ...");
            DetectInstalledPackages();
            m_statusText->SetLabel("Status: Package database successfully loaded.");
        }
        else if (m_task == TASK_LOADFILES)
        {
            m_statusText->SetLabel("Status: Retrieving packages ...");
            m_progress->SetRange(m_vUrls.size());
            m_progress->SetValue(1);

            bool success = true;

            for (size_t i = 0; i < m_vUrls.size(); i++)
            {
                // Test, wether the user wants to close the window
                if (GetThread()->TestDestroy())
                {
                    m_task = TASK_NONE;
                    return (wxThread::ExitCode)0;
                }

                success = success && getFileFromRepo(m_vUrls[i]);
                m_progress->SetValue(i+1);
            }

            m_filesLoaded = success;

            if (success)
                m_statusText->SetLabel("Status: Packages successfully downloaded.");
            else
                m_statusText->SetLabel("Status: One or more packages could not be downloaded.");
        }
    }
    catch (url::Error& e)
    {
        m_statusText->SetLabel("HTTP Error: " + std::string(e.what()));
        m_progress->SetRange(100);
        m_progress->SetValue(100);
    }

    m_task = TASK_NONE;

    wxQueueEvent(GetEventHandler(), new wxThreadEvent());
    return (wxThread::ExitCode)0;
}


/////////////////////////////////////////////////
/// \brief Thread update event handler. Will
/// trigger the actual installation, if the files
/// were loaded successfully.
///
/// \param event wxThreadEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::OnThreadUpdate(wxThreadEvent& event)
{
    if (m_filesLoaded && m_fileNameToInstall.length())
    {
        m_filesLoaded = false;

        wxTreeItemIdValue cookie;
        wxTreeItemId item = m_listCtrl->GetFirstChild(m_listCtrl->GetRootItem(), cookie);

        // Mark downloaded files as installing
        while (item.IsOk())
        {
            if (m_listCtrl->GetItemText(item, INSTALLEDCOLUMN) == "Downloading ...")
                m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "Installing ...");

            item = m_listCtrl->GetNextSibling(item);
        }

        m_terminal->pass_command("install \"packages/" + m_fileNameToInstall + "\"", false);
    }
}


/////////////////////////////////////////////////
/// \brief OnClose event handler. Will terminate
/// the thread, if it's running.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::OnClose(wxCloseEvent& event)
{
    if (m_task != TASK_NONE && GetThread() && GetThread()->IsRunning())
    {
        if (GetThread()->Delete() != wxTHREAD_NO_ERROR && event.CanVeto())
            event.Veto();
    }

    Destroy();
}


/////////////////////////////////////////////////
/// \brief This member function fetches the
/// repository main page as HTML and extracts the
/// list of item links in it. These may be files
/// and folders.
///
/// \param sRepoUrl const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> PackageRepoBrowser::getRepoList(const std::string& sRepoUrl)
{
    std::vector<std::string> vRepoContents;
    std::string sRepoContents = url::get(sRepoUrl + createSalt());
    sRepoContents = sRepoContents.substr(sRepoContents.find("<ul>")+4);
    sRepoContents.erase(sRepoContents.find("</ul>"));

    // Extract each <li></li> pairs content
    while (sRepoContents.length() && sRepoContents.find("</li>") != std::string::npos)
    {
        std::string sCurrentPackage = sRepoContents.substr(0, sRepoContents.find("</li>"));
        sRepoContents.erase(0, sRepoContents.find("</li>")+5);

        if (sCurrentPackage.find("href=\"") == std::string::npos)
            break;

        // Get the address
        sCurrentPackage = sCurrentPackage.substr(sCurrentPackage.find("href=\"") + 6);
        sCurrentPackage.erase(sCurrentPackage.find('"'));

        // Get the resolved content
        vRepoContents.push_back(sRepoUrl + sCurrentPackage);
    }

    return vRepoContents;
}


/////////////////////////////////////////////////
/// \brief Gets an URL and retrieve its
/// counterpart from the repository. If the link
/// does not reference a file but a folder, a
/// recursion is started to resolve the contents
/// of the folder.
///
/// \param sUrl const std::string&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::populatePackageList(const std::string& sUrl)
{
    // We do not want to follow parent directory
    // references
    if (sUrl.find("/../") != std::string::npos)
        return;

    // If the current URL is not a file, it might be a folder,
    // therefore we trigger a recursion here
    if (sUrl.back() == '/')
    {
        std::vector<std::string> vRepoContents = getRepoList(sUrl);

        for (const std::string& url : vRepoContents)
        {
            populatePackageList(url);
        }

        return;
    }

    // We only accept NSCR files at the moment
    if (sUrl.find(".nscr") == std::string::npos)
        return;

    std::string sCurrentPackage = url::get(sUrl + createSalt());

    // Get the information
    std::string sInfo = sCurrentPackage.substr(sCurrentPackage.find("<info>"), sCurrentPackage.find("<endinfo>") - sCurrentPackage.find("<info>"));
    replaceAll(sInfo, "\t", " ");
    replaceAll(sInfo, "\n", " ");
    replaceAll(sInfo, "\r", " ");

    // Fill the package list
    std::string sPackageName = getTagValue(sInfo, "name");
    wxTreeItemId currPackage = m_listCtrl->AppendItem(m_listCtrl->GetRootItem(), sPackageName);
    m_listCtrl->SetItemText(currPackage, REPOCOLUMN, "v" + getTagValue(sInfo, "version"));
    m_listCtrl->SetItemBold(currPackage, true);
    m_listCtrl->SetItemImage(currPackage, PACKAGCOLUMN, m_icons->GetIconIndex("nscr"));

    wxTreeItemId currPackageInfo = m_listCtrl->AppendItem(currPackage, "Author");
    m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, getTagValue(sInfo, "author"));

    currPackageInfo = m_listCtrl->AppendItem(currPackage, "Type");
    m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, getTagValue(sInfo, "type").find("TYPE_PLUGIN") != std::string::npos ? "Plugin" : "Package");

    std::string sReqVersion = getTagValue(sInfo, "requireversion");

    if (sReqVersion.length())
    {
        currPackageInfo = m_listCtrl->AppendItem(currPackage, "Required NumeRe version");
        m_listCtrl->SetItemText(currPackageInfo, 1, "v" + sReqVersion);
    }

    std::string sDeps = getTagValue(sInfo, "requirepackages");

    if (sDeps.length())
    {
        currPackageInfo = m_listCtrl->AppendItem(currPackage, DEPENDENCIES);
        m_listCtrl->SetItemText(currPackageInfo, 1, sDeps);
    }

    currPackageInfo = m_listCtrl->AppendItem(currPackage, "Description");
    std::string sDesc = getTagValue(sInfo, "desc");

    if (!sDesc.length())
        sDesc = getTagValue(sInfo, "plugindesc");

    replaceAll(sDesc, "\\\"", "\"");

    if (!sDesc.length())
        sDesc = "[No description. Please provide a description using the \"desc=DESC\" install info field.]";

    m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, sDesc);
    m_listCtrl->SetItemToolTip(currPackageInfo, sDesc);
    m_listCtrl->SetItemToolTip(currPackage, sDesc);

    if (sDesc.front() == '[')
        m_listCtrl->SetItemTextColour(currPackageInfo, *wxRED);

    std::string sLicense = getTagValue(sInfo, "license");

    currPackageInfo = m_listCtrl->AppendItem(currPackage, "License");

    if (sLicense.length())
        m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, sLicense);
    else
    {
        m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, "[License unknown. Please provide a license using the \"license=LICENSE\" install info field.]");
        m_listCtrl->SetItemTextColour(currPackageInfo, *wxRED);
    }

    currPackageInfo = m_listCtrl->AppendItem(currPackage, REPO_URL);
    m_listCtrl->SetItemText(currPackageInfo, REPOCOLUMN, sUrl);
}


/////////////////////////////////////////////////
/// \brief Button event handler linked to the
/// "install/update" button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::OnInstall(wxCommandEvent& event)
{
    wxTreeItemId item = m_listCtrl->GetSelection();

    if (item.IsOk() && m_listCtrl->HasChildren(item))
    {
        m_vUrls.clear();
        std::vector<std::string> vDeps;
        m_vUrls.push_back(getUrl(item));
        m_fileNameToInstall = m_vUrls.back();
        m_fileNameToInstall.erase(0, m_fileNameToInstall.rfind('/')+1);
        m_installButton->Disable();
        m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "Downloading ...");
        m_listCtrl->SetItemBackgroundColour(item, INSTALLEDCOLOUR);

        std::string sDepList = getDependencies(item);

        if (sDepList.length())
        {
            resolveDependencies(sDepList, vDeps);

            for (const std::string& sDep : vDeps)
            {
                wxTreeItemId dep = findPackage(sDep + ".nscr");
                m_listCtrl->SetItemText(dep, INSTALLEDCOLUMN, "Downloading ...");
                m_listCtrl->SetItemBackgroundColour(dep, INSTALLEDCOLOUR);

                if (dep.IsOk())
                    m_vUrls.push_back(getUrl(dep));
            }
        }

        m_filesLoaded = false;
        StartThread(TASK_LOADFILES);
    }
}


/////////////////////////////////////////////////
/// \brief Button event handler linked to the
/// "uninstall" button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::OnUninstall(wxCommandEvent& event)
{
    wxTreeItemId item = m_listCtrl->GetSelection();

    if (item.IsOk())
    {
        wxString sName = m_listCtrl->GetItemText(item);
        m_terminal->pass_command("uninstall \"" + sName.ToStdString() + "\"", false);
        m_listCtrl->SetItemText(item, INSTALLEDCOLUMN, "");
        m_listCtrl->SetItemBackgroundColour(item, *wxWHITE);
        m_installButton->Enable();
        m_uninstallButton->Disable();
    }
}


/////////////////////////////////////////////////
/// \brief Item select event handler to enable or
/// disable the buttons depending on the state of
/// the package or whether the actual package
/// headline was selected.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::OnItemSelect(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    m_installButton->Disable();
    m_uninstallButton->Disable();

    if (m_task == TASK_NONE && item.IsOk() && m_listCtrl->HasChildren(item))
    {
        if (isInstallable(item) || isUpdateable(item))
            m_installButton->Enable();

        if (!isInstallable(item) || isUpdateable(item))
            m_uninstallButton->Enable();
    }
}


/////////////////////////////////////////////////
/// \brief Simple function to make the URL more
/// unique to avoid server caching (which might
/// resolve in a very delated update of the
/// package list as the server will respond with
/// the already cached contents rather than the
/// current file versions).
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageRepoBrowser::createSalt()
{
    return "?" + std::to_string(clock());
}


/////////////////////////////////////////////////
/// \brief Returns, whether the current item is
/// an installable package (not already installed).
///
/// \param item const wxTreeItemId&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepoBrowser::isInstallable(const wxTreeItemId& item)
{
    return item.IsOk() && m_listCtrl->GetItemBackgroundColour(item) == *wxWHITE;
}


/////////////////////////////////////////////////
/// \brief Returns, whether the current item is
/// an updateable package (already installed but
/// newer version available).
///
/// \param item const wxTreeItemId&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepoBrowser::isUpdateable(const wxTreeItemId& item)
{
    return item.IsOk() && m_listCtrl->GetItemBackgroundColour(item) == UPDATECOLOUR;
}


/////////////////////////////////////////////////
/// \brief Returns the value of the selected
/// identifier or an empty string, if the
/// identifier cannot be found.
///
/// \param item const wxTreeItemId&
/// \param sIdentifier const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageRepoBrowser::getEntry(const wxTreeItemId& item, const std::string& sIdentifier)
{
    if (!item.IsOk())
        return "";

    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_listCtrl->GetFirstChild(item, cookie);

    while (m_listCtrl->GetItemText(child) != sIdentifier)
    {
        child = m_listCtrl->GetNextSibling(child);

        if (!child.IsOk())
            return "";
    }

    return m_listCtrl->GetItemText(child, REPOCOLUMN).ToStdString();
}


/////////////////////////////////////////////////
/// \brief Return the package URL.
///
/// \param item const wxTreeItemId&
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageRepoBrowser::getUrl(const wxTreeItemId& item)
{
    return getEntry(item, REPO_URL);
}


/////////////////////////////////////////////////
/// \brief Return the package dependencies.
///
/// \param item const wxTreeItemId&
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageRepoBrowser::getDependencies(const wxTreeItemId& item)
{
    return getEntry(item, DEPENDENCIES);
}


/////////////////////////////////////////////////
/// \brief Find a package entry from its file
/// name. The returned ID must be checked for
/// validness via wxTreeItemId::IsOk() to detect,
/// if the package has been found.
///
/// \param sPackageFileName const std::string&
/// \return wxTreeItemId
///
/////////////////////////////////////////////////
wxTreeItemId PackageRepoBrowser::findPackage(const std::string& sPackageFileName)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_listCtrl->GetFirstChild(m_listCtrl->GetRootItem(), cookie);

    while (child.IsOk())
    {
        std::string sPackageUrl = getUrl(child);

        if (sPackageUrl.substr(sPackageUrl.rfind('/')) == "/" + sPackageFileName)
            return child;

        child = m_listCtrl->GetNextSibling(child);
    }

    return child;
}


/////////////////////////////////////////////////
/// \brief This member function resolves the
/// dependencies of a selected file using a
/// recursion. Only files, which are updateable
/// or installable are returned. Dependencies,
/// which are not part of the repository, are
/// ignored.
///
/// \param sDepList std::string
/// \param vDeps std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepoBrowser::resolveDependencies(std::string sDepList, std::vector<std::string>& vDeps)
{
    while (sDepList.length())
    {
        std::string sDep = getNextArgument(sDepList, true);
        wxTreeItemId item = findPackage(sDep + ".nscr");

        if ((isInstallable(item) || isUpdateable(item)) && std::find(vDeps.begin(), vDeps.end(), sDep) == vDeps.end())
        {
            vDeps.push_back(sDep);
            resolveDependencies(getDependencies(item), vDeps);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the contents of the passed URL
/// as a file and writes it to its counterpart in
/// the <SCRIPTPATH>/packages directory.
///
/// \param sUrl const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepoBrowser::getFileFromRepo(const std::string& sUrl)
{
    std::string contents = url::get(sUrl);
    std::string filename = sUrl;
    filename.erase(0, filename.rfind('/')+1);

    std::ofstream file((m_scriptPath + "/packages/" + filename).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);

    if (file.good())
    {
        file.write(contents.c_str(), contents.length());
        file.close();

        return true;
    }

    return false;
}














