#ifndef OPTIONS_H
#define OPTIONS_H

#include <wx/wx.h>
#include <wx/validate.h>
#include <wx/valgen.h>
#include <wx/valtext.h>
#include <wx/fileconf.h>

#include <vector>

#include "datastructures.h"

using namespace std;


// copied from stc.h
// PrintColourMode - force black text on white background for printing.
#define wxSTC_PRINT_BLACKONWHITE 2

// PrintColourMode - text stays coloured, but all background is forced to be white for printing.
#define wxSTC_PRINT_COLOURONWHITE 3

struct SyntaxStyles
{
    SyntaxStyles() : foreground(*wxBLACK), background(*wxWHITE), bold(false), italics(false), underline(false), defaultbackground(true)
        {}
    wxColour foreground;
    wxColour background;
    bool bold;
    bool italics;
    bool underline;
    bool defaultbackground;
};

class Options
{
	friend class OptionsDialog;
	friend class wxValidator;
	friend class wxTextValidator;
	friend class wxGenericValidator;
	public:
		Options();
		~Options();

		// Modifiers:
		bool SetPscpApp(wxString path_and_prog);
		bool SetPlinkApp(wxString path_and_prog);
		bool SetMingwBasePath(wxString path);
		bool SetUsername(wxString user);
		bool SetHostname(wxString host);
		bool SetPassphrase(wxString pass);
		bool SetRemoteCompileOut(wxString path_and_file);
		bool SetLocalCompileOut(wxString path_and_file);
		void SetCombineWatchWindow(bool combine) { m_combineWatchWindow = combine; }
		void SetPrintStyle(int style);
		void SetShowToolbarText(bool useText);
		void SetShowCompileCommands(bool showCommands);
		void SetLineNumberPrinting(bool printLineNumbers);
		void SetSaveSession(bool saveSession) {m_saveSession = saveSession;}
		void SetFormatBeforeSaving(bool formatBeforeSave) {m_formatBeforeSaving = formatBeforeSave;}
		void SetTerminalHistorySize(int size);
		void SetMingwBinPaths(wxArrayString paths);
		void SetMingwExecutables(StringFilenameHash files);
		void SetLaTeXRoot(const wxString& root) {m_LaTeXRoot = root;}
		void SetEditorFont(const wxFont& font) {m_editorFont = font; m_editorFont.SetEncoding(wxFONTENCODING_CP1252); }
		void SetKeepBackupFile(bool keepFile) {m_keepBackupFile = keepFile;}
		void SetCaretBlinkTime(int nTime) {m_caretBlinkTime = nTime;}
		void SetDebuggerFocusLine(int nLine) {m_debuggerFocusLine = nLine;}
		void SetShowLinesInStackTrace(bool show) {m_showLineNumbersInStackTrace = show;}
		void SetShowModulesInStackTrace(bool show) {m_showModulesInStackTrace = show;}
		void SetShowProcedureArguments(bool show) {m_showProcedureArguments = show;}
		void SetShowGlobalVariables(bool show) {m_showGlobalVariables = show;}
		void SetFoldDuringLoading(bool fold) {m_foldDuringLoading = fold;}

		// Accessors:  (inlined)
		wxString GetPscpApp() { return m_pscpProg; }
		wxString GetPlinkApp() { return m_plinkProg; }
		wxString GetMingwBasePath() { return m_mingwBasePath; }
		wxString GetUsername() { return m_username; }
		wxString GetHostname() { return m_hostname; }
		wxString GetPassphrase() { return m_password; }
		wxString GetRemoteCompileOut() { return m_remoteCompileOut; }
		wxString GetLocalCompileOut() { return m_localCompileOut; }
		wxString GetLaTeXRoot() { return m_LaTeXRoot;}
		int GetPrintStyle() { return m_printStyle; }
		bool GetShowToolbarText() { return m_showToolbarText; }
		bool GetLineNumberPrinting() {return m_printLineNumbers; }
		bool GetCombineWatchWindow() { return m_combineWatchWindow; }
		bool GetShowCompileCommands() { return m_showCompileCommands; }
		bool GetSaveSession() {return m_saveSession;}
		bool GetFormatBeforeSaving() {return m_formatBeforeSaving;}
		bool GetKeepBackupFile() {return m_keepBackupFile;}
		int GetTerminalHistorySize() { return m_terminalSize; }
		int GetCaretBlinkTime() {return m_caretBlinkTime;}
		int GetDebuggerFocusLine() {return m_debuggerFocusLine;}
		bool GetShowLinesInStackTrace() {return m_showLineNumbersInStackTrace;}
		bool GetShowModulesInStackTrace() {return m_showModulesInStackTrace;}
		bool GetShowProcedureArguments() {return m_showProcedureArguments;}
		bool GetShowGlobalVariables() {return m_showGlobalVariables;}
		bool GetFoldDuringLoading() {return m_foldDuringLoading;}
		wxArrayString GetMingwProgramNames() { return m_mingwProgramNames; }
		StringFilenameHash GetMingwExecutables() { return m_mingwExecutableNames; }
		wxArrayString GetMingwBinPaths() { return m_mingwBinPaths; }
		wxFont GetEditorFont() { return m_editorFont; }

