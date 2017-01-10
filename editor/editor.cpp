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
#define HIGHLIGHT 20
#define HIGHLIGHT_DBLCLK 21

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
	EVT_STC_MARGINCLICK (-1, NumeReEditor::OnMarginClick)
	EVT_MENU			(ID_DEBUG_ADD_BREAKPOINT, NumeReEditor::OnAddBreakpoint)
	EVT_MENU			(ID_DEBUG_REMOVE_BREAKPOINT, NumeReEditor::OnRemoveBreakpoint)
	EVT_MENU			(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, NumeReEditor::OnClearBreakpoints)
	EVT_MENU			(ID_DEBUG_WATCH_SELECTION, NumeReEditor::OnAddWatch)
	EVT_MENU			(ID_DEBUG_DISPLAY_SELECTION, NumeReEditor::OnDisplayVariable)
	//EVT_COMPILER_END	(ChameleonEditor::OnCompilerEnded)
	EVT_MENU			(ID_DEBUG_RUNTOCURSOR, NumeReEditor::OnRunToCursor)
END_EVENT_TABLE()

int CompareInts(int n1, int n2)
{
	return n1 - n2;
}


extern Language _guilang;


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
                                  wxWindow *parent,     wxWindowID id, NumeReSyntax* __syntax, const
                                      wxPoint & pos /* = wxDefaultPosition */,
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

	m_bWrapMode = false;
	m_fileType = FILE_NONSOURCE;

	m_bLoadingFile = false;
	m_bLastSavedRemotely = true;
	m_bHasBeenCompiled = false;

	m_fileNameAndPath.Assign(wxEmptyString);

	m_lastRightClick.x = -1;
	m_lastRightClick.y = -1;
	_syntax = __syntax;

    this->SetTabWidth(4);
    this->SetIndent(4);
    this->SetUseTabs(true);

    this->SetAdditionalSelectionTyping(true);

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
	m_menuShowValue = m_popupMenu.Append(ID_DEBUG_DISPLAY_SELECTION, _guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "selection"));


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



	bool isModified = modified && readonly && canundo;
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

    if( chr == WXK_TAB )
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
	if(chr == '\n')// && m_options->GetPerms()->isEnabled(PERM_AUTOINDENT))
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
	}

    int lenEntered = currentPos-wordstartpos;

    if (lenEntered > 1
        && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_LINE
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_COMMENT_BLOCK
        && GetStyleAt(wordstartpos) != wxSTC_NSCR_STRING)
    {
        this->AutoCompSetIgnoreCase(true);
        this->AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
        this->AutoCompShow(lenEntered, _syntax->getAutoCompList(GetTextRange(wordstartpos, currentPos).ToStdString()));
    }
    this->Colourise(0, -1);
	return;
}


void NumeReEditor::MakeBraceCheck()
{
    char CurrentChar = this->GetCharAt(this->GetCurrentPos());
    char PrevChar = 0;
    if (this->GetCurrentPos())
        PrevChar = this->GetCharAt(this->GetCurrentPos()-1);
    if (CurrentChar == '(' || CurrentChar == ')'
        || CurrentChar == '[' || CurrentChar == ']'
        || CurrentChar == '{' || CurrentChar == '}')
        getMatchingBrace(this->GetCurrentPos());
    else if (PrevChar == '(' || PrevChar == ')'
        || PrevChar == '[' || PrevChar == ']'
        || PrevChar == '{' || PrevChar == '}')
        getMatchingBrace(this->GetCurrentPos()-1);
    else
    {
        this->BraceBadLight(wxSTC_INVALID_POSITION);
        this->BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
    }
    return;
}

void NumeReEditor::OnKeyDn(wxKeyEvent &event)
{
    //wxMessageBox(wxString((char)this->GetCharAt(this->GetCurrentPos())));
    OnKeyDown(event);
    MakeBraceCheck();
    ClearDblClkIndicator();
}

void NumeReEditor::OnKeyRel(wxKeyEvent &event)
{
    //wxMessageBox(wxString((char)this->GetCharAt(this->GetCurrentPos())));
    MakeBraceCheck();
    //OnKeyUp(event);
}

void NumeReEditor::OnMouseUp(wxMouseEvent &event)
{
    OnMouseLeftUp(event);
    MakeBraceCheck();
}

void NumeReEditor::OnMouseDn(wxMouseEvent &event)
{
    OnMouseLeftDown(event);
    ClearDblClkIndicator();
    MakeBraceCheck();
}

