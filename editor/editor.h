#ifndef EDITOR__H
#define EDITOR_H

#include <wx/stc/stc.h>
#include <wx/menu.h>
#include <wx/dynarray.h>
#include "../gui/wxterm.h"
#include "../common/datastructures.h"
#include "../compiler/compilerevent.h"
#include "../kernel/syntax.hpp"
//#include <wx/wx.h>

class NumeReWindow;
class NumeReNotebook;
class wxFileName;
class Options;
class ProjectInfo;
class DebugManager;


class NumeReEditor : public wxStyledTextCtrl
{
public:
	NumeReEditor(NumeReWindow* mframe, DebugManager* debugManager, Options* options, ProjectInfo* project,
					wxWindow *parent, wxWindowID id, NumeReSyntax* __syntax, wxTerm* __terminal, const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize, long style = 0,
					const wxString& name = wxSTCNameStr);
	~NumeReEditor();

    enum EditorSettings
    {
        SETTING_WRAPEOL = 1,
        SETTING_DISPCTRLCHARS = 2
    };
	//bool LoadFile ();
	bool LoadLocalFile (const wxString &filename);
	bool LoadFileText(wxString fileContents);
	//bool SaveFileAs();
	bool SaveFile(const wxString& filename);
	bool SaveFileLocal();

	bool Modified();
	bool HasBeenCompiled();
	bool HasBeenSaved();

	void SetCompiled();

	void OnChar(wxStyledTextEvent &event);
	void OnRightClick(wxMouseEvent &event);
	void OnMarginClick(wxStyledTextEvent &event);
	void OnKeyDn(wxKeyEvent &event);
	void OnKeyRel(wxKeyEvent &event);
	void OnMouseUp(wxMouseEvent &event);
	void OnMouseDn(wxMouseEvent &event);
	void OnMouseDblClk(wxMouseEvent& event);
	void OnMouseDwell(wxStyledTextEvent& event);
	void ClearDblClkIndicator();
	void MakeBraceCheck();

	void getMatchingBrace(int nPos);


	void UpdateSyntaxHighlighting();
	void FocusOnLine(int linenumber, bool showMarker = true);


	wxString GetFileNameAndPath();
	wxString GetFilenameString();
	wxFileName GetFileName(); // capital N
	wxFileName GetExecutableFileName() 	{ return m_executableFilename; }
	wxString GetFilePath();
	wxArrayInt GetBreakpoints();
	ProjectInfo* GetProject(){ return m_project; }


	//void SetFileNameAndPath(wxString path, wxString name, bool fileIsRemote);
	void SetFilename(wxFileName filename, bool fileIsRemote);
	void SetExecutableFilename(wxFileName filename);
	void SetProject(ProjectInfo* project);
	void ResetEditor();
	bool LastSavedRemotely() {	return m_bLastSavedRemotely;}

	void OnAddBreakpoint(wxCommandEvent &event);
	void OnRemoveBreakpoint(wxCommandEvent &event);
	void OnClearBreakpoints(wxCommandEvent &event);
	void OnAddWatch(wxCommandEvent &event);
	void OnDisplayVariable(wxCommandEvent &event);
	void OnFindProcedure(wxCommandEvent &event);

	void AddBreakpoint( int linenum );
	void RemoveBreakpoint( int linenum );
	void SetSyntax(NumeReSyntax* __syntax) {if (!_syntax){_syntax = __syntax;}}
	void SetTerminal(wxTerm* _terminal) {if (!m_terminal){m_terminal = _terminal;}}
	void SetUnsaved();

	void ApplyAutoIndentation();
	void ToggleSettings(EditorSettings _setting);
	bool getEditorSetting(EditorSettings _setting);

private:

    int determineIndentationLevel(std::string sLine, bool& bIsElseCase);
    string addLinebreaks(const string& sLine);
	void OnEditorModified(wxStyledTextEvent &event);

	void OnRunToCursor(wxCommandEvent &event);
	//void OnCompilerEnded(CompilerEvent &event);

	int GetLineForBreakpointOperation();
	void ResetRightClickLocation();
	wxString FindClickedWord();
	wxString FindClickedProcedure();
	wxString FindMarkedProcedure(int charpos);

	void CreateBreakpointEvent(int linenumber, bool addBreakpoint);
	bool BreakpointOnLine(int linenum);


	NumeReWindow* m_mainFrame;
	//ChameleonNotebook* m_parentNotebook;
	Options* m_options;
	DebugManager* m_debugManager;
	ProjectInfo* m_project;

	wxFileName m_fileNameAndPath;
	wxFileName m_executableFilename;
	//wxFileName m_remoteFileName;
	//wxFileName m_localFileName;
	wxString m_simpleFileName;
	wxString m_watchedString;
	wxString m_dblclkString;
	wxDateTime m_filetime;

	wxArrayInt m_breakpoints;
	wxPoint m_lastRightClick;
	wxString m_clickedWord;
	wxString m_clickedProcedure;
	wxMenuItem* m_menuAddWatch;
	wxMenuItem* m_menuShowValue;
	wxMenuItem* m_menuFindProcedure;

	NumeReSyntax* _syntax;
	wxTerm* m_terminal;

	bool m_bLoadingFile;
	bool m_bLastSavedRemotely;
	bool m_bHasBeenCompiled;
	bool m_bNewFile;
	bool m_bSetUnsaved;

	int m_nEditorSetting;
	FileFilterType m_fileType;


	wxMenu m_popupMenu;



	DECLARE_EVENT_TABLE()
};


//BEGIN_EVENT_TABLE(ChameleonEditor, wxStyledTextCtrl)

#endif
