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

#ifndef PACKAGEDIALOG_HPP
#define PACKAGEDIALOG_HPP

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include "../terminal/terminal.hpp"
#include "../IconManager.h"
#include <set>
#include <string>

class PackageDialog : public wxDialog
{
    private:
        wxPropertyGrid* m_packageProperties;
        wxListView* m_fileList;
        NumeReTerminal* m_terminal;
        IconManager* m_icons;

        void OnAutoDetect(wxCommandEvent& event);
        void OnAddItems(wxCommandEvent& event);
        void OnRemoveItems(wxCommandEvent& event);

        void autoDetect(const wxArrayString& mainfiles);
        void followBranch(const std::string& sFile, std::set<std::string>& fileSet);

    public:
        PackageDialog(wxWindow* parent, NumeReTerminal* terminal, IconManager* icons);

        void setMainFile(const wxString& mainfile);
        wxArrayString getProcedures();
        wxString getInstallInfo();
        wxString getPackageName();
        wxString getPackageIdentifier();
        bool includeDocs();
        bool isPlugin();

        DECLARE_EVENT_TABLE()
};

#endif // PACKAGEDIALOG_HPP


