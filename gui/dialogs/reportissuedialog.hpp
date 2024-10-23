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

#ifndef REPORTISSUEDIALOG_HPP
#define REPORTISSUEDIALOG_HPP

#include <wx/wx.h>
#include "../../common/datastructures.h"
#include "../compositions/grouppanel.hpp"

/////////////////////////////////////////////////
/// \brief This dialog shows a form to fill and
/// send as a GitHub issue via the GitJub API.
/////////////////////////////////////////////////
class ReportIssueDialog : public wxDialog
{
    private:
        wxChoice* m_issueType;
        wxButton* m_okButton;
        TextField* m_issueTitle;
        TextField* m_issueDescription;
        TextField* m_issueReproduction;
        TextField* m_expectedBehavior;
        TextField* m_contact;
        wxRadioBox* m_logSelection;
        bool m_isCriticalIssue;

        void OnButtonClick(wxCommandEvent& event);
        void OnDropDown(wxCommandEvent& event);
        void fillCrashData();
        void fillStartupData();

    public:
        ReportIssueDialog(wxWindow* parent, wxWindowID id = wxID_ANY, ErrorLocation errLoc = ERR_NONE);

        DECLARE_EVENT_TABLE();
};


#endif // REPORTISSUEDIALOG_HPP

