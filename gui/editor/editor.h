/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/



#include "../terminal/terminal.hpp"
#include "../../common/datastructures.h"
#include "../../kernel/syntax.hpp"
#include "../dialogs/duplicatecodedialog.hpp"

#ifndef EDITOR_H
#define EDITOR_H

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/menu.h>
#include <wx/dynarray.h>
#include <wx/thread.h>
#include <wx/buffer.h>

#define RM_WS_BOTH 0
#define RM_WS_FRONT 1
#define RM_WS_BACK 2

class NumeReWindow;
class EditorNotebook;
class wxFileName;
class Options;
class DebugManager;
class ProcedureViewer;
class CodeAnalyzer;
class SearchController;
class CodeFormatter;

/** \brief The class of the editor window
 */
class NumeReEditor : public wxStyledTextCtrl, public wxThreadHelper
{
	public:
	    friend class CodeAnalyzer;
	    friend class SearchController;
	    friend class CodeFormatter;

		NumeReEditor(NumeReWindow* mframe, Options* options,
					 wxWindow* parent, wxWindowID id, NumeReSyntax* __syntax, NumeReTerminal* __terminal, const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize, long style = 0,
					 const wxString& name = wxSTCNameStr);
		~NumeReEditor();

		/** \brief The possible per-editor settings
		 */
		enum EditorSettings
		{
			SETTING_WRAPEOL = 1,
			SETTING_DISPCTRLCHARS = 2,
			SETTING_USETXTADV = 4,
			SETTING_USEANALYZER = 8,
			SETTING_INDENTONTYPE = 16,
			SETTING_USESECTIONS = 32
		};
		bool defaultPage;


		bool LoadLocalFile (const wxString& filename);
		bool LoadFileText(wxString fileContents);
		bool SaveFile(const wxString& filename);
		bool SaveNumeReFile(const wxString& filename);
		bool SaveGeneralFile(const wxString& filename);
		bool SaveFileLocal();

		/** \brief Returns whether the current file was modified after the last time saved
		 *
		 * \return bool
		 *
		 */
		bool Modified();
		bool HasBeenSaved();
		void registerProcedureViewer(ProcedureViewer* viewer);

		void OnChar(wxStyledTextEvent& event);
		void OnRightClick(wxMouseEvent& event);
		void OnMarginClick(wxStyledTextEvent& event);
		void OnKeyDn(wxKeyEvent& event);
		void OnKeyRel(wxKeyEvent& event);
		void OnMouseUp(wxMouseEvent& event);
		void OnMouseDn(wxMouseEvent& event);
		void OnMouseDblClk(wxMouseEvent& event);
		void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
		void OnEnter(wxMouseEvent& event);
		void OnLeave(wxMouseEvent& event);
		void OnLoseFocus(wxFocusEvent& event);
		void OnMouseDwell(wxStyledTextEvent& event);
		void OnSavePointReached(wxStyledTextEvent& event);
		void OnSavePointLeft(wxStyledTextEvent& event);
		void ClearDblClkIndicator();
		void MakeBraceCheck();
		void MakeBlockCheck();
		void OnIdle(wxIdleEvent& event);

		// asynch update calls
		void HandleFunctionCallTip();
		void UpdateProcedureViewer();
		string GetCurrentFunctionContext(int& nStartingBrace);
		string GetFunctionCallTip(const string& sFunctionName);
		string GetMethodCallTip(const string& sMethodName);
		string GetCurrentArgument(const string& sCallTip, int nStartingBrace, int& nArgStartPos);

		int CallTipStartPos();
		void AdvCallTipShow(int pos, const wxString& definition);
		void AdvCallTipCancel();

		/** \brief Finds the matching brace to the brace at the position nPos
		 *
		 * \param nPos int
		 * \return void
		 *
		 */
		void getMatchingBrace(int nPos);
		/** \brief Finds the matching flow control element to the one at the position nPos
		 *
		 * \param nPos int
		 * \return void
		 *
		 */
		void getMatchingBlock(int nPos);
		/** \brief Searches for flow control elements
		 *
		 * Returnes a vector. If first element is invalid, the word is no command. If the last
		 * one is invalid, there's no matching partner. It returnes more than two elements for
		 * "if" blocks. If there's no first "if", if one currently focussing on an "else...",
		 * the first element may be invalid, but more can be returned.
		 *
		 * \param nPos int
		 * \return vector<int>
		 *
		 */
		vector<int> BlockMatch(int nPos);
		vector<int> BlockMatchNSCR(int nPos);
		vector<int> BlockMatchMATLAB(int nPos);
		/** \brief Applies the syntax hinghlighting depending on the loaded file type
		 *
		 * \param forceUpdate bool
		 * \return void
		 *
		 */
		void UpdateSyntaxHighlighting(bool forceUpdate = false);
		/** \brief Jumps the cursor to the named line and displays an indicator if desired
		 *
		 * \param linenumber int
		 * \param showMarker bool
		 * \return void
		 *
		 */
		void FocusOnLine(int linenumber, bool showMarker = true);

		FileFilterType GetFileType(const wxString& filename);

