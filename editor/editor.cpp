#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#ifdef _MSC_VER
    #include <crtdbg.h>
#else
    #define _ASSERT(expr) ((void)0)

    #define _ASSERTE(expr) ((void)0)
#endif

#include "../common/CommonHeaders.h"
#include "../kernel/core/language.hpp"
#include "../kernel/core/tools.hpp"

#include <wx/datetime.h>
#include <wx/stdpaths.h>
#include <vector>
#include <string>

#include "editor.h"
#include "../perms/p.h"
#include "../gui/NumeReWindow.h"
#include "../gui/NumeReNotebook.h"

#include "../common/datastructures.h"
#include "../common/Options.h"
#include "../common/DebugEvent.h"
#include "../common/ProjectInfo.h"
#include "../debugger/DebugManager.h"
#include "../common/debug.h"
#include "../common/fixvsbug.h"

#define MARGIN_FOLD 3
#define MARKER_LINEINDICATOR 2
#define HIGHLIGHT 25
#define HIGHLIGHT_DBLCLK 26
#define HIGHLIGHT_MATCHING_BRACE 27
#define HIGHLIGHT_STRIKETHROUGH 28

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
	//EVT_MOTION          (NumeReEditor::OnMouseMotion)
	EVT_ENTER_WINDOW    (NumeReEditor::OnEnter)
	EVT_STC_DWELLSTART  (-1, NumeReEditor::OnMouseDwell)
	EVT_STC_MARGINCLICK (-1, NumeReEditor::OnMarginClick)
	//EVT_STC_START_DRAG  (-1, NumeReEditor::OnStartDrag)
	EVT_STC_DRAG_OVER   (-1, NumeReEditor::OnDragOver)
	//EVT_STC_DO_DROP     (-1, NumeReEditor::OnDrop)
	EVT_MENU			(ID_DEBUG_ADD_BREAKPOINT, NumeReEditor::OnAddBreakpoint)
	EVT_MENU			(ID_DEBUG_REMOVE_BREAKPOINT, NumeReEditor::OnRemoveBreakpoint)
	EVT_MENU			(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, NumeReEditor::OnClearBreakpoints)
	EVT_MENU			(ID_DEBUG_WATCH_SELECTION, NumeReEditor::OnAddWatch)
	EVT_MENU			(ID_DEBUG_DISPLAY_SELECTION, NumeReEditor::OnDisplayVariable)
	EVT_MENU			(ID_FIND_PROCEDURE, NumeReEditor::OnFindProcedure)
	EVT_MENU            (ID_UPPERCASE, NumeReEditor::OnChangeCase)
	EVT_MENU            (ID_LOWERCASE, NumeReEditor::OnChangeCase)
	//EVT_COMPILER_END	(ChameleonEditor::OnCompilerEnded)
	EVT_MENU			(ID_DEBUG_RUNTOCURSOR, NumeReEditor::OnRunToCursor)
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
NumeReEditor::NumeReEditor( NumeReWindow *mframe,
								 DebugManager* debugManager,
								 Options* options,
								 ProjectInfo* project,
                                  wxWindow *parent,     wxWindowID id, NumeReSyntax* __syntax, wxTerm* __terminal,
                                  const wxPoint & pos /* = wxDefaultPosition */,
                                  const wxSize & size /* = wxDefaultSize */,
                                  long style /* = 0 */, const
                                      wxString & name /* = wxSTCNameStr */ ) :
    wxStyledTextCtrl(parent, id, pos, size, style, name)
{
    m_mainFrame = mframe;
	m_debugManager = debugManager;
	//m_parentNotebook = (ChameleonNotebook*)parent;
	m_options = options;
	m_project = project;
	m_project->AddEditor(this);

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

    this->SetTabWidth(4);
    this->SetIndent(4);
    this->SetUseTabs(true);

    this->SetMultipleSelection(true);
    this->SetVirtualSpaceOptions(wxSTC_SCVS_RECTANGULARSELECTION);
    this->SetAdditionalSelectionTyping(true);
    this->SetMultiPaste(true);

    this->SetMarginWidth(0, 40);

    this->SetMarginType(0, wxSTC_MARGIN_NUMBER);

	this->SetMarginWidth(1, 16);
	this->SetMarginType(1, wxSTC_MARGIN_SYMBOL);

    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxInitAllImageHandlers();
	this->RegisterImage(NumeReSyntax::SYNTAX_COMMAND, wxBitmap(f.GetPath(true)+"icons\\cmd.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_FUNCTION, wxBitmap(f.GetPath(true)+"icons\\fnc.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_OPTION, wxBitmap(f.GetPath(true)+"icons\\opt.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_CONSTANT, wxBitmap(f.GetPath(true)+"icons\\cnst.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_SPECIALVAL, wxBitmap(f.GetPath(true)+"icons\\spv.png", wxBITMAP_TYPE_PNG));
	this->RegisterImage(NumeReSyntax::SYNTAX_OPERATOR, wxBitmap(f.GetPath(true)+"icons\\opr.png", wxBITMAP_TYPE_PNG));

    //wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
    wxFont font;
    font.SetNativeFontInfoUserDesc("Consolas 10 WINDOWS-1252");

    this->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

    this->StyleClearAll();

    this->SetMouseDwellTime(250);

    // Editor style setup

    /*// whitespace
    //this->StyleSetForeground(0, wxColour(0x80, 0x80, 0x80));

    // comment
    this->StyleSetForeground(1, wxColour(0x00, 0x7f, 0x00));

    // line comment
    this->StyleSetForeground(2, wxColour(0x00, 0x7f, 0x00));

    // numbers
    this->StyleSetForeground(4, wxColour("Red"));

    // keywords
    this->StyleSetForeground(5, wxColour("Blue"));

    this->StyleSetBold(5, TRUE);

    // double-quoted strings
    this->StyleSetForeground(6, wxColour(0x00, 0x80, 0x80));

    // single-quoted strings
    this->StyleSetForeground(7, wxColour(0x00, 0x80, 0x80));

    // preprocessor
    this->StyleSetForeground(9, wxColour("Purple"));

    // operators
    this->StyleSetForeground(10, wxColour(0x00, 0x00, 0x00));

    this->StyleSetForeground(11, wxColour(0x00, 0x00, 0x00));

    this->StyleSetBold(10, TRUE);

	this->SetSelBackground(true, wxColour(49, 106, 197));
	this->SetSelForeground(true, wxColour("white"));*/

    this->EmptyUndoBuffer();
    m_bSetUnsaved = false;
	m_bNewFile = true;
	UpdateSyntaxHighlighting();
	m_bNewFile = false;

	this->MarkerDefine(MARKER_BREAKPOINT, wxSTC_MARK_CIRCLE);
	this->MarkerSetBackground(MARKER_BREAKPOINT, wxColour("red"));

	//this->MarkerDefine(1, wxSTC_MARK_CIRCLE);
	//this->MarkerSetForeground(1, wxColour("red"));

	this->MarkerDefine(MARKER_FOCUSEDLINE, wxSTC_MARK_SHORTARROW);
	this->MarkerSetBackground(MARKER_FOCUSEDLINE, wxColour("yellow"));

	this->SetMarginSensitive(1, true);


	/*
	for(int i = 0; i <= wxSTC_MARK_PLUS; i++)
	{
		this->MarkerDefine(i, i);
		this->MarkerAdd(i, i);
	}
	*/

	this->UsePopUp(false);

	m_popupMenu.Append(ID_CUT, _guilang.get("GUI_MENU_EDITOR_CUT"));
	m_popupMenu.Append(ID_COPY, _guilang.get("GUI_MENU_EDITOR_COPY"));
	m_popupMenu.Append(ID_PASTE, _guilang.get("GUI_MENU_EDITOR_PASTE"));
	m_popupMenu.AppendSeparator();

	m_popupMenu.Append(ID_DEBUG_ADD_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_ADDBP"));
	m_popupMenu.Append(ID_DEBUG_REMOVE_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_REMOVEBP"));
	m_popupMenu.Append(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, _guilang.get("GUI_MENU_EDITOR_CLEARBP"));

	//m_popupMenu.Append(ID_DEBUG_RUNTOCURSOR, "Run to cursor");

	m_popupMenu.AppendSeparator();

	//m_menuAddWatch = m_popupMenu.Append(ID_DEBUG_WATCH_SELECTION, "Watch selection");
	m_menuFindProcedure = m_popupMenu.Append(ID_FIND_PROCEDURE, _guilang.get("GUI_MENU_EDITOR_FINDPROC", "$procedure"));
	m_menuShowValue = m_popupMenu.Append(ID_DEBUG_DISPLAY_SELECTION, _guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "selection"));
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
    if(m_project->IsSingleFile())
	{
		delete m_project;
	}
	else
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
bool NumeReEditor::SaveFile( const wxString & filename )
{

    // return if no change
    if(!Modified() && filename.IsEmpty()) { return true; }

	wxFileName fn(filename);

    // save edit in file and clear undo
    if(!filename.IsEmpty())
	{
		m_simpleFileName = fn.GetFullName();
	}

    wxFile file (filename, wxFile::write);

    if(!file.IsOpened()) { return false; }

    wxString buf = GetText();

    bool okay = file.Write(buf.ToStdString().c_str(), buf.ToStdString().length());

    file.Close();

    if(!okay) { return false; }

    EmptyUndoBuffer();
    SetSavePoint();

    m_filetime = fn.GetModificationTime();
    m_bSetUnsaved = false;
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
	if(fileContents.Length() > 0)
	{
		m_bLoadingFile = true;
		ClearAll();
		InsertText(0, fileContents);
	}

    EmptyUndoBuffer();

    // determine and set EOL mode
    int eolMode = -1;

    bool eolMix = false;

    wxString eolName;

    if( fileContents.Contains("\r\n") )
    {
        eolMode = wxSTC_EOL_CRLF;

        eolName = _("CR+LF (Windows)");
    }
    else if( fileContents.Contains("\r") )
    {
        if(eolMode != -1) { eolMix = true; }
        else
        {
            eolMode = wxSTC_EOL_CR;

            eolName = _("CR (Macintosh)");
        }
    }
    else if( fileContents.Contains("\n") )
    {
        if(eolMode != -1) { eolMix = true; }
        else
        {
            eolMode = wxSTC_EOL_LF;

            eolName = _("LF (Unix)");
        }
    }

    if( eolMode != -1 )
    {
        if( eolMix && wxMessageBox(_("Convert all line endings to ")
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
void NumeReEditor::OnChar( wxStyledTextEvent &event )
{
    ClearDblClkIndicator();
	const wxChar chr = event.GetKey();
	const int currentLine = GetCurrentLine();
	const int currentPos = GetCurrentPos();
	const int wordstartpos = WordStartPosition(currentPos, true);
	//const int tabWidth = GetTabWidth();
	//const int eolMode = GetEOLMode();
    MarkerDeleteAll(MARKER_FOCUSEDLINE);
    if (chr == WXK_TAB)
    {
        event.Skip(true);

        int startLine = LineFromPosition(GetSelectionStart());

        int endline = LineFromPosition(GetSelectionEnd());

        int newStartPos = PositionFromLine(startLine);

        int newEndPos = PositionFromLine(endline) + LineLength(endline);

        this->SetSelection(newStartPos, newEndPos);

        bool doIndent = event.GetShift();

        int indentWidth = this->GetIndent();

        for( int i = startLine; i <= endline; i++ )
        {
            int lineIndent = this->GetLineIndentation(i);

            if(doIndent)
                { this->SetLineIndentation(i, lineIndent + indentWidth); }
            else
            {
                this->SetLineIndentation(i, lineIndent - indentWidth);
            }
        }
    }

	//if (((eolMode == CRLF || eolMode == LF) && chr == '\n')
	//	|| (eolMode == CR && chr == '\r'))
	if (chr == '\n')// && m_options->GetPerms()->isEnabled(PERM_AUTOINDENT))
	{
		int previousLineInd = 0;

		if (currentLine > 0)
		{
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
        if (this->BraceMatch(currentPos-1) == wxSTC_INVALID_POSITION)
        {
            if (chr == '(')
                InsertText(nMatchingPos, ")");
            else if (chr == '[')
                InsertText(nMatchingPos, "]");
            else
                InsertText(nMatchingPos, "}");
        }
	}

    int lenEntered = currentPos-wordstartpos;

    if (lenEntered > 1
        && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_LINE
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_BLOCK
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_STRING
        && GetStyleAt(wordstartpos) != wxSTC_NPRC_COMMENT_LINE
        && GetStyleAt(wordstartpos) != wxSTC_NPRC_COMMENT_BLOCK
        && GetStyleAt(wordstartpos) != wxSTC_NPRC_STRING)
    {
        this->AutoCompSetIgnoreCase(true);
        this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
        this->AutoCompShow(lenEntered, generateAutoCompList(GetTextRange(wordstartpos, currentPos), _syntax->getAutoCompList(GetTextRange(wordstartpos, currentPos).ToStdString())));
    }
    else if (lenEntered > 1
        && !(m_fileType == FILE_NSCR || m_fileType == FILE_NPRC))
    {
        this->AutoCompSetIgnoreCase(true);
        this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
        this->AutoCompShow(lenEntered, generateAutoCompList(GetTextRange(wordstartpos, currentPos), ""));
    }
    this->Colourise(0, -1);
	event.Skip();
}


void NumeReEditor::MakeBraceCheck()
{
    char CurrentChar = this->GetCharAt(this->GetCurrentPos());
    char PrevChar = 0;
    if (this->GetCurrentPos())
        PrevChar = this->GetCharAt(this->GetCurrentPos()-1);
    if (CurrentChar == ')' || CurrentChar == ']' || CurrentChar == '}')
        getMatchingBrace(this->GetCurrentPos());
    else if (PrevChar == '(' || PrevChar == '[' || PrevChar == '{')
        getMatchingBrace(this->GetCurrentPos()-1);
    else if (CurrentChar == '(' || CurrentChar == '[' || CurrentChar == '{')
        getMatchingBrace(this->GetCurrentPos());
    else if (PrevChar == ')' || PrevChar == ']' || PrevChar == '}')
        getMatchingBrace(this->GetCurrentPos()-1);
    else
    {
        this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
        long int maxpos = this->GetLastPosition();
        this->IndicatorClearRange(0,maxpos);
        this->BraceBadLight(wxSTC_INVALID_POSITION);
        this->BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
    }
    applyStrikeThrough();
    return;
}

void NumeReEditor::OnKeyDn(wxKeyEvent &event)
{
    //wxMessageBox(wxString((char)this->GetCharAt(this->GetCurrentPos())));
    if (this->HasSelection() && event.GetKeyCode() != WXK_SHIFT)
    {
        char chr = event.GetKeyCode();
        if (event.ShiftDown() && (chr == '8' || chr == '9'))
        {
            this->BeginUndoAction();
            int selStart = this->GetSelectionStart();
            int selEnd = this->GetSelectionEnd()+1;
            this->InsertText(selStart, "(");
            this->InsertText(selEnd, ")");
            if (chr == '8')
                this->GotoPos(selStart);
            else
                this->GotoPos(selEnd+1);
            this->EndUndoAction();
            MakeBraceCheck();
            return;
        }
        else if (event.ShiftDown() && chr == '2')
        {
            this->BeginUndoAction();
            int selStart = this->GetSelectionStart();
            int selEnd = this->GetSelectionEnd()+1;
            this->InsertText(selStart, "\"");
            this->InsertText(selEnd, "\"");
            this->GotoPos(selEnd+1);
            this->EndUndoAction();
            MakeBraceCheck();
            return;
        }
        else if (event.ControlDown() && event.AltDown() && (chr == '8' || chr == '9')) // Alt Gr means CTRL+ALT
        {
            this->BeginUndoAction();
            int selStart = this->GetSelectionStart();
            int selEnd = this->GetSelectionEnd()+1;
            this->InsertText(selStart, "[");
            this->InsertText(selEnd, "]");
            if (chr == '8')
                this->GotoPos(selStart);
            else
                this->GotoPos(selEnd+1);
            this->EndUndoAction();
            MakeBraceCheck();
            return;
        }
        else if (event.ControlDown() && event.AltDown() && (chr == '7' || chr == '0'))
        {
            this->BeginUndoAction();
            int selStart = this->GetSelectionStart();
            int selEnd = this->GetSelectionEnd()+1;
            this->InsertText(selStart, "{");
            this->InsertText(selEnd, "}");
            if (chr == '7')
                this->GotoPos(selStart);
            else
                this->GotoPos(selEnd+1);
            this->EndUndoAction();
            MakeBraceCheck();
            return;
        }
    }
    OnKeyDown(event);
    MakeBraceCheck();
    if (!event.ControlDown() && !event.ShiftDown())
        ClearDblClkIndicator();
    //event.Skip();
}

void NumeReEditor::OnKeyRel(wxKeyEvent &event)
{
    //wxMessageBox(wxString((char)this->GetCharAt(this->GetCurrentPos())));
    MakeBraceCheck();
    event.Skip();
    //OnKeyUp(event);
}

void NumeReEditor::OnMouseUp(wxMouseEvent &event)
{
    //OnMouseLeftUp(event);
    MakeBraceCheck();
    event.Skip();
}

void NumeReEditor::OnMouseDn(wxMouseEvent &event)
{
    //OnMouseLeftDown(event);
    if (!event.ControlDown())
        ClearDblClkIndicator();
    MakeBraceCheck();
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
    this->IndicatorClearRange(0,maxpos);
    this->IndicatorSetStyle(HIGHLIGHT_DBLCLK, wxSTC_INDIC_ROUNDBOX);
    this->IndicatorSetAlpha(HIGHLIGHT_DBLCLK, 80);
    this->IndicatorSetForeground(HIGHLIGHT_DBLCLK, wxColor(0,255,0));

    unsigned int nPos = 0;
    unsigned int nCurr = 0;
    unsigned int nLength = endPosition-startPosition;
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

void NumeReEditor::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

void NumeReEditor::OnMouseDwell(wxStyledTextEvent& event)
{
    if ((m_fileType != FILE_NSCR && m_fileType != FILE_NPRC) || m_PopUpActive)
        return;
    //wxPoint pos = event.GetPosition();
    int charpos = event.GetPosition(); //PositionFromPoint(pos);
	int startPosition = WordStartPosition(charpos, true);
	int endPosition = WordEndPosition(charpos, true);

	wxString selection = this->GetTextRange(startPosition, endPosition);
	if (this->CallTipActive())
        this->CallTipCancel();
    if (this->GetStyleAt(charpos) == wxSTC_NSCR_FUNCTION || this->GetStyleAt(charpos) == wxSTC_NPRC_FUNCTION)
    {
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
        this->CallTipShow(charpos, addLinebreaks(_guilang.get("PARSERFUNCS_LISTFUNC_FUNC_"+toUpperCase(selection.ToStdString())+"_*")));
        this->CallTipSetHighlight(0,22);
    }
    else if (this->GetStyleAt(charpos) == wxSTC_NSCR_COMMAND || this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURE_COMMANDS || this->GetStyleAt(charpos) == wxSTC_NPRC_COMMAND)
    {
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
            string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_IF"), lastpos)) + "\n    (...)\n";
            if (selection != "if")
                nLength = sBlock.length();

            sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ELSEIF"), lastpos2)) + "\n    (...)\n";
            if (selection != "if" && selection != "elseif")
                nLength = sBlock.length()+1;

            sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ELSE")) + "\n    (...)\n";
            if (selection != "if" && selection != "elseif" && selection != "else")
                nLength = sBlock.length()+2;

            sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDIF"));
            this->CallTipShow(charpos, sBlock);
            if (selection == "if")
                this->CallTipSetHighlight(nLength,lastpos+nLength);
            else if (selection == "elseif")
                this->CallTipSetHighlight(nLength,lastpos2+nLength);
            else
                this->CallTipSetHighlight(nLength,13+nLength);
        }
        else if (selection == "for" || selection == "endfor")
        {
            size_t nLength = 0;
            size_t lastpos2 = 0;
            string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_FOR"), lastpos)) + "\n    (...)\n";
            if (selection != "for")
                nLength = sBlock.length();
            sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDFOR"), lastpos2));
            this->CallTipShow(charpos, sBlock);
            if (nLength)
                this->CallTipSetHighlight(nLength,lastpos2+nLength);
            else
                this->CallTipSetHighlight(nLength,lastpos+nLength);
        }
        else if (selection == "while" || selection == "endwhile")
        {
            size_t nLength = 0;
            size_t lastpos2 = 0;
            string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_WHILE"), lastpos)) + "\n    (...)\n";
            if (selection != "while")
                nLength = sBlock.length();
            sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDWHILE"), lastpos2));
            this->CallTipShow(charpos, sBlock);
            if (nLength)
                this->CallTipSetHighlight(nLength,lastpos2+nLength);
            else
                this->CallTipSetHighlight(nLength,lastpos+nLength);
        }
        else if (selection == "procedure" || selection == "endprocedure")
        {
            size_t nLength = 0;
            string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_PROCEDURE"), lastpos)) + "\n    (...)\n";
            if (selection != "procedure")
                nLength = sBlock.length();
            sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDPROCEDURE"));
            this->CallTipShow(charpos, sBlock);
            if (nLength)
                this->CallTipSetHighlight(nLength,13+nLength);
            else
                this->CallTipSetHighlight(nLength,lastpos+nLength);
        }
        else if (selection == "compose" || selection == "endcompose")
        {
            size_t nLength = 0;
            string sBlock = addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_COMPOSE")) + "\n    (...)\n";
            if (selection != "compose")
                nLength = sBlock.length();
            sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_ENDCOMPOSE"));
            this->CallTipShow(charpos, sBlock);
            this->CallTipSetHighlight(nLength,13+nLength);
        }
        else
        {
            this->CallTipShow(charpos, addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_"+toUpperCase(selection.ToStdString())), lastpos)));
            this->CallTipSetHighlight(0,lastpos);
        }
    }
    else if (this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES || this->GetStyleAt(charpos) == wxSTC_NPRC_PROCEDURES)
    {
        wxString proc = FindMarkedProcedure(charpos);
        if (!proc.length())
            return;
        this->CallTipShow(charpos, _guilang.get("GUI_EDITOR_CALLTIP_PROC1") + " " + m_clickedProcedure+"(...)\n" + _guilang.get("GUI_EDITOR_CALLTIP_PROC2"));
        this->CallTipSetHighlight(_guilang.get("GUI_EDITOR_CALLTIP_PROC1").length()+1,6+m_clickedProcedure.length()+_guilang.get("GUI_EDITOR_CALLTIP_PROC1").length());
    }
    else if (this->GetStyleAt(charpos) == wxSTC_NSCR_OPTION || this->GetStyleAt(charpos) == wxSTC_NPRC_OPTION)
    {
        selection = _guilang.get("GUI_EDITOR_CALLTIP_OPT_"+toUpperCase(selection.ToStdString()));
        size_t highlightlength = selection.length();
        if (selection.find(' ') != string::npos)
            highlightlength = selection.find(' ');
        this->CallTipShow(charpos, "Option: " + selection);
        this->CallTipSetHighlight(8,8+highlightlength);
    }
    else if (this->GetStyleAt(charpos) == wxSTC_NSCR_PREDEFS || this->GetStyleAt(charpos) == wxSTC_NPRC_PREDEFS)
    {
        this->CallTipShow(charpos, _guilang.get("GUI_EDITOR_CALLTIP_"+toUpperCase(selection.ToStdString())));
        this->CallTipSetHighlight(0, 10);
    }
    else if (this->GetStyleAt(charpos) == wxSTC_NSCR_CONSTANTS || this->GetStyleAt(charpos) == wxSTC_NPRC_CONSTANTS)
    {
        string sCalltip = _guilang.get("GUI_EDITOR_CALLTIP_CONST"+toUpperCase(selection.ToStdString()));
        if (selection == "_G")
            sCalltip = _guilang.get("GUI_EDITOR_CALLTIP_CONST_GRAV");
        this->CallTipShow(charpos, sCalltip);
        this->CallTipSetHighlight(0, sCalltip.find('='));
    }
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
            if (this->GetCharAt(position+2) == ' ')
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+3)
                    nSelectionStart -= 3;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,3);
                nSelectionEnd -= 3;
            }
            else
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+2)
                    nSelectionStart -= 2;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,2);
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
        else if (m_fileType == FILE_TEXSOURCE && GetStyleAt(position+1) == wxSTC_TEX_DEFAULT && GetCharAt(position) == '%')
        {
            if (this->GetCharAt(position+1) == ' ')
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+2)
                    nSelectionStart -= 2;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,2);
                nSelectionEnd -= 2;
            }
            else
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+1)
                    nSelectionStart -= 1;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,1);
                nSelectionEnd -= 1;
            }
        }
        else if (m_fileType == FILE_TEXSOURCE && GetStyleAt(position+1) != wxSTC_TEX_DEFAULT && GetCharAt(position) != '%')
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
            if (this->GetCharAt(position+1) == ' ')
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+2)
                    nSelectionStart -= 2;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,2);
                nSelectionEnd -= 2;
            }
            else
            {
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position+1)
                    nSelectionStart -= 1;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;
                this->DeleteRange(position,1);
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
        while (nFirstPosition && (GetStyleAt(nFirstPosition-1) == wxSTC_NPRC_COMMENT_BLOCK || GetStyleAt(nFirstPosition-1) == wxSTC_NSCR_COMMENT_BLOCK))
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
        if (this->GetTextRange(nLastPosition-3, nLastPosition) == " *#")
        {
            if (nSelectionEnd > nLastPosition-3)
                nSelectionEnd -= 3;
            this->DeleteRange(nLastPosition-3, 3);
        }
        else
        {
            if (nSelectionEnd > nLastPosition-2)
                nSelectionEnd -= 2;
            this->DeleteRange(nFirstPosition-2, 2);
        }
        //text = GetTextRange(nFirstPosition, nFirstPosition+3);
        if (this->GetTextRange(nFirstPosition, nFirstPosition+3) == "#* ")
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
        this->InsertText(nLastPosition+3, " *#");
        nSelectionEnd += 3;
        nSelectionStart +=3;
    }
    this->EndUndoAction();
    this->SetSelection(nSelectionStart, nSelectionEnd);
}

