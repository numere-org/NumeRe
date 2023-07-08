/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#include "controls/treedata.hpp"
#include "wx/dir.h"
#include <string>
#include <vector>
#include "IconManager.h"
#include "../common/datastructures.h"


class DirTraverser : public wxDirTraverser
{
    private:
        wxTreeCtrl* rootNode;
        IconManager* iconManager;
        wxTreeItemId id;
        FileFilterType fileSpec;
        wxString path;
        std::vector<wxTreeItemId> vcurrentnodes;
        size_t ncurrentdepth;
    public:
        DirTraverser(wxTreeCtrl* therootNode, IconManager* theiconmanager, wxTreeItemId theid, const wxString& thepath, FileFilterType thefilespec);

        virtual wxDirTraverseResult OnFile(const wxString& filename);
        virtual wxDirTraverseResult OnDir(const wxString& dirname);
};

