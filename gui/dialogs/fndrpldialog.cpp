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

#include "fndrpldialog.hpp"
#include "../globals.hpp"
#include "../../kernel/core/ui/language.hpp"
extern Language _guilang;


IMPLEMENT_DYNAMIC_CLASS(FindReplaceDialog, wxDialog)

BEGIN_EVENT_TABLE(FindReplaceDialog, wxDialog)
    EVT_BUTTON(wxID_FIND, FindReplaceDialog::OnFind)
    EVT_BUTTON(wxID_REPLACE, FindReplaceDialog::OnReplace)
    EVT_BUTTON(wxID_REPLACE_ALL, FindReplaceDialog::OnReplaceAll)
    EVT_BUTTON(wxID_CANCEL, FindReplaceDialog::OnCancel)

    EVT_UPDATE_UI(wxID_FIND, FindReplaceDialog::OnUpdateFindUI)
    EVT_UPDATE_UI(wxID_REPLACE, FindReplaceDialog::OnUpdateFindUI)
    EVT_UPDATE_UI(wxID_REPLACE_ALL, FindReplaceDialog::OnUpdateFindUI)

//    EVT_ENTER_WINDOW(FindReplaceDialog::OnEnter)
    EVT_SET_FOCUS(FindReplaceDialog::OnGainFocus)
    //EVT_KILL_FOCUS(FindReplaceDialog::OnLoseFocus)

    EVT_CLOSE(FindReplaceDialog::OnCloseWindow)

    EVT_NOTEBOOK_PAGE_CHANGED(-1, FindReplaceDialog::OnPageChange)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxGenericFindReplaceDialog
// ----------------------------------------------------------------------------

void FindReplaceDialog::Init()
{
    m_FindReplaceData = nullptr;

    m_chkWord =
    m_chkCase =
    m_chkWrapAround =
    m_chkWordRepl =
    m_chkWrapAroundRepl =
    m_chkCaseRepl = nullptr;

    m_radioDir =
    m_radioDirRepl = nullptr;

    m_textFind =
    m_textFindRepl =
    m_textRepl = nullptr;

    m_tabs = nullptr;

    m_skipFocusing = false;
}

