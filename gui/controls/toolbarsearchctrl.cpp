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

std::string toString(double, int);

void ToolBarSearchCtrl::selectItem(const wxString& value)
{
    //
}

wxArrayString ToolBarSearchCtrl::getCandidates(const wxString& enteredText)
{
    wxArrayString candidates;

    std::map<double,std::vector<size_t>> matches = searchDB.findRecordsUsingRelevance(enteredText.ToStdString(), std::vector<double>({3.0, 2.0, 1.0}));

    if (!matches.size())
        return candidates;

    double dMax = matches.rbegin()->first;

    for (auto iter = matches.rbegin(); iter != matches.rend(); ++iter)
    {
        if ((iter->first / dMax) < 0.66 || candidates.size() >= 10)
            break;

        for (size_t i = 0; i < iter->second.size(); i++)
            candidates.Add(" " + searchDB.getElement(iter->second[i], 0) + ":   " + searchDB.getElement(iter->second[i], 1));
    }

    return candidates;
}


