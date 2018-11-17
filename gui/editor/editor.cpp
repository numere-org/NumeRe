#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#else
#define _ASSERT(expr) ((void)0)

#define _ASSERTE(expr) ((void)0)
#endif

#include "../../common/CommonHeaders.h"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"

#include <wx/datetime.h>
#include <wx/stdpaths.h>
#include <vector>
#include <string>

#include "editor.h"
#include "../NumeReWindow.h"
#include "../NumeReNotebook.h"

#include "../../common/datastructures.h"
#include "../../common/Options.h"
#include "../../common/DebugEvent.h"
#include "../../common/ProjectInfo.h"
#include "../../common/debug.h"
//#include "../../common/fixvsbug.h"
#include "../globals.hpp"

#define MARGIN_FOLD 3
#define HIGHLIGHT 25
#define HIGHLIGHT_DBLCLK 26
#define HIGHLIGHT_MATCHING_BRACE 6
#define HIGHLIGHT_STRIKETHROUGH 7
#define HIGHLIGHT_MATCHING_BLOCK 8
#define HIGHLIGHT_NOT_MATCHING_BLOCK 9
#define HIGHLIGHT_DIFFERENCES 10
#define HIGHLIGHT_DIFFERENCE_SOURCE 11
#define HIGHLIGHT_ANNOTATION 12
#define ANNOTATION_NOTE 22
#define ANNOTATION_WARN 23
#define ANNOTATION_ERROR 24

#define SEMANTICS_VAR 1
#define SEMANTICS_STRING 2
#define SEMANTICS_NUM 4
#define SEMANTICS_FUNCTION 8

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_EVENT_TABLE(NumeReEditor, wxStyledTextCtrl)
	EVT_STC_CHARADDED	(-1, NumeReEditor::OnChar)
	EVT_STC_MODIFIED	(-1, NumeReEditor::OnEditorModified)
	EVT_KEY_DOWN        (NumeReEditor::OnKeyDn)
	EVT_KEY_UP          (NumeReEditor::OnKeyRel)
	EVT_LEFT_DOWN       (NumeReEditor::OnMouseDn)
	EVT_LEFT_UP         (NumeReEditor::OnMouseUp)
	EVT_RIGHT_DOWN		(NumeReEditor::OnRightClick)
	EVT_LEFT_DCLICK		(NumeReEditor::OnMouseDblClk)
	EVT_MOUSE_CAPTURE_LOST(NumeReEditor::OnMouseCaptureLost)
	EVT_ENTER_WINDOW    (NumeReEditor::OnEnter)
	EVT_LEAVE_WINDOW    (NumeReEditor::OnLeave)
	EVT_KILL_FOCUS      (NumeReEditor::OnLoseFocus)
	EVT_STC_DWELLSTART  (-1, NumeReEditor::OnMouseDwell)
	EVT_STC_MARGINCLICK (-1, NumeReEditor::OnMarginClick)
	EVT_STC_DRAG_OVER   (-1, NumeReEditor::OnDragOver)
	EVT_STC_SAVEPOINTREACHED (-1, NumeReEditor::OnSavePointReached)
	EVT_MENU			(ID_DEBUG_ADD_BREAKPOINT, NumeReEditor::OnAddBreakpoint)
	EVT_MENU			(ID_DEBUG_REMOVE_BREAKPOINT, NumeReEditor::OnRemoveBreakpoint)
	EVT_MENU			(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, NumeReEditor::OnClearBreakpoints)
	EVT_MENU			(ID_BOOKMARK_ADD, NumeReEditor::OnAddBookmark)
	EVT_MENU			(ID_BOOKMARK_REMOVE, NumeReEditor::OnRemoveBookmark)
	EVT_MENU			(ID_BOOKMARK_CLEAR, NumeReEditor::OnClearBookmarks)
	EVT_MENU			(ID_DEBUG_DISPLAY_SELECTION, NumeReEditor::OnDisplayVariable)
	EVT_MENU			(ID_FIND_PROCEDURE, NumeReEditor::OnFindProcedure)
	EVT_MENU			(ID_FIND_INCLUDE, NumeReEditor::OnFindInclude)
	EVT_MENU            (ID_UPPERCASE, NumeReEditor::OnChangeCase)
	EVT_MENU            (ID_LOWERCASE, NumeReEditor::OnChangeCase)
	EVT_MENU            (ID_FOLD_CURRENT_BLOCK, NumeReEditor::OnFoldCurrentBlock)
	EVT_MENU            (ID_MENU_HELP_ON_ITEM, NumeReEditor::OnHelpOnSelection)
	EVT_MENU			(ID_DEBUG_RUNTOCURSOR, NumeReEditor::OnRunToCursor)
	EVT_MENU            (ID_RENAME_SYMBOLS, NumeReEditor::OnRenameSymbols)
	EVT_MENU            (ID_ABSTRAHIZE_SECTION, NumeReEditor::OnAbstrahizeSection)
	EVT_IDLE            (NumeReEditor::OnIdle)
END_EVENT_TABLE()

int CompareInts(int n1, int n2)
{
	return n1 - n2;
}


extern Language _guilang;
using namespace std;

//////////////////////////////////////////////////////////////////////////////
///  public constructor ChameleonEditor
///  General-purpose editor creation and stuff
///
///  @param  mframe  ChameleonWindow * A pointer to the main application frame
///  @param  options Options *         A pointer to the main Options object
///  @param  project ProjectInfo *     A pointer to this editor's project
///  @param  parent  wxWindow *        The editor's parent window
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
NumeReEditor::NumeReEditor( NumeReWindow* mframe,
							Options* options,
							ProjectInfo* project,
							wxWindow* parent,     wxWindowID id, NumeReSyntax* __syntax, wxTerm* __terminal,
							const wxPoint& pos /* = wxDefaultPosition */,
							const wxSize& size /* = wxDefaultSize */,
							long style /* = 0 */, const
							wxString& name /* = wxSTCNameStr */ ) :
	wxStyledTextCtrl(parent, id, pos, size, style, name)
{
	defaultPage = false;
	m_mainFrame = mframe;
	m_options = options;
	m_project = project;
	m_project->AddEditor(this);
	m_duplicateCode = nullptr;
	m_nCallTipStart = 0;
	m_modificationHappened = false;
	m_nDuplicateCodeLines = 6;

	m_watchedString = "";
	m_dblclkString = "";

	m_nEditorSetting = 0;
	m_fileType = FILE_NOTYPE;

	m_bLoadingFile = false;
	m_bLastSavedRemotely = true;
	m_bHasBeenCompiled = false;
	m_PopUpActive = false;

	m_fileNameAndPath.Assign(wxEmptyString);

	m_lastRightClick.x = -1;
	m_lastRightClick.y = -1;
	_syntax = __syntax;
	m_terminal = __terminal;
	m_dragging = false;

	m_nFirstLine = m_nLastLine = 0;
	m_nDuplicateCodeFlag = 0;

	Bind(wxEVT_THREAD, &NumeReEditor::OnThreadUpdate, this);

	this->SetTabWidth(4);
	this->SetIndent(4);
	this->SetUseTabs(true);

	this->SetMultipleSelection(true);
	this->SetVirtualSpaceOptions(wxSTC_SCVS_RECTANGULARSELECTION);
	this->SetAdditionalSelectionTyping(true);
	this->SetMultiPaste(wxSTC_MULTIPASTE_EACH);

	this->SetMarginWidth(0, 40);
	this->SetMarginType(0, wxSTC_MARGIN_NUMBER);

	this->SetMarginWidth(1, 20);
	this->SetMarginType(1, wxSTC_MARGIN_SYMBOL);

	this->SetYCaretPolicy(wxSTC_CARET_SLOP | wxSTC_CARET_STRICT | wxSTC_CARET_EVEN, 1);

	wxFileName f(wxStandardPaths::Get().GetExecutablePath());
	//wxInitAllImageHandlers();
	this->RegisterImage(NumeReSyntax::SYNTAX_COMMAND, wxBitmap(f.GetPath(true) + "icons\\cmd.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_FUNCTION, wxBitmap(f.GetPath(true) + "icons\\fnc.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_OPTION, wxBitmap(f.GetPath(true) + "icons\\opt.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_CONSTANT, wxBitmap(f.GetPath(true) + "icons\\cnst.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_SPECIALVAL, wxBitmap(f.GetPath(true) + "icons\\spv.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_OPERATOR, wxBitmap(f.GetPath(true) + "icons\\opr.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_METHODS, wxBitmap(f.GetPath(true) + "icons\\mthd.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_PROCEDURE, wxBitmap(f.GetPath(true) + "icons\\prc.png", wxBITMAP_TYPE_PNG));

	wxFont font = m_options->GetEditorFont();
	this->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

	// Add the characters for procedures to the word char list
	// this->SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$~");

	this->StyleClearAll();

	this->SetMouseDwellTime(500);

	this->EmptyUndoBuffer();
	m_bSetUnsaved = false;
	m_bNewFile = true;
	UpdateSyntaxHighlighting();
	m_bNewFile = false;

	this->MarkerDefine(MARKER_BREAKPOINT, wxSTC_MARK_CIRCLE);
	this->MarkerSetBackground(MARKER_BREAKPOINT, wxColour("red"));

	this->MarkerDefine(MARKER_BOOKMARK, wxSTC_MARK_SMALLRECT);
	this->MarkerSetBackground(MARKER_BOOKMARK, wxColour(192, 0, 64));

	this->MarkerDefine(MARKER_FOCUSEDLINE, wxSTC_MARK_SHORTARROW);
	this->MarkerSetBackground(MARKER_FOCUSEDLINE, wxColour("yellow"));

	this->MarkerDefine(MARKER_MODIFIED, wxSTC_MARK_LEFTRECT);
	this->MarkerSetBackground(MARKER_MODIFIED, wxColour(255, 220, 0));

	this->MarkerDefine(MARKER_SAVED, wxSTC_MARK_LEFTRECT);
	this->MarkerSetBackground(MARKER_SAVED, wxColour("green"));

	this->MarkerDefine(MARKER_SECTION, wxSTC_MARK_ARROWDOWN);
	//this->MarkerSetBackground(MARKER_SECTION, wxColor(192,192,192));
	//this->MarkerSetBackground(MARKER_SECTION, wxColor(255,192,192));
	this->MarkerSetBackground(MARKER_SECTION, m_options->GetSyntaxStyle(Options::COMMENT).foreground);

	this->SetMarginSensitive(1, true);

	this->UsePopUp(false);

	m_refactoringMenu = new wxMenu();
	m_refactoringMenu->Append(ID_RENAME_SYMBOLS, _guilang.get("GUI_MENU_EDITOR_RENAME_SYMBOLS"));
	m_refactoringMenu->Append(ID_ABSTRAHIZE_SECTION, _guilang.get("GUI_MENU_EDITOR_ABSTRAHIZE_SECTION"));

	m_popupMenu.Append(ID_MENU_CUT, _guilang.get("GUI_MENU_EDITOR_CUT"));
	m_popupMenu.Append(ID_MENU_COPY, _guilang.get("GUI_MENU_EDITOR_COPY"));
	m_popupMenu.Append(ID_MENU_PASTE, _guilang.get("GUI_MENU_EDITOR_PASTE"));
	m_popupMenu.AppendSeparator();

	m_popupMenu.Append(ID_FOLD_CURRENT_BLOCK, _guilang.get("GUI_MENU_EDITOR_FOLDCURRENTBLOCK"));
	m_popupMenu.AppendSeparator();

	m_popupMenu.Append(ID_DEBUG_ADD_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_ADDBP"));
	m_popupMenu.Append(ID_DEBUG_REMOVE_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_REMOVEBP"));
	m_popupMenu.Append(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, _guilang.get("GUI_MENU_EDITOR_CLEARBP"));

	//m_popupMenu.Append(ID_DEBUG_RUNTOCURSOR, "Run to cursor");

	m_popupMenu.AppendSeparator();

	m_popupMenu.Append(ID_BOOKMARK_ADD, _guilang.get("GUI_MENU_EDITOR_ADDBM"));
	m_popupMenu.Append(ID_BOOKMARK_REMOVE, _guilang.get("GUI_MENU_EDITOR_REMOVEBM"));
	m_popupMenu.Append(ID_BOOKMARK_CLEAR, _guilang.get("GUI_MENU_EDITOR_CLEARBM"));

	m_popupMenu.AppendSeparator();

	//m_menuAddWatch = m_popupMenu.Append(ID_DEBUG_WATCH_SELECTION, "Watch selection");
	m_menuFindProcedure = m_popupMenu.Append(ID_FIND_PROCEDURE, _guilang.get("GUI_MENU_EDITOR_FINDPROC", "$procedure"));
	m_menuFindInclude = m_popupMenu.Append(ID_FIND_INCLUDE, _guilang.get("GUI_MENU_EDITOR_FINDINCLUDE", "script"));
	m_menuShowValue = m_popupMenu.Append(ID_DEBUG_DISPLAY_SELECTION, _guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "selection"));
	m_menuHelpOnSelection = m_popupMenu.Append(ID_MENU_HELP_ON_ITEM, _guilang.get("GUI_TREE_PUP_HELPONITEM", "..."));
	m_menuRefactoring = m_popupMenu.Append(ID_REFACTORING_MENU, _guilang.get("GUI_MENU_EDITOR_REFACTORING"), m_refactoringMenu);
	m_popupMenu.AppendSeparator();
	m_popupMenu.Append(ID_UPPERCASE, _guilang.get("GUI_MENU_EDITOR_UPPERCASE"));
	m_popupMenu.Append(ID_LOWERCASE, _guilang.get("GUI_MENU_EDITOR_LOWERCASE"));


	int modmask =	wxSTC_MOD_INSERTTEXT
					| wxSTC_MOD_DELETETEXT
					//| wxSTC_MOD_CHANGESTYLE
					| wxSTC_PERFORMED_UNDO
					| wxSTC_PERFORMED_REDO;

	this->SetModEventMask(modmask);
}

//////////////////////////////////////////////////////////////////////////////
///  public destructor ~ChameleonEditor
///  Handles the pseudo-reference counting for the editor's project
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
NumeReEditor::~NumeReEditor()
{
	if (m_project && m_project->IsSingleFile())
	{
		delete m_project;
		m_project = nullptr;
	}
	else if (m_project)
	{
		m_project->RemoveEditor(this);
	}
}



//////////////////////////////////////////////////////////////////////////////
///  public SaveFileLocal
///  Saves the editor's contents with the current filename
///
///  @return bool Whether or not the save succeeded
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::SaveFileLocal()
{
	return SaveFile(m_fileNameAndPath.GetFullPath());
}

//////////////////////////////////////////////////////////////////////////////
///  public SaveFile
///  Saves the editor's contents with the given filename
///
///  @param  filename const wxString & The filename to save to
///
///  @return bool     Whether or not the save succeeded
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::SaveFile( const wxString& filename )
{
	// return if no change
	if (!Modified() && filename.IsEmpty())
	{
		return true;
	}

	wxFileName fn(filename);

	// save edit in file and clear undo
	if (!filename.IsEmpty())
	{
		m_simpleFileName = fn.GetFullName();
	}

	// create a backup of the original file, if it exists
	if (wxFileExists(filename))
	{
		wxCopyFile(filename, filename + ".backup", true);
	}

	bool bWriteSuccess = false;

	// Write the file depending on its type
	if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || filename.find("numere.history") != string::npos)
    {
        bWriteSuccess = SaveNumeReFile(filename);
    }
    else
    {
        bWriteSuccess = SaveGeneralFile(filename);
    }

	// Check the contents of the newly created file
	wxFile filecheck;
	filecheck.Open(filename);
	if (!bWriteSuccess)
	{
		// if the contents are not matching, restore the backup and signalize that an error occured
		if (wxFileExists(filename + ".backup"))
			wxCopyFile(filename + ".backup", filename, true);
		return false;
	}
	else if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC) && filecheck.Length() != this->GetLength() - countUmlauts(this->GetText().ToStdString()))
	{
        // if the contents are not matching, restore the backup and signalize that an error occured
		if (wxFileExists(filename + ".backup"))
			wxCopyFile(filename + ".backup", filename, true);
		return false;
	}
	else if ((m_fileType != FILE_NSCR && m_fileType != FILE_NPRC) && !filecheck.Length() && this->GetLength())
    {
		// if the contents are not matching, restore the backup and signalize that an error occured
		if (wxFileExists(filename + ".backup"))
			wxCopyFile(filename + ".backup", filename, true);
		return false;
    }

	// Only mark the editor as saved, if the saving process was successful
	markSaved();
	EmptyUndoBuffer();
	SetSavePoint();

	m_filetime = fn.GetModificationTime();
	m_bSetUnsaved = false;
	return true;
}


// Saves a NumeRe-specific file and tries to stick to ASCII encoding
bool NumeReEditor::SaveNumeReFile(const wxString& filename)
{
    // create a std::ofstream to avoid encoding issues
	std::ofstream file;
	file.open(filename.ToStdString().c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

	if (!file.is_open())
		return false;

	// write the contents of the file linewise
	for (int i = 0; i < this->GetLineCount(); i++)
	{
		file << this->GetLine(i).ToStdString();
	}

	// flush the files content explicitly
	file.flush();
	file.close();

	return true;
}

// Saves a general file without touching the encoding
bool NumeReEditor::SaveGeneralFile(const wxString& filename)
{
    // Create file and check, if it has been opened successfully
    wxFile file (filename, wxFile::write);

    if (!file.IsOpened())
    {
        return false;
    }

    // Get text and write it to the file
    wxString buf = GetText();
    bool okay = file.Write(buf.ToStdString().c_str(), buf.ToStdString().length());

    file.Close();

    // Notify caller that there was an error during writing
    if (!okay)
    {
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
///  public LoadFileText
///  Loads a file from the given string
///
///  @param  fileContents wxString  The text of the file
///
///  @return bool         Whether or not the load succeeded
///
///  @remarks  This isn't actually used right now... probably ought to be cleaned up
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::LoadFileText(wxString fileContents)
{
	if (fileContents.Length() > 0)
	{
		m_bLoadingFile = true;
		defaultPage = false;
		SetReadOnly(false);
		if (getEditorSetting(SETTING_USETXTADV))
			ToggleSettings(SETTING_USETXTADV);
		ClearAll();
		StyleClearAll();
		InsertText(0, fileContents);
	}

	EmptyUndoBuffer();

	// determine and set EOL mode
	int eolMode = -1;

	bool eolMix = false;

	wxString eolName;

	if ( fileContents.Contains("\r\n") )
	{
		eolMode = wxSTC_EOL_CRLF;

		eolName = _("CR+LF (Windows)");
	}
	else if ( fileContents.Contains("\r") )
	{
		if (eolMode != -1)
		{
			eolMix = true;
		}
		else
		{
			eolMode = wxSTC_EOL_CR;

			eolName = _("CR (Macintosh)");
		}
	}
	else if ( fileContents.Contains("\n") )
	{
		if (eolMode != -1)
		{
			eolMix = true;
		}
		else
		{
			eolMode = wxSTC_EOL_LF;

			eolName = _("LF (Unix)");
		}
	}

	if ( eolMode != -1 )
	{
		if ( eolMix && wxMessageBox(_("Convert all line endings to ")
									+ eolName + _("?"), _("Line endings"), wxYES_NO | wxICON_QUESTION)
				== wxYES )
		{
			ConvertEOLs(eolMode);

			// set staus bar text
			// g_statustext->Clear();
			//g_statustext->Append(_("Converted line endings to "));
			//g_statustext->Append(eolName);
		}

		SetEOLMode(eolMode);
	}

	m_bLoadingFile = false;
	UpdateSyntaxHighlighting(true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////
///  public Modified
///  Checks whether or not the editor has been modified
///
///  @return bool Whether or not the editor has been modified
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::Modified ()
{
	// return modified state
	bool modified = GetModify();
	bool readonly = !GetReadOnly();
	bool canundo = CanUndo();



	bool isModified = (modified && readonly && canundo) || m_bSetUnsaved;
	return isModified;
}

//////////////////////////////////////////////////////////////////////////////
///  public OnChar
///  Handles auto-indentation and such whenever the user enters a character
///
///  @param  event wxStyledTextEvent & The generated event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnChar( wxStyledTextEvent& event )
{
	ClearDblClkIndicator();
	//CallAfter(NumeReEditor::AsynchActions);
	const wxChar chr = event.GetKey();
	const int currentLine = GetCurrentLine();
	const int currentPos = GetCurrentPos();
	const int wordstartpos = WordStartPosition(currentPos, true);

	MarkerDeleteAll(MARKER_FOCUSEDLINE);
	if (chr == WXK_TAB)
	{
		event.Skip(true);

		int startLine = LineFromPosition(GetSelectionStart());
		int endline = LineFromPosition(GetSelectionEnd());
		int newStartPos = PositionFromLine(startLine);
		int newEndPos = PositionFromLine(endline) + LineLength(endline);
		bool doIndent = event.GetShift();
		int indentWidth = this->GetIndent();

		this->SetSelection(newStartPos, newEndPos);

		for (int i = startLine; i <= endline; i++)
		{
			int lineIndent = this->GetLineIndentation(i);

			if (doIndent)
			{
				this->SetLineIndentation(i, lineIndent + indentWidth);
			}
			else
			{
				this->SetLineIndentation(i, lineIndent - indentWidth);
			}
		}
	}

	if (chr == '\n')
	{
		markModified(currentLine);
		int previousLineInd = 0;

		if (currentLine > 0)
		{
			markModified(currentLine - 1);
			previousLineInd = GetLineIndentation(currentLine - 1);
		}

		if (previousLineInd == 0)
		{
			return;
		}

		SetLineIndentation(currentLine, previousLineInd);

		// If tabs are being used then change previousLineInd to tab sizes
		if (GetUseTabs())
		{
			previousLineInd /= GetTabWidth();
		}

		GotoPos(PositionFromLine(currentLine) + previousLineInd);
		return;
	}

	if (chr == '"')
	{
		if (GetStyleAt(currentPos) != wxSTC_NSCR_STRING && GetStyleAt(currentPos) != wxSTC_NPRC_STRING)
			InsertText(currentPos, "\"");
	}
	if (chr == '(' || chr == '[' || chr == '{')
	{
		int nMatchingPos = currentPos;
		if (this->HasSelection())
			nMatchingPos = this->GetSelectionEnd();
		if (this->BraceMatch(currentPos - 1) == wxSTC_INVALID_POSITION)
		{
			if (chr == '(')
				InsertText(nMatchingPos, ")");
			else if (chr == '[')
				InsertText(nMatchingPos, "]");
			else
				InsertText(nMatchingPos, "}");
		}
	}

	int lenEntered = currentPos - wordstartpos;

	if (lenEntered > 1
			&& (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
			&& GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_LINE
			&& GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_BLOCK
			&& GetStyleAt(wordstartpos) != wxSTC_NSCR_STRING
			&& GetStyleAt(wordstartpos) != wxSTC_NSCR_PROCEDURES
			&& GetStyleAt(wordstartpos) != wxSTC_NPRC_COMMENT_LINE
			&& GetStyleAt(wordstartpos) != wxSTC_NPRC_COMMENT_BLOCK
			&& GetStyleAt(wordstartpos) != wxSTC_NPRC_STRING)
	{
		this->AutoCompSetIgnoreCase(true);
		this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
		wxString sAutoCompList = generateAutoCompList(GetTextRange(wordstartpos, currentPos), _syntax->getAutoCompList(GetTextRange(wordstartpos, currentPos).ToStdString()));
		if (sAutoCompList.length())
			this->AutoCompShow(lenEntered, sAutoCompList);
	}
	else if (lenEntered > 1
			 && m_fileType == FILE_MATLAB
			 && GetStyleAt(wordstartpos) != wxSTC_MATLAB_COMMENT
			 && GetStyleAt(wordstartpos) != wxSTC_MATLAB_STRING)
	{
		this->AutoCompSetIgnoreCase(true);
		this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
		wxString sAutoCompList = generateAutoCompList(GetTextRange(wordstartpos, currentPos), _syntax->getAutoCompListMATLAB(GetTextRange(wordstartpos, currentPos).ToStdString()));
		if (sAutoCompList.length())
			this->AutoCompShow(lenEntered, sAutoCompList);
	}
	else if (lenEntered > 1
			 && m_fileType == FILE_CPP
			 && GetStyleAt(wordstartpos) != wxSTC_C_COMMENT
			 && GetStyleAt(wordstartpos) != wxSTC_C_COMMENTLINE
			 && GetStyleAt(wordstartpos) != wxSTC_C_STRING)
	{
		this->AutoCompSetIgnoreCase(true);
		this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
		wxString sAutoCompList = generateAutoCompList(GetTextRange(wordstartpos, currentPos), _syntax->getAutoCompListCPP(GetTextRange(wordstartpos, currentPos).ToStdString()));
		if (sAutoCompList.length())
			this->AutoCompShow(lenEntered, sAutoCompList);
	}
	else if (lenEntered > 1
			 && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
			 && GetStyleAt(wordstartpos) == wxSTC_NSCR_PROCEDURES)
	{
		wxString sNamespace;
		wxString sSelectedNamespace;
		int nNameSpacePosition = wordstartpos;
		while (GetStyleAt(nNameSpacePosition - 1) == wxSTC_NSCR_PROCEDURES && GetCharAt(nNameSpacePosition - 1) != '$')
			nNameSpacePosition--;
		if (nNameSpacePosition == wordstartpos)
		{
			sNamespace = FindNameSpaceOfProcedure(wordstartpos) + "~";
		}
		else
			sSelectedNamespace = GetTextRange(nNameSpacePosition, wordstartpos);

		// If namespace == "this~" then replace it with the current namespace
		if (sNamespace == "this~")
		{
			string filename = GetFileNameAndPath().ToStdString();
			filename = replacePathSeparator(filename);
			vector<string> vPaths = m_terminal->getPathSettings();
			if (filename.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
			{
				filename.erase(0, vPaths[PROCPATH].length());
				if (filename.find('/') != string::npos)
					filename.erase(filename.rfind('/') + 1);
				while (filename.front() == '/')
					filename.erase(0, 1);
				while (filename.find('/') != string::npos)
					filename[filename.find('/')] = '~';
				sNamespace = filename;
			}
			else
				sNamespace = "";
		}
		else if (sSelectedNamespace == "this~")
		{
			string filename = GetFileNameAndPath().ToStdString();
			filename = replacePathSeparator(filename);
			vector<string> vPaths = m_terminal->getPathSettings();
			if (filename.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
			{
				filename.erase(0, vPaths[PROCPATH].length());
				if (filename.find('/') != string::npos)
					filename.erase(filename.rfind('/') + 1);
				while (filename.front() == '/')
					filename.erase(0, 1);
				while (filename.find('/') != string::npos)
					filename[filename.find('/')] = '~';
				sSelectedNamespace = filename;
			}
			else
				sSelectedNamespace = "";
		}
		// If namespace == "thisfile~" then search for all procedures in the current file and use them as the
		// autocompletion list entries
		else if (sNamespace == "thisfile" || sNamespace == "thisfile~" || sSelectedNamespace == "thisfile" || sSelectedNamespace == "thisfile~")
		{
			this->AutoCompSetIgnoreCase(true);
			this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
			this->AutoCompShow(lenEntered, FindProceduresInCurrentFile(GetTextRange(wordstartpos, currentPos), sSelectedNamespace));
			this->Colourise(0, -1);
			event.Skip();
			return;
		}
		// If namespace == "main~" (or similiar) then clear it's contents
		else if (sNamespace == "main" || sNamespace == "main~" || sNamespace == "~")
			sNamespace = "";
		else if (sSelectedNamespace == "main" || sSelectedNamespace == "main~" || sSelectedNamespace == "~")
			sSelectedNamespace = "";

		this->AutoCompSetIgnoreCase(true);
		this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
		this->AutoCompShow(lenEntered, _syntax->getProcAutoCompList(GetTextRange(wordstartpos, currentPos).ToStdString(), sNamespace.ToStdString(), sSelectedNamespace.ToStdString()));
	}
	else if (lenEntered > 1
			 && !(m_fileType == FILE_NSCR || m_fileType == FILE_NPRC))
	{
		this->AutoCompSetIgnoreCase(true);
		this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
		this->AutoCompShow(lenEntered, generateAutoCompList(GetTextRange(wordstartpos, currentPos), ""));
	}
	this->Colourise(0, -1);
	//CallAfter(NumeReEditor::HandleFunctionCallTip);
	event.Skip();
}


void NumeReEditor::MakeBraceCheck()
{
	char CurrentChar = this->GetCharAt(this->GetCurrentPos());
	char PrevChar = 0;
	if (this->GetCurrentPos())
		PrevChar = this->GetCharAt(this->GetCurrentPos() - 1);
	if (CurrentChar == ')' || CurrentChar == ']' || CurrentChar == '}')
		getMatchingBrace(this->GetCurrentPos());
	else if (PrevChar == '(' || PrevChar == '[' || PrevChar == '{')
		getMatchingBrace(this->GetCurrentPos() - 1);
	else if (CurrentChar == '(' || CurrentChar == '[' || CurrentChar == '{')
		getMatchingBrace(this->GetCurrentPos());
	else if (PrevChar == ')' || PrevChar == ']' || PrevChar == '}')
		getMatchingBrace(this->GetCurrentPos() - 1);
	else
	{
		this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
		long int maxpos = this->GetLastPosition();
		this->IndicatorClearRange(0, maxpos);
		this->BraceBadLight(wxSTC_INVALID_POSITION);
		this->BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
	}
	applyStrikeThrough();
	return;
}

void NumeReEditor::MakeBlockCheck()
{
	if (this->m_fileType != FILE_NSCR && this->m_fileType != FILE_NPRC && !FILE_MATLAB)
		return;
	this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BLOCK);
	this->IndicatorClearRange(0, GetLastPosition());
	this->SetIndicatorCurrent(HIGHLIGHT_NOT_MATCHING_BLOCK);
	this->IndicatorClearRange(0, GetLastPosition());
	if (GetStyleAt(GetCurrentPos()) != wxSTC_NSCR_COMMAND && GetStyleAt(GetCurrentPos()) != wxSTC_NPRC_COMMAND && GetStyleAt(GetCurrentPos()) != wxSTC_MATLAB_KEYWORD
			&& !(GetCurrentPos() && (GetStyleAt(GetCurrentPos() - 1) == wxSTC_NSCR_COMMAND || GetStyleAt(GetCurrentPos() - 1) == wxSTC_NPRC_COMMAND || GetStyleAt(GetCurrentPos() - 1) == wxSTC_MATLAB_KEYWORD)))
	{
		return;
	}
	wxString currentWord = this->GetTextRange(WordStartPosition(GetCurrentPos(), true), WordEndPosition(GetCurrentPos(), true));
	if (currentWord == "if"
			|| currentWord == "else"
			|| currentWord == "elseif"
			|| currentWord == "endif"
			|| currentWord == "for"
			|| currentWord == "endfor"
			|| currentWord == "while"
			|| currentWord == "endwhile"
			|| currentWord == "compose"
			|| currentWord == "endcompose"
			|| currentWord == "procedure"
			|| currentWord == "endprocedure"
			|| currentWord == "end"
			|| currentWord == "function"
			|| currentWord == "classdef"
			|| currentWord == "properties"
			|| currentWord == "methods"
			|| currentWord == "switch"
			|| currentWord == "case"
			|| currentWord == "otherwise"
			|| currentWord == "try"
			|| currentWord == "catch"
	   )
	{
		getMatchingBlock(GetCurrentPos());
	}
}



// asynch update calls
void NumeReEditor::HandleFunctionCallTip()
{
	// do nothing if an autocompletion list is active
	if (this->AutoCompActive())
		return;
	// do nothing, if language is not supported
	if (this->getFileType() != FILE_NSCR && this->getFileType() != FILE_NPRC)
		return;
	int nStartingBrace = 0;
	int nArgStartPos = 0;
	string sFunctionContext = this->GetCurrentFunctionContext(nStartingBrace);
	string sDefinition;
	if (!sFunctionContext.length())
		return;
	if (sFunctionContext.front() == '$')
	{
		sDefinition = this->FindProcedureDefinition().ToStdString();
		if (sDefinition.find('\n') != string::npos)
            sDefinition.erase(sDefinition.find('\n'));
		if (sDefinition.find(')') != string::npos)
			sDefinition.erase(sDefinition.rfind(')') + 1);
	}
	else if (sFunctionContext.front() == '.')
	{
		sDefinition = this->GetMethodCallTip(sFunctionContext.substr(1));
		size_t nDotPos = sDefinition.find('.');
		if (sDefinition.find(')', nDotPos) != string::npos)
			sDefinition.erase(sDefinition.find(')', nDotPos) + 1);
		else
			sDefinition.erase(sDefinition.find(' ', nDotPos));
	}
	else
	{
		sDefinition = this->GetFunctionCallTip(sFunctionContext);
		if (sDefinition.find(')') != string::npos)
			sDefinition.erase(sDefinition.find(')') + 1);
	}

	if (!sDefinition.length())
		return;

	string sArgument = this->GetCurrentArgument(sDefinition, nStartingBrace, nArgStartPos);
	/*if (sArgument.length())
	{
	    if (sArgument.substr(0,3) == "STR")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_STR", sArgument);
	    else if (sArgument == "CHAR")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_CHAR", sArgument);
	    else if (sArgument == "MAT")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_MAT", sArgument);
	    else if (sArgument == "...")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_REPEATTYPE", sArgument);
	    else if (sArgument == "x" || sArgument == "y" || sArgument == "z" || sArgument == "x1" || sArgument == "x0")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_FLOAT", sArgument);
	    else if (sArgument == "l" || sArgument == "n" || sArgument == "m" || sArgument == "k" || sArgument == "P" || sArgument == "POS" || sArgument == "LEN")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_INTEGER", sArgument);
	    else if (sArgument == "theta" || sArgument == "phi")
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_ANGLE", sArgument);
	    else
	        sDefinition += "\n" + _guilang.get("GUI_EDITOR_ARGCALLTIP_NOHEURISTICS", sArgument);

	}*/
	if (this->CallTipActive() && this->CallTipStartPos() != nStartingBrace)
	{
		this->AdvCallTipCancel();
		this->AdvCallTipShow(nStartingBrace, sDefinition);
	}
	else if (!this->CallTipActive())
		this->AdvCallTipShow(nStartingBrace, sDefinition);
	if (sArgument.length())
		this->CallTipSetHighlight(nArgStartPos, nArgStartPos + sArgument.length());
}

string NumeReEditor::GetCurrentFunctionContext(int& nStartingBrace)
{
	int nCurrentLineStart = this->PositionFromLine(this->GetCurrentLine());
	int nCurrentPos = this->GetCurrentPos();

	for (int i = nCurrentPos; i > nCurrentLineStart; i--)
	{
		if (this->GetCharAt(i) == '('
				&& (this->BraceMatch(i) >= nCurrentPos || this->BraceMatch(i) == -1) // either no brace (yet) or the brace further right
				&& (this->GetStyleAt(i - 1) == wxSTC_NSCR_FUNCTION || this->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES || this->GetStyleAt(i - 1) == wxSTC_NSCR_METHOD))
		{
			nStartingBrace = i;
			if (this->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES)
				return this->FindMarkedProcedure(i - 1).ToStdString();
			else
			{
				if (this->GetStyleAt(i - 1) == wxSTC_NSCR_METHOD)
					return "." + this->GetTextRange(this->WordStartPosition(i - 1, true), this->WordEndPosition(i - 1, true)).ToStdString();
				return this->GetTextRange(this->WordStartPosition(i - 1, true), this->WordEndPosition(i - 1, true)).ToStdString();
			}
		}
	}
	return "";
}

string NumeReEditor::GetFunctionCallTip(const string& sFunctionName)
{
	string selection = sFunctionName;
	if (selection == "arcsin")
		selection = "asin";
	else if (selection == "arccos")
		selection = "acos";
	else if (selection == "arctan")
		selection = "atan";
	else if (selection == "arsinh")
		selection = "asinh";
	else if (selection == "arcosh")
		selection = "acosh";
	else if (selection == "artanh")
		selection = "atanh";
	return _guilang.get("PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(selection) + "_[*");
}

string NumeReEditor::GetMethodCallTip(const string& sMethodName)
{
	if (_guilang.get("PARSERFUNCS_LISTFUNC_METHOD_" + toUpperCase(sMethodName) + "_[STRING]") != "PARSERFUNCS_LISTFUNC_METHOD_" + toUpperCase(sMethodName) + "_[STRING]")
		return "STRINGVAR." + _guilang.get("PARSERFUNCS_LISTFUNC_METHOD_" + toUpperCase(sMethodName) + "_[STRING]");
	else
		return "TABLE()." + _guilang.get("PARSERFUNCS_LISTFUNC_METHOD_" + toUpperCase(sMethodName) + "_[DATA]");
}

string NumeReEditor::GetCurrentArgument(const string& sCallTip, int nStartingBrace, int& nArgStartPos)
{
	int nCurrentPos = this->GetCurrentPos();
	int nCurrentArg = 0;
	size_t nParensPos = 0;
	char currentChar;
	if (sCallTip.find('(') == string::npos)
		return "";
	nParensPos = sCallTip.find('(');
	if (sCallTip.find("().") != string::npos)
	{
		if (sCallTip.find('(', sCallTip.find("().") + 3) == string::npos)
			return "";
		nParensPos = sCallTip.find('(', sCallTip.find("().") + 3);
	}
	string sArgList = sCallTip.substr(nParensPos);
	sArgList.erase(getMatchingParenthesis(sArgList));

	for (int i = nStartingBrace + 1; i < nCurrentPos && i < this->GetLineEndPosition(this->GetCurrentLine()); i++)
	{
		if (this->GetStyleAt(i) == wxSTC_NSCR_STRING
				|| this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_LINE
				|| this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_BLOCK)
			continue;
		currentChar = this->GetCharAt(i);
		if (currentChar == ',')
			nCurrentArg++;
		if ((currentChar == '(' || currentChar == '[' || currentChar == '{')
				&& this->BraceMatch(i) != -1)
			i = this->BraceMatch(i);
	}

	for (size_t i = 1; i < sArgList.length(); i++)
	{
		if (!nCurrentArg && (sArgList[i - 1] == '(' || sArgList[i - 1] == ','))
		{
			nArgStartPos = i + nParensPos;
			string sArgument = sArgList.substr(i);
			/*if (sArgument.find(')') < sArgument.find(','))
			    sArgument.erase(sArgument.find(')'));
			else*/
			if (sArgument.find(',') != string::npos)
				sArgument.erase(sArgument.find(','));

			return sArgument;
		}
		if (sArgList[i] == ',')
			nCurrentArg--;
	}
	return "";
}

int NumeReEditor::CallTipStartPos()
{
	return m_nCallTipStart;
}

void NumeReEditor::AdvCallTipShow(int pos, const wxString& definition)
{
	m_nCallTipStart = pos;
	if (m_sCallTipContent != definition)
	{
		if (CallTipActive())
			CallTipCancel();
		m_sCallTipContent = definition;
	}
	CallTipShow(pos, definition);
}

void NumeReEditor::AdvCallTipCancel()
{
	m_nCallTipStart = 0;
	m_sCallTipContent.clear();
	CallTipCancel();
}

void NumeReEditor::OnKeyDn(wxKeyEvent& event)
{
	if (this->HasSelection()
			&& event.GetKeyCode() != WXK_SHIFT
			&& event.GetKeyCode() != WXK_CAPITAL
			&& event.GetKeyCode() != WXK_END
			&& event.GetKeyCode() != WXK_HOME)
	{
		char chr = event.GetKeyCode();
		if (event.ShiftDown() && (chr == '8' || chr == '9'))
		{
			this->BeginUndoAction();
			int selStart = this->GetSelectionStart();
			int selEnd = this->GetSelectionEnd() + 1;
			this->InsertText(selStart, "(");
			this->InsertText(selEnd, ")");
			if (chr == '8')
				this->GotoPos(selStart);
			else
				this->GotoPos(selEnd + 1);
			this->EndUndoAction();
			MakeBraceCheck();
			MakeBlockCheck();
			return;
		}
		else if (event.ShiftDown() && chr == '2')
		{
			this->BeginUndoAction();
			int selStart = this->GetSelectionStart();
			int selEnd = this->GetSelectionEnd() + 1;
			this->InsertText(selStart, "\"");
			this->InsertText(selEnd, "\"");
			this->GotoPos(selEnd + 1);
			this->EndUndoAction();
			MakeBraceCheck();
			MakeBlockCheck();
			return;
		}
		else if (event.ControlDown() && event.AltDown() && (chr == '8' || chr == '9')) // Alt Gr means CTRL+ALT
		{
			this->BeginUndoAction();
			int selStart = this->GetSelectionStart();
			int selEnd = this->GetSelectionEnd() + 1;
			this->InsertText(selStart, "[");
			this->InsertText(selEnd, "]");
			if (chr == '8')
				this->GotoPos(selStart);
			else
				this->GotoPos(selEnd + 1);
			this->EndUndoAction();
			MakeBraceCheck();
			MakeBlockCheck();
			return;
		}
		else if (event.ControlDown() && event.AltDown() && (chr == '7' || chr == '0'))
		{
			this->BeginUndoAction();
			int selStart = this->GetSelectionStart();
			int selEnd = this->GetSelectionEnd() + 1;
			this->InsertText(selStart, "{");
			this->InsertText(selEnd, "}");
			if (chr == '7')
				this->GotoPos(selStart);
			else
				this->GotoPos(selEnd + 1);
			this->EndUndoAction();
			MakeBraceCheck();
			MakeBlockCheck();
			return;
		}
	}
	OnKeyDown(event);
	if (this->GetSelections() <= 1)
		MakeBraceCheck();
	MakeBlockCheck();
	if (!event.ControlDown() && !event.ShiftDown())
		ClearDblClkIndicator();
	//event.Skip();
}

void NumeReEditor::OnKeyRel(wxKeyEvent& event)
{
	//wxMessageBox(wxString((char)this->GetCharAt(this->GetCurrentPos())));
	if (this->GetSelections() <= 1)
		MakeBraceCheck();
	MakeBlockCheck();
	event.Skip();
	CallAfter(NumeReEditor::AsynchActions);
	//AnalyseCode();
	//OnKeyUp(event);
}

void NumeReEditor::OnMouseUp(wxMouseEvent& event)
{
	//OnMouseLeftUp(event);
	MakeBraceCheck();
	MakeBlockCheck();
	CallAfter(NumeReEditor::AsynchActions);
	event.Skip();
}

void NumeReEditor::OnMouseDn(wxMouseEvent& event)
{
	//OnMouseLeftDown(event);
	if (!event.ControlDown())
		ClearDblClkIndicator();
	MakeBraceCheck();
	MakeBlockCheck();
	event.Skip();
}

void NumeReEditor::OnMouseDblClk(wxMouseEvent& event)
{
	int charpos = PositionFromPoint(event.GetPosition());
	int startPosition = WordStartPosition(charpos, true);
	int endPosition = WordEndPosition(charpos, true);
	wxString selection = this->GetTextRange(startPosition, endPosition);
	if (!selection.length())
	{
		event.Skip();
		return;
	}
	if (event.ControlDown() && this->HasSelection())
		this->AddSelection(endPosition, startPosition);
	else
		this->SetSelection(startPosition, endPosition);
	m_dblclkString = selection;
	long int maxpos = this->GetLastPosition();
	this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
	this->IndicatorClearRange(0, maxpos);
	this->IndicatorSetStyle(HIGHLIGHT_DBLCLK, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT_DBLCLK, 80);
	this->IndicatorSetForeground(HIGHLIGHT_DBLCLK, wxColor(0, 255, 0));

	unsigned int nPos = 0;
	unsigned int nCurr = 0;
	unsigned int nLength = endPosition - startPosition;
	vector<unsigned int> vSelectionList;

	while ((nPos = this->FindText(nCurr, maxpos, selection, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != string::npos)
	{
		vSelectionList.push_back(nPos);
		nCurr = nPos + nLength;//selection.length();
	}

	//this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);

	for (size_t i = 0; i < vSelectionList.size(); i++)
	{
		this->IndicatorFillRange(vSelectionList[i], nLength); //selection.length());
	}
	event.Skip();
}

void NumeReEditor::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if (GetCapture() == this)
	{
		ReleaseMouse();
		Refresh();
	}
}

void NumeReEditor::OnEnter(wxMouseEvent& event)
{
	if (g_findReplace != nullptr && g_findReplace->IsShown())
	{
		event.Skip();
		return;
	}
	this->SetFocus();
	event.Skip();
}

void NumeReEditor::OnLeave(wxMouseEvent& event)
{
	if (this->CallTipActive())
		this->AdvCallTipCancel();
	event.Skip();
}


void NumeReEditor::OnLoseFocus(wxFocusEvent& event)
{
	if (this->CallTipActive())
		this->AdvCallTipCancel();
	event.Skip();
}

void NumeReEditor::OnMouseDwell(wxStyledTextEvent& event)
{
	//CallAfter(NumeReEditor::AsynchEvaluations);
	if ((m_fileType != FILE_NSCR && m_fileType != FILE_NPRC) || m_PopUpActive || !this->HasFocus())
		return;
	//wxPoint pos = event.GetPosition();
	int charpos = event.GetPosition(); //PositionFromPoint(pos);
	int startPosition = WordStartPosition(charpos, true);
	int endPosition = WordEndPosition(charpos, true);

	wxString selection = this->GetTextRange(startPosition, endPosition);

	if (this->GetStyleAt(charpos) == wxSTC_NSCR_FUNCTION || this->GetStyleAt(charpos) == wxSTC_NPRC_FUNCTION)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		size_t lastpos = 22;
		this->AdvCallTipShow(startPosition, addLinebreaks(realignLangString(GetFunctionCallTip(selection.ToStdString()), lastpos)));
		this->CallTipSetHighlight(0, lastpos);
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_COMMAND || this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURE_COMMANDS || this->GetStyleAt(charpos) == wxSTC_NPRC_COMMAND)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		if (selection == "showf")
			selection = "show";
		else if (selection == "view")
			selection = "edit";
		else if (selection == "undef")
			selection = "undefine";
		else if (selection == "ifndef")
			selection = "ifndefined";
		else if (selection == "redef")
			selection = "redefine";
		else if (selection == "del")
			selection = "delete";
		else if (selection == "search")
			selection = "find";
		else if (selection == "vector")
			selection = "vect";
		else if (selection == "vector3d")
			selection = "vect3d";
		else if (selection == "graph")
			selection = "plot";
		else if (selection == "graph3d")
			selection = "plot3d";
		else if (selection == "gradient")
			selection = "grad";
		else if (selection == "gradient3d")
			selection = "grad3d";
		else if (selection == "surface")
			selection = "surf";
		else if (selection == "surface3d")
			selection = "surf3d";
		else if (selection == "meshgrid")
			selection = "mesh";
		else if (selection == "meshgrid3d")
			selection = "mesh3d";
		else if (selection == "density")
			selection = "dens";
		else if (selection == "density3d")
			selection = "dens3d";
		else if (selection == "contour")
			selection = "cont";
		else if (selection == "contour3d")
			selection = "cont3d";
		else if (selection == "mtrxop")
			selection = "matop";
		else if (selection == "man")
			selection = "help";
		else if (selection == "credits" || selection == "info")
			selection = "about";
		else if (selection == "integrate2" || selection == "integrate2d")
			selection = "integrate";

		size_t lastpos = 0;
		if (selection == "if" || selection == "endif" || selection == "else" || selection == "elseif")
		{
			size_t nLength = 0;
			size_t lastpos2 = 0;
			string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_IF_*"), lastpos)) + "\n  [...]\n";
			if (selection != "if")
				nLength = sBlock.length();

			sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ELSEIF_*"), lastpos2)) + "\n  [...]\n";
			if (selection != "if" && selection != "elseif")
				nLength = sBlock.length() + countUmlauts(sBlock);

			sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ELSE_*")) + "\n  [...]\n";
			if (selection != "if" && selection != "elseif" && selection != "else")
				nLength = sBlock.length() + countUmlauts(sBlock);

			sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDIF_*"));
			this->AdvCallTipShow(startPosition, sBlock);
			if (selection == "if")
				this->CallTipSetHighlight(nLength, lastpos + nLength);
			else if (selection == "elseif")
				this->CallTipSetHighlight(nLength, lastpos2 + nLength);
			else
				this->CallTipSetHighlight(nLength, 13 + nLength);
		}
		else if (selection == "for" || selection == "endfor")
		{
			size_t nLength = 0;
			size_t lastpos2 = 0;
			string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_FOR_*"), lastpos)) + "\n  [...]\n";
			if (selection != "for")
				nLength = sBlock.length() + countUmlauts(sBlock);
			sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDFOR_*"), lastpos2));
			this->AdvCallTipShow(startPosition, sBlock);
			if (nLength)
				this->CallTipSetHighlight(nLength, lastpos2 + nLength);
			else
				this->CallTipSetHighlight(nLength, lastpos + nLength);
		}
		else if (selection == "while" || selection == "endwhile")
		{
			size_t nLength = 0;
			size_t lastpos2 = 0;
			string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_WHILE_*"), lastpos)) + "\n  [...]\n";
			if (selection != "while")
				nLength = sBlock.length() + countUmlauts(sBlock);
			sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDWHILE_*"), lastpos2));
			this->AdvCallTipShow(startPosition, sBlock);
			if (nLength)
				this->CallTipSetHighlight(nLength, lastpos2 + nLength);
			else
				this->CallTipSetHighlight(nLength, lastpos + nLength);
		}
		else if (selection == "procedure" || selection == "endprocedure")
		{
			size_t nLength = 0;
			string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_PROCEDURE_*"), lastpos)) + "\n  [...]\n";
			if (selection != "procedure")
				nLength = sBlock.length() + countUmlauts(sBlock);
			sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDPROCEDURE_*"));
			this->AdvCallTipShow(startPosition, sBlock);
			if (nLength)
				this->CallTipSetHighlight(nLength, 13 + nLength);
			else
				this->CallTipSetHighlight(nLength, lastpos + nLength);
		}
		else if (selection == "compose" || selection == "endcompose")
		{
			size_t nLength = 0;
			string sBlock = addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_COMPOSE_*")) + "\n  [...]\n";
			if (selection != "compose")
				nLength = sBlock.length() + countUmlauts(sBlock);
			sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDCOMPOSE_*"));
			this->AdvCallTipShow(startPosition, sBlock);
			this->CallTipSetHighlight(nLength, 13 + nLength);
		}
		else
		{
			this->AdvCallTipShow(startPosition, addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_" + toUpperCase(selection.ToStdString()) + "_*"), lastpos)));
			this->CallTipSetHighlight(0, lastpos);
		}
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES || this->GetStyleAt(charpos) == wxSTC_NPRC_PROCEDURES)
	{
		if (GetCharAt(charpos) != '$')
            startPosition--;

        if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();

		wxString proc = FindMarkedProcedure(charpos);
		if (!proc.length())
			return;
		wxString procdef = FindProcedureDefinition();
		wxString flags = "";
		if (!procdef.length())
			procdef = m_clickedProcedure + "(...)";
		if (procdef.find("::") != string::npos)
		{
			flags = procdef.substr(procdef.find("::"));
			procdef.erase(procdef.find("::"));
		}
		else if (procdef.find('\n') != string::npos)
        {
            flags = procdef.substr(procdef.find('\n'));
            procdef.erase(procdef.find('\n'));
        }


		if (flags.find('\n') != string::npos)
            this->AdvCallTipShow(startPosition, procdef + flags);
        else
            this->AdvCallTipShow(startPosition, procdef + flags + "\n    " + _guilang.get("GUI_EDITOR_CALLTIP_PROC2"));
		this->CallTipSetHighlight(0, procdef.length());
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_OPTION || this->GetStyleAt(charpos) == wxSTC_NPRC_OPTION)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		selection = _guilang.get("GUI_EDITOR_CALLTIP_OPT_" + toUpperCase(selection.ToStdString()));
		size_t highlightlength = selection.length();
		if (selection.find(' ') != string::npos)
			highlightlength = selection.find(' ');
		this->AdvCallTipShow(startPosition, "Option: " + selection);
		this->CallTipSetHighlight(8, 8 + highlightlength);
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_METHOD || this->GetStyleAt(charpos) == wxSTC_NPRC_METHOD)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		selection = GetMethodCallTip(selection.ToStdString());
		size_t highlightlength;
		size_t highlightStart = selection.find('.') + 1;
		if (selection.find(' ') != string::npos)
			highlightlength = selection.find(' ');
		this->AdvCallTipShow(startPosition, addLinebreaks(realignLangString(selection.ToStdString(), highlightlength)));
		this->CallTipSetHighlight(highlightStart, highlightlength);
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_PREDEFS || this->GetStyleAt(charpos) == wxSTC_NPRC_PREDEFS)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		size_t highlightLength = 10;
		this->AdvCallTipShow(startPosition, addLinebreaks(realignLangString(_guilang.get("GUI_EDITOR_CALLTIP_" + toUpperCase(selection.ToStdString())), highlightLength)));
		this->CallTipSetHighlight(0, highlightLength);
	}
	else if (this->GetStyleAt(charpos) == wxSTC_NSCR_CONSTANTS || this->GetStyleAt(charpos) == wxSTC_NPRC_CONSTANTS)
	{
		if (this->CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            this->AdvCallTipCancel();
		string sCalltip = _guilang.get("GUI_EDITOR_CALLTIP_CONST" + toUpperCase(selection.ToStdString()) + "_*");
		if (selection == "_G")
			sCalltip = _guilang.get("GUI_EDITOR_CALLTIP_CONST_GRAV_*");
		this->AdvCallTipShow(startPosition, sCalltip);
		this->CallTipSetHighlight(0, sCalltip.find('='));
	}
}

void NumeReEditor::OnIdle(wxIdleEvent& event)
{
	if (!m_modificationHappened)
		return;
	m_modificationHappened = false;
	CallAfter(NumeReEditor::AsynchEvaluations);
}

void NumeReEditor::OnSavePointReached(wxStyledTextEvent& event)
{
	markSaved();
	event.Skip();
}

void NumeReEditor::ToggleCommentLine()
{
	if (m_fileType == FILE_NONSOURCE)
		return;
	int nFirstLine = 0;
	int nLastLine = 0;
	int nSelectionStart = -1;
	int nSelectionEnd = 0;

	if (this->HasSelection())
	{
		nSelectionStart = this->GetSelectionStart();
		nSelectionEnd = this->GetSelectionEnd();
		nFirstLine = this->LineFromPosition(nSelectionStart);
		nLastLine = this->LineFromPosition(nSelectionEnd);
	}
	else
	{
		nFirstLine = this->GetCurrentLine();
		nLastLine = nFirstLine;
	}
	this->BeginUndoAction();
	for (int i = nFirstLine; i <= nLastLine; i++)
	{
		int position = this->PositionFromLine(i);
		while (this->GetCharAt(position) == ' ' || this->GetCharAt(position) == '\t')
			position++;
		int style = this->GetStyleAt(position);
		//int style = this->GetStyleAt(this->PositionFromLine(i));
		if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
				&& (style == wxSTC_NPRC_COMMENT_LINE || style == wxSTC_NSCR_COMMENT_LINE))
		{
			if (this->GetCharAt(position + 2) == ' ')
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 3)
					nSelectionStart -= 3;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 3);
				nSelectionEnd -= 3;
			}
			else
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 2)
					nSelectionStart -= 2;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 2);
				nSelectionEnd -= 2;
			}
		}
		else if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
				 && !(style == wxSTC_NPRC_COMMENT_LINE || style == wxSTC_NSCR_COMMENT_LINE))
		{
			this->InsertText(this->PositionFromLine(i), "## " );
			if (nSelectionStart >= 0)
			{
				nSelectionStart += 3;
				nSelectionEnd += 3;
			}
		}
		else if (m_fileType == FILE_TEXSOURCE && GetStyleAt(position + 1) == wxSTC_TEX_DEFAULT && GetCharAt(position) == '%')
		{
			if (this->GetCharAt(position + 1) == ' ')
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 2)
					nSelectionStart -= 2;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 2);
				nSelectionEnd -= 2;
			}
			else
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 1)
					nSelectionStart -= 1;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 1);
				nSelectionEnd -= 1;
			}
		}
		else if (m_fileType == FILE_TEXSOURCE && GetStyleAt(position + 1) != wxSTC_TEX_DEFAULT && GetCharAt(position) != '%')
		{
			this->InsertText(this->PositionFromLine(i), "% " );
			if (nSelectionStart >= 0)
			{
				nSelectionStart += 2;
				nSelectionEnd += 2;
			}
		}
		else if (m_fileType == FILE_DATAFILES && style == wxSTC_MATLAB_COMMENT)
		{
			if (this->GetCharAt(position + 1) == ' ')
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 2)
					nSelectionStart -= 2;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 2);
				nSelectionEnd -= 2;
			}
			else
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 1)
					nSelectionStart -= 1;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 1);
				nSelectionEnd -= 1;
			}
		}
		else if (m_fileType == FILE_DATAFILES && style != wxSTC_MATLAB_COMMENT)
		{
			this->InsertText(this->PositionFromLine(i), "# " );
			if (nSelectionStart >= 0)
			{
				nSelectionStart += 2;
				nSelectionEnd += 2;
			}
		}
		else if (m_fileType == FILE_MATLAB && style == wxSTC_MATLAB_COMMENT)
		{
			if (this->GetCharAt(position + 1) == ' ')
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 2)
					nSelectionStart -= 2;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 2);
				nSelectionEnd -= 2;
			}
			else
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 1)
					nSelectionStart -= 1;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 1);
				nSelectionEnd -= 1;
			}
		}
		else if (m_fileType == FILE_MATLAB && style != wxSTC_MATLAB_COMMENT)
		{
			this->InsertText(this->PositionFromLine(i), "% " );
			if (nSelectionStart >= 0)
			{
				nSelectionStart += 2;
				nSelectionEnd += 2;
			}
		}
		else if (m_fileType == FILE_CPP && style == wxSTC_C_COMMENTLINE)
		{
			if (this->GetCharAt(position + 1) == ' ')
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 2)
					nSelectionStart -= 2;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 3);
				nSelectionEnd -= 2;
			}
			else
			{
				if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + 1)
					nSelectionStart -= 1;
				else if (i == nFirstLine && nSelectionStart >= 0)
					nSelectionStart = position;
				this->DeleteRange(position, 2);
				nSelectionEnd -= 1;
			}
		}
		else if (m_fileType == FILE_CPP && style != wxSTC_C_COMMENTLINE)
		{
			this->InsertText(this->PositionFromLine(i), "// " );
			if (nSelectionStart >= 0)
			{
				nSelectionStart += 3;
				nSelectionEnd += 3;
			}
		}
	}
	if (nSelectionStart >= 0)
	{
		this->SetSelection(nSelectionStart, nSelectionEnd);
	}
	this->EndUndoAction();
}

