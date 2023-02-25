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

#define ELEMENT_BORDER 5

#include "../../common/CommonHeaders.h"
#include "../../common/Options.h"
#include "../../kernel/core/ui/language.hpp"

#include <wx/checklst.h>
#include <wx/valtext.h>
#include <wx/dirdlg.h>
#include <wx/dir.h>

#include "OptionsDialog.h"
#include "../NumeReWindow.h"

#include "../compositions/grouppanel.hpp"

extern Language _guilang;
/*!
 * OptionsDialog type definition
 */

IMPLEMENT_CLASS( OptionsDialog, wxDialog )

/*!
 * OptionsDialog event table definition
 */

BEGIN_EVENT_TABLE( OptionsDialog, wxDialog )
    EVT_BUTTON( ID_BUTTON_OK, OptionsDialog::OnButtonOkClick )
    EVT_BUTTON( ID_BUTTON_CANCEL, OptionsDialog::OnButtonCancelClick )
    EVT_BUTTON(ID_BTN_LOADPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_SAVEPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_SCRIPTPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_PROCPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_PLOTPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_BTN_LATEXPATH, OptionsDialog::OnButtonClick)
    EVT_BUTTON(ID_RESETCOLOR, OptionsDialog::OnButtonClick)
    EVT_CHECKBOX(ID_DEFAULTBACKGROUND, OptionsDialog::OnButtonClick)
    EVT_CHECKBOX(ID_BOLD, OptionsDialog::OnFontCheckClick)
    EVT_CHECKBOX(ID_ITALICS, OptionsDialog::OnFontCheckClick)
    EVT_CHECKBOX(ID_UNDERLINE, OptionsDialog::OnFontCheckClick)
    EVT_COMBOBOX(ID_CLRSPIN, OptionsDialog::OnColorTypeChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_FORE, OptionsDialog::OnColorPickerChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_BACK, OptionsDialog::OnColorPickerChange)
END_EVENT_TABLE()

std::string replacePathSeparator(const std::string&);



/////////////////////////////////////////////////
/// \brief OptionsDialog constructor. Performs
/// two-step creation.
///
/// \param parent wxWindow*
/// \param options Options*
/// \param id wxWindowID
/// \param caption const wxString&
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
///
/////////////////////////////////////////////////
OptionsDialog::OptionsDialog(wxWindow* parent, Options* options, wxWindowID id,  const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
	m_parentFrame = static_cast<NumeReWindow*>(parent);
	m_options = options;
    Create(parent, id, caption, pos, size, style);
}


