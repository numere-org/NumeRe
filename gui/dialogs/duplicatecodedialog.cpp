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
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../editor/editor.h"
#include <wx/clipbrd.h>
#include <algorithm>

#define ID_DUPLICATECODE_START 12000
#define ID_DUPLICATECODE_COPY 12001
#define ID_DUPLICATECODE_REPORT 12002
#define SEMANTICS_VAR 1
#define SEMANTICS_STRING 2
#define SEMANTICS_NUM 4
#define SEMANTICS_FUNCTION 8

extern Language _guilang;

struct compare_index
{
	const vector<double> base_arr;
	compare_index (const vector<double>& arr) : base_arr (arr)
	{}

	bool operator () (int a, int b) const
	{
		return (base_arr[a] < base_arr[b]);
	}
};


BEGIN_EVENT_TABLE(DuplicateCodeDialog, ViewerFrame)
	EVT_BUTTON                  (wxID_OK, DuplicateCodeDialog::OnButtonOK)
	EVT_BUTTON                  (ID_DUPLICATECODE_START, DuplicateCodeDialog::OnButtonStart)
	EVT_BUTTON                  (ID_DUPLICATECODE_COPY, DuplicateCodeDialog::OnButtonCopy)
	EVT_BUTTON                  (ID_DUPLICATECODE_REPORT, DuplicateCodeDialog::OnButtonReport)
	EVT_CLOSE                   (DuplicateCodeDialog::OnClose)
	EVT_LIST_ITEM_SELECTED      (-1, DuplicateCodeDialog::OnItemClick)
	EVT_LIST_ITEM_RIGHT_CLICK   (-1, DuplicateCodeDialog::OnItemRightClick)
	EVT_LIST_COL_CLICK          (wxID_ANY, DuplicateCodeDialog::OnColumnHeaderClick)
END_EVENT_TABLE()