void NumeReEditor::ToggleCommentSelection()
{
	if (m_fileType == FILE_NONSOURCE)
		return;
	if (!this->HasSelection())
		return;
	int nFirstPosition = this->GetSelectionStart();
	int nLastPosition = this->GetSelectionEnd();
	int nSelectionStart = nFirstPosition;
	int nSelectionEnd = nLastPosition;
	int style = GetStyleAt(nFirstPosition);

	if (m_fileType != FILE_NSCR && m_fileType != FILE_NPRC)
	{
		ToggleCommentLine();
		return;
	}
	this->BeginUndoAction();
	if (style == wxSTC_NPRC_COMMENT_BLOCK || style == wxSTC_NSCR_COMMENT_BLOCK)
	{
		// Position before
		while (nFirstPosition && (GetStyleAt(nFirstPosition - 1) == wxSTC_NPRC_COMMENT_BLOCK || GetStyleAt(nFirstPosition - 1) == wxSTC_NSCR_COMMENT_BLOCK))
		{
			nFirstPosition--;
		}
		if (GetStyleAt(nLastPosition) != wxSTC_NPRC_COMMENT_BLOCK || GetStyleAt(nLastPosition) != wxSTC_NSCR_COMMENT_BLOCK)
			nLastPosition = nFirstPosition;
		// Position after
		while (nLastPosition < this->GetLastPosition() && (GetStyleAt(nLastPosition) == wxSTC_NPRC_COMMENT_BLOCK || GetStyleAt(nLastPosition) == wxSTC_NSCR_COMMENT_BLOCK))
		{
			nLastPosition++;
		}
		//wxString text = GetTextRange(nLastPosition-3, nLastPosition);
		if (this->GetTextRange(nLastPosition - 3, nLastPosition) == " *#")
		{
			if (nSelectionEnd > nLastPosition - 3)
				nSelectionEnd -= 3;
			this->DeleteRange(nLastPosition - 3, 3);
		}
		else
		{
			if (nSelectionEnd > nLastPosition - 2)
				nSelectionEnd -= 2;
			this->DeleteRange(nFirstPosition - 2, 2);
		}
		//text = GetTextRange(nFirstPosition, nFirstPosition+3);
		if (this->GetTextRange(nFirstPosition, nFirstPosition + 3) == "#* ")
		{
			if (nFirstPosition != nSelectionStart)
				nSelectionStart -= 3;
			this->DeleteRange(nFirstPosition, 3);
			nSelectionEnd -= 3;
		}
		else
		{
			if (nFirstPosition != nSelectionStart)
				nSelectionStart -= 2;
			this->DeleteRange(nFirstPosition, 2);
			nSelectionEnd -= 2;
		}
	}
	else if (!(style == wxSTC_NPRC_COMMENT_LINE || style == wxSTC_NSCR_COMMENT_LINE))
	{
		this->InsertText(nFirstPosition, "#* ");
		this->InsertText(nLastPosition + 3, " *#");
		nSelectionEnd += 3;
		nSelectionStart += 3;
	}
	this->EndUndoAction();
	this->SetSelection(nSelectionStart, nSelectionEnd);
}

void NumeReEditor::FoldAll()
{
	for (int i = this->GetLineCount() - 1; i >= 0; i--)
	{
		if (this->GetFoldLevel(i) & wxSTC_FOLDLEVELHEADERFLAG && this->GetFoldExpanded(i))
			this->ToggleFold(i);
	}
}

void NumeReEditor::UnfoldAll()
{
	for (int i = 0; i < this->GetLineCount(); i++)
	{
		if (this->GetFoldLevel(i) & wxSTC_FOLDLEVELHEADERFLAG && !this->GetFoldExpanded(i))
			this->ToggleFold(i);
	}
}

// This member function is the wrapper for the static code analyzer
// It may handle NumeRe and MATLAB files
void NumeReEditor::AnalyseCode()
{
    // Clear all annotations
	this->AnnotationClearAll();

	// Clear the corresponding indicators
    this->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
	this->IndicatorClearRange(0, GetLastPosition());
	this->IndicatorSetStyle(HIGHLIGHT_ANNOTATION, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetForeground(HIGHLIGHT_ANNOTATION, wxColor(0, 0, 255));

	// Ensure that the correct file type is used and that the setting is active
	if (!getEditorSetting(SETTING_USEANALYZER) || (m_fileType != FILE_NSCR && m_fileType != FILE_NPRC && m_fileType != FILE_MATLAB && m_fileType != FILE_CPP))
		return;

    // Determine the annotation style
	this->AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

	int currentLine = 0;
	bool isContinuedLine = false;
	bool hasProcedureDefinition = false;
	bool isAlreadyMeasured = false;
	bool isSuppressed = false;
	string sCurrentLine = "";
	string sStyles = "";
	string sFirstLine = "";
	string sFirstStyles = "";
	int nFirstLine = 0;
	string sNote = _guilang.get("GUI_ANALYZER_NOTE");
	string sWarn = _guilang.get("GUI_ANALYZER_WARN");
	string sError = _guilang.get("GUI_ANALYZER_ERROR");
	AnnotationCount AnnotCount;

	// Some code metric constants
	const double MINCOMMENTDENSITY = 0.5;
	const double MAXCOMMENTDENSITY = 1.5;
	const int MAXCOMPLEXITYNOTIFY = 15;
	const int MAXCOMPLEXITYWARN = 20;
	const int MAXLINESOFCODE = 100;

	// Go through the whole file
	for (int i = 0; i < this->GetLastPosition(); i++)
	{
	    // Ignore comments
		if (isStyleType(STYLE_COMMENT_BLOCK, i) || isStyleType(STYLE_COMMENT_LINE, i))
			continue;

		// It's a new line?
		if (currentLine < this->LineFromPosition(i))
		{
		    // Get the line's contents
			string sLine = this->GetLine(currentLine).ToStdString();
			StripSpaces(sLine);

            // catch constant expressions
			if (sLine.length() && sLine.find_first_not_of("\n\r\t") != string::npos && sLine.find_first_not_of("0123456789+-*/.,^(){} \t\r\n") == string::npos)
				AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sLine.substr(0, sLine.find_last_not_of("0123456789+-*/.,^()")), sWarn, _guilang.get("GUI_ANALYZER_CONSTEXPR")), ANNOTATION_WARN);

			// Handle line continuations
			if (sLine.find("\\\\") != string::npos)
				isContinuedLine = true;
			else
			{
				isContinuedLine = false;
				hasProcedureDefinition = false;
			}

			// If there are gathered annotations for the current line
			if (sCurrentLine.length())
			{
			    // Find the summary line for the current file
				if (!sFirstLine.length())
				{
					sFirstLine = sCurrentLine;
					sFirstStyles = sStyles;
					nFirstLine = currentLine;
				}
				else
				{
				    // Write the annotion to the current line
					this->AnnotationSetText(currentLine, sCurrentLine);
					this->AnnotationSetStyles(currentLine, sStyles);
				}
			}
			currentLine = this->LineFromPosition(i);

			// Get the new line for finding the trailing semicolon
			sLine = this->GetLine(currentLine).ToStdString();

			// Ensure that there's no trailing comment
			if (sLine.rfind("##") != string::npos)
                sLine.erase(sLine.rfind("##"));

            // Remove also block comments
            size_t nBlockStart = 0;
            while ((nBlockStart = sLine.find("#*")) != string::npos)
            {
                if (sLine.find("*#", nBlockStart+2) == string::npos)
                {
                    sLine.erase(nBlockStart);
                    break;
                }
                else
                {
                    sLine.erase(nBlockStart, sLine.find("*#", nBlockStart+2)+2 - nBlockStart);
                }
            }

			// Find the last visible character
			size_t lastVisibleChar = sLine.find_last_not_of(" \t\r\n");

			// Determine, whether it is a semicolon or a line continuation
			if (lastVisibleChar != string::npos
                && (sLine[lastVisibleChar] == ';' || (lastVisibleChar > 0 && sLine.substr(lastVisibleChar-1, 2) == "\\\\")))
                isSuppressed = true;
            else
                isSuppressed = false;

			sCurrentLine = "";
			sStyles = "";
		}

		// Get code metrics for scripts if not already done
		if (m_fileType == FILE_NSCR && !isAlreadyMeasured)
		{
			string sLine = this->GetLine(currentLine).ToStdString();
			StripSpaces(sLine);
			if (sLine.length() && sLine.find_first_not_of(" \n\r\t") != string::npos)
			{
				string sSyntaxElement =  GetFilenameString().ToStdString();
				isAlreadyMeasured = true;

				// Calculate the code metrics:
				// Complexity
				int nCyclomaticComplexity = calculateCyclomaticComplexity(currentLine, this->LineFromPosition(this->GetLastPosition()));

				// LinesOfcode
				int nLinesOfCode = calculateLinesOfCode(currentLine, this->LineFromPosition(this->GetLastPosition()));

				// Number of comments
				int nNumberOfComments = countNumberOfComments(currentLine, this->LineFromPosition(this->GetLastPosition()));

				// comment density
				double dCommentDensity = (double)nNumberOfComments / (double)nLinesOfCode;

				// Compare the metrics with the contants and issue a note or a warning
				if (nCyclomaticComplexity > MAXCOMPLEXITYWARN)
					AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sWarn, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_WARN);
				else if (nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY)
					AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_NOTE);
				if (nLinesOfCode > MAXLINESOFCODE)
					AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_MANYLINES", toString(nLinesOfCode))), ANNOTATION_NOTE);
				if (dCommentDensity < MINCOMMENTDENSITY)
					AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_LOWCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);
				if (dCommentDensity > MAXCOMMENTDENSITY)
					AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_HIGHCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);
			}
		}

		// Handle the different style types
		if (isStyleType(STYLE_COMMAND, i))
		{
		    if (!isSuppressed)
            {
                string sWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true)).ToStdString();

                // the commands "matop" or "mtrxop" are needing semicolons
                // and are the first element in a command line
                if (sWord != "matop" && sWord != "mtrxop")
                    isSuppressed = true;
            }
		    // Handle commands
		    AnnotCount += analyseCommands(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError);
		}
		else if (isStyleType(STYLE_FUNCTION, i)
           || ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC) && this->GetStyleAt(i) == wxSTC_NSCR_METHOD))
		{
		    if (!isSuppressed)
            {
                string sWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true)).ToStdString();
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sWord + "()", WordStartPosition(i, true), sWord.length()), sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle standard functions
		    AnnotCount += analyseFunctions(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError, isContinuedLine);
		}
		else if (isStyleType(STYLE_PROCEDURE, i))
		{
		    // Handle NumeRe procedure calls (NumeRe only)
		    AnnotCount += analyseProcedures(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError);
		}
		else if (isStyleType(STYLE_IDENTIFIER, i)
				 && this->GetCharAt(i) != ' '
				 && this->GetCharAt(i) != '\t'
				 && this->GetCharAt(i) != '\r'
				 && this->GetCharAt(i) != '\n')
		{
		    if (!isSuppressed)
            {
                string sWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true)).ToStdString();
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sWord, WordStartPosition(i, true), sWord.length()), sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle identifiers (like variable names)
		    AnnotCount += analyseIdentifiers(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError);
		}
		else if (isStyleType(STYLE_OPERATOR, i))
		{
		    if (this->GetCharAt(i) == '=' && !isSuppressed)
            {
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer("=", i, 1), sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle special operators
		    AnnotCount += analyseOperators(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError);
		}
		else if (isStyleType(STYLE_NUMBER, i))
		{
		    // Handle numbers
		    AnnotCount += analyseNumbers(i, currentLine, hasProcedureDefinition, sCurrentLine, sStyles, sNote, sWarn, sError);
		}
	}

	// Clear the annotation and style cache
	sCurrentLine.clear();
	sStyles.clear();

	// Write the summary lines
	if (AnnotCount.nNotes)
		addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_NOTE_TOTAL", toString(AnnotCount.nNotes)), ANNOTATION_NOTE);
	if (AnnotCount.nWarnings)
		addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_WARN_TOTAL", toString(AnnotCount.nWarnings)), ANNOTATION_WARN);
	if (AnnotCount.nErrors)
		addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_ERROR_TOTAL", toString(AnnotCount.nErrors)), ANNOTATION_ERROR);

    // Append the summary line to the first line (if it is not empty)
	if (sCurrentLine.length())
	{
		sCurrentLine += "\n" + sFirstLine;
		sStyles += sStyles.back() + sFirstStyles;
	}
	else
	{
		sCurrentLine = sFirstLine;
		sStyles = sFirstStyles;
	}

	// Write the first line if it is not empty
	if (sCurrentLine.length())
	{
		this->AnnotationSetText(nFirstLine, sCurrentLine);
		this->AnnotationSetStyles(nFirstLine, sStyles);
	}
}


