/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#include "toolbarsearchctrl.hpp"
#include "../NumeReWindow.h"
#include "../terminal/terminal.hpp"
#include "../editor/editor.h"

#include <wx/event.h>

/////////////////////////////////////////////////
/// \brief This member function performs the
/// actions necessary, when the user selects an
/// item in the popup list (e.g. via double
/// click).
///
/// \param value const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool ToolBarSearchCtrl::selectItem(const wxString& value)
{
    size_t record = searchDB.findRecord(value.ToStdString());

    if (record == std::string::npos)
        return false;

    std::string sTermInput = searchDB.getElement(record, 3);

    if (sTermInput.substr(0, 5) == "help ")
        m_mainframe->ShowHelp(sTermInput.substr(5));
    else if (sTermInput.substr(0, 1) == ".")
        m_mainframe->ShowHelp(sTermInput.substr(1));
    else if (sTermInput.front() != '"' && sTermInput.find("(") != std::string::npos)
        m_mainframe->ShowHelp(sTermInput);
    else
    {
        NumeReEditor* edit = m_mainframe->GetCurrentEditor();

        if (edit)
        {
            edit->InsertText(edit->GetCurrentPos(), sTermInput);
            edit->GotoPos(edit->GetCurrentPos()+sTermInput.length());
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function provides the text
/// to be inserted, when the user drag-drops one
/// of the results. This is different from the
/// inserted text, when the user selects an item
/// in the list.
///
/// \param value const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString ToolBarSearchCtrl::getDragDropText(const wxString& value)
{
    size_t record = searchDB.findRecord(value.ToStdString());

    if (record == std::string::npos)
        return "";

    wxString text = searchDB.getElement(record, 3);

    if (text.substr(0, 5) == "help ")
        text.erase(0, text.find_first_not_of(' ', 4));

    if (text.find(" ... ") != std::string::npos)
        text.Replace(" ... ", "\n\t\n");

    return text;
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
wxArrayString ToolBarSearchCtrl::getCandidates(const wxString& enteredText)
{
    wxArrayString candidates;

    // Find the matches in the database
    std::map<double,std::vector<size_t>> matches = searchDB.findRecordsUsingRelevance(enteredText.ToStdString(), std::vector<double>({3.0, 2.0, 1.0}));

    if (!matches.size())
        return candidates;

    // Format the results for presenting in the
    // popup of the search control
    for (auto iter = matches.rbegin(); iter != matches.rend(); ++iter)
    {
        for (size_t i = 0; i < iter->second.size(); i++)
            candidates.Add(searchDB.getElement(iter->second[i], 0) + "~" + searchDB.getElement(iter->second[i], 1));
    }

    return candidates;
}


