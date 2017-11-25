/////////////////////////////////////////////////////////////////////////////
// Name:        AboutChameleonDialog.cpp
// Purpose:
// Author:
// Modified by:
// Created:     04/20/04 01:22:14
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "AboutChameleonDialog.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include <wx/msw/private.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "AboutChameleonDialog.h"
#include "../../common/verinfo.h"
#include "../../kernel/core/ui/language.hpp"

////@begin XPM images
//#include "chamlogo_1.xpm"
//#include "team_1.xpm"
////@end XPM images

/*!
 * AboutChameleonDialog type definition
 */

IMPLEMENT_CLASS( AboutChameleonDialog, wxDialog )

/*!
 * AboutChameleonDialog event table definition
 */

BEGIN_EVENT_TABLE( AboutChameleonDialog, wxDialog )

////@begin AboutChameleonDialog event table entries
    EVT_BUTTON( ID_BUTTONOK, AboutChameleonDialog::OnButtonOKClick )

////@end AboutChameleonDialog event table entries

END_EVENT_TABLE()

extern Language _guilang;
extern const string sVersion;

/*!
 * AboutChameleonDialog constructors
 */

AboutChameleonDialog::AboutChameleonDialog( )
{
}

AboutChameleonDialog::AboutChameleonDialog( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * AboutChameleonDialog creator
 */

bool AboutChameleonDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AboutChameleonDialog member initialisation
    m_sizerProgram = NULL;
    m_lblVersion = NULL;
////@end AboutChameleonDialog member initialisation

////@begin AboutChameleonDialog creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end AboutChameleonDialog creation


	HINSTANCE hInstance = wxGetInstance();
	CFileVersionInfo fvi;
	fvi.Open(hInstance);

	int major = fvi.GetFileVersionMajor();
	int minor = fvi.GetFileVersionMinor();
	int revision = fvi.GetFileVersionQFE();
	int  build = fvi.GetFileVersionBuild();

	wxString versionString = wxString::Format("Version %d.%d.%d.%d", major, minor, build, revision);


	m_lblVersion->SetLabel("v"+sVersion); //(versionString);
	m_sizerProgram->Layout();
    return TRUE;
}

/*!
 * Control creation for AboutChameleonDialog
 */