// This member function analyses syntax elements, which are highlighted as commands
AnnotationCount NumeReEditor::analyseCommands(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError)
{
    AnnotationCount AnnotCount;

    bool canContinue = false;
    int wordstart = this->WordStartPosition(nCurPos, true);
    int wordend = this->WordEndPosition(nCurPos, true);

    // Some code metric constants
	const double MINCOMMENTDENSITY = 0.5;
	const double MAXCOMMENTDENSITY = 1.5;
	const int MAXCOMPLEXITYNOTIFY = 15;
	const int MAXCOMPLEXITYWARN = 20;
	const int MAXLINESOFCODE = 100;

	// Get the current syntax element
    string sSyntaxElement = this->GetTextRange(wordstart, wordend).ToStdString();

    // add a message to "throw"
    if (sSyntaxElement == "throw")
    {
        for (int j = wordend; j < this->GetLineEndPosition(currentLine); j++)
        {
            if (this->GetStyleAt(j) == wxSTC_NSCR_STRING || this->GetStyleAt(j) == wxSTC_NSCR_STRING_PARSER)
            {
                canContinue = true;
                break;
            }
        }

        // Was a message found?
        if (!canContinue)
        {
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, 5), sNote, _guilang.get("GUI_ANALYZER_THROW_ADDMESSAGE")), ANNOTATION_NOTE);
        }
    }

    // check the namespace command
    if (sSyntaxElement == "namespace")
    {
        string sArgs = this->GetTextRange(wordend, this->GetLineEndPosition(currentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Is there an explicit namespace name? If no, warn the user
        if (!sArgs.length())
        {
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sWarn, _guilang.get("GUI_ANALYZER_NAMESPACE_ALWAYSMAIN")), ANNOTATION_WARN);
        }
        nCurPos = wordend;

        // Advance the character pointer and return the number of gathered annotations
        while (this->GetCharAt(nCurPos) != ';' && this->GetCharAt(nCurPos) != '\r' && this->GetCharAt(nCurPos) != '\n')
            nCurPos++;
        return AnnotCount;
    }

    // The progress command needs extra runtime (2-4 times). Inform the user about this issue
    if (sSyntaxElement == "progress")
    {
        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_PROGRESS_RUNTIME")), ANNOTATION_NOTE);
    }

    // The install or the start commands are not allowed in scripts and procedures
    if (sSyntaxElement == "install" || sSyntaxElement == "uninstall" || sSyntaxElement == "start")
    {
        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_NOTALLOWED")), ANNOTATION_ERROR);
    }

    // Handle the memory clearance commands
    if (sSyntaxElement == "clear" || sSyntaxElement == "delete" || sSyntaxElement == "remove")
    {
        // some caches may not be removec
        if (sSyntaxElement == "remove" && this->GetStyleAt(this->WordStartPosition(wordend + 1, true)) == wxSTC_NSCR_PREDEFS)
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_CANNOTREMOVEPREDEFS")), ANNOTATION_ERROR);

        // Get everything after the clearance command
        string sArgs = this->GetTextRange(wordend, this->GetLineEndPosition(currentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();

        // Inform the user that he should append "ignore" as parameter
        if (!matchParams(sArgs, "ignore")
                && !matchParams(sArgs, "i")
                && (sSyntaxElement != "remove" || this->GetStyleAt(this->WordStartPosition(wordend + 1, true)) != wxSTC_NSCR_CUSTOM_FUNCTION))
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_APPENDIGNORE")), ANNOTATION_NOTE);
    }

    // check, whether the current command do have an expression
    // Ignore some special commands, which do not need an expression
    if (sSyntaxElement != "hline"
            && sSyntaxElement != "continue"
            && sSyntaxElement != "break"
            && sSyntaxElement != "else"
            && sSyntaxElement != "end"
            && sSyntaxElement != "endif"
            && sSyntaxElement != "endfor"
            && sSyntaxElement != "endwhile"
            && sSyntaxElement != "endprocedure"
            && sSyntaxElement != "endcompose"
            && sSyntaxElement != "about"
            && sSyntaxElement != "abort"
            && sSyntaxElement != "compose"
            && sSyntaxElement != "help"
            && sSyntaxElement != "quit"
            && sSyntaxElement != "return"
            && sSyntaxElement != "subplot"
            && sSyntaxElement != "try"
            && sSyntaxElement != "catch"
            && sSyntaxElement != "otherwise"
            && sSyntaxElement != "throw"
            && sSyntaxElement != "namespace" //warning
       )
    {
        canContinue = false;

        // Get everything in the current line after the command
        string sArgs = this->GetTextRange(wordend, this->GetLineEndPosition(currentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // If the line after the command is empty
        if (!sArgs.length())
        {
            // is used as a parameter (legacy)
            for (int j = wordstart; j >= PositionFromLine(currentLine); j--)
            {
                if (GetCharAt(j) == '-')
                    canContinue = true;
            }

            // No expression found and not used as a parameter?
            if (!canContinue)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_EMPTYEXPRESSION")), ANNOTATION_ERROR);
        }
    }

    // There are some command, which will return values.
    // Check, whether there results are stored into a variable
    if (sSyntaxElement == "zeroes"
            || sSyntaxElement == "extrema"
            || sSyntaxElement == "integrate"
            || sSyntaxElement == "eval"
            || sSyntaxElement == "get"
            || sSyntaxElement == "read"
            || sSyntaxElement == "pulse"
            || sSyntaxElement == "diff")
    {
        canContinue = false;

        // Try to find an assignment operator
        for (int j = PositionFromLine(currentLine); j < wordstart; j++)
        {
            if (GetCharAt(j) == '=')
            {
                canContinue = true;
                break;
            }
        }

        // Was an assignment operator found?
        if (!canContinue)
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE")), ANNOTATION_WARN);
    }

    // Examine the if, elseif and while commands
    if (sSyntaxElement == "if" || sSyntaxElement == "elseif" || sSyntaxElement == "while")
    {
        for (int j = wordend; j < this->GetLineEndPosition(currentLine); j++)
        {
            // Only examine elements in the first parenthesis for NumeRe syntax.
            // Ignore the parenthesis in the MATLAB case
            if (this->GetCharAt(j) == '(' || m_fileType == FILE_MATLAB)
            {
                int nPos = this->BraceMatch(j);

                // If the first character is a parenthesis
                if (this->GetCharAt(j) == '(')
                {
                    nPos = this->BraceMatch(j);
                    if (nPos < 0)
                    {
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j, 1), sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
                        break;
                    }
                }
                else
                    nPos = this->GetLineEndPosition(currentLine);

                // Get the argument
                string sArgument = this->GetTextRange(j + 1, nPos).ToStdString();
                StripSpaces(sArgument);

                // Is the argument available?
                if (!sArgument.length())
                {
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j, 2), sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
                    break;
                }

                // Is it a constant?
                if (sArgument == "true" || (sArgument.find_first_not_of("1234567890.") == string::npos && sArgument != "0"))
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sWarn, _guilang.get("GUI_ANALYZER_WHILE_ALWAYSTRUE")), ANNOTATION_WARN);
                    else
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sWarn, _guilang.get("GUI_ANALYZER_IF_ALWAYSTRUE")), ANNOTATION_WARN);
                }
                else if (sArgument == "false" || sArgument == "0")
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sWarn, _guilang.get("GUI_ANALYZER_WHILE_ALWAYSFALSE")), ANNOTATION_WARN);
                    else
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sWarn, _guilang.get("GUI_ANALYZER_IF_ALWAYSFALSE")), ANNOTATION_WARN);
                }
                else if (containsAssignment(sArgument))
                {
                    // Does it contain an assignment? Warn the user as this is probably not intendet
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sWarn, _guilang.get("GUI_ANALYZER_ASSIGNMENTINARGUMENT")), ANNOTATION_WARN);
                }
                break;
            }
        }

        // There's an faster, inline if-else operator in NumeRe
        // Propose that, if the current if-else block is quite short
        if (sSyntaxElement == "if" && m_fileType != FILE_MATLAB)
        {
            vector<int> vBlock = BlockMatch(nCurPos);

            // Was the end of the current block found?
            if (vBlock.back() != wxSTC_INVALID_POSITION)
            {
                // Check the length of the current block
                if (this->LineFromPosition(vBlock.back()) - currentLine < 5)
                {
                    canContinue = false;

                    // Ensure that no commands except of further ifs and elses are used inside of the found block
                    for (int pos = wordend; pos <= vBlock.back(); pos++)
                    {
                        if (this->GetStyleAt(pos) == wxSTC_NSCR_COMMAND
                                && this->GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true)) != "if"
                                && this->GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true)) != "else"
                                && this->GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true)) != "endif")
                        {
                            // Other command found
                            canContinue = true;
                            break;
                        }
                        else
                        {
                            // jump over other elements
                            pos = WordEndPosition(pos, true);
                        }
                    }

                    // Was an other command found?
                    if (!canContinue)
                    {
                        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_USEINLINEIF")), ANNOTATION_NOTE);
                    }
                }
            }
        }
    }

    // Examine the for command
    // Only used for NumeRe syntax
    if (sSyntaxElement == "for" && m_fileType != FILE_MATLAB)
    {
        // Go through the current line
        for (int j = wordend; j < this->GetLineEndPosition(currentLine); j++)
        {
            // If the current character is an opening parenthesis
            if (this->GetCharAt(j) == '(')
            {
                // Examine the argument
                int nPos = this->BraceMatch(j);
                if (nPos < 0)
                {
                    // Missing parenthesis
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j, 1), sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
                    break;
                }

                // Get the argument from the parenthesis
                string sArgument = this->GetTextRange(j + 1, nPos).ToStdString();
                StripSpaces(sArgument);

                // Argument is empty?
                if (!sArgument.length())
                {
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j, 2), sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
                    break;
                }

                // Important parts of the argument are missing?
                if (sArgument.find(':') == string::npos || sArgument.find('=') == string::npos)
                {
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, j+1, sArgument.length()), sError, _guilang.get("GUI_ANALYZER_FOR_INTERVALERROR")), ANNOTATION_ERROR);
                }
                break;
            }
        }
    }

    // Examine the current usage of the local variable declarators
    // Esp. ensure that the declared variables are used
    if (m_fileType == FILE_NPRC && (sSyntaxElement == "var" || sSyntaxElement == "str" || sSyntaxElement == "tab"))
    {
        // Handle the special case "list -var"
        if (sSyntaxElement == "var" && this->GetTextRange(this->PositionFromLine(currentLine), this->GetLineEndPosition(currentLine)).find("list") < (size_t)(wordstart - this->PositionFromLine(currentLine)))
        {
            nCurPos = wordend;
            return AnnotCount;
        }

        // Get the next line
        int nNextLine = this->GetLineEndPosition(currentLine) + 1;

        // Find the end of the current procedure
        int nProcedureEnd = this->FindText(nNextLine, this->GetLastPosition(), "endprocedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        // extract the arguments and strip the spaces
        string sArgs = this->GetTextRange(wordend, this->GetLineEndPosition(currentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Ensure that the end of the procedure is available
        if (nProcedureEnd == -1)
        {
            nProcedureEnd = this->GetLastPosition();
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_MISSINGENDPROCEDURE")), ANNOTATION_ERROR);
        }

        // If there are variables available
        if (sArgs.length())
        {
            string currentArg = "";

            // Extract variable by variable
            while (getNextArgument(sArgs, false).length())
            {
                currentArg = getNextArgument(sArgs, true);

                // remove assignments and parentheses and strip the spaces
                if (currentArg.find('=') != string::npos)
                    currentArg.erase(currentArg.find('='));
                if (currentArg.find('(') != string::npos)
                    currentArg.erase(currentArg.find('('));
                StripSpaces(currentArg);

                // Try to find the variable in the remaining code
                if (this->FindText(nNextLine, nProcedureEnd, currentArg, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) == -1)
                {
                    // No variable found
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, this->FindText(wordstart, nProcedureEnd, currentArg, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD), currentArg.length()), sWarn, _guilang.get("GUI_ANALYZER_UNUSEDVARIABLE", currentArg)), ANNOTATION_WARN);
                }
            }
        }
        else // No varibles are available
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_NOVARIABLES")), ANNOTATION_ERROR);
    }

    // Examine the procedure / MATLAB function starting at this position
    // This includes esp. the calculation of the standard coding metrics
    if ((m_fileType == FILE_NPRC && sSyntaxElement == "procedure") || (m_fileType == FILE_MATLAB && sSyntaxElement == "function"))
    {
        // Use the block match function, which is capable of doing both: NumeRe and MATLAB syntax
        vector<int> vBlock = BlockMatch(nCurPos);
        int nProcedureEnd = 0;
        hasProcedureDefinition = true;
        if (vBlock.back() == wxSTC_INVALID_POSITION)
        {
            nProcedureEnd = this->GetLastPosition();
        }
        else
        {
            nProcedureEnd = vBlock.back();

            // This is only needed for NumeRe procedures
            if (m_fileType == FILE_NPRC)
            {
                // check the name of the procedure - is there a naming procedure?
                int nNamingProcedure = FindNamingProcedure();
                if (nNamingProcedure == wxNOT_FOUND)
                {
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sError, _guilang.get("GUI_ANALYZER_NONAMINGPROCEDURE")), ANNOTATION_ERROR);
                }
                else if (nNamingProcedure != currentLine)
                {
                    AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sWarn, _guilang.get("GUI_ANALYZER_THISFILEPROCEDURE")), ANNOTATION_WARN);
                }
            }
            // Calculate the code metrics:
            // Complexity
            int nCyclomaticComplexity = calculateCyclomaticComplexity(currentLine, LineFromPosition(nProcedureEnd));

            // LinesOfCode
            int nLinesOfCode = calculateLinesOfCode(currentLine, LineFromPosition(nProcedureEnd)) - 2;

            // Number of comments
            int nNumberOfComments = countNumberOfComments(currentLine, LineFromPosition(nProcedureEnd));

            // Comment density
            double dCommentDensity = (double)nNumberOfComments / (double)nLinesOfCode;

            // Compare the metrics with the contants and issue a note or a warning
            if (nLinesOfCode < 3)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sWarn, _guilang.get("GUI_ANALYZER_INLINING")), ANNOTATION_WARN);
            if (nCyclomaticComplexity > MAXCOMPLEXITYWARN)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sWarn, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_WARN);
            else if (nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_NOTE);
            if (nLinesOfCode > MAXLINESOFCODE)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_MANYLINES", toString(nLinesOfCode))), ANNOTATION_NOTE);
            if (dCommentDensity < MINCOMMENTDENSITY)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_LOWCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);
            if (dCommentDensity > MAXCOMMENTDENSITY)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_HIGHCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);

        }
    }

    // Handle the "return" command in procedures (not needed in MATLAB)
    if (m_fileType == FILE_NPRC && sSyntaxElement == "return")
    {
        // Try to find the end of the current procedure
        int nProcedureEnd = this->FindText(nCurPos, this->GetLastPosition(), "endprocedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        // Get the argument of the return command and strip the spaces
        string sArgs = this->GetTextRange(wordend, this->GetLineEndPosition(currentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Ensure that the end of the procedure was found
        if (nProcedureEnd == -1)
        {
            nProcedureEnd = this->GetLastPosition();
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_MISSINGENDPROCEDURE")), ANNOTATION_ERROR);
        }

        // Examine the argument
        if (sArgs.length())
        {
            // Inform the user to add an semicolon to the arguments, if he uses something else than "void"
            if (sArgs.back() != ';' && sArgs != "void")
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordend + 1, sArgs.length()), sNote, _guilang.get("GUI_ANALYZER_RETURN_ADDSEMICOLON")), ANNOTATION_NOTE);
        }
        else
        {
            // Inform the user that the return value will always be "true", if he doesn't append a value
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_RETURN_ALWAYSTRUE")), ANNOTATION_NOTE);
        }
    }

    // Handle blocks with their corresponding end
    if (sSyntaxElement == "if"
            || sSyntaxElement == "end"
            || sSyntaxElement == "elseif"
            || sSyntaxElement == "else"
            || sSyntaxElement == "endif"
            || sSyntaxElement == "for"
            || sSyntaxElement == "endfor"
            || sSyntaxElement == "while"
            || sSyntaxElement == "endwhile"
            || sSyntaxElement == "compose"
            || sSyntaxElement == "endcompose"
            || sSyntaxElement == "function"
            || sSyntaxElement == "procedure"
            || sSyntaxElement == "endprocedure")
    {
        // Try to find the matching block parts
        vector<int> vMatch = this->BlockMatch(nCurPos);
        if (vMatch.size() > 1)
        {
            // If there's an invalid position, this means that the current block is unfinished
            if (vMatch.front() == wxSTC_INVALID_POSITION || vMatch.back() == wxSTC_INVALID_POSITION)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sError, _guilang.get("GUI_ANALYZER_UNFINISHEDBLOCK")), ANNOTATION_ERROR);
        }
    }
    nCurPos = wordend;

    // Return the counted annotations
    return AnnotCount;
}

// This member function analyses syntax elements, which are highlighted as functions
AnnotationCount NumeReEditor::analyseFunctions(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError, bool isContinuedLine)
{
    AnnotationCount AnnotCount;

    bool canContinue = false;
    int wordstart = this->WordStartPosition(nCurPos, true);
    int wordend = this->WordEndPosition(nCurPos, true);

    // Get the corresponding syntax element
    string sSyntaxElement = this->GetTextRange(wordstart, wordend).ToStdString();

    // Handle method (modifier) calls, also appends a pair of parentheses if needed
    if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC) && this->GetStyleAt(nCurPos) == wxSTC_NSCR_METHOD)
    {
        // ignore modifiers, i.e. method without parentheses
        string sModifier = ",len,cols,lines,grid,avg,std,min,max,med,sum,prd,cnt,num,norm,and,or,xor,";
        if (sModifier.find("," + sSyntaxElement + ",") == string::npos)
            sSyntaxElement += "()";
        sSyntaxElement.insert(0, "VAR.");
    }
    else
        sSyntaxElement += "()";

    // Is the current function called without a target variable?
    if (this->PositionFromLine(currentLine) == wordstart && !isContinuedLine)
    {
        // The function is called at the first position without a target variable
        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, wordend-wordstart), sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE")), ANNOTATION_WARN);
    }
    else
    {
        // Try to find a assignment operator before the function
        // Other possibilities are commands and procedure calls
        for (int j = PositionFromLine(currentLine); j < wordstart; j++)
        {
            if (GetCharAt(j) == '=' || isStyleType(STYLE_COMMAND, j) || isStyleType(STYLE_PROCEDURE, j))
            {
                canContinue = true;
                break;
            }
        }

        // Was an operator or a command found?
        if (!canContinue && !isContinuedLine)
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, wordend-wordstart), sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE")), ANNOTATION_WARN);
    }

    // There's a missing parenthesis?
    if (this->BraceMatch(wordend) < 0 && sSyntaxElement.find('(') != string::npos)
    {
        // MATLAB doesn't require a parenthesis pair for empty arguments.
        // However, issue a warning as it is good practice to visually distinguish between variables and functions
        if (m_fileType == FILE_MATLAB)
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, wordend-wordstart), sWarn, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_WARN);
        else
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, wordend-wordstart), sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
    }
    else if (sSyntaxElement != "time()" && sSyntaxElement != "clock()" && sSyntaxElement != "version()" && sSyntaxElement.find('(') != string::npos)
    {
        // Check for missing arguments
        int nPos = this->BraceMatch(wordend);
        string sArgument = this->GetTextRange(wordend + 1, nPos).ToStdString();
        StripSpaces(sArgument);
        if (!sArgument.length())
        {
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordend, 2), sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
        }
    }
    nCurPos = wordend;

    // return the counted number of gathered annotations
    return AnnotCount;
}

// This member function analyses syntax elements, which are highlighted as procedure calls
AnnotationCount NumeReEditor::analyseProcedures(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError)
{
    AnnotationCount AnnotCount;

    int nProcStart = nCurPos;

    // Try to find the current procedure call
    string sSyntaxElement = FindMarkedProcedure(nCurPos).ToStdString();
    if (!sSyntaxElement.length())
        return AnnotCount;

    // Advance the character pointer until the style type changes
    while (isStyleType(STYLE_PROCEDURE, nCurPos + 1))
        nCurPos++;

    // Try to find the correspondung procedure definition
    if (!FindProcedureDefinition().length())
    {
        // Procedure definition was not found
        AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, nProcStart, nCurPos-nProcStart+1), sError, _guilang.get("GUI_ANALYZER_PROCEDURENOTFOUND")), ANNOTATION_ERROR);
    }

    // return the number of gathered annotations
    return AnnotCount;
}

// This member function analyses syntax elements, which are highlighted as identifiers (aka variable names)
AnnotationCount NumeReEditor::analyseIdentifiers(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError)
{
    AnnotationCount AnnotCount;

    int wordstart = this->WordStartPosition(nCurPos, true);
    int wordend = this->WordEndPosition(nCurPos, true);

    // Shift the word end position, if the following character is a dot
    if (this->GetCharAt(wordend) == '.' && this->GetStyleAt(wordend + 1) != wxSTC_NSCR_METHOD)
        wordend = this->WordEndPosition(wordend + 1, true);

    // Get the corresponding syntax element
    string sSyntaxElement = this->GetTextRange(wordstart, wordend).ToStdString();

    // Handle very short variable names
    if (sSyntaxElement.length() < 4 && sSyntaxElement.length() > 1 && sSyntaxElement.find_first_not_of("\r\n") != string::npos && sSyntaxElement.find('.') == string::npos)
    {
        // Too short
        if (!(sSyntaxElement.length() == 2 && ((sSyntaxElement[1] >= '0' && sSyntaxElement[1] <= '9') || sSyntaxElement[0] == 'd')))
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, sNote, _guilang.get("GUI_ANALYZER_VARNAMETOOSHORT")), ANNOTATION_NOTE);
    }

    // Handle the variable's names: are they following guidelines?
    if (sSyntaxElement.length() > 2 && sSyntaxElement.find_first_not_of("\r\n") != string::npos && sSyntaxElement.find('.') == string::npos)
    {
        size_t shift = 0;

        // We want to start the procedures arguments with an underscore (not possible in MATLAB)
        if (sSyntaxElement[0] == '_' && m_fileType == FILE_NPRC)
            shift++;

        // Because function names definitions are not highlighted different in MATLAB code, we leave the function
        // at this position
        if (m_fileType == FILE_MATLAB && hasProcedureDefinition && this->GetCharAt(wordend) == '(')
        {
            nCurPos = wordend;
            return AnnotCount;
        }
        // numerical/int string float standard vars (x,y,z,t)
        string sFirstChars = "nsfbxyzt";

        if (sFirstChars.find(sSyntaxElement[shift]) == string::npos
                || ((sSyntaxElement[shift + 1] < 'A' || sSyntaxElement[shift + 1] > 'Z') && sSyntaxElement[shift + 1] != '_'))
        {
            // var not type-oriented
            // Add and underscore to indicate the procedures arguments
            if (hasProcedureDefinition && !shift && m_fileType != FILE_MATLAB)
                AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_INDICATEARGUMENT")), ANNOTATION_NOTE);

            // variable should begin with lowercase letter indicate its type
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_VARNOTTYPEORIENTED")), ANNOTATION_NOTE);
        }
        else if (hasProcedureDefinition && !shift && m_fileType != FILE_MATLAB)
        {
            // Add and underscore to indicate the procedures arguments
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sSyntaxElement, wordstart, sSyntaxElement.length()), sNote, _guilang.get("GUI_ANALYZER_INDICATEARGUMENT")), ANNOTATION_NOTE);
        }
    }
    nCurPos = wordend;

    // Return the gathered number of annotations
    return AnnotCount;
}

// This member function analyses syntax elements, which are highlighted as operators
AnnotationCount NumeReEditor::analyseOperators(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError)
{
    AnnotationCount AnnotCount;

    // If the current operator is a parenthesis, try to find the matching one
    if (this->GetCharAt(nCurPos) == '(' || this->GetCharAt(nCurPos) == '[' || this->GetCharAt(nCurPos) == '{'
            || this->GetCharAt(nCurPos) == ')' || this->GetCharAt(nCurPos) == ']' || this->GetCharAt(nCurPos) == '}')
    {
        int nPos = this->BraceMatch(nCurPos);
        if (nPos < 0)
        {
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(string(1, this->GetCharAt(nCurPos)), nCurPos, 1), sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
        }
    }

    return AnnotCount;
}

// This member function analyses syntax elements, which are highlighted as numbers
AnnotationCount NumeReEditor::analyseNumbers(int& nCurPos, int currentLine, bool& hasProcedureDefinition, string& sCurrentLine, string& sStyles, const string& sNote, const string& sWarn, const string& sError)
{
    AnnotationCount AnnotCount;
    int nNumberStart = nCurPos;
    int nLineStartPos = this->PositionFromLine(currentLine);

    // Advance until the style of the next character is not a number any more
    while (isStyleType(STYLE_NUMBER, nCurPos + 1))
        nCurPos++;

    // Get the number
    string sCurrentNumber = this->GetTextRange(nNumberStart, nCurPos + 1).ToStdString();

    // Go inversely through the line and try to find an assignment operator
    for (int i = nNumberStart; i >= nLineStartPos; i--)
    {
        // If the current character is a operator and the previous one is not
        if (isStyleType(STYLE_OPERATOR, i))
        {
            // Is an assignment -> no magic number
            if (this->GetCharAt(i) == '='
                && this->GetCharAt(i - 1) != '<'
                && this->GetCharAt(i - 1) != '>'
                && this->GetCharAt(i - 1) != '!'
                && this->GetCharAt(i - 1) != '~'
                && this->GetCharAt(i - 1) != '=')
                break;

            // All other operators are indicating the current number as magic number
            AnnotCount += addToAnnotation(sCurrentLine, sStyles, _guilang.get("GUI_ANALYZER_TEMPLATE", constructSyntaxElementForAnalyzer(sCurrentNumber, nNumberStart, sCurrentNumber.length()), sWarn, _guilang.get("GUI_ANALYZER_MAGICNUMBER")), ANNOTATION_WARN);
            break;
        }
    }

    // Return the number of gathered annotations
    return AnnotCount;
}


void NumeReEditor::JumpToBookmark(bool down)
{
	int nCurrentLine = this->GetCurrentLine();
	if (this->MarkerOnLine(nCurrentLine, MARKER_BOOKMARK) || this->MarkerOnLine(nCurrentLine, MARKER_SECTION))
	{
		if (down)
			nCurrentLine++;
		else
			nCurrentLine--;
	}
	int nMarker = nCurrentLine;
	int nMarkerMask = (1 << MARKER_BOOKMARK) | (1 << MARKER_SECTION);
	if (down)
		nMarker = this->MarkerNext(nCurrentLine, nMarkerMask);
	else
		nMarker = this->MarkerPrevious(nCurrentLine, nMarkerMask);
	if (nMarker == -1)
	{
		if (down)
			nMarker = this->MarkerNext(0, nMarkerMask);
		else
			nMarker = this->MarkerPrevious(this->LineFromPosition(this->GetLastPosition()), nMarkerMask);
	}
	if (nMarker != -1)
		this->GotoLine(nMarker);
}

void NumeReEditor::removeWhiteSpaces(int nType)
{
	int nFirstline = 0;
	int nLastLine = GetLineCount() - 1;
	if (HasSelection())
	{
		nFirstline = LineFromPosition(GetSelectionStart());
		nLastLine = LineFromPosition(GetSelectionEnd());
	}
	this->BeginUndoAction();
	for (int i = nFirstline; i <= nLastLine; i++)
	{
		if (nType == RM_WS_FRONT)
			SetLineIndentation(i, 0);
		else if (nType == RM_WS_BACK || nType == RM_WS_BOTH)
		{
			wxString sLine = this->GetLine(i);
			if (sLine.find_first_of("\r\n") != string::npos)
				sLine.erase(sLine.find_first_of("\r\n"));
			int nLineEndPos = sLine.length();
			while (nLineEndPos && (sLine[nLineEndPos - 1] == ' ' || sLine[nLineEndPos - 1] == '\t'))
			{
				sLine.erase(nLineEndPos - 1);
				nLineEndPos--;
			}
			if (nType == RM_WS_BOTH)
			{
				while (sLine[0] == ' ' || sLine[0] == '\t')
					sLine.erase(0, 1);
			}
			Replace(this->PositionFromLine(i), this->GetLineEndPosition(i), sLine);
		}
	}
	this->EndUndoAction();
}

void NumeReEditor::toggleBookmark()
{
	int nLine = this->GetCurrentLine();
	if (MarkerOnLine(nLine, MARKER_BOOKMARK))
		this->MarkerDelete(nLine, MARKER_BOOKMARK);
	else
		this->MarkerAdd(nLine, MARKER_BOOKMARK);
}

void NumeReEditor::clearBookmarks()
{
	this->MarkerDeleteAll(MARKER_BOOKMARK);
}

void NumeReEditor::sortSelection(bool ascending)
{
	int nFirstline = 0;
	int nLastLine = GetLineCount() - 1;
	map<string, int> mSortMap;
	vector<wxString> vSortVector;
	string sCurrentLine;
	if (HasSelection())
	{
		nFirstline = LineFromPosition(GetSelectionStart());
		nLastLine = LineFromPosition(GetSelectionEnd());
	}
	this->BeginUndoAction();
	for (int i = nFirstline; i <= nLastLine; i++)
	{
		vSortVector.push_back(this->GetLine(i));
		if (vSortVector[i - nFirstline].find_first_of("\r\n") != string::npos)
			vSortVector[i - nFirstline].erase(vSortVector[i - nFirstline].find_first_of("\r\n"));
		sCurrentLine = toLowerCase(vSortVector[i - nFirstline].ToStdString());
		StripSpaces(sCurrentLine);
		if (!sCurrentLine.length())
			sCurrentLine = " " + toString(i + 256);
		if (mSortMap.find(sCurrentLine) != mSortMap.end())
			sCurrentLine += "\n" + toString(i + 256); // need a value smaller than space therefore the \n
		mSortMap[sCurrentLine] = i - nFirstline;
	}
	if (ascending)
	{
		for (auto iter = mSortMap.begin(); iter != mSortMap.end(); ++iter)
		{
			this->Replace(this->PositionFromLine(nFirstline), this->GetLineEndPosition(nFirstline), vSortVector[iter->second]);
			nFirstline++;
		}
	}
	else
	{
		for (auto iter = mSortMap.rbegin(); iter != mSortMap.rend(); ++iter)
		{
			this->Replace(this->PositionFromLine(nFirstline), this->GetLineEndPosition(nFirstline), vSortVector[iter->second]);
			nFirstline++;
		}
	}
	this->EndUndoAction();
}

string NumeReEditor::GetStrippedLine(int nLine)
{
	string sCurrentLine = this->GetLine(nLine).ToStdString();
	if (sCurrentLine.find_first_not_of(" \t\r\n") == string::npos)
		return "";
	else
		sCurrentLine.erase(0, sCurrentLine.find_first_not_of(" \t\r\n"));
	if (sCurrentLine.find_last_not_of(" \r\t\n") != string::npos)
		sCurrentLine.erase(sCurrentLine.find_last_not_of(" \r\t\n") + 1);
	return sCurrentLine;
}

string NumeReEditor::GetStrippedRange(int nPos1, int nPos2, bool encode)
{
	string sTextRange = this->GetTextRange(nPos1, nPos2).ToStdString();
	while (sTextRange.front() == ' ' || sTextRange.front() == '\r' || sTextRange.front() == '\n' )//|| sTextRange.front() == '\t')
		sTextRange.erase(0, 1);
	while (sTextRange.back() == ' ' || sTextRange.back() == '\t' || sTextRange.back() == '\r' || sTextRange.back() == '\n')
		sTextRange.erase(sTextRange.length() - 1);
	while (sTextRange.find("\r\n") != string::npos)
		sTextRange.replace(sTextRange.find("\r\n"), 2, "\n");
	if (encode)
	{
		for (size_t i = 0; i < sTextRange.length(); i++)
		{
			switch (sTextRange[i])
			{
				case 'Ä':
					sTextRange.replace(i, 1, "Ae");
					break;
				case 'ä':
					sTextRange.replace(i, 1, "ae");
					break;
				case 'Ö':
					sTextRange.replace(i, 1, "Oe");
					break;
				case 'ö':
					sTextRange.replace(i, 1, "oe");
					break;
				case 'Ü':
					sTextRange.replace(i, 1, "Ue");
					break;
				case 'ü':
					sTextRange.replace(i, 1, "ue");
					break;
				case 'ß':
					sTextRange.replace(i, 1, "ss");
					break;
			}
		}
	}
	if (sTextRange.find_first_not_of('\t') == string::npos)
		return "";
	return sTextRange;
}

bool NumeReEditor::writeLaTeXFile(const string& sLaTeXFileName)
{
	if (this->getFileType() != FILE_NSCR && this->getFileType() != FILE_NPRC)
		return false;

	string sFileContents;
	ofstream file_out;

	bool bTextMode = true;
	int startpos = 0;

	sFileContents += "% Created by NumeRe from the source of " + this->GetFileNameAndPath().ToStdString() + "\n\n";


	for (int i = 0; i < this->GetLastPosition(); i++)
	{
		if (this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_LINE && this->GetTextRange(i, i + 3) == "##!") // That's a documentation
		{
			if (!bTextMode)
			{
				if (i - startpos > 1)
				{
					sFileContents += this->GetStrippedRange(startpos, i) + "\n";
				}
				bTextMode = true;
				sFileContents += "\\end{lstlisting}\n";
			}
			sFileContents += this->parseDocumentation(i + 3, this->GetLineEndPosition(this->LineFromPosition(i))) + "\n";
			i = this->GetLineEndPosition(this->LineFromPosition(i)) + 1;
			startpos = i;
		}
		else if (this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_LINE && this->GetTextRange(i, i + 3) == "##~") // ignore that (escaped comment)
		{
			if (i - startpos > 1)
			{
				sFileContents += this->GetStrippedRange(startpos, i) + "\n";
			}
			i = this->GetLineEndPosition(this->LineFromPosition(i)) + 1;
			startpos = i;
		}
		else if (this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_BLOCK && this->GetTextRange(i, i + 3) == "#*!") // that's also a documentation
		{
			if (!bTextMode)
			{
				if (i - startpos > 1)
				{
					sFileContents += this->GetStrippedRange(startpos, i) + "\n";
				}
				bTextMode = true;
				sFileContents += "\\end{lstlisting}\n";
			}

			for (int j = i + 3; j < this->GetLastPosition(); j++)
			{
				if (this->GetStyleAt(j + 3) != wxSTC_NSCR_COMMENT_BLOCK || j + 1 == this->GetLastPosition())
				{
					sFileContents += this->parseDocumentation(i + 3, j) + "\n";
					i = j + 2;
					break;
				}
			}
			startpos = i;
		}
		else if (this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_BLOCK && this->GetTextRange(i, i + 3) == "#**") // ignore that, that's also an escaped comment
		{
			if (i - startpos > 1)
			{
				sFileContents += this->GetStrippedRange(startpos, i) + "\n";
			}

			for (int j = i + 3; j < this->GetLastPosition(); j++)
			{
				if (this->GetStyleAt(j + 3) != wxSTC_NSCR_COMMENT_BLOCK || j + 1 == this->GetLastPosition())
				{
					i = j + 2;
					break;
				}
			}
			startpos = i + 1;
		}
		else // a normal code fragment
		{
			if (bTextMode)
			{
				startpos = i;
				bTextMode = false;
				sFileContents += "\\begin{lstlisting}\n";
			}
			if (i + 1 == this->GetLastPosition())
			{
				if (i - startpos > 1)
				{
					sFileContents += this->GetStrippedRange(startpos, i) + "\n";
				}
			}
		}
	}
	if (!bTextMode)
		sFileContents += "\\end{lstlisting}\n";

	if (!sFileContents.length())
		return false;
	file_out.open(sLaTeXFileName.c_str());
	if (!file_out.good())
		return false;
	file_out << sFileContents;
	file_out.close();
	return true;
}

string NumeReEditor::parseDocumentation(int nPos1, int nPos2)
{
	string sTextRange = this->GetStrippedRange(nPos1, nPos2, false);
	if (sTextRange.find("\n- ") != string::npos) // thats a unordered list
	{
		while (sTextRange.find("\n- ") != string::npos)
		{
			size_t nItemizeStart = sTextRange.find("\n- ");
			for (size_t i = nItemizeStart; i < sTextRange.length(); i++)
			{
				if (sTextRange.substr(i, 3) == "\n- ")
				{
					sTextRange.replace(i + 1, 1, "\\item");
					continue;
				}
				if ((sTextRange[i] == '\n' && sTextRange.substr(i, 3) != "\n  ") || i + 1 == sTextRange.length())
				{
					if (sTextRange[i] == '\n')
						sTextRange.insert(i, "\\end{itemize}");
					else
						sTextRange += "\\end{itemize}";
					sTextRange.insert(nItemizeStart + 1, "\\begin{itemize}");
					break;
				}
			}
		}
	}
	for (size_t i = 0; i < sTextRange.length(); i++)
	{
		switch (sTextRange[i])
		{
			case 'Ä':
				sTextRange.replace(i, 1, "\\\"A");
				break;
			case 'ä':
				sTextRange.replace(i, 1, "\\\"a");
				break;
			case 'Ö':
				sTextRange.replace(i, 1, "\\\"O");
				break;
			case 'ö':
				sTextRange.replace(i, 1, "\\\"o");
				break;
			case 'Ü':
				sTextRange.replace(i, 1, "\\\"U");
				break;
			case 'ü':
				sTextRange.replace(i, 1, "\\\"u");
				break;
			case 'ß':
				sTextRange.replace(i, 1, "\\ss ");
				break;
		}
	}
	for (size_t i = 0; i < sTextRange.length(); i++)
	{
		if (sTextRange.substr(i, 2) == "!!")
		{
			for (size_t j = i + 2; j < sTextRange.length(); j++)
			{
				if (sTextRange.substr(j, 2) == "!!")
				{
					sTextRange.replace(j, 2, "`");
					sTextRange.replace(i, 2, "\\lstinline`");
					break;
				}
			}
		}
	}
	return sTextRange;
}

void NumeReEditor::notifyDialogClose()
{
	m_duplicateCode = nullptr;
}

void NumeReEditor::SetEditorFont(const wxFont& font)
{
    wxFont newFont = font;
    StyleSetFont(wxSTC_STYLE_DEFAULT, newFont);
    StyleClearAll();
    UpdateSyntaxHighlighting(true);
}

bool NumeReEditor::getEditorSetting(EditorSettings _setting)
{
	return m_nEditorSetting & _setting;
}

void NumeReEditor::ToggleSettings(int _setting)
{
	this->SetWhitespaceForeground(true, wxColor(170, 190, 210));
	this->SetWhitespaceSize(2);
	if (!(m_nEditorSetting & _setting))
	{
		m_nEditorSetting |= _setting;

		if (_setting & SETTING_WRAPEOL)
		{
			this->SetWrapMode(wxSTC_WRAP_WORD);
			this->SetWrapIndentMode(wxSTC_WRAPINDENT_INDENT);
			this->SetWrapStartIndent(1);
			this->SetWrapVisualFlags(wxSTC_WRAPVISUALFLAG_END);
			this->SetWrapVisualFlagsLocation(wxSTC_WRAPVISUALFLAGLOC_END_BY_TEXT);
		}
		if (_setting & SETTING_DISPCTRLCHARS)
		{
			this->SetViewWhiteSpace(wxSTC_WS_VISIBLEALWAYS);
			this->SetViewEOL(true);
		}
		if (_setting & SETTING_USESECTIONS)
			markSections(true);
	}
	else
	{
		m_nEditorSetting &= ~_setting;
		if (_setting == SETTING_WRAPEOL)
			this->SetWrapMode(wxSTC_WRAP_NONE);
		else if (_setting == SETTING_DISPCTRLCHARS)
		{
			this->SetViewEOL(false);
			this->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
		}
		else if (_setting == SETTING_USETXTADV)
		{
			this->SetIndicatorCurrent(HIGHLIGHT_STRIKETHROUGH);
			this->IndicatorClearRange(0, this->GetLastPosition());
		}
		else if (_setting == SETTING_USESECTIONS)
		{
			this->MarkerDeleteAll(MARKER_SECTION);
		}
	}
	UpdateSyntaxHighlighting();
	AnalyseCode();
}

void NumeReEditor::getMatchingBrace(int nPos)
{
	int nMatch = this->BraceMatch(nPos);
	if (nMatch == wxSTC_INVALID_POSITION)
		this->BraceBadLight(nPos);
	else
	{
		this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
		this->IndicatorClearRange(0, GetLastPosition());
		this->IndicatorSetStyle(HIGHLIGHT_MATCHING_BRACE, wxSTC_INDIC_DIAGONAL);
		//this->IndicatorSetForeground(HIGHLIGHT_MATCHING_BRACE, wxColour(0,220,0));
		this->IndicatorSetForeground(HIGHLIGHT_MATCHING_BRACE, wxColour(0, 150, 0));

		if (nMatch < nPos)
		{
			this->BraceHighlight(nMatch, nPos);
			this->IndicatorFillRange(nMatch + 1, nPos - nMatch - 1);
		}
		else
		{
			this->BraceHighlight(nPos, nMatch);
			this->IndicatorFillRange(nPos + 1, nMatch - nPos - 1);
		}
	}
}

