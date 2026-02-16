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

#ifndef TABLEBROWSER_HPP
#define TABLEBROWSER_HPP

#include <vector>

#include "viewerframe.hpp"
#include "viewerbook.hpp"

#include "../../kernel/core/datamanagement/container.hpp"
#include "../../kernel/core/datamanagement/table.hpp"

class NumeReWindow;


/////////////////////////////////////////////////
/// \brief This class implements a tabbed table
/// viewer called DataViewer in app.
/////////////////////////////////////////////////
class TableBrowser : public ViewerFrame
{
    private:
        enum PanelType
        {
            TYPE_TABLEVIEWER,
            TYPE_TABLEPANEL
        };

        ViewerBook* m_tabs;
        NumeReWindow* m_topWindow;
        std::vector<PanelType> m_panelTypes;

        void createMenuBar();
        int findPage(const std::string& tableDisplayName, const std::string& sIntName);

    public:
        TableBrowser(wxWindow* parent, NumeReWindow* topWindow);

        void openTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName, const std::string& sIntName);
        void openTable(NumeRe::Table _table, const std::string& tableDisplayName, const std::string& sIntName);

        void onPageChange(wxAuiNotebookEvent& event);
        void onPageClose(wxAuiNotebookEvent& event);
        void onMenuEvent(wxCommandEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // TABLEBROWSER_HPP

