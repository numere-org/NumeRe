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

#ifndef TREESEARCHCTRL_HPP
#define TREESEARCHCTRL_HPP

#include "searchctrl.hpp"
#include <wx/wx.h>
#include <wx/treectrl.h>
#include "treelistctrl.h"


/////////////////////////////////////////////////
/// \brief This class specializes the generic
/// search control to interact with a wxTreeCtrl.
/////////////////////////////////////////////////
class TreeSearchCtrl : public SearchCtrl
{
    private:
        wxTreeCtrl* m_associatedCtrl;
        bool m_searchInToolTip;
        bool m_isFileTree;

    protected:
        // Interaction functions with the wxTreeCtrl
        virtual bool selectItem(const wxString& value) override;
        wxTreeItemId findItem(const wxString& value, wxTreeItemId node);
        virtual wxArrayString getCandidates(const wxString& enteredText) override;
        wxArrayString getChildCandidates(const wxString& enteredText, wxTreeItemId node);

    public:
        TreeSearchCtrl(wxWindow* parent, wxWindowID id, const wxString& hint = wxEmptyString, const wxString& calltip = wxEmptyString, wxTreeCtrl* associatedCtrl = nullptr, bool searchInToolTip = false, bool isFileTree = false) : SearchCtrl(parent, id, wxEmptyString), m_associatedCtrl(associatedCtrl), m_searchInToolTip(searchInToolTip), m_isFileTree(isFileTree)
        {
            // Provide a neat hint to the user, what he
            // may expect from this control
            SetHint(hint);
            popUp->SetCallTips(calltip);

            if (m_isFileTree)
            {
                // Define the number of columns
                wxArrayInt sizes;
                sizes.Add(1, 1);
                sizes.Add(250, 1);

                popUp->SetColSizes(sizes);
            }
        }

};

#endif // TREESEARCHCTRL_HPP