void NumeReEditor::getMatchingBlock(int nPos)
{
	vector<int> vMatch = this->BlockMatch(nPos);
	if (vMatch.size() == 1 && vMatch[0] == wxSTC_INVALID_POSITION)
		return;

	if (vMatch.front() == wxSTC_INVALID_POSITION || vMatch.back() == wxSTC_INVALID_POSITION)
		this->SetIndicatorCurrent(HIGHLIGHT_NOT_MATCHING_BLOCK);
	else
		this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BLOCK);

	this->IndicatorClearRange(0, GetLastPosition());
	this->IndicatorSetStyle(HIGHLIGHT_MATCHING_BLOCK, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT_MATCHING_BLOCK, 100);
	this->IndicatorSetForeground(HIGHLIGHT_MATCHING_BLOCK, wxColour(0, 220, 0));
	this->IndicatorSetStyle(HIGHLIGHT_NOT_MATCHING_BLOCK, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT_NOT_MATCHING_BLOCK, 128);
	this->IndicatorSetForeground(HIGHLIGHT_NOT_MATCHING_BLOCK, wxColour(255, 0, 0));

	for (size_t i = 0; i < vMatch.size(); i++)
	{
		if (vMatch[i] == wxSTC_INVALID_POSITION)
			continue;
		this->IndicatorFillRange(vMatch[i], WordEndPosition(vMatch[i], true) - vMatch[i]);
	}
}

// returnes a vector. If first element is invalid, the word is no command.
// If the last one is invalid, there's no matching partner. It returnes more than two elements for "if" blocks
// If there's no first "if", if one currently focussing on an "else...", the first element may be invalid, but more can be returned.
vector<int> NumeReEditor::BlockMatch(int nPos)
{
	if (this->getFileType() == FILE_NSCR || this->getFileType() == FILE_NPRC)
		return BlockMatchNSCR(nPos);
	else if (this->getFileType() == FILE_MATLAB)
		return BlockMatchMATLAB(nPos);
	else
	{
		vector<int> vPos;
		vPos.push_back(wxSTC_INVALID_POSITION);
		return vPos;
	}
}

vector<int> NumeReEditor::BlockMatchNSCR(int nPos)
{
	int nFor = 0;
	int nIf = 0;
	int nWhile = 0;
	int nCompose = 0;
	int nProcedure = 0;
	int nStartPos = WordStartPosition(nPos, true);
	vector<int> vPos;
	wxString startblock;
	wxString endblock;
	bool bSearchForIf = false; //if we search for an if block element. If yes => also mark the "else..." parts.
	int nSearchDir = 1; //direction in which to search for the matching block partner
	if (this->GetStyleAt(nPos) != wxSTC_NSCR_COMMAND && this->GetStyleAt(nPos) != wxSTC_NPRC_COMMAND)
	{
		if (nPos && (this->GetStyleAt(nPos - 1) == wxSTC_NSCR_COMMAND || this->GetStyleAt(nPos - 1) == wxSTC_NPRC_COMMAND))
			nPos--;
		else
		{
			vPos.push_back(wxSTC_INVALID_POSITION);
			return vPos;
		}
	}


	startblock = this->GetTextRange(WordStartPosition(nPos, true), WordEndPosition(nPos, true));
	if (startblock.substr(0, 3) == "end")
	{
		endblock = startblock.substr(3);
		nSearchDir = -1;
	}
	else if (startblock == "else" || startblock == "elseif")
	{
		// search for starting "if"
		// adding 1 to nIf, because we're already inside of an "if"
		nIf++;
		for (int i = WordEndPosition(nPos, true); i >= 0; i--)
		{
			if (this->GetStyleAt(i) == wxSTC_NSCR_COMMAND || this->GetStyleAt(i) == wxSTC_NPRC_COMMAND)
			{
				wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
				if (currentWord == "for")
					nFor--; //if we iterate upwards, the closing blocks shall increment and the opening blocks decrement the counter
				else if (currentWord == "endfor")
					nFor++;
				else if (currentWord == "while")
					nWhile--;
				else if (currentWord == "endwhile")
					nWhile++;
				else if (currentWord == "if")
					nIf--;
				else if (currentWord == "endif")
					nIf++;
				else if (currentWord == "compose")
					nCompose--;
				else if (currentWord == "endcompose")
					nCompose++;
				else if (currentWord == "procedure")
					nProcedure--;
				else if (currentWord == "endprocedure")
					nProcedure++;
				if (currentWord == "if" && !nFor && !nIf && !nWhile && !nCompose && !nProcedure)
				{
					nStartPos = WordStartPosition(i, true);
					break;
				}
				i -= currentWord.length();
			}
			if (nFor < 0 || nWhile < 0 || nIf < 0 || nCompose < 0 || nProcedure < 0)
			{
				// There's no matching partner
				// set the first to invalid but do not return
				vPos.push_back(wxSTC_INVALID_POSITION);
				break;
			}
		}

		if (nFor > 0 || nWhile > 0 || nIf > 0 || nCompose > 0 || nProcedure > 0)
		{
			// There's no matching partner
			// set the first to invalid but do not return
			vPos.push_back(wxSTC_INVALID_POSITION);
			nIf = 1;
		}
		else
			nIf = 0;
		nFor = 0;
		nWhile = 0;
		nCompose = 0;
		nProcedure = 0;

		bSearchForIf = true;
		endblock = "endif";
	}
	else if (startblock == "if" || startblock == "for" || startblock == "while" || startblock == "compose" || startblock == "procedure")
	{
		endblock = "end" + startblock;
	}
	else
	{
		vPos.push_back(wxSTC_INVALID_POSITION);
		return vPos;
	}

	if (startblock == "if" || endblock == "if")
		bSearchForIf = true;

	vPos.push_back(nStartPos);

	if (nSearchDir == -1)
		nStartPos = WordEndPosition(nPos, true);
	for (int i = nStartPos; (i < this->GetLastPosition() && i >= 0); i += nSearchDir) // iterates down, if nSearchDir == 1, and up of nSearchDir == -1
	{
		if (this->GetStyleAt(i) == wxSTC_NSCR_COMMAND || this->GetStyleAt(i) == wxSTC_NPRC_COMMAND)
		{
			wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
			if (currentWord == "for")
				nFor += nSearchDir; //if we iterate upwards, the closing blocks shall increment and the opening blocks decrement the counter
			else if (currentWord == "endfor")
				nFor -= nSearchDir;
			else if (currentWord == "while")
				nWhile += nSearchDir;
			else if (currentWord == "endwhile")
				nWhile -= nSearchDir;
			else if (currentWord == "if")
				nIf += nSearchDir;
			else if (currentWord == "endif")
				nIf -= nSearchDir;
			else if (currentWord == "compose")
				nCompose += nSearchDir;
			else if (currentWord == "endcompose")
				nCompose -= nSearchDir;
			else if (currentWord == "procedure")
				nProcedure += nSearchDir;
			else if (currentWord == "endprocedure")
				nProcedure -= nSearchDir;
			if (bSearchForIf && nIf == 1 && !nFor && !nWhile && !nProcedure && !nCompose // only in the current if block
					&& (currentWord == "else" || currentWord == "elseif"))
			{
				vPos.push_back(WordStartPosition(i, true));
			}
			if (currentWord == endblock && !nFor && !nIf && !nWhile && !nProcedure && !nCompose)
			{
				vPos.push_back(WordStartPosition(i, true));
				break;
			}
			i += nSearchDir * currentWord.length();
		}
		if (nFor < 0 || nWhile < 0 || nIf < 0 || nProcedure < 0 || nCompose < 0)
		{
			// There's no matching partner
			vPos.push_back(wxSTC_INVALID_POSITION);
			break;
		}
	}
	if (!vPos.size()
			|| (nFor > 0 || nWhile > 0 || nIf > 0 || nProcedure > 0 || nCompose > 0))
		vPos.push_back(wxSTC_INVALID_POSITION);
	return vPos;
}

vector<int> NumeReEditor::BlockMatchMATLAB(int nPos)
{
	int nBlock = 0;

	int nStartPos = WordStartPosition(nPos, true);
	vector<int> vPos;
	wxString startblock;
	wxString endblock;
	bool bSearchForIf = false; //if we search for an if block element. If yes => also mark the "else..." parts.
	bool bSearchForSwitch = false;
	bool bSearchForCatch = false;
	int nSearchDir = 1; //direction in which to search for the matching block partner
	if (this->GetStyleAt(nPos) != wxSTC_MATLAB_KEYWORD)
	{
		if (nPos && this->GetStyleAt(nPos - 1) == wxSTC_MATLAB_KEYWORD)
			nPos--;
		else
		{
			vPos.push_back(wxSTC_INVALID_POSITION);
			return vPos;
		}
	}


	startblock = this->GetTextRange(WordStartPosition(nPos, true), WordEndPosition(nPos, true));
	if (startblock == "end")
	{
		// search for starting block
		// adding 1 to nBlock, because we're already inside of an "block"
		//nBlock++;
		for (int i = WordStartPosition(nPos, true); i >= 0; i--)
		{
			if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
			{
				wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
				if (currentWord == "for"
						|| currentWord == "while"
						|| currentWord == "function"
						|| currentWord == "if"
						|| currentWord == "switch"
						|| currentWord == "try"
						|| currentWord == "classdef"
						|| currentWord == "properties"
						|| currentWord == "methods")
					nBlock--;
				else if (currentWord == "end")
					nBlock++;

				if (!nBlock)
				{
					nStartPos = WordStartPosition(i, true);
					startblock = currentWord;
					if (currentWord == "if")
						bSearchForIf = true;
					if (currentWord == "switch")
						bSearchForSwitch = true;
					if (currentWord == "try")
						bSearchForCatch = true;
					break;
				}
				i -= currentWord.length();
			}
			if (nBlock < 0)
			{
				// There's no matching partner
				// set the first to invalid but do not return
				vPos.push_back(wxSTC_INVALID_POSITION);
				break;
			}
		}
		endblock = "end";
	}
	else if (startblock == "else" || startblock == "elseif")
	{
		// search for starting "if"
		// adding 1 to nBlock, because we're already inside of an "if"
		nBlock++;
		for (int i = WordEndPosition(nPos, true); i >= 0; i--)
		{
			if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
			{
				wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
				if (currentWord == "for"
						|| currentWord == "while"
						|| currentWord == "function"
						|| currentWord == "if"
						|| currentWord == "switch"
						|| currentWord == "try"
						|| currentWord == "classdef"
						|| currentWord == "properties"
						|| currentWord == "methods")
					nBlock--;
				else if (currentWord == "end")
					nBlock++;

				if (currentWord == "if" && !nBlock)
				{
					nStartPos = WordStartPosition(i, true);
					startblock = "if";
					break;
				}
				i -= currentWord.length();
			}
			if (nBlock < 0)
			{
				// There's no matching partner
				// set the first to invalid but do not return
				vPos.push_back(wxSTC_INVALID_POSITION);
				break;
			}
		}

		if (nBlock > 0)
		{
			// There's no matching partner
			// set the first to invalid but do not return
			vPos.push_back(wxSTC_INVALID_POSITION);
			nBlock = 1;
		}
		else
			nBlock = 0;

		bSearchForIf = true;
		endblock = "end";
	}
	else if (startblock == "case" || startblock == "otherwise")
	{
		// search for starting "if"
		// adding 1 to nBlock, because we're already inside of an "if"
		nBlock++;
		for (int i = WordEndPosition(nPos, true); i >= 0; i--)
		{
			if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
			{
				wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
				if (currentWord == "for"
						|| currentWord == "while"
						|| currentWord == "function"
						|| currentWord == "if"
						|| currentWord == "switch"
						|| currentWord == "try"
						|| currentWord == "classdef"
						|| currentWord == "properties"
						|| currentWord == "methods")
					nBlock--;
				else if (currentWord == "end")
					nBlock++;

				if (currentWord == "switch" && !nBlock)
				{
					nStartPos = WordStartPosition(i, true);
					startblock = "switch";
					break;
				}
				i -= currentWord.length();
			}
			if (nBlock < 0)
			{
				// There's no matching partner
				// set the first to invalid but do not return
				vPos.push_back(wxSTC_INVALID_POSITION);
				break;
			}
		}

		if (nBlock > 0)
		{
			// There's no matching partner
			// set the first to invalid but do not return
			vPos.push_back(wxSTC_INVALID_POSITION);
			nBlock = 1;
		}
		else
			nBlock = 0;

		bSearchForSwitch = true;
		endblock = "end";
	}
	else if (startblock == "catch")
	{
		// search for starting "catch"
		// adding 1 to nBlock, because we're already inside of an "if"
		nBlock++;
		for (int i = WordEndPosition(nPos, true); i >= 0; i--)
		{
			if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
			{
				wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
				if (currentWord == "for"
						|| currentWord == "while"
						|| currentWord == "function"
						|| currentWord == "if"
						|| currentWord == "switch"
						|| currentWord == "try"
						|| currentWord == "classdef"
						|| currentWord == "properties"
						|| currentWord == "methods")
					nBlock--;
				else if (currentWord == "end")
					nBlock++;

				if (currentWord == "try" && !nBlock)
				{
					nStartPos = WordStartPosition(i, true);
					startblock = "try";
					break;
				}
				i -= currentWord.length();
			}
			if (nBlock < 0)
			{
				// There's no matching partner
				// set the first to invalid but do not return
				vPos.push_back(wxSTC_INVALID_POSITION);
				break;
			}
		}

		if (nBlock > 0)
		{
			// There's no matching partner
			// set the first to invalid but do not return
			vPos.push_back(wxSTC_INVALID_POSITION);
			nBlock = 1;
		}
		else
			nBlock = 0;

		bSearchForCatch = true;
		endblock = "end";
	}
	if (startblock == "for"
			|| startblock == "while"
			|| startblock == "function"
			|| startblock == "if"
			|| startblock == "switch"
			|| startblock == "try"
			|| startblock == "classdef"
			|| startblock == "properties"
			|| startblock == "methods")
	{
		endblock = "end";
	}
	else
	{
		vPos.push_back(wxSTC_INVALID_POSITION);
		return vPos;
	}

	if (startblock == "if" || endblock == "if")
		bSearchForIf = true;
	if (startblock == "switch" || endblock == "switch")
		bSearchForSwitch = true;
	if (startblock == "try" || endblock == "try")
		bSearchForCatch = true;

	vPos.push_back(nStartPos);

	if (nSearchDir == -1)
		nStartPos = WordEndPosition(nPos, true);
	for (int i = nStartPos; (i < this->GetLastPosition() && i >= 0); i += nSearchDir) // iterates down, if nSearchDir == 1, and up of nSearchDir == -1
	{
		if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
		{
			wxString currentWord = this->GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));
			if (currentWord == "for"
					|| currentWord == "while"
					|| currentWord == "function"
					|| currentWord == "if"
					|| currentWord == "switch"
					|| currentWord == "try"
					|| currentWord == "classdef"
					|| currentWord == "properties"
					|| currentWord == "methods")
				nBlock += nSearchDir; //if we iterate upwards, the closing blocks shall increment and the opening blocks decrement the counter
			else if (currentWord == "end")
				nBlock -= nSearchDir;
			if (bSearchForIf && nBlock == 1 // only in the current if block
					&& (currentWord == "else" || currentWord == "elseif"))
			{
				vPos.push_back(WordStartPosition(i, true));
			}
			if (bSearchForSwitch && nBlock == 1 // only in the current if block
					&& (currentWord == "case" || currentWord == "otherwise"))
			{
				vPos.push_back(WordStartPosition(i, true));
			}
			if (bSearchForCatch && nBlock == 1 // only in the current if block
					&& currentWord == "catch")
			{
				vPos.push_back(WordStartPosition(i, true));
			}
			if (currentWord == endblock && !nBlock)
			{
				vPos.push_back(WordStartPosition(i, true));
				break;
			}
			i += nSearchDir * currentWord.length();
		}
		if (nBlock < 0)
		{
			// There's no matching partner
			vPos.push_back(wxSTC_INVALID_POSITION);
			break;
		}
	}
	if (!vPos.size()
			|| (nBlock > 0))
		vPos.push_back(wxSTC_INVALID_POSITION);
	return vPos;
}

//////////////////////////////////////////////////////////////////////////////
///  public HasBeenSaved
///  Checks if the editor has been saved
///
///  @return bool Whether or not the editor has been saved
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::HasBeenSaved()
{
	bool result = m_fileNameAndPath.GetFullPath() != wxEmptyString;
	return result;// && !m_bSetUnsaved;
}

void NumeReEditor::SetUnsaved()
{
	m_bSetUnsaved = true;
}


//////////////////////////////////////////////////////////////////////////////
///  public UpdateSyntaxHighlighting
///  Sets up the editor's syntax highlighting
///
///  @return void
///
///  @remarks Currently only called on creation.  If syntax highlighting customization was
///  @remarks allowed, this is where the user's choices would be used
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::UpdateSyntaxHighlighting(bool forceUpdate)
{
	wxString filename = GetFileNameAndPath();

	this->StyleSetBackground(wxSTC_STYLE_DEFAULT, m_options->GetSyntaxStyle(Options::STANDARD).background);

	FileFilterType filetype = m_project->GetFileType(filename);
	if (m_fileType != filetype)
		m_fileType = filetype;
	else if (!forceUpdate && (m_fileType == FILE_NSCR
							  || m_fileType == FILE_NPRC
							  || m_fileType == FILE_TEXSOURCE
							  || m_fileType == FILE_DATAFILES
							  || m_fileType == FILE_MATLAB
							  || m_fileType == FILE_CPP))
		return;


	// make it for both: NSCR and NPRC
	if (filetype == FILE_NSCR || filetype == FILE_NPRC || filetype == FILE_MATLAB || filetype == FILE_CPP)
	{
		this->SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

		this->SetMarginType(MARGIN_FOLD, wxSTC_MARGIN_SYMBOL);
		this->SetMarginWidth(MARGIN_FOLD, 13);
		this->SetMarginMask(MARGIN_FOLD, wxSTC_MASK_FOLDERS);
		this->SetMarginSensitive(MARGIN_FOLD, true);
		this->StyleSetBackground(MARGIN_FOLD, wxColor(200, 200, 200) );
		this->SetMarginSensitive(MARGIN_FOLD, true);

		wxColor grey( 100, 100, 100 );
		this->MarkerDefine (wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDER, "WHITE");
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDER, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDEROPEN, "WHITE");
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDEROPEN, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDERSUB, grey);
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDERSUB, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDEREND, "WHITE");
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDEREND, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDEROPENMID, "WHITE");
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDEROPENMID, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDERMIDTAIL, grey);
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDERMIDTAIL, grey);

		this->MarkerDefine (wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER);
		this->MarkerSetForeground (wxSTC_MARKNUM_FOLDERTAIL, grey);
		this->MarkerSetBackground (wxSTC_MARKNUM_FOLDERTAIL, grey);

		this->MarkerEnableHighlight(true);
	}
	if (filetype == FILE_NSCR)
	{
		m_fileType = FILE_NSCR;
		this->SetLexer(wxSTC_LEX_NSCR);
		this->SetProperty("fold", "1");
		if (_syntax)
		{
			this->SetKeyWords(0, _syntax->getCommands());
			this->SetKeyWords(1, _syntax->getOptions());
			this->SetKeyWords(2, _syntax->getFunctions());
			this->SetKeyWords(3, _syntax->getMethods());
			this->SetKeyWords(4, "x y z t");
			this->SetKeyWords(5, _syntax->getConstants());
			this->SetKeyWords(6, _syntax->getSpecial());
			this->SetKeyWords(7, _syntax->getOperators());
			this->SetKeyWords(8, _syntax->getNPRCCommands());
		}

		for (int i = 0; i <= wxSTC_NSCR_PROCEDURE_COMMANDS; i++)
		{
			SyntaxStyles _style;
			switch (i)
			{
				case wxSTC_NSCR_DEFAULT:
				case wxSTC_NSCR_IDENTIFIER:
					_style = m_options->GetSyntaxStyle(Options::STANDARD);
					break;
				case wxSTC_NSCR_NUMBERS:
					_style = m_options->GetSyntaxStyle(Options::NUMBER);
					break;
				case wxSTC_NSCR_COMMENT_BLOCK:
				case wxSTC_NSCR_COMMENT_LINE:
					_style = m_options->GetSyntaxStyle(Options::COMMENT);
					break;
				case wxSTC_NSCR_COMMAND:
					_style = m_options->GetSyntaxStyle(Options::COMMAND);
					break;
				case wxSTC_NSCR_OPTION:
					_style = m_options->GetSyntaxStyle(Options::OPTION);
					break;
				case wxSTC_NSCR_CONSTANTS:
					_style = m_options->GetSyntaxStyle(Options::CONSTANT);
					break;
				case wxSTC_NSCR_FUNCTION:
					_style = m_options->GetSyntaxStyle(Options::FUNCTION);
					break;
				case wxSTC_NSCR_METHOD:
					_style = m_options->GetSyntaxStyle(Options::METHODS);
					break;
				case wxSTC_NSCR_PREDEFS:
					_style = m_options->GetSyntaxStyle(Options::SPECIALVAL);
					break;
				case wxSTC_NSCR_STRING:
					_style = m_options->GetSyntaxStyle(Options::STRING);
					break;
				case wxSTC_NSCR_STRING_PARSER:
					_style = m_options->GetSyntaxStyle(Options::STRINGPARSER);
					break;
				case wxSTC_NSCR_INCLUDES:
					_style = m_options->GetSyntaxStyle(Options::INCLUDES);
					break;
				case wxSTC_NSCR_PROCEDURES:
					_style = m_options->GetSyntaxStyle(Options::PROCEDURE);
					break;
				case wxSTC_NSCR_PROCEDURE_COMMANDS:
					_style = m_options->GetSyntaxStyle(Options::PROCEDURE_COMMAND);
					break;
				case wxSTC_NSCR_INSTALL:
					_style = m_options->GetSyntaxStyle(Options::INSTALL);
					break;
				case wxSTC_NSCR_DEFAULT_VARS:
					_style = m_options->GetSyntaxStyle(Options::DEFAULT_VARS);
					break;
				case wxSTC_NSCR_CUSTOM_FUNCTION:
					_style = m_options->GetSyntaxStyle(Options::CUSTOM_FUNCTION);
					break;
				case wxSTC_NSCR_OPERATORS:
				case wxSTC_NSCR_OPERATOR_KEYWORDS:
					_style = m_options->GetSyntaxStyle(Options::OPERATOR);
					break;
			}

			this->StyleSetForeground(i, _style.foreground);
			if (!_style.defaultbackground)
				this->StyleSetBackground(i, _style.background);
			else
				this->StyleSetBackground(i, this->StyleGetBackground(wxSTC_STYLE_DEFAULT));
			this->StyleSetBold(i, _style.bold);
			this->StyleSetItalic(i, _style.italics);
			this->StyleSetUnderline(i, _style.underline);
		}
	}
	else if (filetype == FILE_NPRC)
	{
		m_fileType = FILE_NPRC;
		this->SetLexer(wxSTC_LEX_NPRC);
		this->SetProperty("fold", "1");
		if (_syntax)
		{
			this->SetKeyWords(0, _syntax->getCommands() + _syntax->getNPRCCommands());
			this->SetKeyWords(1, _syntax->getOptions());
			this->SetKeyWords(2, _syntax->getFunctions());
			this->SetKeyWords(3, _syntax->getMethods());
			this->SetKeyWords(4, "x y z t");
			this->SetKeyWords(5, _syntax->getConstants());
			this->SetKeyWords(6, _syntax->getSpecial());
			this->SetKeyWords(7, _syntax->getOperators());
		}


		for (int i = 0; i <= wxSTC_NPRC_FLAGS; i++)
		{
			SyntaxStyles _style;
			switch (i)
			{
				case wxSTC_NPRC_DEFAULT:
				case wxSTC_NPRC_IDENTIFIER:
					_style = m_options->GetSyntaxStyle(Options::STANDARD);
					break;
				case wxSTC_NPRC_NUMBERS:
					_style = m_options->GetSyntaxStyle(Options::NUMBER);
					break;
				case wxSTC_NPRC_COMMENT_BLOCK:
				case wxSTC_NPRC_COMMENT_LINE:
					_style = m_options->GetSyntaxStyle(Options::COMMENT);
					break;
				case wxSTC_NPRC_COMMAND:
					_style = m_options->GetSyntaxStyle(Options::COMMAND);
					break;
				case wxSTC_NPRC_OPTION:
					_style = m_options->GetSyntaxStyle(Options::OPTION);
					break;
				case wxSTC_NPRC_CONSTANTS:
					_style = m_options->GetSyntaxStyle(Options::CONSTANT);
					break;
				case wxSTC_NPRC_FUNCTION:
					_style = m_options->GetSyntaxStyle(Options::FUNCTION);
					break;
				case wxSTC_NPRC_METHOD:
					_style = m_options->GetSyntaxStyle(Options::METHODS);
					break;
				case wxSTC_NPRC_PREDEFS:
					_style = m_options->GetSyntaxStyle(Options::SPECIALVAL);
					break;
				case wxSTC_NPRC_STRING:
					_style = m_options->GetSyntaxStyle(Options::STRING);
					break;
				case wxSTC_NPRC_STRING_PARSER:
					_style = m_options->GetSyntaxStyle(Options::STRINGPARSER);
					break;
				case wxSTC_NPRC_INCLUDES:
					_style = m_options->GetSyntaxStyle(Options::INCLUDES);
					break;
				case wxSTC_NPRC_PROCEDURES:
				case wxSTC_NPRC_FLAGS:
					_style = m_options->GetSyntaxStyle(Options::PROCEDURE);
					break;
				case wxSTC_NPRC_DEFAULT_VARS:
					_style = m_options->GetSyntaxStyle(Options::DEFAULT_VARS);
					break;
				case wxSTC_NPRC_CUSTOM_FUNCTION:
					_style = m_options->GetSyntaxStyle(Options::CUSTOM_FUNCTION);
					break;
				case wxSTC_NPRC_OPERATORS:
				case wxSTC_NPRC_OPERATOR_KEYWORDS:
					_style = m_options->GetSyntaxStyle(Options::OPERATOR);
					break;
			}

			this->StyleSetForeground(i, _style.foreground);
			if (!_style.defaultbackground)
				this->StyleSetBackground(i, _style.background);
			else
				this->StyleSetBackground(i, this->StyleGetBackground(wxSTC_STYLE_DEFAULT));
			this->StyleSetBold(i, _style.bold);
			this->StyleSetItalic(i, _style.italics);
			this->StyleSetUnderline(i, _style.underline);
		}
	}
	else if (filetype == FILE_TEXSOURCE)
	{
		this->SetLexer(wxSTC_LEX_TEX);
		this->StyleSetForeground(wxSTC_TEX_DEFAULT, wxColor(0, 128, 0)); //Comment
		this->StyleSetForeground(wxSTC_TEX_COMMAND, wxColor(0, 0, 255)); //Command
		this->StyleSetBold(wxSTC_TEX_COMMAND, true);
		this->StyleSetUnderline(wxSTC_TEX_COMMAND, false);
		this->StyleSetForeground(wxSTC_TEX_TEXT, wxColor(0, 0, 0)); // Actual text
		this->StyleSetForeground(wxSTC_TEX_GROUP, wxColor(0, 128, 0)); // Grouping elements like $ $ or { }
		this->StyleSetBackground(wxSTC_TEX_GROUP, wxColor(255, 255, 183)); // Grouping elements like $ $ or { }
		this->StyleSetBold(wxSTC_TEX_GROUP, true);
		this->StyleSetForeground(wxSTC_TEX_SPECIAL, wxColor(255, 0, 196)); // Parentheses/Brackets
		this->StyleSetItalic(wxSTC_TEX_SPECIAL, false);
		this->StyleSetBold(wxSTC_TEX_SPECIAL, true);
		this->StyleSetForeground(wxSTC_TEX_SYMBOL, wxColor(255, 0, 0)); // Operators
		this->StyleSetBackground(wxSTC_TEX_SYMBOL, wxColor(255, 255, 255));
		this->StyleSetBold(wxSTC_TEX_SYMBOL, false);
	}
	else if (filetype == FILE_DATAFILES)
	{
		this->SetLexer(wxSTC_LEX_OCTAVE);
		this->StyleSetForeground(wxSTC_MATLAB_COMMENT, wxColor(0, 128, 0));
		this->StyleSetItalic(wxSTC_MATLAB_COMMENT, false);
		this->StyleSetForeground(wxSTC_MATLAB_OPERATOR, wxColor(255, 0, 0));
		this->StyleSetBold(wxSTC_MATLAB_OPERATOR, false);
		this->StyleSetForeground(wxSTC_MATLAB_NUMBER, wxColor(0, 0, 128));
		this->StyleSetBackground(wxSTC_MATLAB_NUMBER, wxColor(255, 255, 255));
		this->StyleSetForeground(wxSTC_MATLAB_IDENTIFIER, wxColor(0, 0, 0));
		this->StyleSetBold(wxSTC_MATLAB_IDENTIFIER, false);
	}
	else if (filetype == FILE_MATLAB)
	{
		this->SetLexer(wxSTC_LEX_MATLAB);
		this->SetProperty("fold", "1");
		if (_syntax)
		{
			this->SetKeyWords(0, _syntax->getMatlab());
			this->SetKeyWords(1, _syntax->getMatlabFunctions());
		}

		for (int i = 0; i <= wxSTC_MATLAB_FUNCTIONS; i++)
		{
			SyntaxStyles _style;
			switch (i)
			{
				case wxSTC_MATLAB_DEFAULT:
				case wxSTC_MATLAB_IDENTIFIER:
					_style = m_options->GetSyntaxStyle(Options::STANDARD);
					break;
				case wxSTC_MATLAB_NUMBER:
					_style = m_options->GetSyntaxStyle(Options::NUMBER);
					break;
				case wxSTC_MATLAB_COMMENT:
					_style = m_options->GetSyntaxStyle(Options::COMMENT);
					break;
				case wxSTC_MATLAB_COMMAND:
				case wxSTC_MATLAB_KEYWORD:
					_style = m_options->GetSyntaxStyle(Options::COMMAND);
					break;
				case wxSTC_MATLAB_FUNCTIONS:
					_style = m_options->GetSyntaxStyle(Options::FUNCTION);
					break;
				case wxSTC_MATLAB_STRING:
				case wxSTC_MATLAB_DOUBLEQUOTESTRING:
					_style = m_options->GetSyntaxStyle(Options::STRING);
					break;
				case wxSTC_MATLAB_OPERATOR:
					_style = m_options->GetSyntaxStyle(Options::OPERATOR);
					break;
			}

			this->StyleSetForeground(i, _style.foreground);
			if (!_style.defaultbackground)
				this->StyleSetBackground(i, _style.background);
			else
				this->StyleSetBackground(i, this->StyleGetBackground(wxSTC_STYLE_DEFAULT));
			this->StyleSetBold(i, _style.bold);
			this->StyleSetItalic(i, _style.italics);
			this->StyleSetUnderline(i, _style.underline);
		}

	}
	else if (filetype == FILE_CPP)
	{
		this->SetLexer(wxSTC_LEX_CPP);
		this->SetProperty("fold", "1");
		if (_syntax)
		{
			this->SetKeyWords(0, _syntax->getCpp());
			this->SetKeyWords(1, _syntax->getCppFunctions());
		}

		for (int i = 0; i <= wxSTC_C_PREPROCESSORCOMMENT; i++)
		{
			SyntaxStyles _style;
			switch (i)
			{
				case wxSTC_C_DEFAULT :
				case wxSTC_C_IDENTIFIER:
					_style = m_options->GetSyntaxStyle(Options::STANDARD);
					break;
				case wxSTC_C_NUMBER:
					_style = m_options->GetSyntaxStyle(Options::NUMBER);
					break;
				case wxSTC_C_COMMENT:
				case wxSTC_C_COMMENTLINE:
					_style = m_options->GetSyntaxStyle(Options::COMMENT);
					break;
				case wxSTC_C_WORD:
					_style = m_options->GetSyntaxStyle(Options::COMMAND);
					break;
				case wxSTC_C_WORD2:
					_style = m_options->GetSyntaxStyle(Options::FUNCTION);
					break;
				case wxSTC_C_STRING:
					_style = m_options->GetSyntaxStyle(Options::STRING);
					break;
				case wxSTC_C_CHARACTER:
					_style = m_options->GetSyntaxStyle(Options::STRINGPARSER);
					break;
				case wxSTC_C_PREPROCESSOR:
					_style = m_options->GetSyntaxStyle(Options::INCLUDES);
					break;
				case wxSTC_C_OPERATOR:
					_style = m_options->GetSyntaxStyle(Options::OPERATOR);
					break;
			}

			this->StyleSetForeground(i, _style.foreground);
			if (!_style.defaultbackground)
				this->StyleSetBackground(i, _style.background);
			else
				this->StyleSetBackground(i, this->StyleGetBackground(wxSTC_STYLE_DEFAULT));
			this->StyleSetBold(i, _style.bold);
			this->StyleSetItalic(i, _style.italics);
			this->StyleSetUnderline(i, _style.underline);
		}

		/*this->StyleSetForeground(wxSTC_C_IDENTIFIER, wxColor(0,0,0));
		this->StyleSetForeground(wxSTC_C_COMMENT, wxColor(0,128,0));
		this->StyleSetItalic(wxSTC_C_COMMENT, true);
		this->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColor(0,128,0));
		this->StyleSetItalic(wxSTC_C_COMMENTLINE, true);

		this->StyleSetForeground(wxSTC_C_OPERATOR, wxColor(255,0,0));

		this->StyleSetForeground(wxSTC_C_WORD, wxColor(0,0,255));
		this->StyleSetBold(wxSTC_C_WORD, true);

		this->StyleSetForeground(wxSTC_C_WORD2, wxColor(0,0,128));
		this->StyleSetBold(wxSTC_C_WORD2, true);

		this->StyleSetForeground(wxSTC_C_STRING, wxColor(64,64,255));

		this->StyleSetForeground(wxSTC_C_CHARACTER, wxColor(128,128,128));

		this->StyleSetForeground(wxSTC_C_PREPROCESSOR, wxColor(128,0,0));

		this->StyleSetForeground(wxSTC_C_NUMBER, wxColor(255,128,128));*/
	}
	else
	{
		if (!getEditorSetting(SETTING_USETXTADV))
		{
			this->SetLexer(wxSTC_LEX_NULL);
			this->ClearDocumentStyle();
		}
		else
		{
			this->SetLexer(wxSTC_LEX_TXTADV);
			this->StyleSetItalic(wxSTC_TXTADV_DEFAULT, false);
			this->StyleSetItalic(wxSTC_TXTADV_MODIFIER, true);
			this->StyleSetForeground(wxSTC_TXTADV_MODIFIER, wxColor(255, 180, 180));
			this->StyleSetItalic(wxSTC_TXTADV_ITALIC, true);
			this->StyleSetItalic(wxSTC_TXTADV_BOLD, false);
			this->StyleSetBold(wxSTC_TXTADV_BOLD, true);
			this->StyleSetItalic(wxSTC_TXTADV_BOLD_ITALIC, true);
			this->StyleSetBold(wxSTC_TXTADV_BOLD_ITALIC, true);
			this->StyleSetUnderline(wxSTC_TXTADV_UNDERLINE, true);
			this->StyleSetForeground(wxSTC_TXTADV_STRIKETHROUGH, wxColor(140, 140, 140));
			this->StyleSetItalic(wxSTC_TXTADV_STRIKETHROUGH, true);
			this->StyleSetUnderline(wxSTC_TXTADV_URL, true);
			this->StyleSetForeground(wxSTC_TXTADV_URL, wxColor(0, 0, 255));
			this->StyleSetUnderline(wxSTC_TXTADV_HEAD, true);
			this->StyleSetBold(wxSTC_TXTADV_HEAD, true);
			this->StyleSetUnderline(wxSTC_TXTADV_BIGHEAD, true);
			this->StyleSetBold(wxSTC_TXTADV_BIGHEAD, true);
			this->StyleSetSize(wxSTC_TXTADV_BIGHEAD, this->StyleGetSize(0) + 1);
			this->StyleSetCase(wxSTC_TXTADV_BIGHEAD, wxSTC_CASE_UPPER);
		}
		//this->ClearDocumentStyle();
	}
	applyStrikeThrough();
	updateDefaultHighlightSettings();
	this->Colourise(0, -1);
}