bool FindReplaceDialog::Create(wxWindow *parent,
                                        wxFindReplaceData *data,
                                        const wxString& title,
                                        int style)
{
    parent = GetParentForModalDialog(parent, style);

    if ( !wxDialog::Create(parent, wxID_ANY, title,
                           wxDefaultPosition, wxSize(470*g_pixelScale,230*g_pixelScale),
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
                           | style) )
    {
        return false;
    }

    SetData(data);

    wxCHECK_MSG( m_FindReplaceData, false,
                 wxT("can't create dialog without data") );

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    m_tabs = new ViewerBook(this, wxID_ANY, wxDefaultPosition , wxSize(470*g_pixelScale,230*g_pixelScale));
    ViewerPanel* findpage = new ViewerPanel(m_tabs, wxID_ANY);
    ViewerPanel* replacepage = new ViewerPanel(m_tabs, wxID_ANY);

    wxBoxSizer *leftsizer_find = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *leftsizer_replace = new wxBoxSizer( wxVERTICAL );

    // 3 columns because there is a spacer in the middle
    wxFlexGridSizer *sizer2Col_find = new wxFlexGridSizer(3);
    wxFlexGridSizer *sizer2Col_replace = new wxFlexGridSizer(3);
    sizer2Col_find->AddGrowableCol(2);
    sizer2Col_replace->AddGrowableCol(2);

    sizer2Col_find->Add(new wxStaticText(findpage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_SEARCHFOR"),
                                    wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                                    0,
                                    wxALIGN_CENTRE_VERTICAL | wxALIGN_RIGHT);

    sizer2Col_replace->Add(new wxStaticText(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_SEARCHFOR"),
                                    wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                                    0,
                                    wxALIGN_CENTRE_VERTICAL | wxALIGN_RIGHT);

    sizer2Col_find->Add(10, 0);
    sizer2Col_replace->Add(10, 0);

    m_textFind = new wxTextCtrl(findpage, wxID_ANY, m_FindReplaceData->GetFindString());
    sizer2Col_find->Add(m_textFind, 1, wxALIGN_CENTRE_VERTICAL | wxEXPAND);
    m_textFindRepl = new wxTextCtrl(replacepage, wxID_ANY, m_FindReplaceData->GetFindString());
    sizer2Col_replace->Add(m_textFindRepl, 1, wxALIGN_CENTRE_VERTICAL | wxEXPAND);

    //if ( style & wxFR_REPLACEDIALOG )
    {
        sizer2Col_replace->Add(new wxStaticText(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_REPLACEWITH"),
                                        wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                                        0,
                                        wxALIGN_CENTRE_VERTICAL |
                                        wxALIGN_RIGHT | wxTOP, 5);

        sizer2Col_replace->Add(isPda ? 2 : 10, 0);

        m_textRepl = new wxTextCtrl(replacepage, wxID_ANY,
                                    m_FindReplaceData->GetReplaceString());
        sizer2Col_replace->Add(m_textRepl, 1,
                       wxALIGN_CENTRE_VERTICAL | wxEXPAND | wxTOP, 5);
    }

    leftsizer_find->Add(sizer2Col_find, 0, wxEXPAND | wxALL, 5);
    leftsizer_replace->Add(sizer2Col_replace, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer *optsizer_find = new wxBoxSizer( isPda ? wxVERTICAL : wxHORIZONTAL );
    wxBoxSizer *optsizer_replace = new wxBoxSizer( isPda ? wxVERTICAL : wxHORIZONTAL );

    wxBoxSizer *chksizer_find = new wxBoxSizer( wxVERTICAL);
    wxBoxSizer *chksizer_replace = new wxBoxSizer( wxVERTICAL);

    m_chkWord = new wxCheckBox(findpage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_WHOLEWORD"));
    chksizer_find->Add(m_chkWord, 0, wxALL, 3);

    m_chkCase = new wxCheckBox(findpage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_MATCHCASE"));
    chksizer_find->Add(m_chkCase, 0, wxALL, 3);

    m_chkWrapAround = new wxCheckBox(findpage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_WRAPAROUND"));
    chksizer_find->Add(m_chkWrapAround, 0, wxALL, 3);

    m_chkWordRepl = new wxCheckBox(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_WHOLEWORD"));
    chksizer_replace->Add(m_chkWordRepl, 0, wxALL, 3);

    m_chkCaseRepl = new wxCheckBox(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_MATCHCASE"));
    chksizer_replace->Add(m_chkCaseRepl, 0, wxALL, 3);

    m_chkWrapAroundRepl = new wxCheckBox(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_WRAPAROUND"));
    chksizer_replace->Add(m_chkWrapAroundRepl, 0, wxALL, 3);

    optsizer_find->Add(chksizer_find, 0, wxALL, 10);
    optsizer_replace->Add(chksizer_replace, 0, wxALL, 10);

    static const wxString searchDirections[] = {_guilang.get("GUI_DLG_FNDRPLC_DIR_UP"),_guilang.get("GUI_DLG_FNDRPLC_DIR_DOWN")};
    int majorDimension = 0;
    int rbStyle ;
    if (isPda)
        rbStyle = wxRA_SPECIFY_ROWS;
    else
        rbStyle = wxRA_SPECIFY_COLS;

    m_radioDir = new wxRadioBox(findpage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_DIRECTION"),
                                wxDefaultPosition, wxDefaultSize,
                                WXSIZEOF(searchDirections), searchDirections,
                                majorDimension, rbStyle);

    optsizer_find->Add(m_radioDir, 0, wxALL, isPda ? 5 : 10);

    m_radioDirRepl = new wxRadioBox(replacepage, wxID_ANY, _guilang.get("GUI_DLG_FNDRPLC_DIRECTION"),
                                wxDefaultPosition, wxDefaultSize,
                                WXSIZEOF(searchDirections), searchDirections,
                                majorDimension, rbStyle);

    optsizer_replace->Add(m_radioDirRepl, 0, wxALL, isPda ? 5 : 10);

    leftsizer_find->Add(optsizer_find);
    leftsizer_replace->Add(optsizer_replace);

    wxBoxSizer *bttnsizer_find = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *bttnsizer_replace = new wxBoxSizer(wxVERTICAL);

    wxButton* btn_find = new wxButton(findpage, wxID_FIND, _guilang.get("GUI_DLG_FNDRPLC_BTN_FIND"));
    wxButton* btn_replace = new wxButton(replacepage, wxID_FIND, _guilang.get("GUI_DLG_FNDRPLC_BTN_FIND"));
    btn_find->SetDefault();

    bttnsizer_find->Add(btn_find, 0, wxALL, 3);
    bttnsizer_replace->Add(btn_replace, 0, wxALL, 3);

    bttnsizer_find->Add(new wxButton(findpage, wxID_CANCEL, _guilang.get("GUI_DLG_FNDRPLC_BTN_CANCEL")), 0, wxALL, 3);
    bttnsizer_replace->Add(new wxButton(replacepage, wxID_CANCEL, _guilang.get("GUI_DLG_FNDRPLC_BTN_CANCEL")), 0, wxALL, 3);

    //if ( style & wxFR_REPLACEDIALOG )
    {
        bttnsizer_replace->Add(new wxButton(replacepage, wxID_REPLACE, _guilang.get("GUI_DLG_FNDRPLC_BTN_REPLACE")),
                                    0, wxALL, 3);

        bttnsizer_replace->Add(new wxButton(replacepage, wxID_REPLACE_ALL, _guilang.get("GUI_DLG_FNDRPLC_BTN_REPLACEALL")),
                                    0, wxALL, 3);
    }

    wxBoxSizer *topsizer_find = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer *topsizer_replace = new wxBoxSizer( wxHORIZONTAL );

    topsizer_find->Add(leftsizer_find, 1, wxALL, isPda ? 0 : 5);
    topsizer_find->Add(bttnsizer_find, 0, wxALL, isPda ? 0 : 5);
    topsizer_replace->Add(leftsizer_replace, 1, wxALL, isPda ? 0 : 5);
    topsizer_replace->Add(bttnsizer_replace, 0, wxALL, isPda ? 0 : 5);

    InitPage();

    //SetAutoLayout( true );

    findpage->SetSizer(topsizer_find);
    replacepage->SetSizer(topsizer_replace);
    topsizer_find->SetSizeHints( findpage );
    topsizer_find->Fit( findpage );
    topsizer_replace->SetSizeHints( replacepage );
    topsizer_replace->Fit( replacepage );

    m_tabs->AddPage(findpage, _guilang.get("GUI_DLG_FIND"), (style & wxFR_REPLACEDIALOG) ? false : true);
    m_tabs->AddPage(replacepage, _guilang.get("GUI_DLG_REPLACE"), (style & wxFR_REPLACEDIALOG) ? true : false);
    //SetSizer( topsizer_find );


    Centre( wxBOTH );

    //m_textFind->SetFocus();

    return true;
}

void FindReplaceDialog::InitPage()
{
    m_textFind->SetValue(m_FindReplaceData->GetFindString());
    m_textFindRepl->SetValue(m_FindReplaceData->GetFindString());
    m_textRepl->SetValue(m_FindReplaceData->GetReplaceString());

    int flags = m_FindReplaceData->GetFlags();

    if ( flags & wxFR_MATCHCASE )
    {
        m_chkCase->SetValue(true);
        m_chkCaseRepl->SetValue(true);
    }
    if ( flags & wxFR_WHOLEWORD )
    {
        m_chkWord->SetValue(true);
        m_chkWordRepl->SetValue(true);
    }
    if ( flags & wxFR_WRAPAROUND )
    {
        m_chkWrapAround->SetValue(true);
        m_chkWrapAroundRepl->SetValue(true);
    }
    m_radioDir->SetSelection( flags & wxFR_DOWN );
    m_radioDirRepl->SetSelection( flags & wxFR_DOWN );

    /*if ( style & wxFR_NOMATCHCASE )
        m_chkCase->Enable(false);

    if ( style & wxFR_NOWHOLEWORD )
        m_chkWord->Enable(false);

    if ( style & wxFR_NOUPDOWN)
        m_radioDir->Enable(false);*/
}


void FindReplaceDialog::OnPageChange(wxNotebookEvent& event)
{
    InitPage();
    wxDialog::SetTitle("NumeRe: " + m_tabs->GetPageText(event.GetSelection()));
}

// ----------------------------------------------------------------------------
// send the notification event
// ----------------------------------------------------------------------------

void FindReplaceDialog::SendEvent(const wxEventType& evtType)
{
    wxFindDialogEvent event(evtType, GetId());
    event.SetEventObject(this);
    int flags = 0;
    if (m_tabs->GetSelection()) //replace
    {
        event.SetFindString(m_textFindRepl->GetValue());
        event.SetReplaceString(m_textRepl->GetValue());
        if ( m_chkCaseRepl->GetValue() )
            flags |= wxFR_MATCHCASE;
        if ( m_chkWordRepl->GetValue() )
            flags |= wxFR_WHOLEWORD;
        if ( m_chkWrapAroundRepl->GetValue() )
            flags |= wxFR_WRAPAROUND;
        if ( !m_radioDirRepl || m_radioDirRepl->GetSelection() == 1 )
        {
            flags |= wxFR_DOWN;
        }
    }
    else // find
    {
        event.SetFindString(m_textFind->GetValue());
        if ( m_chkCase->GetValue() )
            flags |= wxFR_MATCHCASE;
        if ( m_chkWord->GetValue() )
            flags |= wxFR_WHOLEWORD;
        if ( m_chkWrapAround->GetValue() )
            flags |= wxFR_WRAPAROUND;
        if ( !m_radioDir || m_radioDir->GetSelection() == 1 )
        {
            flags |= wxFR_DOWN;
        }
    }

    event.SetFlags(flags);

    wxFindReplaceDialogBase::Send(event);
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void FindReplaceDialog::OnFind(wxCommandEvent& WXUNUSED(event))
{
    SendEvent(wxEVT_FIND_NEXT);
}

void FindReplaceDialog::OnReplace(wxCommandEvent& WXUNUSED(event))
{
    SendEvent(wxEVT_FIND_REPLACE);
}

void FindReplaceDialog::OnReplaceAll(wxCommandEvent& WXUNUSED(event))
{
    SendEvent(wxEVT_FIND_REPLACE_ALL);
}

void FindReplaceDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    SendEvent(wxEVT_FIND_CLOSE);

    Show(false);
}

void FindReplaceDialog::OnEnter(wxMouseEvent& event)
{
    if (!m_skipFocusing)
        this->SetFocus();
    event.Skip();
}

void FindReplaceDialog::OnGainFocus(wxFocusEvent& event)
{
    m_tabs->GetCurrentPage()->SetFocus();
    if (m_tabs->GetSelection()) // findreplace
    {
        m_textFindRepl->SetFocus();
    }
    else // find
    {
        m_textFind->SetFocus();
    }
    event.Skip();
}

void FindReplaceDialog::OnLoseFocus(wxFocusEvent& event)
{
    this->SetTransparent(100);
    event.Skip();
}

void FindReplaceDialog::OnUpdateFindUI(wxUpdateUIEvent &event)
{
    // we can't search for empty strings
    event.Enable( !m_textFind->GetValue().empty() || !m_textFindRepl->GetValue().empty() );
}

void FindReplaceDialog::OnCloseWindow(wxCloseEvent &)
{
    SendEvent(wxEVT_FIND_CLOSE);
}

void FindReplaceDialog::SetFocus()
{
    m_tabs->GetCurrentPage()->SetFocus();
    if (m_tabs->GetSelection()) // findreplace
    {
        m_textFindRepl->SetFocus();
    }
    else // find
    {
        m_textFind->SetFocus();
    }
}