		wxString VerifyMingwPath(wxString mingwPath);

		enum Styles
		{
            STANDARD,
            CONSOLE_STD,
            COMMAND,
            COMMENT,
            OPTION,
            FUNCTION,
            CUSTOM_FUNCTION,
            CLUSTER,
            CONSTANT,
            SPECIALVAL, // ans cache ...
            STRING,
            STRINGPARSER,
            INCLUDES,
            OPERATOR,
            PROCEDURE,
            NUMBER,
            PROCEDURE_COMMAND,
            METHODS,
            INSTALL,
            DEFAULT_VARS, // x y z t
            ACTIVE_LINE,
            STYLE_END
		};

		enum AnalyzerOptions
		{
		    USE_NOTES,
		    USE_WARNINGS,
		    USE_ERRORS,
		    COMMENT_DENSITY,
		    LINES_OF_CODE,
		    COMPLEXITY,
            MAGIC_NUMBERS,
            ALWAYS_SHOW_METRICS,
            INLINE_IF,
            CONSTANT_EXPRESSION,
            RESULT_SUPPRESSION,
            RESULT_ASSIGNMENT,
            TYPE_ORIENTATION,
            ARGUMENT_UNDERSCORE,
            VARIABLE_LENGTH,
            UNUSED_VARIABLES,
            PROCEDURE_LENGTH,
            THISFILE_NAMESPACE,
            PROGRESS_RUNTIME,
            SWITCH_FALLTHROUGH,
            GLOBAL_VARIABLES,
		    ANALYZER_OPTIONS_END
		};

        SyntaxStyles GetDefaultSyntaxStyle(size_t i);
		inline SyntaxStyles GetSyntaxStyle(size_t i) const
            {
                if (vSyntaxStyles.size() > i && i < Styles::STYLE_END)
                    return vSyntaxStyles[i];
                return SyntaxStyles();
            }

        void SetAnalyzerOption(AnalyzerOptions opt, int nVal);
        int GetAnalyzerOption(AnalyzerOptions opt);

        void readColoursFromConfig(wxFileConfig* _config);
        void writeColoursToConfig(wxFileConfig* _config);
        void readAnalyzerOptionsFromConfig(wxFileConfig* _config);
        void writeAnalyzerOptionsToConfig(wxFileConfig* _config);

        void SetStyleForeground(size_t i, const wxColour& color);
        void SetStyleBackground(size_t i, const wxColour& color);
        void SetStyleDefaultBackground(size_t i, bool _defaultbackground = true);
        void SetStyleBold(size_t i, bool _bold = false);
        void SetStyleItalics(size_t i, bool _italics = false);
        void SetStyleUnderline(size_t i, bool _underline = false);
        wxArrayString GetStyleIdentifier();
        size_t GetIdByIdentifier(const wxString& identifier);

	private:
		wxString m_pscpProg;
		wxString m_plinkProg;
		wxString m_mingwBasePath; // path only? (may be a good idea)

		wxString m_username;
		wxString m_hostname;
		wxString m_password;

		wxString m_remoteCompileOut;
		wxString m_localCompileOut;
		wxString m_LaTeXRoot;

		int m_printStyle;
		int m_terminalSize;
		int m_caretBlinkTime;

		// debugger options
		int m_debuggerFocusLine;
		bool m_showLineNumbersInStackTrace;
		bool m_showModulesInStackTrace;
		bool m_showProcedureArguments;
		bool m_showGlobalVariables;

		bool m_showToolbarText;
		bool m_printLineNumbers;
		bool m_combineWatchWindow;
		bool m_showCompileCommands;
		bool m_saveSession;
		bool m_formatBeforeSaving;
		bool m_keepBackupFile;
		bool m_foldDuringLoading;

		StringFilenameHash m_mingwExecutableNames;
		wxArrayString m_mingwBinPaths;
		wxArrayString m_mingwProgramNames;

		vector<int> vAnalyzerOptions;
		vector<SyntaxStyles> vSyntaxStyles;
		wxFont m_editorFont;

		void setDefaultSyntaxStyles();
		wxString convertToString(const SyntaxStyles& _style);
		SyntaxStyles convertFromString(const wxString& styleString);
		wxString toString(const wxColour& color);
		wxColour StrToColor(wxString colorstring);
};



#endif