void AboutChameleonDialog::CreateControls()
{
////@begin AboutChameleonDialog content construction
    AboutChameleonDialog* itemDialog1 = this;

    //this->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxNotebook* itemNotebook3 = new wxNotebook( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
    //itemNotebook3->SetForegroundColour(wxColour(0, 0, 0));
    //itemNotebook3->SetBackgroundColour(wxColour(255, 255, 255));
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook3Sizer = new wxNotebookSizer(itemNotebook3);
#endif

    wxPanel* itemPanel4 = new wxPanel( itemNotebook3, ID_PROGPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemPanel4->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel4->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(m_sizerProgram);

    wxBitmap itemStaticBitmap6Bitmap(itemDialog1->GetBitmapResource(wxT("chamlogo_1.xpm")));
    wxStaticBitmap* itemStaticBitmap6 = new wxStaticBitmap( itemPanel4, wxID_STATIC, itemStaticBitmap6Bitmap, wxDefaultPosition, wxSize(300, 300), 0 );
    itemStaticBitmap6->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram->Add(itemStaticBitmap6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText( itemPanel4, wxID_STATIC, _("NumeRe:\nFramework für Numerische Rechnungen"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText7->SetForegroundColour(wxColour(0, 0, 0));
    itemStaticText7->SetBackgroundColour(wxColour(255, 255, 255));
    itemStaticText7->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Arial")));
    m_sizerProgram->Add(itemStaticText7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    m_lblVersion = new wxStaticText( itemPanel4, wxID_STATIC, _("Version 9.8.7.6"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxNO_BORDER );
    m_lblVersion->SetForegroundColour(wxColour(0, 0, 0));
    m_lblVersion->SetBackgroundColour(wxColour(255, 255, 255));
    m_lblVersion->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Arial")));
    m_sizerProgram->Add(m_lblVersion, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText9 = new wxStaticText( itemPanel4, wxID_STATIC, _(_guilang.get("GUI_ABOUT_LICENCE_SHORT")), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText9->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    m_sizerProgram->Add(itemStaticText9, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook3->AddPage(itemPanel4, _("NumeRe"));

    wxPanel* itemPanel10 = new wxPanel( itemNotebook3, ID_TEAMPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemPanel10->SetForegroundColour(wxColour(255, 255, 255));
    itemPanel10->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    wxStaticText* itemStaticText12 = new wxStaticText( itemPanel10, wxID_STATIC, _(_guilang.get("GUI_ABOUT_TEAM_INTRO")), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText12->SetForegroundColour(wxColour(0, 0, 0));
    itemStaticText12->SetBackgroundColour(wxColour(255, 255, 255));
    itemStaticText12->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* aboutTextCtrl = new wxTextCtrl(itemPanel10, wxID_ANY, _guilang.get("GUI_ABOUT_TEAM"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_READONLY);
    aboutTextCtrl->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer11->Add(aboutTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    itemNotebook3->AddPage(itemPanel10, _("Team"));

    wxPanel* infoPanel = new wxPanel(itemNotebook3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    infoPanel->SetForegroundColour(*wxBLACK);
    infoPanel->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* infoBoxSizer = new wxBoxSizer(wxVERTICAL);
    infoPanel->SetSizer(infoBoxSizer);

    wxTextCtrl* infoTextCtrl = new wxTextCtrl(infoPanel, wxID_ANY, _guilang.get("GUI_ABOUT_INFO"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_READONLY);
    infoTextCtrl->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    infoBoxSizer->Add(infoTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    itemNotebook3->AddPage(infoPanel, "Info");

    wxPanel* itemPanel15 = new wxPanel( itemNotebook3, ID_CREDITSPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemPanel15->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel15->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxVERTICAL);
    itemPanel15->SetSizer(itemBoxSizer16);

    wxTextCtrl* creditsTextCtrl = new wxTextCtrl(itemPanel15, wxID_ANY, _guilang.get("GUI_ABOUT_CREDITS"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_READONLY);
    creditsTextCtrl->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer16->Add(creditsTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    /*wxStaticText* itemStaticText17 = new wxStaticText( itemPanel15, wxID_STATIC, _(_guilang.get("GUI_ABOUT_CREDITS")), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText17->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer16->Add(itemStaticText17, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 5);*/

    itemNotebook3->AddPage(itemPanel15, _("Credits"));

    wxPanel* itemPanel18 = new wxPanel( itemNotebook3, ID_STATSPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemPanel18->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel18->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxVERTICAL);
    itemPanel18->SetSizer(itemBoxSizer19);

    wxTextCtrl* statsTextCtrl = new wxTextCtrl(itemPanel18, wxID_ANY, _guilang.get("GUI_ABOUT_STATS"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_READONLY);
    statsTextCtrl->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer19->Add(statsTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    /*4wxStaticText* itemStaticText20 = new wxStaticText( itemPanel18, wxID_STATIC, _(_guilang.get("GUI_ABOUT_STATS")), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText20->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer19->Add(itemStaticText20, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 5);*/

    itemNotebook3->AddPage(itemPanel18, _("Stats"));

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(itemNotebook3Sizer, 0, wxGROW|wxALL, 5);
#else
    itemBoxSizer2->Add(itemNotebook3, 0, wxGROW|wxALL, 5);
#endif

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer21, 0, wxALIGN_RIGHT|wxALL, 0);

    wxButton* itemButton22 = new wxButton( itemDialog1, ID_BUTTONOK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end AboutChameleonDialog content construction
}

/*!
 * Should we show tooltips?
 */

bool AboutChameleonDialog::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTONOK
 */

void AboutChameleonDialog::OnButtonOKClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
	EndModal(wxOK);
}



/*!
 * Get bitmap resources
 */

wxBitmap AboutChameleonDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AboutChameleonDialog bitmap retrieval
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxUnusedVar(name);
    if (name == _T("chamlogo_1.xpm"))
    {
        wxBitmap bitmap(f.GetPath(true)+"icons\\folder.png", wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    /*else if (name == _T("team_1.xpm"))
    {
        wxBitmap bitmap(team_1_xpm);
        return bitmap;
    }*/
    return wxNullBitmap;
////@end AboutChameleonDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AboutChameleonDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AboutChameleonDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end AboutChameleonDialog icon retrieval
}
