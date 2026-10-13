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

#include "tipdialog.hpp"
#include "../../kernel/core/utils/stringtools.hpp"

#include <wx/statline.h>
#include <wx/artprov.h>

#define wxID_NEXT_TIP 32000

BEGIN_EVENT_TABLE(TipDialog, wxDialog)
    EVT_BUTTON(wxID_NEXT_TIP, TipDialog::OnNextTip)
END_EVENT_TABLE()

/////////////////////////////////////////////////
/// \brief Construct the tip dialog window.
///
/// \param parent wxWindow*
/// \param tipProvider MyTipProvider*
/// \param text const wxArrayString&
/// \param showAtStartup bool
///
/////////////////////////////////////////////////
TipDialog::TipDialog(wxWindow* parent, MyTipProvider* tipProvider, const wxArrayString& text, bool showAtStartup)
    : wxDialog(GetParentForModalDialog(parent, 0), wxID_ANY, text[0], wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_textSnippets = text;
    m_tipProvider = tipProvider;

    // 1) create all controls in tab order
    m_header = new wxStaticText(this, wxID_ANY, m_textSnippets[1]);

    wxFont font = m_header->GetFont();
    font.SetPointSize(int(1.6 * font.GetPointSize()));
    //font.SetWeight(wxFONTWEIGHT_BOLD);
    m_header->SetFont(font);

    m_text = new TextField(this, wxID_ANY, wxEmptyString, wxSize(200, 160),
                           wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_READONLY);

    wxIcon icon = wxArtProvider::GetIcon(wxART_TIP, wxART_CMN_DIALOG);
    wxStaticBitmap* bmp = new wxStaticBitmap(this, wxID_ANY, icon);


    m_checkbox = new wxCheckBox(this, wxID_ANY, m_textSnippets[3]);
    m_checkbox->SetValue(showAtStartup);
    m_checkbox->SetFocus();

    wxButton* btnNext = new wxButton(this, wxID_NEXT_TIP, m_textSnippets[2]);
    wxButton* btnClose = new wxButton(this, wxID_CLOSE, m_textSnippets[4]);
    SetAffirmativeId(wxID_CLOSE);

    // 2) put them in boxes
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* icon_text = new wxBoxSizer(wxHORIZONTAL);
    icon_text->Add(bmp, 0, wxCENTER);
    icon_text->Add(m_header, 1, wxCENTER | wxLEFT, 10);

    topsizer->Add(icon_text, 0, wxEXPAND | wxALL, 10);
    topsizer->Add(m_text, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(m_checkbox, 0, wxCENTER);

    bottom->Add(10, 10, 1);
    bottom->Add(btnNext, 0, wxCENTER | wxLEFT, 10);
    bottom->Add(btnClose, 0, wxCENTER | wxLEFT, 10);

    topsizer->Add(bottom, 0, wxEXPAND | wxALL, 10);

    SetTipText();

    SetSizer(topsizer);

    topsizer->SetSizeHints(this);
    topsizer->Fit(this);

    Centre(wxBOTH | wxCENTER_FRAME);
}


/////////////////////////////////////////////////
/// \brief Update the displayed tip with the next
/// available tip.
///
/// \return void
///
/////////////////////////////////////////////////
void TipDialog::SetTipText()
{
    m_text->SetMarkupText(m_tipProvider->GetTip());
    m_text->SetInsertionPoint(0);

    auto index = m_tipProvider->getIndex();

    SetTitle(m_textSnippets[0] + " (" + index.first + "/" + index.second + ")");
    m_header->SetLabel(m_textSnippets[1] + " " + index.first);
}


/////////////////////////////////////////////////
/// \brief Simple helper function to open up the
/// tip dialog.
///
/// \param parent wxWindow*
/// \param tipProvider MyTipProvider*
/// \param text const wxArrayString&
/// \param showAtStartUp bool
/// \return bool
///
/////////////////////////////////////////////////
bool ShowTip(wxWindow* parent, MyTipProvider* tipProvider, const wxArrayString& text, bool showAtStartUp)
{
    TipDialog dlg(parent, tipProvider, text, showAtStartUp);
    dlg.ShowModal();

    return dlg.ShowTipsOnStartup();
}





/////////////////////////////////////////////////
/// \brief Construct a tip provider instance.
///
/// \param vTipList const std::vector<std::string>&
///
/////////////////////////////////////////////////
MyTipProvider::MyTipProvider(const std::vector<std::string>& vTipList) : wxTipProvider(vTipList.size())
{
    vTip = vTipList;

    for (std::string& tip : vTip)
    {
        replaceAll(tip, "\\n", "\n");
    }

    nth_tip = 0;
    // --> Einen Seed (aus der Zeit generiert) an die rand()-Funktion zuweisen <--
    srand(time(NULL));

    if (!vTip.size())
        return;
    // --> Die aktuelle Begruessung erhalten wir als modulo(nGreetings)-Operation auf rand() <--
    nth_tip = (rand() % vTip.size());
    if (nth_tip >= vTip.size())
        nth_tip = 0;
}

