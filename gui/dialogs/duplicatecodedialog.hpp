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

#ifndef DUPLICATECODEDIALOG_HPP
#define DUPLICATECODEDIALOG_HPP

#include "../viewerframe.hpp"
#include <wx/panel.h>
#include <wx/gauge.h>
#include <wx/listctrl.h>
#include <string>
#include <vector>
using namespace std;

class DuplicateCodeDialog : public ViewerFrame
{
    private:
        wxGauge* m_progressGauge;
        wxPanel* m_mainPanel;
        wxListCtrl* m_resultList;
        wxWindow* m_parent;
        wxCheckBox* m_varSemantics;
        wxCheckBox* m_StringSemantics;
        wxCheckBox* m_NumSemantics;
        wxCheckBox* m_FunctionSemantics;

        wxString createTextFromList();

    public:
        DuplicateCodeDialog(wxWindow* _parent, const wxString& title);

        void SetProgress(double dPercentage);
        void SetResult(const vector<string>& vResult);
        void OnButtonOK(wxCommandEvent& event);
        void OnButtonStart(wxCommandEvent& event);
        void OnButtonCopy(wxCommandEvent& event);
        void OnButtonReport(wxCommandEvent& event);
        void OnItemClick(wxListEvent& event);

        void OnStart();

        DECLARE_EVENT_TABLE();
};

#endif // DUPLICATECODEDIALOG_HPP

