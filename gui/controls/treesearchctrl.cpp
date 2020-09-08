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

#include "treesearchctrl.hpp"


/////////////////////////////////////////////////
/// \brief This method searches and selects the
/// item with the passed label in the associated
/// tree.
///
/// \param value const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool TreeSearchCtrl::selectItem(const wxString& value)
{
    // Ensure that a tree as associated
    if (m_associatedCtrl)
    {
        // Get the root node and the first child
        wxTreeItemIdValue cookie;
        wxTreeItemId root = m_associatedCtrl->GetRootItem();
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(root, cookie);

        // If the child exists, try to find the passed string
        // in the labels of all childs
        if (child.IsOk())
        {
            wxTreeItemId match = findItem(value[0] == ' ' ? value.substr(1).Lower() : value.Lower(), child);

            // If a label was found, ensure it is visible
            // and select it
            if (match.IsOk())
            {
                m_associatedCtrl->EnsureVisible(match);
                m_associatedCtrl->SelectItem(match);

                return true;
            }
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This method searches for the tree
/// item, whose label corresponds to the passed
/// string.
///
/// \param value const wxString&
/// \param node wxTreeItemId
/// \return wxTreeItemId
///
/////////////////////////////////////////////////
wxTreeItemId TreeSearchCtrl::findItem(const wxString& value, wxTreeItemId node)
{
    // Go through all siblings
    do
    {
        // Return the current node, if it
        // corresponds to the passed string
        if (m_associatedCtrl->GetItemText(node).Lower() == value)
            return node;

        // Search the first child
        wxTreeItemIdValue cookie;
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(node, cookie);

        // If the child exists, try to find the
        // passed string in its or its siblings
        // labels
        if (child.IsOk())
        {
            wxTreeItemId match = findItem(value, child);

            // Return the id, if it exists
            if (match.IsOk())
                return match;
        }
    }
    while ((node = m_associatedCtrl->GetNextSibling(node)).IsOk());

    // Return an invalid tree item id, if
    // nothing had been found
    return wxTreeItemId();
}


/////////////////////////////////////////////////
/// \brief This method returns an array of
/// strings containing possible candidates for
/// the passed search string.
///
/// \param enteredText const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString TreeSearchCtrl::getCandidates(const wxString& enteredText)
{
    // Ensure that a tree control was associated
    if (m_associatedCtrl)
    {
        // Find root node and its child
        wxTreeItemIdValue cookie;
        wxTreeItemId root = m_associatedCtrl->GetRootItem();
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(root, cookie);

        // If the child exists, get all labels of its
        // siblings and their childs, which are candidates
        // for the passed string
        if (child.IsOk())
            return getChildCandidates(enteredText.Lower(), child);
    }

    // Return an empty string otherwise
    return wxArrayString();
}


/////////////////////////////////////////////////
/// \brief This method returns an array of
/// strings containing possible candiates for the
/// passed search string, which correspond to the
/// current tree node, its siblings and its
/// childs.
///
/// \param enteredText const wxString&
/// \param node wxTreeItemId
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString TreeSearchCtrl::getChildCandidates(const wxString& enteredText, wxTreeItemId node)
{
    wxArrayString stringArray;

    // Go through all siblings
    do
    {
        // Append the current label, if it contains the
        // searched string
        if (m_associatedCtrl->GetItemText(node).Lower().find(enteredText) != std::string::npos)
            stringArray.Add(m_associatedCtrl->GetItemText(node));

        // Find the first child of the current node
        wxTreeItemIdValue cookie;
        wxTreeItemId child = m_associatedCtrl->GetFirstChild(node, cookie);

        // If the child exists, find the candidates in
        // its siblings and childs
        if (child.IsOk())
        {
            wxArrayString childArray = getChildCandidates(enteredText, child);

            // Insert the candidates into the current
            // string array
            for (size_t i = 0; i < childArray.size(); i++)
                stringArray.Add(childArray[i]);
        }
    }
    while ((node = m_associatedCtrl->GetNextSibling(node)).IsOk());

    return stringArray;
}




