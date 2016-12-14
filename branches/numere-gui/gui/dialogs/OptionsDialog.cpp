/////////////////////////////////////////////////////////////////////////////
// Name:        OptionsDialog.cpp
// Purpose:
// Author:      Mark Erikson
// Modified by:
// Created:     11/23/03 16:02:26
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "OptionsDialog.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "../../common/CommonHeaders.h"
#include "../../common/Options.h"


////@begin includes
////@end includes

#include <wx/checklst.h>
#include <wx/valtext.h>
#include <wx/dirdlg.h>
#include <wx/dir.h>

#include "OptionsDialog.h"
#include "../NumeReWindow.h"

#include "../../perms/p.h"
#include "../../common/debug.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

////@begin XPM images
////@end XPM images

/*!
 * OptionsDialog type definition
 */

IMPLEMENT_CLASS( OptionsDialog, wxDialog )

/*!
 * OptionsDialog event table definition
 */

BEGIN_EVENT_TABLE( OptionsDialog, wxDialog )

////@begin OptionsDialog event table entries
    EVT_BUTTON( ID_SETAUTHCODE, OptionsDialog::OnUpdateAuthCode )

    EVT_TEXT( ID_TEXTMINGWPATH, OptionsDialog::OnTextmingwpathUpdated )

    EVT_BUTTON( ID_BTNFINDMINGW, OptionsDialog::OnFindMingwClick )

    EVT_BUTTON( ID_BUTTON1, OptionsDialog::OnVerifyMingwClick )

    EVT_BUTTON( ID_BUTTON_OK, OptionsDialog::OnButtonOkClick )

    EVT_BUTTON( ID_BUTTON_CANCEL, OptionsDialog::OnButtonCancelClick )

////@end OptionsDialog event table entries
	EVT_CHAR(OptionsDialog::OnChar)
	EVT_TEXT_ENTER( ID_PROFCODE, OptionsDialog::OnEnter )
	EVT_TEXT_ENTER(ID_HOSTNAME, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_USERNAME, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_PASSWORD1, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_PASSWORD2, OptionsDialog::OnEnter)

END_EVENT_TABLE()

/*!
 * OptionsDialog constructors
 */

OptionsDialog::OptionsDialog( )
{
}

OptionsDialog::OptionsDialog( wxWindow* parent, Options* options, wxWindowID id,  const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
	m_parentFrame = (NumeReWindow*)parent;
	m_options = options;

	wxTextValidator textval(wxFILTER_EXCLUDE_CHAR_LIST);
	wxStringList exclude;
	exclude.Add(wxT("\""));
	m_password1->SetValidator(textval);
	m_password2->SetValidator(textval);
}

/*!
 * OptionsDialog creator
 */

bool OptionsDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin OptionsDialog member initialisation
    m_optionsNotebook = NULL;
    m_checkList = NULL;
    m_txtProfCode = NULL;
    m_butSetAuthCode = NULL;
    m_authCodeLabel = NULL;
    m_hostname = NULL;
    m_username = NULL;
    m_password1 = NULL;
    m_password2 = NULL;
    m_txtMingwPath = NULL;
    m_chkShowCompileCommands = NULL;
    m_printStyle = NULL;
    m_cbPrintLineNumbers = NULL;
    m_showToolbarText = NULL;
    m_chkCombineWatchWindow = NULL;
    m_termHistory = NULL;
////@end OptionsDialog member initialisation

////@begin OptionsDialog creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end OptionsDialog creation
    return TRUE;
}

/*!
 * Control creation for OptionsDialog
 */