void NumeReEditor::FoldAll()
{
    for (int i = this->GetLineCount()-1; i >= 0; i--)
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


void NumeReEditor::removeWhiteSpaces(int nType)
{
    int nFirstline = 0;
    int nLastLine = GetLineCount()-1;
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
            while (nLineEndPos && (sLine[nLineEndPos-1] == ' ' || sLine[nLineEndPos-1] == '\t'))
            {
                sLine.erase(nLineEndPos-1);
                nLineEndPos--;
            }
            if (nType == RM_WS_BOTH)
            {
                while (sLine[0] == ' ' || sLine[0] == '\t')
                    sLine.erase(0,1);
            }
            Replace(this->PositionFromLine(i), this->GetLineEndPosition(i), sLine);
        }
    }
    this->EndUndoAction();
}

void NumeReEditor::sortSelection(bool ascending)
{
    int nFirstline = 0;
    int nLastLine = GetLineCount()-1;
    map<string,int> mSortMap;
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
        if (vSortVector[i-nFirstline].find_first_of("\r\n") != string::npos)
            vSortVector[i-nFirstline].erase(vSortVector[i-nFirstline].find_first_of("\r\n"));
        sCurrentLine = toLowerCase(vSortVector[i-nFirstline].ToStdString());
        StripSpaces(sCurrentLine);
        if (!sCurrentLine.length())
            sCurrentLine = " " + toString(i+256);
        if (mSortMap.find(sCurrentLine) != mSortMap.end())
            sCurrentLine += "\n" + toString(i+256); // need a value smaller than space therefore the \n
        mSortMap[sCurrentLine] = i-nFirstline;
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


bool NumeReEditor::getEditorSetting(EditorSettings _setting)
{
    return m_nEditorSetting & _setting;
}

void NumeReEditor::ToggleSettings(EditorSettings _setting)
{
    this->SetWhitespaceForeground(true, wxColor(170,190,210));
    this->SetWhitespaceSize(2);
    if (!(m_nEditorSetting & _setting))
    {
        m_nEditorSetting |= _setting;

        if (_setting == SETTING_WRAPEOL)
        {
            this->SetWrapMode(wxSTC_WRAP_WORD);
            this->SetWrapIndentMode(wxSTC_WRAPINDENT_INDENT);
            this->SetWrapStartIndent(1);
            this->SetWrapVisualFlags(wxSTC_WRAPVISUALFLAG_END);
            this->SetWrapVisualFlagsLocation(wxSTC_WRAPVISUALFLAGLOC_END_BY_TEXT);
        }
        else if (_setting == SETTING_DISPCTRLCHARS)
        {
            this->SetViewWhiteSpace(wxSTC_WS_VISIBLEALWAYS);
            this->SetViewEOL(true);
        }
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
    }
    UpdateSyntaxHighlighting();
}

void NumeReEditor::getMatchingBrace(int nPos)
{
    int nMatch = this->BraceMatch(nPos);
    if (nMatch == wxSTC_INVALID_POSITION)
        this->BraceBadLight(nPos);
    else
    {
        this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
        this->IndicatorClearRange(0,GetLastPosition());
        this->IndicatorSetStyle(HIGHLIGHT_MATCHING_BRACE, wxSTC_INDIC_DIAGONAL);
        //this->IndicatorSetForeground(HIGHLIGHT_MATCHING_BRACE, wxColour(0,220,0));
        this->IndicatorSetForeground(HIGHLIGHT_MATCHING_BRACE, wxColour(0,150,0));

        if (nMatch < nPos)
        {
            this->BraceHighlight(nMatch, nPos);
            this->IndicatorFillRange(nMatch+1, nPos-nMatch-1);
        }
        else
        {
            this->BraceHighlight(nPos, nMatch);
            this->IndicatorFillRange(nPos+1, nMatch-nPos-1);
        }
    }
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

/*
void ChameleonEditor::SetRemoteFileNameAndPath(wxString path, wxString name)
{
	m_remoteFileName.Assign(path, name, wxPATH_UNIX);
}

void ChameleonEditor::SetLocalFileNameAndPath(wxString path, wxString name)
{
	m_localFileName.Assign(path, name, wxPATH_DOS);
}
*/

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
void NumeReEditor::UpdateSyntaxHighlighting()
{
	wxString filename = GetFileNameAndPath();

	FileFilterType filetype = m_project->GetFileType(filename);
	if (m_fileType != filetype)
        m_fileType = filetype;
    else if (m_fileType == FILE_NSCR
        || m_fileType == FILE_NPRC
        || m_fileType == FILE_TEXSOURCE
        || m_fileType == FILE_DATAFILES)
        return;


	if (filetype == FILE_NSCR)
	{
        m_fileType = FILE_NSCR;
		this->SetLexer(wxSTC_LEX_NSCR);

		this->SetProperty("fold", "1");
		//this->SetProperty("fold.comment", "1");
		this->SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

		this->SetMarginType(MARGIN_FOLD, wxSTC_MARGIN_SYMBOL);
		this->SetMarginWidth(MARGIN_FOLD, 15);
		this->SetMarginMask(MARGIN_FOLD, wxSTC_MASK_FOLDERS);
		this->SetMarginSensitive(MARGIN_FOLD, true);
		this->StyleSetBackground(MARGIN_FOLD, wxColor(200, 200, 200) );
        this->SetMarginSensitive(MARGIN_FOLD, true);

        //this->BraceBadLightIndicator(true, wxSTC_NSCR_DEFAULT);
        //this->BraceHighlightIndicator(true, wxSTC_NSCR_COMMENT_LINE);

        // Properties found from http://www.scintilla.org/SciTEDoc.html
        //text->SetProperty (wxT("fold"),         wxT("1") );
        //text->SetProperty (wxT("fold.comment"), wxT("1") );
        //text->SetProperty (wxT("fold.compact"), wxT("1") );

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

        if (_syntax)
        {
            this->SetKeyWords(0, _syntax->getCommands());
            this->SetKeyWords(1, _syntax->getOptions());
            this->SetKeyWords(2, _syntax->getFunctions());
            this->SetKeyWords(3, "x y z t");
            this->SetKeyWords(4, _syntax->getConstants());
            this->SetKeyWords(5, _syntax->getSpecial());
            this->SetKeyWords(6, _syntax->getOperators());
            this->SetKeyWords(7, _syntax->getNPRCCommands());
        }
		/*this->SetCaretLineVisible(true);
		this->SetCaretLineBackground(wxColour(196,211,255));
		this->SetIndentationGuides(true);*/

		this->StyleSetForeground(wxSTC_NSCR_DEFAULT, wxColour(0,0,0));
		this->StyleSetItalic(wxSTC_NSCR_DEFAULT, true);

		this->StyleSetForeground(wxSTC_NSCR_IDENTIFIER, wxColour(0,0,0));
		this->StyleSetItalic(wxSTC_NSCR_IDENTIFIER, true);

		this->StyleSetForeground(wxSTC_NSCR_NUMBERS, wxColour(255,128,64));

		this->StyleSetForeground(wxSTC_NSCR_COMMENT_LINE, wxColour(0,128,0));
		this->StyleSetBackground(wxSTC_NSCR_COMMENT_LINE, wxColour(255,255,183));
		this->StyleSetBold(wxSTC_NSCR_COMMENT_LINE, true);

		this->StyleSetForeground(wxSTC_NSCR_COMMENT_BLOCK, wxColour(0,128,0));
		this->StyleSetBackground(wxSTC_NSCR_COMMENT_BLOCK, wxColour(255,255,183));
		this->StyleSetBold(wxSTC_NSCR_COMMENT_BLOCK, true);

		this->StyleSetForeground(wxSTC_NSCR_COMMAND, wxColour(0, 128, 255));
		this->StyleSetBold(wxSTC_NSCR_COMMAND, true);
		this->StyleSetUnderline(wxSTC_NSCR_COMMAND, true);

		this->StyleSetForeground(wxSTC_NSCR_OPTION, wxColour(0, 128, 128));
		this->StyleSetBold(wxSTC_NSCR_OPTION, false);

        this->StyleSetForeground(wxSTC_NSCR_CONSTANTS, wxColour(255, 0, 128));
		this->StyleSetBold(wxSTC_NSCR_CONSTANTS, true);

        this->StyleSetForeground(wxSTC_NSCR_FUNCTION, wxColour(0, 0, 255));
		this->StyleSetBold(wxSTC_NSCR_FUNCTION, true);

        this->StyleSetForeground(wxSTC_NSCR_PREDEFS, wxColour(0, 0, 0));
		this->StyleSetBold(wxSTC_NSCR_PREDEFS, true);

		this->StyleSetForeground(wxSTC_NSCR_STRING, wxColour(128,128,255));

		this->StyleSetForeground(wxSTC_NSCR_STRING_PARSER, wxColour(0,128,192));
		this->StyleSetBold(wxSTC_NSCR_STRING_PARSER, true);

		this->StyleSetForeground(wxSTC_NSCR_INCLUDES, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NSCR_INCLUDES, true);

		this->StyleSetForeground(wxSTC_NSCR_PROCEDURES, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NSCR_PROCEDURES, true);
		this->StyleSetForeground(wxSTC_NSCR_PROCEDURE_COMMANDS, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NSCR_PROCEDURE_COMMANDS, true);

		this->StyleSetForeground(wxSTC_NSCR_INSTALL, wxColour(128,128,128));
		this->StyleSetBold(wxSTC_NSCR_INSTALL, false);
		this->StyleSetItalic(wxSTC_NSCR_INSTALL, false);

        this->StyleSetForeground(wxSTC_NSCR_DEFAULT_VARS, wxColour(0, 0, 160));
		this->StyleSetBold(wxSTC_NSCR_DEFAULT_VARS, true);
		this->StyleSetItalic(wxSTC_NSCR_DEFAULT_VARS, true);

        this->StyleSetForeground(wxSTC_NSCR_CUSTOM_FUNCTION, wxColour(0, 0, 160));

        this->StyleSetForeground(wxSTC_NSCR_OPERATORS, wxColour(255, 0, 0));
        this->StyleSetForeground(wxSTC_NSCR_OPERATOR_KEYWORDS, wxColour(255, 0, 0));
	}
	else if (filetype == FILE_NPRC)
	{
		this->SetLexer(wxSTC_LEX_NPRC);

		this->SetProperty("fold", "1");
		//this->SetProperty("fold.comment", "1");
		this->SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

		this->SetMarginType(MARGIN_FOLD, wxSTC_MARGIN_SYMBOL);
		this->SetMarginWidth(MARGIN_FOLD, 15);
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

        if (_syntax)
        {
            this->SetKeyWords(0, _syntax->getCommands()+_syntax->getNPRCCommands());
            this->SetKeyWords(1, _syntax->getOptions());
            this->SetKeyWords(2, _syntax->getFunctions());
            this->SetKeyWords(3, "x y z t");
            this->SetKeyWords(4, _syntax->getConstants());
            this->SetKeyWords(5, _syntax->getSpecial());
            this->SetKeyWords(6, _syntax->getOperators());
        }
		this->StyleSetForeground(wxSTC_NPRC_DEFAULT, wxColour(0,0,0));
		this->StyleSetItalic(wxSTC_NPRC_DEFAULT, true);

		this->StyleSetForeground(wxSTC_NPRC_IDENTIFIER, wxColour(0,0,0));
		this->StyleSetItalic(wxSTC_NPRC_IDENTIFIER, true);

		this->StyleSetForeground(wxSTC_NPRC_NUMBERS, wxColour(255,128,64));

		this->StyleSetForeground(wxSTC_NPRC_COMMENT_LINE, wxColour(0,128,0));
		this->StyleSetBackground(wxSTC_NPRC_COMMENT_LINE, wxColour(255,255,183));
		this->StyleSetBold(wxSTC_NPRC_COMMENT_LINE, true);

		this->StyleSetForeground(wxSTC_NPRC_COMMENT_BLOCK, wxColour(0,128,0));
		this->StyleSetBackground(wxSTC_NPRC_COMMENT_BLOCK, wxColour(255,255,183));
		this->StyleSetBold(wxSTC_NPRC_COMMENT_BLOCK, true);

		this->StyleSetForeground(wxSTC_NPRC_COMMAND, wxColour(0, 128, 255));
		this->StyleSetBold(wxSTC_NPRC_COMMAND, true);
		this->StyleSetUnderline(wxSTC_NPRC_COMMAND, true);

		this->StyleSetForeground(wxSTC_NPRC_OPTION, wxColour(0, 128, 128));
		this->StyleSetBold(wxSTC_NPRC_OPTION, false);

        this->StyleSetForeground(wxSTC_NPRC_CONSTANTS, wxColour(255, 0, 128));
		this->StyleSetBold(wxSTC_NPRC_CONSTANTS, true);

        this->StyleSetForeground(wxSTC_NPRC_FUNCTION, wxColour(0, 0, 255));
		this->StyleSetBold(wxSTC_NPRC_FUNCTION, true);

        this->StyleSetForeground(wxSTC_NPRC_PREDEFS, wxColour(0, 0, 0));
		this->StyleSetBold(wxSTC_NPRC_PREDEFS, true);

		this->StyleSetForeground(wxSTC_NPRC_STRING, wxColour(128,128,255));

		this->StyleSetForeground(wxSTC_NPRC_STRING_PARSER, wxColour(0,128,192));
		this->StyleSetBold(wxSTC_NPRC_STRING_PARSER, true);

		this->StyleSetForeground(wxSTC_NPRC_INCLUDES, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NPRC_INCLUDES, true);

		this->StyleSetForeground(wxSTC_NPRC_PROCEDURES, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NPRC_PROCEDURES, true);
		this->StyleSetForeground(wxSTC_NPRC_FLAGS, wxColour(128,0,0));
		this->StyleSetBold(wxSTC_NPRC_FLAGS, true);

        this->StyleSetForeground(wxSTC_NPRC_DEFAULT_VARS, wxColour(0, 0, 160));
		this->StyleSetBold(wxSTC_NPRC_DEFAULT_VARS, true);
		this->StyleSetItalic(wxSTC_NPRC_DEFAULT_VARS, true);

        this->StyleSetForeground(wxSTC_NPRC_CUSTOM_FUNCTION, wxColour(0, 0, 160));

        this->StyleSetForeground(wxSTC_NPRC_OPERATORS, wxColour(255, 0, 0));
        this->StyleSetForeground(wxSTC_NPRC_OPERATOR_KEYWORDS, wxColour(255, 0, 0));
	}
	else if (filetype == FILE_TEXSOURCE)
	{
        this->SetLexer(wxSTC_LEX_TEX);
        this->StyleSetForeground(wxSTC_TEX_DEFAULT, wxColor(0,128,0)); //Comment
        this->StyleSetForeground(wxSTC_TEX_COMMAND, wxColor(0,0,255)); //Command
        this->StyleSetBold(wxSTC_TEX_COMMAND, true);
        this->StyleSetUnderline(wxSTC_TEX_COMMAND, false);
        this->StyleSetForeground(wxSTC_TEX_TEXT, wxColor(0,0,0)); // Actual text
        this->StyleSetForeground(wxSTC_TEX_GROUP, wxColor(0,128,0)); // Grouping elements like $ $ or { }
        this->StyleSetBackground(wxSTC_TEX_GROUP, wxColor(255,255,183)); // Grouping elements like $ $ or { }
        this->StyleSetBold(wxSTC_TEX_GROUP, true);
        this->StyleSetForeground(wxSTC_TEX_SPECIAL, wxColor(255,0,196)); // Parentheses/Brackets
        this->StyleSetItalic(wxSTC_TEX_SPECIAL, false);
        this->StyleSetBold(wxSTC_TEX_SPECIAL, true);
        this->StyleSetForeground(wxSTC_TEX_SYMBOL, wxColor(255,0,0)); // Operators
        this->StyleSetBackground(wxSTC_TEX_SYMBOL, wxColor(255,255,255));
        this->StyleSetBold(wxSTC_TEX_SYMBOL, false);
	}
	else if (filetype == FILE_DATAFILES)
	{
        this->SetLexer(wxSTC_LEX_OCTAVE);
        this->StyleSetForeground(wxSTC_MATLAB_COMMENT, wxColor(0,128,0));
        this->StyleSetItalic(wxSTC_MATLAB_COMMENT, false);
        this->StyleSetForeground(wxSTC_MATLAB_OPERATOR, wxColor(255,0,0));
        this->StyleSetBold(wxSTC_MATLAB_OPERATOR, false);
        this->StyleSetForeground(wxSTC_MATLAB_NUMBER, wxColor(0,0,128));
        this->StyleSetBackground(wxSTC_MATLAB_NUMBER, wxColor(255,255,255));
        this->StyleSetForeground(wxSTC_MATLAB_IDENTIFIER, wxColor(0,0,0));
        this->StyleSetBold(wxSTC_MATLAB_IDENTIFIER, false);
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
            this->StyleSetForeground(wxSTC_TXTADV_MODIFIER, wxColor(255,180,180));
            this->StyleSetItalic(wxSTC_TXTADV_ITALIC, true);
            this->StyleSetItalic(wxSTC_TXTADV_BOLD, false);
            this->StyleSetBold(wxSTC_TXTADV_BOLD, true);
            this->StyleSetItalic(wxSTC_TXTADV_BOLD_ITALIC, true);
            this->StyleSetBold(wxSTC_TXTADV_BOLD_ITALIC, true);
            this->StyleSetUnderline(wxSTC_TXTADV_UNDERLINE, true);
            this->StyleSetForeground(wxSTC_TXTADV_STRIKETHROUGH, wxColor(140,140,140));
            this->StyleSetItalic(wxSTC_TXTADV_STRIKETHROUGH, true);
            this->StyleSetUnderline(wxSTC_TXTADV_URL, true);
            this->StyleSetForeground(wxSTC_TXTADV_URL, wxColor(0,0,255));
            this->StyleSetUnderline(wxSTC_TXTADV_HEAD, true);
            this->StyleSetBold(wxSTC_TXTADV_HEAD, true);
            this->StyleSetUnderline(wxSTC_TXTADV_BIGHEAD, true);
            this->StyleSetBold(wxSTC_TXTADV_BIGHEAD, true);
            this->StyleSetSize(wxSTC_TXTADV_BIGHEAD, this->StyleGetSize(0)+1);
            this->StyleSetCase(wxSTC_TXTADV_BIGHEAD, wxSTC_CASE_UPPER);
		}
		//this->ClearDocumentStyle();
	}
	applyStrikeThrough();
	updateDefaultHighlightSettings();
    this->Colourise(0,-1);
}


void NumeReEditor::updateDefaultHighlightSettings()
{
    this->SetCaretLineVisible(true);
    this->SetIndentationGuides(true);
    //this->SetCaretLineBackground(wxColour(196,211,255));
    //this->SetCaretLineBackground(wxColour(206,219,255));
    this->SetCaretLineBackground(wxColour(221,230,255));

    this->StyleSetForeground(wxSTC_STYLE_BRACELIGHT, wxColour(0,150,0));
    this->StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour(0,220,0));
    this->StyleSetBold(wxSTC_STYLE_BRACELIGHT, true);
    this->StyleSetSize(wxSTC_STYLE_BRACELIGHT, this->StyleGetSize(0)+1);
    this->StyleSetForeground(wxSTC_STYLE_BRACEBAD, wxColour(150,0,0));
    this->StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(220,0,0));
    this->StyleSetBold(wxSTC_STYLE_BRACEBAD, true);
    this->StyleSetSize(wxSTC_STYLE_BRACEBAD, this->StyleGetSize(0)+1);
}

