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


#include "duplicatecodedialog.hpp"
#include "../../kernel/core/language.hpp"
#include "../editor/editor.h"

#define ID_START 12000

extern Language _guilang;

int StrToInt(const string&);

BEGIN_EVENT_TABLE(DuplicateCodeDialog, ViewerFrame)
    EVT_BUTTON  (wxID_OK, DuplicateCodeDialog::OnButtonOK)
    EVT_BUTTON  (ID_START, DuplicateCodeDialog::OnButtonStart)
    EVT_LIST_ITEM_SELECTED (-1, DuplicateCodeDialog::OnItemClick)
END_EVENT_TABLE()

DuplicateCodeDialog::DuplicateCodeDialog(wxWindow* _parent, const wxString& title) : ViewerFrame(_parent, title)
{
    m_parent = _parent;
    m_mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition);

    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* checkBox = new wxStaticBoxSizer(wxHORIZONTAL, m_mainPanel, _guilang.get("GUI_DUPCODE_SETTINGS"));

    m_varSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_VARSEMANTICS"));
    m_StringSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_STRINGSEMANTICS"));
    m_NumSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_NUMSEMANTICS"));

    checkBox->Add(m_varSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    checkBox->Add(m_StringSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    checkBox->Add(m_NumSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    wxButton* buttonStart = new wxButton(m_mainPanel, ID_START, _guilang.get("GUI_DUPCODE_START"));
    wxButton* buttonOK = new wxButton(m_mainPanel, wxID_OK);

    hSizer->Add(buttonStart, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    hSizer->Add(buttonOK, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    m_resultList = new wxListCtrl(m_mainPanel,  wxID_ANY, wxDefaultPosition, wxSize(400, 400), wxLC_REPORT);
    m_resultList->AppendColumn(_guilang.get("GUI_DUPCODE_MATCH"));
    m_resultList->AppendColumn(_guilang.get("GUI_DUPCODE_PERCENTAGE"));

    m_progressGauge = new wxGauge(m_mainPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1,24), wxHORIZONTAL);

    vSizer->Add(m_resultList, 2, wxEXPAND | wxALL, 5);
    vSizer->Add(m_progressGauge, 0, wxEXPAND | wxALL, 5);
    vSizer->Add(checkBox, 0, wxEXPAND | wxALL, 5);
    vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    m_mainPanel->SetSizer(vSizer);

    m_progressGauge->SetValue(0);
    m_varSemantics->SetValue(true);

    this->SetSize(420,400);
}


void DuplicateCodeDialog::SetProgress(double dPercentage)
{
    if ((int)(dPercentage) != m_progressGauge->GetValue())
        m_progressGauge->SetValue((int)(dPercentage));
}

void DuplicateCodeDialog::SetResult(const vector<string>& vResult)
{
    for (size_t i = 0; i < vResult.size(); i++)
    {
        m_resultList->InsertItem(i, vResult[i].substr(0,vResult[i].find('[')));
        m_resultList->SetItem(i, 1, vResult[i].substr(vResult[i].find('[')));
    }
    m_resultList->SetColumnWidth(0, 200);
    m_resultList->SetColumnWidth(1, 160);
}

void DuplicateCodeDialog::OnButtonOK(wxCommandEvent& event)
{
    Close();
    NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
    edit->IndicateDuplicatedLine(-1,-1,-1,-1);
}

void DuplicateCodeDialog::OnButtonStart(wxCommandEvent& event)
{
    CallAfter(DuplicateCodeDialog::OnStart);
}

void DuplicateCodeDialog::OnItemClick(wxListEvent& event)
{
    string sItemText = event.GetText().ToStdString();
    int nStart1, nEnd1, nStart2, nEnd2;

    nStart1 = StrToInt(sItemText.substr(0,sItemText.find('-')));
    nEnd1 = StrToInt(sItemText.substr(sItemText.find('-')+1, sItemText.find(' ')-sItemText.find('-')-1));
    sItemText.erase(0,sItemText.find("== ")+3);
    nStart2 = StrToInt(sItemText.substr(0,sItemText.find('-')));
    nEnd2 = StrToInt(sItemText.substr(sItemText.find('-')+1));

    NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
    edit->IndicateDuplicatedLine(nStart1-1, nEnd1-1, nStart2-1, nEnd2-1);
}

void DuplicateCodeDialog::OnStart()
{
    int nFlags = 0;
    if (m_varSemantics->IsChecked())
        nFlags |= 1;
    if (m_StringSemantics->IsChecked())
        nFlags |= 2;
    if (m_NumSemantics->IsChecked())
        nFlags |= 4;
    m_resultList->DeleteAllItems();
    NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
    edit->OnFindDuplicateCode(nFlags);
}
