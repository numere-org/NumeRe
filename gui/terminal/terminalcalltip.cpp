/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#include "terminalcalltip.hpp"
#include <wx/panel.h>


/////////////////////////////////////////////////
/// \brief TerminalCallTip constructor.
///
/// \param parent wxWindow*
/// \param s const wxSize&
///
/////////////////////////////////////////////////
TerminalCallTip::TerminalCallTip(wxWindow* parent, const wxSize& s)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, s+wxSize(2,2), wxBORDER_SIMPLE | wxCLIP_CHILDREN)
{
    SetBackgroundColour(wxColour(245, 245, 245));

    m_text = new wxTextCtrl(this, wxID_ANY, "STANDARDTEXT", wxDefaultPosition, s, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxBORDER_NONE);

    m_text->SetFont(wxFont(8, wxMODERN, wxNORMAL, wxNORMAL, false, "Consolas"));
    m_text->SetBackgroundColour(wxColour(245, 245, 245));

    Hide();
}


/////////////////////////////////////////////////
/// \brief Show the calltip at the desired
/// position. This function does not check,
/// whether the calltip is acutally visible.
///
/// \param pos const wxPoint&
/// \param text const wxString&
/// \return void
///
/////////////////////////////////////////////////
void TerminalCallTip::PopUp(const wxPoint& pos, const wxString& text)
{
    m_text->Clear();
    m_text->SetDefaultStyle(wxTextAttr(wxColour(128,128,128)));
    m_text->AppendText(text);
    SetPosition(pos-wxPoint(2,0));

    if (!IsShown())
        Show();
}


/////////////////////////////////////////////////
/// \brief Remove the shown calltip.
///
/// \return void
///
/////////////////////////////////////////////////
void TerminalCallTip::Dismiss()
{
    Hide();
}


/////////////////////////////////////////////////
/// \brief Change the calltip's size.
///
/// \param s const wxSize&
/// \return void
///
/////////////////////////////////////////////////
void TerminalCallTip::Resize(const wxSize& s)
{
    SetSize(s+wxSize(2,2));
    m_text->SetSize(s);
}


/////////////////////////////////////////////////
/// \brief Hightlight the section of the text in
/// the calltip.
///
/// \param start size_t
/// \param len size_t
/// \return void
///
/////////////////////////////////////////////////
void TerminalCallTip::Highlight(size_t start, size_t len)
{
    wxString text = m_text->GetValue();
    m_text->Clear();
    m_text->SetDefaultStyle(wxTextAttr(wxColour(128, 128, 128)));
    m_text->AppendText(text.substr(0, start));
    m_text->SetDefaultStyle(wxTextAttr(*wxBLUE));
    m_text->AppendText(text.substr(start, len));
    m_text->SetDefaultStyle(wxTextAttr(wxColour(128, 128, 128)));
    m_text->AppendText(text.substr(start+len));
}