void NumeReEditor::applyStrikeThrough()
{
    this->SetIndicatorCurrent(HIGHLIGHT_STRIKETHROUGH);
    this->IndicatorClearRange(0, GetLastPosition());
    this->IndicatorSetStyle(HIGHLIGHT_STRIKETHROUGH, wxSTC_INDIC_STRIKE);
    this->IndicatorSetForeground(HIGHLIGHT_STRIKETHROUGH, wxColor(255,0,0));
    if (!getEditorSetting(SETTING_USETXTADV)
        || m_fileType == FILE_NSCR
        || m_fileType == FILE_NPRC
        || m_fileType == FILE_TEXSOURCE
        || m_fileType == FILE_DATAFILES)
        return;
    for (int i = 0; i < GetLastPosition(); i++)
    {
        if (GetStyleAt(i) == wxSTC_TXTADV_STRIKETHROUGH)
        {
            for (int j = i; j < GetLastPosition(); j++)
            {
                if (GetStyleAt(j) == wxSTC_TXTADV_MODIFIER || j == GetLastPosition()-1)
                {
                    this->IndicatorFillRange(i,j-i);
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

	if(m_project->IsSingleFile())
	{
		wxString oldFileName = m_fileNameAndPath.GetFullPath(m_bLastSavedRemotely ? wxPATH_UNIX : wxPATH_DOS);

		if(m_project->FileExistsInProject(oldFileName))
		{
			FileFilterType oldFilterType;
			wxString oldExtension = m_fileNameAndPath.GetExt();
			if(oldExtension.StartsWith("h"))
			{
				oldFilterType = FILE_NPRC;
			}
			else if(oldExtension.StartsWith("c"))
			{
				oldFilterType = FILE_NSCR;
			}
			else if(oldExtension.StartsWith("txt"))
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
		if(!m_project->FileExistsInProject(newFileName))
		{
			FileFilterType newFilterType;
			wxString newExtension = filename.GetExt();
			if(newExtension.StartsWith("h"))
			{
				newFilterType = FILE_NPRC;
			}
			else if(newExtension.StartsWith("c"))
			{
				newFilterType = FILE_NSCR;
			}
			else if(newExtension.StartsWith("txt"))
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
wxFileName NumeReEditor::GetFileName() {
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

	if(m_project != NULL && m_project->IsSingleFile())
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
void NumeReEditor::OnRightClick(wxMouseEvent &event)
{
    m_PopUpActive = true;
	m_lastRightClick = event.GetPosition();
	int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = LineFromPosition(charpos);

	bool breakpointOnLine = BreakpointOnLine(linenum);

	bool breakpointsAllowed = (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC);
	//bool isDebugging = true; //m_debugManager->IsDebugging();//m_mainFrame->IsDebugging();

	if(m_popupMenu.FindItem(ID_DEBUG_DISPLAY_SELECTION) != nullptr)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		//m_popupMenu.Remove(ID_DEBUG_DISPLAY_SELECTION);
		m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, false);
		m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "..."));
	}
	if(m_popupMenu.FindItem(ID_FIND_PROCEDURE) != nullptr)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		m_popupMenu.Remove(ID_FIND_PROCEDURE);
	}

	/*if(isDebugging)
	{
		breakpointsAllowed = m_debugManager->IsDebuggerPaused();//m_mainFrame->IsDebuggerPaused();
	}*/

	m_popupMenu.Enable(ID_DEBUG_ADD_BREAKPOINT, breakpointsAllowed && !breakpointOnLine);
	m_popupMenu.Enable(ID_DEBUG_REMOVE_BREAKPOINT, breakpointsAllowed && breakpointOnLine);
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

	//if(!isDebugging || (isDebugging && breakpointsAllowed))
	{
		wxString clickedWord = FindClickedWord();

		if(clickedWord.Length() > 0)
		{
            if (this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES)
            {
                wxString clickedProc = FindClickedProcedure();
                if (clickedProc.length())
                {
                    m_popupMenu.Insert(9, m_menuFindProcedure);
                    //m_popupMenu.Append(m_menuFindProcedure);
                    m_menuFindProcedure->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_FINDPROC", clickedProc.ToStdString()));
                }
            }
			//wxString watchWord = wxString::Format("Watch \"%s\"", clickedWord);
			//wxString displayWord = wxString::Format("Display \"%s\"", clickedWord);

			//m_popupMenu.Append(m_menuAddWatch);
			//m_popupMenu.Append(m_menuShowValue);
			m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, true);

			/// TODO: find definition of clicked procedure name
			//m_menuAddWatch->SetText(watchWord);
			m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", clickedWord.ToStdString()));
		}
	}

    if (this->CallTipActive())
        this->CallTipCancel();
	PopupMenu(&m_popupMenu, m_lastRightClick);
	m_PopUpActive = false;
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
void NumeReEditor::OnEditorModified(wxStyledTextEvent &event)
{
	m_project->SetCompiled(false);
	event.Skip();
}


void NumeReEditor::OnStartDrag(wxStyledTextEvent& event)
{
    /*wxTextDataObject curr_selection(this->GetSelectedText());
    wxDropSource dragSource(this);
    dragSource.SetData(curr_selection);
    wxDragResult result = dragSource.DoDragDrop(true);*/
    wxString gtxt = event.GetDragText();
    //event.SetDragText("");
    //event.SetDragResult(wxDragNone);
    //event.SetDragFlags(wxDrag_AllowMove);
    //event.SetDragResult(wxDragMove);
    //m_dragging = true;
    //DoDragOver(event.GetX(), event.GetY(), wxDragMove);
    //event.Skip();
}

void NumeReEditor::OnDragOver(wxStyledTextEvent& event)
{
    event.SetDragResult(wxDragMove);
    event.Skip();
}

void NumeReEditor::OnDrop(wxStyledTextEvent & event)
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
void NumeReEditor::OnAddBreakpoint(wxCommandEvent &event)
{
	//int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = GetLineForBreakpointOperation(); //LineFromPosition(charpos);

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
void NumeReEditor::OnRemoveBreakpoint(wxCommandEvent &event)
{
	int linenum = GetLineForBreakpointOperation();

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
void NumeReEditor::OnClearBreakpoints(wxCommandEvent &event)
{
	// m_breakpoints should have been cleared of any orphaned marker
	// handles during the right-click that led us here
	//int numBreakpoints = m_breakpoints.GetCount();
	int numBreakpoints = GetBreakpoints().GetCount();

	for(int i = 0; i < numBreakpoints; i++)
	{
		int markerHandle = m_breakpoints[i];
		int linenum = this->MarkerLineFromHandle(markerHandle);
		this->MarkerDeleteHandle(markerHandle);
		CreateBreakpointEvent(linenum, false);
	}
	m_terminal->_guimessenger.clearBreakpoints(GetFileNameAndPath().ToStdString());

	ResetRightClickLocation();
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
	for(int i = 0; i < numStoredBreakpoints; i++)
	{
		int markerHandle = m_breakpoints[i];

		int linenum = this->MarkerLineFromHandle(markerHandle);

		if(linenum != -1)
		{
			linenumbers.Add(linenum + 1);
		}
		else
		{
			invalidBreakpoints.Add(markerHandle);
		}
	}

	for(int i = 0; i < (int)invalidBreakpoints.GetCount(); i++)
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
	if(showMarker)
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
	if(m_project != NULL && m_project->IsSingleFile())
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
void NumeReEditor::OnRunToCursor(wxCommandEvent &event)
{
	/*
	int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = LineFromPosition(charpos);
	// adjust for Scintilla's internal zero-based line numbering
	linenum++;
	*/

	int linenum = GetLineForBreakpointOperation();
	wxDebugEvent debugEvent;
	debugEvent.SetId(ID_DEBUG_RUNTOCURSOR);
	debugEvent.SetSourceFilename(GetFilenameString());
	debugEvent.SetLineNumber(linenum);
	m_debugManager->AddPendingEvent(debugEvent);//m_mainFrame->AddPendingEvent(debugEvent);

	ResetRightClickLocation();
}

int NumeReEditor::GetLineForBreakpointOperation()
{
	int lineNum = 0;

	if(m_lastRightClick.x < 0 || m_lastRightClick.y < 0)
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
    m_clickedWordLength = endPosition-startPosition;
	m_clickedWord = clickedWord;
	return clickedWord;
}


wxString NumeReEditor::FindClickedProcedure()
{
	int charpos = PositionFromPoint(m_lastRightClick);
	return FindMarkedProcedure(charpos);
}

wxString NumeReEditor::FindMarkedProcedure(int charpos)
{
	int startPosition = WordStartPosition(charpos, true);
	while (startPosition && GetStyleAt(startPosition-1) == wxSTC_NSCR_PROCEDURES)
        startPosition--;
	int endPosition = WordEndPosition(charpos, true);
	while (endPosition < GetLastPosition() && GetStyleAt(endPosition) == wxSTC_NSCR_PROCEDURES)
        endPosition++;

    wxString currentline = this->GetLine(LineFromPosition(startPosition));
    if (currentline.find("procedure") != string::npos && currentline[currentline.find_first_not_of(' ', currentline.find("procedure")+9)] == '$')
        return "";

	wxString clickedWord = this->GetTextRange(startPosition, endPosition);

	if (m_fileType == FILE_NPRC && clickedWord.find('~') == string::npos)
	{
        int minpos = 0;
        int maxpos = charpos;
        while (minpos < charpos && FindText(minpos, maxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
        {
            minpos = FindText(minpos, maxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)+1;
        }
        if (FindText(minpos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
        {
            while (minpos < charpos && FindText(minpos, maxpos, "namespace", wxSTC_FIND_WHOLEWORD | wxSTC_FIND_MATCHCASE) != -1)
            {
                minpos = FindText(minpos, maxpos, "namespace", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)+1;
            }
            string currentNamespace = GetLine(LineFromPosition(minpos)).ToStdString();
            currentNamespace.erase(0,currentNamespace.find("namespace")+9);
            while (currentNamespace.back() == '\r' || currentNamespace.back() =='\n')
                currentNamespace.pop_back();
            StripSpaces(currentNamespace);
            while (currentNamespace.back() == '~')
                currentNamespace.pop_back();

            if (clickedWord[0] == '$')
                clickedWord.insert(1,currentNamespace + "~");
            else
                clickedWord = "$"+currentNamespace + "~" + clickedWord;
        }
	}

	m_clickedProcedure = clickedWord;

	if (clickedWord.find('~') != string::npos)
        clickedWord.erase(1, clickedWord.rfind('~'));
    if (clickedWord[0] != '$')
        clickedWord.insert(0,1,'$');

	return clickedWord + "()";
}

wxString NumeReEditor::generateAutoCompList(const wxString& wordstart, string sPreDefList)
{
    map<wxString,int> mAutoCompMap;
    wxString wReturn = "";
    string sCurrentWord = "";
    if (sPreDefList.length())
    {
        while (sPreDefList.length())
        {
            sCurrentWord = sPreDefList.substr(0,sPreDefList.find(' '));
            if (sCurrentWord.find('(') != string::npos)
                mAutoCompMap[toLowerCase(sCurrentWord.substr(0,sCurrentWord.find('('))) + " |" + sCurrentWord] = -1;
            else
                mAutoCompMap[toLowerCase(sCurrentWord.substr(0,sCurrentWord.find('?'))) + " |" + sCurrentWord] = -1;
            sPreDefList.erase(0,sPreDefList.find(' '));
            if (sPreDefList.front() == ' ')
                sPreDefList.erase(0,1);
        }
    }
    unsigned int nPos = 0;
    while ((nPos = this->FindText(nPos, this->GetLastPosition(), wordstart, wxSTC_FIND_WORDSTART)) != string::npos)
    {
        if (nPos > (size_t)this->GetCurrentPos() || WordEndPosition(nPos+1, true) < this->GetCurrentPos())
            mAutoCompMap[toLowerCase(this->GetTextRange(nPos, WordEndPosition(nPos+1, true)).ToStdString()) + " |" + this->GetTextRange(nPos, WordEndPosition(nPos+1, true))] = 1;
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
        wReturn += (iter->first).substr((iter->first).find('|')+1) + " ";
    }
    return wReturn;
}

void NumeReEditor::OnAddWatch(wxCommandEvent &event)
{
	wxDebugEvent dbg;

	dbg.SetId(ID_DEBUG_ADD_WATCH);

	/*
	//wxString varName = avwd.GetVariableName();
	//wxString funcName = avwd.GetFunctionName();
	//wxString className = wxEmptyString;

	if(avwd.FunctionInClass())
	{
	className = avwd.GetClassName();
	}
	*/

	wxArrayString vars;
	vars.Add(m_clickedWord);
	dbg.SetVariableNames(vars);

	m_debugManager->AddPendingEvent(dbg);//m_mainFrame->AddPendingEvent(dbg);
}

void NumeReEditor::OnDisplayVariable(wxCommandEvent &event)
{
    long int maxpos = this->GetLastPosition();
    this->SetIndicatorCurrent(HIGHLIGHT);
    this->IndicatorClearRange(0,maxpos);
    this->IndicatorSetStyle(HIGHLIGHT, wxSTC_INDIC_ROUNDBOX);
    this->IndicatorSetAlpha(HIGHLIGHT, 100);
    this->IndicatorSetForeground(HIGHLIGHT, wxColor(255,0,0));

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

void NumeReEditor::OnFindProcedure(wxCommandEvent &event)
{
    if (!m_clickedProcedure.length())
        return;
    vector<std::string> vPaths = m_terminal->getPathSettings();
    wxString pathname = m_clickedProcedure;
    if (pathname.find("$this~") != string::npos)
    {
        wxString thispath = GetFileNameAndPath();
        pathname.replace(pathname.find("$this~"), 6, thispath.substr(0,thispath.rfind('\\')+1));
    }
    else if (pathname.find("$thisfile~") != string::npos)
    {
        wxString procedurename = pathname.substr(pathname.rfind('~')+1);
        wxString procedureline;
        int nminpos = 0;
        int nmaxpos = GetLastPosition();
        while (nminpos < nmaxpos && FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
        {
            nminpos = FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;
            procedureline = GetLine(LineFromPosition(nminpos));
            if (procedureline.find("$"+procedurename) != string::npos && procedureline[procedureline.find_first_not_of(' ', procedureline.find("$"+procedurename)+procedurename.length()+1)] == '(')
            {
                this->SetFocus();
                this->EnsureVisible(LineFromPosition(nminpos));
                this->GotoLine(LineFromPosition(nminpos));
                return;
            }
        }
    }
    else
    {
        if (pathname.find("$main~") != string::npos)
            pathname.erase(pathname.find("$main~")+1,5);
        while (pathname.find('~') != string::npos)
            pathname[pathname.find('~')] = '/';
        if (pathname[0] == '$')
            pathname.replace(0,1,vPaths[5]+"/");
        else
            pathname.insert(0,vPaths[5]);
    }
    wxArrayString pathnames;
    pathnames.Add(pathname+".nprc");
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

void NumeReEditor::ClearDblClkIndicator()
{
    if (!m_dblclkString.length())
        return;
    m_dblclkString.clear();

    this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
    long int maxpos = this->GetLastPosition();
    this->IndicatorClearRange(0,maxpos);
    /*if (m_watchedString.length())
    {
        this->IndicatorSetStyle(HIGHLIGHT, wxSTC_INDIC_ROUNDBOX);
        this->IndicatorSetAlpha(HIGHLIGHT, 100);
        this->IndicatorSetForeground(HIGHLIGHT, wxColor(255,0,0));

        unsigned int nPos = 0;
        unsigned int nCurr = 0;
        vector<unsigned int> vSelectionList;

        while ((nPos = this->FindText(nCurr, maxpos, m_watchedString, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != string::npos)
        {
            vSelectionList.push_back(nPos);
            nCurr = nPos + m_watchedString.length();
        }

        this->SetIndicatorCurrent(HIGHLIGHT);

        for (size_t i = 0; i < vSelectionList.size(); i++)
        {
            this->IndicatorFillRange(vSelectionList[i], m_watchedString.length());
        }
    }*/
}

void NumeReEditor::OnMarginClick( wxStyledTextEvent &event )
{
	// we know it's margin 2, because that's the only sensitive margin

    if (m_fileType != FILE_NSCR && m_fileType != FILE_NPRC)
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
        if(BreakpointOnLine(linenum))
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
	int markerNum = this->MarkerAdd(linenum, MARKER_BREAKPOINT);

	m_breakpoints.Add(markerNum);
	CreateBreakpointEvent(linenum, true);
	m_terminal->_guimessenger.addBreakpoint(GetFileNameAndPath().ToStdString(), linenum);
}

bool NumeReEditor::BreakpointOnLine( int linenum )
{
	int markerLineBitmask = this->MarkerGet(linenum);

	bool breakpointOnLine = (markerLineBitmask & (1 << MARKER_BREAKPOINT));

	return breakpointOnLine;
}

void NumeReEditor::RemoveBreakpoint( int linenum )
{
	// need to remove the marker handle from the array - use
	// LineFromHandle on debug start and clean up then
	this->MarkerDelete(linenum, MARKER_BREAKPOINT);
	CreateBreakpointEvent(linenum, false);
	m_terminal->_guimessenger.removeBreakpoint(GetFileNameAndPath().ToStdString(), linenum);
}


int NumeReEditor::determineIndentationLevel(std::string sLine, bool& bIsElseCase)
{
    int nIndentCount = 0;
    Match _mMatch;
    while (sLine.length())
    {
        if (sLine.find("<install>") != string::npos
            || sLine.find("<info>") != string::npos
            || sLine.find("<helpindex>") != string::npos
            || sLine.find("<helpfile>") != string::npos
            || sLine.find("<article") != string::npos
            || sLine.find("<keywords>") != string::npos
            || sLine.find("<keyword>") != string::npos
            || sLine.find("<codeblock>") != string::npos
            || sLine.find("<exprblock>") != string::npos
            || sLine.find("<example") != string::npos
            || sLine.find("<item") != string::npos
            || sLine.find("<list") != string::npos)
        {
            nIndentCount++;
            sLine.erase(0,sLine.find('>')+1);
        }
        if (sLine.find("<endinstall>") != string::npos
            || sLine.find("<endinfo>") != string::npos
            || sLine.find("</helpindex>") != string::npos
            || sLine.find("</helpfile>") != string::npos
            || sLine.find("</article>") != string::npos
            || sLine.find("</keywords>") != string::npos
            || sLine.find("</keyword>") != string::npos
            || sLine.find("</codeblock>") != string::npos
            || sLine.find("</exprblock>") != string::npos
            || sLine.find("</example>") != string::npos
            || sLine.find("</item>") != string::npos
            || sLine.find("</list>") != string::npos)
        {
            nIndentCount--;
            sLine.erase(0,sLine.find('>')+1);
        }
        _mMatch = findCommand(sLine);
        if (_mMatch.nPos == string::npos)
            return nIndentCount;
        if (_mMatch.sString == "endif"
            || _mMatch.sString == "endwhile"
            || _mMatch.sString == "endfor"
            || _mMatch.sString == "endcompose"
            || _mMatch.sString == "endprocedure")
        {
            nIndentCount--;
        }
        else if (_mMatch.sString == "if"
            || _mMatch.sString == "while"
            || _mMatch.sString == "for"
            || _mMatch.sString == "compose"
            || _mMatch.sString == "procedure")
        {
            nIndentCount++;
        }
        else if (_mMatch.sString == "elseif"
            || _mMatch.sString == "else")
        {
            bIsElseCase = true;
        }
        sLine.erase(0,_mMatch.nPos+_mMatch.sString.length());
    }
    return nIndentCount;
}

string NumeReEditor::realignLangString(string sLine, size_t& lastpos)
{
    lastpos = sLine.find(' ');
    size_t firstpos = sLine.find_first_not_of(' ', lastpos);
    if (sLine.find("- ") == firstpos)
        return sLine;
    if (firstpos-lastpos > 2)
    {
        sLine.erase(lastpos, firstpos-lastpos-2);
        sLine.insert(sLine.find("- "), firstpos-lastpos-2, ' ');
    }
    return sLine;
}

string NumeReEditor::addLinebreaks(const string& sLine)
{
    const unsigned int nMAXLINE = 80;
    if (sLine.length() < nMAXLINE)
        return sLine;

    string sReturn = sLine;
    unsigned int nIndentPos = sReturn.find("- ")+2;
    unsigned int nLastLineBreak = 0;
    for (unsigned int i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == '\n')
            nLastLineBreak = i;
        if ((i == nMAXLINE && !nLastLineBreak)
            || (nLastLineBreak && i - nLastLineBreak == nMAXLINE))
        {
            for (int j = i; j >= 0; j--)
            {
                if (sReturn[j] == ' ')
                {
                    sReturn[j] = '\n';
                    sReturn.insert(j+1, nIndentPos, ' ');
                    nLastLineBreak = j;
                    break;
                }
                else if (sReturn[j] == '-' && j != (int)i)
                {
                    // --> Minuszeichen: nicht immer ist das Trennen an dieser Stelle sinnvoll. Wir pruefen die einfachsten Faelle <--
                    if (j &&
                        (sReturn[j-1] == ' '
                        || sReturn[j-1] == '('
                        || sReturn[j+1] == ')'
                        || sReturn[j-1] == '['
                        || (sReturn[j+1] >= '0' && sReturn[j+1] <= '9')
                        || sReturn[j+1] == ','
                        || (sReturn[j+1] == '"' && sReturn[j-1] == '"')
                        ))
                        continue;
                    sReturn.insert(j+1, "\n");
                    sReturn.insert(j+2, nIndentPos, ' ');
                    nLastLineBreak = j+1;
                    break;
                }
                else if (sReturn[j] == ',' && j != (int)i && sReturn[j+1] != ' ')
                {
                    sReturn.insert(j+1, "\n");
                    sReturn.insert(j+2, nIndentPos, ' ');
                    nLastLineBreak = j+1;
                    break;
                }
            }
        }
    }
    return sReturn;
}



void NumeReEditor::ApplyAutoIndentation()
{
    int nIndentCount = 0;
    int nCurrentIndent = 0;
    unsigned int nLines = this->GetLineCount();
    string currentLine = "";
    bool bBlockComment = false;
    bool bIsElseCase = false;
    this->SetTabWidth(4);
    this->BeginUndoAction();
    //this->SetUseTabs(true);
    for (size_t i = 0; i < nLines; i++)
    {
        bIsElseCase = false;
        currentLine = this->GetLine(i).ToStdString();
        while (currentLine.back() == '\r' || currentLine.back() == '\n')
            currentLine.pop_back();

        if (currentLine.find("##") != string::npos)
            currentLine.erase(currentLine.find("##"));
        if (currentLine.find("*#") != string::npos && bBlockComment)
        {
            currentLine.erase(0, currentLine.find("*#")+2);
            bBlockComment = false;
        }
        else if (bBlockComment)
            continue;
        if (currentLine.find("#*") != string::npos && !bBlockComment)
        {
            while (currentLine.find("#*") != string::npos && currentLine.find("*#", currentLine.find("#*")+2) != string::npos)
            {
                currentLine.erase(currentLine.find("#*"), currentLine.find("*#", currentLine.find("#*")+2)-currentLine.find("#*")+2);
            }
            if (currentLine.find("#*") != string::npos)
            {
                bBlockComment = true;
                currentLine.erase(currentLine.find("#*"));
            }
        }
        if (currentLine.find_first_not_of(" \t") == string::npos)
            continue;
        currentLine = " " + currentLine + " ";

        nCurrentIndent = determineIndentationLevel(currentLine, bIsElseCase);
        if (!nCurrentIndent && bIsElseCase)
        {
            this->SetLineIndentation(i, 4*(nIndentCount-1));
        }
        else if (!nCurrentIndent)
        {
            this->SetLineIndentation(i, 4*nIndentCount);
        }
        else if (nCurrentIndent < 0)
        {
            nIndentCount += nCurrentIndent;
            this->SetLineIndentation(i, 4*nIndentCount);
        }
        else
        {
            this->SetLineIndentation(i, 4*nIndentCount);
            nIndentCount += nCurrentIndent;
        }
    }
    this->EndUndoAction();
}





