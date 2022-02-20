/////////////////////////////////////////////////////////////////////////////
// Name:        OptionsDialog.h
// Purpose:
// Author:      Mark Erikson
// Modified by:
// Created:     11/23/03 16:02:26
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIONSDIALOG_H_
#define _OPTIONSDIALOG_H_

#ifdef __GNUG__
#pragma interface "OptionsDialog.h"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/spinctrl.h"
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>

#include "../../kernel/core/settings.hpp"
////@end includes


/*!
 * Forward declarations
 */

////@begin forward declarations
class wxNotebook;
class wxSpinCtrl;
////@end forward declarations

class NumeReWindow;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define ID_NOTEBOOK 10001
#define ID_PANELFEATURES 10006
#define ID_CHECKLISTBOX 10007
#define ID_PROFCODE 10004
#define ID_SETAUTHCODE 10012
#define ID_PANELNETWORK 10005
#define ID_HOSTNAME 10008
#define ID_USERNAME 10009
#define ID_PASSWORD1 10010
#define ID_PASSWORD2 10011
#define ID_PANELCOMPILER 10015
#define ID_TEXTMINGWPATH 10014
#define ID_BTN_LOADPATH 10030
#define ID_BTN_SAVEPATH 10031
#define ID_BTN_SCRIPTPATH 10032
#define ID_BTN_PROCPATH 10033
#define ID_BTN_PLOTPATH 10034
#define ID_BTN_LATEXPATH 10035
#define ID_BTNFINDMINGW 10020
#define ID_BUTTON1 10021
#define ID_CHECKBOX1 10023
#define ID_PANELMISC 10016
#define ID_PRINTSTYLE 10017
#define ID_PRINTLINENUMBERS 10013
#define ID_SHOWTOOLBARTEXT 10018
#define ID_COMBINEWATCH 10022
#define ID_SPINCTRL 10019
#define ID_BUTTON_OK 10002
#define ID_BUTTON_CANCEL 10003

#define ID_CLRSPIN 10040
#define ID_CLRPICKR_FORE 10041
#define ID_CLRPICKR_BACK 10042
#define ID_RESETCOLOR 10043
#define ID_DEFAULTBACKGROUND 10044
#define ID_BOLD 10045
#define ID_ITALICS 10046
#define ID_UNDERLINE 10047

#define SYMBOL_OPTIONSDIALOG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_OPTIONSDIALOG_TITLE _("Options")
#define SYMBOL_OPTIONSDIALOG_IDNAME ID_DIALOG
#define SYMBOL_OPTIONSDIALOG_SIZE wxSize(420, 315)
#define SYMBOL_OPTIONSDIALOG_POSITION wxDefaultPosition
////@end control identifiers

//#include <wx/wx.h>

class wxTextCtrl;
class wxButton;
class wxCheckListBox;
class wxString;
class Options;

/////////////////////////////////////////////////
/// \brief This class represents the settings
/// dialog in memory.
/////////////////////////////////////////////////
class OptionsDialog: public wxDialog
{
        DECLARE_CLASS( OptionsDialog )
        DECLARE_EVENT_TABLE()

        void CreateConfigPage();
        void CreatePathPage();
        void CreateEditorPage();
        void CreateStylePage();
        void CreateMiscPage();
        void CreateAnalyzerPage();
        void CreateDebuggerPage();

    public:
        /// Constructors
        OptionsDialog( wxWindow* parent, Options* options, wxWindowID id = -1,  const wxString& caption = _("Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

        /// Creation
        bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

        /// Creates the controls and sizers
        void CreateControls();

        /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK
        void OnButtonOkClick( wxCommandEvent& event );

        /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
        void OnButtonCancelClick( wxCommandEvent& event );
        void OnColorPickerChange(wxColourPickerEvent& event);
        void OnColorTypeChange(wxCommandEvent& event);
        void OnButtonClick(wxCommandEvent& event);
        void OnFontCheckClick(wxCommandEvent& event);
        void synchronizeColors();

        void BrowseForDir(wxTextCtrl* textbox, wxString name);
        bool EvaluateOptions();

        void InitializeDialog();
        void ExitDialog();

        /// Should we show tooltips?
        static bool ShowToolTips();

    ////@begin OptionsDialog member variables
        wxNotebook* m_optionsNotebook;
        wxCheckListBox* m_checkList;;

        wxCheckBox* m_compactTables;
        wxCheckBox* m_AutoLoadDefines;
        wxCheckBox* m_showGreeting;
        wxCheckBox* m_LoadCompactTables;
        wxComboBox* m_colorType;
        wxColourPickerCtrl* m_foreColor;
        wxColourPickerCtrl* m_backColor;
        wxCheckBox* m_defaultBackground;
        wxButton* m_resetButton;
        wxCheckBox* m_boldCheck;
        wxCheckBox* m_italicsCheck;
        wxCheckBox* m_underlineCheck;
        wxFontPickerCtrl* m_fontPicker;
        wxCheckBox* m_highlightLocalVariables;

        wxCheckBox* m_ExtendedInfo;
        wxCheckBox* m_ShowHints;
        wxCheckBox* m_CustomLanguage;
        wxCheckBox* m_ESCinScripts;
        wxCheckBox* m_UseLogfile;
        wxCheckBox* m_UseExternalViewer;
        wxCheckBox* m_FilePathsInTabs;
        wxTextCtrl* m_LoadPath;
        wxTextCtrl* m_SavePath;
        wxTextCtrl* m_ScriptPath;
        wxTextCtrl* m_ProcPath;
        wxTextCtrl* m_PlotPath;
        wxTextCtrl* m_LaTeXRoot;

        wxComboBox* m_defaultFont;
        wxSpinCtrl* m_precision;
        wxSpinCtrl* m_autosaveinterval;

        wxCheckBox* m_chkShowCompileCommands;
        wxComboBox* m_printStyle;
        wxCheckBox* m_cbPrintLineNumbers;
        wxCheckBox* m_showToolbarText;
        wxCheckBox* m_saveSession;
        wxCheckBox* m_saveBookmarksInSession;
        wxSpinCtrl* m_termHistory;
        wxSpinCtrl* m_caretBlinkTime;
        wxCheckBox* m_useExecuteCommand;
        wxCheckBox* m_formatBeforeSaving;
        wxCheckBox* m_saveBeforeExecuting;
        wxCheckBox* m_useMaskAsDefault;
        wxCheckBox* m_keepBackupFiles;
        wxCheckBox* m_foldDuringLoading;
        wxCheckBox* m_useTabs;
        wxCheckBox* m_homeEndCancels;
        wxCheckBox* m_braceAutoComp;
        wxCheckBox* m_blockAutoComp;
        wxCheckBox* m_quoteAutoComp;
        wxCheckBox* m_lineLengthIndicator;

        wxSpinCtrl* m_debuggerFocusLine;
        wxCheckBox* m_debuggerShowLineNumbers;
        wxCheckBox* m_debuggerShowModules;
        wxCheckBox* m_debuggerShowProcedureArguments;
        wxCheckBox* m_debuggerShowGlobals;
        wxCheckBox* m_debuggerDecodeArguments;
        wxCheckBox* m_debuggerFlashTaskbar;

        wxCheckBox* m_analyzer[Options::ANALYZER_OPTIONS_END];

    ////@end OptionsDialog member variables

        NumeReWindow* m_parentFrame;
        Options* m_options;
        Options m_colorOptions;
};

#endif
    // _OPTIONSDIALOG_H_