void NumeReEditor::updateDefaultHighlightSettings()
{
	this->CallTipSetForegroundHighlight(*wxBLUE);
	this->SetCaretLineVisible(true);
	this->SetIndentationGuides(true);

	if (!m_options->GetSyntaxStyle(Options::ACTIVE_LINE).defaultbackground)
		this->SetCaretLineBackground(m_options->GetSyntaxStyle(Options::ACTIVE_LINE).background);
	else
		this->SetCaretLineVisible(false);

	// standard settings for the brace highlighting
	this->StyleSetForeground(wxSTC_STYLE_BRACELIGHT, wxColour(0, 150, 0));
	this->StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour(0, 220, 0));
	this->StyleSetBold(wxSTC_STYLE_BRACELIGHT, true);
	this->StyleSetSize(wxSTC_STYLE_BRACELIGHT, this->StyleGetSize(0) + 1);
	this->StyleSetForeground(wxSTC_STYLE_BRACEBAD, wxColour(150, 0, 0));
	this->StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(220, 0, 0));
	this->StyleSetBold(wxSTC_STYLE_BRACEBAD, true);
	this->StyleSetSize(wxSTC_STYLE_BRACEBAD, this->StyleGetSize(0) + 1);

	// Style settings for the displayed annotations
	int nAnnotationFontSize = this->StyleGetSize(wxSTC_STYLE_DEFAULT);

	if (nAnnotationFontSize >= 10)
        nAnnotationFontSize -= 2;
    else if (nAnnotationFontSize >= 8)
        nAnnotationFontSize -= 1;

	this->StyleSetBackground(ANNOTATION_NOTE, wxColour(240, 240, 240));
	this->StyleSetForeground(ANNOTATION_NOTE, wxColour(120, 120, 120));
	this->StyleSetSize(ANNOTATION_NOTE, nAnnotationFontSize);
	this->StyleSetItalic(ANNOTATION_NOTE, true);
	this->StyleSetFaceName(ANNOTATION_NOTE, "Segoe UI");
	this->StyleSetBackground(ANNOTATION_WARN, wxColour(255, 255, 220));
	this->StyleSetForeground(ANNOTATION_WARN, wxColour(160, 160, 0));
	this->StyleSetSize(ANNOTATION_WARN, nAnnotationFontSize);
	this->StyleSetItalic(ANNOTATION_WARN, true);
	this->StyleSetFaceName(ANNOTATION_WARN, "Segoe UI");
	this->StyleSetBackground(ANNOTATION_ERROR, wxColour(255, 200, 200));
	this->StyleSetForeground(ANNOTATION_ERROR, wxColour(170, 0, 0));
	this->StyleSetSize(ANNOTATION_ERROR, nAnnotationFontSize);
	this->StyleSetItalic(ANNOTATION_ERROR, true);
	this->StyleSetFaceName(ANNOTATION_ERROR, "Segoe UI");
}

void NumeReEditor::applyStrikeThrough()
{
	if (!getEditorSetting(SETTING_USETXTADV)
			|| m_fileType == FILE_NSCR
			|| m_fileType == FILE_NPRC
			|| m_fileType == FILE_TEXSOURCE
			|| m_fileType == FILE_DATAFILES)
		return;
	this->SetIndicatorCurrent(HIGHLIGHT_STRIKETHROUGH);
	this->IndicatorClearRange(0, GetLastPosition());
	this->IndicatorSetStyle(HIGHLIGHT_STRIKETHROUGH, wxSTC_INDIC_STRIKE);
	this->IndicatorSetForeground(HIGHLIGHT_STRIKETHROUGH, wxColor(255, 0, 0));
	for (int i = 0; i < GetLastPosition(); i++)
	{
		if (GetStyleAt(i) == wxSTC_TXTADV_STRIKETHROUGH)
		{
			for (int j = i; j < GetLastPosition(); j++)
			{
				if (GetStyleAt(j) == wxSTC_TXTADV_MODIFIER || j == GetLastPosition() - 1)
				{
					this->IndicatorFillRange(i, j - i);
					i = j;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public SetFilename
///  Sets the filename for the editor
///
///  @param  filename     wxFileName  The filename for this editor
///  @param  fileIsRemote bool        Whether this file is remote or local
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::SetFilename(wxFileName filename, bool fileIsRemote)
{
	m_bLastSavedRemotely = fileIsRemote;

	//m_fileNameAndPath.Assign(path, name, fileIsRemote ? wxPATH_UNIX : wxPATH_DOS);

	if (m_project->IsSingleFile())
	{
		wxString oldFileName = m_fileNameAndPath.GetFullPath(m_bLastSavedRemotely ? wxPATH_UNIX : wxPATH_DOS);

		if (m_project->FileExistsInProject(oldFileName))
		{
			FileFilterType oldFilterType;
			wxString oldExtension = m_fileNameAndPath.GetExt();
			if (oldExtension.StartsWith("h"))
			{
				oldFilterType = FILE_NPRC;
			}
			else if (oldExtension.StartsWith("c"))
			{
				oldFilterType = FILE_NSCR;
			}
			else if (oldExtension.StartsWith("txt"))
			{
				oldFilterType = FILE_NONSOURCE;
			}
			else
			{
				oldFilterType = FILE_NONSOURCE;
			}

			m_project->RemoveFileFromProject(oldFileName, oldFilterType);
		}

		wxString newFileName = filename.GetFullPath(fileIsRemote ? wxPATH_UNIX : wxPATH_DOS);
		if (!m_project->FileExistsInProject(newFileName))
		{
			FileFilterType newFilterType;
			wxString newExtension = filename.GetExt();
			if (newExtension.StartsWith("h"))
			{
				newFilterType = FILE_NPRC;
			}
			else if (newExtension.StartsWith("c"))
			{
				newFilterType = FILE_NSCR;
			}
			else if (newExtension.StartsWith("txt"))
			{
				newFilterType = FILE_NONSOURCE;
			}
			else
			{
				newFilterType = FILE_NONSOURCE;
			}
			m_project->AddFileToProject(newFileName, newFilterType);
		}

		m_project->SetRemote(fileIsRemote);
	}

	m_fileNameAndPath = filename;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetFileNameAndPath
///  Gets the full pathname of this file as a string
///
///  @return wxString The full pathname of this file
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString NumeReEditor::GetFileNameAndPath()
{
	wxString nameAndPath = m_fileNameAndPath.GetFullPath();//m_bLastSavedRemotely ? wxPATH_UNIX : wxPATH_DOS);
	return nameAndPath;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetFilenameString
///  Gets the name of this file with no path
///
///  @return wxString The name of this file
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString NumeReEditor::GetFilenameString()
{
	return m_fileNameAndPath.GetFullName();
}

//////////////////////////////////////////////////////////////////////////////
///  public GetFileName
///  Gets the wxFileName for this file
///
///  @return wxFileName The editor's filename
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxFileName NumeReEditor::GetFileName()
{
	return m_fileNameAndPath;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetFilePath
///  Gets the path for this file
///
///  @return wxString The path for this file
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString NumeReEditor::GetFilePath()
{
	return m_fileNameAndPath.GetPath(false, m_bLastSavedRemotely ? wxPATH_UNIX : wxPATH_DOS);
}

//////////////////////////////////////////////////////////////////////////////
///  public ResetEditor
///  Clears out the editor's contents and resets it completely
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::ResetEditor()
{
	ClearAll();

	m_fileNameAndPath.Clear();
	m_breakpoints.Clear();

	SetReadOnly(false);
	SetText(wxEmptyString);
	SetSavePoint();
	EmptyUndoBuffer();

	MarkerDeleteAll(MARKER_BREAKPOINT);
	MarkerDeleteAll(MARKER_FOCUSEDLINE);
	MarkerDeleteAll(MARKER_MODIFIED);
	MarkerDeleteAll(MARKER_SAVED);

	if (m_project != NULL && m_project->IsSingleFile())
	{
		delete m_project;
	}

	m_project = new ProjectInfo();
}

//////////////////////////////////////////////////////////////////////////////
///  public OnRightClick
///  Handles a right-click in the editor
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnRightClick(wxMouseEvent& event)
{
	m_PopUpActive = true;
	m_lastRightClick = event.GetPosition();
	int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = LineFromPosition(charpos);

	bool breakpointOnLine = MarkerOnLine(linenum, MARKER_BREAKPOINT);
	bool bookmarkOnLine = MarkerOnLine(linenum, MARKER_BOOKMARK);

	bool breakpointsAllowed = (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC);
	//bool isDebugging = true; //m_debugManager->IsDebugging();//m_mainFrame->IsDebugging();

	if (m_popupMenu.FindItem(ID_DEBUG_DISPLAY_SELECTION) != nullptr)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		//m_popupMenu.Remove(ID_DEBUG_DISPLAY_SELECTION);
		m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, false);
		m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "..."));
	}
	if (m_popupMenu.FindItem(ID_FIND_PROCEDURE) != nullptr)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		m_popupMenu.Remove(ID_FIND_PROCEDURE);
	}
	if (m_popupMenu.FindItem(ID_FIND_INCLUDE) != nullptr)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		m_popupMenu.Remove(ID_FIND_INCLUDE);
	}
	if (m_popupMenu.FindItem(ID_MENU_HELP_ON_ITEM) != nullptr)
	{
		m_popupMenu.Remove(ID_MENU_HELP_ON_ITEM);
	}
	if (m_popupMenu.FindItem(ID_REFACTORING_MENU) != nullptr)
	{
	    m_refactoringMenu->Enable(ID_RENAME_SYMBOLS, false);
	    m_refactoringMenu->Enable(ID_ABSTRAHIZE_SECTION, false);
		m_popupMenu.Remove(ID_REFACTORING_MENU);
	}

	/*if(isDebugging)
	{
		breakpointsAllowed = m_debugManager->IsDebuggerPaused();//m_mainFrame->IsDebuggerPaused();
	}*/

	m_popupMenu.Enable(ID_FOLD_CURRENT_BLOCK, breakpointsAllowed);

	m_popupMenu.Enable(ID_DEBUG_ADD_BREAKPOINT, breakpointsAllowed && !breakpointOnLine);
	m_popupMenu.Enable(ID_DEBUG_REMOVE_BREAKPOINT, breakpointsAllowed && breakpointOnLine);

	m_popupMenu.Enable(ID_BOOKMARK_ADD, !bookmarkOnLine);
	m_popupMenu.Enable(ID_BOOKMARK_REMOVE, bookmarkOnLine);
	//m_popupMenu.Enable(ID_DEBUG_RUNTOCURSOR, breakpointsAllowed && isDebugging);

	// returns a copy of a member variable, which would seem sort of pointless, but
	// GetBreakpoints cleans up any stray marker IDs in the list before returning
	// so we have an accurate count of how many breakpoints there are
	wxArrayInt currentBreakpoints = GetBreakpoints();
	bool canClearBreakpoints = currentBreakpoints.GetCount() > 0;

	m_popupMenu.Enable(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, canClearBreakpoints);

	if (HasSelection())
	{
		m_popupMenu.Enable(ID_UPPERCASE, true);
		m_popupMenu.Enable(ID_LOWERCASE, true);
	}
	else
	{
		m_popupMenu.Enable(ID_UPPERCASE, false);
		m_popupMenu.Enable(ID_LOWERCASE, false);
	}

    wxString clickedWord = FindClickedWord();

    if (clickedWord.Length() > 0 || HasSelection())
    {
        if (this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES)
        {
            // Show "find procedure"
            wxString clickedProc = FindClickedProcedure();
            if (clickedProc.length())
            {
                m_popupMenu.Insert(15, m_menuFindProcedure);
                m_menuFindProcedure->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_FINDPROC", clickedProc.ToStdString()));
            }
        }
        else if (this->GetStyleAt(charpos) == wxSTC_NSCR_COMMAND
                || this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURE_COMMANDS
                || this->GetStyleAt(charpos) == wxSTC_NSCR_OPTION)
        {
            // Show "help on item"
            m_popupMenu.Insert(15, m_menuHelpOnSelection);
            m_menuHelpOnSelection->SetItemLabel(_guilang.get("GUI_TREE_PUP_HELPONITEM", clickedWord.ToStdString()));
        }
        else if (this->GetStyleAt(charpos) == wxSTC_NSCR_INCLUDES
                || this->GetStyleAt(charpos) == wxSTC_NPRC_INCLUDES)
        {
            // Show "find included file"
            wxString clickedInclude = FindClickedInclude();
            if (clickedInclude.length())
            {
                m_popupMenu.Insert(15, m_menuFindInclude);
                m_menuFindInclude->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_FINDINCLUDE", clickedInclude.ToStdString()));
            }
        }
        else
        {
            // Show the refactoring menu
            m_popupMenu.Insert(15, m_menuRefactoring);
            if (this->isStyleType(STYLE_DEFAULT, charpos) || this->isStyleType(STYLE_IDENTIFIER, charpos) || this->isStyleType(STYLE_CUSTOMFUNCTION, charpos))
                m_refactoringMenu->Enable(ID_RENAME_SYMBOLS, true);
            if (HasSelection())
                m_refactoringMenu->Enable(ID_ABSTRAHIZE_SECTION, true);
        }

        m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, true);
        m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", clickedWord.ToStdString()));
    }

	if (this->CallTipActive())
		this->AdvCallTipCancel();
	PopupMenu(&m_popupMenu, m_lastRightClick);
	m_PopUpActive = false;
}


// MARKER_MODIFIED
// MARKER_SAVED
void NumeReEditor::markModified(int nLine)
{
	int nMarkSaved = 1 << (MARKER_SAVED);
	int nMarkModified = 1 << (MARKER_MODIFIED);
	while (this->MarkerGet(nLine) & nMarkSaved)
		this->MarkerDelete(nLine, MARKER_SAVED);

	if (!(this->MarkerGet(nLine) & nMarkModified))
		this->MarkerAdd(nLine, MARKER_MODIFIED);
}

void NumeReEditor::markSaved()
{
	int nMarkModified = 1 << (MARKER_MODIFIED);
	int nMarkSaved = 1 << (MARKER_SAVED);
	int nNextLine = 0;
	while ((nNextLine = this->MarkerNext(0, nMarkModified)) != -1)
	{
		this->MarkerDelete(nNextLine, MARKER_MODIFIED);

		if (!(this->MarkerGet(nNextLine) & nMarkSaved))
			this->MarkerAdd(nNextLine, MARKER_SAVED);
	}
	this->MarkerDeleteAll(MARKER_MODIFIED);
}


//////////////////////////////////////////////////////////////////////////////
///  private OnEditorModified
///  Updates the editor's project when the editor is modified
///
///  @param  event wxStyledTextEvent & The generated editor event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnEditorModified(wxStyledTextEvent& event)
{
	m_project->SetCompiled(false);
	if (!m_bLoadingFile && (event.GetModificationType() & wxSTC_MOD_INSERTTEXT || event.GetModificationType() & wxSTC_MOD_DELETETEXT))
	{
		m_modificationHappened = true;
		int nLine = this->LineFromPosition(event.GetPosition());
		int nLinesAdded = event.GetLinesAdded();
		if (nLinesAdded > 0)
		{
			for (int i = 0; i < nLinesAdded; i++)
			{
				this->markModified(i + nLine);
			}
		}
		else if (nLinesAdded < 0)
		{
			this->markModified(nLine);
		}
		else
			this->markModified(nLine);
	}
	/*if (getEditorSetting(SETTING_USEANALYZER))
	    CallAfter(AnalyseCode);*/
	//CallAfter(markSections);
	event.Skip();
}

void NumeReEditor::FoldCurrentBlock(int nLine)
{
	// Get parent line
	int nParentline = this->GetFoldParent(nLine);
	// Probably the current line is also a parent line -> take this one
	if (this->GetFoldLevel(nLine) & wxSTC_FOLDLEVELHEADERFLAG)
		nParentline = nLine;
	// if not found -> return
	if (nParentline == -1)
		return;
	// if already folded -> return
	if (!this->GetFoldExpanded(nLine))
		return;
	// toggle the fold state of the current header line -> only folds because of previous condition
	this->ToggleFold(nParentline);
}

void NumeReEditor::markSections(bool bForceRefresh)
{
	if (!getEditorSetting(SETTING_USESECTIONS))
		return;
	if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB)
	{
		if (bForceRefresh)
			this->MarkerDeleteAll(MARKER_SECTION);
		int startline = 0;
		int endline = this->GetLineCount();

		if (!bForceRefresh)
		{
			int markermask = (1 << MARKER_SECTION);
			if ((startline = this->MarkerPrevious(this->GetCurrentLine() - 1, markermask)) == -1)
				startline = 0;
			if ((endline = this->MarkerNext(this->GetCurrentLine() + 1, markermask)) == -1)
				endline = this->GetLineCount();
		}

		for (int i = startline; i < endline; i++)
		{
			if (i && this->MarkerOnLine(i - 1, MARKER_SECTION))
				continue;

			for (int j = this->PositionFromLine(i); j < this->GetLineEndPosition(i) + 1; j++)
			{
				if (this->GetCharAt(j) == ' ' || this->GetCharAt(j) == '\t')
					continue;
				if (isStyleType(STYLE_COMMENT_SECTION_LINE, j)
						|| isStyleType(STYLE_COMMENT_SECTION_BLOCK, j))
				{
					if (!this->MarkerOnLine(i, MARKER_SECTION))
						this->MarkerAdd(i, MARKER_SECTION);
					break;
				}
				// only section markers which are the first characters in line are interpreted
				if (this->GetCharAt(j) != ' ' && this->GetCharAt(j) != '\t')
				{
					if (this->MarkerOnLine(i, MARKER_SECTION))
						this->MarkerDelete(i, MARKER_SECTION);
					break;
				}
			}
		}
	}
	if (m_fileType == FILE_TEXSOURCE)
	{
		if (bForceRefresh)
			this->MarkerDeleteAll(MARKER_SECTION);
		int startline = 0;
		int endline = this->GetLineCount();

		if (!bForceRefresh)
		{
			int markermask = (1 << MARKER_SECTION);
			if ((startline = this->MarkerPrevious(this->GetCurrentLine() - 1, markermask)) == -1)
				startline = 0;
			if ((endline = this->MarkerNext(this->GetCurrentLine() + 1, markermask)) == -1)
				endline = this->GetLineCount();
		}

		for (int i = startline; i < endline; i++)
		{
			for (int j = this->PositionFromLine(i); j < this->GetLineEndPosition(i) + 1; j++)
			{
				if (this->GetCharAt(j) == ' ' || this->GetCharAt(j) == '\t')
					continue;
				if (this->GetStyleAt(j) == wxSTC_TEX_COMMAND)
				{
					int wordstart = this->WordStartPosition(j, false);
					int wordend = this->WordEndPosition(j, false);

					wxString word = this->GetTextRange(wordstart, wordend);
					if (word == "maketitle"
							|| word == "part"
							|| word == "chapter"
							|| word == "section"
							|| word == "subsection"
							|| word == "subsubsection"
							|| word == "subsubsubsection"
							|| word == "paragraph"
							|| word == "subparagraph"
							|| word == "addchap"
							|| word == "addsec")
					{
						if (!this->MarkerOnLine(i, MARKER_SECTION))
							this->MarkerAdd(i, MARKER_SECTION);
					}
					j = wordend;
				}
			}
		}
	}
	if (m_fileType == FILE_NONSOURCE && this->getEditorSetting(SETTING_USETXTADV))
	{
		if (bForceRefresh)
			this->MarkerDeleteAll(MARKER_SECTION);
		int startline = 0;
		int endline = this->GetLineCount();

		if (!bForceRefresh)
		{
			int markermask = (1 << MARKER_SECTION);
			if ((startline = this->MarkerPrevious(this->GetCurrentLine() - 1, markermask)) == -1)
				startline = 0;
			if ((endline = this->MarkerNext(this->GetCurrentLine() + 1, markermask)) == -1)
				endline = this->GetLineCount();
		}

		for (int i = startline; i < endline; i++)
		{
			for (int j = this->PositionFromLine(i); j < this->GetLineEndPosition(i) + 1; j++)
			{
				if (this->GetCharAt(j) == ' ' || this->GetCharAt(j) == '\t')
					continue;
				if (this->GetStyleAt(j) == wxSTC_TXTADV_BIGHEAD || this->GetStyleAt(j) == wxSTC_TXTADV_HEAD)
				{
					if (!this->MarkerOnLine(i, MARKER_SECTION))
						this->MarkerAdd(i, MARKER_SECTION);
					break;
				}
			}
			if (this->GetLine(i).find('#') == string::npos)
			{
				if (this->MarkerOnLine(i, MARKER_SECTION))
					this->MarkerDelete(i, MARKER_SECTION);
			}
		}
	}
}

void NumeReEditor::AsynchActions()
{
	if (!this->AutoCompActive() && this->getEditorSetting(SETTING_INDENTONTYPE) && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB || m_fileType == FILE_CPP))
		ApplyAutoIndentation(0, this->GetCurrentLine() + 1);
	HandleFunctionCallTip();
}

void NumeReEditor::AsynchEvaluations()
{
	if (getEditorSetting(SETTING_USEANALYZER))
		AnalyseCode();
	markSections();
}

void NumeReEditor::OnStartDrag(wxStyledTextEvent& event)
{
	wxString gtxt = event.GetDragText();
}

void NumeReEditor::OnDragOver(wxStyledTextEvent& event)
{
	event.SetDragResult(wxDragMove);
	event.Skip();
}

void NumeReEditor::OnDrop(wxStyledTextEvent& event)
{
	event.Skip();
}

void NumeReEditor::OnMouseMotion(wxMouseEvent& event)
{
	if (m_dragging)
		DoDragOver(event.GetX(), event.GetY(), wxDragMove);
	event.Skip();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnAddBreakpoint
///  Adds a breakpoint to this file
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnAddBreakpoint(wxCommandEvent& event)
{
	//int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = GetLineForMarkerOperation(); //LineFromPosition(charpos);

	AddBreakpoint(linenum);

	ResetRightClickLocation();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnRemoveBreakpoint
///  Removes a breakpoint from this file
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @remarks This doesn't clean out the marker handle that STC gives us,
///  @remarks since there's no way to check what marker handles are on a given line.
///  @remarks Orphaned marker handles are cleaned up in GetBreakpoints.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnRemoveBreakpoint(wxCommandEvent& event)
{
	int linenum = GetLineForMarkerOperation();

	RemoveBreakpoint(linenum);


	ResetRightClickLocation();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnClearBreakpoints
///  Clears all breakpoints from this file
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnClearBreakpoints(wxCommandEvent& event)
{
	// m_breakpoints should have been cleared of any orphaned marker
	// handles during the right-click that led us here
	//int numBreakpoints = m_breakpoints.GetCount();
	int numBreakpoints = GetBreakpoints().GetCount();

	for (int i = 0; i < numBreakpoints; i++)
	{
		int markerHandle = m_breakpoints[i];
		int linenum = this->MarkerLineFromHandle(markerHandle);
		this->MarkerDeleteHandle(markerHandle);
		CreateBreakpointEvent(linenum, false);
	}
	m_terminal->_guimessenger.clearBreakpoints(GetFileNameAndPath().ToStdString());

	ResetRightClickLocation();
}

void NumeReEditor::OnAddBookmark(wxCommandEvent& event)
{
	int nLineNumber = GetLineForMarkerOperation();
	this->MarkerAdd(nLineNumber, MARKER_BOOKMARK);
	ResetRightClickLocation();
}
void NumeReEditor::OnRemoveBookmark(wxCommandEvent& event)
{
	int nLineNumber = GetLineForMarkerOperation();
	this->MarkerDelete(nLineNumber, MARKER_BOOKMARK);
}
void NumeReEditor::OnClearBookmarks(wxCommandEvent& event)
{
	this->MarkerDeleteAll(MARKER_BOOKMARK);
}


//////////////////////////////////////////////////////////////////////////////
///  public GetBreakpoints
///  Gets a list of all breakpoint line numbers.  Also clears out any invalid (removed) breakpoint IDs.
///
///  @return wxArrayInt The line numbers for all the breakpoints in this file
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxArrayInt NumeReEditor::GetBreakpoints()
{
	wxArrayInt linenumbers;
	wxArrayInt invalidBreakpoints;

	int numStoredBreakpoints = m_breakpoints.GetCount();
	for (int i = 0; i < numStoredBreakpoints; i++)
	{
		int markerHandle = m_breakpoints[i];

		int linenum = this->MarkerLineFromHandle(markerHandle);

		if (linenum != -1)
		{
			linenumbers.Add(linenum + 1);
		}
		else
		{
			invalidBreakpoints.Add(markerHandle);
		}
	}

	for (int i = 0; i < (int)invalidBreakpoints.GetCount(); i++)
	{
		m_breakpoints.Remove(invalidBreakpoints[i]);
	}

	linenumbers.Sort((CMPFUNC_wxArraywxArrayInt)CompareInts);
	return linenumbers;
}

//////////////////////////////////////////////////////////////////////////////
///  public HasBeenCompiled
///  Returns the compiled status for this editor's project
///
///  @return bool Whether or not the editor's project has been compiled
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReEditor::HasBeenCompiled()
{
	return m_project->IsCompiled();
}

//////////////////////////////////////////////////////////////////////////////
///  public SetCompiled
///  Set this editor's project's compiled status
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::SetCompiled()
{
	m_project->SetCompiled(true);
}

//////////////////////////////////////////////////////////////////////////////
///  public FocusOnLine
///  Moves the cursor to the given line number, optionally showing a highlight marker
///
///  @param  linenumber int   The line to go to
///  @param  showMarker bool  [=true] Whether or not to mark the line
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::FocusOnLine(int linenumber, bool showMarker)
{
	GotoLine(linenumber);
	EnsureVisible(linenumber);
	if (showMarker)
	{
		MarkerDeleteAll(MARKER_FOCUSEDLINE);
		MarkerAdd(linenumber, MARKER_FOCUSEDLINE);
	}
}
//////////////////////////////////////////////////////////////////////////////
///  private CreateBreakpointEvent
///  Sets up a debug event when a breakpoint is added or deleted
///
///  @param  linenumber    int   The line number of the toggled breakpoint
///  @param  addBreakpoint bool  Whether the breakpoint is being added or deleted
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::CreateBreakpointEvent(int linenumber, bool addBreakpoint)
{
	wxDebugEvent dbg;
	wxString filename = m_fileNameAndPath.GetFullPath(wxPATH_UNIX);
	//wxArrayString sources;
	//sources.Add(filename);
	dbg.SetSourceFilename(filename);
	// adjust for the zero-based index
	dbg.SetLineNumber(linenumber + 1);
	int type = addBreakpoint ? ID_DEBUG_ADD_BREAKPOINT : ID_DEBUG_REMOVE_BREAKPOINT;
	dbg.SetId(type);
	dbg.SetId(type);
	dbg.SetProject(m_project);
	//m_mainFrame->AddPendingEvent(dbg);
}


//////////////////////////////////////////////////////////////////////////////
///  public SetProject
///  Sets the project for this editor
///
///  @param  project ProjectInfo * The new project
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::SetProject(ProjectInfo* project)
{
	if (m_project != NULL && m_project->IsSingleFile())
	{
		delete m_project;
	}

	m_project = project;
	m_project->AddEditor(this);
}

//////////////////////////////////////////////////////////////////////////////
///  private OnRunToCursor
///  Creates a "one-shot" breakpoint and tells the debugger to run to that line
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReEditor::OnRunToCursor(wxCommandEvent& event)
{
	/*
	int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = LineFromPosition(charpos);
	// adjust for Scintilla's internal zero-based line numbering
	linenum++;
	*/

	/*int linenum = GetLineForBreakpointOperation();
	wxDebugEvent debugEvent;
	debugEvent.SetId(ID_DEBUG_RUNTOCURSOR);
	debugEvent.SetSourceFilename(GetFilenameString());
	debugEvent.SetLineNumber(linenum);
	m_debugManager->AddPendingEvent(debugEvent);//m_mainFrame->AddPendingEvent(debugEvent);*/

	ResetRightClickLocation();
}

int NumeReEditor::GetLineForMarkerOperation()
{
	int lineNum = 0;

	if (m_lastRightClick.x < 0 || m_lastRightClick.y < 0)
	{
		lineNum =  GetCurrentLine();
	}
	else
	{
		int charpos = PositionFromPoint(m_lastRightClick);
		lineNum = LineFromPosition(charpos);
		//lineNum++;
	}


	return lineNum;
}

void NumeReEditor::ResetRightClickLocation()
{
	m_lastRightClick.x = -1;
	m_lastRightClick.y = -1;
}

wxString NumeReEditor::FindClickedWord()
{
	int charpos = PositionFromPoint(m_lastRightClick);
	int startPosition = WordStartPosition(charpos, true);
	int endPosition = WordEndPosition(charpos, true);

	wxString clickedWord = this->GetTextRange(startPosition, endPosition);
	m_clickedWordLength = endPosition - startPosition;
	m_clickedWord = clickedWord;
	return clickedWord;
}


wxString NumeReEditor::FindClickedInclude()
{
	int charpos = PositionFromPoint(m_lastRightClick);

	int startPosition = WordStartPosition(charpos, true);
	while (startPosition && GetStyleAt(startPosition - 1) == wxSTC_NSCR_INCLUDES && GetCharAt(startPosition - 1) != '@')
		startPosition--;
	if (GetCharAt(startPosition) == '"')
		startPosition++;
	int endPosition = WordEndPosition(startPosition + 1, true);
	while (endPosition < GetLastPosition() && GetStyleAt(endPosition) == wxSTC_NSCR_INCLUDES && GetCharAt(endPosition) != ':' && GetCharAt(endPosition) != '"')
		endPosition++;

	wxString clickedWord = this->GetTextRange(startPosition, endPosition);

	if (clickedWord.find('<') != string::npos)
	{
		if (clickedWord.find("<>") != string::npos)
			clickedWord.replace(clickedWord.find("<>"), 2, m_terminal->getPathSettings()[EXEPATH]);
		if (clickedWord.find("<this>") != string::npos)
			clickedWord.replace(clickedWord.find("<this>"), 6, this->GetFileName().GetPath(wxPATH_GET_VOLUME));
		if (clickedWord.find("<loadpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<loadpath>"), 10, m_terminal->getPathSettings()[LOADPATH]);
		if (clickedWord.find("<savepath>") != string::npos)
			clickedWord.replace(clickedWord.find("<savepath>"), 10, m_terminal->getPathSettings()[SAVEPATH]);
		if (clickedWord.find("<scriptpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<scriptpath>"), 12, m_terminal->getPathSettings()[SCRIPTPATH]);
		if (clickedWord.find("<procpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<procpath>"), 10, m_terminal->getPathSettings()[PROCPATH]);
		if (clickedWord.find("<plotpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<plotpath>"), 10, m_terminal->getPathSettings()[PLOTPATH]);
	}

	if (clickedWord.find('/') != string::npos || clickedWord.find('\\') != string::npos)
	{
		m_clickedInclude = clickedWord + ".nscr";
	}
	else
	{
		m_clickedInclude = m_terminal->getPathSettings()[SCRIPTPATH] + "/" + clickedWord + ".nscr";
	}

	return replacePathSeparator(clickedWord.ToStdString());
}

wxString NumeReEditor::FindClickedProcedure()
{
	int charpos = PositionFromPoint(m_lastRightClick);
	return FindMarkedProcedure(charpos);
}

wxString NumeReEditor::FindMarkedProcedure(int charpos)
{
	int startPosition = WordStartPosition(charpos, true);
	while (startPosition && GetStyleAt(startPosition - 1) == wxSTC_NSCR_PROCEDURES)
		startPosition--;
	int endPosition = WordEndPosition(charpos, true);
	while (endPosition < GetLastPosition() && GetStyleAt(endPosition) == wxSTC_NSCR_PROCEDURES)
		endPosition++;

	wxString currentline = this->GetLine(LineFromPosition(startPosition));
	if (currentline.find("procedure") != string::npos && currentline[currentline.find_first_not_of(' ', currentline.find("procedure") + 9)] == '$')
		return "";

	wxString clickedWord = this->GetTextRange(startPosition, endPosition);

	if (m_fileType == FILE_NPRC && clickedWord.find('~') == string::npos)
	{
		wxString sNameSpace = FindNameSpaceOfProcedure(charpos);
		if (sNameSpace.length())
		{
			if (clickedWord[0] == '$')
				clickedWord.insert(1, sNameSpace + "~");
			else
				clickedWord = "$" + sNameSpace + "~" + clickedWord;
		}
	}

	m_clickedProcedure = clickedWord;

	if (clickedWord.find('~') != string::npos)
		clickedWord.erase(1, clickedWord.rfind('~'));
	if (clickedWord[0] != '$')
		clickedWord.insert(0, 1, '$');

	return clickedWord + "()";
}

wxString NumeReEditor::FindNameSpaceOfProcedure(int charpos)
{
	wxString sNameSpace = "";
	if (m_fileType == FILE_NPRC)
	{
		int minpos = 0;
		int maxpos = charpos;
		int nextpos = 0;
		while (minpos < charpos
				&& nextpos < charpos
				&& (nextpos = FindText(nextpos, maxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != -1)
		{
			if (nextpos == -1)
				break;
			nextpos++;
			if (this->GetStyleAt(nextpos) == wxSTC_NSCR_COMMAND)
				minpos = nextpos;
		}
		if (FindText(minpos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
		{
			while (minpos < charpos && FindText(minpos, maxpos, "namespace", wxSTC_FIND_WHOLEWORD | wxSTC_FIND_MATCHCASE) != -1)
			{
				int nCurrentPos = FindText(minpos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;
				while (this->GetStyleAt(nCurrentPos) != wxSTC_NPRC_COMMAND && FindText(nCurrentPos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
					nCurrentPos = FindText(nCurrentPos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;
				if (this->GetStyleAt(nCurrentPos) == wxSTC_NPRC_COMMAND)
					minpos = nCurrentPos;
				else
					break;
			}
			string currentNamespace = GetLine(LineFromPosition(minpos)).ToStdString();
			if (currentNamespace.find("namespace") != string::npos)
			{
				currentNamespace.erase(0, currentNamespace.find("namespace") + 9);
				while (currentNamespace.back() == '\r' || currentNamespace.back() == '\n')
					currentNamespace.pop_back();
				StripSpaces(currentNamespace);
				while (currentNamespace.back() == '~')
					currentNamespace.pop_back();

				sNameSpace = currentNamespace;
			}
		}
	}
	return sNameSpace;
}

wxString NumeReEditor::FindProceduresInCurrentFile(wxString sFirstChars, wxString sSelectedNamespace)
{
	wxString sThisFileProcedures;
	for (int i = 0; i < this->GetLineCount(); i++)
	{
		wxString currentline = this->GetLine(i);
		if (currentline.find("procedure") != string::npos
				&& currentline.find('$', currentline.find("procedure")) != string::npos
				&& this->GetStyleAt(this->PositionFromLine(i) + currentline.find("procedure")) != wxSTC_NSCR_COMMENT_BLOCK
				&& this->GetStyleAt(this->PositionFromLine(i) + currentline.find("procedure")) != wxSTC_NSCR_COMMENT_LINE)
		{
			currentline.erase(0, currentline.find('$') + 1);
			if (currentline.find('(') == string::npos)
				continue;
			currentline.erase(currentline.find('('));
			if (currentline.substr(0, sFirstChars.length()) == sFirstChars)
				sThisFileProcedures += currentline + "(?" + toString(NumeReSyntax::SYNTAX_PROCEDURE) + " ";
		}
	}
	if (sSelectedNamespace.length())
		return sThisFileProcedures;
	return sThisFileProcedures + _syntax->getNameSpaceAutoCompList(sFirstChars.ToStdString());
}

// This member function searches for the definition of
// the procedure, which is currently below the cursor.
wxString NumeReEditor::FindProcedureDefinition()
{
    // do nothing, if there's no currently selected procedure
	if (!m_clickedProcedure.length())
		return "";
	vector<std::string> vPaths = m_terminal->getPathSettings();
	wxString pathname = m_clickedProcedure;
	wxString procedurename = pathname.substr(pathname.rfind('~') + 1); // contains a "$", if it's not used for the "thisfile~" case

	// Handle the namespaces
	if (pathname.find("$this~") != string::npos)
	{
	    // This namespace (the current folder)
		wxString thispath = GetFileNameAndPath();
		pathname.replace(pathname.find("$this~"), 6, thispath.substr(0, thispath.rfind('\\') + 1));
	}
	else if (pathname.find("$thisfile~") != string::npos)
	{
	    // local namespace
		return FindProcedureDefinitionInLocalFile(procedurename);
	}
	else
	{
	    // All other namespaces
		if (pathname.find("$main~") != string::npos)
			pathname.erase(pathname.find("$main~") + 1, 5);

		while (pathname.find('~') != string::npos)
			pathname[pathname.find('~')] = '/';

		// Add the root folders to the path name
		if (pathname[0] == '$' && pathname.find(':') == string::npos)
			pathname.replace(0, 1, vPaths[PROCPATH] + "/");
		else if (pathname.find(':') == string::npos)
			pathname.insert(0, vPaths[PROCPATH]);
		else // pathname.find(':') != string::npos
		{
		    // Absolute file paths
			pathname = pathname.substr(pathname.find('\'') + 1, pathname.rfind('\'') - pathname.find('\'') - 1);
		}
	}

	// Find the namespace in absolute procedure paths
	while (procedurename.find('\'') != string::npos)
		procedurename.erase(procedurename.find('\''), 1);
	if (procedurename.find('/') != string::npos)
		procedurename = "$" + procedurename.substr(procedurename.rfind('/') + 1);
	if (procedurename.find('\\') != string::npos)
		procedurename = "$" + procedurename.substr(procedurename.rfind('\\') + 1);
	if (procedurename[0] != '$')
		procedurename.insert(0, 1, '$');

	// Find procedure in a global procedure file
	return FindProcedureDefinitionInOtherFile(pathname, procedurename);
}

// This private member function searches for the procedure
// definition in the currently opened procedure file
wxString NumeReEditor::FindProcedureDefinitionInLocalFile(const wxString& procedurename)
{
    wxString procedureline;
    int nminpos = 0;
    int nmaxpos = GetLastPosition();

    // Force Scintilla to style the whole document
    if (GetLastPosition() > GetEndStyled() && !GetWrapMode())
    {
        SetWrapMode(wxSTC_WRAP_WORD);
        SetWrapMode(wxSTC_WRAP_NONE);
    }

    while (nminpos < nmaxpos && FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
    {
        nminpos = FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;

        // Ignore comments
        if (this->GetStyleAt(nminpos) == wxSTC_NSCR_COMMENT_BLOCK || this->GetStyleAt(nminpos) == wxSTC_NSCR_COMMENT_LINE)
            continue;

        procedureline = GetLine(LineFromPosition(nminpos));

        if (procedureline.find("$" + procedurename) != string::npos && procedureline[procedureline.find_first_not_of(' ', procedureline.find("$" + procedurename) + procedurename.length() + 1)] == '(')
        {
            if (getMatchingParenthesis(procedureline.substr(procedureline.find("$" + procedurename)).ToStdString()) == string::npos)
                return "";

            // Extraxt the procedure definition
            string sProcDef = procedureline.substr(procedureline.find("$" + procedurename), getMatchingParenthesis(procedureline.substr(procedureline.find("$" + procedurename)).ToStdString()) + 1).ToStdString();
            size_t nFirstParens = sProcDef.find('(');
            string sArgList = sProcDef.substr(nFirstParens + 1, getMatchingParenthesis(sProcDef.substr(nFirstParens)) - 1);
            sProcDef.erase(nFirstParens + 1);

            // Handle the argument list
            while (sArgList.length())
            {
                string currentarg = getNextArgument(sArgList, true);
                if (currentarg.front() == '_')
                    currentarg.erase(0, 1);
                sProcDef += currentarg;
                if (sArgList.length())
                    sProcDef += ", ";
            }

            sProcDef += ") :: local";

            // Handle the flags
            if (procedureline.find("::") != string::npos)
            {
                string sFlags = procedureline.substr(procedureline.find("::") + 2).ToStdString();
                if (sFlags.find("##") != string::npos)
                    sFlags.erase(sFlags.find("##"));
                if (sFlags.find_first_of("\r\n") != string::npos)
                    sFlags.erase(sFlags.find_first_of("\r\n"));
                StripSpaces(sFlags);
                sProcDef += " " + sFlags;
            }

            wxString sDocumentation;

            // Find now the documentations - documentation lines above
            // the current line are preferred:
            for (int docline = LineFromPosition(nminpos)-1; docline >= 0; docline--)
            {
                if (!isStyleType(STYLE_COMMENT_BLOCK, GetLineIndentPosition(docline))
                    && !isStyleType(STYLE_COMMENT_SECTION_LINE, GetLineIndentPosition(docline))
                    && !isStyleType(STYLE_COMMENT_SECTION_BLOCK, GetLineIndentPosition(docline)))
                {
                    if (docline < LineFromPosition(nminpos)-1)
                    {
                        for (int curline = docline+1; curline < LineFromPosition(nminpos); curline++)
                        {
                            wxString curdocline = GetLine(curline);
                            curdocline.erase(0, curdocline.find_first_not_of(" \t#*!"));
                            curdocline.erase(curdocline.find_first_of("\r\n"));
                            if (curdocline.find("*#") != string::npos)
                                curdocline.erase(curdocline.find("*#"));

                            AppendToDocumentation(sDocumentation, curdocline);
                        }
                    }
                    break;
                }
            }

            // If the documentation string is still empty
            // search below the definition
            if (!sDocumentation.length())
            {
                for (int docline = LineFromPosition(nminpos)+1; docline < GetLineCount(); docline++)
                {
                    if (!isStyleType(STYLE_COMMENT_BLOCK, GetLineIndentPosition(docline))
                        && !isStyleType(STYLE_COMMENT_SECTION_LINE, GetLineIndentPosition(docline))
                        && !isStyleType(STYLE_COMMENT_SECTION_BLOCK, GetLineIndentPosition(docline)))
                    {
                        if (docline > LineFromPosition(nminpos)+1)
                        {
                            for (int curline = LineFromPosition(nminpos)+1; curline < docline; curline++)
                            {
                                wxString curdocline = GetLine(curline);
                                curdocline.erase(0, curdocline.find_first_not_of(" \t#*!"));
                                curdocline.erase(curdocline.find_first_of("\r\n"));

                                AppendToDocumentation(sDocumentation, curdocline);
                            }
                        }
                        break;
                    }
                }
            }

            // clean the documentation
            sDocumentation = CleanDocumentation(sDocumentation);

            // Append the documentation if it is present
            if (sDocumentation.length())
                sProcDef += "\n" + sDocumentation.ToStdString();

            return sProcDef;
        }
    }
    return "";
}

// This private member function searches for the procedure
// definition in a selected global procedure file
wxString NumeReEditor::FindProcedureDefinitionInOtherFile(const wxString& pathname, const wxString& procedurename)
{
	if (!fileExists((pathname + ".nprc").ToStdString()))
	{
		return "";
	}
	else
	{
		ifstream procedure_in;
		string sProcCommandLine;
		bool bBlockComment = false;
		wxString sDocumentation;
		bool bDocFound = false;
		procedure_in.open((pathname + ".nprc").c_str());

		// Ensure that the file is in good state
		if (!procedure_in.good())
			return "";

		// As long as we're not at the end of the file
		while (!procedure_in.eof())
		{
		    // Read one line and strip all spaces
			getline(procedure_in, sProcCommandLine);
			StripSpaces(sProcCommandLine);

			// Ignore empty lines
			if (!sProcCommandLine.length())
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

            // Ignore comment lines
			if (sProcCommandLine.substr(0, 2) == "##")
            {
                // Append each documentation string
                if (sProcCommandLine.substr(0, 3) == "##!")
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                }
				continue;
            }

            // Erase line comment parts
			if (sProcCommandLine.find("##") != string::npos)
				sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));

			// Remove block comments and continue
			if (sProcCommandLine.substr(0, 2) == "#*" && sProcCommandLine.find("*#", 2) == string::npos)
			{
			    if (sProcCommandLine.substr(0, 3) == "#*!")
                {
                    bDocFound = true;
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                }
				bBlockComment = true;
				continue;
			}

			// Search for the end of the current block comment
			if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
			{
				bBlockComment = false;
				if (bDocFound)
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(0, sProcCommandLine.find("*#")));
                }
				if (sProcCommandLine.find("*#") == sProcCommandLine.length() - 2)
				{
					continue;
				}
				else
					sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#") + 2);
			}
			else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
			{
			    // if the documentation has a length, append the current block
			    if (bDocFound)
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine);
                }
				continue;
			}

			// Ignore includes
			if (sProcCommandLine[0] != '@' && findCommand(sProcCommandLine).sString != "procedure")
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }
			else if (sProcCommandLine[0] == '@')
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

			// Ignore lines without "procedure"
			if (findCommand(sProcCommandLine).sString != "procedure")
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

			// Search for the current procedure name
			if (sProcCommandLine.find(procedurename.ToStdString()) == string::npos || sProcCommandLine.find('(') == string::npos)
            {
                // clear the documentation string
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }
			else
			{
                // Found the procedure name, now extract the definition
				if (getMatchingParenthesis(sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()))) == string::npos)
					return "";
				string sProcDef = sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()), getMatchingParenthesis(sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()))) + 1);
				size_t nFirstParens = sProcDef.find('(');
				string sArgList = sProcDef.substr(nFirstParens + 1, getMatchingParenthesis(sProcDef.substr(nFirstParens)) - 1);
				sProcDef.erase(nFirstParens + 1);
				while (sArgList.length())
				{
					string currentarg = getNextArgument(sArgList, true);
					if (currentarg.front() == '_')
						currentarg.erase(0, 1);
					sProcDef += currentarg;
					if (sArgList.length())
						sProcDef += ", ";
				}
				sProcDef += ")";

				if (sProcCommandLine.find("::") != string::npos)
				{
					string sFlags = sProcCommandLine.substr(sProcCommandLine.find("::") + 2).c_str();
					if (sFlags.find("##") != string::npos)
						sFlags.erase(sFlags.find("##"));
					StripSpaces(sFlags);
					sProcDef += " :: " + sFlags;
				}

				// If no documentation was found, search in the following lines
				if (!sDocumentation.length())
                {
                    while (!procedure_in.eof())
                    {
                        getline(procedure_in, sProcCommandLine);
                        StripSpaces(sProcCommandLine);
                        if (sProcCommandLine.substr(0, 3) == "##!")
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                        }
                        else if (sProcCommandLine.substr(0, 3) == "#*!")
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                            bBlockComment = true;
                        }
                        else if (bBlockComment)
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(0, sProcCommandLine.find("*#")));
                            if (sProcCommandLine.find("*#") != string::npos)
                                break;
                        }
                        else
                            break;
                    }
                }

                // clean the documentation
                sDocumentation = CleanDocumentation(sDocumentation);

				// if the documentation has a length, append it here
				if (sDocumentation.length())
                    sProcDef += "\n" + sDocumentation.ToStdString();
				return sProcDef;
			}
		}
	}
    return "";
}