void NumeReEditor::OnMouseDblClk(wxMouseEvent& event)
{
    m_lastRightClick = event.GetPosition();
    wxString selection = FindClickedWord();
    if (!selection.length())
        return;

    m_dblclkString = selection;
    long int maxpos = this->GetLastPosition();
    this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
    this->IndicatorClearRange(0,maxpos);
    this->IndicatorSetStyle(HIGHLIGHT_DBLCLK, wxSTC_INDIC_ROUNDBOX);
    this->IndicatorSetAlpha(HIGHLIGHT_DBLCLK, 80);
    this->IndicatorSetForeground(HIGHLIGHT_DBLCLK, wxColor(0,255,0));

    unsigned int nPos = 0;
    unsigned int nCurr = 0;
    vector<unsigned int> vSelectionList;

    while ((nPos = this->FindText(nCurr, maxpos, selection, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != string::npos)
    {
        vSelectionList.push_back(nPos);
        nCurr = nPos + selection.length();
    }

    //this->SetIndicatorCurrent(HIGHLIGHT_DBLCLK);

    for (size_t i = 0; i < vSelectionList.size(); i++)
    {
        this->IndicatorFillRange(vSelectionList[i], selection.length());
    }

}


bool NumeReEditor::getWrapMode()
{
    return m_bWrapMode;
}

void NumeReEditor::ToggleLineWrap()
{
    if (!m_bWrapMode)
    {
        m_bWrapMode = true;
        this->SetWrapMode(wxSTC_WRAP_WORD);
        this->SetWrapIndentMode(wxSTC_WRAPINDENT_INDENT);
        this->SetWrapStartIndent(1);
        this->SetWrapVisualFlags(wxSTC_WRAPVISUALFLAG_END);
        this->SetWrapVisualFlagsLocation(wxSTC_WRAPVISUALFLAGLOC_END_BY_TEXT);
    }
    else
    {
        m_bWrapMode = false;
        this->SetWrapMode(wxSTC_WRAP_NONE);
    }
}

void NumeReEditor::getMatchingBrace(int nPos)
{
    int nMatch = this->BraceMatch(nPos);
    if (nMatch == wxSTC_INVALID_POSITION)
        this->BraceBadLight(nPos);
    else
    {
        if (nMatch < nPos)
            this->BraceHighlight(nMatch, nPos);
        else
            this->BraceHighlight(nPos, nMatch);
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
	return result;
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
	m_fileType = filetype;
    this->SetCaretLineVisible(true);
    this->SetIndentationGuides(true);
    this->SetCaretLineBackground(wxColour(196,211,255));

	if (m_bNewFile || filetype == FILE_NSCR || filetype == FILE_NPRC || !HasBeenSaved())
	{
        m_fileType = FILE_NSCR;
		this->SetLexer(wxSTC_LEX_NSCR);

		this->SetProperty("fold", "1");
		//this->SetProperty("fold.comment", "1");
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
            this->SetKeyWords(0, _syntax->getCommands());//"about append audio break clear compose cont cont3d continue contour contour3d copy credits datagrid define del delete dens dens3d density density3d diff draw draw3d edit eval explicit export extrema fft find fit fitw get global grad gradient grad3d gradient3d graph graph3d help hist hist2d hline ifndef ifndefined info integrate integrate2 integrate2d list load man matop mesh mesh3d meshgrid meshgrid3d move mtrxop new odesolve plot plot3d print progress pulse quit random read redef redefine regularize reload remove rename repl replaceline resample retoque save script search set show showf smooth sort stats stfa subplot surf surf3d surface surface3d swap taylor undef undefine uninstall vect vect3d vector vector3d view workpath write zeroes else elseif endfor endif endwhile endcompose explicit for if inline main namespace private readline return throw while procedure endprocedure str var");
            this->SetKeyWords(1, _syntax->getOptions());//"addxaxis addyaxis adventor all allmedium alpha alphamask animate app append area aspect asstr asval autosave axis axisbind background bar bars bcancel bgcolorscheme binlabel binomial bins bonum bottomleft bottomright box boxplot buffersize cancel cartesian chimap chorus clog cloudplot cmd coarse coast cold colorbar colormask colorrange colorscheme colortheme cols comment comments compact complete complex connect const copper coords countlabel cscale cticklabels cticks cursor cut defcontrol desc dir distrib draftmode editor eps errorbars every exprvar extendedfileinfo faststart fcont file files fine first flength flow font free freedman freedoms func fx0 gamma gauss greeting grey grid gridstyle hbars head heros heroscn hints hires hot ignore interpolate inverse iter keepdim last lborder lcont legend legendstyle light lines linesizes linestyles lnumctrl loademptycols loadpath logic logscale lyapunov map marks mask max maxline mean medium method min minline mode moy msg multiplot noalpha noalphamask noanimate noarea noaxis nobackground nobars nobox noboxplot noclog nocloudplot nocolorbar nocolormask noconnect nocut noerrorbars nofcont noflength noflow nogrid nohbars nohires nointerpolate nolcont nolight nologscale nomarks noopen noorthoproject nopcont nopipe nopoints noquotes noregion normal noschematic nosteps nosilent noxlog noyerrorbars noylog nozlog nq oeps ogif onlycolors onlystyles open opng order origin orthoproject osvg otex override pagella params pattern pcont peek perspective pipe plasma plotcolors plotfont plotparams plotpath plugin plugins points pointstyles poisson polar precision prob proc procpath rainbow rborder real recursive region reset restrict rk2 rk4 rk8pd rkck rkf45 rotate samples savepath saverr scale schematic schola scott scriptpath settings shape silent simpson single slices sliding spherical std steps student styles sv target termes textsize title tocache tol topleft topright transpose trapezoidal trunc type ubound uniform unique units usecustomlang useescinscripts var viridis viewer vline width windowsize with xerrorbars xlabel xlog xscale xticklabels xticks xvals xy xz yerrorbars ylabel ylog yscale yticklabels yticks zlabel zlog zscale zticklabels zticks");
            this->SetKeyWords(2, _syntax->getFunctions());//"abs acos acosh Ai arc arcv arccos arcsin arctan arcosh arsinh artanh asin asinh ascii atan atanh avg bessel betheweizsaecker Bi binom char circle cmp cnt cone conev cos cosh cot cross cuboid curve date dblfacul degree det diag diagonalize drop eigenvals eigenvects ellipse ellipsev ellipticD ellipticE ellipticF ellipticPi erf erfc exp face facev faculty findfile findparam floor gamma gauss gcd getfilelist getfolderlist getindices getmatchingparens getopt heaviside hermite identity imY invert is_data is_nan is_string laguerre laguerre_a lcm legendre legendre_a ln log log10 log2 matfc matfcf matfl matflf max med min neumann norm num one pct phi point polygon polygonv prd radian rand range rect repeat replace replaceall rint roof round sbessel sign sin sinc sinh sneumann solve split sphere sqrt std strfnd string_cast strlen strrfnd student_t substr sum tan tanh text theta time to_char to_cmd to_lowercase to_string to_uppercase to_value trace tracev transpose triangle trianglev valtostr version Y zero");
            this->SetKeyWords(3, "x y z t");
            this->SetKeyWords(4, _syntax->getConstants());//"_pi _2pi _g _R _alpha_fs _c _coul_norm _elek_feldkonst _elem_ladung _g _h _hbar _k_boltz _m_amu _m_elektron _m_erde _m_muon _m_neutron _m_proton _m_sonne _m_tau _magn_feldkonst _mu_bohr _mu_kern _n_avogadro _pi _r_bohr _r_erde _r_sonne _theta_weinberg");
            this->SetKeyWords(5, _syntax->getSpecial());//"... ans cache data false inf nan string true void");
            this->SetKeyWords(6, _syntax->getOperators());//"<loadpath> <savepath> <scriptpath> <procpath> <plotpath> <this> <wp> 'Torr 'eV 'fm 'A 'b 'AU 'ly 'pc 'mile 'yd 'ft 'in 'cal 'TC 'TF 'Ci 'G 'kmh 'kn 'l 'mph 'psi 'Ps 'mol 'Gs 'M 'k 'm 'mu 'n");
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

        this->StyleSetForeground(wxSTC_NSCR_DEFAULT_VARS, wxColour(0, 0, 160));
		this->StyleSetBold(wxSTC_NSCR_DEFAULT_VARS, true);
		this->StyleSetItalic(wxSTC_NSCR_DEFAULT_VARS, true);

        this->StyleSetForeground(wxSTC_NSCR_CUSTOM_FUNCTION, wxColour(0, 0, 160));

        this->StyleSetForeground(wxSTC_NSCR_OPERATORS, wxColour(255, 0, 0));
        this->StyleSetForeground(wxSTC_NSCR_OPERATOR_KEYWORDS, wxColour(255, 0, 0));

        this->StyleSetForeground(34, wxColour(0,150,0));
        this->StyleSetBackground(34, wxColour(0,220,0));
        this->StyleSetBold(34, true);
        this->StyleSetForeground(35, wxColour(150,0,0));
        this->StyleSetBackground(35, wxColour(220,0,0));
        this->StyleSetBold(35, true);
		this->Colourise(0, -1);
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
        this->StyleSetForeground(wxSTC_TEX_SPECIAL, wxColor(255,0,196)); // Parentheses/Brackets
        this->StyleSetItalic(wxSTC_TEX_SPECIAL, false);
        this->StyleSetBold(wxSTC_TEX_SPECIAL, true);
        this->StyleSetForeground(wxSTC_TEX_SYMBOL, wxColor(255,0,0)); // Operators
        this->StyleSetBackground(wxSTC_TEX_SYMBOL, wxColor(255,255,255));
        this->StyleSetBold(wxSTC_TEX_SYMBOL, false);

        this->StyleSetForeground(34, wxColour(0,150,0));
        this->StyleSetBackground(34, wxColour(0,220,0));
        this->StyleSetBold(34, true);
        this->StyleSetForeground(35, wxColour(150,0,0));
        this->StyleSetBackground(35, wxColour(220,0,0));
        this->StyleSetBold(35, true);
        this->Colourise(0,-1);
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
        this->StyleSetForeground(34, wxColour(0,150,0));
        this->StyleSetBackground(34, wxColour(0,220,0));
        this->StyleSetBold(34, true);
        this->StyleSetForeground(35, wxColour(150,0,0));
        this->StyleSetBackground(35, wxColour(220,0,0));
        this->StyleSetBold(35, true);
        this->Colourise(0,-1);
	}
	else
	{
		this->SetLexer(wxSTC_LEX_NULL);
		this->StyleSetItalic(0,false);
		this->ClearDocumentStyle();
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
	m_lastRightClick = event.GetPosition();
	int charpos = PositionFromPoint(m_lastRightClick);
	int linenum = LineFromPosition(charpos);

	bool breakpointOnLine = BreakpointOnLine(linenum);

	bool breakpointsAllowed = true;
	bool isDebugging = true; //m_debugManager->IsDebugging();//m_mainFrame->IsDebugging();

	if(m_popupMenu.FindItem(ID_DEBUG_DISPLAY_SELECTION) != NULL)
	{
		//m_popupMenu.Remove(ID_DEBUG_WATCH_SELECTION);
		m_popupMenu.Remove(ID_DEBUG_DISPLAY_SELECTION);
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


	if(!isDebugging || (isDebugging && breakpointsAllowed))
	{
		wxString clickedWord = FindClickedWord();

		if(clickedWord.Length() > 0)
		{
			//wxString watchWord = wxString::Format("Watch \"%s\"", clickedWord);
			//wxString displayWord = wxString::Format("Display \"%s\"", clickedWord);

			//m_popupMenu.Append(m_menuAddWatch);
			m_popupMenu.Append(m_menuShowValue);

			/// TODO: find definition of clicked procedure name
			//m_menuAddWatch->SetText(watchWord);
			m_menuShowValue->SetText(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", clickedWord.ToStdString()));
		}
	}

	PopupMenu(&m_popupMenu, m_lastRightClick);
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

	m_clickedWord = clickedWord;
	return clickedWord;
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
        nCurr = nPos + m_clickedWord.length();
    }

   // this->SetIndicatorCurrent(HIGHLIGHT);

    for (size_t i = 0; i < vSelectionList.size(); i++)
    {
        this->IndicatorFillRange(vSelectionList[i], m_clickedWord.length());
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
}


int NumeReEditor::determineIndentationLevel(std::string sLine, bool& bIsElseCase)
{
    int nIndentCount = 0;
    Match _mMatch;
    while (sLine.length())
    {
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

void NumeReEditor::ApplyAutoIndentation()
{
    int nIndentCount = 0;
    int nCurrentIndent = 0;
    unsigned int nLines = this->GetLineCount();
    string currentLine = "";
    bool bBlockComment = false;
    bool bIsElseCase = false;
    this->SetTabWidth(4);
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
}





