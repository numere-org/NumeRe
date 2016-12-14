#ifndef OPTIONS_H
#define OPTIONS_H

#include <wx/wx.h>
#include <wx/validate.h>
#include <wx/valgen.h>
#include <wx/valtext.h>

#include "datastructures.h"

class Permission;

// copied from stc.h
// PrintColourMode - force black text on white background for printing.
#define wxSTC_PRINT_BLACKONWHITE 2

// PrintColourMode - text stays coloured, but all background is forced to be white for printing.
#define wxSTC_PRINT_COLOURONWHITE 3


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
		void SetTerminalHistorySize(int size);
		void SetMingwBinPaths(wxArrayString paths);
		void SetMingwExecutables(StringFilenameHash files);

		// Accessors:  (inlined)
		wxString GetPscpApp() { return m_pscpProg; }
		wxString GetPlinkApp() { return m_plinkProg; }
		wxString GetMingwBasePath() { return m_mingwBasePath; }
		wxString GetUsername() { return m_username; }
		wxString GetHostname() { return m_hostname; }
		wxString GetPassphrase() { return m_password; }
		wxString GetRemoteCompileOut() { return m_remoteCompileOut; }
		wxString GetLocalCompileOut() { return m_localCompileOut; }
		Permission* GetPerms() { return m_perms; }
		int GetPrintStyle() { return m_printStyle; }
		bool GetShowToolbarText() { return m_showToolbarText; }
		bool GetLineNumberPrinting() {return m_printLineNumbers; }
		bool GetCombineWatchWindow() { return m_combineWatchWindow; }
		bool GetShowCompileCommands() { return m_showCompileCommands; }
		int GetTerminalHistorySize() { return m_terminalSize; }
		wxArrayString GetMingwProgramNames() { return m_mingwProgramNames; }
		StringFilenameHash GetMingwExecutables() { return m_mingwExecutableNames; }
		wxArrayString GetMingwBinPaths() { return m_mingwBinPaths; }

		wxString VerifyMingwPath(wxString mingwPath);

	private:
		wxString m_pscpProg;
		wxString m_plinkProg;
		wxString m_mingwBasePath; // path only? (may be a good idea)

		wxString m_username;
		wxString m_hostname;
		wxString m_password;

		wxString m_remoteCompileOut;
		wxString m_localCompileOut;

		int m_printStyle;
		int m_terminalSize;

		bool m_showToolbarText;
		bool m_printLineNumbers;
		bool m_combineWatchWindow;
		bool m_showCompileCommands;

		StringFilenameHash m_mingwExecutableNames;
		wxArrayString m_mingwBinPaths;
		wxArrayString m_mingwProgramNames;

		Permission* m_perms;
};



#endif