// This member function appends a found documentation line to the overall
// documentation
void NumeReEditor::AppendToDocumentation(wxString& sDocumentation, const wxString& sNewDocLine)
{
    static bool bBeginEnd = false;

    if (sNewDocLine.find("\\begin{") != string::npos && sNewDocLine.find("\\end{") == string::npos)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";
        bBeginEnd = true;
    }
    else if (sNewDocLine.find("\\begin{") == string::npos && sNewDocLine.find("\\end{") != string::npos)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";
        bBeginEnd = false;
    }
    else if ((sNewDocLine.length() && sNewDocLine.substr(0, 2) == "- ") || bBeginEnd)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";
    }
    else
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != ' ')
            sDocumentation += " ";
    }
    sDocumentation += sNewDocLine;
}

// This member function checks the layout of the found documentations
// and applies some special modifications
string NumeReEditor::CleanDocumentation(const wxString& __sDoc)
{
    string sDocumentation = __sDoc.ToStdString();
    if (sDocumentation.find_first_not_of(" \n") != string::npos)
    {
        // Clean whitespace before and after the documentation
        sDocumentation.erase(0, sDocumentation.find_first_not_of(" \n"));
        if (sDocumentation.back() == ' ' || sDocumentation.back() == '\n')
        {
            sDocumentation.erase(sDocumentation.find_last_not_of(" \n")+1);
        }

        // Remove doubled exclamation marks
        while (sDocumentation.find("!!") != string::npos)
            sDocumentation.erase(sDocumentation.find("!!"), 2);

        // Replace \begin{} and \end{} with line breaks
        // This logic bases upon the replacements done
        // in NumeReEditor::AppendToDocumentation
        size_t nMatch = 0;
        while ((nMatch = sDocumentation.find("\\begin{")) != string::npos)
        {
            sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch);
            if (sDocumentation.substr(nMatch, 5) == "\n    ")
                sDocumentation.erase(nMatch, 5);
        }
        while ((nMatch = sDocumentation.find("\\end{")) != string::npos)
        {
            sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch + 1);
        }

        // Check the length of the line
        sDocumentation = addLinebreaks(sDocumentation, true);
    }
    else
        sDocumentation.clear();

    return sDocumentation;
}