/////////////////////////////////////////////////
/// \brief OptionsDialog creator function.
/// Initializes all UI elements in the dialog.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param caption const wxString&
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
/// \return bool
///
/////////////////////////////////////////////////
bool OptionsDialog::Create(wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls();

    if (GetSizer())
        GetSizer()->SetSizeHints(this);

    Centre();
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of the complete controls and the
/// order of the pages.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateControls()
{
    // Create the main vertical sizer
    wxBoxSizer* optionVSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(optionVSizer);

    // Create the notebook
    m_optionsNotebook = new wxNotebook(this, ID_NOTEBOOK, wxDefaultPosition, wxSize(450*g_pixelScale, 520*g_pixelScale), wxNB_DEFAULT | wxNB_TOP | wxNB_MULTILINE);

    // Create the single pages in the following
    // private member functions. This approach
    // reduces the amount of local variables
    //
    // To change the order of the settings pages
    // simply change the order of the following
    // functions
    //
    // Configuration panel
    CreateConfigPage();

    // Path settings panel
    CreatePathPage();

    // Editor settings panel
    CreateEditorPage();

    // Style panel
    CreateStylePage();

    // Static analyzer panel
    CreateAnalyzerPage();

    // Static debugger panel
    CreateDebuggerPage();

    // Misc panel
    CreateMiscPage();

    // Add the notebook to the vertical page sizer
    optionVSizer->Add(m_optionsNotebook, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, ELEMENT_BORDER);

    // Add the buttons
    wxBoxSizer* optionButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    optionVSizer->Add(optionButtonSizer, 0, wxALIGN_RIGHT | wxALL, 0);

    wxButton* okButton = new wxButton(this, ID_BUTTON_OK, _guilang.get("GUI_OPTIONS_OK"), wxDefaultPosition, wxDefaultSize, 0);
    optionButtonSizer->Add(okButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);

    wxButton* cancelButton = new wxButton(this, ID_BUTTON_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"), wxDefaultPosition, wxDefaultSize, 0);
    optionButtonSizer->Add(cancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);

    // end content construction
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "configuration" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateConfigPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_USERINTERFACE"));

    m_compactTables = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_COMPACTTABLES"));
    m_ExtendedInfo = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXTENDEDINFO"));
    m_CustomLanguage = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_CUSTOMLANG"));
    m_ESCinScripts = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ESCINSCRIPTS"));
    m_floatOnParent = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_FLOATONPARENT"));
    //m_UseExternalViewer = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXTERNALVIEWER"));

    group = panel->createGroup(_guilang.get("GUI_OPTIONS_TOOLBAR"));
    m_showToolbarText = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SHOW_TOOLBARTEXT"));
    m_enableToolbarStretch = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ENABLE_TOOLBARSTRETCH"));
    wxStaticText* iconStyleText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_ICONSTYLE"), wxDefaultPosition, wxDefaultSize, 0);
    group->Add(iconStyleText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5);
    wxArrayString iconStyles;
    iconStyles.Add("Purist");
    iconStyles.Add("Focused");
    iconStyles.Add("Colorful");

    m_iconStyle = new wxComboBox( group->GetStaticBox(), wxID_ANY, "Focused", wxDefaultPosition, wxDefaultSize, iconStyles, wxCB_READONLY );
    m_iconStyle->SetStringSelection("Focused");
    group->Add(m_iconStyle, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_INTERNALS"));

    m_AutoLoadDefines = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEFCTRL"));
    m_LoadCompactTables = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EMPTYCOLS"));
    m_useMaskAsDefault = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_USEMASKASDEFAULT"));
    m_UseLogfile = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LOGFILE"));
    m_useExecuteCommand = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXECUTECOMMAND"));
    m_alwaysReferenceTables = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ALWAYSREFERENCETABLES"));
    m_autosaveinterval = panel->CreateSpinControl(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_AUTOSAVE"), 10, 600, 30);

    // Enable scrolling for this page, because it might be very large
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_CONFIG"));
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "paths" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreatePathPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_DEFAULTPATHS"));

    m_LoadPath = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LOADPATH"), ID_BTN_LOADPATH);
    m_SavePath = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVEPATH"), ID_BTN_SAVEPATH);
    m_ScriptPath = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SCRIPTPATH"), ID_BTN_SCRIPTPATH);
    m_ProcPath = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PROCPATH"), ID_BTN_PROCPATH);
    m_PlotPath = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PLOTPATH"), ID_BTN_PLOTPATH);

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_MISCPATHS"));

    m_LaTeXRoot = panel->CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LATEXPATH"), ID_BTN_LATEXPATH);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_PATHS"));
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "editor" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateEditorPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_USERINTERFACE"));

    m_foldDuringLoading = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_FOLD_DURING_LOADING"));
    m_FilePathsInTabs = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SHOW_FILEPATHS"));
    m_IconsOnTabs = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SHOW_ICONS_ON_TABS"));
    m_useTabs = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_USE_TABS"));
    m_lineLengthIndicator = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SHOW_LINE_LENGTH_INDICATOR"));
    m_caretBlinkTime = panel->CreateSpinControl(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_CARET_BLINK_TIME"), 100, 2000, 500);

    group = panel->createGroup(_guilang.get("GUI_OPTIONS_AUTOCOMPLETION"));
    m_braceAutoComp = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_BRACE_AUTOCOMP"));
    m_quoteAutoComp = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_QUOTE_AUTOCOMP"));
    m_blockAutoComp = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_BLOCK_AUTOCOMP"));
    m_smartSense = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SMARTSENSE_AUTOCOMP"));
    m_homeEndCancels = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_HOME_END_CANCELS"));

    // Enable scrolling for this page, because it might be very large
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, "Editor");
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "style" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateStylePage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_SYNTAXHIGHLIGHTING"));

    wxFlexGridSizer* colorGroupSizer = new wxFlexGridSizer(2, 0, 5);
    wxBoxSizer* colorGroupHSizer = new wxBoxSizer(wxHORIZONTAL);
    wxArrayString styles = m_options->GetStyleIdentifier();

    m_colorType = new wxComboBox( group->GetStaticBox(), ID_CLRSPIN, styles[0], wxDefaultPosition, wxDefaultSize, styles, wxCB_READONLY );
    m_colorType->SetStringSelection(styles[0]);

    m_foreColor = new wxColourPickerCtrl(group->GetStaticBox(), ID_CLRPICKR_FORE, m_options->GetSyntaxStyle(0).foreground);
    m_backColor = new wxColourPickerCtrl(group->GetStaticBox(), ID_CLRPICKR_BACK, m_options->GetSyntaxStyle(0).background);

    m_resetButton = new wxButton(group->GetStaticBox(), ID_RESETCOLOR, _guilang.get("GUI_OPTIONS_RESETHIGHLIGHT"), wxDefaultPosition, wxDefaultSize, 0);
    m_defaultBackground = new wxCheckBox(group->GetStaticBox(), ID_DEFAULTBACKGROUND, _guilang.get("GUI_OPTIONS_DEFAULTBACKGROUND"));

    m_boldCheck = new wxCheckBox(group->GetStaticBox(), ID_BOLD, _guilang.get("GUI_OPTIONS_BOLD"));
    m_italicsCheck = new wxCheckBox(group->GetStaticBox(), ID_ITALICS, _guilang.get("GUI_OPTIONS_ITALICS"));
    m_underlineCheck = new wxCheckBox(group->GetStaticBox(), ID_UNDERLINE, _guilang.get("GUI_OPTIONS_UNDERLINE"));

    colorGroupHSizer->Add(m_colorType, 0, wxALIGN_CENTER | wxRIGHT, ELEMENT_BORDER);
    colorGroupHSizer->Add(m_resetButton, 0, wxALIGN_LEFT | wxLEFT, ELEMENT_BORDER);

    colorGroupSizer->Add(m_foreColor, 1, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);
    colorGroupSizer->Add(colorGroupHSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);
    colorGroupSizer->Add(m_backColor, 1, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);
    colorGroupSizer->Add(m_defaultBackground, 1, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);

    group->Add(colorGroupSizer, 0, wxALIGN_LEFT, ELEMENT_BORDER);

    wxBoxSizer* fontStyleSize = new wxBoxSizer(wxHORIZONTAL);

    fontStyleSize->Add(m_boldCheck, 1, wxALIGN_LEFT | wxALL, 0);
    fontStyleSize->Add(m_italicsCheck, 1, wxALIGN_LEFT | wxALL, 0);
    fontStyleSize->Add(m_underlineCheck, 1, wxALIGN_LEFT | wxALL, 0);

    group->Add(fontStyleSize, 0, wxALIGN_LEFT | wxALL, ELEMENT_BORDER);
    m_highlightLocalVariables = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_HIGHLIGHTLOCALVARIABLES"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_FONTS"), wxHORIZONTAL);

    wxBoxSizer* fontSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* plotFontSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* editorFontStaticText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_EDITORFONT"), wxDefaultPosition, wxDefaultSize, 0);
    fontSizer->Add(editorFontStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 0);
    wxFont font;
	font.SetNativeFontInfoUserDesc("Consolas 10");
    m_fontPicker = new wxFontPickerCtrl(group->GetStaticBox(), wxID_ANY, font, wxDefaultPosition, wxSize(200,-1), wxFNTP_DEFAULT_STYLE);
    fontSizer->Add(m_fontPicker, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT| wxBOTTOM, 0);

    wxStaticText* terminalFontStaticText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_TERMINALFONT"), wxDefaultPosition, wxDefaultSize, 0);
    fontSizer->Add(terminalFontStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 0);
	font.SetNativeFontInfoUserDesc("Consolas 8");
    m_fontPickerTerminal = new wxFontPickerCtrl(group->GetStaticBox(), wxID_ANY, font, wxDefaultPosition, wxSize(200,-1), wxFNTP_DEFAULT_STYLE);
    fontSizer->Add(m_fontPickerTerminal, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT| wxBOTTOM, 0);

    wxStaticText* historyFontStaticText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_HISTORYFONT"), wxDefaultPosition, wxDefaultSize, 0);
    fontSizer->Add(historyFontStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 0);
    m_fontPickerHistory = new wxFontPickerCtrl(group->GetStaticBox(), wxID_ANY, font, wxDefaultPosition, wxSize(200,-1), wxFNTP_DEFAULT_STYLE);
    fontSizer->Add(m_fontPickerHistory, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT| wxBOTTOM, 0);


    wxStaticText* defaultFontStaticText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _(_guilang.get("GUI_OPTIONS_DEFAULTFONT")), wxDefaultPosition, wxDefaultSize, 0);
    plotFontSizer->Add(defaultFontStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 0);

    wxArrayString defaultFont;
    defaultFont.Add("pagella");
    defaultFont.Add("adventor");
    defaultFont.Add("bonum");
    defaultFont.Add("chorus");
    defaultFont.Add("heros");
    defaultFont.Add("heroscn");
    defaultFont.Add("schola");
    defaultFont.Add("termes");

    m_defaultFont = new wxComboBox(group->GetStaticBox(), ID_PRINTSTYLE, "pagella", wxDefaultPosition, wxSize(200,-1), defaultFont, wxCB_READONLY );
    m_defaultFont->SetStringSelection("pagella");
    plotFontSizer->Add(m_defaultFont, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, 0);

    group->Add(fontSizer, 0, wxALIGN_LEFT | wxALL, ELEMENT_BORDER);
    group->Add(plotFontSizer, 0, wxALIGN_LEFT | wxALL, ELEMENT_BORDER);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, "Style");
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "misc" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateMiscPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_PRINTING"));

    wxStaticText* printingStaticText = new wxStaticText( group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_PRINT"), wxDefaultPosition, wxDefaultSize, 0 );
    group->Add(printingStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, ELEMENT_BORDER);

    wxArrayString m_printStyleStrings;
    m_printStyleStrings.Add(_guilang.get("GUI_OPTIONS_PRINT_BW"));
    m_printStyleStrings.Add(_guilang.get("GUI_OPTIONS_PRINT_COLOR"));

    m_printStyle = new wxComboBox( group->GetStaticBox(), ID_PRINTSTYLE, _guilang.get("GUI_OPTIONS_PRINT_BW"), wxDefaultPosition, wxDefaultSize, m_printStyleStrings, wxCB_READONLY );
    m_printStyle->SetStringSelection(_guilang.get("GUI_OPTIONS_PRINT_BW"));
    group->Add(m_printStyle, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);

    m_cbPrintLineNumbers = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PRINT_LINENUMBERS"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_SAVING"));

    m_formatBeforeSaving = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_FORMAT_BEFORE_SAVING"));
    m_keepBackupFiles = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_KEEP_BACKUP_FILES"));
    m_saveBeforeExecuting = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_AUTOSAVE_BEFORE_EXEC"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_STARTING"));

    m_saveSession = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVE_SESSION"));
    m_saveBookmarksInSession = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVE_BOOKMARKS_IN_SESSION"));
    m_saveSashPositions = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVE_SASH_POSITIONS"));
    m_saveWindowPosition = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVE_WINDOW_POSITION"));
    m_showGreeting = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_GREETING"));
    m_ShowHints = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_HINTS"));

    // Those are not part of any group
    m_termHistory = panel->CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_HISTORY_LINES"), 100, 1000, 100);
    m_precision = panel->CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_PRECISION"), 1, 14, 7);

    // Enable scrolling for this page
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_MISC"));
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "Static analyzer" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateAnalyzerPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_ANALYZER_MAIN"));

    m_analyzer[Options::USE_NOTES] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_USE_NOTES"));
    m_analyzer[Options::USE_WARNINGS] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_USE_WARNINGS"));
    m_analyzer[Options::USE_ERRORS] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_USE_ERRORS"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_ANALYZER_METRICS"));
    m_analyzer[Options::COMMENT_DENSITY] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_COMMENT_DENSITY"));
    m_analyzer[Options::LINES_OF_CODE] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_LINES_OF_CODE"));
    m_analyzer[Options::COMPLEXITY] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_COMPLEXITY"));
    m_analyzer[Options::MAGIC_NUMBERS] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_MAGIC_NUMBERS"));
    m_analyzer[Options::ALWAYS_SHOW_METRICS] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_ALWAYS_SHOW_METRICS"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_ANALYZER_OPTIMIZATION"));
    m_analyzer[Options::INLINE_IF] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_INLINE_IF"));
    m_analyzer[Options::CONSTANT_EXPRESSION] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_CONSTANT_EXPRESSION"));
    m_analyzer[Options::PROGRESS_RUNTIME] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_PROGRESS_RUNTIME"));
    m_analyzer[Options::PROCEDURE_LENGTH] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_PROCEDURE_LENGTH"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_ANALYZER_UNINTENDED_BEHAVIOR"));
    m_analyzer[Options::RESULT_SUPPRESSION] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_RESULT_SUPPRESSION"));
    m_analyzer[Options::RESULT_ASSIGNMENT] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_RESULT_ASSIGNMENT"));
    m_analyzer[Options::UNUSED_VARIABLES] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_UNUSED_VARIABLES"));
    m_analyzer[Options::THISFILE_NAMESPACE] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_THISFILE_NAMESPACE"));
    m_analyzer[Options::SWITCH_FALLTHROUGH] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_SWITCH_FALLTHROUGH"));
    m_analyzer[Options::GLOBAL_VARIABLES] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_GLOBAL_VARIABLES"));
    m_analyzer[Options::MISLEADING_TYPE] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_MISLEADING_TYPE"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_ANALYZER_STYLE"));
    m_analyzer[Options::TYPE_ORIENTATION] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_TYPE_ORIENTATION"));
    m_analyzer[Options::ARGUMENT_UNDERSCORE] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_ARGUMENT_UNDERSCORE"));
    m_analyzer[Options::VARIABLE_LENGTH] = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ANALYZER_VARIABLE_LENGTH"));

    // Enable scrolling for this page, because it might be very large
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_ANALYZER"));

}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the "debugger" page.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::CreateDebuggerPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_STACKTRACE"));

    m_debuggerShowLineNumbers = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEBUGGER_SHOW_LINENUMBERS"));
    m_debuggerShowModules = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEBUGGER_SHOW_MODULES"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_VARVIEWER"));

    m_debuggerShowProcedureArguments = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEBUGGER_SHOW_ARGUMENTS"));
    m_debuggerDecodeArguments = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEBUGGER_DECODE_ARGUMENTS"));
    m_debuggerShowGlobals = panel->CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEBUGGER_SHOW_GLOBALS"));

    // Those are not part of any group
    m_debuggerFlashTaskbar = panel->CreateCheckBox(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_DEBUGGER_FLASH_TASKBAR"));
    m_alwaysPointToError = panel->CreateCheckBox(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_DEBUGGER_POINT_TO_ERROR"));
    m_debuggerFocusLine = panel->CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_DEBUGGER_FOCUS_LINE"), 1, 30, 10);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_DEBUGGER"));
}

