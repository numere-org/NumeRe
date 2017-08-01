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
#include "../../kernel/core/language.hpp"

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

extern Language _guilang;
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

    EVT_BUTTON(ID_BTN_LOADPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_SAVEPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_SCRIPTPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_PROCPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_PLOTPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_RESETCOLOR, OptionsDialog::OnButtonClick)

    EVT_COMBOBOX(ID_CLRSPIN, OptionsDialog::OnColorTypeChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_FORE, OptionsDialog::OnColorPickerChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_BACK, OptionsDialog::OnColorPickerChange)

////@end OptionsDialog event table entries
	EVT_CHAR(OptionsDialog::OnChar)
	EVT_TEXT_ENTER( ID_PROFCODE, OptionsDialog::OnEnter )
	EVT_TEXT_ENTER(ID_HOSTNAME, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_USERNAME, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_PASSWORD1, OptionsDialog::OnEnter)
	EVT_TEXT_ENTER(ID_PASSWORD2, OptionsDialog::OnEnter)

END_EVENT_TABLE()

std::string replacePathSeparator(const std::string&);

/*!
 * OptionsDialog constructors
 */

OptionsDialog::OptionsDialog( )
{
}

OptionsDialog::OptionsDialog( wxWindow* parent, Options* options, wxWindowID id,  const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
	m_parentFrame = (NumeReWindow*)parent;
	m_options = options;
    Create(parent, id, caption, pos, size, style);

	//wxTextValidator textval(wxFILTER_EXCLUDE_CHAR_LIST);
	//wxStringList exclude;
	//exclude.Add(wxT("\""));
	//m_password1->SetValidator(textval);
	//m_password2->SetValidator(textval);
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
    m_saveSession = NULL;
    m_termHistory = NULL;

    m_compactTables = nullptr;
    m_AutoLoadDefines = nullptr;
    m_showGreeting = nullptr;
    m_LoadCompactTables = nullptr;
    m_ExtendedInfo = nullptr;
    m_ShowHints = nullptr;
    m_CustomLanguage = nullptr;
    m_ESCinScripts = nullptr;
    m_UseLogfile = nullptr;
    m_UseExternalViewer = nullptr;
    m_LoadPath = nullptr;
    m_SavePath = nullptr;
    m_ScriptPath = nullptr;
    m_ProcPath = nullptr;
    m_PlotPath = nullptr;
    m_defaultFont = nullptr;
    m_precision = nullptr;
    m_autosaveinterval = nullptr;

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
    OptionsDialog* optionDialog = this;

    wxBoxSizer* optionVSizer = new wxBoxSizer(wxVERTICAL);
    optionDialog->SetSizer(optionVSizer);

    m_optionsNotebook = new wxNotebook( optionDialog, ID_NOTEBOOK, wxDefaultPosition, wxSize(400, 400), wxNB_DEFAULT|wxNB_TOP );

    /**wxPanel* itemPanel4 = new wxPanel( m_optionsNotebook, ID_PANELFEATURES, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
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

*/

    /// Configuration panel
    wxPanel* configurationPanel = new wxPanel( m_optionsNotebook, ID_PANELCOMPILER, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* configVSizer_2 = new wxBoxSizer(wxVERTICAL);
    configurationPanel->SetSizer(configVSizer_2);

    wxBoxSizer* configHSizer = new wxBoxSizer(wxHORIZONTAL);
    configVSizer_2->Add(configHSizer, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* configVSizer = new wxBoxSizer(wxVERTICAL);
    configHSizer->Add(configVSizer, 0, wxALIGN_TOP|wxALL, 0);

    m_compactTables = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_COMPACTTABLES")), wxDefaultPosition, wxDefaultSize, 0 );
    m_compactTables->SetValue(false);
    configVSizer->Add(m_compactTables, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_AutoLoadDefines = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_DEFCTRL")), wxDefaultPosition, wxDefaultSize, 0 );
    m_AutoLoadDefines->SetValue(false);
    configVSizer->Add(m_AutoLoadDefines, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_LoadCompactTables = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_EMPTYCOLS")), wxDefaultPosition, wxDefaultSize, 0 );
    m_LoadCompactTables->SetValue(false);
    configVSizer->Add(m_LoadCompactTables, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_ExtendedInfo = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_EXTENDEDINFO")), wxDefaultPosition, wxDefaultSize, 0 );
    m_ExtendedInfo->SetValue(false);
    configVSizer->Add(m_ExtendedInfo, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_CustomLanguage = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_CUSTOMLANG")), wxDefaultPosition, wxDefaultSize, 0 );
    m_CustomLanguage->SetValue(false);
    configVSizer->Add(m_CustomLanguage, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_ESCinScripts = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_ESCINSCRIPTS")), wxDefaultPosition, wxDefaultSize, 0 );
    m_ESCinScripts->SetValue(false);
    configVSizer->Add(m_ESCinScripts, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_UseLogfile = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_LOGFILE")), wxDefaultPosition, wxDefaultSize, 0 );
    m_UseLogfile->SetValue(false);
    configVSizer->Add(m_UseLogfile, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_UseExternalViewer = new wxCheckBox( configurationPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_EXTERNALVIEWER")), wxDefaultPosition, wxDefaultSize, 0 );
    m_UseExternalViewer->SetValue(false);
    configVSizer->Add(m_UseExternalViewer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
    configVSizer->Add(itemBoxSizer51);
    m_autosaveinterval = new wxSpinCtrl( configurationPanel, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 10, 600, 30);
    itemBoxSizer51->Add(m_autosaveinterval, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxStaticText* itemStaticText50 = new wxStaticText( configurationPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_AUTOSAVE")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_optionsNotebook->AddPage(configurationPanel, _(_guilang.get("GUI_OPTIONS_CONFIG")));


    /// Path settings panel
    wxPanel* pathPanel = new wxPanel( m_optionsNotebook, ID_PANELNETWORK, wxDefaultPosition, wxSize(200, 200), wxTAB_TRAVERSAL );
    wxBoxSizer* pathHSizer = new wxBoxSizer(wxHORIZONTAL);
    pathPanel->SetSizer(pathHSizer);
    wxBoxSizer* pathVSizer = new wxBoxSizer(wxVERTICAL);
    pathHSizer->Add(pathVSizer, 0, wxALIGN_TOP, 5);

    wxStaticText* itemStaticText20 = new wxStaticText( pathPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_LOADPATH")), wxDefaultPosition, wxDefaultSize, 0 );
    pathVSizer->Add(itemStaticText20, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* pathVSizer_1 = new wxBoxSizer(wxHORIZONTAL);
    pathVSizer->Add(pathVSizer_1, wxALIGN_LEFT);
    m_LoadPath = new wxTextCtrl( pathPanel, wxID_ANY, _T(""), wxDefaultPosition, wxSize(280, -1), wxTE_PROCESS_ENTER );
    m_LoadPath->SetValue("<loadpath>");
    pathVSizer_1->Add(m_LoadPath, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* loadbutton = new wxButton(pathPanel, ID_BTN_LOADPATH, _guilang.get("GUI_OPTIONS_CHOOSE"));
    pathVSizer_1->Add(loadbutton, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText22 = new wxStaticText( pathPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_SAVEPATH")), wxDefaultPosition, wxDefaultSize, 0 );
    pathVSizer->Add(itemStaticText22, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* pathVSizer_2 = new wxBoxSizer(wxHORIZONTAL);
    pathVSizer->Add(pathVSizer_2, wxALIGN_LEFT);
    m_SavePath = new wxTextCtrl( pathPanel, wxID_ANY, _T(""), wxDefaultPosition, wxSize(280, -1), wxTE_PROCESS_ENTER );
    m_SavePath->SetValue("<savepath>");
    pathVSizer_2->Add(m_SavePath, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* savebutton = new wxButton(pathPanel, ID_BTN_SAVEPATH, _guilang.get("GUI_OPTIONS_CHOOSE"));
    pathVSizer_2->Add(savebutton, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText24 = new wxStaticText( pathPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_SCRIPTPATH")), wxDefaultPosition, wxDefaultSize, 0 );
    pathVSizer->Add(itemStaticText24, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* pathVSizer_3 = new wxBoxSizer(wxHORIZONTAL);
    pathVSizer->Add(pathVSizer_3, wxALIGN_LEFT);
    m_ScriptPath = new wxTextCtrl( pathPanel, wxID_ANY, _T(""), wxDefaultPosition, wxSize(280, -1), wxTE_PROCESS_ENTER );
    m_ScriptPath->SetValue("<scriptpath>");
    pathVSizer_3->Add(m_ScriptPath, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* scriptbutton = new wxButton(pathPanel, ID_BTN_SCRIPTPATH, _guilang.get("GUI_OPTIONS_CHOOSE"));
    pathVSizer_3->Add(scriptbutton, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText26 = new wxStaticText( pathPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_PROCPATH")), wxDefaultPosition, wxDefaultSize, 0 );
    pathVSizer->Add(itemStaticText26, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* pathVSizer_4 = new wxBoxSizer(wxHORIZONTAL);
    pathVSizer->Add(pathVSizer_4, wxALIGN_LEFT);
    m_ProcPath = new wxTextCtrl( pathPanel, wxID_ANY, _T(""), wxDefaultPosition, wxSize(280, -1), wxTE_PROCESS_ENTER );
    m_ProcPath->SetValue("<procpath>");
    pathVSizer_4->Add(m_ProcPath, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* procbutton = new wxButton(pathPanel, ID_BTN_PROCPATH, _guilang.get("GUI_OPTIONS_CHOOSE"));
    pathVSizer_4->Add(procbutton, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText27 = new wxStaticText( pathPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_PLOTPATH")), wxDefaultPosition, wxDefaultSize, 0 );
    pathVSizer->Add(itemStaticText27, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* pathVSizer_5 = new wxBoxSizer(wxHORIZONTAL);
    pathVSizer->Add(pathVSizer_5, wxALIGN_LEFT);
    m_PlotPath = new wxTextCtrl( pathPanel, wxID_ANY, _T(""), wxDefaultPosition, wxSize(280, -1), wxTE_PROCESS_ENTER );
    m_PlotPath->SetValue("<plotpath>");
    pathVSizer_5->Add(m_PlotPath, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* plotbutton = new wxButton(pathPanel, ID_BTN_PLOTPATH, _guilang.get("GUI_OPTIONS_CHOOSE"));
    pathVSizer_5->Add(plotbutton, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_optionsNotebook->AddPage(pathPanel, _(_guilang.get("GUI_OPTIONS_PATHS")));

    /// Style panel
    wxPanel* stylePanel = new wxPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxSize(100,80), wxTAB_TRAVERSAL);
    wxBoxSizer* styleHSizer = new wxBoxSizer(wxHORIZONTAL);
    stylePanel->SetSizer(styleHSizer);
    wxBoxSizer* styleVSizer = new wxBoxSizer(wxVERTICAL);
    styleHSizer->Add(styleVSizer, 1, wxALIGN_TOP, 5);

    wxStaticText* styleStaticText = new wxStaticText(stylePanel, wxID_STATIC, _guilang.get("GUI_OPTIONS_SYNTAXHIGHLIGHTING"));
    styleVSizer->Add(styleStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxFlexGridSizer* colorGroupSizer = new wxFlexGridSizer(2, 0, 5);

    wxArrayString styles = m_options->GetStyleIdentifier();
    m_colorType = new wxComboBox( stylePanel, ID_CLRSPIN, styles[0], wxDefaultPosition, wxDefaultSize, styles, wxCB_READONLY );
    m_colorType->SetStringSelection(styles[0]);

    m_foreColor = new wxColourPickerCtrl(stylePanel, ID_CLRPICKR_FORE, m_options->GetSyntaxStyle(0).foreground);
    m_backColor = new wxColourPickerCtrl(stylePanel, ID_CLRPICKR_BACK, m_options->GetSyntaxStyle(0).background);

    m_resetButton = new wxButton(stylePanel, ID_RESETCOLOR, _guilang.get("GUI_OPTIONS_RESETHIGHLIGHT"), wxDefaultPosition, wxDefaultSize, 0);

    colorGroupSizer->Add(m_foreColor, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    colorGroupSizer->Add(m_colorType, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    colorGroupSizer->Add(m_backColor, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    colorGroupSizer->Add(m_resetButton, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    styleVSizer->Add(colorGroupSizer, 1, wxALIGN_LEFT, 5);

    wxStaticText* defaultFontStaticText = new wxStaticText(stylePanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_DEFAULTFONT")), wxDefaultPosition, wxDefaultSize, 0 );
    styleVSizer->Add(defaultFontStaticText, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString defaultFont;
    defaultFont.Add("pagella");
    defaultFont.Add("adventor");
    defaultFont.Add("bonum");
    defaultFont.Add("chorus");
    defaultFont.Add("heros");
    defaultFont.Add("heroscn");
    defaultFont.Add("schola");
    defaultFont.Add("termes");
    m_defaultFont = new wxComboBox(stylePanel, ID_PRINTSTYLE, "pagella", wxDefaultPosition, wxDefaultSize, defaultFont, wxCB_READONLY );
    m_defaultFont->SetStringSelection("pagella");
    styleVSizer->Add(m_defaultFont, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_optionsNotebook->AddPage(stylePanel, "Style");

    /// Misc panel
    wxPanel* miscPanel = new wxPanel( m_optionsNotebook, ID_PANELMISC, wxDefaultPosition, wxSize(100, 80), wxTAB_TRAVERSAL );
    wxBoxSizer* miscHSizer = new wxBoxSizer(wxHORIZONTAL);
    miscPanel->SetSizer(miscHSizer);
    wxBoxSizer* miscVSizer = new wxBoxSizer(wxVERTICAL);
    miscHSizer->Add(miscVSizer, 1, wxALIGN_TOP, 5);

    wxStaticText* itemStaticText42 = new wxStaticText( miscPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_PRINT")), wxDefaultPosition, wxDefaultSize, 0 );
    miscVSizer->Add(itemStaticText42, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_printStyleStrings;
    m_printStyleStrings.Add(_(_guilang.get("GUI_OPTIONS_PRINT_BW")));
    m_printStyleStrings.Add(_(_guilang.get("GUI_OPTIONS_PRINT_COLOR")));
    m_printStyle = new wxComboBox( miscPanel, ID_PRINTSTYLE, _(_guilang.get("GUI_OPTIONS_PRINT_BW")), wxDefaultPosition, wxDefaultSize, m_printStyleStrings, wxCB_READONLY );
    m_printStyle->SetStringSelection(_(_guilang.get("GUI_OPTIONS_PRINT_BW")));
    miscVSizer->Add(m_printStyle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_cbPrintLineNumbers = new wxCheckBox( miscPanel, ID_PRINTLINENUMBERS, _(_guilang.get("GUI_OPTIONS_PRINT_LINENUMBERS")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_cbPrintLineNumbers->SetValue(false);
    miscVSizer->Add(m_cbPrintLineNumbers, 0, wxALIGN_LEFT|wxALL, 5);

    m_saveSession = new wxCheckBox( miscPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_SAVE_SESSION")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_saveSession->SetValue(false);
    miscVSizer->Add(m_saveSession, 0, wxALIGN_LEFT|wxALL, 5);

    m_showToolbarText = new wxCheckBox( miscPanel, ID_SHOWTOOLBARTEXT, _(_guilang.get("GUI_OPTIONS_SHOW_TOOLBARTEXT")), wxDefaultPosition, wxDefaultSize, 0 );
    m_showToolbarText->SetValue(false);
    miscVSizer->Add(m_showToolbarText, 1, wxGROW|wxALL, 5);

    m_showGreeting = new wxCheckBox( miscPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_GREETING")), wxDefaultPosition, wxDefaultSize, 0 );
    m_showGreeting->SetValue(false);
    miscVSizer->Add(m_showGreeting, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_ShowHints = new wxCheckBox( miscPanel, wxID_ANY, _(_guilang.get("GUI_OPTIONS_HINTS")), wxDefaultPosition, wxDefaultSize, 0 );
    m_ShowHints->SetValue(false);
    miscVSizer->Add(m_ShowHints, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    /**m_chkCombineWatchWindow = new wxCheckBox( miscPanel, ID_COMBINEWATCH, _("Combine watch window and debug output into one tab"), wxDefaultPosition, wxDefaultSize, 0 );
    m_chkCombineWatchWindow->SetValue(false);
    miscVSizer->Add(m_chkCombineWatchWindow, 0, wxALIGN_LEFT|wxALL, 5);*/

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    miscVSizer->Add(itemBoxSizer47, 0, wxALIGN_LEFT|wxALL, 0);

    m_termHistory = new wxSpinCtrl( miscPanel, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 100, 500, 100 );
    itemBoxSizer47->Add(m_termHistory, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxStaticText* itemStaticText48 = new wxStaticText( miscPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_HISTORY_LINES")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    miscVSizer->Add(itemBoxSizer48, 0, wxALIGN_LEFT | wxALL, 0);

    m_precision = new wxSpinCtrl( miscPanel, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 14, 7 );
    itemBoxSizer48->Add(m_precision, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxStaticText* itemStaticText49 = new wxStaticText( miscPanel, wxID_STATIC, _(_guilang.get("GUI_OPTIONS_PRECISION")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText49, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_optionsNotebook->AddPage(miscPanel, _(_guilang.get("GUI_OPTIONS_MISC")));



    optionVSizer->Add(m_optionsNotebook, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* optionButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    optionVSizer->Add(optionButtonSizer, 0, wxALIGN_RIGHT|wxALL, 0);

    wxButton* okButton = new wxButton( optionDialog, ID_BUTTON_OK, _(_guilang.get("GUI_OPTIONS_OK")), wxDefaultPosition, wxDefaultSize, 0 );
    optionButtonSizer->Add(okButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* cancelButton = new wxButton( optionDialog, ID_BUTTON_CANCEL, _(_guilang.get("GUI_OPTIONS_CANCEL")), wxDefaultPosition, wxDefaultSize, 0 );
    optionButtonSizer->Add(cancelButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

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


void OptionsDialog::synchronizeColors()
{
    for (size_t i = 0; i < m_colorOptions.GetStyleIdentifier().size(); i++)
    {
        m_options->SetStyleForeground(i, m_colorOptions.GetSyntaxStyle(i).foreground);
        m_options->SetStyleBackground(i, m_colorOptions.GetSyntaxStyle(i).background);
    }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
 */

void OptionsDialog::OnButtonCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
	EndModal(wxID_CANCEL);
	m_optionsNotebook->SetSelection(0);
}


void OptionsDialog::OnColorPickerChange(wxColourPickerEvent& event)
{
    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
    if (event.GetId() == ID_CLRPICKR_FORE)
        m_colorOptions.SetStyleForeground(id, m_foreColor->GetColour());
    else
        m_colorOptions.SetStyleBackground(id, m_backColor->GetColour());
}

void OptionsDialog::OnColorTypeChange(wxCommandEvent& event)
{
    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());

    m_foreColor->SetColour(m_colorOptions.GetSyntaxStyle(id).foreground);
    m_backColor->SetColour(m_colorOptions.GetSyntaxStyle(id).background);
}

void OptionsDialog::OnButtonClick(wxCommandEvent& event)
{
    wxString defaultpath;
    switch (event.GetId())
    {
        case ID_BTN_LOADPATH:
            defaultpath = m_LoadPath->GetValue();
            break;
        case ID_BTN_SAVEPATH:
            defaultpath = m_SavePath->GetValue();
            break;
        case ID_BTN_SCRIPTPATH:
            defaultpath = m_ScriptPath->GetValue();
            break;
        case ID_BTN_PROCPATH:
            defaultpath = m_ProcPath->GetValue();
            break;
        case ID_BTN_PLOTPATH:
            defaultpath = m_PlotPath->GetValue();
            break;
        case ID_RESETCOLOR:
        {
            size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
            m_foreColor->SetColour(m_colorOptions.GetDefaultSyntaxStyle(id).foreground);
            m_backColor->SetColour(m_colorOptions.GetDefaultSyntaxStyle(id).background);
            m_colorOptions.SetStyleForeground(id, m_foreColor->GetColour());
            m_colorOptions.SetStyleBackground(id, m_backColor->GetColour());
            return;
        }
    }
    wxDirDialog dialog(this, _guilang.get("GUI_OPTIONS_CHOOSEPATH"), defaultpath);
    int ret = dialog.ShowModal();
    if (ret != wxID_OK)
        return;
    switch (event.GetId())
    {
        case ID_BTN_LOADPATH:
            m_LoadPath->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
        case ID_BTN_SAVEPATH:
            m_SavePath->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
        case ID_BTN_SCRIPTPATH:
            m_ScriptPath->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
        case ID_BTN_PROCPATH:
            m_ProcPath->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
        case ID_BTN_PLOTPATH:
            m_PlotPath->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
    }
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
	/**if(!m_mingwPathValidated && !VerifyMingwPath())
	{
		return;
	}

	m_txtProfCode->Clear();

	wxString pwd1 = m_password1->GetValue();
	wxString pwd2 = m_password2->GetValue();

	if(pwd1 == pwd2)
	{
		//Permission* perms = m_options->GetPerms();

	}
	else
	{
		wxMessageBox("Please enter the same password in both fields");
	}*/
    if(EvaluateOptions())
    {
        UpdateChecklist();
        EndModal(wxID_OK);
        m_optionsNotebook->SetSelection(0);
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
	/**wxColour white("white");
	m_hostname->SetEditable(true);
	m_hostname->SetBackgroundColour(white);
	m_username->SetEditable(true);
	m_username->SetBackgroundColour(white);
	m_password1->SetEditable(true);
	m_password1->SetBackgroundColour(white);
	m_password2->SetEditable(true);
	m_password2->SetBackgroundColour(white);*/
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
	/**wxColour grey("light grey");
	m_hostname->SetEditable(false);
	m_hostname->SetBackgroundColour(grey);
	m_username->SetEditable(false);
	m_username->SetBackgroundColour(grey);
	m_password1->SetEditable(false);
	m_password1->SetBackgroundColour(grey);
	m_password2->SetEditable(false);
	m_password2->SetBackgroundColour(grey);*/
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
		/**Permission* perms = m_options->GetPerms();
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
		m_options->SetPassphrase(m_password1->GetValue());*/

		_option->setbCompact(m_compactTables->GetValue());
		_option->setbDefineAutoLoad(m_AutoLoadDefines->GetValue());
		_option->setbGreeting(m_showGreeting->GetValue());
		_option->setbLoadEmptyCols(m_LoadCompactTables->GetValue());
        _option->setbExtendedFileInfo(m_ExtendedInfo->GetValue());
        _option->setbShowHints(m_ShowHints->GetValue());
        _option->setUserLangFiles(m_CustomLanguage->GetValue());
        _option->setbUseESCinScripts(m_ESCinScripts->GetValue());
        _option->setbUseLogFile(m_UseLogfile->GetValue());
        _option->setExternalDocViewer(m_UseExternalViewer->GetValue());
        _option->setLoadPath(m_LoadPath->GetValue().ToStdString());
        _option->setSavePath(m_SavePath->GetValue().ToStdString());
        _option->setScriptPath(m_ScriptPath->GetValue().ToStdString());
        _option->setProcPath(m_ProcPath->GetValue().ToStdString());
        _option->setPlotOutputPath(m_PlotPath->GetValue().ToStdString());
        _option->setprecision(m_precision->GetValue());
        _option->setDefaultPlotFont(m_defaultFont->GetValue().ToStdString());
        _option->setWindowBufferSize(0, m_termHistory->GetValue());
        _option->setAutoSaveInterval(m_autosaveinterval->GetValue());
		m_options->SetTerminalHistorySize(m_termHistory->GetValue());

		wxString selectedPrintStyleString = m_printStyle->GetValue();

		if(selectedPrintStyleString == _guilang.get("GUI_OPTIONS_PRINT_COLOR"))
		{
			m_options->SetPrintStyle(wxSTC_PRINT_COLOURONWHITE);
		}
		else
		{
			m_options->SetPrintStyle(wxSTC_PRINT_BLACKONWHITE);
		}

		m_options->SetShowToolbarText(m_showToolbarText->IsChecked());
		m_options->SetLineNumberPrinting(m_cbPrintLineNumbers->IsChecked());
		m_options->SetSaveSession(m_saveSession->IsChecked());
		///m_options->SetCombineWatchWindow(m_chkCombineWatchWindow->IsChecked());
		///m_options->SetShowCompileCommands(m_chkShowCompileCommands->IsChecked());

        synchronizeColors();
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
	/**Permission* perms = m_options->GetPerms();

	UpdateChecklist();

	m_hostname->SetValue(m_options->GetHostname());
	m_username->SetValue(m_options->GetUsername());
	wxString password = m_options->GetPassphrase();
	m_password1->SetValue(password);
	m_password2->SetValue(password);*/

	//m_txtMingwPath->SetValue(m_options->GetMingwPath());

	///m_authCodeLabel->SetLabel(perms->GetAuthCode());

	wxString printStyleString;
	if(m_options->GetPrintStyle() == wxSTC_PRINT_COLOURONWHITE)
	{
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_COLOR");
	}
	else
	{
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_BW");
	}

	m_printStyle->SetValue(printStyleString);
	m_termHistory->SetValue(_option->getBuffer(1));//m_options->GetTerminalHistorySize());

	m_showToolbarText->SetValue(m_options->GetShowToolbarText());
	m_cbPrintLineNumbers->SetValue(m_options->GetLineNumberPrinting());
    m_saveSession->SetValue(m_options->GetSaveSession());

    m_compactTables->SetValue(_option->getbCompact());
    m_AutoLoadDefines->SetValue(_option->getbDefineAutoLoad());
    m_showGreeting->SetValue(_option->getbGreeting());
    m_LoadCompactTables->SetValue(_option->getbLoadEmptyCols());
    m_ExtendedInfo->SetValue(_option->getbShowExtendedFileInfo());
    m_ShowHints->SetValue(_option->getbShowHints());
    m_CustomLanguage->SetValue(_option->getUseCustomLanguageFiles());
    m_ESCinScripts->SetValue(_option->getbUseESCinScripts());
    m_UseLogfile->SetValue(_option->getbUseLogFile());
    m_UseExternalViewer->SetValue(_option->getUseExternalViewer());
    m_LoadPath->SetValue(_option->getLoadPath());
    m_SavePath->SetValue(_option->getSavePath());
    m_ScriptPath->SetValue(_option->getScriptPath());
    m_ProcPath->SetValue(_option->getProcsPath());
    m_PlotPath->SetValue(_option->getPlotOutputPath());

    m_defaultFont->SetValue(_option->getDefaultPlotFont());
    m_precision->SetValue(_option->getPrecision());
    m_autosaveinterval->SetValue(_option->getAutoSaveInterval());


    for (size_t i = 0; i < m_options->GetStyleIdentifier().size(); i++)
    {
        m_colorOptions.SetStyleForeground(i, m_options->GetSyntaxStyle(i).foreground);
        m_colorOptions.SetStyleBackground(i, m_options->GetSyntaxStyle(i).background);
    }

    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());

    m_foreColor->SetColour(m_colorOptions.GetSyntaxStyle(id).foreground);
    m_backColor->SetColour(m_colorOptions.GetSyntaxStyle(id).background);

	/**m_chkCombineWatchWindow->SetValue(m_options->GetCombineWatchWindow());
	m_chkShowCompileCommands->SetValue(m_options->GetShowCompileCommands());

	m_txtMingwPath->SetValue(m_options->GetMingwBasePath());

	VerifyMingwPath(false);*/

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
    return;
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

