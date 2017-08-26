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


#include "wx/wx.h"
#include "wx/dir.h"
#include <string>
#include <vector>
#include "IconManager.h"
#include "../common/datastructures.h"

using namespace std;
class wxTreeItemData;

class FileNameTreeData : public wxTreeItemData
{
    public:
        FileNameTreeData() : isDir(false), isFunction(false), isCommand(false) {};
        bool isDir;
        bool isFunction;
        bool isCommand;
        wxString filename;
        wxString tooltip;
};

class DirTraverser : public wxDirTraverser
{
    private:
        wxTreeCtrl* rootNode;
        IconManager* iconManager;
        wxTreeItemId id;
        FileFilterType fileSpec;
        wxString path;
        vector<wxTreeItemId> vcurrentnodes;
        unsigned int ncurrentdepth;
    public:
        DirTraverser(wxTreeCtrl* therootNode, IconManager* theiconmanager, wxTreeItemId theid, const wxString& thepath, FileFilterType thefilespec);

        virtual wxDirTraverseResult OnFile(const wxString& filename);
        virtual wxDirTraverseResult OnDir(const wxString& dirname);
};