/*!
 * Should we show tooltips?
 */

bool OptionsDialog::ShowToolTips()
{
  return TRUE;
}


/////////////////////////////////////////////////
/// \brief Event handler, which gets fired, when
/// the user clicks the OK button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::OnButtonOkClick( wxCommandEvent& event )
{
    event.Skip();

	ExitDialog();
}


/////////////////////////////////////////////////
/// \brief Event handler, which gets fired, when
/// the user clicks the Cancel button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::OnButtonCancelClick(wxCommandEvent& event)
{
    event.Skip();
	EndModal(wxID_CANCEL);
	m_optionsNotebook->SetSelection(0);
}


/////////////////////////////////////////////////
/// \brief Copies the selected syntax styles to
/// the Options class.
///
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::synchronizeColors()
{
    for (size_t i = 0; i < m_colorOptions.GetStyleIdentifier().size(); i++)
    {
        m_options->SetSyntaxStyle(i, m_colorOptions.GetSyntaxStyle(i));
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for changing the colours.
///
/// \param event wxColourPickerEvent&
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::OnColorPickerChange(wxColourPickerEvent& event)
{
    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
    SyntaxStyles style = m_colorOptions.GetSyntaxStyle(id);

    if (event.GetId() == ID_CLRPICKR_FORE)
        style.foreground = m_foreColor->GetColour();
    else
        style.background = m_backColor->GetColour();

    m_colorOptions.SetSyntaxStyle(id, style);
}


/////////////////////////////////////////////////
/// \brief Event handler for switching the syntax
/// elements for selecting the styling.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::OnColorTypeChange(wxCommandEvent& event)
{
    SyntaxStyles style = m_colorOptions.GetSyntaxStyle(m_colorOptions.GetIdByIdentifier(m_colorType->GetValue()));

    m_foreColor->SetColour(style.foreground);
    m_backColor->SetColour(style.background);
    m_defaultBackground->SetValue(style.defaultbackground);
    m_backColor->Enable(!m_defaultBackground->GetValue());
    m_boldCheck->SetValue(style.bold);
    m_italicsCheck->SetValue(style.italics);
    m_underlineCheck->SetValue(style.underline);
}


/////////////////////////////////////////////////
/// \brief Button event handler for all other
/// buttons.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
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
        case ID_BTN_LATEXPATH:
            defaultpath = m_LaTeXRoot->GetValue();
            break;
        case ID_RESETCOLOR:
        {
            size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
            SyntaxStyles style = m_colorOptions.GetDefaultSyntaxStyle(id);
            m_foreColor->SetColour(style.foreground);
            m_backColor->SetColour(style.background);
            m_defaultBackground->SetValue(style.defaultbackground);
            m_boldCheck->SetValue(style.bold);
            m_italicsCheck->SetValue(style.italics);
            m_underlineCheck->SetValue(style.underline);
            m_colorOptions.SetSyntaxStyle(id, style);
            m_backColor->Enable(!m_defaultBackground->GetValue());
            return;
        }
        case ID_DEFAULTBACKGROUND:
        {
            size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
            SyntaxStyles style = m_colorOptions.GetDefaultSyntaxStyle(id);
            style.defaultbackground = m_defaultBackground->GetValue();
            m_colorOptions.SetSyntaxStyle(id, style);
            m_backColor->Enable(!m_defaultBackground->GetValue());
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
        case ID_BTN_LATEXPATH:
            m_LaTeXRoot->SetValue(replacePathSeparator(dialog.GetPath().ToStdString()));
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for the font style check
/// boxes.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void OptionsDialog::OnFontCheckClick(wxCommandEvent& event)
{
    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
    SyntaxStyles style = m_colorOptions.GetSyntaxStyle(id);

    switch (event.GetId())
    {
        case ID_BOLD:
            style.bold = m_boldCheck->GetValue();
            break;
        case ID_ITALICS:
            style.italics = m_italicsCheck->GetValue();
            break;
        case ID_UNDERLINE:
            style.underline = m_underlineCheck->GetValue();
            break;
    }

    m_colorOptions.SetSyntaxStyle(id, style);
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
    if (EvaluateOptions())
    {
        EndModal(wxID_OK);
        m_optionsNotebook->SetSelection(0);
    }

}


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
    std::map<std::string, SettingsValue>& mSettings = m_options->getSettings();

    mSettings[SETTING_B_COMPACT].active() = m_compactTables->IsChecked();
    mSettings[SETTING_B_DEFCONTROL].active() = m_AutoLoadDefines->IsChecked();
    mSettings[SETTING_B_GREETING].active() = m_showGreeting->IsChecked();
    mSettings[SETTING_B_LOADEMPTYCOLS].active() = m_LoadCompactTables->IsChecked();
    mSettings[SETTING_B_EXTENDEDFILEINFO].active() = m_ExtendedInfo->IsChecked();
    mSettings[SETTING_B_SHOWHINTS].active() = m_ShowHints->IsChecked();
    mSettings[SETTING_B_USECUSTOMLANG].active() = m_CustomLanguage->IsChecked();
    mSettings[SETTING_B_USEESCINSCRIPTS].active() = m_ESCinScripts->IsChecked();
    mSettings[SETTING_B_FLOATONPARENT].active() = m_floatOnParent->IsChecked();
    mSettings[SETTING_B_LOGFILE].active() = m_UseLogfile->IsChecked();
    //mSettings[SETTING_B_EXTERNALDOCWINDOW].active() = m_UseExternalViewer->IsChecked();
    mSettings[SETTING_B_ENABLEEXECUTE].active() = m_useExecuteCommand->IsChecked();
    mSettings[SETTING_B_MASKDEFAULT].active() = m_useMaskAsDefault->IsChecked();
    mSettings[SETTING_B_TABLEREFS].active() = m_alwaysReferenceTables->IsChecked();
    mSettings[SETTING_B_DECODEARGUMENTS].active() = m_debuggerDecodeArguments->IsChecked();
    mSettings[SETTING_S_LOADPATH].stringval() = m_LoadPath->GetValue().ToStdString();
    mSettings[SETTING_S_SAVEPATH].stringval() = m_SavePath->GetValue().ToStdString();
    mSettings[SETTING_S_SCRIPTPATH].stringval() = m_ScriptPath->GetValue().ToStdString();
    mSettings[SETTING_S_PROCPATH].stringval() = m_ProcPath->GetValue().ToStdString();
    mSettings[SETTING_S_PLOTPATH].stringval() = m_PlotPath->GetValue().ToStdString();
    mSettings[SETTING_S_PLOTFONT].stringval() = m_defaultFont->GetValue().ToStdString();
    mSettings[SETTING_V_PRECISION].value() = m_precision->GetValue();
    mSettings[SETTING_V_BUFFERSIZE].value() = m_termHistory->GetValue();
    mSettings[SETTING_V_AUTOSAVE].value() = m_autosaveinterval->GetValue();

    mSettings[SETTING_V_CARETBLINKTIME].value() = m_caretBlinkTime->GetValue();
    mSettings[SETTING_S_LATEXROOT].stringval() = m_LaTeXRoot->GetValue().ToStdString();
    mSettings[SETTING_B_TOOLBARTEXT].active() = m_showToolbarText->IsChecked();
    mSettings[SETTING_B_TOOLBARSTRETCH].active() = m_enableToolbarStretch->IsChecked();
    mSettings[SETTING_B_PATHSONTABS].active() = m_FilePathsInTabs->IsChecked();
    mSettings[SETTING_B_ICONSONTABS].active() = m_IconsOnTabs->IsChecked();
    mSettings[SETTING_B_PRINTLINENUMBERS].active() = m_cbPrintLineNumbers->IsChecked();
    mSettings[SETTING_B_SAVESESSION].active() = m_saveSession->IsChecked();
    mSettings[SETTING_B_SAVEBOOKMARKS].active() = m_saveBookmarksInSession->IsChecked();
    mSettings[SETTING_B_FORMATBEFORESAVING].active() = m_formatBeforeSaving->IsChecked();
    mSettings[SETTING_B_USEREVISIONS].active() = m_keepBackupFiles->IsChecked();
    mSettings[SETTING_B_FOLDLOADEDFILE].active() = m_foldDuringLoading->IsChecked();
    mSettings[SETTING_V_FOCUSEDLINE].value() = m_debuggerFocusLine->GetValue();
    mSettings[SETTING_B_GLOBALVARS].active() = m_debuggerShowGlobals->IsChecked();
    mSettings[SETTING_B_LINESINSTACK].active() = m_debuggerShowLineNumbers->IsChecked();
    mSettings[SETTING_B_MODULESINSTACK].active() = m_debuggerShowModules->IsChecked();
    mSettings[SETTING_B_PROCEDUREARGS].active() = m_debuggerShowProcedureArguments->IsChecked();
    mSettings[SETTING_B_FLASHTASKBAR].active() = m_debuggerFlashTaskbar->IsChecked();
    mSettings[SETTING_B_HIGHLIGHTLOCALS].active() = m_highlightLocalVariables->IsChecked();
    mSettings[SETTING_B_USETABS].active() = m_useTabs->IsChecked();
    mSettings[SETTING_B_HOMEENDCANCELS].active() = m_homeEndCancels->IsChecked();
    mSettings[SETTING_B_BRACEAUTOCOMP].active() = m_braceAutoComp->IsChecked();
    mSettings[SETTING_B_BLOCKAUTOCOMP].active() = m_blockAutoComp->IsChecked();
    mSettings[SETTING_B_QUOTEAUTOCOMP].active() = m_quoteAutoComp->IsChecked();
    mSettings[SETTING_B_SMARTSENSE].active() = m_smartSense->IsChecked();
    mSettings[SETTING_B_AUTOSAVEEXECUTION].active() = m_saveBeforeExecuting->IsChecked();
    mSettings[SETTING_B_LINELENGTH].active() = m_lineLengthIndicator->IsChecked();
    mSettings[SETTING_B_POINTTOERROR].active() = m_alwaysPointToError->IsChecked();
    mSettings[SETTING_B_SAVESASHS].active() = m_saveSashPositions->IsChecked();
    mSettings[SETTING_B_SAVEWINDOWSIZE].active() = m_saveWindowPosition->IsChecked();
    mSettings[SETTING_S_TOOLBARICONSTYLE].stringval() = m_iconStyle->GetValue();

    wxString selectedPrintStyleString = m_printStyle->GetValue();

    if (selectedPrintStyleString == _guilang.get("GUI_OPTIONS_PRINT_COLOR"))
        m_options->SetPrintStyle(wxSTC_PRINT_COLOURONWHITE);
    else
        m_options->SetPrintStyle(wxSTC_PRINT_BLACKONWHITE);

    mSettings[SETTING_S_EDITORFONT].stringval() = Options::toString(m_fontPicker->GetSelectedFont());

    if (m_fontPickerTerminal->GetSelectedFont().IsFixedWidth())
        mSettings[SETTING_S_TERMINALFONT].stringval() = Options::toString(m_fontPickerTerminal->GetSelectedFont());

    mSettings[SETTING_S_HISTORYFONT].stringval() = Options::toString(m_fontPickerHistory->GetSelectedFont());

    for (int i = 0; i < Options::ANALYZER_OPTIONS_END; i++)
    {
        m_options->SetAnalyzerOption((Options::AnalyzerOptions)i, m_analyzer[i]->IsChecked());
    }

    synchronizeColors();

	return true;
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
	wxString printStyleString;

	if (m_options->GetPrintStyle() == wxSTC_PRINT_COLOURONWHITE)
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_COLOR");
	else
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_BW");

	std::map<std::string, SettingsValue>& mSettings = m_options->getSettings();

	m_printStyle->SetValue(printStyleString);
	m_iconStyle->SetValue(mSettings[SETTING_S_TOOLBARICONSTYLE].stringval());
	m_termHistory->SetValue(mSettings[SETTING_V_BUFFERSIZE].value());
	m_caretBlinkTime->SetValue(mSettings[SETTING_V_CARETBLINKTIME].value());
	m_showToolbarText->SetValue(mSettings[SETTING_B_TOOLBARTEXT].active());
	m_enableToolbarStretch->SetValue(mSettings[SETTING_B_TOOLBARSTRETCH].active());
	m_FilePathsInTabs->SetValue(mSettings[SETTING_B_PATHSONTABS].active());
	m_IconsOnTabs->SetValue(mSettings[SETTING_B_ICONSONTABS].active());
	m_cbPrintLineNumbers->SetValue(mSettings[SETTING_B_PRINTLINENUMBERS].active());
    m_saveSession->SetValue(mSettings[SETTING_B_SAVESESSION].active());
    m_saveBookmarksInSession->SetValue(mSettings[SETTING_B_SAVEBOOKMARKS].active());
    m_formatBeforeSaving->SetValue(mSettings[SETTING_B_FORMATBEFORESAVING].active());
    m_compactTables->SetValue(mSettings[SETTING_B_COMPACT].active());
    m_AutoLoadDefines->SetValue(mSettings[SETTING_B_DEFCONTROL].active());
    m_showGreeting->SetValue(mSettings[SETTING_B_GREETING].active());
    m_LoadCompactTables->SetValue(mSettings[SETTING_B_LOADEMPTYCOLS].active());
    m_ExtendedInfo->SetValue(mSettings[SETTING_B_EXTENDEDFILEINFO].active());
    m_ShowHints->SetValue(mSettings[SETTING_B_SHOWHINTS].active());
    m_CustomLanguage->SetValue(mSettings[SETTING_B_USECUSTOMLANG].active());
    m_ESCinScripts->SetValue(mSettings[SETTING_B_USEESCINSCRIPTS].active());
    m_floatOnParent->SetValue(mSettings[SETTING_B_FLOATONPARENT].active());
    m_UseLogfile->SetValue(mSettings[SETTING_B_LOGFILE].active());
    //m_UseExternalViewer->SetValue(mSettings[SETTING_B_EXTERNALDOCWINDOW].active());
    m_useExecuteCommand->SetValue(mSettings[SETTING_B_ENABLEEXECUTE].active());
    m_useMaskAsDefault->SetValue(mSettings[SETTING_B_MASKDEFAULT].active());
    m_alwaysReferenceTables->SetValue(mSettings[SETTING_B_TABLEREFS].active());
    m_LoadPath->SetValue(mSettings[SETTING_S_LOADPATH].stringval());
    m_SavePath->SetValue(mSettings[SETTING_S_SAVEPATH].stringval());
    m_ScriptPath->SetValue(mSettings[SETTING_S_SCRIPTPATH].stringval());
    m_ProcPath->SetValue(mSettings[SETTING_S_PROCPATH].stringval());
    m_PlotPath->SetValue(mSettings[SETTING_S_PLOTPATH].stringval());
    m_defaultFont->SetValue(mSettings[SETTING_S_PLOTFONT].stringval());

    m_precision->SetValue(mSettings[SETTING_V_PRECISION].value());
    m_autosaveinterval->SetValue(mSettings[SETTING_V_AUTOSAVE].value());
    m_LaTeXRoot->SetValue(mSettings[SETTING_S_LATEXROOT].stringval());

    for (size_t i = 0; i < m_options->GetStyleIdentifier().size(); i++)
    {
        m_colorOptions.SetSyntaxStyle(i, m_options->GetSyntaxStyle(i));
    }

    SyntaxStyles style = m_colorOptions.GetSyntaxStyle(m_colorOptions.GetIdByIdentifier(m_colorType->GetValue()));

    m_foreColor->SetColour(style.foreground);
    m_backColor->SetColour(style.background);
    m_boldCheck->SetValue(style.bold);
    m_italicsCheck->SetValue(style.italics);
    m_underlineCheck->SetValue(style.underline);
    m_defaultBackground->SetValue(style.defaultbackground);
    m_backColor->Enable(!m_defaultBackground->GetValue());

    m_fontPicker->SetSelectedFont(Options::toFont(mSettings[SETTING_S_EDITORFONT].stringval()));
    m_fontPickerTerminal->SetSelectedFont(Options::toFont(mSettings[SETTING_S_TERMINALFONT].stringval()));
    m_fontPickerHistory->SetSelectedFont(Options::toFont(mSettings[SETTING_S_HISTORYFONT].stringval()));

    m_keepBackupFiles->SetValue(mSettings[SETTING_B_USEREVISIONS].active());
    m_foldDuringLoading->SetValue(mSettings[SETTING_B_FOLDLOADEDFILE].active());
    m_highlightLocalVariables->SetValue(mSettings[SETTING_B_HIGHLIGHTLOCALS].active());
    m_useTabs->SetValue(mSettings[SETTING_B_USETABS].active());
    m_homeEndCancels->SetValue(mSettings[SETTING_B_HOMEENDCANCELS].active());
    m_braceAutoComp->SetValue(mSettings[SETTING_B_BRACEAUTOCOMP].active());
    m_blockAutoComp->SetValue(mSettings[SETTING_B_BLOCKAUTOCOMP].active());
    m_quoteAutoComp->SetValue(mSettings[SETTING_B_QUOTEAUTOCOMP].active());
    m_smartSense->SetValue(mSettings[SETTING_B_SMARTSENSE].active());
    m_saveBeforeExecuting->SetValue(mSettings[SETTING_B_AUTOSAVEEXECUTION].active());
    m_lineLengthIndicator->SetValue(mSettings[SETTING_B_LINELENGTH].active());
    m_alwaysPointToError->SetValue(mSettings[SETTING_B_POINTTOERROR].active());
    m_saveSashPositions->SetValue(mSettings[SETTING_B_SAVESASHS].active());
    m_saveWindowPosition->SetValue(mSettings[SETTING_B_SAVEWINDOWSIZE].active());

    m_debuggerFocusLine->SetValue(mSettings[SETTING_V_FOCUSEDLINE].value());
    m_debuggerDecodeArguments->SetValue(mSettings[SETTING_B_DECODEARGUMENTS].active());
    m_debuggerShowGlobals->SetValue(mSettings[SETTING_B_GLOBALVARS].active());
    m_debuggerShowLineNumbers->SetValue(mSettings[SETTING_B_LINESINSTACK].active());
    m_debuggerShowModules->SetValue(mSettings[SETTING_B_MODULESINSTACK].active());
    m_debuggerShowProcedureArguments->SetValue(mSettings[SETTING_B_PROCEDUREARGS].active());
    m_debuggerFlashTaskbar->SetValue(mSettings[SETTING_B_FLASHTASKBAR].active());

    for (int i = 0; i < Options::ANALYZER_OPTIONS_END; i++)
    {
        m_analyzer[i]->SetValue(m_options->GetAnalyzerOption((Options::AnalyzerOptions)i));
    }

}

