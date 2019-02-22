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

#include "../../common/debug.h"

#include "grouppanel.hpp"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
    EVT_CHECKBOX(ID_BOLD, OptionsDialog::OnStyleButtonClick)
    EVT_CHECKBOX(ID_ITALICS, OptionsDialog::OnStyleButtonClick)
    EVT_CHECKBOX(ID_UNDERLINE, OptionsDialog::OnStyleButtonClick)
    EVT_COMBOBOX(ID_CLRSPIN, OptionsDialog::OnColorTypeChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_FORE, OptionsDialog::OnColorPickerChange)
    EVT_COLOURPICKER_CHANGED(ID_CLRPICKR_BACK, OptionsDialog::OnColorPickerChange)
END_EVENT_TABLE()

std::string replacePathSeparator(const std::string&);

/*!
 * OptionsDialog constructors
 */

OptionsDialog::OptionsDialog()
{
}

OptionsDialog::OptionsDialog(wxWindow* parent, Options* options, wxWindowID id,  const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
	m_parentFrame = static_cast<NumeReWindow*>(parent);
	m_options = options;
    Create(parent, id, caption, pos, size, style);
}

/*!
 * OptionsDialog creator
 */

bool OptionsDialog::Create(wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
    m_optionsNotebook = nullptr;
    m_checkList = nullptr;
    m_chkShowCompileCommands = nullptr;
    m_printStyle = nullptr;
    m_cbPrintLineNumbers = nullptr;
    m_showToolbarText = nullptr;
    m_saveSession = nullptr;
    m_termHistory = nullptr;
    m_caretBlinkTime = nullptr;
    m_formatBeforeSaving = nullptr;
    m_useMaskAsDefault = nullptr;

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
    m_useExecuteCommand = nullptr;

    m_boldCheck = nullptr;
    m_italicsCheck = nullptr;
    m_underlineCheck = nullptr;

    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls();

    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }

    Centre();
    return true;
}

// This member function handles the creation of
// the complete controls and the order of the
// pages
void OptionsDialog::CreateControls()
{
    // Create the main vertical sizer
    wxBoxSizer* optionVSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(optionVSizer);

    // Create the notebook
    m_optionsNotebook = new wxNotebook(this, ID_NOTEBOOK, wxDefaultPosition, wxSize(450, 500), wxNB_DEFAULT | wxNB_TOP | wxNB_MULTILINE);

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

    // Style panel
    CreateStylePage();

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

// This private member function creates the
// "configuration" page
void OptionsDialog::CreateConfigPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_USERINTERFACE"));

    m_compactTables = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_COMPACTTABLES"));
    m_ExtendedInfo = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXTENDEDINFO"));
    m_CustomLanguage = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_CUSTOMLANG"));
    m_ESCinScripts = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_ESCINSCRIPTS"));
    m_UseExternalViewer = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXTERNALVIEWER"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_INTERNALS"));

    m_AutoLoadDefines = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_DEFCTRL"));
    m_LoadCompactTables = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EMPTYCOLS"));
    m_useMaskAsDefault = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_USEMASKASDEFAULT"));
    m_UseLogfile = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LOGFILE"));
    m_useExecuteCommand = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_EXECUTECOMMAND"));
    m_autosaveinterval = CreateSpinControl(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_AUTOSAVE"), 10, 600, 30);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_CONFIG"));
}

// This private member function creates the
// "paths" page
void OptionsDialog::CreatePathPage()
{
    // Create a grouped page
    GroupPanel* panel = new GroupPanel(m_optionsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // Create a group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_OPTIONS_DEFAULTPATHS"));

    m_LoadPath = CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LOADPATH"), ID_BTN_LOADPATH);
    m_SavePath = CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVEPATH"), ID_BTN_SAVEPATH);
    m_ScriptPath = CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SCRIPTPATH"), ID_BTN_SCRIPTPATH);
    m_ProcPath = CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PROCPATH"), ID_BTN_PROCPATH);
    m_PlotPath = CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PLOTPATH"), ID_BTN_PLOTPATH);

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_MISCPATHS"));

    m_LaTeXRoot= CreatePathInput(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_LATEXPATH"), ID_BTN_LATEXPATH);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_PATHS"));
}

