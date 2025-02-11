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

#ifndef DEFAULTPAGE_HPP
#define DEFAULTPAGE_HPP

#include "grouppanel.hpp"
#include <vector>

class IconManager;
class RecentFilesManager;

/////////////////////////////////////////////////
/// \brief This class represents the default page
/// presented to all users with no other files
/// open in the editor.
/////////////////////////////////////////////////
class DefaultPage : public GroupPanel
{
    private:
        IconManager* m_icons;
        TextField* m_rightBox;
        TextField* m_bottomBox;
        wxListView* m_recentFiles;

        void resizeColumns();

    public:
        DefaultPage(wxWindow* parent, const wxArrayString& searchPaths, bool showReleaseNotes);
        void setIconManager(IconManager* icons);
        void setRecentFiles(const RecentFilesManager& recentFiles);
        void loadLatestRelease();

        void OnCommandEvent(wxCommandEvent& event);
        void OnListItemActivate(wxListEvent& event);
        void OnSizeEvent(wxSizeEvent& event);

    DECLARE_EVENT_TABLE();
};

#endif // DEFAULTPAGE_HPP

