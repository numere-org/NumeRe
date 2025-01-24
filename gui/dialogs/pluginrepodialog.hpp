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

#ifndef PLUGINREPODIALOG_HPP
#define PLUGINREPODIALOG_HPP

#define PACKAGE_REPO_BROWSER_TITLE "NumeRe: Package Manager"

#include <wx/wx.h>
#include <wx/thread.h>
#include "../controls/treelistctrl.h"
#include "../compositions/viewerframe.hpp"
#include "../terminal/terminal.hpp"
#include "../../network/packagerepo.hpp"
#include <string>
#include <vector>


/////////////////////////////////////////////////
/// \brief This class represents a simple
/// browser for the package repository with an
/// install and uninstall capability.
/////////////////////////////////////////////////
class PackageRepoBrowser : public ViewerFrame, public wxThreadHelper
{
    private:
        NumeReTerminal* m_terminal;
        IconManager* m_icons;
        std::string m_scriptPath;
        std::string m_fileNameToInstall;
        PackageRepo m_repo;

        wxcode::wxTreeListCtrl* m_listCtrl;
        wxButton* m_installButton;
        wxButton* m_uninstallButton;

        // Threading part
        wxGauge* m_progress;
        wxStaticText* m_statusText;

        enum ThreadTask
        {
            TASK_NONE,
            TASK_LOADREPO,
            TASK_LOADFILES
        };

        ThreadTask m_task;
        bool m_filesLoaded;
        std::vector<std::string> m_vUrls;

        void StartThread(ThreadTask task);
        virtual wxThread::ExitCode Entry();
        void OnThreadUpdate(wxThreadEvent& event);
        void OnClose(wxCloseEvent& event);
        // End threading part

        void populatePackageList(const std::string& sUrl);

        void OnInstall(wxCommandEvent& event);
        void OnUninstall(wxCommandEvent& event);
        void OnItemSelect(wxTreeEvent& event);

        bool isInstallable(const wxTreeItemId& item);
        bool isUpdateable(const wxTreeItemId& item);
        std::string getEntry(const wxTreeItemId& item, const std::string& sIdentifier);
        std::string getUrl(const wxTreeItemId& item);
        std::string getDependencies(const wxTreeItemId& item);
        wxTreeItemId findPackage(const std::string& sPackageFileName);
        void resolveDependencies(std::string sDepList, std::vector<std::string>& vDeps);

        bool getFileFromRepo(const std::string& sUrl);

    public:
        PackageRepoBrowser(wxWindow* parent, NumeReTerminal* terminal, IconManager* icons);
        void DetectInstalledPackages();

        DECLARE_EVENT_TABLE()
};


#endif // PLUGINREPODIALOG_HPP

