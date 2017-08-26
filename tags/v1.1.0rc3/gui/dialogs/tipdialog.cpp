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
#include <wx/statline.h>
#include <wx/artprov.h>

#define wxID_NEXT_TIP 32000
#if defined(__SMARTPHONE__)
    #define wxLARGESMALL(large,small) small
#else
    #define wxLARGESMALL(large,small) large
#endif

BEGIN_EVENT_TABLE(TipDialog, wxDialog)
    EVT_BUTTON(wxID_NEXT_TIP, TipDialog::OnNextTip)
END_EVENT_TABLE()

TipDialog::TipDialog(wxWindow *parent, wxTipProvider *tipProvider, const wxArrayString& text, bool showAtStartup)
           : wxDialog(GetParentForModalDialog(parent, 0), wxID_ANY, text[0], wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_tipProvider = tipProvider;
    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    // 1) create all controls in tab order
    wxStaticText *__text = new wxStaticText(this, wxID_ANY, text[1]);

    if (!isPda)
    {
        wxFont font = __text->GetFont();
        font.SetPointSize(int(1.6 * font.GetPointSize()));
        font.SetWeight(wxFONTWEIGHT_BOLD);
        __text->SetFont(font);
    }

    m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                            wxDefaultPosition, wxSize(200, 160),
                            wxTE_MULTILINE |
                            wxTE_READONLY |
                            wxTE_NO_VSCROLL |
                            wxTE_RICH2 | // a hack to get rid of vert scrollbar
                            wxDEFAULT_CONTROL_BORDER
                            );
#if defined(__WXMSW__)
    m_text->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxNORMAL));
#endif

//#if defined(__WXPM__)
    //
    // The only way to get icons into an OS/2 static bitmap control
    //
//    wxBitmap                        vBitmap;

//    vBitmap.SetId(wxICON_TIP); // OS/2 specific bitmap method--OS/2 wxBitmaps all have an ID.
//                               // and for StatBmp's under OS/2 it MUST be a valid resource ID.
//
//    wxStaticBitmap*                 bmp = new wxStaticBitmap(this, wxID_ANY, vBitmap);
//
//#else

    wxIcon icon = wxArtProvider::GetIcon(wxART_TIP, wxART_CMN_DIALOG);
    wxStaticBitmap *bmp = new wxStaticBitmap(this, wxID_ANY, icon);

//#endif

    m_checkbox = new wxCheckBox(this, wxID_ANY, text[3]);
    m_checkbox->SetValue(showAtStartup);
    m_checkbox->SetFocus();

    // smart phones does not support or do not waste space for wxButtons
#ifndef __SMARTPHONE__
    wxButton *btnNext = new wxButton(this, wxID_NEXT_TIP, text[2]);
#endif

    // smart phones does not support or do not waste space for wxButtons
#ifndef __SMARTPHONE__
    wxButton *btnClose = new wxButton(this, wxID_CLOSE, text[4]);
    SetAffirmativeId(wxID_CLOSE);
#endif


    // 2) put them in boxes

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *icon_text = new wxBoxSizer( wxHORIZONTAL );
    icon_text->Add( bmp, 0, wxCENTER );
    icon_text->Add( __text, 1, wxCENTER | wxLEFT, wxLARGESMALL(20,0) );
    topsizer->Add( icon_text, 0, wxEXPAND | wxALL, wxLARGESMALL(10,0) );

    topsizer->Add( m_text, 1, wxEXPAND | wxLEFT|wxRIGHT, wxLARGESMALL(10,0) );

    wxBoxSizer *bottom = new wxBoxSizer( wxHORIZONTAL );
    if (isPda)
        topsizer->Add( m_checkbox, 0, wxCENTER|wxTOP );
    else
        bottom->Add( m_checkbox, 0, wxCENTER );

    // smart phones does not support or do not waste space for wxButtons
#ifdef __SMARTPHONE__
    SetRightMenu(wxID_NEXT_TIP, _("Next"));
    SetLeftMenu(wxID_CLOSE);
#else
    if (!isPda)
        bottom->Add( 10,10,1 );
    bottom->Add( btnNext, 0, wxCENTER | wxLEFT, wxLARGESMALL(10,0) );
    bottom->Add( btnClose, 0, wxCENTER | wxLEFT, wxLARGESMALL(10,0) );
#endif

    if (isPda)
        topsizer->Add( bottom, 0, wxCENTER | wxALL, 5 );
    else
        topsizer->Add( bottom, 0, wxEXPAND | wxALL, wxLARGESMALL(10,0) );

    SetTipText();

    SetSizer( topsizer );

    topsizer->SetSizeHints( this );
    topsizer->Fit( this );

    Centre(wxBOTH | wxCENTER_FRAME);
}


bool ShowTip(wxWindow* parent, wxTipProvider* tipProvider, const wxArrayString& text, bool showAtStartUp)
{
    TipDialog dlg(parent, tipProvider, text, showAtStartUp);
    dlg.ShowModal();

    return dlg.ShowTipsOnStartup();
}


MyTipProvider::MyTipProvider(const std::vector<std::string>& vTipList) : wxTipProvider(vTipList.size())
{
    vTip = vTipList;
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

