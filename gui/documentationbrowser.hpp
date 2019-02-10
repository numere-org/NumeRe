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

#ifndef DOCUMENTATIONBROWSER_HPP
#define DOCUMENTATIONBROWSER_HPP

#include <wx/treectrl.h>
#include <wx/splitter.h>
#include "viewerframe.hpp"
#include "helpviewer.hpp"
#include "IconManager.h"

class NumeReWindow;

class DocumentationBrowser : public ViewerFrame
{
    private:
        HelpViewer* m_viewer;
        wxTreeCtrl* m_doctree;
        IconManager* m_manager;

        void fillDocTree(NumeReWindow* mainwindow);

    public:
        DocumentationBrowser(wxWindow* parent, const wxString& titletemplate, NumeReWindow* mainwindow);
        ~DocumentationBrowser();

        bool SetStartPage(const wxString& docId);
        void OnTreeClick(wxTreeEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // DOCUMENTATIONBROWSER_HPP