// This private member function creates the
// "style" page
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

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_FONTS"), wxHORIZONTAL);

    wxBoxSizer* editorFontSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* plotFontSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* editorFontStaticText = new wxStaticText(group->GetStaticBox(), wxID_STATIC, _guilang.get("GUI_OPTIONS_EDITORFONT"), wxDefaultPosition, wxDefaultSize, 0);
    editorFontSizer->Add(editorFontStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 0);
    wxFont font;
	font.SetNativeFontInfoUserDesc("Consolas 10");
    m_fontPicker = new wxFontPickerCtrl(group->GetStaticBox(), wxID_ANY, font, wxDefaultPosition, wxDefaultSize, wxFNTP_DEFAULT_STYLE);
    editorFontSizer->Add(m_fontPicker, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT| wxBOTTOM, 0);

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

    m_defaultFont = new wxComboBox(group->GetStaticBox(), ID_PRINTSTYLE, "pagella", wxDefaultPosition, wxDefaultSize, defaultFont, wxCB_READONLY );
    m_defaultFont->SetStringSelection("pagella");
    plotFontSizer->Add(m_defaultFont, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, 0);

    group->Add(editorFontSizer, 0, wxALIGN_LEFT | wxALL, ELEMENT_BORDER);
    group->Add(plotFontSizer, 0, wxALIGN_LEFT | wxALL, ELEMENT_BORDER);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, "Style");
}

// This private member function creates the
// "misc" page
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

    m_cbPrintLineNumbers = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_PRINT_LINENUMBERS"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_SAVING"));

    m_formatBeforeSaving = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_FORMAT_BEFORE_SAVING"));
    m_keepBackupFiles = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_KEEP_BACKUP_FILES"));

    // Create a group
    group = panel->createGroup(_guilang.get("GUI_OPTIONS_STARTING"));

    m_saveSession = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_SAVE_SESSION"));
    m_showGreeting = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_GREETING"));
    m_ShowHints = CreateCheckBox(group->GetStaticBox(), group, _guilang.get("GUI_OPTIONS_HINTS"));

    // Those are not part of any group
    m_showToolbarText = CreateCheckBox(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_SHOW_TOOLBARTEXT"));
    m_termHistory = CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_HISTORY_LINES"), 100, 1000, 100);
    m_caretBlinkTime = CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_CARET_BLINK_TIME"), 100, 2000, 500);
    m_precision = CreateSpinControl(panel, panel->getVerticalSizer(), _guilang.get("GUI_OPTIONS_PRECISION"), 1, 14, 7);

    // Add the grouped page to the notebook
    m_optionsNotebook->AddPage(panel, _guilang.get("GUI_OPTIONS_MISC"));
}

// This private member function creates the
// layout for a path input dialog including
// the "choose" button
wxTextCtrl* OptionsDialog::CreatePathInput(wxWindow* parent, wxSizer* sizer, const wxString& description, int buttonID)
{
    // Create the text above the input line
    wxStaticText* inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(inputStaticText, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, ELEMENT_BORDER);

    // Create a horizontal sizer for the input
    // line and the buttoon
    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(horizontalSizer, wxALIGN_LEFT);

    // Create the input line
    wxTextCtrl* textCtrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(310, -1), wxTE_PROCESS_ENTER);

    // Create the button
    wxButton* button = new wxButton(parent, buttonID, _guilang.get("GUI_OPTIONS_CHOOSE"));

    // Add both to the horizontal sizer
    horizontalSizer->Add(textCtrl, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);
    horizontalSizer->Add(button, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);

    return textCtrl;
}

// This private member function creates the
// layout for a usual checkbox
wxCheckBox* OptionsDialog::CreateCheckBox(wxWindow* parent, wxSizer* sizer, const wxString& description)
{
    // Create the checkbox and assign it to the passed sizer
    wxCheckBox* checkBox = new wxCheckBox(parent, wxID_ANY, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);

    return checkBox;
}

