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

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "AboutChameleonDialog.h"
#include "../../common/verinfo.h"

////@begin XPM images
#include "chamlogo_1.xpm"
#include "team_1.xpm"
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


	m_lblVersion->SetLabel(versionString);
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

    this->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxNotebook* itemNotebook3 = new wxNotebook( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
    itemNotebook3->SetForegroundColour(wxColour(0, 0, 0));
    itemNotebook3->SetBackgroundColour(wxColour(255, 255, 255));
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook3Sizer = new wxNotebookSizer(itemNotebook3);
#endif

    wxPanel* itemPanel4 = new wxPanel( itemNotebook3, ID_PROGPANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemPanel4->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel4->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(m_sizerProgram);

    wxBitmap itemStaticBitmap6Bitmap(itemDialog1->GetBitmapResource(wxT("chamlogo_1.xpm")));
    wxStaticBitmap* itemStaticBitmap6 = new wxStaticBitmap( itemPanel4, wxID_STATIC, itemStaticBitmap6Bitmap, wxDefaultPosition, wxSize(225, 142), 0 );
    itemStaticBitmap6->SetBackgroundColour(wxColour(255, 255, 255));
    m_sizerProgram->Add(itemStaticBitmap6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText( itemPanel4, wxID_STATIC, _("Chameleon\nThe Adaptive Instructional IDE"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText7->SetForegroundColour(wxColour(0, 0, 0));
    itemStaticText7->SetBackgroundColour(wxColour(255, 255, 255));
    itemStaticText7->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Verdana")));
    m_sizerProgram->Add(itemStaticText7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    m_lblVersion = new wxStaticText( itemPanel4, wxID_STATIC, _("Version 9.8.7.6"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxNO_BORDER );
    m_lblVersion->SetForegroundColour(wxColour(0, 0, 0));
    m_lblVersion->SetBackgroundColour(wxColour(255, 255, 255));
    m_lblVersion->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD, false, _T("Verdana")));
    m_sizerProgram->Add(m_lblVersion, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText9 = new wxStaticText( itemPanel4, wxID_STATIC, _("Chameleon is licensed under the General Public License, \navailable at http://www.gnu.org/licenses/gpl.html"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText9->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    m_sizerProgram->Add(itemStaticText9, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook3->AddPage(itemPanel4, _("The Program"));

    wxPanel* itemPanel10 = new wxPanel( itemNotebook3, ID_TEAMPANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemPanel10->SetForegroundColour(wxColour(255, 255, 255));
    itemPanel10->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    wxStaticText* itemStaticText12 = new wxStaticText( itemPanel10, wxID_STATIC, _("Chameleon was brought to you by:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText12->SetForegroundColour(wxColour(0, 0, 0));
    itemStaticText12->SetBackgroundColour(wxColour(255, 255, 255));
    itemStaticText12->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Verdana")));
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBitmap itemStaticBitmap13Bitmap(itemDialog1->GetBitmapResource(wxT("team_1.xpm")));
    wxStaticBitmap* itemStaticBitmap13 = new wxStaticBitmap( itemPanel10, wxID_STATIC, itemStaticBitmap13Bitmap, wxDefaultPosition, wxSize(340, 175), 0 );
    itemStaticBitmap13->SetBackgroundColour(wxColour(255, 255, 255));
    itemBoxSizer11->Add(itemStaticBitmap13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText14 = new wxStaticText( itemPanel10, wxID_STATIC, _("Ben Carhart\n - Requirements, Quality Assurance, Testing Lead\n - Debugger, Permissions\nDavid Czechowski\n - Design Lead, Configuration Management \n - Networking, Compiler\nMark Erikson\n - Team Lead, Project Lead\n - GUI, Editor, Terminal"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemStaticText14->SetForegroundColour(wxColour(0, 0, 0));
    itemStaticText14->SetBackgroundColour(wxColour(255, 255, 255));
    itemStaticText14->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Verdana")));
    itemBoxSizer11->Add(itemStaticText14, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook3->AddPage(itemPanel10, _("The Team"));

    wxPanel* itemPanel15 = new wxPanel( itemNotebook3, ID_CREDITSPANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemPanel15->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel15->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxVERTICAL);
    itemPanel15->SetSizer(itemBoxSizer16);

    wxStaticText* itemStaticText17 = new wxStaticText( itemPanel15, wxID_STATIC, _("Chameleon wouldn't have been possible without the tools,\nexamples, parts, and programs we were able to use.  \nWe'd like to recognize them here:\n\n* The wxWidgets toolkit, which made development\n   a lot easier than it could have been.\n* Neil Hodgson, author of the Scintilla editor widget.\n* Robin Dunn, who created the wxStyledTextCtrl \n   wrapper for Scintilla.\n* Simon Tatham, author of the incredible Putty suite of \n   SSH clients.  Without his Plink tool, our project \n   would have been effectively impossible.\n* Otto Wyss, creator of the wxGuide example program .\n* Derry Bryson, creator of the wxTerm terminal widget\n   class, and Timothy Miller, who wrote the GTerm \n   core that wxTerm is based on.\n* Jan van de Baard, who wrote some checksum \n   routines that came in handy.\n* Anyone else whose code we ever looked at or used."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText17->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer16->Add(itemStaticText17, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook3->AddPage(itemPanel15, _("The Credits"));

    wxPanel* itemPanel18 = new wxPanel( itemNotebook3, ID_STATSPANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemPanel18->SetForegroundColour(wxColour(0, 0, 0));
    itemPanel18->SetBackgroundColour(wxColour(255, 255, 255));
    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxVERTICAL);
    itemPanel18->SetSizer(itemBoxSizer19);

    wxStaticText* itemStaticText20 = new wxStaticText( itemPanel18, wxID_STATIC, _("* Total lines of code: ~24,000\n   - ~8,300 statements\n   - ~8,000 lines of comments\n* ~820 CVS commmits, totaling 41,000 lines of changes\n* Number of source files: ~100\n* Project length: 221 days (9/18/03 - 4/26/04)\n* Number of times Mark broke CVS: at least 15\n* Number of times Ben had to rewrite the debugger: 4\n* Number of bugs fixed the night before the final \n   presentation: 15\n* Number of bugs Mark introduced the night before the \n   final presentation that showed up during the demo: 2\n* Number of beta releases: 5\n* Number of beta comments we got back: 2\n* Number of times David asked to discuss something\n  over the phone instead of AIM: too many to count\n* Number of all-nighters: technically none\n* Number of times one of us was up past 5:00 AM\n   writing code: we lost track\n* Amount of caffeine consumed: immeasurable"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText20->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial")));
    itemBoxSizer19->Add(itemStaticText20, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook3->AddPage(itemPanel18, _("The Stats"));

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
    wxUnusedVar(name);
    if (name == _T("chamlogo_1.xpm"))
    {
        wxBitmap bitmap(chamlogo_1_xpm);
        return bitmap;
    }
    else if (name == _T("team_1.xpm"))
    {
        wxBitmap bitmap(team_1_xpm);
        return bitmap;
    }
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
