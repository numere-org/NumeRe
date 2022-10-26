/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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


#include "wx/treectrl.h"

/////////////////////////////////////////////////
/// \brief This class provides the basic
/// functionality to provide a tooltip for a tree
/// item.
/////////////////////////////////////////////////
class ToolTipTreeData : public wxTreeItemData
{
    public:
        ToolTipTreeData(const wxString& tip = wxEmptyString) : tooltip(tip) {};

        wxString tooltip;
};


/////////////////////////////////////////////////
/// \brief This class provides the needed
/// functionalities for the file tree and the
/// symbols tree.
/////////////////////////////////////////////////
class FileNameTreeData : public ToolTipTreeData
{
    public:
        FileNameTreeData() : ToolTipTreeData(), isDir(false), isFunction(false), isCommand(false), isConstant(false), isMethod(false) {};
        bool isDir;
        bool isFunction;
        bool isCommand;
        bool isConstant;
        bool isMethod;
        wxString filename;
};
