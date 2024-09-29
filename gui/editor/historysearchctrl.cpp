/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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


#include "historysearchctrl.hpp"
#include <vector>


BEGIN_EVENT_TABLE(HistorySearchCtrl, SearchCtrl)
    EVT_SIZE(HistorySearchCtrl::OnSizeEvent)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief If we select an item, we want to send
/// it to the terminal.
///
/// \param value const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool HistorySearchCtrl::selectItem(const wxString& value)
{
    if (m_history)
    {
        m_history->sendToTerminal(value);
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Get the search candidates by searching
/// through the (valid) lines of the history.
///
/// \param enteredText const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString HistorySearchCtrl::getCandidates(const wxString& enteredText)
{
    if (!m_history)
        return wxArrayString(1, &enteredText);

    wxArrayString entered = wxStringTokenize(enteredText);
    std::vector<wxString> stringArray;
    std::vector<size_t> matchWeights;
    std::vector<size_t> index;

    for (int i = 0; i < m_history->GetLineCount(); i++)
    {
        wxString line = m_history->GetLineText(i);

        // Ignore the header lines
        if (line.substr(0, 6) != "## ---" && std::find(stringArray.begin(), stringArray.end(), line) == stringArray.end())
        {
            size_t matchWeight = 0;

            for (size_t j = 0; j < entered.GetCount(); j++)
            {
                if (entered[j].length() < 3)
                    continue;

                if (line.find(entered[j]) != std::string::npos)
                    matchWeight += entered.GetCount() - j;
            }

            // Only add if enough weights
            if ((matchWeight && entered.GetCount() == 1)
                || matchWeight > entered.GetCount())
            {
                stringArray.push_back(line);
                index.push_back(matchWeights.size());
                matchWeights.push_back(matchWeight);
            }
        }
    }

    std::sort(index.begin(), index.end(), [&matchWeights](int a, int b){return matchWeights[a] > matchWeights[b];});
    wxArrayString candidates;

    for (size_t i = 0; i < stringArray.size(); i++)
    {
        candidates.Add(stringArray[index[i]]);
    }

    return candidates;
}


/////////////////////////////////////////////////
/// \brief Resize the shown columns to fit the
/// history parent's size.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void HistorySearchCtrl::OnSizeEvent(wxSizeEvent& event)
{
    wxArrayInt sizes;
    sizes.Add(event.GetSize().x-20, 1);
    popUp->SetColSizes(sizes);
    event.Skip();
}