// This member function identifies the position of the procedure
// head, to which the selected position belongs.
int NumeReEditor::FindCurrentProcedureHead(int pos)
{
    int minpos = 0;
    int maxpos = pos;
    int nextpos = 0;

    // Select the correct headline command
    wxString sProcCommand = "procedure";
    if (m_fileType == FILE_MATLAB)
        sProcCommand = "function";

    // Start from the top while searching for the next
    // procedure head
    while (minpos < pos
            && nextpos < pos
            && (nextpos = FindText(nextpos, maxpos, sProcCommand, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != -1)
    {
        if (nextpos == -1)
            break;
        nextpos++;

        // Ensure that the current position is styled as
        // a command
        if (this->isStyleType(STYLE_COMMAND, nextpos))
            minpos = nextpos;
    }
    return minpos;
}

int NumeReEditor::FindNamingProcedure()
{
	wxString sNamingProcedure = "$" + this->GetFilenameString();
	if (sNamingProcedure.find('.') != string::npos)
		sNamingProcedure.erase(sNamingProcedure.find('.'));
	sNamingProcedure += "(";
	for (int i = 0; i < this->LineFromPosition(this->GetLastPosition()); i++)
	{
		wxString currentline = this->GetLine(i);
		if (currentline.find("procedure") != string::npos && currentline.find(sNamingProcedure) != string::npos)
		{
			int linepos = this->PositionFromLine(i);
			int offset = 0;
			while (currentline.find("procedure", offset) != string::npos && this->GetStyleAt(linepos + currentline.find("procedure", offset) + 1) != wxSTC_NPRC_COMMAND)
				offset = currentline.find("procedure", offset) + 10;

			if (currentline.find("procedure", offset) != string::npos && this->GetStyleAt(linepos + currentline.find("procedure", offset) + 1) == wxSTC_NPRC_COMMAND)
			{
				int procloc = currentline.find("procedure", offset) + 9;
				while (currentline.find(sNamingProcedure, procloc) != string::npos && this->GetStyleAt(linepos + currentline.find(sNamingProcedure, procloc) + 1) != wxSTC_NPRC_PROCEDURES)
					procloc = currentline.find(sNamingProcedure, procloc) + 1 + sNamingProcedure.length();

				if (currentline.find(sNamingProcedure, procloc) != string::npos && this->GetStyleAt(linepos + currentline.find(sNamingProcedure) + 1) == wxSTC_NPRC_PROCEDURES)
					return i;
			}
		}
	}
	return wxNOT_FOUND;
}

// This member function detects all occurences of a code symbol
// between the passed positions. It does take the current style
// into account and returns the matches as a vector
vector<int> NumeReEditor::FindAll(const wxString& sSymbol, int nStyle, int nStartPos, int nEndPos)
{
    vector<int> vMatches;
    int nCurrentPos = 0;

    // Change the style of the string parser to the identifier
    // style
    if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC) && nStyle == wxSTC_NSCR_STRING_PARSER)
        nStyle = wxSTC_NSCR_IDENTIFIER;

    // Search the next occurence
    while ((nCurrentPos = this->FindText(nStartPos, nEndPos, sSymbol, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != wxSTC_INVALID_POSITION)
    {
        nStartPos = nCurrentPos+1;

        // Is it the correct style and no field of a structure?
        if ((this->GetCharAt(nCurrentPos-1) != '.' && this->GetStyleAt(nCurrentPos) == nStyle)
            || (this->GetStyleAt(nCurrentPos) == wxSTC_NSCR_STRING_PARSER && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)))
            vMatches.push_back(nCurrentPos);
    }

    // return the found matches
    return vMatches;
}

// This member function replaces the found matches with a new
// symbol
void NumeReEditor::ReplaceMatches(const vector<int>& vMatches, const wxString& sSymbol, const wxString& sNewSymbol)
{
    // During the replacement, the positions are moving
    // this variable tracks the differences
    int nInc = sNewSymbol.length() - sSymbol.length();

    // Do nothing, if no match was found
    if (!vMatches.size())
        return;

    // replace every match with the new symbol name
    for (size_t i = 0; i < vMatches.size(); i++)
    {
        this->Replace(vMatches[i] + i*nInc, vMatches[i]+sSymbol.length() + i*nInc, sNewSymbol);
    }
}

// This member function replaces the selected symbol with a
// new name supplied by the user. This method also supplies the
// needed user interaction
void NumeReEditor::RenameSymbols(int nPos)
{
    wxString sNewName;
    wxString sCurrentName;

    int nStartPos = 0;
    int nEndPos = this->GetLastPosition();

    // Find the current symbol's name and ensure that it
    // exists
    sCurrentName = this->GetTextRange(this->WordStartPosition(nPos, true), this->WordEndPosition(nPos, true));
    if (!sCurrentName.length())
        return;

    // Prepare and show the text entry dialog, so that the
    // user may supply a new symbol name
    wxTextEntryDialog textdialog(this, _guilang.get("GUI_DLG_RENAMESYMBOLS_QUESTION"), _guilang.get("GUI_DLG_RENAMESYMBOLS"), sCurrentName);
    int retval = textdialog.ShowModal();
    if (retval == wxID_CANCEL)
        return;

    // Get the new symbol name and ensure that it
    // exists
    sNewName = textdialog.GetValue();
    if (!sNewName.length())
        return;

    // The selected symbol is probably part of a procedure. If this is
    // the case, get the start and end position here
    if (m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB)
    {
        // Find the head of the current procedure
        nStartPos = this->FindCurrentProcedureHead(nPos);

        // Find the end of the current procedure depending on
        // the located head
        vector<int> vBlock = this->BlockMatch(nStartPos);
        if (vBlock.back() != wxSTC_INVALID_POSITION)
            nEndPos = vBlock.back();
    }

    // Gather all operations into one undo step
    this->BeginUndoAction();

    // Perform the renaming of symbols
    this->ReplaceMatches(this->FindAll(sCurrentName, this->GetStyleAt(nPos), nStartPos, nEndPos), sCurrentName, sNewName);
    this->EndUndoAction();
}

// This member function extracts a selected code section into
// a new procedure. The interface is created depending upon the
// used variables inside of the selected block
void NumeReEditor::AbstrahizeSection()
{
    // Do nothing, if the user didn't select anything
    if (!HasSelection())
        return;

    // Get start and end position (i.e. the corresponding lines)
    int nStartPos = PositionFromLine(LineFromPosition(GetSelectionStart()));
    int nEndPos = GetLineEndPosition(LineFromPosition(GetSelectionEnd()));
    if (GetSelectionEnd() == PositionFromLine(LineFromPosition(GetSelectionEnd())))
        nEndPos = GetLineEndPosition(LineFromPosition(GetSelectionEnd()-1));

    int nCurrentBlockStart = 0;
    int nCurrentBlockEnd = GetLastPosition();

    list<wxString> lInputTokens;
    list<wxString> lOutputTokens;


    // If we have a procedure file, consider scoping
    if (m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB)
    {
        nCurrentBlockStart = FindCurrentProcedureHead(nStartPos);
        vector<int> vBlock = BlockMatch(nCurrentBlockStart);
        if (vBlock.back() != wxSTC_INVALID_POSITION && vBlock.back() > nEndPos)
            nCurrentBlockEnd = vBlock.back();
    }

    // Determine the interface by searching for variables
    // and tables, which are used before or after the code
    // section and occure inside of the section
    for (int i = nStartPos; i <= nEndPos; i++)
    {
        if (isStyleType(STYLE_IDENTIFIER, i) || isStyleType(STYLE_STRINGPARSER, i))
        {
            // Regular indices
            //
            // Jump over string parser characters
            while (isStyleType(STYLE_STRINGPARSER, i) && (GetCharAt(i) == '#' || GetCharAt(i) == '~'))
                i++;
            if (isStyleType(STYLE_OPERATOR, i))
                continue;

            // Get the token name
            wxString sCurrentToken = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

            // Ignore MATLAB structure fields
            if (GetCharAt(WordStartPosition(i, true)-1) == '.')
                continue;

            // Find all occurences
            vector<int> vMatch = FindAll(sCurrentToken, this->GetStyleAt(i), nCurrentBlockStart, nCurrentBlockEnd);

            if (vMatch.size())
            {
                // Determine, whether the token is used before
                // or afer the current section
                if (vMatch.front() < nStartPos)
                    lInputTokens.push_back(sCurrentToken);
                if (vMatch.back() > nEndPos && vMatch.front() < nStartPos && IsModifiedInSection(nStartPos, nEndPos, sCurrentToken, vMatch))
                    lOutputTokens.push_back(sCurrentToken);
                else if (vMatch.back() > nEndPos && vMatch.front() >= nStartPos)
                    lOutputTokens.push_back(sCurrentToken);
            }

            i += sCurrentToken.length();
        }
        else if (isStyleType(STYLE_CUSTOMFUNCTION, i))
        {
            // Tables
            //
            // Get the token name
            wxString sCurrentToken = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

            // Find all occurences
            vector<int> vMatch = FindAll(sCurrentToken, this->GetStyleAt(i), nCurrentBlockStart, nCurrentBlockEnd);

            if (vMatch.size())
            {
                // Determine, whether the token is used before
                // or afer the current section
                if (vMatch.front() < nStartPos || vMatch.back() > nEndPos)
                    lInputTokens.push_back(sCurrentToken + "()");
            }

            i += sCurrentToken.length();
        }
    }

    // Only use each token once. We use the list
    // functionalities of C++
    if (lInputTokens.size())
    {
        lInputTokens.sort();
        lInputTokens.unique();
    }
    if (lOutputTokens.size())
    {
        lOutputTokens.sort();
        lOutputTokens.unique();
    }

    wxString sInputList;
    wxString sOutputList;

    // Create the interface definition lists
    for (auto iter = lInputTokens.begin(); iter != lInputTokens.end(); ++iter)
    {
        sInputList += *iter + ",";
    }
    if (sInputList.length())
        sInputList.erase(sInputList.length()-1);
    for (auto iter = lOutputTokens.begin(); iter != lOutputTokens.end(); ++iter)
    {
        sOutputList += *iter + ",";
    }
    if (sOutputList.length())
        sOutputList.erase(sOutputList.length()-1);

    // Use these interfaces and the positions to
    // create the new procedure in a new window
    CreateProcedureFromSection(nStartPos, nEndPos, sInputList, sOutputList);
}

// This member function is used to create a new procedure
// from the analysed code section. The new procedure will be
// displayed in a new window.
void NumeReEditor::CreateProcedureFromSection(int nStartPos, int nEndPos, const wxString& sInputList, const wxString sOutputList)
{
    // Creata a new window and a new editor
    ViewerFrame* copyFrame = new ViewerFrame(m_mainFrame, _guilang.get("GUI_REFACTORING_COPYWINDOW_HEAD"));
    NumeReEditor* edit = new NumeReEditor(m_mainFrame, m_options, new ProjectInfo(true), copyFrame, wxID_ANY, _syntax, m_terminal, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
    wxStatusBar* statusbar = copyFrame->CreateStatusBar();
    int sizes[] = {-2, -1};
    statusbar->SetFieldsCount(2, sizes);
    statusbar->SetStatusText(_guilang.get("GUI_STATUSBAR_UNSAVEDFILE"), 0);

    // Fill the editor with the new procedure and its
    // determined interface
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        // Set a default file name with the corresponding
        // extension
        edit->SetFilename(wxFileName("numere.nprc"), false);
        edit->SetText("\r\n");

        // Write some preface comment
        edit->AddText("## " + _guilang.get("GUI_REFACTORING_NOTE") + "\r\n");
        edit->AddText("##\r\n");
        edit->AddText("## " + _guilang.get("GUI_REFACTORING_ARGUMENTLIST") + "\r\n");

        // Write the input argument list
        edit->AddText("procedure $NEWPROCEDURE(" + sInputList + ")\r\n");

        // Write the actual section of code
        edit->AddText(this->GetTextRange(nStartPos, nEndPos) + "\r\n");

        // Write the output list
        if (sOutputList.length())
        {
            edit->AddText("\t## " + _guilang.get("GUI_REFACTORING_RETURNVALUES") + "\r\n");
            if (sOutputList.find(',') != string::npos)
                edit->AddText("return {" + sOutputList + "};\r\n");
            else
                edit->AddText("return " + sOutputList + ";\r\n");
        }
        else
            edit->AddText("return void\r\n");
        edit->AddText("endprocedure\r\n");
        statusbar->SetStatusText(_guilang.get("GUI_STATUSBAR_NPRC"), 1);
    }
    else if (m_fileType == FILE_MATLAB)
    {
        // Set a default file name with the corresponding
        // extension
        edit->SetFilename(wxFileName("numere.m"), false);
        edit->SetText("\r\n");

        // Write some preface comment
        edit->AddText("% " + _guilang.get("GUI_REFACTORING_NOTE") + "\r\n");
        edit->AddText("%\r\n");
        edit->AddText("% " + _guilang.get("GUI_REFACTORING_ARGUMENTLIST") + "\r\n");
        if (sOutputList.length())
            edit->AddText("% " + _guilang.get("GUI_REFACTORING_RETURNVALUES") + "\r\n");

        edit->AddText("function ");

        // Write the output list
        if (sOutputList.length())
        {
            edit->AddText("[" + sOutputList + "] = ");
        }

        // Write the input argument list
        edit->AddText("NEWFUNCTION(" + sInputList + ")\r\n");

        // Write the actual section of code
        edit->AddText(this->GetTextRange(nStartPos, nEndPos) + "\r\n");
        edit->AddText("end\r\n");
        statusbar->SetStatusText(_guilang.get("GUI_STATUSBAR_M"), 1);
    }

    // Update the syntax highlighting
    // and use the autoformatting feature
    edit->UpdateSyntaxHighlighting(true);
    edit->ApplyAutoFormat(0, -1);
    edit->ToggleSettings(SETTING_WRAPEOL);

    // Set a reasonable window size and
    // display it to the user
    copyFrame->SetSize(800, 600);
    copyFrame->SetIcon(wxIcon(m_mainFrame->getProgramFolder() + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    copyFrame->Show();
    copyFrame->SetFocus();
}

// This member function determines, whether a string token,
// which corresponds to a variable, is semantically modified
// in the code section (i.e. overwritten). This is used for
// variables, which are both input and possible output.
bool NumeReEditor::IsModifiedInSection(int nSectionStart, int nSectionEnd, const wxString& sToken, const vector<int>& vMatch)
{
    // define the set of modifying operators
    static wxString sModificationOperators = "+= -= /= ^= *= ++ --";

    // Go through all occurences of the current token
    for (size_t i = 0; i < vMatch.size(); i++)
    {
        // Ignore occurences before or after the current
        // code section
        if (vMatch[i] < nSectionStart)
            continue;
        if (vMatch[i] > nSectionEnd)
            break;

        // Examine the code part left of the token, whether
        // there's a modifying operator
        for (int j = vMatch[i]+sToken.length(); j < nSectionEnd; j++)
        {
            // Ignore whitespaces
            if (GetCharAt(j) == ' ' || GetCharAt(j) == '\t')
                continue;

            // We only examine operator characters
            if (isStyleType(STYLE_OPERATOR, j) && isStyleType(STYLE_OPERATOR, j+1) && sModificationOperators.find(GetTextRange(j, j+2)) != string::npos)
                return true;
            else if (isStyleType(STYLE_OPERATOR, j) && GetCharAt(j) == '=' && GetCharAt(j+1) != '=')
                return true;
            else if (isStyleType(STYLE_OPERATOR, j) && (GetCharAt(j) == '(' || GetCharAt(j) == '[' || GetCharAt(j) == '{'))
            {
                // Jump over parentheses
                j = BraceMatch(j);
                if (j == wxSTC_INVALID_POSITION)
                    return false;
            }
            else if (isStyleType(STYLE_OPERATOR, j) && (GetCharAt(j) == ')' || GetCharAt(j) == ']' || GetCharAt(j) == '}' || GetCharAt(j) == ':'))
            {
                // ignore closing parentheses
                continue;
            }
            else if (isStyleType(STYLE_OPERATOR, j) && GetCharAt(j) == '.' && isStyleType(STYLE_IDENTIFIER, j+1))
            {
                while (isStyleType(STYLE_IDENTIFIER, j+1))
                    j++;
            }
            else if (isStyleType(STYLE_OPERATOR, j) && GetCharAt(j) == ',')
            {
                // Try to find the end of the current bracket
                // or the current brace - we don't use parentheses
                // as terminators, because they are not used to
                // gather return values
                for (int k = j; k < nSectionEnd; k++)
                {
                    if (isStyleType(STYLE_OPERATOR, k) && (GetCharAt(k) == ']' || GetCharAt(k) == '}'))
                    {
                        j = k;
                        break;
                    }
                    else if (isStyleType(STYLE_OPERATOR, k) && GetCharAt(k) == ';')
                        break;
                }
            }
            else
                break;
        }
    }

    return false;
}

wxString NumeReEditor::getTemplateContent(const wxString& sFileName)
{
	wxString template_file, template_type, timestamp;

	template_type = "tmpl_procedure.nlng";
	timestamp = getTimeStamp(false);

	if (m_terminal->getKernelSettings().getUseCustomLanguageFiles() && wxFileExists(m_mainFrame->getProgramFolder() + "\\user\\lang\\" + template_type))
		m_mainFrame->GetFileContents(m_mainFrame->getProgramFolder() + "\\user\\lang\\" + template_type, template_file, template_type);
	else
		m_mainFrame->GetFileContents(m_mainFrame->getProgramFolder() + "\\lang\\" + template_type, template_file, template_type);

	while (template_file.find("%%1%%") != string::npos)
		template_file.replace(template_file.find("%%1%%"), 5, sFileName);
	while (template_file.find("%%2%%") != string::npos)
		template_file.replace(template_file.find("%%2%%"), 5, timestamp);
	return template_file;
}

wxString NumeReEditor::generateAutoCompList(const wxString& wordstart, string sPreDefList)
{
	map<wxString, int> mAutoCompMap;
	wxString wReturn = "";
	string sCurrentWord = "";
	if (sPreDefList.length())
	{
		while (sPreDefList.length())
		{
			sCurrentWord = sPreDefList.substr(0, sPreDefList.find(' '));
			if (sCurrentWord.find('(') != string::npos)
				mAutoCompMap[toLowerCase(sCurrentWord.substr(0, sCurrentWord.find('('))) + " |" + sCurrentWord] = -1;
			else
				mAutoCompMap[toLowerCase(sCurrentWord.substr(0, sCurrentWord.find('?'))) + " |" + sCurrentWord] = -1;
			sPreDefList.erase(0, sPreDefList.find(' '));
			if (sPreDefList.front() == ' ')
				sPreDefList.erase(0, 1);
		}
	}
	unsigned int nPos = 0;
	while ((nPos = this->FindText(nPos, this->GetLastPosition(), wordstart, wxSTC_FIND_WORDSTART)) != string::npos)
	{
		if (nPos > (size_t)this->GetCurrentPos() || WordEndPosition(nPos + 1, true) < this->GetCurrentPos())
			mAutoCompMap[toLowerCase(this->GetTextRange(nPos, WordEndPosition(nPos + 1, true)).ToStdString()) + " |" + this->GetTextRange(nPos, WordEndPosition(nPos + 1, true))] = 1;
		nPos++;
	}
	// remove duplicates
	for (auto iter = mAutoCompMap.begin(); iter != mAutoCompMap.end(); ++iter)
	{
		if (iter->second == -1)
		{
			if ((iter->first).find('(') != string::npos)
			{
				if (mAutoCompMap.find((iter->first).substr(0, (iter->first).find('('))) != mAutoCompMap.end())
				{
					mAutoCompMap.erase((iter->first).substr(0, (iter->first).find('(')));
					iter = mAutoCompMap.begin();
				}
			}
			else
			{
				if (mAutoCompMap.find((iter->first).substr(0, (iter->first).find('?'))) != mAutoCompMap.end())
				{
					mAutoCompMap.erase((iter->first).substr(0, (iter->first).find('?')));
					iter = mAutoCompMap.begin();
				}
			}
		}
	}
	for (auto iter = mAutoCompMap.begin(); iter != mAutoCompMap.end(); ++iter)
	{
		wReturn += (iter->first).substr((iter->first).find('|') + 1) + " ";
	}
	return wReturn;
}


void NumeReEditor::OnDisplayVariable(wxCommandEvent& event)
{
	long int maxpos = this->GetLastPosition();
	this->SetIndicatorCurrent(HIGHLIGHT);
	this->IndicatorClearRange(0, maxpos);
	this->IndicatorSetStyle(HIGHLIGHT, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT, 100);
	this->IndicatorSetForeground(HIGHLIGHT, wxColor(255, 0, 0));

	unsigned int nPos = 0;
	unsigned int nCurr = 0;
	vector<unsigned int> vSelectionList;

	m_watchedString = m_clickedWord;
	while ((nPos = this->FindText(nCurr, maxpos, m_clickedWord, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != string::npos)
	{
		vSelectionList.push_back(nPos);
		nCurr = nPos +  m_clickedWordLength; //m_clickedWord.length();
	}

	// this->SetIndicatorCurrent(HIGHLIGHT);

	for (size_t i = 0; i < vSelectionList.size(); i++)
	{
		this->IndicatorFillRange(vSelectionList[i], m_clickedWordLength); //m_clickedWord.length());
	}

	/*wxDebugEvent dbg;

	dbg.SetId(ID_DEBUG_DISPLAY_SELECTION);

	wxArrayString vars;
	vars.Add(m_clickedWord);
	dbg.SetVariableNames(vars);

	m_debugManager->AddPendingEvent(dbg);*///m_mainFrame->AddPendingEvent(dbg);


	// TODO Need to signal that it's a one-shot, which needs to be
	// handled appropriately in the debugger.
}

void NumeReEditor::OnHelpOnSelection(wxCommandEvent& event)
{
	m_mainFrame->openHTML(m_terminal->getDocumentation(m_clickedWord.ToStdString()));
}

void NumeReEditor::OnFindProcedure(wxCommandEvent& event)
{
	if (!m_clickedProcedure.length())
		return;
	vector<std::string> vPaths = m_terminal->getPathSettings();
	wxString pathname = m_clickedProcedure;
	if (pathname.find("$this~") != string::npos)
	{
		wxString thispath = GetFileNameAndPath();
		pathname.replace(pathname.find("$this~"), 6, thispath.substr(0, thispath.rfind('\\') + 1));
	}
	else if (pathname.find("$thisfile~") != string::npos)
	{
		wxString procedurename = pathname.substr(pathname.rfind('~') + 1);
		wxString procedureline;
		int nminpos = 0;
		int nmaxpos = GetLastPosition();
		while (nminpos < nmaxpos && FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
		{
			nminpos = FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;
			if (this->GetStyleAt(nminpos) == wxSTC_NSCR_COMMENT_BLOCK || this->GetStyleAt(nminpos) == wxSTC_NSCR_COMMENT_LINE)
				continue;
			procedureline = GetLine(LineFromPosition(nminpos));
			if (procedureline.find("$" + procedurename) != string::npos && procedureline[procedureline.find_first_not_of(' ', procedureline.find("$" + procedurename) + procedurename.length() + 1)] == '(')
			{
				this->SetFocus();
				this->EnsureVisible(LineFromPosition(nminpos));
				this->GotoLine(LineFromPosition(nminpos));
				return;
			}
		}
		int ret = wxMessageBox(_guilang.get("GUI_DLG_PROC_NEXISTS_CREATE", m_clickedProcedure.ToStdString()), _guilang.get("GUI_DLG_PROC_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);
		if (ret != wxYES)
			return;
		wxString proctemplate = getTemplateContent(procedurename);

		int nLastLine = this->GetLineCount();
		this->GotoLine(nLastLine);
		this->AddText("\n");
		this->AddText(proctemplate);
		nLastLine = FindText(this->PositionFromLine(nLastLine), this->GetLastPosition(), "procedure $" + procedurename, wxSTC_FIND_MATCHCASE);
		this->GotoPos(nLastLine);

		UpdateSyntaxHighlighting(true);
		AnalyseCode();

		return;
	}
	else
	{
		if (pathname.find("$main~") != string::npos)
			pathname.erase(pathname.find("$main~") + 1, 5);
		while (pathname.find('~') != string::npos)
			pathname[pathname.find('~')] = '/';
		if (pathname[0] == '$' && pathname.find(':') == string::npos)
			pathname.replace(0, 1, vPaths[5] + "/");
		else if (pathname.find(':') == string::npos)
			pathname.insert(0, vPaths[5]);
		else // pathname.find(':') != string::npos
		{
			pathname = pathname.substr(pathname.find('\'') + 1, pathname.rfind('\'') - pathname.find('\'') - 1);
		}
	}
	wxArrayString pathnames;
	pathnames.Add(pathname + ".nprc");
	if (!fileExists((pathname + ".nprc").ToStdString()))
	{
		int ret = wxMessageBox(_guilang.get("GUI_DLG_PROC_NEXISTS_CREATE", m_clickedProcedure.ToStdString()), _guilang.get("GUI_DLG_PROC_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);
		if (ret != wxYES)
			return;
		m_mainFrame->NewFile(FILE_NPRC, m_clickedProcedure);
	}
	else
		m_mainFrame->OpenSourceFile(pathnames);
}

void NumeReEditor::OnFindInclude(wxCommandEvent& event)
{
	if (!m_clickedInclude.length())
		return;
	wxArrayString pathnames;
	pathnames.Add(m_clickedInclude);
	if (!fileExists((m_clickedInclude).ToStdString()))
	{
		int ret = wxMessageBox(_guilang.get("GUI_DLG_SCRIPT_NEXISTS_CREATE", m_clickedInclude.ToStdString()), _guilang.get("GUI_DLG_SCRIPT_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);
		if (ret != wxYES)
			return;
		m_mainFrame->NewFile(FILE_NSCR, m_clickedInclude);
	}
	else
		m_mainFrame->OpenSourceFile(pathnames);
}

void NumeReEditor::OnChangeCase(wxCommandEvent& event)
{
	if (!HasSelection())
		return;
	int nFirstPos = GetSelectionStart();
	int nLastPos = GetSelectionEnd();

	if (event.GetId() == ID_UPPERCASE)
	{
		Replace(nFirstPos, nLastPos, toUpperCase(GetSelectedText().ToStdString()));
	}
	else
	{
		Replace(nFirstPos, nLastPos, toLowerCase(GetSelectedText().ToStdString()));
	}
}

void NumeReEditor::OnFoldCurrentBlock(wxCommandEvent& event)
{
	FoldCurrentBlock(this->LineFromPosition(this->PositionFromPoint(m_lastRightClick)));
}

void NumeReEditor::OnRenameSymbols(wxCommandEvent& event)
{
    this->RenameSymbols(this->PositionFromPoint(m_lastRightClick));
}

void NumeReEditor::OnAbstrahizeSection(wxCommandEvent& event)
{
    this->AbstrahizeSection();
}

bool NumeReEditor::InitDuplicateCode()
{
	if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB || m_fileType == FILE_CPP)
	{
		m_duplicateCode = new DuplicateCodeDialog(this, "NumeRe: " + _guilang.get("GUI_DUPCODE_TITLE") + " [" + this->GetFilenameString() + "]");
		m_duplicateCode->SetIcon(wxIcon(m_mainFrame->getProgramFolder() + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
		m_duplicateCode->Show();
		m_duplicateCode->SetFocus();
		m_duplicateCode->Refresh();
		return true;
	}
	return false;
}

void NumeReEditor::OnFindDuplicateCode(int nDuplicateFlag, int nNumDuplicatedLines)
{
	detectCodeDuplicates(0, this->LineFromPosition(this->GetLastPosition()), nDuplicateFlag, nNumDuplicatedLines);
}

void NumeReEditor::OnThreadUpdate(wxThreadEvent& event)
{
	if (m_nProcessValue < 100)
	{
		if (m_duplicateCode && m_duplicateCode->IsShown())
			m_duplicateCode->SetProgress(m_nProcessValue);
	}
	else
	{
		if (m_duplicateCode && m_duplicateCode->IsShown())
			m_duplicateCode->SetProgress(100);
		else
		{
			vDuplicateCodeResults.clear();
			return;
		}

		wxCriticalSectionLocker lock(m_editorCS);
		m_duplicateCode->SetResult(vDuplicateCodeResults);
		vDuplicateCodeResults.clear();
	}

}

void NumeReEditor::IndicateDuplicatedLine(int nStart1, int nEnd1, int nStart2, int nEnd2, int nSelectionLine)
{
	this->MarkerDefine(MARKER_DUPLICATEINDICATOR_ONE, wxSTC_MARK_BACKGROUND);
	this->MarkerSetBackground(MARKER_DUPLICATEINDICATOR_ONE, wxColour(220, 255, 220));
	this->MarkerDefine(MARKER_DUPLICATEINDICATOR_TWO, wxSTC_MARK_BACKGROUND);
	this->MarkerSetBackground(MARKER_DUPLICATEINDICATOR_TWO, wxColour(255, 220, 220));
	this->MarkerDeleteAll(MARKER_DUPLICATEINDICATOR_ONE);
	this->MarkerDeleteAll(MARKER_DUPLICATEINDICATOR_TWO);

	this->SetIndicatorCurrent(HIGHLIGHT_DIFFERENCES);
	this->IndicatorSetStyle(HIGHLIGHT_DIFFERENCES, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT_DIFFERENCES, 64);
	this->IndicatorSetForeground(HIGHLIGHT_DIFFERENCES, wxColour(128, 0, 128));

	this->IndicatorClearRange(0, GetLastPosition());

	this->SetIndicatorCurrent(HIGHLIGHT_DIFFERENCE_SOURCE);
	this->IndicatorSetStyle(HIGHLIGHT_DIFFERENCE_SOURCE, wxSTC_INDIC_ROUNDBOX);
	this->IndicatorSetAlpha(HIGHLIGHT_DIFFERENCE_SOURCE, 64);
	this->IndicatorSetForeground(HIGHLIGHT_DIFFERENCE_SOURCE, wxColour(0, 128, 128));

	this->IndicatorClearRange(0, GetLastPosition());

	if (nStart1 == -1 && nStart2 == -1 && nEnd1 == -1 && nEnd2 == -1)
		return;

	for (int i = nStart1; i <= nEnd1; i++)
		this->MarkerAdd(i, MARKER_DUPLICATEINDICATOR_ONE);
	for (int i = nStart2; i <= nEnd2; i++)
		this->MarkerAdd(i, MARKER_DUPLICATEINDICATOR_TWO);

	map<int, int> mDifferences = this->getDifferences(nStart1, nEnd1, nStart2, nEnd2);

	for (auto iter = mDifferences.begin(); iter != mDifferences.end(); ++iter)
	{
		if ((iter->first) < 0)
			this->SetIndicatorCurrent(HIGHLIGHT_DIFFERENCE_SOURCE);
		else
			this->SetIndicatorCurrent(HIGHLIGHT_DIFFERENCES);

		this->IndicatorFillRange(abs(iter->first), iter->second);
	}
	this->ScrollToLine(nSelectionLine);
}

void NumeReEditor::ClearDblClkIndicator()
{
	if (!m_dblclkString.length())
		return;
	m_dblclkString.clear();

	this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
	long int maxpos = this->GetLastPosition();
	this->IndicatorClearRange(0, maxpos);
}

void NumeReEditor::OnMarginClick( wxStyledTextEvent& event )
{
	// we know it's margin 2, because that's the only sensitive margin

	if (m_fileType != FILE_NSCR && m_fileType != FILE_NPRC && m_fileType != FILE_MATLAB && m_fileType != FILE_CPP)
		return;
	int position = event.GetPosition();

	int linenum = this->LineFromPosition(position);

	if (event.GetMargin() == MARGIN_FOLD)
	{
		int levelClick = this->GetFoldLevel(linenum);
		if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
			this->ToggleFold(linenum);
	}
	else
	{
		if (MarkerOnLine(linenum, MARKER_BREAKPOINT))
		{
			RemoveBreakpoint(linenum);
		}
		else
		{
			AddBreakpoint(linenum);
		}
	}
}

void NumeReEditor::AddBreakpoint( int linenum )
{
	for (int i = this->PositionFromLine(linenum); i < this->GetLineEndPosition(linenum); i++)
	{
		if (this->GetStyleAt(i) != wxSTC_NSCR_COMMENT_BLOCK
				&& this->GetStyleAt(i) != wxSTC_NSCR_COMMENT_LINE
				&& this->GetCharAt(i) != '\r'
				&& this->GetCharAt(i) != '\n'
				&& this->GetCharAt(i) != ' '
				&& this->GetCharAt(i) != '\t')
		{
			int markerNum = this->MarkerAdd(linenum, MARKER_BREAKPOINT);

			m_breakpoints.Add(markerNum);
			CreateBreakpointEvent(linenum, true);
			m_terminal->_guimessenger.addBreakpoint(GetFileNameAndPath().ToStdString(), linenum);
			break;
		}
	}
}

bool NumeReEditor::MarkerOnLine(int linenum, int nMarker)
{
	int markerLineBitmask = this->MarkerGet(linenum);

	bool markerOnLine = (markerLineBitmask & (1 << nMarker));

	return markerOnLine;
}

int NumeReEditor::calculateCyclomaticComplexity(int startline, int endline)
{
	int nCycComplx = 1;
	for (int i = this->PositionFromLine(startline); i < this->GetLineEndPosition(endline); i++)
	{
		if (isStyleType(STYLE_COMMAND, i))
		{
			int wordstart = this->WordStartPosition(i, true);
			int wordend = this->WordEndPosition(i, true);
			if (this->GetTextRange(wordstart, wordend) == "if"
					|| this->GetTextRange(wordstart, wordend) == "elseif"
					|| this->GetTextRange(wordstart, wordend) == "while"
					|| this->GetTextRange(wordstart, wordend) == "case"
					|| this->GetTextRange(wordstart, wordend) == "for")
				nCycComplx++;
			i = wordend;
		}
		if (isStyleType(STYLE_FUNCTION, i))
		{
			int wordstart = this->WordStartPosition(i, true);
			int wordend = this->WordEndPosition(i, true);
			if (this->GetTextRange(wordstart, wordend) == "and"
					|| this->GetTextRange(wordstart, wordend) == "or"
					|| this->GetTextRange(wordstart, wordend) == "xor")
				nCycComplx++;
			i = wordend;
		}
		if (isStyleType(STYLE_OPERATOR, i))
		{
			int j = i;
			while (isStyleType(STYLE_OPERATOR, j))
				j++;
			if (this->GetTextRange(i, j) == "&"
                    || this->GetTextRange(i, j) == "&&"
					|| this->GetTextRange(i, j) == "|"
					|| this->GetTextRange(i, j) == "||"
					|| this->GetTextRange(i, j) == "|||")
				nCycComplx++;
			i = j;
		}
	}
	return nCycComplx;
}

int NumeReEditor::calculateLinesOfCode(int startline, int endline)
{
	int nLinesOfCode = 0;
	string currentline;
	for (int i = startline; i <= endline; i++)
	{
		currentline = this->GetLine(i).ToStdString();
		if (currentline.find("##") != string::npos)
			currentline.erase(currentline.find("##"));
		if (currentline.find_first_not_of(" \t\r\n") != string::npos)
		{
			for (size_t j = this->PositionFromLine(i); j < currentline.length() + this->PositionFromLine(i); j++)
			{
				if (!isStyleType(STYLE_COMMENT_BLOCK, j)
                        && !isStyleType(STYLE_COMMENT_LINE, j)
						&& this->GetCharAt(j) != ' '
						&& this->GetCharAt(j) != '\t'
						&& this->GetCharAt(j) != '\r'
						&& this->GetCharAt(j) != '\n')
				{
					nLinesOfCode++;
					break;
				}
			}
		}
	}
	return nLinesOfCode;
}

int NumeReEditor::countNumberOfComments(int startline, int endline)
{
	int nComments = 0;
	for (int i = this->PositionFromLine(startline); i < this->GetLineEndPosition(endline); i++)
	{
		if (isStyleType(STYLE_COMMENT_BLOCK, i) || isStyleType(STYLE_COMMENT_LINE, i))
		{
			nComments++;
			for (int j = i; j < this->GetLineEndPosition(endline); j++)
			{
				if (!isStyleType(STYLE_COMMENT_LINE, j) && !isStyleType(STYLE_COMMENT_BLOCK, j))
				{
					i = j;
					break;
				}
				if (j > i + 1 && this->PositionFromLine(this->LineFromPosition(j)) == j)
					nComments++;
			}
		}
	}
	return nComments;
}

int NumeReEditor::insertTextAndMove(int nPosition, const wxString& sText)
{
	this->InsertText(nPosition, sText);
	return sText.length();
}


wxThread::ExitCode NumeReEditor::Entry()
{
	vDuplicateCodeResults.clear();

	// Create the memory for the parsed line
	// This is done before the processing to avoid reallocs during the process
	vParsedSemanticCode.resize(m_nLastLine + 1);

	double dMatch = 0.0;
	int nLongestMatch = 0;
	int nBlankLines = 0;
	int nLastStatusVal = 0;
	int currentDuplicateCodeLength = m_nDuplicateCodeLines;
	if (getFileType() == FILE_CPP)
		currentDuplicateCodeLength *= 2;

	for (int i = m_nFirstLine; i <= m_nLastLine - currentDuplicateCodeLength; i++)
	{
		if (GetThread()->TestDestroy())
			break;
		if (m_duplicateCode && m_duplicateCode->IsShown())
		{
			// display some status value
			if ((int)((double)i / (double)(m_nLastLine - currentDuplicateCodeLength - m_nFirstLine) * 100) != nLastStatusVal)
			{
				nLastStatusVal = (double)i / (double)(m_nLastLine - currentDuplicateCodeLength - m_nFirstLine) * 100;
				wxCriticalSectionLocker lock(m_editorCS);
				m_nProcessValue = nLastStatusVal;
				wxQueueEvent(GetEventHandler(), new wxThreadEvent());
			}
		}
		else
		{
			// Stop this processing, if the duplicate code window was closed
			break;
		}
		for (int j = i + currentDuplicateCodeLength; j <= m_nLastLine - currentDuplicateCodeLength; j++)
		{
			dMatch = compareCodeLines(i, j, m_nDuplicateCodeFlag);
			if (dMatch >= 0.75)
			{
				double dComp;
				for (int k = 1; k <= m_nLastLine - j; k++)
				{
					dComp = compareCodeLines(i + k, j + k, m_nDuplicateCodeFlag);
					if (dComp == -1.0)
					{
						nBlankLines++;
						continue;
					}
					else if (dComp < 0.75 || i + k == j)
					{
						if (k - nBlankLines > currentDuplicateCodeLength)
						{
							wxCriticalSectionLocker lock(m_editorCS);
							vDuplicateCodeResults.push_back(toString(i + 1) + "-" + toString(i + k) + " == " + toString(j + 1) + "-" + toString(j + k) + " [" + toString(dMatch * 100.0 / (double)(k - nBlankLines), 3) + " %]");
							if (nLongestMatch < k - 1 - nBlankLines)
								nLongestMatch = k - 1 - nBlankLines;
						}
						break;
					}
					else
						dMatch += dComp;
				}
				nBlankLines = 0;
			}
			else if (dMatch < 0.0) // empty line at pos i
				break;
		}
		i += nLongestMatch;
		nLongestMatch = 0;
	}

	// clear the parsed code
	vParsedSemanticCode.clear();

	wxCriticalSectionLocker lock(m_editorCS);
	m_nProcessValue = 100;
	wxQueueEvent(GetEventHandler(), new wxThreadEvent());
	return (wxThread::ExitCode)0;
}

void NumeReEditor::detectCodeDuplicates(int startline, int endline, int nDuplicateFlags, int nNumDuplicatedLines)
{
	m_nDuplicateCodeFlag = nDuplicateFlags;
	m_nFirstLine = startline;
	m_nLastLine = endline;
	m_nDuplicateCodeLines = nNumDuplicatedLines;
	if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR)
		return;
	if (GetThread()->Run() != wxTHREAD_NO_ERROR)
		return;
}

// semantic comparison between two code lines. Will return a double representing a matching percentage
// This double is constructed out of the actual matching of the characters and out of a semantic
// match. If the characters match completely, the semantic comparison is omitted.
double NumeReEditor::compareCodeLines(int nLine1, int nLine2, int nDuplicateFlag)
{

	size_t nMatchedCount = 0;
	string sSemLine1 = this->getSemanticLine(nLine1, nDuplicateFlag);
	string sSemLine2 = this->getSemanticLine(nLine2, nDuplicateFlag);

	// It is possible that the lines are semantical identical although they may contain different vars
	if (!sSemLine1.length() && sSemLine2.length())
		return -2.0;
	else if (!sSemLine1.length() && !sSemLine2.length())
		return -1.0;
	/*else if (sSemLine1 == sSemLine2)
	    return 1.0;*/
	else if (sSemLine1.length() * 1.5 < sSemLine2.length() || sSemLine1.length() > sSemLine2.length() * 1.5)
		return 0.0;

	for (size_t i = 0; i < sSemLine1.length(); i++)
	{
		if (i >= sSemLine2.length())
			break;
		if (sSemLine1[i] == sSemLine2[i])
			nMatchedCount++;
	}

	return (double)nMatchedCount / (double)max(sSemLine1.length(), sSemLine2.length());
}

string NumeReEditor::getSemanticLine(int nLine, int nDuplicateFlag)
{
	if (vParsedSemanticCode[nLine].length())
		return vParsedSemanticCode[nLine];
	if (getFileType() == FILE_NSCR || getFileType() == FILE_NPRC)
		return getSemanticLineNSCR(nLine, nDuplicateFlag);
	else if (getFileType() == FILE_MATLAB)
		return getSemanticLineMATLAB(nLine, nDuplicateFlag);
	else if (getFileType() == FILE_CPP)
		return getSemanticLineCPP(nLine, nDuplicateFlag);
	else
		return "";
}

// 0 = direct comparison, 1 = use var semanticals, 2 = use string semanticals, 4 = use numeric semanticals
string NumeReEditor::getSemanticLineNSCR(int nLine, int nDuplicateFlag)
{
	string sSemLine = "";
	for (int i = this->PositionFromLine(nLine); i < this->GetLineEndPosition(nLine); i++)
	{
		if (this->GetCharAt(i) == ' '
				|| this->GetCharAt(i) == '\t'
				|| this->GetCharAt(i) == '\r'
				|| this->GetCharAt(i) == '\n'
				|| this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_BLOCK
				|| this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_LINE)
			continue;
		else if ((nDuplicateFlag & SEMANTICS_VAR)
				 && (this->GetStyleAt(i) == wxSTC_NSCR_DEFAULT
					 || this->GetStyleAt(i) == wxSTC_NSCR_DEFAULT_VARS
					 || this->GetStyleAt(i) == wxSTC_NSCR_IDENTIFIER))
		{
			// replace vars with a placeholder
			i = this->WordEndPosition(i, true) - 1;
			while (this->GetStyleAt(i + 1) == wxSTC_NSCR_DEFAULT
					|| this->GetStyleAt(i + 1) == wxSTC_NSCR_DEFAULT_VARS
					|| this->GetStyleAt(i + 1) == wxSTC_NSCR_IDENTIFIER)
				i++;
			sSemLine += "VAR";
		}
		else if ((nDuplicateFlag & SEMANTICS_STRING) && this->GetStyleAt(i) == wxSTC_NSCR_STRING)
		{
			// replace string literals with a placeholder
			i++;
			while (this->GetStyleAt(i + 1) == wxSTC_NSCR_STRING)
				i++;
			sSemLine += "STR";
		}
		else if ((nDuplicateFlag & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_NSCR_NUMBERS)
		{
			// replace numeric literals with a placeholder
			while (this->GetStyleAt(i + 1) == wxSTC_NSCR_NUMBERS)
				i++;
			if (sSemLine.back() == '-' || sSemLine.back() == '+')
			{
				if (sSemLine.length() == 1)
					sSemLine.clear();
				else
				{
					char cDelim = sSemLine[sSemLine.length() - 2];
					if (cDelim == ':' || cDelim == '=' || cDelim == '?' || cDelim == ',' || cDelim == ';' || cDelim == '(' || cDelim == '[' || cDelim == '{')
						sSemLine.pop_back();
				}
			}
			sSemLine += "NUM";
		}
		else if ((nDuplicateFlag & SEMANTICS_FUNCTION) && this->GetStyleAt(i) == wxSTC_NSCR_CUSTOM_FUNCTION)
		{
			// replace functions and caches with a placeholder
			while (this->GetStyleAt(i + 1) == wxSTC_NSCR_CUSTOM_FUNCTION)
				i++;
			sSemLine += "FUNC";
		}
		else
			sSemLine += this->GetCharAt(i);

	}
	// Store the result to avoid repeated processing of this line
	vParsedSemanticCode[nLine] = sSemLine;
	return sSemLine;
}

string NumeReEditor::getSemanticLineMATLAB(int nLine, int nDuplicateFlag)
{
	string sSemLine = "";
	for (int i = this->PositionFromLine(nLine); i < this->GetLineEndPosition(nLine); i++)
	{
		if (this->GetCharAt(i) == ' '
				|| this->GetCharAt(i) == '\t'
				|| this->GetCharAt(i) == '\r'
				|| this->GetCharAt(i) == '\n'
				|| this->GetStyleAt(i) == wxSTC_MATLAB_COMMENT)
			continue;
		else if ((nDuplicateFlag & SEMANTICS_VAR)
				 && (this->GetStyleAt(i) == wxSTC_MATLAB_DEFAULT
					 || this->GetStyleAt(i) == wxSTC_MATLAB_IDENTIFIER))
		{
			// replace vars with a placeholder
			i = this->WordEndPosition(i, true) - 1;
			while (this->GetStyleAt(i + 1) == wxSTC_MATLAB_DEFAULT
					|| this->GetStyleAt(i + 1) == wxSTC_MATLAB_IDENTIFIER)
				i++;
			sSemLine += "VAR";
		}
		else if ((nDuplicateFlag & SEMANTICS_STRING) && this->GetStyleAt(i) == wxSTC_MATLAB_STRING)
		{
			// replace string literals with a placeholder
			i++;
			while (this->GetStyleAt(i + 1) == wxSTC_MATLAB_STRING)
				i++;
			sSemLine += "STR";
		}
		else if ((nDuplicateFlag & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_MATLAB_NUMBER)
		{
			// replace numeric literals with a placeholder
			while (this->GetStyleAt(i + 1) == wxSTC_MATLAB_NUMBER)
				i++;
			if (sSemLine.back() == '-' || sSemLine.back() == '+')
			{
				if (sSemLine.length() == 1)
					sSemLine.clear();
				else
				{
					char cDelim = sSemLine[sSemLine.length() - 2];
					if (cDelim == ':' || cDelim == '=' || cDelim == '?' || cDelim == ',' || cDelim == ';' || cDelim == '(' || cDelim == '[' || cDelim == '{')
						sSemLine.pop_back();
				}
			}
			sSemLine += "NUM";
		}
		/*else if ((nDuplicateFlag & SEMANTICS_FUNCTION) && this->GetStyleAt(i) == wxSTC_NSCR_CUSTOM_FUNCTION)
		{
		    // replace functions and caches with a placeholder
		    while (this->GetStyleAt(i+1) == wxSTC_NSCR_CUSTOM_FUNCTION)
		        i++;
		    sSemLine += "FUNC";
		}*/
		else
			sSemLine += this->GetCharAt(i);

	}
	// Store the result to avoid repeated processing of this line
	vParsedSemanticCode[nLine] = sSemLine;
	return sSemLine;
}

string NumeReEditor::getSemanticLineCPP(int nLine, int nDuplicateFlag)
{
	string sSemLine = "";
	for (int i = this->PositionFromLine(nLine); i < this->GetLineEndPosition(nLine); i++)
	{
		if (this->GetCharAt(i) == ' '
				|| this->GetCharAt(i) == '\t'
				|| this->GetCharAt(i) == '\r'
				|| this->GetCharAt(i) == '\n'
				|| isStyleType(STYLE_COMMENT_LINE, i) || isStyleType(STYLE_COMMENT_BLOCK, i))
			continue;
		else if ((nDuplicateFlag & SEMANTICS_VAR)
				 && (this->GetStyleAt(i) == wxSTC_C_DEFAULT
					 || this->GetStyleAt(i) == wxSTC_C_IDENTIFIER))
		{
			// replace vars with a placeholder
			i = this->WordEndPosition(i, true) - 1;
			while (this->GetStyleAt(i + 1) == wxSTC_C_DEFAULT
					|| this->GetStyleAt(i + 1) == wxSTC_C_IDENTIFIER)
				i++;
			sSemLine += "VAR";
		}
		else if ((nDuplicateFlag & SEMANTICS_STRING) && (this->GetStyleAt(i) == wxSTC_C_STRING || this->GetStyleAt(i) == wxSTC_C_CHARACTER))
		{
			// replace string literals with a placeholder
			i++;
			while (this->GetStyleAt(i + 1) == wxSTC_C_STRING || this->GetStyleAt(i + 1) == wxSTC_C_CHARACTER)
				i++;
			sSemLine += "STR";
		}
		else if ((nDuplicateFlag & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_C_NUMBER)
		{
			// replace numeric literals with a placeholder
			while (this->GetStyleAt(i + 1) == wxSTC_C_NUMBER)
				i++;
			if (sSemLine.back() == '-' || sSemLine.back() == '+')
			{
				if (sSemLine.length() == 1)
					sSemLine.clear();
				else
				{
					char cDelim = sSemLine[sSemLine.length() - 2];
					if (cDelim == ':' || cDelim == '=' || cDelim == '?' || cDelim == ',' || cDelim == ';' || cDelim == '(' || cDelim == '[' || cDelim == '{')
						sSemLine.pop_back();
				}
			}
			sSemLine += "NUM";
		}
		/*else if ((nDuplicateFlag & SEMANTICS_FUNCTION) && this->GetStyleAt(i) == wxSTC_NSCR_CUSTOM_FUNCTION)
		{
		    // replace functions and caches with a placeholder
		    while (this->GetStyleAt(i+1) == wxSTC_NSCR_CUSTOM_FUNCTION)
		        i++;
		    sSemLine += "FUNC";
		}*/
		else
			sSemLine += this->GetCharAt(i);

	}
	// Store the result to avoid repeated processing of this line
	vParsedSemanticCode[nLine] = sSemLine;
	return sSemLine;
}


map<int, int> NumeReEditor::getDifferences(int nStart1, int nEnd1, int nStart2, int nEnd2)
{
	map<int, int> mDifferences;
	int nLinePos1 = 0;
	int nLinePos2 = 0;
	wxString sToken1;
	wxString sToken2;

	for (int i = 0; i <= nEnd1 - nStart1; i++)
	{
		nLinePos1 = this->PositionFromLine(nStart1 + i);
		nLinePos2 = this->PositionFromLine(nStart2 + i);
		while (nLinePos1 < this->GetLineEndPosition(nStart1 + i) || nLinePos2 < this->GetLineEndPosition(nStart2 + i))
		{
			sToken1 = getNextToken(nLinePos1);
			sToken2 = getNextToken(nLinePos2);

			if (!sToken1.length() && !sToken2.length())
				break;
			if (sToken1.length() && !sToken2.length())
			{
				mDifferences[-nLinePos1] = sToken1.length();
			}
			else if (sToken2.length() && !sToken1.length())
			{
				mDifferences[nLinePos2] = sToken2.length();
			}
			else
			{
				if (sToken1 != sToken2)
				{
					mDifferences[-nLinePos1] = sToken1.length();
					mDifferences[nLinePos2] = sToken2.length();
				}
			}
			nLinePos1 += sToken1.length();
			nLinePos2 += sToken2.length();
			if (nLinePos1 > this->GetLineEndPosition(nStart1 + i))
				nLinePos1 = this->GetLineEndPosition(nStart1 + i);
			if (nLinePos2 > this->GetLineEndPosition(nStart2 + i))
				nLinePos2 = this->GetLineEndPosition(nStart2 + i);
		}
	}
	return mDifferences;
}

wxString NumeReEditor::getNextToken(int& nPos)
{
	int nCurrentLineEnd = this->GetLineEndPosition(this->LineFromPosition(nPos));

	// return nothing, if already at line end
	if (nPos >= nCurrentLineEnd)
		return "";
	int nCurrentStyle;
	int nEndPos;

	// forward over all whitespace characters
	while (this->GetCharAt(nPos) == ' '
			|| this->GetCharAt(nPos) == '\t'
			|| this->GetCharAt(nPos) == '\r'
			|| this->GetCharAt(nPos) == '\n')
		nPos++;

	// return nothing, if already at line end (set position to line end)
	if (nPos >= nCurrentLineEnd)
	{
		nPos = nCurrentLineEnd;
		return "";
	}

	// get the current style
	nCurrentStyle = this->GetStyleAt(nPos);
	nEndPos = nPos;

	// while the style is identical forward the end position
	while (this->GetStyleAt(nEndPos) == nCurrentStyle)
	{
		nEndPos++;
	}

	// it is possible that we walked over the last position
	if (nEndPos > nCurrentLineEnd)
		return this->GetTextRange(nPos, nCurrentLineEnd);
	return this->GetTextRange(nPos, nEndPos);
}

void NumeReEditor::RemoveBreakpoint( int linenum )
{
	// need to remove the marker handle from the array - use
	// LineFromHandle on debug start and clean up then
	this->MarkerDelete(linenum, MARKER_BREAKPOINT);
	CreateBreakpointEvent(linenum, false);
	m_terminal->_guimessenger.removeBreakpoint(GetFileNameAndPath().ToStdString(), linenum);
}

int NumeReEditor::determineIndentationLevel(int nLine, int& singleLineIndent)
{
	if (getFileType() == FILE_NSCR || getFileType() == FILE_NPRC)
		return determineIndentationLevelNSCR(nLine, singleLineIndent);
	else if (getFileType() == FILE_MATLAB)
		return determineIndentationLevelMATLAB(nLine, singleLineIndent);
	else if (getFileType() == FILE_CPP)
		return determineIndentationLevelCPP(nLine, singleLineIndent);
	return 0;
}

int NumeReEditor::determineIndentationLevelNSCR(int nLine, int& singleLineIndent)
{
	int nIndentCount = 0;

	int nLineStart = this->PositionFromLine(nLine);
	int nLineEnd = this->GetLineEndPosition(nLine);

	for (int i = nLineStart; i < nLineEnd; i++)
	{
		if (this->GetStyleAt(i) == wxSTC_NSCR_COMMAND)
		{
			wxString word = this->GetTextRange(i, this->WordEndPosition(i + 1, true));
			if (word == "endif" || word == "endfor" || word == "endwhile" || word == "endcompose" || word == "endprocedure")
			{
				nIndentCount--;
			}
			else if (word == "if" || word == "for" || word == "while" || word == "compose" || word == "procedure")
			{
				nIndentCount++;
			}
			else if (word == "else" || word == "elseif")
			{
				singleLineIndent = -1;
			}
			i += word.length();
		}
		if (getFileType() == FILE_NSCR && this->GetStyleAt(i) == wxSTC_NSCR_INSTALL)
		{
			wxString word;
			if (this->GetCharAt(i) == '<' && this->FindText(i, nLineEnd, ">") != -1)
			{
				word = this->GetTextRange(i, this->WordEndPosition(i + 2, true) + 1);
				if (word == "<install>"
						|| word == "<info>"
						|| word == "<helpindex>"
						|| word == "<helpfile>"
						|| word == "<keywords>"
						|| word == "<keyword>"
						|| word == "<list>"
						|| word == "<codeblock>"
						|| word == "<exprblock>"
						|| word == "<article "
						|| word == "<item "
						|| word == "<list "
						|| word == "<example ")
					nIndentCount++;
				else if (word == "<endinstall>"
						 || word == "<endinfo>"
						 || word == "</helpindex>"
						 || word == "</helpfile>"
						 || word == "</article>"
						 || word == "</keywords>"
						 || word == "</keyword>"
						 || word == "</codeblock>"
						 || word == "</exprblock>"
						 || word == "</example>"
						 || word == "</item>"
						 || word == "</list>")
					nIndentCount--;
			}
			else
			{
				word = this->GetTextRange(i, this->WordEndPosition(i, true));
				if (word == "procedure"
						|| word == "if"
						|| word == "for"
						|| word == "while"
						|| word == "compose")
					nIndentCount++;
				else if (word == "endprocedure"
						 || word == "endif"
						 || word == "endfor"
						 || word == "endwhile"
						 || word == "endcompose")
					nIndentCount--;
				else if (word == "else" || word == "elseif")
					singleLineIndent = -1;
			}
			if (word.length())
				i += word.length() - 1;
		}
	}

	return nIndentCount;
}

int NumeReEditor::determineIndentationLevelMATLAB(int nLine, int& singleLineIndent)
{
	int nIndentCount = 0;

	int nLineStart = this->PositionFromLine(nLine);
	int nLineEnd = this->GetLineEndPosition(nLine);

	for (int i = nLineStart; i < nLineEnd; i++)
	{
		if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
		{
			wxString word = this->GetTextRange(i, this->WordEndPosition(i + 1, true));
			if (word == "end")
			{
				nIndentCount--;
			}
			else if (word == "if"
					 || word == "for"
					 || word == "while"
					 || word == "classdef"
					 || word == "function"
					 || word == "do"
					 || word == "try"
					 || word == "switch"
					 || word == "properties"
					 || word == "methods")
			{
				nIndentCount++;
			}
			else if (word == "else"
					 || word == "elseif"
					 || word == "catch"
					 || word == "case"
					 || word == "otherwise")
			{
				singleLineIndent = -1;
			}
			i += word.length();
		}
		if (this->GetStyleAt(i) == wxSTC_MATLAB_OPERATOR && this->GetTextRange(i, i + 3) == "...")
			singleLineIndent = 1;
	}

	return nIndentCount;
}

int NumeReEditor::determineIndentationLevelCPP(int nLine, int& singleLineIndent)
{
	int nIndentCount = 0;

	int nLineStart = this->PositionFromLine(nLine);
	int nLineEnd = this->GetLineEndPosition(nLine);

	for (int i = nLineStart; i < nLineEnd; i++)
	{
		if (this->GetStyleAt(i) == wxSTC_C_OPERATOR)
		{
			if (this->GetCharAt(i) == '{')
				nIndentCount++;
			if (this->GetCharAt(i) == '}')
				nIndentCount--;
		}
		if (this->GetStyleAt(i) == wxSTC_C_WORD)
		{
			wxString word = this->GetTextRange(i, this->WordEndPosition(i + 1, true));
			if (word == "private"
					|| word == "protected"
					|| word == "public"
					|| word == "case"
					|| word == "default")
			{
				singleLineIndent = -1;
			}
			if (word == "if" || word == "else" || word == "for" || word == "while")
			{
				singleLineIndent = 1;
			}
			i += word.length();
		}
	}

	return nIndentCount;
}

// This member function summarizes determining, which style type
// the selected character is using
bool NumeReEditor::isStyleType(StyleType _type, int nPos)
{
	switch (this->getFileType())
	{
		case FILE_NSCR:
		case FILE_NPRC:
			{
				switch (_type)
				{
					case STYLE_DEFAULT:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_DEFAULT;
					case STYLE_COMMENT_LINE:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_LINE;
					case STYLE_COMMENT_BLOCK:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_BLOCK;
					case STYLE_COMMENT_SECTION_LINE:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_LINE && this->GetTextRange(nPos, nPos + 3) == "##!";
					case STYLE_COMMENT_SECTION_BLOCK:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_BLOCK && this->GetTextRange(nPos, nPos + 3) == "#*!";
					case STYLE_COMMAND:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_COMMAND
							   || this->GetStyleAt(nPos) == wxSTC_NPRC_COMMAND;
					case STYLE_FUNCTION:
						return this->GetStyleAt(nPos) == wxSTC_NSCR_FUNCTION;
                    case STYLE_CUSTOMFUNCTION:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_CUSTOM_FUNCTION;
                    case STYLE_OPERATOR:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_OPERATORS;
                    case STYLE_PROCEDURE:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_PROCEDURES;
                    case STYLE_IDENTIFIER:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_IDENTIFIER;
                    case STYLE_NUMBER:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_NUMBERS;
                    case STYLE_STRINGPARSER:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_STRING_PARSER;
                    case STYLE_STRING:
                        return this->GetStyleAt(nPos) == wxSTC_NSCR_STRING;
				}
				break;
			}
		case FILE_MATLAB:
			{
				switch (_type)
				{
					case STYLE_DEFAULT:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_DEFAULT;
					case STYLE_COMMENT_LINE:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT;
					case STYLE_COMMENT_BLOCK:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT;
					case STYLE_COMMENT_SECTION_LINE:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT && this->GetTextRange(nPos, nPos + 2) == "%%";
					case STYLE_COMMENT_SECTION_BLOCK:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT && this->GetTextRange(nPos, nPos + 2) == "%%";
					case STYLE_COMMAND:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_KEYWORD;
					case STYLE_FUNCTION:
						return this->GetStyleAt(nPos) == wxSTC_MATLAB_FUNCTIONS;
                    case STYLE_CUSTOMFUNCTION:
                        return false;
                    case STYLE_OPERATOR:
                        return this->GetStyleAt(nPos) == wxSTC_MATLAB_OPERATOR;
                    case STYLE_PROCEDURE:
                        return false;
                    case STYLE_IDENTIFIER:
                        return this->GetStyleAt(nPos) == wxSTC_MATLAB_IDENTIFIER;
                    case STYLE_NUMBER:
                        return this->GetStyleAt(nPos) == wxSTC_MATLAB_NUMBER;
                    case STYLE_STRINGPARSER:
                        return false;
                    case STYLE_STRING:
                        return this->GetStyleAt(nPos) == wxSTC_MATLAB_STRING;
				}
				break;
			}
		case FILE_CPP:
			{
				switch (_type)
				{
					case STYLE_DEFAULT:
						return this->GetStyleAt(nPos) == wxSTC_C_DEFAULT;
					case STYLE_COMMENT_LINE:
						return this->GetStyleAt(nPos) == wxSTC_C_COMMENTLINE
							   || this->GetStyleAt(nPos) == wxSTC_C_COMMENTLINEDOC
							   || this->GetStyleAt(nPos) == wxSTC_C_COMMENTDOCKEYWORD;
					case STYLE_COMMENT_BLOCK:
						return this->GetStyleAt(nPos) == wxSTC_C_COMMENT
							   || this->GetStyleAt(nPos) == wxSTC_C_COMMENTDOC
							   || this->GetStyleAt(nPos) == wxSTC_C_COMMENTDOCKEYWORD;
					case STYLE_COMMENT_SECTION_LINE:
					case STYLE_COMMENT_SECTION_BLOCK:
						return false;
					case STYLE_COMMAND:
						return this->GetStyleAt(nPos) == wxSTC_C_WORD;
					case STYLE_FUNCTION:
						return this->GetStyleAt(nPos) == wxSTC_C_WORD2;
                    case STYLE_CUSTOMFUNCTION:
                        return false;
                    case STYLE_OPERATOR:
                        return this->GetStyleAt(nPos) == wxSTC_C_OPERATOR;
                    case STYLE_PROCEDURE:
                        return false;
                    case STYLE_IDENTIFIER:
                        return this->GetStyleAt(nPos) == wxSTC_C_IDENTIFIER;
                    case STYLE_NUMBER:
                        return this->GetStyleAt(nPos) == wxSTC_C_NUMBER;
                    case STYLE_STRINGPARSER:
                        return false;
                    case STYLE_STRING:
                        return this->GetStyleAt(nPos) == wxSTC_C_STRING;
				}
				break;
			}
		default:
			return false;
	}
	return false;
}

int NumeReEditor::countUmlauts(const string& sStr)
{
	int nUmlauts = 0;
	for (size_t i = 0; i < sStr.length(); i++)
	{
		if (sStr[i] == 'Ä'
				|| sStr[i] == 'ä'
				|| sStr[i] == 'Ö'
				|| sStr[i] == 'ö'
				|| sStr[i] == 'Ü'
				|| sStr[i] == 'ü'
				|| sStr[i] == 'ß'
				|| sStr[i] == '°'
				|| sStr[i] == 'µ'
				|| sStr[i] == (char)142
				|| sStr[i] == (char)132
				|| sStr[i] == (char)153
				|| sStr[i] == (char)148
				|| sStr[i] == (char)154
				|| sStr[i] == (char)129
				|| sStr[i] == (char)225
				|| sStr[i] == (char)167
				|| sStr[i] == (char)230
		   )
			nUmlauts++;
	}
	return nUmlauts;
}

string NumeReEditor::realignLangString(string sLine, size_t& lastpos)
{
	lastpos = sLine.find(' ');
	if (lastpos == string::npos)
		return sLine;
	size_t firstpos = sLine.find_first_not_of(' ', lastpos);
	if (sLine.find(')') < lastpos || sLine.find('.') < lastpos)
	{
		sLine.replace(lastpos, firstpos - lastpos, " -> ");
	}
	else
	{
		if (sLine.find("- ") == firstpos)
			return sLine;
		if (firstpos - lastpos > 2)
		{
			sLine.erase(lastpos, firstpos - lastpos - 2);
			sLine.insert(sLine.find("- "), firstpos - lastpos - 2, ' ');
		}
	}
	return sLine;
}

// This member function adds linebreaks at the maximal line length
// of 100 characters. It is used for the tooltips of functions, commands
// and procedures
string NumeReEditor::addLinebreaks(const string& sLine, bool onlyDocumentation /* = false*/)
{
	const unsigned int nMAXLINE = 100;

	string sReturn = sLine;

	// Remove escaped dollar signs
	while (sReturn.find("\\$") != string::npos)
	{
		sReturn.erase(sReturn.find("\\$"), 1);
	}
	unsigned int nDescStart = sReturn.find("- ");
	unsigned int nIndentPos = 4;
	unsigned int nLastLineBreak = 0;
    bool isItemize = false;

    // Handle the first indent depending on whether this is
    // only a documentation string or a whole definition
	if (onlyDocumentation)
    {
        nDescStart = 0;
        sReturn.insert(0, "    ");
    }
    else
        sReturn.replace(nDescStart, 2, "\n    ");

	if (nDescStart == string::npos)
		return sLine;

	nLastLineBreak = nDescStart;

	for (unsigned int i = nDescStart; i < sReturn.length(); i++)
	{
		if (sReturn[i] == '\n')
        {
			nLastLineBreak = i;
            if (sReturn.substr(i, 7) == "\n    - ")
                isItemize = true;
            else
                isItemize = false;
        }

		if ((i == nMAXLINE && !nLastLineBreak)
				|| (nLastLineBreak && i - nLastLineBreak == nMAXLINE))
		{
			for (int j = i; j >= 0; j--)
			{
				if (sReturn[j] == ' ')
				{
					sReturn[j] = '\n';
					sReturn.insert(j + 1, nIndentPos + 2*isItemize, ' ');
					nLastLineBreak = j;
					break;
				}
				else if (sReturn[j] == '-' && j != (int)i)
				{
					// --> Minuszeichen: nicht immer ist das Trennen an dieser Stelle sinnvoll. Wir pruefen die einfachsten Faelle <--
					if (j &&
							(sReturn[j - 1] == ' '
							 || sReturn[j - 1] == '('
							 || sReturn[j + 1] == ')'
							 || sReturn[j - 1] == '['
							 || (sReturn[j + 1] >= '0' && sReturn[j + 1] <= '9')
							 || sReturn[j + 1] == ','
							 || (sReturn[j + 1] == '"' && sReturn[j - 1] == '"')
							))
						continue;
					sReturn.insert(j + 1, "\n");
					sReturn.insert(j + 2, nIndentPos + 2*isItemize, ' ');
					nLastLineBreak = j + 1;
					break;
				}
				else if (sReturn[j] == ',' && j != (int)i && sReturn[j + 1] != ' ')
				{
					sReturn.insert(j + 1, "\n");
					sReturn.insert(j + 2, nIndentPos + 2*isItemize, ' ');
					nLastLineBreak = j + 1;
					break;
				}
			}
		}
	}
	return sReturn;
}

AnnotationCount NumeReEditor::addToAnnotation(string& sCurrentLine, string& sStyles, const string& sMessage, int nStyle)
{
	AnnotationCount annoCount;
	int chartoadd = 0;
	// Do not show the same message multiple times
	if (sCurrentLine.find(sMessage) != string::npos
			&& (!sCurrentLine.find(sMessage) || sCurrentLine[sCurrentLine.find(sMessage) - 1] == '\n'))
		return annoCount;
	if (sCurrentLine.length())
	{
		sCurrentLine += "\n";
		chartoadd++;
	}
	sCurrentLine += sMessage;
	chartoadd += countUmlauts(sMessage);

	sStyles.append(sMessage.length() + chartoadd, nStyle);
	if (nStyle == ANNOTATION_NOTE)
		annoCount.nNotes++;
	else if (nStyle == ANNOTATION_WARN)
		annoCount.nWarnings++;
	else if (nStyle == ANNOTATION_ERROR)
		annoCount.nErrors++;
	return annoCount;
}

string NumeReEditor::getTextCoordsAsString(int nPos)
{
	string sCoords = "Pos. ";
	int nLine = this->LineFromPosition(nPos);
	sCoords += toString(nPos - this->PositionFromLine(nLine) + 1);
	return sCoords;
}

string NumeReEditor::constructSyntaxElementForAnalyzer(const string& sElement, int nPos, int nLength)
{
    this->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
    this->IndicatorFillRange(nPos, nLength);
	return sElement + " ";
}

bool NumeReEditor::containsAssignment(const string& sCurrentLine)
{
	if (sCurrentLine.find('=') == string::npos)
		return false;
	for (size_t i = 1; i < sCurrentLine.length() - 1; i++)
	{
		if (sCurrentLine[i] == '='
				&& sCurrentLine[i - 1] != '<'
				&& sCurrentLine[i - 1] != '>'
				&& sCurrentLine[i - 1] != '!'
				&& sCurrentLine[i - 1] != '='
				&& sCurrentLine[i + 1] != '='
				&& !isInQuotes(sCurrentLine, i))
			return true;
	}
	return false;
}

void NumeReEditor::ApplyAutoIndentation(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
	if (nFirstLine < 0)
		nFirstLine = 0;
	if (nLastLine <= 0 || nLastLine > this->GetLineCount())
		nLastLine = this->GetLineCount();

	int nIndentCount = 0;
	int nCurrentIndent = 0;

	int singleLineIndent = 0;
	int nLastSingleIndent = 0;
	this->SetTabWidth(4);
	this->BeginUndoAction();
	for (int i = nFirstLine; i < nLastLine; i++)
	{
		nLastSingleIndent = singleLineIndent;
		singleLineIndent = 0;
		int pos = this->PositionFromLine(i);
		if (isStyleType(STYLE_COMMENT_LINE, pos))
			continue;
		while (isStyleType(STYLE_COMMENT_BLOCK, pos) && pos < this->GetLineEndPosition(nLastLine))
		{
			pos++;
		}
		if (pos > this->GetLineEndPosition(i) - 1)
			continue;
		nCurrentIndent = determineIndentationLevel(i, singleLineIndent);
		if (this->getFileType() == FILE_CPP && nLastSingleIndent && nCurrentIndent)
			nLastSingleIndent = 0;
		if (!nCurrentIndent && singleLineIndent < 0)
		{
			singleLineIndent = 0;
			this->SetLineIndentation(i, 4 * (nIndentCount - 1));
		}
		else if (!nCurrentIndent)
		{
			this->SetLineIndentation(i, 4 * (nIndentCount + nLastSingleIndent));
		}
		else if (nCurrentIndent < 0)
		{
			nIndentCount += nCurrentIndent;
			this->SetLineIndentation(i, 4 * nIndentCount);
		}
		else
		{
			this->SetLineIndentation(i, 4 * nIndentCount);
			nIndentCount += nCurrentIndent;
		}
		if (this->GetCurrentLine() == i
				&& this->GetCurrentPos() - pos < nIndentCount
				&& !this->HasSelection()
				&& this->GetTextRange(pos, this->GetLineEndPosition(i)).find_first_not_of(" \t\n\r") == string::npos)
			this->GotoPos(pos + nIndentCount);
	}
	this->EndUndoAction();
}

void NumeReEditor::ApplyAutoFormat(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
	if (this->getFileType() == FILE_NSCR || this->getFileType() == FILE_NPRC)
		ApplyAutoFormatNSCR(nFirstLine, nLastLine);
	else if (this->getFileType() == FILE_MATLAB)
		ApplyAutoFormatMATLAB(nFirstLine, nLastLine);
	else if (this->getFileType() == FILE_CPP)
		ApplyAutoFormatCPP(nFirstLine, nLastLine);
	ApplyAutoIndentation(nFirstLine, this->LineFromPosition(this->GetLineEndPosition(nLastLine)));
}

void NumeReEditor::ApplyAutoFormatNSCR(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
	if (this->getFileType() != FILE_NSCR && this->getFileType() != FILE_NPRC)
		return;
	this->BeginUndoAction();
	if (nFirstLine < 0)
		nFirstLine = 0;
	if (nLastLine <= 0 || nLastLine > this->GetLineCount())
		nLastLine = this->GetLineCount();

	int nFirstPosition = this->PositionFromLine(nFirstLine);
	int nLastPosition = this->GetLineEndPosition(nLastLine);
	int nIndentationLevel = (this->getFileType() == FILE_NPRC) ? -1 : 0;

	for (int i = nFirstPosition; i < nLastPosition; i++)
	{
		if (this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_BLOCK || this->GetStyleAt(i) == wxSTC_NSCR_COMMENT_LINE)
			continue;
		if (this->GetCharAt(i) == '\r' || this->GetCharAt(i) == '\n')
			continue;
		if (this->GetStyleAt(i) == wxSTC_NSCR_OPERATOR_KEYWORDS || (this->GetStyleAt(i) == wxSTC_NSCR_OPERATORS && this->GetTextRange(i, i + 1) == "<>"))
		{
			for (; i < nLastPosition; i++)
			{
				if ((!this->GetStyleAt(i) == wxSTC_NSCR_STRING && (this->GetCharAt(i) == ' ' || this->GetCharAt(i) == ';')) || this->GetCharAt(i) == '\r' || this->GetCharAt(i) == '\n')
					break;
			}
		}
		if (this->GetStyleAt(i) == wxSTC_NSCR_OPERATORS)
		{
			int currentChar = this->GetCharAt(i);
			int prevChar = this->GetCharAt(i - 1);
			int nextChar = this->GetCharAt(i + 1);
			string sParens = "(){}[]";
			if (currentChar == '(' && this->GetStyleAt(i - 1) == wxSTC_NSCR_COMMAND)
				nLastPosition += insertTextAndMove(i, " ");
			else if (currentChar == '('
					 && (this->GetStyleAt(i - 1) == wxSTC_NSCR_FUNCTION
						 || this->GetStyleAt(i - 1) == wxSTC_NSCR_CUSTOM_FUNCTION
						 || this->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES
						 || this->GetStyleAt(i - 1) == wxSTC_NSCR_METHOD
						 || this->GetStyleAt(i - 1) == wxSTC_NSCR_PREDEFS))
			{
				int nParens = this->BraceMatch(i);
				if (nParens > 0)
				{
					for (; i < nParens; i++)
					{
						if (this->GetStyleAt(i) == wxSTC_NSCR_OPERATORS)
						{
							currentChar = this->GetCharAt(i);
							nextChar = this->GetCharAt(i + 1);
							prevChar = this->GetCharAt(i - 1);
							if (currentChar == ',' && nextChar != ' ')
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && nextChar != ' ' && this->GetStyleAt(i + 1) != wxSTC_NSCR_OPERATORS)
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && prevChar != ' ' && this->GetStyleAt(i - 1) != wxSTC_NSCR_OPERATORS)
							{
								nLastPosition += insertTextAndMove(i, " ");
								nParens++;
							}
						}
					}
				}
			}
			else if (sParens.find(currentChar) != string::npos)
				continue;
			else if (currentChar == ',' && nextChar != ' ')
				nLastPosition += insertTextAndMove(i + 1, " ");
			else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
			{
				if (nextChar != ' ')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '<' || currentChar == '>' || currentChar == '!' || currentChar == '=')
			{
				string sLeadingChars = " (=+-!*/^<>:";
				if (currentChar == '='
						&& (this->GetStyleAt(i - 1) == wxSTC_NSCR_OPTION
							|| this->GetStyleAt(i - 1) == wxSTC_NSCR_OPTION
							|| this->GetStyleAt(i - 1) == wxSTC_NSCR_COMMAND))
					continue;
				if (nextChar != ' ' && nextChar != '=' && nextChar != '>' && currentChar != '!' && !(currentChar == '>' && prevChar == '<') && this->GetStyleAt(i - 1) != wxSTC_NSCR_OPTION  && this->GetStyleAt(i + 1) != wxSTC_NSCR_OPTION)
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (sLeadingChars.find(prevChar) == string::npos && !(currentChar == '<' && nextChar == '>'))
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '+' || currentChar == '-')
			{
				if (nextChar != ' '
						&& nextChar != currentChar
						&& nextChar != '='
						&& nextChar != ';'
						&& this->GetStyleAt(i + 1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i + 1) != wxSTC_NSCR_OPTION
						&& this->GetStyleAt(i - 1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i - 1) != wxSTC_NSCR_OPTION
						&& prevChar != '('
						&& prevChar != '{'
						&& prevChar != '['
						&& this->GetCharAt(i - 2) != ','
						&& this->GetCharAt(i - 2) != '=')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' '
						&& prevChar != currentChar
						&& nextChar != currentChar
						&& prevChar != '('
						&& prevChar != '['
						&& prevChar != '{')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '&' || currentChar == '|')
			{
				if (nextChar != ' ' && nextChar != currentChar)
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ' && prevChar != currentChar)
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if ((currentChar == '*' || currentChar == '/' || currentChar == '^')
					 && nextChar == '='
					 && prevChar != ' '
					 && nextChar != currentChar
					 && prevChar != currentChar)
				nLastPosition += insertTextAndMove(i, " ");
		}
		if (this->GetStyleAt(i) == wxSTC_NSCR_COMMAND)
		{
			int nPos1 = i;
			while (this->GetStyleAt(i + 1) == wxSTC_NSCR_COMMAND)
				i++;
			wxString command = this->GetTextRange(nPos1, i + 1);
			int nCurrentLineStart = this->PositionFromLine(this->LineFromPosition(nPos1));
			int nCurrentLineEnd = this->GetLineEndPosition(this->LineFromPosition(nPos1));
			if (command == "global" || command == "load" || command == "append")
			{
				i = nCurrentLineEnd;
				continue;
			}
			else if (command == "set")
			{
				for (; i <= nCurrentLineEnd; i++)
				{
					if (this->GetStyleAt(i) == wxSTC_NSCR_OPERATORS && this->GetCharAt(i) == '[')
					{
						int bracepos = this->BraceMatch(i);
						if (bracepos > 0)
						{
							for (; i < bracepos; i++)
							{
								if (this->GetCharAt(i) == ',' && this->GetCharAt(i + 1) != ' ')
									nLastPosition += insertTextAndMove(i + 1, " ");
							}
						}
					}
				}
			}

			if (command == "if"
					|| command == "elseif"
					|| command == "for"
					|| command == "while")
			{
				int parens = i;
				parens = FindText(i, nCurrentLineEnd, "(");
				if (parens > 0)
				{
					parens = this->BraceMatch(parens);
					if (parens > 0)
					{
						if (this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos
								&& !isStyleType(STYLE_COMMENT_LINE, parens + 1 + this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t"))
								&& !isStyleType(STYLE_COMMENT_BLOCK, parens + 1 + this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t")))
							nLastPosition += insertTextAndMove(parens + 1, "\r\n");
					}
				}
			}
			else if (command == "else")
			{
				if (this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos
						&& !isStyleType(STYLE_COMMENT_LINE, i + 1 + this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t"))
						&& !isStyleType(STYLE_COMMENT_BLOCK, i + 1 + this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t")))
					nLastPosition += insertTextAndMove(i + 1, "\r\n");
			}
			if (command == "if"
					|| command == "for"
					|| command == "else"
					|| command == "elseif"
					|| command == "while"
					|| command == "endif"
					|| command == "endfor"
					|| command == "endwhile")
			{
				if (this->GetTextRange(nCurrentLineStart, nPos1).find_first_not_of(" \t") != string::npos)
				{
					nLastPosition += insertTextAndMove(nPos1, "\r\n");
					i += 2;
				}
			}
			if (command == "if" || command == "for" || command == "while" || command == "compose" || command == "procedure")
			{
				if (nIndentationLevel <= 0)
				{
					int nLine = this->LineFromPosition(i);
					int position = this->PositionFromLine(nLine - 1);
					while (this->GetCharAt(position) == ' ' || this->GetCharAt(position) == '\t')
						position++;
					if (nLine
							&& this->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != string::npos
							&& this->GetStyleAt(this->PositionFromLine(nLine - 1)) != wxSTC_NSCR_COMMENT_BLOCK
							&& this->GetStyleAt(this->PositionFromLine(nLine - 1)) != wxSTC_NSCR_COMMENT_LINE
							&& this->GetStyleAt(position) != wxSTC_NSCR_COMMENT_LINE)
					{
						nLastPosition += insertTextAndMove(this->PositionFromLine(nLine), "\r\n");
						i += 2;
					}
				}
				nIndentationLevel++;
			}
			if (command == "endif" || command == "endfor" || command == "endwhile" || command == "endcompose" || command == "endprocedure")
			{
				nIndentationLevel--;
				if (nIndentationLevel <= 0)
				{
					int nLine = this->LineFromPosition(i);
					if (nLine < this->GetLineCount() - 1
							&& this->GetLine(nLine + 1).find_first_not_of(" \t\r\n") != string::npos)
					{
						nLastPosition += insertTextAndMove(this->PositionFromLine(nLine + 1), "\r\n");
					}
				}
			}
		}
		if (this->GetStyleAt(i) == wxSTC_NSCR_STRING)
		{
			if (this->GetStyleAt(i + 1) != wxSTC_NSCR_STRING
					&& this->GetLineEndPosition(this->LineFromPosition(i)) != i + 1
					&& this->GetCharAt(i + 1) != ' '
					&& this->GetCharAt(i + 1) != ','
					&& this->GetCharAt(i + 1) != ';'
					&& this->GetCharAt(i + 1) != ')'
					&& this->GetCharAt(i + 1) != ']'
					&& this->GetCharAt(i + 1) != '}')
				nLastPosition += insertTextAndMove(i + 1, " ");
			if (this->GetStyleAt(i - 1) != wxSTC_NSCR_STRING
					&& this->PositionFromLine(this->LineFromPosition(i)) != i
					&& this->GetCharAt(i - 1) != ' '
					&& !(this->GetCharAt(i - 1) == '=' && this->GetStyleAt(i - 2) == wxSTC_NSCR_OPTION)
					&& this->GetCharAt(i - 1) != '('
					&& this->GetCharAt(i - 1) != '['
					&& this->GetCharAt(i - 1) != '{')
				nLastPosition += insertTextAndMove(i, " ");
		}
	}
	this->EndUndoAction();
}

void NumeReEditor::ApplyAutoFormatMATLAB(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
	if (this->getFileType() != FILE_MATLAB)
		return;
	this->BeginUndoAction();
	if (nFirstLine < 0)
		nFirstLine = 0;
	if (nLastLine <= 0 || nLastLine > this->GetLineCount())
		nLastLine = this->GetLineCount();

	int nFirstPosition = this->PositionFromLine(nFirstLine);
	int nLastPosition = this->GetLineEndPosition(nLastLine);
	int nIndentationLevel = -1;

	for (int i = nFirstPosition; i < nLastPosition; i++)
	{
		if (this->GetStyleAt(i) == wxSTC_MATLAB_COMMENT)
			continue;
		if (this->GetCharAt(i) == '\r' || this->GetCharAt(i) == '\n')
			continue;
		if (this->GetStyleAt(i) == wxSTC_MATLAB_OPERATOR)
		{
			int currentChar = this->GetCharAt(i);
			int prevChar = this->GetCharAt(i - 1);
			int nextChar = this->GetCharAt(i + 1);
			string sParens = "(){}[]";
			if (currentChar == '('
					&& (this->GetStyleAt(i - 1) == wxSTC_MATLAB_IDENTIFIER
						|| this->GetStyleAt(i - 1) == wxSTC_MATLAB_KEYWORD))
			{
				int nParens = this->BraceMatch(i);
				if (nParens > 0)
				{
					for (; i < nParens; i++)
					{
						if (this->GetStyleAt(i) == wxSTC_MATLAB_OPERATOR)
						{
							currentChar = this->GetCharAt(i);
							nextChar = this->GetCharAt(i + 1);
							prevChar = this->GetCharAt(i - 1);
							if (currentChar == ',' && nextChar != ' ')
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && nextChar != ' ' && this->GetStyleAt(i + 1) != wxSTC_MATLAB_OPERATOR)
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && prevChar != ' ' && this->GetStyleAt(i - 1) != wxSTC_MATLAB_OPERATOR)
							{
								nLastPosition += insertTextAndMove(i, " ");
								nParens++;
							}
						}
					}
				}
			}
			else if (sParens.find(currentChar) != string::npos)
				continue;
			else if (currentChar == ',' && nextChar != ' ')
				nLastPosition += insertTextAndMove(i + 1, " ");
			else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
			{
				if (nextChar != ' ')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '<' || currentChar == '>' || currentChar == '~' || currentChar == '=')
			{
				string sLeadingChars = " (=+-*/^<>:~";
				if (nextChar != ' ' && nextChar != '=' && nextChar != '>' && currentChar != '~' && !(currentChar == '>' && prevChar == '<'))
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (sLeadingChars.find(prevChar) == string::npos && !(currentChar == '<' && nextChar == '>'))
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '+' || currentChar == '-')
			{
				if (nextChar != ' '
						&& nextChar != currentChar
						&& nextChar != '='
						&& nextChar != ';'
						/*&& this->GetStyleAt(i+1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i+1) != wxSTC_NSCR_OPTION
						&& this->GetStyleAt(i-1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i-1) != wxSTC_NSCR_OPTION*/
						&& prevChar != '('
						&& prevChar != '{'
						&& prevChar != '['
						&& this->GetCharAt(i - 2) != ','
						&& this->GetCharAt(i - 2) != '=')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' '
						&& prevChar != currentChar
						&& nextChar != currentChar
						&& prevChar != '('
						&& prevChar != '['
						&& prevChar != '{')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '&' || currentChar == '|')
			{
				if (nextChar != ' ' && nextChar != currentChar)
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ' && prevChar != currentChar)
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if ((currentChar == '*' || currentChar == '/' || currentChar == '^')
					 && nextChar == '='
					 && prevChar != ' '
					 && nextChar != currentChar
					 && prevChar != currentChar)
				nLastPosition += insertTextAndMove(i, " ");
		}
		if (this->GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
		{
			int nPos1 = i;
			while (this->GetStyleAt(i + 1) == wxSTC_MATLAB_KEYWORD)
				i++;

			wxString command = this->GetTextRange(nPos1, i + 1);
			int nCurrentLineStart = this->PositionFromLine(this->LineFromPosition(nPos1));
			int nCurrentLineEnd = this->GetLineEndPosition(this->LineFromPosition(nPos1));

			if (command == "if"
					|| command == "elseif"
					|| command == "switch"
					|| command == "case"
					|| command == "for"
					|| command == "while")
			{
				if (this->GetCharAt(i + 1) != ' ')
					nCurrentLineEnd += insertTextAndMove(i + 1, " ");
				int parens = i + 1;
				while (this->GetCharAt(parens) == ' ')
					parens++;
				if (parens != '(' && parens != '[' && parens != '{')
					parens = -1;
				if (parens > 0)
				{
					parens = this->BraceMatch(parens);
					if (parens > 0)
					{
						if (this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
							nLastPosition += insertTextAndMove(parens + 1, "\r\n");
					}
				}
				else
				{
					parens = this->FindText(i, nCurrentLineEnd, ";");
					if (parens > 0)
					{
						if (this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
							nLastPosition += insertTextAndMove(parens + 1, "\r\n");
					}
				}
			}
			else if (command == "else")
			{
				if (this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos)
					nLastPosition += insertTextAndMove(i + 1, "\r\n");
			}
			if (command == "if"
					|| command == "for"
					|| command == "else"
					|| command == "elseif"
					|| command == "switch"
					|| command == "case"
					|| command == "do"
					|| command == "otherwise"
					|| command == "try"
					|| command == "catch"
					|| command == "until"
					|| command == "while"
					|| command == "end")
			{
				if (this->GetTextRange(nCurrentLineStart, nPos1).find_first_not_of(" \t") != string::npos)
				{
					nLastPosition += insertTextAndMove(nPos1, "\r\n");
					i += 2;
				}
			}
			if (command == "if"
					|| command == "for"
					|| command == "while"
					|| command == "try"
					|| command == "do"
					|| command == "function"
					|| command == "switch"
					|| command == "case")
			{
				if (nIndentationLevel <= 0)
				{
					int nLine = this->LineFromPosition(i);
					int position = this->PositionFromLine(nLine - 1);
					while (this->GetCharAt(position) == ' ' || this->GetCharAt(position) == '\t')
						position++;
					if (nLine
							&& this->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != string::npos
							&& this->GetStyleAt(this->PositionFromLine(nLine - 1)) != wxSTC_MATLAB_COMMENT
							&& this->GetStyleAt(position) != wxSTC_MATLAB_COMMENT)
					{
						nLastPosition += insertTextAndMove(this->PositionFromLine(nLine), "\r\n");
						i += 2;
					}
				}
				nIndentationLevel++;
			}
			if (command == "end")
			{
				nIndentationLevel--;
				if (nIndentationLevel <= 0)
				{
					int nLine = this->LineFromPosition(i);
					if (nLine < this->GetLineCount() - 1
							&& this->GetLine(nLine + 1).find_first_not_of(" \t\r\n") != string::npos)
					{
						nLastPosition += insertTextAndMove(this->PositionFromLine(nLine + 1), "\r\n");
					}
				}
			}
		}
		if (this->GetStyleAt(i) == wxSTC_MATLAB_STRING)
		{
			if (this->GetStyleAt(i + 1) != wxSTC_MATLAB_STRING
					&& this->GetLineEndPosition(this->LineFromPosition(i)) != i + 1
					&& this->GetCharAt(i + 1) != ' '
					&& this->GetCharAt(i + 1) != ','
					&& this->GetCharAt(i + 1) != ';'
					&& this->GetCharAt(i + 1) != ')'
					&& this->GetCharAt(i + 1) != ']'
					&& this->GetCharAt(i + 1) != '}')
				nLastPosition += insertTextAndMove(i + 1, " ");
			if (this->GetStyleAt(i - 1) != wxSTC_MATLAB_STRING
					&& this->PositionFromLine(this->LineFromPosition(i)) != i
					&& this->GetCharAt(i - 1) != ' '
					&& this->GetCharAt(i - 1) != '('
					&& this->GetCharAt(i - 1) != '['
					&& this->GetCharAt(i - 1) != '{')
				nLastPosition += insertTextAndMove(i, " ");
		}
	}
	this->EndUndoAction();
}

void NumeReEditor::ApplyAutoFormatCPP(int nFirstLine, int nLastLine) // int nFirstLine = 0, int nLastLine = -1
{
	if (this->getFileType() != FILE_CPP)
		return;
	this->BeginUndoAction();
	if (nFirstLine < 0)
		nFirstLine = 0;
	if (nLastLine <= 0 || nLastLine > this->GetLineCount())
		nLastLine = this->GetLineCount();

	int nFirstPosition = this->PositionFromLine(nFirstLine);
	int nLastPosition = this->GetLineEndPosition(nLastLine);
	int nIndentationLevel = -1;

	for (int i = nFirstPosition; i < nLastPosition; i++)
	{
		if (isStyleType(STYLE_COMMENT_BLOCK, i) || isStyleType(STYLE_COMMENT_LINE, i))
			continue;
		if (this->GetCharAt(i) == '\r' || this->GetCharAt(i) == '\n')
			continue;

		if (this->GetStyleAt(i) == wxSTC_C_OPERATOR)
		{
			int currentChar = this->GetCharAt(i);
			int prevChar = this->GetCharAt(i - 1);
			int nextChar = this->GetCharAt(i + 1);
			int nCurrentLineStart = this->PositionFromLine(this->LineFromPosition(i));
			int nCurrentLineEnd = this->GetLineEndPosition(this->LineFromPosition(i));
			string sParens = "()[]";
			if (currentChar == '-' && nextChar == '>')
			{
				i++;
				continue;
			}

			if (currentChar == '(' && this->GetStyleAt(i - 1) == wxSTC_C_WORD)
				nLastPosition += insertTextAndMove(i, " ");
			else if (currentChar == '('
					 && (this->GetStyleAt(i - 1) == wxSTC_C_WORD2))
			{
				int nParens = this->BraceMatch(i);
				if (nParens > 0)
				{
					for (; i < nParens; i++)
					{
						if (this->GetStyleAt(i) == wxSTC_C_OPERATOR)
						{
							currentChar = this->GetCharAt(i);
							nextChar = this->GetCharAt(i + 1);
							prevChar = this->GetCharAt(i - 1);
							if (currentChar == ',' && nextChar != ' ')
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && nextChar != ' ' && this->GetStyleAt(i + 1) != wxSTC_C_OPERATOR)
							{
								nLastPosition += insertTextAndMove(i + 1, " ");
								nParens++;
							}
							if (currentChar == '=' && prevChar != ' ' && this->GetStyleAt(i - 1) != wxSTC_C_OPERATOR)
							{
								nLastPosition += insertTextAndMove(i, " ");
								nParens++;
							}
						}
					}
				}
			}
			else if (sParens.find(currentChar) != string::npos)
				continue;
			else if (currentChar == '{' || currentChar == '}')
			{
				if (currentChar == '{')
					nIndentationLevel++;
				if (currentChar == '}')
					nIndentationLevel--;
				if (this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \t\r\n") != string::npos)
				{
					nLastPosition += 2;
					insertTextAndMove(i + 1, "\r\n");
				}
				if (this->GetTextRange(nCurrentLineStart, i).find_first_not_of(" \t") != string::npos)
				{
					nLastPosition += 2;
					i += insertTextAndMove(i, "\r\n");
				}
			}
			else if (currentChar == ',' && nextChar != ' ')
				nLastPosition += insertTextAndMove(i + 1, " ");
			else if (currentChar == '?' && nextChar != currentChar && prevChar != currentChar)
			{
				if (nextChar != ' ')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '<' || currentChar == '>' || currentChar == '!' || currentChar == '=')
			{
				string sLeadingChars = " (=+-!*/^<>:";
				if (currentChar == '<' && (this->GetStyleAt(i - 1) == wxSTC_C_WORD || this->GetStyleAt(i - 1) == wxSTC_C_WORD))
					continue;
				if (currentChar == '>' && (this->GetStyleAt(i - 1) == wxSTC_C_WORD || this->GetStyleAt(i + 1) == wxSTC_C_WORD))
					continue;
				if (nextChar != ' ' && nextChar != '=' && nextChar != '>' && currentChar != '!' && !(currentChar == '>' && prevChar == '<') && this->GetStyleAt(i - 1) != wxSTC_NSCR_OPTION  && this->GetStyleAt(i + 1) != wxSTC_NSCR_OPTION)
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (sLeadingChars.find(prevChar) == string::npos && !(currentChar == '<' && nextChar == '>'))
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '+' || currentChar == '-')
			{
				if (nextChar != ' '
						&& nextChar != currentChar
						&& nextChar != '='
						&& nextChar != ';'
						/*&& this->GetStyleAt(i+1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i+1) != wxSTC_NSCR_OPTION
						&& this->GetStyleAt(i-1) != wxSTC_NSCR_COMMAND
						&& this->GetStyleAt(i-1) != wxSTC_NSCR_OPTION*/
						&& prevChar != '('
						&& prevChar != '{'
						&& prevChar != '['
						&& this->GetCharAt(i - 2) != ','
						&& this->GetCharAt(i - 2) != '=')
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' '
						&& prevChar != currentChar
						&& nextChar != currentChar
						&& prevChar != '('
						&& prevChar != '['
						&& prevChar != '{')
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if (currentChar == '&' || currentChar == '|')
			{
				if (nextChar != ' ' && nextChar != currentChar)
					nLastPosition += insertTextAndMove(i + 1, " ");
				if (prevChar != ' ' && prevChar != currentChar)
					nLastPosition += insertTextAndMove(i, " ");
			}
			else if ((currentChar == '*' || currentChar == '/' || currentChar == '^')
					 && nextChar == '='
					 && prevChar != ' '
					 && nextChar != currentChar
					 && prevChar != currentChar)
				nLastPosition += insertTextAndMove(i, " ");
		}
		if (this->GetStyleAt(i) == wxSTC_C_WORD)
		{
			int nPos1 = i;
			while (this->GetStyleAt(i + 1) == wxSTC_C_WORD)
				i++;
			wxString command = this->GetTextRange(nPos1, i + 1);
			int nCurrentLineStart = this->PositionFromLine(this->LineFromPosition(nPos1));
			int nCurrentLineEnd = this->GetLineEndPosition(this->LineFromPosition(nPos1));

			if (command == "if"
					|| command == "switch"
					|| command == "for"
					|| command == "while")
			{
				int parens = i;
				parens = FindText(i, nCurrentLineEnd, "(");
				if (parens > 0)
				{
					parens = this->BraceMatch(parens);
					if (parens > 0)
					{
						if (this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t;") != string::npos
								&& !isStyleType(STYLE_COMMENT_LINE, parens + 1 + this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t"))
								&& !isStyleType(STYLE_COMMENT_BLOCK, parens + 1 + this->GetTextRange(parens + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t")))
							nLastPosition += insertTextAndMove(parens + 1, "\r\n");
					}
				}
			}
			else if (command == "else")
			{
				if (this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t") != string::npos
						&& !isStyleType(STYLE_COMMAND, i + 1 + this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t"))
						&& !isStyleType(STYLE_COMMENT_LINE, i + 1 + this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t"))
						&& !isStyleType(STYLE_COMMENT_BLOCK, i + 1 + this->GetTextRange(i + 1, nCurrentLineEnd).find_first_not_of(" \r\n\t")))
					nLastPosition += insertTextAndMove(i + 1, "\r\n");
			}
			/*if (command == "if"
			    || command == "for"
			    || command == "else"
			    || command == "elseif"
			    || command == "while"
			    || command == "endif"
			    || command == "endfor"
			    || command == "endwhile")
			{
			    if (this->GetTextRange(nCurrentLineStart, nPos1).find_first_not_of(" \t") != string::npos)
			    {
			        nLastPosition += insertTextAndMove(nPos1, "\r\n");
			        i += 2;
			    }
			}*/
			if (command == "if"
					|| command == "for"
					|| command == "while"
					|| command == "do"
					|| command == "try")
			{
				if (command == "while" && this->GetTextRange(i + 1, nCurrentLineEnd).find(';') != string::npos)
					continue;
				if (command == "if" && this->GetTextRange(nCurrentLineStart, i).find("else") != string::npos)
					continue;
				if (nIndentationLevel <= 0)
				{
					int nLine = this->LineFromPosition(i);
					int position = this->PositionFromLine(nLine - 1);
					while (this->GetCharAt(position) == ' ' || this->GetCharAt(position) == '\t')
						position++;
					if (nLine
							&& this->GetLine(nLine - 1).find_first_not_of(" \t\r\n") != string::npos
							&& !isStyleType(STYLE_COMMENT_BLOCK, this->PositionFromLine(nLine - 1))
							&& !isStyleType(STYLE_COMMENT_LINE, this->PositionFromLine(nLine - 1))
							&& !isStyleType(STYLE_COMMENT_LINE, position))
					{
						nLastPosition += insertTextAndMove(this->PositionFromLine(nLine), "\r\n");
						i += 2;
					}
				}
			}
			/*if (command == "endif" || command == "endfor" || command == "endwhile" || command == "endcompose" || command == "endprocedure")
			{
			    nIndentationLevel--;
			    if (nIndentationLevel <= 0)
			    {
			        int nLine = this->LineFromPosition(i);
			        if (nLine < this->GetLineCount()-1
			            && this->GetLine(nLine+1).find_first_not_of(" \t\r\n") != string::npos)
			        {
			            nLastPosition += insertTextAndMove(this->PositionFromLine(nLine+1), "\r\n");
			        }
			    }
			}*/
		}
		if (this->GetStyleAt(i) == wxSTC_C_STRING)
		{
			if (this->GetStyleAt(i + 1) != wxSTC_C_STRING
					&& this->GetLineEndPosition(this->LineFromPosition(i)) != i + 1
					&& this->GetCharAt(i + 1) != ' '
					&& this->GetCharAt(i + 1) != ','
					&& this->GetCharAt(i + 1) != ';'
					&& this->GetCharAt(i + 1) != ')'
					&& this->GetCharAt(i + 1) != ']'
					&& this->GetCharAt(i + 1) != '}')
				nLastPosition += insertTextAndMove(i + 1, " ");
			if (this->GetStyleAt(i - 1) != wxSTC_C_STRING
					&& this->PositionFromLine(this->LineFromPosition(i)) != i
					&& this->GetCharAt(i - 1) != ' '
					&& this->GetCharAt(i - 1) != '('
					&& this->GetCharAt(i - 1) != '['
					&& this->GetCharAt(i - 1) != '{')
				nLastPosition += insertTextAndMove(i, " ");
		}
	}
	this->EndUndoAction();
}



