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
#include <wx/notebook.h>
#include "compositions/viewerframe.hpp"
#include "compositions/viewerbook.hpp"
#include "IconManager.h"

class NumeReWindow;

/////////////////////////////////////////////////
/// \brief This represents the main frame of the
/// documentation browser, which contains the
/// tabbed layout, the index tree and the
/// documentation viewers themselves.
/////////////////////////////////////////////////
class DocumentationBrowser : public ViewerFrame
{
    private:
        wxTreeCtrl* m_doctree;
        IconManager* m_manager;
        ViewerBook* m_docTabs;
        wxString m_titleTemplate;

        void prepareToolbar();
        void fillDocTree(NumeReWindow* mainwindow);
        bool findAndSelectPage(const wxString& title);

    public:
        DocumentationBrowser(wxWindow* parent, const wxString& titletemplate, NumeReWindow* mainwindow);
        ~DocumentationBrowser();

        bool SetStartPage(const wxString& docId);
        void OnTreeClick(wxTreeEvent& event);
        void OnToolbarEvent(wxCommandEvent& event);

        bool createNewPage(const wxString& docId);
        void setCurrentTabText(const wxString& text);
        void onPageChange(wxAuiNotebookEvent& event);
        void onPageClose(wxAuiNotebookEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // DOCUMENTATIONBROWSER_HPP