DuplicateCodeDialog::DuplicateCodeDialog(wxWindow* _parent, const wxString& title) : ViewerFrame(_parent, title)
{
	m_parent = _parent;
	m_mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition);

	wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* paramSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer* checkBox = new wxStaticBoxSizer(wxVERTICAL, m_mainPanel, _guilang.get("GUI_DUPCODE_SETTINGS"));

	m_varSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_VARSEMANTICS"));
	m_StringSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_STRINGSEMANTICS"));
	m_NumSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_NUMSEMANTICS"));
	m_FunctionSemantics = new wxCheckBox(checkBox->GetStaticBox(), wxID_ANY, _guilang.get("GUI_DUPCODE_FUNCTIONSEMANTICS"));
	wxStaticText* spinctrlLabel = new wxStaticText(m_mainPanel, wxID_ANY, _guilang.get("GUI_DUPCODE_NUMLINES"));
	m_NumLines = new wxSpinCtrl(m_mainPanel, wxID_ANY, "6", wxDefaultPosition, wxDefaultSize, 0x4000 | wxALIGN_RIGHT, 3, 100, 6);

	checkBox->Add(m_varSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	checkBox->Add(m_StringSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	checkBox->Add(m_NumSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	checkBox->Add(m_FunctionSemantics, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	paramSizer->Add(checkBox, 0, wxEXPAND | wxALL, 5);
	paramSizer->Add(m_NumLines, 1, wxALIGN_LEFT | wxALL, 5);
	paramSizer->Add(spinctrlLabel, 0, wxEXPAND | wxALL, 5);

	wxButton* buttonStart = new wxButton(m_mainPanel, ID_DUPLICATECODE_START, _guilang.get("GUI_DUPCODE_START"));
	wxButton* buttonCopy = new wxButton(m_mainPanel, ID_DUPLICATECODE_COPY, _guilang.get("GUI_DUPCODE_COPY"));
	wxButton* buttonReport = new wxButton(m_mainPanel, ID_DUPLICATECODE_REPORT, _guilang.get("GUI_DUPCODE_REPORT"));
	wxButton* buttonOK = new wxButton(m_mainPanel, wxID_OK);

	hSizer->Add(buttonStart, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	hSizer->Add(buttonCopy, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	hSizer->Add(buttonReport, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	hSizer->Add(buttonOK, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_resultList = new wxListCtrl(m_mainPanel,  wxID_ANY, wxDefaultPosition, wxSize(400, 400), wxLC_REPORT);
	m_resultList->AppendColumn(_guilang.get("GUI_DUPCODE_MATCH"));
	m_resultList->AppendColumn(_guilang.get("GUI_DUPCODE_PERCENTAGE"));
	m_resultList->AppendColumn(_guilang.get("GUI_DUPCODE_LINES"));

	m_progressGauge = new wxGauge(m_mainPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 24), wxHORIZONTAL);

	vSizer->Add(m_resultList, 2, wxEXPAND | wxALL, 5);
	vSizer->Add(m_progressGauge, 0, wxEXPAND | wxALL, 5);
	vSizer->Add(paramSizer, 0, wxEXPAND | wxALL, 5);
	vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5);

	m_mainPanel->SetSizer(vSizer);

	m_progressGauge->SetValue(0);
	m_varSemantics->SetValue(true);

	this->SetSize(480, 500);
}


wxString DuplicateCodeDialog::createTextFromList()
{
	wxString sText;
	for (int i = 0; i < m_resultList->GetItemCount(); i++)
	{
		sText += m_resultList->GetItemText(i) + "\t" + m_resultList->GetItemText(i, 1) + "\t" + m_resultList->GetItemText(i, 2) + "\n";
	}
	if (sText.length())
		return this->GetTitle() + "\n" + _guilang.get("GUI_DUPCODE_MATCH") + "\t" + _guilang.get("GUI_DUPCODE_PERCENTAGE") + "\t" + _guilang.get("GUI_DUPCODE_LINES") + "\n" + sText;
	return sText;
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
		m_resultList->InsertItem(i, vResult[i].substr(0, vResult[i].find('[')));
		string itemtext = m_resultList->GetItemText(i).ToStdString();
		int line1 = atoi(itemtext.substr(0, itemtext.find('-')).c_str());
		int line2 = atoi(itemtext.substr(itemtext.find('-') + 1, itemtext.find('=') - itemtext.find('-') - 1).c_str());
		m_resultList->SetItem(i, 1, vResult[i].substr(vResult[i].find('[')));
		m_resultList->SetItem(i, 2, toString(line2 - line1 + 1));
	}
	m_resultList->SetColumnWidth(0, 200);
	m_resultList->SetColumnWidth(1, 120);
	m_resultList->SetColumnWidth(2, 120);
}

void DuplicateCodeDialog::OnButtonOK(wxCommandEvent& event)
{
	Close();
	NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
	edit->IndicateDuplicatedLine(-1, -1, -1, -1, -1);
}

void DuplicateCodeDialog::OnClose(wxCloseEvent& event)
{
	NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
	edit->notifyDialogClose();
	this->Destroy();
}

void DuplicateCodeDialog::OnButtonStart(wxCommandEvent& event)
{
	CallAfter(DuplicateCodeDialog::OnStart);
}

void DuplicateCodeDialog::OnButtonCopy(wxCommandEvent& event)
{
	wxString sSelection = createTextFromList();
	if (!sSelection.length())
		return;
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(sSelection));
		wxTheClipboard->Close();
	}
}

void DuplicateCodeDialog::OnButtonReport(wxCommandEvent& event)
{
	wxString sSelection = createTextFromList();
	if (!sSelection.length())
		return;

	wxFileDialog filedialog(this, _guilang.get("GUI_DUPCODE_SAVEREPORT"), wxEmptyString, "duplicatecodereport.txt", _guilang.get("COMMON_FILETYPE_TXT") + " (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (filedialog.ShowModal() == wxID_CANCEL)
		return;
	wxString filename = filedialog.GetPath();
	wxFile file(filename, wxFile::write);
	if (file.IsOpened())
		file.Write(sSelection);
	file.Close();
}

void DuplicateCodeDialog::OnItemClick(wxListEvent& event)
{
	this->highlightSelection(event.GetText(), true);
}

void DuplicateCodeDialog::OnItemRightClick(wxListEvent& event)
{
	this->highlightSelection(event.GetText(), false);
}

void DuplicateCodeDialog::highlightSelection(const wxString& sSelection, bool firstMatch)
{
	string sItemText = sSelection.ToStdString();
	int nStart1, nEnd1, nStart2, nEnd2, nSelection;

	nStart1 = StrToInt(sItemText.substr(0, sItemText.find('-')));
	nEnd1 = StrToInt(sItemText.substr(sItemText.find('-') + 1, sItemText.find(' ') - sItemText.find('-') - 1));
	sItemText.erase(0, sItemText.find("== ") + 3);
	nStart2 = StrToInt(sItemText.substr(0, sItemText.find('-')));
	nEnd2 = StrToInt(sItemText.substr(sItemText.find('-') + 1));

	if (firstMatch)
		nSelection = nStart1 - 1;
	else
		nSelection = nStart2 - 1;

	NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
	edit->IndicateDuplicatedLine(nStart1 - 1, nEnd1 - 1, nStart2 - 1, nEnd2 - 1, nSelection);
}

void DuplicateCodeDialog::OnStart()
{
	int nFlags = 0;
	if (m_varSemantics->IsChecked())
		nFlags |= SEMANTICS_VAR;
	if (m_StringSemantics->IsChecked())
		nFlags |= SEMANTICS_STRING;
	if (m_NumSemantics->IsChecked())
		nFlags |= SEMANTICS_NUM;
	if (m_FunctionSemantics->IsChecked())
		nFlags |= SEMANTICS_FUNCTION;
	m_resultList->DeleteAllItems();
	NumeReEditor* edit = static_cast<NumeReEditor*>(m_parent);
	edit->OnFindDuplicateCode(nFlags, m_NumLines->GetValue());
}

void DuplicateCodeDialog::OnColumnHeaderClick(wxListEvent& event)
{
	int nCol = event.GetColumn();

	if (!m_resultList->GetItemCount())
		return;

	vector<double> vData;
	vector<int> vIndex;
	vector<wxString> vStringData;

	string sCurrentItem;

	for (int i = 0; i < m_resultList->GetItemCount(); i++)
	{
		sCurrentItem = m_resultList->GetItemText(i, nCol).ToStdString();
		vIndex.push_back(vData.size());
		switch (nCol)
		{
			case 0:
				while (sCurrentItem.find_first_of("-= ") != string::npos)
					sCurrentItem.erase(sCurrentItem.find_first_of("-= "), 1);
				vData.push_back(StrToDb(sCurrentItem));
				break;
			case 1:
				vData.push_back(StrToDb(sCurrentItem.substr(sCurrentItem.find('[') + 1, sCurrentItem.find(']') - sCurrentItem.find('[') - 1)));
				break;
			case 2:
				vData.push_back(StrToDb(sCurrentItem));
				break;
		}
	}

	std::stable_sort(vIndex.begin(), vIndex.end(), compare_index(vData));

	for (int j = 0; j < m_resultList->GetColumnCount(); j++)
	{
		for (int i = 0; i < m_resultList->GetItemCount(); i++)
		{
			vStringData.push_back(m_resultList->GetItemText(i, j));
		}
		for (int i = 0; i < m_resultList->GetItemCount(); i++)
		{
			m_resultList->SetItem(i, j, vStringData[vIndex[i]]);
		}
		vStringData.clear();
	}
}