void OptionsDialog::CreateControls()
{
////@begin OptionsDialog content construction
    OptionsDialog* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    m_optionsNotebook = new wxNotebook( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxSize(400, 270), wxNB_DEFAULT|wxNB_TOP );

    wxPanel* itemPanel4 = new wxPanel( m_optionsNotebook, ID_PANELFEATURES, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemPanel4, wxID_STATIC, _("Chameleon has a variety of features that can be enabled by your professor.\nHere you can see what features are enabled, as well as enter an activation code."), wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    itemBoxSizer5->Add(itemStaticText6, 0, wxGROW|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer7, 0, wxGROW, 5);
    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer7->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText9 = new wxStaticText( itemPanel4, wxID_STATIC, _("Current authorized features:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_checkListStrings;
    m_checkList = new wxCheckListBox( itemPanel4, ID_CHECKLISTBOX, wxDefaultPosition, wxSize(180, 175), m_checkListStrings, wxLB_SINGLE );
    itemBoxSizer8->Add(m_checkList, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer7->Add(itemBoxSizer11, 0, wxGROW, 5);
    wxStaticText* itemStaticText12 = new wxStaticText( itemPanel4, wxID_STATIC, _("Enter the code from your professor here:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    m_txtProfCode = new wxTextCtrl( itemPanel4, ID_PROFCODE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemBoxSizer11->Add(m_txtProfCode, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_butSetAuthCode = new wxButton( itemPanel4, ID_SETAUTHCODE, _("Set authorization code"), wxDefaultPosition, wxSize(120, -1), 0 );
    itemBoxSizer11->Add(m_butSetAuthCode, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText15 = new wxStaticText( itemPanel4, wxID_STATIC, _("Current authorization code:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_authCodeLabel = new wxStaticText( itemPanel4, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(m_authCodeLabel, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    m_optionsNotebook->AddPage(itemPanel4, _("Features"));

    wxPanel* itemPanel17 = new wxPanel( m_optionsNotebook, ID_PANELNETWORK, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel17->SetSizer(itemBoxSizer18);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer18->Add(itemBoxSizer19, 0, wxALIGN_TOP, 5);
    wxStaticText* itemStaticText20 = new wxStaticText( itemPanel17, wxID_STATIC, _("Network server address:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText20, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_hostname = new wxTextCtrl( itemPanel17, ID_HOSTNAME, _T(""), wxDefaultPosition, wxSize(160, -1), wxTE_PROCESS_ENTER );
    itemBoxSizer19->Add(m_hostname, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText22 = new wxStaticText( itemPanel17, wxID_STATIC, _("Username:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText22, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_username = new wxTextCtrl( itemPanel17, ID_USERNAME, _T(""), wxDefaultPosition, wxSize(160, -1), wxTE_PROCESS_ENTER );
    itemBoxSizer19->Add(m_username, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText24 = new wxStaticText( itemPanel17, wxID_STATIC, _("Password (no quote marks allowed):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText24, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_password1 = new wxTextCtrl( itemPanel17, ID_PASSWORD1, _T(""), wxDefaultPosition, wxSize(160, -1), wxTE_PROCESS_ENTER|wxTE_PASSWORD );
    itemBoxSizer19->Add(m_password1, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText26 = new wxStaticText( itemPanel17, wxID_STATIC, _("Confirm password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText26, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_password2 = new wxTextCtrl( itemPanel17, ID_PASSWORD2, _T(""), wxDefaultPosition, wxSize(160, -1), wxTE_PROCESS_ENTER|wxTE_PASSWORD );
    itemBoxSizer19->Add(m_password2, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_optionsNotebook->AddPage(itemPanel17, _("Network"));

    wxPanel* itemPanel28 = new wxPanel( m_optionsNotebook, ID_PANELCOMPILER, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxVERTICAL);
    itemPanel28->SetSizer(itemBoxSizer29);

    wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer29->Add(itemBoxSizer30, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer31 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer30->Add(itemBoxSizer31, 0, wxALIGN_TOP|wxALL, 0);
    wxStaticText* itemStaticText32 = new wxStaticText( itemPanel28, wxID_STATIC, _("MinGW installation path:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer31->Add(itemStaticText32, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer31->Add(itemBoxSizer33, 0, wxALIGN_LEFT|wxALL, 5);
    m_txtMingwPath = new wxTextCtrl( itemPanel28, ID_TEXTMINGWPATH, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    itemBoxSizer33->Add(m_txtMingwPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton35 = new wxButton( itemPanel28, ID_BTNFINDMINGW, _("Select"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer33->Add(itemButton35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton36 = new wxButton( itemPanel28, ID_BUTTON1, _("Verify"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer33->Add(itemButton36, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer29->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALL, 5);
    m_chkShowCompileCommands = new wxCheckBox( itemPanel28, ID_CHECKBOX1, _("Show compiler command lines"), wxDefaultPosition, wxDefaultSize, 0 );
    m_chkShowCompileCommands->SetValue(false);
    itemBoxSizer37->Add(m_chkShowCompileCommands, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_optionsNotebook->AddPage(itemPanel28, _("Compiler"));

    wxPanel* itemPanel39 = new wxPanel( m_optionsNotebook, ID_PANELMISC, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel39->SetSizer(itemBoxSizer40);

    wxBoxSizer* itemBoxSizer41 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer40->Add(itemBoxSizer41, 1, wxALIGN_TOP, 5);
    wxStaticText* itemStaticText42 = new wxStaticText( itemPanel39, wxID_STATIC, _("Print text in:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer41->Add(itemStaticText42, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_printStyleStrings;
    m_printStyleStrings.Add(_("Black and white"));
    m_printStyleStrings.Add(_("Color"));
    m_printStyle = new wxComboBox( itemPanel39, ID_PRINTSTYLE, _("Black and white"), wxDefaultPosition, wxDefaultSize, m_printStyleStrings, wxCB_READONLY );
    m_printStyle->SetStringSelection(_("Black and white"));
    itemBoxSizer41->Add(m_printStyle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_cbPrintLineNumbers = new wxCheckBox( itemPanel39, ID_PRINTLINENUMBERS, _("Print line numbers"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_cbPrintLineNumbers->SetValue(false);
    itemBoxSizer41->Add(m_cbPrintLineNumbers, 0, wxALIGN_LEFT|wxALL, 5);

    m_showToolbarText = new wxCheckBox( itemPanel39, ID_SHOWTOOLBARTEXT, _("Show text on toolbar buttons"), wxDefaultPosition, wxDefaultSize, 0 );
    m_showToolbarText->SetValue(false);
    itemBoxSizer41->Add(m_showToolbarText, 1, wxGROW|wxALL, 5);

    m_chkCombineWatchWindow = new wxCheckBox( itemPanel39, ID_COMBINEWATCH, _("Combine watch window and debug output into one tab"), wxDefaultPosition, wxDefaultSize, 0 );
    m_chkCombineWatchWindow->SetValue(false);
    itemBoxSizer41->Add(m_chkCombineWatchWindow, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer41->Add(itemBoxSizer47, 0, wxALIGN_LEFT|wxALL, 0);
    wxStaticText* itemStaticText48 = new wxStaticText( itemPanel39, wxID_STATIC, _("Maximum history lines in the terminal:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_termHistory = new wxSpinCtrl( itemPanel39, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 25, 2500, 0 );
    itemBoxSizer47->Add(m_termHistory, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_optionsNotebook->AddPage(itemPanel39, _("Miscellaneous"));

    itemBoxSizer2->Add(m_optionsNotebook, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer50, 0, wxALIGN_RIGHT|wxALL, 0);

    wxButton* itemButton51 = new wxButton( itemDialog1, ID_BUTTON_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemButton51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton52 = new wxButton( itemDialog1, ID_BUTTON_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end OptionsDialog content construction
}

/*!
 * Should we show tooltips?
 */

bool OptionsDialog::ShowToolTips()
{
  return TRUE;
}

/*
wxCheckListBox* OptionsDialog::GetListBox()
{
    return this->m_checkList;
}

wxString OptionsDialog::GetServerAddress()
{
    return m_hostname->GetValue();
}

wxString OptionsDialog::GetUsername()
{
	return m_username->GetValue();
}

wxString OptionsDialog::GetPassword1()
{
	return m_password1->GetValue();
}

wxString OptionsDialog::GetPassword2()
{
	return m_password2->GetValue();
}

void OptionsDialog::SetServerAddress(wxString address)
{
	m_hostname->SetValue(address);
}

void OptionsDialog::SetUsername(wxString username)
{
	m_username->SetValue(username);
}

void OptionsDialog::SetPassword1(wxString pwd)
{
	m_password1->SetValue(pwd);
}

void OptionsDialog::SetPassword2(wxString pwd)
{
	m_password2->SetValue(pwd);
}
*/

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK
 */

void OptionsDialog::OnButtonOkClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	ExitDialog();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
 */

void OptionsDialog::OnButtonCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
	EndModal(wxCANCEL);
	m_optionsNotebook->SetSelection(0);
}

void OptionsDialog::OnChar(wxKeyEvent &event)
{
	if(event.GetKeyCode() == WXK_RETURN)
	{
		event.Skip();
	}

}
/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_PROFCODE
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnEnter
///  Allows the user to press Enter to close the dialog
///
///  @param  event wxCommandEvent & The generated event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::OnEnter( wxCommandEvent& event )
{
    // Insert custom code here
	event.Skip();

	ExitDialog();
}


//////////////////////////////////////////////////////////////////////////////
///  public ExitDialog
///  Ensures that everything's correct before exiting the dialog
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::ExitDialog()
{
	if(!m_mingwPathValidated && !VerifyMingwPath())
	{
		return;
	}

	m_txtProfCode->Clear();

	wxString pwd1 = m_password1->GetValue();
	wxString pwd2 = m_password2->GetValue();

	if(pwd1 == pwd2)
	{
		//Permission* perms = m_options->GetPerms();

		if(EvaluateOptions())
		{
			UpdateChecklist();
			EndModal(wxOK);
			m_optionsNotebook->SetSelection(0);
		}

	}
	else
	{
		wxMessageBox("Please enter the same password in both fields");
	}
}

/*
wxString OptionsDialog::GetAuthCode()
{
	return m_txtProfCode->GetValue();

	if(authCodeString == wxEmptyString)
	{
		return -1;
	}

	long authCodeLong = 0;

	authCodeString.ToLong(&authCodeLong);


	return authCodeLong;

}
*/

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnUpdateAuthCode
///  Checks the newly entered authorization code and updates the permissions manager
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::OnUpdateAuthCode( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	wxString newAuthCode = m_txtProfCode->GetValue();
	newAuthCode.MakeUpper();

	Permission* perms = m_options->GetPerms();

	if(!perms->setGlobalAuthorized(newAuthCode))
	{

		wxMessageBox("Invalid authorization code.  Please check that it was entered correctly and try again.");
	}
	else
	{
		UpdateChecklist();

		m_txtProfCode->Clear();
		m_authCodeLabel->SetLabel(newAuthCode);

		wxMessageBox("Authorized features updated.");

	}

}


//////////////////////////////////////////////////////////////////////////////
///  public EnableServerSettings
///  Enables the network text fields
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::EnableServerSettings()
{
	wxColour white("white");
	m_hostname->SetEditable(true);
	m_hostname->SetBackgroundColour(white);
	m_username->SetEditable(true);
	m_username->SetBackgroundColour(white);
	m_password1->SetEditable(true);
	m_password1->SetBackgroundColour(white);
	m_password2->SetEditable(true);
	m_password2->SetBackgroundColour(white);
}

//////////////////////////////////////////////////////////////////////////////
///  public DisableServerSettings
///  Disables the network text fields
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::DisableServerSettings()
{
	wxColour grey("light grey");
	m_hostname->SetEditable(false);
	m_hostname->SetBackgroundColour(grey);
	m_username->SetEditable(false);
	m_username->SetBackgroundColour(grey);
	m_password1->SetEditable(false);
	m_password1->SetBackgroundColour(grey);
	m_password2->SetEditable(false);
	m_password2->SetBackgroundColour(grey);
}

/*
void OptionsDialog::SetAuthCode(wxString authcode)
{
	m_authCodeLabel->SetLabel(authcode);
}
*/

//////////////////////////////////////////////////////////////////////////////
///  public BrowseForDir
///  Lets the user browse for the MinGW directory
///
///  @param  textbox wxTextCtrl * The textbox to fill
///  @param  name    wxString     The title of the browse dialog
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::BrowseForDir(wxTextCtrl* textbox, wxString title)
{
	wxString currentDir = textbox->GetValue();

	title = "Select the directory where MinGW is installed (usually inside the Chameleon directory)";

	wxString newDir;
	wxString defaultDir;

	if(wxFileName::DirExists(currentDir))
	{
		defaultDir = currentDir;
	}
	else
	{
		defaultDir = wxEmptyString;
	}

	wxString resultDir = wxDirSelector(title, defaultDir);

	if(resultDir != wxEmptyString)
	{
		textbox->SetValue(resultDir);
	}


}/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_MINGWBROWSE
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnMinGWBrowseClick
///  Calls the BrowseForDir function
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////

/*
void OptionsDialog::OnMinGWBrowseClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

    BrowseForDir(m_txtMingwPath, wxEmptyString);
}
*/

//////////////////////////////////////////////////////////////////////////////
///  public EvaluateOptions
///  Validates the options items before exiting the dialog
///
///  @return bool Whether or not the options are valid
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool OptionsDialog::EvaluateOptions()
{
	bool validOptions = true;

	wxString outputMessage = wxEmptyString;

	if(validOptions)
	{
		Permission* perms = m_options->GetPerms();
		for(int i = 0; i < m_checkList->GetCount(); i++)
		{
			int mappedPerm = m_permMappings[i];

			if(m_checkList->IsChecked(i))
			{
				perms->enable(mappedPerm);
			}
			else
			{
				perms->disable(mappedPerm);
			}
		}

		m_options->SetHostname(m_hostname->GetValue());
		m_options->SetUsername(m_username->GetValue());
		m_options->SetPassphrase(m_password1->GetValue());
		m_options->SetTerminalHistorySize(m_termHistory->GetValue());

		wxString selectedPrintStyleString = m_printStyle->GetValue();

		if(selectedPrintStyleString == "Color")
		{
			m_options->SetPrintStyle(wxSTC_PRINT_COLOURONWHITE);
		}
		else
		{
			m_options->SetPrintStyle(wxSTC_PRINT_BLACKONWHITE);
		}

		m_options->SetShowToolbarText(m_showToolbarText->IsChecked());
		m_options->SetLineNumberPrinting(m_cbPrintLineNumbers->IsChecked());
		m_options->SetCombineWatchWindow(m_chkCombineWatchWindow->IsChecked());
		m_options->SetShowCompileCommands(m_chkShowCompileCommands->IsChecked());
	}
	else
	{
		wxMessageBox(outputMessage, "Invalid Option", wxOK | wxICON_WARNING);
	}

	return validOptions;
}

//////////////////////////////////////////////////////////////////////////////
///  public InitializeDialog
///  Sets up the dialog's contents before being displayed
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::InitializeDialog()
{
	Permission* perms = m_options->GetPerms();

	UpdateChecklist();

	m_hostname->SetValue(m_options->GetHostname());
	m_username->SetValue(m_options->GetUsername());
	wxString password = m_options->GetPassphrase();
	m_password1->SetValue(password);
	m_password2->SetValue(password);

	//m_txtMingwPath->SetValue(m_options->GetMingwPath());

	m_authCodeLabel->SetLabel(perms->GetAuthCode());

	wxString printStyleString;
	if(m_options->GetPrintStyle() == wxSTC_PRINT_COLOURONWHITE)
	{
		printStyleString = "Color";
	}
	else
	{
		printStyleString = "Black and white";
	}

	m_printStyle->SetValue(printStyleString);
	m_termHistory->SetValue(m_options->GetTerminalHistorySize());

	m_showToolbarText->SetValue(m_options->GetShowToolbarText());
	m_cbPrintLineNumbers->SetValue(m_options->GetLineNumberPrinting());
	m_chkCombineWatchWindow->SetValue(m_options->GetCombineWatchWindow());
	m_chkShowCompileCommands->SetValue(m_options->GetShowCompileCommands());

	m_txtMingwPath->SetValue(m_options->GetMingwBasePath());

	VerifyMingwPath(false);

}

//////////////////////////////////////////////////////////////////////////////
///  public UpdateChecklist
///  Updates the items in the permissions checklist, based on the current permissions
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void OptionsDialog::UpdateChecklist()
{
	Permission* perms = m_options->GetPerms();
	m_checkList->Clear();
	m_permMappings.Clear();

	wxString optionname;

	for(int i = PERM_FIRST; i < PERM_LAST; i++)
	{
		if(perms->isAuthorized(i))
		{
			optionname = perms->getPermName(i);
			m_checkList->Append(optionname);
			int checkIndex = m_permMappings.GetCount();
			m_permMappings.Add(i);

			if(perms->isEnabled(i))
			{
				m_checkList->Check(checkIndex, true);
			}
		}
	}
}

/*!
 * Get bitmap resources
 */

wxBitmap OptionsDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin OptionsDialog bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end OptionsDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon OptionsDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin OptionsDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end OptionsDialog icon retrieval
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BTNFINDMINGW
 */

void OptionsDialog::OnFindMingwClick( wxCommandEvent& event )
{
	const wxString& dir = wxDirSelector("Choose the MinGW installation folder:");
	if ( !dir.empty() )
	{
		m_txtMingwPath->SetValue(dir);
	}

}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void OptionsDialog::OnVerifyMingwClick( wxCommandEvent& event )
{
	VerifyMingwPath(true);
	return;

}

bool OptionsDialog::VerifyMingwPath(bool showResults)
{
	wxString mingwPath = m_txtMingwPath->GetValue();


	bool result = true;

	if(mingwPath.IsEmpty())
	{
		return true;
	}



	wxString errorMessage = m_options->VerifyMingwPath(mingwPath);
	wxString messageBoxCaption = wxEmptyString;
	int messageBoxOptions = wxOK;

	if(errorMessage != wxEmptyString)
	{
		messageBoxCaption = "MinGW Validation Problem";
		messageBoxOptions |= wxICON_ERROR;
		result = false;


	}
	else
	{
		errorMessage = "MinGW successfully detected!";
		messageBoxCaption = "MinGW Installation Found";
		messageBoxOptions |= wxICON_INFORMATION;

		result = true;

	}

	if(showResults)
	{
		wxMessageBox(errorMessage, messageBoxCaption, messageBoxOptions);
	}

	m_mingwPathValidated = result;

	return result;
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTMINGWPATH
 */

void OptionsDialog::OnTextmingwpathUpdated( wxCommandEvent& event )
{
	m_mingwPathValidated = false;
}