		wxString GetFileNameAndPath();
		wxString GetFilenameString();
		wxFileName GetFileName(); // capital N
		wxFileName GetExecutableFileName()
		{
			return m_executableFilename;
		}
		wxString GetFilePath();
		wxArrayInt GetBreakpoints();

		void SetFilename(wxFileName filename, bool fileIsRemote);
		void SetExecutableFilename(wxFileName filename);
		void ResetEditor();
		bool LastSavedRemotely()
		{
			return m_bLastSavedRemotely;
		}

		void GotoPipe(int nStartPos = 0);
		void OnAddBreakpoint(wxCommandEvent& event);
		void OnRemoveBreakpoint(wxCommandEvent& event);
		void OnClearBreakpoints(wxCommandEvent& event);
		void OnAddBookmark(wxCommandEvent& event);
		void OnRemoveBookmark(wxCommandEvent& event);
		void OnClearBookmarks(wxCommandEvent& event);
		//void OnAddWatch(wxCommandEvent &event);
		void OnDisplayVariable(wxCommandEvent& event);
		void OnHelpOnSelection(wxCommandEvent& event);
		void OnFindProcedure(wxCommandEvent& event);
		void OnFindProcedureFromMenu();
		void OnFindInclude(wxCommandEvent& event);
		void OnFindIncludeFromMenu();
		void OnChangeCase(wxCommandEvent& event);
		void OnFoldCurrentBlock(wxCommandEvent& event);
		void OnHideSelection(wxCommandEvent& event);
		void OnUnhideAllFromMenu();
		void OnRenameSymbols(wxCommandEvent& event);
		void OnRenameSymbolsFromMenu();
		void OnAbstrahizeSection(wxCommandEvent& event);
		void OnAbstrahizeSectionFromMenu();
		void OnTranspose();
		void OnExtractAsHTML();

		void OnAnalyzerTimer(wxTimerEvent& event);
		bool InitDuplicateCode();
		void OnFindDuplicateCode(int nDuplicateFlag = 1, int nNumDuplicatedLines = 6); // 0 = direct comparison, 1 = use var semanticals, 2 = use string semanticals,
		void IndicateDuplicatedLine(int nStart1, int nEnd1, int nStart2, int nEnd2, int nSelectionLine);

		// for the duplicate code analysis
		virtual wxThread::ExitCode Entry();
		void OnThreadUpdate(wxThreadEvent& event);

		void AddBreakpoint( int linenum );
		void RemoveBreakpoint( int linenum );
		void SynchronizeBreakpoints();

		void AddProcedureDocumentation();

		void SetSyntax(NumeReSyntax* __syntax)
		{
			if (!_syntax)
			{
				_syntax = __syntax;
			}
		}
		void SetTerminal(NumeReTerminal* _terminal)
		{
			if (!m_terminal)
			{
				m_terminal = _terminal;
			}
		}
		void SetUnsaved();

		void ApplyAutoIndentation(int nFirstLine = 0, int nLastLine = -1);
		void ApplyAutoFormat(int nFirstLine = 0, int nLastLine = -1);
		void Transpose(int nFirstLine = 0, int nLastLine = -1);
		void ExtractAsHTML(int nFirstLine = 0, int nLastLine = -1);

		void ToggleSettings(int _setting);
		bool getEditorSetting(EditorSettings _setting);
		int getSettings()
		{
			return m_nEditorSetting;
		}

		void ToggleCommentLine();
		void ToggleCommentSelection();
		/** \brief Folds all fold markers
		 *
		 * \return void
		 *
		 */
		void FoldAll();
		/** \brief Unfolds all fold markers
		 *
		 * \return void
		 *
		 */
		void UnfoldAll();
		void MoveSelection(bool down = true)
		{
			if (down)
				MoveSelectedLinesDown();
			else
				MoveSelectedLinesUp();
		}
		/** \brief Jumps the cursor to the next bookmark in the current file
		 *
		 * \param down bool direction to jump
		 * \return void
		 *
		 */
		void JumpToBookmark(bool down = true);
		vector<int> getBookmarks();
		void setBookmarks(const vector<int>& vBookmarks);
		/** \brief Removes whitespaces depending on the passed type
		 *
		 * nType defines the type of whitespaces to be removed. This is either front, back
		 * or both. Either the selection (if any) is affected or the whole document.
		 *
		 * \param nType int Either RM_WS_BOTH, RM_WS_FRONT or RM_WS_BACK
		 * \return void
		 *
		 */
		void removeWhiteSpaces(int nType = RM_WS_BOTH);
		void toggleBookmark();
		void clearBookmarks();
		/** \brief Sorts the lines de- or ascending
		 *
		 * Sorts the lines. Either the selection (if any) is affected or the whole document.
		 *
		 * \param ascending bool if true, then ascending, else descending
		 * \return void
		 *
		 */
		void sortSelection(bool ascending = true);

		FileFilterType getFileType()
		{
			return m_fileType;
		}

		void notifyDialogClose();
		void SetEditorFont(const wxFont& font);
		bool isNumeReFileType()
		{
		    return m_fileType == FILE_NSCR || m_fileType == FILE_NPRC;
		}
		bool isCodeFile()
		{
		    return m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_CPP || m_fileType == FILE_MATLAB;
		}