// This private member function creates the
// layout for a spin control including the
// assigned text
wxSpinCtrl* OptionsDialog::CreateSpinControl(wxWindow* parent, wxSizer* sizer, const wxString& description, int nMin, int nMax, int nInitial)
{
    // Create a horizontal sizer for the
    // spin control and its assigned text
    wxBoxSizer* spinCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(spinCtrlSizer, 0, wxALIGN_LEFT|wxALL, 0);

    // Create the spin control
    wxSpinCtrl* spinCtrl = new wxSpinCtrl(parent, wxID_ANY, _T("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, nMin, nMax, nInitial);

    // Create the assigned static text
    wxStaticText* spinCtrlStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);

    // Add both to the horizontal sizer
    spinCtrlSizer->Add(spinCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, ELEMENT_BORDER);
    spinCtrlSizer->Add(spinCtrlStaticText, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);

    return spinCtrl;
}

/*!
 * Should we show tooltips?
 */

bool OptionsDialog::ShowToolTips()
{
  return TRUE;
}

void OptionsDialog::OnButtonOkClick( wxCommandEvent& event )
{
    event.Skip();

	ExitDialog();
}


void OptionsDialog::synchronizeColors()
{
    for (size_t i = 0; i < m_colorOptions.GetStyleIdentifier().size(); i++)
    {
        m_options->SetStyleForeground(i, m_colorOptions.GetSyntaxStyle(i).foreground);
        m_options->SetStyleBackground(i, m_colorOptions.GetSyntaxStyle(i).background);
        m_options->SetStyleDefaultBackground(i, m_colorOptions.GetSyntaxStyle(i).defaultbackground);
        m_options->SetStyleBold(i, m_colorOptions.GetSyntaxStyle(i).bold);
        m_options->SetStyleItalics(i, m_colorOptions.GetSyntaxStyle(i).italics);
        m_options->SetStyleUnderline(i, m_colorOptions.GetSyntaxStyle(i).underline);
    }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
 */

void OptionsDialog::OnButtonCancelClick(wxCommandEvent& event)
{
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
    m_defaultBackground->SetValue(m_colorOptions.GetSyntaxStyle(id).defaultbackground);
    m_backColor->Enable(!m_defaultBackground->GetValue());
    m_boldCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).bold);
    m_italicsCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).italics);
    m_underlineCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).underline);
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
        case ID_BTN_LATEXPATH:
            defaultpath = m_LaTeXRoot->GetValue();
            break;
        case ID_RESETCOLOR:
        {
            size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
            m_foreColor->SetColour(m_colorOptions.GetDefaultSyntaxStyle(id).foreground);
            m_backColor->SetColour(m_colorOptions.GetDefaultSyntaxStyle(id).background);
            m_defaultBackground->SetValue(m_colorOptions.GetDefaultSyntaxStyle(id).defaultbackground);
            m_boldCheck->SetValue(m_colorOptions.GetDefaultSyntaxStyle(id).bold);
            m_italicsCheck->SetValue(m_colorOptions.GetDefaultSyntaxStyle(id).italics);
            m_underlineCheck->SetValue(m_colorOptions.GetDefaultSyntaxStyle(id).underline);
            m_colorOptions.SetStyleForeground(id, m_foreColor->GetColour());
            m_colorOptions.SetStyleBackground(id, m_backColor->GetColour());
            m_colorOptions.SetStyleDefaultBackground(id, m_defaultBackground->GetValue());
            m_colorOptions.SetStyleBold(id, m_boldCheck->GetValue());
            m_colorOptions.SetStyleItalics(id, m_italicsCheck->GetValue());
            m_colorOptions.SetStyleUnderline(id, m_underlineCheck->GetValue());
            m_backColor->Enable(!m_defaultBackground->GetValue());
            return;
        }
        case ID_DEFAULTBACKGROUND:
        {
            size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
            m_colorOptions.SetStyleDefaultBackground(id, m_defaultBackground->GetValue());
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

void OptionsDialog::OnStyleButtonClick(wxCommandEvent& event)
{
    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());
    switch (event.GetId())
    {
        case ID_BOLD:
            m_colorOptions.SetStyleBold(id, m_boldCheck->GetValue());
            break;
        case ID_ITALICS:
            m_colorOptions.SetStyleItalics(id, m_italicsCheck->GetValue());
            break;
        case ID_UNDERLINE:
            m_colorOptions.SetStyleUnderline(id, m_underlineCheck->GetValue());
            break;
    }
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
	wxString outputMessage = wxEmptyString;

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
    _option->setUseExecuteCommand(m_useExecuteCommand->GetValue());
    _option->setLoadPath(m_LoadPath->GetValue().ToStdString());
    _option->setSavePath(m_SavePath->GetValue().ToStdString());
    _option->setScriptPath(m_ScriptPath->GetValue().ToStdString());
    _option->setProcPath(m_ProcPath->GetValue().ToStdString());
    _option->setPlotOutputPath(m_PlotPath->GetValue().ToStdString());
    _option->setprecision(m_precision->GetValue());
    _option->setDefaultPlotFont(m_defaultFont->GetValue().ToStdString());
    _option->setWindowBufferSize(0, m_termHistory->GetValue());
    _option->setAutoSaveInterval(m_autosaveinterval->GetValue());
    _option->setUseMaskAsDefault(m_useMaskAsDefault->GetValue());
    m_options->SetTerminalHistorySize(m_termHistory->GetValue());
    m_options->SetCaretBlinkTime(m_caretBlinkTime->GetValue());

    wxString selectedPrintStyleString = m_printStyle->GetValue();

    if(selectedPrintStyleString == _guilang.get("GUI_OPTIONS_PRINT_COLOR"))
    {
        m_options->SetPrintStyle(wxSTC_PRINT_COLOURONWHITE);
    }
    else
    {
        m_options->SetPrintStyle(wxSTC_PRINT_BLACKONWHITE);
    }

    m_options->SetLaTeXRoot(m_LaTeXRoot->GetValue());

    m_options->SetShowToolbarText(m_showToolbarText->IsChecked());
    m_options->SetLineNumberPrinting(m_cbPrintLineNumbers->IsChecked());
    m_options->SetSaveSession(m_saveSession->IsChecked());
    m_options->SetFormatBeforeSaving(m_formatBeforeSaving->IsChecked());
    m_options->SetEditorFont(m_fontPicker->GetSelectedFont());
    m_options->SetKeepBackupFile(m_keepBackupFiles->IsChecked());


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
	{
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_COLOR");
	}
	else
	{
		printStyleString = _guilang.get("GUI_OPTIONS_PRINT_BW");
	}

	m_printStyle->SetValue(printStyleString);
	m_termHistory->SetValue(_option->getBuffer(1));//m_options->GetTerminalHistorySize());
	m_caretBlinkTime->SetValue(m_options->GetCaretBlinkTime());

	m_showToolbarText->SetValue(m_options->GetShowToolbarText());
	m_cbPrintLineNumbers->SetValue(m_options->GetLineNumberPrinting());
    m_saveSession->SetValue(m_options->GetSaveSession());
    m_formatBeforeSaving->SetValue(m_options->GetFormatBeforeSaving());

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
    m_useExecuteCommand->SetValue(_option->getUseExecuteCommand());
    m_useMaskAsDefault->SetValue(_option->getUseMaskAsDefault());
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
        m_colorOptions.SetStyleDefaultBackground(i, m_options->GetSyntaxStyle(i).defaultbackground);
        m_colorOptions.SetStyleBold(i, m_options->GetSyntaxStyle(i).bold);
        m_colorOptions.SetStyleItalics(i, m_options->GetSyntaxStyle(i).italics);
        m_colorOptions.SetStyleUnderline(i, m_options->GetSyntaxStyle(i).underline);
    }

    size_t id = m_colorOptions.GetIdByIdentifier(m_colorType->GetValue());

    m_foreColor->SetColour(m_colorOptions.GetSyntaxStyle(id).foreground);
    m_backColor->SetColour(m_colorOptions.GetSyntaxStyle(id).background);
    m_boldCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).bold);
    m_italicsCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).italics);
    m_underlineCheck->SetValue(m_colorOptions.GetSyntaxStyle(id).underline);
    m_defaultBackground->SetValue(m_colorOptions.GetSyntaxStyle(id).defaultbackground);
    m_fontPicker->SetSelectedFont(m_options->GetEditorFont());
    m_backColor->Enable(!m_defaultBackground->GetValue());
    m_LaTeXRoot->SetValue(m_options->GetLaTeXRoot());
    m_keepBackupFiles->SetValue(m_options->GetKeepBackupFile());

}

