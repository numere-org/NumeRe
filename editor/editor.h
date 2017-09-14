#ifndef EDITOR_H
#define EDITOR_H

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/menu.h>
#include <wx/dynarray.h>
#include "../gui/wxterm.h"
#include "../common/datastructures.h"
#include "../compiler/compilerevent.h"
#include "../kernel/syntax.hpp"
#include "../gui/dialogs/duplicatecodedialog.hpp"
//#include <wx/wx.h>

#define RM_WS_BOTH 0
#define RM_WS_FRONT 1
#define RM_WS_BACK 2

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
        SETTING_DISPCTRLCHARS = 2,
        SETTING_USETXTADV = 4,
        SETTING_USEANALYZER = 8,
        SETTING_INDENTONTYPE = 16
    };
    bool defaultPage;

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
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
	void OnEnter(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnLoseFocus(wxFocusEvent& event);
	void OnMouseDwell(wxStyledTextEvent& event);
	void OnSavePointReached(wxStyledTextEvent& event);
	void ClearDblClkIndicator();
	void MakeBraceCheck();
	void MakeBlockCheck();

	void getMatchingBrace(int nPos);
	void getMatchingBlock(int nPos);
	vector<int> BlockMatch(int nPos);


	void UpdateSyntaxHighlighting(bool forceUpdate = false);
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
	void OnHelpOnSelection(wxCommandEvent& event);
	void OnFindProcedure(wxCommandEvent &event);
	void OnFindInclude(wxCommandEvent &event);
	void OnChangeCase(wxCommandEvent& event);
	void OnFoldCurrentBlock(wxCommandEvent& event);
	bool InitDuplicateCode();
	void OnFindDuplicateCode(int nDuplicateFlag = 1); // 0 = direct comparison, 1 = use var semanticals, 2 = use string semanticals, 3 = use all semanticals
	void IndicateDuplicatedLine(int nStart1, int nEnd1, int nStart2, int nEnd2);

	void AddBreakpoint( int linenum );
	void RemoveBreakpoint( int linenum );
	void SetSyntax(NumeReSyntax* __syntax) {if (!_syntax){_syntax = __syntax;}}
	void SetTerminal(wxTerm* _terminal) {if (!m_terminal){m_terminal = _terminal;}}
	void SetUnsaved();

	void ApplyAutoIndentation(int nFirstLine = 0, int nLastLine = -1);
	void ApplyAutoFormat(int nFirstLine = 0, int nLastLine = -1);
	void ToggleSettings(int _setting);
	bool getEditorSetting(EditorSettings _setting);
	void ToggleCommentLine();
	void ToggleCommentSelection();
	void FoldAll();
	void UnfoldAll();
	void AnalyseCode();
	void MoveSelection(bool down = true)
        {
            if (down)
                MoveSelectedLinesDown();
            else
                MoveSelectedLinesUp();
        }
    void removeWhiteSpaces(int nType = RM_WS_BOTH);
    void sortSelection(bool ascending = true);
	FileFilterType getFileType() {return m_fileType;}
	int getSettings() {return m_nEditorSetting;}
	string GetStrippedLine(int nLine);

protected:
	Options* m_options;

private:

    void FoldCurrentBlock(int nLine);

    void AsynchActions();

    void updateDefaultHighlightSettings();
    void applyStrikeThrough();
    int determineIndentationLevel(std::string sLine, bool& bIsElseCase);
    int countUmlauts(const string& sStr);
    string realignLangString(string sLine, size_t& lastpos);
    string addLinebreaks(const string& sLine);
    void addToAnnotation(string& sCurrentLine, string& sStyles, const string& sMessage, int nStyle);
    bool containsAssignment(const string& sCurrentLine);

    void markModified(int nLine);
    void markSaved();

	void OnEditorModified(wxStyledTextEvent &event);
	void OnStartDrag(wxStyledTextEvent& event);
	void OnDragOver(wxStyledTextEvent& event);
	void OnDrop(wxStyledTextEvent & event);
	void OnMouseMotion(wxMouseEvent& event);

	void OnRunToCursor(wxCommandEvent &event);
	//void OnCompilerEnded(CompilerEvent &event);

	int GetLineForBreakpointOperation();
	void ResetRightClickLocation();
	wxString FindClickedWord();
	wxString FindClickedInclude();
	wxString FindClickedProcedure();
	wxString FindMarkedProcedure(int charpos);
	wxString FindNameSpaceOfProcedure(int charpos);
	wxString FindProceduresInCurrentFile(wxString sFirstChars, wxString sSelectedNameSpace);
	wxString FindProcedureDefinition();
	int FindNamingProcedure();
    wxString getTemplateContent(const wxString& sFileName);

	wxString generateAutoCompList(const wxString& wordstart, string sPreDefList);

	void CreateBreakpointEvent(int linenumber, bool addBreakpoint);
	bool BreakpointOnLine(int linenum);

	int calculateCyclomaticComplexity(int startline, int endline);
	int calculateLinesOfCode(int startline, int endline);
	int countNumberOfComments(int startline, int endline);
	int insertTextAndMove(int nPosition, const wxString& sText);
	vector<string> detectCodeDuplicates(int startline, int endline, int nDuplicateFlags);
	double compareCodeLines(int nLine1, int nLine2, int nDuplicateFlags);
	string getSemanticLine(int nLine, int nDuplicateFlags);
	map<int,int> getDifferences(int nStart1, int nEnd1, int nStart2, int nEnd2);
	wxString getNextToken(int& nPos);

	NumeReWindow* m_mainFrame;
	//ChameleonNotebook* m_parentNotebook;
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
	wxString m_clickedInclude;
	size_t m_clickedWordLength;
	wxMenuItem* m_menuAddWatch;
	wxMenuItem* m_menuShowValue;
	wxMenuItem* m_menuFindProcedure;
	wxMenuItem* m_menuHelpOnSelection;
	wxMenuItem* m_menuFindInclude;

	NumeReSyntax* _syntax;
	wxTerm* m_terminal;

	DuplicateCodeDialog* m_duplicateCode;

	bool m_bLoadingFile;
	bool m_bLastSavedRemotely;
	bool m_bHasBeenCompiled;
	bool m_bNewFile;
	bool m_bSetUnsaved;
	bool m_PopUpActive;
	bool m_dragging;

	int m_nEditorSetting;
	FileFilterType m_fileType;


	wxMenu m_popupMenu;



	DECLARE_EVENT_TABLE()
};


//BEGIN_EVENT_TABLE(ChameleonEditor, wxStyledTextCtrl)

#endif