		void AnalyseCode();

		void FindAndOpenProcedure(const wxString& procedurename);
		void FindAndOpenInclude(const wxString& includename);
		vector<wxString> getProceduresInFile();

	protected:
		Options* m_options;

	private:
		enum StyleType
		{
			STYLE_DEFAULT,
			STYLE_COMMENT_LINE,
			STYLE_COMMENT_BLOCK,
			STYLE_COMMENT_SECTION_LINE,
			STYLE_COMMENT_SECTION_BLOCK,
			STYLE_COMMAND,
			STYLE_FUNCTION,
			STYLE_CUSTOMFUNCTION,
			STYLE_OPERATOR,
			STYLE_PROCEDURE,
			STYLE_IDENTIFIER,
			STYLE_DATAOBJECT,
			STYLE_NUMBER,
			STYLE_STRINGPARSER,
			STYLE_STRING
		};


		void FoldCurrentBlock(int nLine);

		void markSections(bool bForceRefresh = false);
		void markLocalVariables(bool bForceRefresh = false);
		void markLocalVariableOfType(const wxString& command, bool bForceRefresh);
		bool isNoAutoIndentionKey(int keycode);
		void AsynchActions();
		void AsynchEvaluations();

		void updateDefaultHighlightSettings();
		void applyStrikeThrough();

		bool isStyleType(StyleType _type, int nPos);

		int countUmlauts(const string& sStr);
		string realignLangString(string sLine, size_t& lastpos);
		string addLinebreaks(const string& sLine, bool onlyDocumentation = false);

		void markModified(int nLine);
		void markSaved();

		void OnEditorModified(wxStyledTextEvent& event);
		void OnStartDrag(wxStyledTextEvent& event);
		void OnDragOver(wxStyledTextEvent& event);
		void OnDrop(wxStyledTextEvent& event);
		void OnMouseMotion(wxMouseEvent& event);

		void OnRunToCursor(wxCommandEvent& event);

		int GetLineForMarkerOperation();
		void ResetRightClickLocation();
		void ReplaceMatches(const vector<int>& vMatches, const wxString& sSymbol, const wxString& sNewSymbol);
		void RenameSymbols(int nPos);
		void AbstrahizeSection();
		void CreateProcedureFromSection(int nStartPos, int nEndPos, const wxString& sInputList, const wxString sOutputList);
		bool IsModifiedInSection(int nSectionStart, int nSectionEnd, const wxString& sToken, const vector<int>& vMatch);
		wxString getFunctionArgumentList(int nFunctionStartLine);
		wxString getMatlabReturnList(int nMatlabFunctionStartLine);
		wxString getTemplateContent(const wxString& sFileName);

		wxString generateAutoCompList(const wxString& wordstart, string sPreDefList);

		bool MarkerOnLine(int linenum, int nMarker);

		void detectCodeDuplicates(int startline, int endline, int nDuplicateFlags, int nNumDuplicatedLines);
		double compareCodeLines(int nLine1, int nLine2, int nDuplicateFlags);
		string getSemanticLine(int nLine, int nDuplicateFlags);
		string getSemanticLineNSCR(int nLine, int nDuplicateFlags);
		string getSemanticLineMATLAB(int nLine, int nDuplicateFlags);
		string getSemanticLineCPP(int nLine, int nDuplicateFlags);
		map<int, int> getDifferences(int nStart1, int nEnd1, int nStart2, int nEnd2);
		wxString getNextToken(int& nPos);

		NumeReWindow* m_mainFrame;
		ProcedureViewer* m_procedureViewer;
		CodeAnalyzer* m_analyzer;
		SearchController* m_search;
		CodeFormatter* m_formatter;

		wxFileName m_fileNameAndPath;
		wxFileName m_executableFilename;
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
		wxMenuItem* m_menuRefactoring;

		NumeReSyntax* _syntax;
		NumeReTerminal* m_terminal;

		DuplicateCodeDialog* m_duplicateCode;
		wxCriticalSection m_editorCS;
		vector<string> vDuplicateCodeResults;
		vector<string> vParsedSemanticCode;
		vector<wxString> vRenameSymbolsChangeLog;
		int m_nProcessValue;
		int m_nDuplicateCodeFlag;
		int m_nDuplicateCodeLines;
		int m_nFirstLine;
		int m_nLastLine;
		int m_nLastReleasedKey;
		int m_nCallTipStart;
		string m_sCallTipContent;

		bool m_bLoadingFile;
		bool m_bLastSavedRemotely;
		bool m_bHasBeenCompiled;
		bool m_bNewFile;
		bool m_bSetUnsaved;
		bool m_PopUpActive;
		bool m_dragging;
		bool m_modificationHappened;

		int m_nEditorSetting;
		FileFilterType m_fileType;


		wxMenu m_popupMenu;
        wxMenu* m_refactoringMenu;
        wxTimer* m_analyzerTimer;


		DECLARE_EVENT_TABLE()
};


//BEGIN_EVENT_TABLE(ChameleonEditor, wxStyledTextCtrl)

#endif
