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
#include <fstream>

#ifdef __BORLANDC__
#pragma hdrstop
#endif


#include "AboutChameleonDialog.h"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/version.h"
#include "../compositions/grouppanel.hpp"


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
extern const std::string sVersion;

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

	m_lblVersion->SetLabel("v"+sVersion);
	m_sizerProgram->Layout();
    return TRUE;
}

/*!
 * Control creation for AboutChameleonDialog
 */

void AboutChameleonDialog::CreateControls()
{
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(itemBoxSizer2);

    wxNotebook* aboutDialogNoteBook = new wxNotebook(this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP);

#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook3Sizer = new wxNotebookSizer(aboutDialogNoteBook);
#endif

    // MAIN PAGE
    wxPanel* mainAboutPanel = new wxPanel( aboutDialogNoteBook, ID_PROGPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    mainAboutPanel->SetForegroundColour(wxColour(0, 0, 0));
    mainAboutPanel->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram = new wxBoxSizer(wxVERTICAL);
    mainAboutPanel->SetSizer(m_sizerProgram);

    wxBitmap itemStaticBitmap6Bitmap(this->GetBitmapResource(wxT("chamlogo_1.xpm")));
    wxStaticBitmap* itemStaticBitmap6 = new wxStaticBitmap( mainAboutPanel, wxID_STATIC, itemStaticBitmap6Bitmap, wxDefaultPosition, wxSize(300, 300), 0 );
    itemStaticBitmap6->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram->Add(itemStaticBitmap6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxString sAppName = _guilang.get("COMMON_APPNAME");
    sAppName.Replace(": ", ":\n");

    wxStaticText* mainStaticText = new wxStaticText( mainAboutPanel, wxID_STATIC, sAppName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    mainStaticText->SetForegroundColour(wxColour(0, 0, 0));
    mainStaticText->SetBackgroundColour(wxColour(255, 255, 255));
    mainStaticText->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Arial")));
    m_sizerProgram->Add(mainStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    m_lblVersion = new wxStaticText( mainAboutPanel, wxID_STATIC, _("Version 9.8.7.6"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxNO_BORDER );
    m_lblVersion->SetForegroundColour(wxColour(0, 0, 0));
    m_lblVersion->SetBackgroundColour(wxColour(255, 255, 255));
    m_lblVersion->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Arial")));
    m_sizerProgram->Add(m_lblVersion, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* licenceStaticText = new wxStaticText( mainAboutPanel, wxID_STATIC, _(_guilang.get("GUI_ABOUT_LICENCE_SHORT", AutoVersion::YEAR)), wxDefaultPosition, wxDefaultSize, 0 );
    licenceStaticText->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    m_sizerProgram->Add(licenceStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    aboutDialogNoteBook->AddPage(mainAboutPanel, "NumeRe");

    // TEAM PAGE
    wxPanel* teamPanel = new wxPanel( aboutDialogNoteBook, ID_TEAMPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    teamPanel->SetForegroundColour(wxColour(255, 255, 255));
    teamPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* teamBoxSizer = new wxBoxSizer(wxVERTICAL);
    teamPanel->SetSizer(teamBoxSizer);

    wxStaticText* teamStaticText = new wxStaticText( teamPanel, wxID_STATIC, _(_guilang.get("GUI_ABOUT_TEAM_INTRO")), wxDefaultPosition, wxDefaultSize, 0 );
    teamStaticText->SetForegroundColour(wxColour(0, 0, 0));
    teamStaticText->SetBackgroundColour(wxColour(255, 255, 255));
    teamStaticText->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    teamBoxSizer->Add(teamStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    TextField* aboutTextCtrl = new TextField(teamPanel, wxID_ANY, _guilang.get("GUI_ABOUT_TEAM"), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    teamBoxSizer->Add(aboutTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(teamPanel, "Team");

    // INFO PAGE
    wxPanel* infoPanel = new wxPanel(aboutDialogNoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    infoPanel->SetForegroundColour(*wxBLACK);
    infoPanel->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* infoBoxSizer = new wxBoxSizer(wxVERTICAL);
    infoPanel->SetSizer(infoBoxSizer);

    TextField* infoTextCtrl = new TextField(infoPanel, wxID_ANY, _guilang.get("GUI_ABOUT_INFO"), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    infoBoxSizer->Add(infoTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(infoPanel, "Info");

    // CREDITS PAGE
    wxPanel* creditsPanel = new wxPanel(aboutDialogNoteBook, ID_CREDITSPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    creditsPanel->SetForegroundColour(wxColour(0, 0, 0));
    creditsPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* creditsBoxSizer = new wxBoxSizer(wxVERTICAL);
    creditsPanel->SetSizer(creditsBoxSizer);

    TextField* creditsTextCtrl = new TextField(creditsPanel, wxID_ANY, _guilang.get("GUI_ABOUT_CREDITS"), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    creditsBoxSizer->Add(creditsTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(creditsPanel, "Credits");

    // STATS PAGE
    wxPanel* statsPanel = new wxPanel(aboutDialogNoteBook, ID_STATSPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    statsPanel->SetForegroundColour(wxColour(0, 0, 0));
    statsPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* statsBoxSizer = new wxBoxSizer(wxVERTICAL);
    statsPanel->SetSizer(statsBoxSizer);

    TextField* statsTextCtrl = new TextField(statsPanel, wxID_ANY, _guilang.get("GUI_ABOUT_STATS"), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    statsBoxSizer->Add(statsTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(statsPanel, "Stats");

    // JOIN PAGE
    wxPanel* joinPanel = new wxPanel(aboutDialogNoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    joinPanel->SetForegroundColour(wxColour(0, 0, 0));
    joinPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* joinBoxSizer = new wxBoxSizer(wxVERTICAL);
    joinPanel->SetSizer(joinBoxSizer);

    TextField* joinTextCtrl = new TextField(joinPanel, wxID_ANY, _guilang.get("GUI_ABOUT_JOIN"), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    joinBoxSizer->Add(joinTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(joinPanel, "Contribute");

    // Legal PAGE
    wxPanel* legalPanel = new wxPanel(aboutDialogNoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    legalPanel->SetForegroundColour(wxColour(0, 0, 0));
    legalPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* legalBoxSizer = new wxBoxSizer(wxVERTICAL);
    legalPanel->SetSizer(legalBoxSizer);

    std::string licenses;

    if (wxFileName::Exists(_guilang.ValidFileName("<>/THIRD_PARTY.licenses", ".licenses", false, false)))
    {
        std::ifstream licenseFile(_guilang.ValidFileName("<>/THIRD_PARTY.licenses", ".licenses", false, false), std::ios::in | std::ios::ate);

        size_t size = licenseFile.tellg();
        licenseFile.seekg(0, std::ios::beg);
        licenses.resize(size);
        licenseFile.read(licenses.data(), size);
    }
    else
        licenses = "LICENSES FILE MISSING";

    TextField* legalTextCtrl = new TextField(legalPanel, wxID_ANY, _guilang.get("GUI_ABOUT_LEGAL", licenses), wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_RICH2);
    legalBoxSizer->Add(legalTextCtrl, 1, wxGROW | wxEXPAND | wxALL, 5);

    aboutDialogNoteBook->AddPage(legalPanel, "Legal");

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(itemNotebook3Sizer, 0, wxGROW|wxALL, 5);
#else
    itemBoxSizer2->Add(aboutDialogNoteBook, 0, wxGROW|wxALL, 5);
#endif

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer21, 0, wxALIGN_RIGHT|wxALL, 0);

    wxButton* itemButton22 = new wxButton(this, ID_BUTTONOK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
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
        wxBitmap bitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR)+"icons\\folder.png", wxBITMAP_TYPE_PNG);
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
