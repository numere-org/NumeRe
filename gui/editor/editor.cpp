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
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <regex>

#include "editor.h"
#include "../NumeReWindow.h"
#include "../NumeReNotebook.h"
#include "codeanalyzer.hpp"
#include "searchcontroller.hpp"
#include "codeformatter.hpp"

#include "../../common/datastructures.h"
#include "../../common/Options.h"
#include "../../common/vcsmanager.hpp"
#include "../../common/filerevisions.hpp"
#include "../dialogs/renamesymbolsdialog.hpp"
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
#define HIGHLIGHT_LOCALVARIABLES 13
#define ANNOTATION_NOTE wxSTC_NSCR_PROCEDURE_COMMANDS+1
#define ANNOTATION_WARN wxSTC_NSCR_PROCEDURE_COMMANDS+2
#define ANNOTATION_ERROR wxSTC_NSCR_PROCEDURE_COMMANDS+3

#define SEMANTICS_VAR 1
#define SEMANTICS_STRING 2
#define SEMANTICS_NUM 4
#define SEMANTICS_FUNCTION 8

#define LINELENGTH_COLUMN 100


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
//    EVT_ENTER_WINDOW    (NumeReEditor::OnEnter)
    EVT_LEAVE_WINDOW    (NumeReEditor::OnLeave)
    EVT_KILL_FOCUS      (NumeReEditor::OnLoseFocus)
    EVT_STC_DWELLSTART  (-1, NumeReEditor::OnMouseDwell)
    EVT_STC_MARGINCLICK (-1, NumeReEditor::OnMarginClick)
    EVT_STC_DRAG_OVER   (-1, NumeReEditor::OnDragOver)
    EVT_STC_SAVEPOINTREACHED (-1, NumeReEditor::OnSavePointReached)
    EVT_STC_SAVEPOINTLEFT (-1, NumeReEditor::OnSavePointLeft)
    EVT_STC_AUTOCOMP_SELECTION (-1, NumeReEditor::OnAutoCompletion)
    EVT_MENU			(ID_DEBUG_ADD_BREAKPOINT, NumeReEditor::OnAddBreakpoint)
    EVT_MENU			(ID_DEBUG_EDIT_BREAKPOINT, NumeReEditor::OnEditBreakpoint)
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
    EVT_MENU            (ID_HIDE_SELECTION, NumeReEditor::OnHideSelection)
    EVT_MENU            (ID_MENU_HELP_ON_ITEM, NumeReEditor::OnHelpOnSelection)
    EVT_MENU            (ID_RENAME_SYMBOLS, NumeReEditor::OnRenameSymbols)
    EVT_MENU            (ID_FIX_TYPE, NumeReEditor::OnFixTypes)
    EVT_MENU            (ID_ABSTRAHIZE_SECTION, NumeReEditor::OnAbstrahizeSection)
    EVT_MENU            (ID_MENU_COPY, NumeReEditor::OnMenuEvent)
    EVT_MENU            (ID_MENU_CUT, NumeReEditor::OnMenuEvent)
    EVT_MENU            (ID_MENU_PASTE, NumeReEditor::OnMenuEvent)
    EVT_IDLE            (NumeReEditor::OnIdle)
    EVT_TIMER           (ID_ANALYZERTIMER, NumeReEditor::OnAnalyzerTimer)
END_EVENT_TABLE()

int CompareInts(int n1, int n2)
{
    return n1 - n2;
}


extern Language _guilang;
using namespace std;

/////////////////////////////////////////////////
/// \brief Editor constructor.
///
/// \param mframe NumeReWindow*
/// \param options Options*
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param __syntax NumeReSyntax*
/// \param __terminal wxTerm*
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
/// \param name const wxString&
///
/////////////////////////////////////////////////
NumeReEditor::NumeReEditor(NumeReWindow* mframe, Options* options, wxWindow* parent, wxWindowID id,
                           NumeReSyntax* __syntax, NumeReTerminal* __terminal, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
                               wxStyledTextCtrl(parent, id, pos, size, style, name)
{
    defaultPage = false;
    m_mainFrame = mframe;
    m_options = options;
    m_analyzer = new CodeAnalyzer(this, options);
    m_search = new SearchController(this, __terminal);
    m_formatter = new CodeFormatter(this);
    m_duplicateCode = nullptr;
    m_nCallTipStart = 0;
    m_modificationHappened = false;
    m_nDuplicateCodeLines = 6;
    m_braceIndicatorActive = false;
    m_blockIndicatorActive = false;

    m_nextChar = 0;
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
    vBlockDefs = _syntax->getFullBlockDefs();
    m_terminal = __terminal;
    m_dragging = false;

    m_nFirstLine = m_nLastLine = 0;
    m_nLastReleasedKey = 0;
    m_nDuplicateCodeFlag = 0;
    m_procedureViewer = nullptr;
    m_analyzerTimer = new wxTimer(this, ID_ANALYZERTIMER);
    m_isFunctionContext = false;

    Bind(wxEVT_THREAD, &NumeReEditor::OnThreadUpdate, this);

    SetTabWidth(4);
    SetIndent(4);
    SetUseTabs(true);

    AutoCompSetIgnoreCase(true);
    AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_RESPECTCASE);

    SetMultipleSelection(true);
    SetVirtualSpaceOptions(wxSTC_SCVS_RECTANGULARSELECTION);
    SetAdditionalSelectionTyping(true);
    SetMultiPaste(wxSTC_MULTIPASTE_EACH);

    SetMarginWidth(0, 40);
    SetMarginType(0, wxSTC_MARGIN_NUMBER);

    SetMarginWidth(1, 20);
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);

    SetYCaretPolicy(wxSTC_CARET_SLOP | wxSTC_CARET_STRICT | wxSTC_CARET_EVEN, 1);

    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    //wxInitAllImageHandlers();
    RegisterImage(NumeReSyntax::SYNTAX_COMMAND,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\cmd.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_FUNCTION,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\fnc.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_OPTION,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\opt.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_CONSTANT,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\cnst.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_SPECIALVAL,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\spv.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_OPERATOR,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\opr.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_METHODS,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\mthd.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_PROCEDURE,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\prc.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_STD,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\var.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_CLUSTER,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\cst.png", wxBITMAP_TYPE_PNG));
    RegisterImage(NumeReSyntax::SYNTAX_TABLE,
                  wxBitmap(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "icons\\tab.png", wxBITMAP_TYPE_PNG));

    wxFont font = m_options->GetEditorFont();
    StyleSetFont(wxSTC_STYLE_DEFAULT, font);

    // Add the characters for procedures to the word char list
    // this->SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$~");

    StyleClearAll();

    SetMouseDwellTime(500);

    EmptyUndoBuffer();
    m_bSetUnsaved = false;
    m_bNewFile = true;
    UpdateSyntaxHighlighting();
    m_bNewFile = false;

    MarkerDefine(MARKER_BREAKPOINT, wxSTC_MARK_CIRCLE);
    MarkerSetBackground(MARKER_BREAKPOINT, wxColour("red"));

    MarkerDefine(MARKER_CONDITIONALBREAKPOINT, wxSTC_MARK_CIRCLE);
//    MarkerDefineBitmap(MARKER_CONDITIONALBREAKPOINT, wxBitmap(wxImage(m_mainFrame->getProgramFolder() +  "/icons/color/add-bp.png", wxBITMAP_TYPE_PNG)));
    MarkerSetBackground(MARKER_CONDITIONALBREAKPOINT, wxColour(128,0,255));

    MarkerDefine(MARKER_BOOKMARK, wxSTC_MARK_SMALLRECT);
    MarkerSetBackground(MARKER_BOOKMARK, wxColour(192, 0, 64));

    MarkerDefine(MARKER_FOCUSEDLINE, wxSTC_MARK_SHORTARROW);
    MarkerSetBackground(MARKER_FOCUSEDLINE, wxColour("yellow"));

    MarkerDefine(MARKER_MODIFIED, wxSTC_MARK_LEFTRECT);
    MarkerSetBackground(MARKER_MODIFIED, wxColour(255, 220, 0));

    MarkerDefine(MARKER_SAVED, wxSTC_MARK_LEFTRECT);
    MarkerSetBackground(MARKER_SAVED, wxColour("green"));

    MarkerDefine(MARKER_SECTION, wxSTC_MARK_ARROWDOWN);
    MarkerSetBackground(MARKER_SECTION, m_options->GetSyntaxStyle(Options::COMMENT).foreground);

    MarkerDefine(MARKER_HIDDEN, wxSTC_MARK_UNDERLINE);
    MarkerSetBackground(MARKER_HIDDEN, wxColour(128, 128, 128));
    MarkerDefine(MARKER_HIDDEN_MARGIN, wxSTC_MARK_DOTDOTDOT);
    MarkerSetBackground(MARKER_HIDDEN_MARGIN, wxColour(128, 128, 128));

    MarkerDefine(MARKER_DUPLICATEINDICATOR_ONE, wxSTC_MARK_BACKGROUND);
    MarkerSetBackground(MARKER_DUPLICATEINDICATOR_ONE, wxColour(220, 255, 220));

    MarkerDefine(MARKER_DUPLICATEINDICATOR_TWO, wxSTC_MARK_BACKGROUND);
    MarkerSetBackground(MARKER_DUPLICATEINDICATOR_TWO, wxColour(255, 220, 220));

    SetMarginSensitive(1, true);

    UsePopUp(false);

    UpdateIndicators();

    SetCaretPeriod(m_options->GetCaretBlinkTime());

    m_refactoringMenu = new wxMenu();
    m_refactoringMenu->Append(ID_RENAME_SYMBOLS,
                              _guilang.get("GUI_MENU_EDITOR_RENAME_SYMBOLS"),
                              _guilang.get("GUI_MENU_EDITOR_RENAME_SYMBOLS_TTP"));
    m_refactoringMenu->Append(ID_FIX_TYPE,
                              _guilang.get("GUI_MENU_EDITOR_FIX_TYPE"),
                              _guilang.get("GUI_MENU_EDITOR_FIX_TYPE_TTP"));
    m_refactoringMenu->Append(ID_ABSTRAHIZE_SECTION,
                              _guilang.get("GUI_MENU_EDITOR_ABSTRAHIZE_SECTION"),
                              _guilang.get("GUI_MENU_EDITOR_ABSTRAHIZE_SECTION_TTP"));

    m_popupMenu.Append(ID_MENU_CUT, _guilang.get("GUI_MENU_EDITOR_CUT"));
    m_popupMenu.Append(ID_MENU_COPY, _guilang.get("GUI_MENU_EDITOR_COPY"));
    m_popupMenu.Append(ID_MENU_PASTE, _guilang.get("GUI_MENU_EDITOR_PASTE"));
    m_popupMenu.AppendSeparator();

    m_popupMenu.Append(ID_MENU_EXECUTE_FROM_LINE, _guilang.get("GUI_MENU_EDITOR_RUN_FROM_LINE"));
    m_popupMenu.AppendSeparator();

    m_popupMenu.Append(ID_FOLD_CURRENT_BLOCK, _guilang.get("GUI_MENU_EDITOR_FOLDCURRENTBLOCK"));
    m_popupMenu.Append(ID_HIDE_SELECTION, _guilang.get("GUI_MENU_EDITOR_HIDECURRENTBLOCK"));
    m_popupMenu.AppendSeparator();

    m_popupMenu.Append(ID_DEBUG_ADD_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_ADDBP"));
    m_popupMenu.Append(ID_DEBUG_EDIT_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_EDITBP"));
    m_popupMenu.Append(ID_DEBUG_REMOVE_BREAKPOINT, _guilang.get("GUI_MENU_EDITOR_REMOVEBP"));
    m_popupMenu.Append(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, _guilang.get("GUI_MENU_EDITOR_CLEARBP"));
    m_popupMenu.AppendSeparator();

    m_popupMenu.Append(ID_BOOKMARK_ADD, _guilang.get("GUI_MENU_EDITOR_ADDBM"));
    m_popupMenu.Append(ID_BOOKMARK_REMOVE, _guilang.get("GUI_MENU_EDITOR_REMOVEBM"));
    m_popupMenu.Append(ID_BOOKMARK_CLEAR, _guilang.get("GUI_MENU_EDITOR_CLEARBM"));

    m_popupMenu.AppendSeparator();

    //m_menuAddWatch = m_popupMenu.Append(ID_DEBUG_WATCH_SELECTION, "Watch selection");
    m_menuFindProcedure = m_popupMenu.Append(ID_FIND_PROCEDURE, _guilang.get("GUI_MENU_EDITOR_FINDPROC", "$procedure"));
    m_menuFindInclude = m_popupMenu.Append(ID_FIND_INCLUDE, _guilang.get("GUI_MENU_EDITOR_FINDINCLUDE", "script"));
    m_menuShowValue = m_popupMenu.Append(ID_DEBUG_DISPLAY_SELECTION, _guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "selection"), "", wxITEM_CHECK);
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
    SetScrollWidthTracking(true);
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
    if (m_analyzer)
    {
        delete m_analyzer;
        m_analyzer = nullptr;
    }

    if (m_search)
    {
        delete m_search;
        m_search = nullptr;
    }

    if (m_formatter)
    {
        delete m_formatter;
        m_formatter = nullptr;
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
        if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
            && GetFileNameAndPath().ToStdString().length())
            m_terminal->clearBreakpoints(GetFileNameAndPath().ToStdString());

        m_simpleFileName = fn.GetFullName();
    }

    bool createRevision = m_options->GetKeepBackupFile();

    VersionControlSystemManager manager(m_mainFrame);
    std::unique_ptr<FileRevisions> revisions;

    // The history does not need a real revision
    if (filename.find("numere.history") == string::npos)
        revisions.reset(manager.getRevisions(filename));

    if (revisions)
    {
        if (!revisions->getRevisionCount() && wxFileExists(filename) && createRevision)
        {
            wxFile tempfile(filename);
            wxString contents;
            tempfile.ReadAll(&contents);
            revisions->addRevision(contents);
        }
    }
    else if (wxFileExists(filename))
    {
        wxCopyFile(filename, filename + ".backup", true);
    }

    bool bWriteSuccess = false;

    // Write the file depending on its type
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || filename.find("numere.history") != string::npos)
        bWriteSuccess = SaveNumeReFile(filename);
    else
        bWriteSuccess = SaveGeneralFile(filename);

    // Check the contents of the newly created file
    wxFile filecheck;
    filecheck.Open(filename);

    if (!bWriteSuccess)
    {
        // if the contents are not matching, restore the backup and signalize that an error occured
        if (wxFileExists(filename + ".backup"))
            wxCopyFile(filename + ".backup", filename, true);
        else if (revisions && revisions->getRevisionCount())
            revisions->restoreRevision(revisions->getRevisionCount()-1, filename);

        return false;
    }
    else if ((m_fileType == FILE_NSCR || m_fileType == FILE_NPRC) && filecheck.Length() != GetText().length())// - countUmlauts(this->GetText().ToStdString()))
    {
        // if the contents are not matching, restore the backup and signalize that an error occured
        if (wxFileExists(filename + ".backup"))
            wxCopyFile(filename + ".backup", filename, true);
        else if (revisions && revisions->getRevisionCount())
            revisions->restoreRevision(revisions->getRevisionCount()-1, filename);

        return false;
    }
    else if ((m_fileType != FILE_NSCR && m_fileType != FILE_NPRC) && !filecheck.Length() && GetTextLength())
    {
        // if the contents are not matching, restore the backup and signalize that an error occured
        if (wxFileExists(filename + ".backup"))
            wxCopyFile(filename + ".backup", filename, true);
        else if (revisions && revisions->getRevisionCount())
            revisions->restoreRevision(revisions->getRevisionCount()-1, filename);

        return false;
    }

    // Add the current text to the revisions, if the saving process was
    // successful
    if (revisions && createRevision)
        revisions->addRevision(GetText());

    // If the user doesn't want to keep the backup files
    // delete it here
    if (!m_options->GetKeepBackupFile() && wxFileExists(filename + ".backup"))
        wxRemoveFile(filename + ".backup");

    // Only mark the editor as saved, if the saving process was successful
    markSaved();
    SetSavePoint();
    UpdateProcedureViewer();

    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        SynchronizeBreakpoints();

    m_filetime = fn.GetModificationTime();
    m_bSetUnsaved = false;
    return true;
}


/////////////////////////////////////////////////
/// \brief Saves a NumeRe-specific file and tries
/// to stick to ASCII encoding
///
/// \param filename const wxString&
/// \return bool
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Saves a general file without touching the encoding
///
/// \param filename const wxString&
/// \return bool
///
/////////////////////////////////////////////////
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
    if (fileContents.length() > 0)
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
    SetSavePoint();
    m_bSetUnsaved = false;

    // determine and set EOL mode
    int eolMode = -1;

    //wxString eolName;

    if (fileContents.find("\r\n") != std::string::npos)
    {
        eolMode = wxSTC_EOL_CRLF;
        //eolName = _("CR+LF (Windows)");
    }
    else if (fileContents.find("\r") != std::string::npos)
    {
		eolMode = wxSTC_EOL_CR;
		//eolName = _("CR (Macintosh)");
    }
    else if (fileContents.find("\n") != std::string::npos)
    {
		eolMode = wxSTC_EOL_LF;
		//eolName = _("LF (Unix)");
    }

    if (eolMode != -1)
    {
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


/////////////////////////////////////////////////
/// \brief Static helper function to determine,
/// whether a character is an actual brace
/// character.
///
/// \param chr wxChar
/// \return bool
///
/////////////////////////////////////////////////
static bool isBrace(wxChar chr)
{
    return chr == '(' || chr == '[' || chr == '{' || chr == ')' || chr == ']' || chr == '}';
}


/////////////////////////////////////////////////
/// \brief Static helper function to determine,
/// whether a brace is an opening brace.
///
/// \param chr wxChar
/// \return bool
///
/////////////////////////////////////////////////
static bool isOpeningBrace(wxChar chr)
{
    return chr == '(' || chr == '[' || chr == '{';
}


/////////////////////////////////////////////////
/// \brief Static helper function to get the
/// pairing brace character to the selected one.
///
/// \param chr wxChar
/// \return wxChar
///
/////////////////////////////////////////////////
static wxChar getBracePartner(wxChar chr)
{
    switch (chr)
    {
    case '(':
        return ')';
    case ')':
        return '(';
    case '{':
        return '}';
    case '}':
        return '{';
    case '[':
        return ']';
    case ']':
        return '[';
    }

    return 0;
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
    const wxChar chr = event.GetKey();
    int currentLine = GetCurrentLine();
    int currentPos = GetCurrentPos();
    int wordstartpos = WordStartPosition(currentPos, true);

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
                this->SetLineIndentation(i, lineIndent + indentWidth);
            else
                this->SetLineIndentation(i, lineIndent - indentWidth);
        }
    }

    if (chr == '\n')
    {
        m_nextChar = 0;
        markModified(currentLine);
        int previousLineInd = 0;

        if (currentLine > 0)
        {
            markModified(currentLine - 1);
            previousLineInd = GetLineIndentation(currentLine - 1);
        }

        if (previousLineInd == 0)
            return;

        SetLineIndentation(currentLine, previousLineInd);

        // If tabs are being used then change previousLineInd to tab sizes
        if (GetUseTabs())
            previousLineInd /= GetTabWidth();

        GotoPos(PositionFromLine(currentLine) + previousLineInd);
        return;
    }

    if ((isspace(chr) || (ispunct(chr) && chr != '_' && chr != '~')) && AutoCompActive())
        AutoCompCancel();

    if (m_options->getSetting(SETTING_B_QUOTEAUTOCOMP).active() && chr == '"')
    {
        if (m_currSelection.length())
            InsertText(currentPos-1, "\"" + m_currSelection);
        else if (m_nextChar == '"') // Jump over partners
            DeleteRange(currentPos, 1);
        else if (GetStyleAt(currentPos) != wxSTC_NSCR_STRING)
            InsertText(currentPos, "\"");
    }

    if (m_options->getSetting(SETTING_B_BRACEAUTOCOMP).active() && isBrace(chr))
    {
        if (m_currSelection.length())
        {
            if (isOpeningBrace(chr))
            {
                InsertText(currentPos, m_currSelection + getBracePartner(chr));
                GotoPos(currentPos-1);
            }
            else
                InsertText(currentPos-1, getBracePartner(chr) + m_currSelection);
        }
        else if (m_nextChar == chr && !isOpeningBrace(chr)) // Jump over closing partners
            DeleteRange(currentPos, 1);
        else if (isOpeningBrace(chr)
                 && (BraceMatch(currentPos-1) == wxSTC_INVALID_POSITION || GetCharAt(currentPos) == getBracePartner(chr)))
            InsertText(currentPos, wxString(getBracePartner(chr)));
    }

    m_currSelection.clear();
    m_nextChar = 0;

    int lenEntered = currentPos - wordstartpos;
    bool useSmartSense = m_options->getSetting(SETTING_B_SMARTSENSE).active() && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC);
    bool isMethod = false;

    if (useSmartSense)
    {
        if (chr == '.')
            wordstartpos = currentPos;

        if (GetCharAt(wordstartpos-1) == '.')
            isMethod = true;
    }

    AutoCompSetAutoHide(!useSmartSense);
    wxString sAutoCompList;

    if (lenEntered > 1 || isMethod)
    {
        if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        {
            if (GetStyleAt(wordstartpos) == wxSTC_NSCR_PROCEDURES)
            {
                wxString sNamespace;
                wxString sSelectedNamespace;
                int nNameSpacePosition = wordstartpos;

                while (GetStyleAt(nNameSpacePosition - 1) == wxSTC_NSCR_PROCEDURES && GetCharAt(nNameSpacePosition - 1) != '$')
                    nNameSpacePosition--;

                if (nNameSpacePosition == wordstartpos)
                    sNamespace = m_search->FindNameSpaceOfProcedure(wordstartpos) + "~";
                else
                    sSelectedNamespace = GetTextRange(nNameSpacePosition, wordstartpos);

                std::string filename = GetFileNameAndPath().ToStdString();
                filename = replacePathSeparator(filename);
                std::vector<std::string> vPaths = m_terminal->getPathSettings();

                if (filename.starts_with(vPaths[PROCPATH]))
                {
                    filename.erase(0, vPaths[PROCPATH].length());

                    if (filename.find('/') != string::npos)
                        filename.erase(filename.rfind('/'));

                    while (filename.front() == '/')
                        filename.erase(0, 1);

                    replaceAll(filename, "/", "~");
                }

                // If namespace == "this~" then replace it with the current namespace
                if (sNamespace.StartsWith("this~"))
                    sNamespace.replace(0, 4, filename);
                else if (sSelectedNamespace.StartsWith("this~"))
                    sSelectedNamespace.replace(0, 4, filename);
                /*{
                    std::string filename = GetFileNameAndPath().ToStdString();
                    filename = replacePathSeparator(filename);
                    std::vector<std::string> vPaths = m_terminal->getPathSettings();

                    if (filename.starts_with(vPaths[PROCPATH]))
                    {
                        filename.erase(0, vPaths[PROCPATH].length());

                        if (filename.find('/') != string::npos)
                            filename.erase(filename.rfind('/') + 1);

                        while (filename.front() == '/')
                            filename.erase(0, 1);

                        replaceAll(filename, "~~", "/../");
                        replaceAll(filename, "~", "/");

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

                    if (filename.starts_with(vPaths[PROCPATH]))
                    {
                        filename.erase(0, vPaths[PROCPATH].length());

                        if (filename.find('/') != string::npos)
                            filename.erase(filename.rfind('/') + 1);

                        while (filename.front() == '/')
                            filename.erase(0, 1);

                        replaceAll(filename, "~~", "/../");
                        replaceAll(filename, "~", "/");

                        sSelectedNamespace = filename;
                    }
                    else
                        sSelectedNamespace = "";
                }*/
                // If namespace == "thisfile~" then search for all procedures in the current file and use them as the
                // autocompletion list entries
                else if (sNamespace == "thisfile"
                         || sNamespace == "thisfile~"
                         || sSelectedNamespace == "thisfile"
                         || sSelectedNamespace == "thisfile~")
                {
                    AutoCompSetIgnoreCase(true);
                    AutoCompSetCaseInsensitiveBehaviour(wxSTC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
                    AutoCompShow(lenEntered, m_search->FindProceduresInCurrentFile(GetTextRange(wordstartpos, currentPos),
                                                                                   sSelectedNamespace));
                    Colourise(0, -1);
                    event.Skip();
                    return;
                }
                // If namespace == "main~" (or similiar) then clear it's contents
                else if (sNamespace == "main" || sNamespace == "main~" || sNamespace == "~")
                    sNamespace = "";
                else if (sSelectedNamespace == "main" || sSelectedNamespace == "main~" || sSelectedNamespace == "~")
                    sSelectedNamespace = "";

                sAutoCompList = _syntax->getProcAutoCompList(GetTextRange(wordstartpos, currentPos).ToStdString(),
                                                             sNamespace.ToStdString(), sSelectedNamespace.ToStdString());
            }
            else if (!isStyleType(STYLE_COMMENT, wordstartpos)
                     && !isStyleType(STYLE_STRING, wordstartpos)
                     && !isStyleType(STYLE_NUMBER, wordstartpos))
            {
                int smartSenseWordStart = wordstartpos;
                NumeReSyntax::SyntaxColors varType = NumeReSyntax::SYNTAX_METHODS;

                // SmartSense extension: match only methods
                if (isMethod)
                {
                    smartSenseWordStart--;

                    // Identify the type of the current method host. Is currently quite rudimentary
                    // and should be extended with the semi-static parser model
                    if (GetStyleAt(wordstartpos-2) == wxSTC_NSCR_IDENTIFIER)
                        varType = NumeReSyntax::SYNTAX_STD;
                    else if (GetStyleAt(wordstartpos-2) == wxSTC_NSCR_METHOD
                             || (GetTextRange(wordstartpos-3, wordstartpos-1) == "()"
                                 && (GetStyleAt(wordstartpos-4) == wxSTC_NSCR_CUSTOM_FUNCTION
                                     || GetStyleAt(wordstartpos-4) == wxSTC_NSCR_PREDEFS)))
                        varType = NumeReSyntax::SYNTAX_TABLE;
                }

                sAutoCompList = generateAutoCompList(smartSenseWordStart, currentPos,
                                                     _syntax->getAutoCompList(GetTextRange(smartSenseWordStart, currentPos).ToStdString(),
                                                                              useSmartSense, varType));

                if (sAutoCompList.length())
                    AutoCompShow(lenEntered, sAutoCompList);
            }
        }
        else if (m_fileType == FILE_MATLAB
                 && !isStyleType(STYLE_COMMENT, wordstartpos)
                 && !isStyleType(STYLE_STRING, wordstartpos))
        {
            sAutoCompList = generateAutoCompList(wordstartpos, currentPos,
                                                 _syntax->getAutoCompListMATLAB(GetTextRange(wordstartpos, currentPos).ToStdString()));
        }
        else if (m_fileType == FILE_CPP
                 && !isStyleType(STYLE_COMMENT, wordstartpos)
                 && !isStyleType(STYLE_STRING, wordstartpos))
        {
            sAutoCompList = generateAutoCompList(wordstartpos, currentPos,
                                                 _syntax->getAutoCompListCPP(GetTextRange(wordstartpos, currentPos).ToStdString()));
        }
        else if (m_fileType == FILE_TEXSOURCE
                 && GetStyleAt(wordstartpos) == wxSTC_TEX_COMMAND)
        {
            sAutoCompList = generateAutoCompList(wordstartpos, currentPos,
                                                 _syntax->getAutoCompListTeX(GetTextRange(wordstartpos, currentPos).ToStdString()));
        }
        else
            sAutoCompList = generateAutoCompList(wordstartpos, currentPos, "");
    }

    if (sAutoCompList.length())
        AutoCompShow(lenEntered, sAutoCompList);

    // if line indicator setting is active, provide autowrapping for comments
    bool isLineIndicatorActive = m_options->getSetting(SETTING_B_LINELENGTH).active();
    bool isNumereFile = m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_NLYT;
    bool isComment = isStyleType(STYLE_COMMENT, currentPos) && isStyleType(STYLE_COMMENT, GetLineIndentPosition(currentLine)+1);

    // The whole wrap shall be considered as a single action undoable with one
    // user interaction
    BeginUndoAction();

    while (isNumereFile
           && isLineIndicatorActive
           && isComment
           && (GetColumn(currentPos) > LINELENGTH_COLUMN))
    {
        // Only calculate this information if really necessary
        int lineBreakPos = WordStartPosition(FindColumn(currentLine, LINELENGTH_COLUMN), true);
        int charsToErase = 0;

        // Ensure that we do not break right after an opening brace or
        // the corresponding function name
        while (isOpeningBrace(GetCharAt(lineBreakPos-1)))
        {
            while (isOpeningBrace(GetCharAt(lineBreakPos-1)))
                lineBreakPos--;

            lineBreakPos = WordStartPosition(lineBreakPos, true);
        }

        // Ensure that trailing whitespaces right after the linebreak
        // position will be removed
        if (std::isblank(GetCharAt(lineBreakPos)))
            charsToErase = WordEndPosition(lineBreakPos, false) - lineBreakPos;

        // Wrap only, if the breaking point before last word on line is more than half of LINELENGTH_COLUMN after the indentation
        bool isWordSmall = (lineBreakPos - GetLineIndentPosition(currentLine)) > (LINELENGTH_COLUMN/2);

        if (!isWordSmall)
            break;

        // Get the start of the last line to use as a reference for indentation
        wxString sLineStart = GetTextRange(PositionFromLine(currentLine), GetLineIndentPosition(currentLine)+20);

        // Handle also enumerations. Those have to follow
        // the DIGIT.WHITESPACE pattern
        if (sLineStart.find_first_not_of("#*! \t") != std::string::npos
            && std::isdigit(sLineStart[sLineStart.find_first_not_of("#*! ")]))
        {
            size_t enumStart = sLineStart.find_first_not_of("#*! \t");
            size_t enumLength = 1;

            while (sLineStart.length() > enumStart+enumLength && std::isdigit(sLineStart[enumStart+enumLength]))
                enumLength++;

            if (sLineStart.length() > enumStart+enumLength+2
                && sLineStart.compare(enumStart+enumLength, 2, ". ") == 0
                && sLineStart.find_first_not_of(' ', enumStart+enumLength+1) != std::string::npos)
            {
                enumLength++;

                sLineStart.erase(sLineStart.find_first_not_of(' ', enumStart+enumLength));
                sLineStart.replace(enumStart, enumLength, enumLength, wxUniChar(' '));
            }
            else
                sLineStart.erase(enumStart);
        }
        else
            sLineStart.erase(sLineStart.find_first_not_of("#*! -\t"));

        sLineStart.Replace("-", " ");

        if (GetStyleAt(currentPos) == wxSTC_NSCR_COMMENT_LINE
            || GetStyleAt(currentPos) == wxSTC_NSCR_DOCCOMMENT_LINE)
        {
            if (charsToErase)
                DeleteRange(lineBreakPos, charsToErase);

            InsertText(lineBreakPos, "\r\n" + sLineStart);
            GotoPos(currentPos+2+sLineStart.length() - std::min(charsToErase, currentPos-lineBreakPos));
        }
        else if (isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, currentPos))
        {
            sLineStart.Replace("#*", " *");
            sLineStart.Replace("!", "");

            if (charsToErase)
                DeleteRange(lineBreakPos, charsToErase);

            InsertText(lineBreakPos, "\r\n"+sLineStart);
            GotoPos(currentPos+2+sLineStart.length() - std::min(charsToErase, currentPos-lineBreakPos));
        }
        else
            break;

        currentLine = GetCurrentLine();
        currentPos = GetCurrentPos();
    }

    EndUndoAction();

    Colourise(0, -1);

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Checks for corresponding braces.
///
/// \return void
///
/// This function checks for corresponding braces and
/// handles the indicators correspondingly.
/////////////////////////////////////////////////
void NumeReEditor::MakeBraceCheck()
{
    int curPos = GetCurrentPos();
    char CurrentChar = GetCharAt(curPos);
    char PrevChar = 0;

    // Find the previous character, if available
    if (curPos)
        PrevChar = GetCharAt(curPos - 1);

    // Find the matching brace
    if (CurrentChar == ')' || CurrentChar == ']' || CurrentChar == '}')
        getMatchingBrace(curPos);
    else if (PrevChar == '(' || PrevChar == '[' || PrevChar == '{')
        getMatchingBrace(curPos - 1);
    else if (CurrentChar == '(' || CurrentChar == '[' || CurrentChar == '{')
        getMatchingBrace(curPos);
    else if (PrevChar == ')' || PrevChar == ']' || PrevChar == '}')
        getMatchingBrace(curPos - 1);
    else if (m_braceIndicatorActive)
    {
        // Deactivate all indicators, if there's no brace is at cursor's
        // position
        this->SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
        this->IndicatorClearRange(0, GetLastPosition());
        this->BraceBadLight(wxSTC_INVALID_POSITION);
        this->BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);

        m_braceIndicatorActive = false;
    }

    applyStrikeThrough();
    return;
}


/////////////////////////////////////////////////
/// \brief Checks for corresponding flow control statements.
///
/// \return void
///
/// This function checks for corresponding flow
/// control statements and handles the indicators
/// correspondingly.
/////////////////////////////////////////////////
void NumeReEditor::MakeBlockCheck()
{
    if (this->m_fileType != FILE_NSCR && this->m_fileType != FILE_NPRC && this->m_fileType != FILE_MATLAB)
        return;

    // clear all indicators first
    if (m_blockIndicatorActive)
    {
        SetIndicatorCurrent(HIGHLIGHT_MATCHING_BLOCK);
        IndicatorClearRange(0, GetLastPosition());
        SetIndicatorCurrent(HIGHLIGHT_NOT_MATCHING_BLOCK);
        IndicatorClearRange(0, GetLastPosition());

        m_blockIndicatorActive = false;
    }

    // Ensure that we have a command below the cursor
    if (!isStyleType(STYLE_COMMAND, GetCurrentPos()) && !isStyleType(STYLE_COMMAND, GetCurrentPos()-1))
        return;

    // Get the word below the cursor
    wxString currentWord = this->GetTextRange(WordStartPosition(GetCurrentPos(), true), WordEndPosition(GetCurrentPos(), true));

    // If the word is a flow control statement
    // find the matching block
    if (isBlockStart(currentWord, true) != wxNOT_FOUND || isBlockEnd(currentWord) != wxNOT_FOUND)
        getMatchingBlock(GetCurrentPos());
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a string.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isString(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.front() == 's' && viewedArg.length() > 1 && isupper(viewedArg[1]);
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a character.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isChr(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.front() == 'c' && viewedArg.length() > 1 && isupper(viewedArg[1]);
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a float.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isFloat(StringView viewedArg, StringView defaultValue)
{
    if (viewedArg.length() == 1)
        return viewedArg == "x"
            || viewedArg == "y"
            || viewedArg == "z"
            || viewedArg == "t"
            || viewedArg == "p"
            || viewedArg == "q";
    else if (viewedArg.length() == 2 && isdigit(viewedArg.back()))
        return viewedArg.front() == 'x'
            || viewedArg.front() == 'y'
            || viewedArg.front() == 'z'
            || viewedArg.front() == 't'
            || viewedArg.front() == 'a';

    return viewedArg.front() == 'f' && viewedArg.length() > 1 && isupper(viewedArg[1]);
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is an integer.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isInt(StringView viewedArg, StringView defaultValue)
{
    return viewedArg == "n"
        || viewedArg == "m"
        || viewedArg == "l"
        || viewedArg == "k"
        || (viewedArg.front() == 'n' && viewedArg.length() > 1 && isupper(viewedArg[1]));
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a date-time value.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isDateTime(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.front() == 't' && viewedArg.length() > 1 && isupper(viewedArg[1]);
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a matrix.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isMat(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.front() == 'm' && viewedArg.length() > 1 && isupper(viewedArg[1]);
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a boolean value.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isBool(StringView viewedArg, StringView defaultValue)
{
    return defaultValue == "true"
        || defaultValue == "false"
        || ((viewedArg.starts_with("is") || viewedArg.starts_with("as"))
            && viewedArg.length() > 2
            && isupper(viewedArg[2]));
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a table.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isTable(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.ends_with("()");
}


/////////////////////////////////////////////////
/// \brief Static function to detect, whether an
/// argument is a cluster.
///
/// \param viewedArg StringView
/// \param defaultValue StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool isCluster(StringView viewedArg, StringView defaultValue)
{
    return viewedArg.ends_with("{}");
}


/////////////////////////////////////////////////
/// \brief This function handles the descriptive
/// function call tip.
///
/// \return void
///
/// The function searches for the corresponding procedure
/// definition or function documentation, formats it
/// correspondingly, searches for the current marked
/// argument and displays the calltip with the current
/// marked argument highlighted. This function is called
/// asynchronously.
/////////////////////////////////////////////////
void NumeReEditor::HandleFunctionCallTip()
{
    m_isFunctionContext = false;

    // do nothing if an autocompletion list is active
    if (AutoCompActive())
        return;

    // do nothing, if language is not supported
    if (getFileType() != FILE_NSCR && getFileType() != FILE_NPRC)
        return;

    int currPos = GetCurrentPos();

    // If the last released key is zero, it was a mouse click. Now examine
    // whether the cursor is now on a syntax element with dwelling calltip
    // and prefer this over the context calltip
    if (!m_nLastReleasedKey
        && (isStyleType(STYLE_FUNCTION, currPos)
            || isStyleType(STYLE_COMMAND, currPos)
            || isStyleType(STYLE_PROCEDURE, currPos)
            || GetStyleAt(currPos) == wxSTC_NSCR_METHOD
            || GetStyleAt(currPos) == wxSTC_NSCR_CONSTANTS
            || GetStyleAt(currPos) == wxSTC_NSCR_PREDEFS))
    {
        ShowDwellingCallTip(currPos);
        return;
    }

    bool detectArgumentTypes = m_options->getSetting(SETTING_B_CALLTIP_ARGS).active();
    int nStartingBrace = 0;
    int nArgStartPos = 0;
    size_t nDotPos = 0;
    string sFunctionContext = GetCurrentFunctionContext(nStartingBrace);
    static NumeRe::CallTipProvider _provider = *m_terminal->getProvider();
    NumeRe::CallTip _cTip;

    if (!sFunctionContext.length())
        return;

    if (sFunctionContext.front() == '$')
    {
        _cTip.sDefinition = m_search->FindProcedureDefinition().ToStdString();

        if (_cTip.sDefinition.find('\n') != string::npos)
            _cTip.sDefinition.erase(_cTip.sDefinition.find('\n'));

        if (!detectArgumentTypes)
        {
            if (_cTip.sDefinition.find(')') != string::npos)
                _cTip.sDefinition.erase(_cTip.sDefinition.rfind(')') + 1);
        }
        else
        {
            if (_cTip.sDefinition.find("::") != std::string::npos)
                _cTip.sDefinition.erase(_cTip.sDefinition.find("::"), _cTip.sDefinition.find("->")-_cTip.sDefinition.find("::"));
        }
    }
    else if (sFunctionContext.front() == '.')
    {
        _cTip = _provider.getMethod(sFunctionContext.substr(1));
        nDotPos = _cTip.sDefinition.find('.');

        if (!detectArgumentTypes)
        {
            if (_cTip.sDefinition.find(')', nDotPos) != string::npos)
                _cTip.sDefinition.erase(_cTip.sDefinition.find(')', nDotPos) + 1);
            else
                _cTip.sDefinition.erase(_cTip.sDefinition.find(' ', nDotPos));
        }
    }
    else
    {
        _cTip = _provider.getFunction(sFunctionContext);

        if (_cTip.sDefinition.find(')') != string::npos && !detectArgumentTypes)
            _cTip.sDefinition.erase(_cTip.sDefinition.find(')') + 1);
    }

    if (!_cTip.sDefinition.length())
        return;

    std::string sArgument = GetCurrentArgument(_cTip.sDefinition, nStartingBrace, nArgStartPos);
    StringView viewedArg(sArgument);
    viewedArg.strip();

    if (viewedArg.length() && detectArgumentTypes)
    {
        bool isRef = false;
        bool isVect = false;
        std::string sArgumentType;

        // Is the argument a reference?
        if (viewedArg.front() == '&')
        {
            viewedArg.trim_front(1);
            isRef = true;
        }
        else if (viewedArg.back() == '&')
        {
            viewedArg.trim_back(1);
            isRef = true;
        }

        viewedArg.strip();
        StringView defaultValue;

        // Remove possible default values
        if (viewedArg.find('=') != std::string::npos)
        {
            defaultValue = viewedArg.subview(viewedArg.find('=')+1);
            viewedArg = viewedArg.subview(0, viewedArg.find('='));

            defaultValue.strip();
            viewedArg.strip();
        }

        // Is the argument a vector?
        if (viewedArg.front() == '{' && viewedArg.back() == '}')
        {
            isVect = true;
            viewedArg.trim_front(1);
            viewedArg.trim_back(1);
            viewedArg.strip();
        }

        // Detect the argument type
        if (isString(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_STR", viewedArg.to_string());
        else if (isChr(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_CHR", viewedArg.to_string());
        else if (isMat(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_MAT", viewedArg.to_string());
        else if (isInt(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_INTEGER", viewedArg.to_string());
        else if (isFloat(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_FLOAT", viewedArg.to_string());
        else if (isDateTime(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_TIME", viewedArg.to_string());
        else if (isBool(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_BOOL", viewedArg.to_string());
        else if (isTable(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_TABLE", viewedArg.to_string());
        else if (isCluster(viewedArg, defaultValue))
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_CLUSTER", viewedArg.to_string());
        else if (viewedArg == "...")
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_REPEATTYPE");

        // Set it to "Any", if no heuristic succeeded
        if (!sArgumentType.length())
            sArgumentType = _guilang.get("GUI_EDITOR_ARGCALLTIP_ANY", viewedArg.to_string());

        // If we detected a reference, add this information here
        if (isRef)
            sArgumentType += " [" + _guilang.get("GUI_EDITOR_ARGCALLTIP_REFERENCE") + "]";

        // If we detected a vector, add this information here
        if (isVect)
            sArgumentType += " [" + _guilang.get("GUI_EDITOR_ARGCALLTIP_VECTOR") + "]";

        if (sArgumentType.length())
            _cTip.sDefinition += "\n" + strfill(" ", nArgStartPos + sArgument.find(viewedArg.to_string()), ' ') + sArgumentType;
    }

    // Adapt the starting position so that the opening braces align
    if (_cTip.sDefinition.find("(", nDotPos) != std::string::npos
        && _cTip.sDefinition.find("(", nDotPos) <= GetColumn(nStartingBrace))
        nStartingBrace -= _cTip.sDefinition.find("(", nDotPos);

    if (CallTipActive() && (CallTipStartPos() != nStartingBrace || m_sCallTipContent != _cTip.sDefinition))
    {
        AdvCallTipCancel();
        AdvCallTipShow(nStartingBrace, _cTip.sDefinition);
    }
    else if (!CallTipActive())
        AdvCallTipShow(nStartingBrace, _cTip.sDefinition);

    if (sArgument.length())
        CallTipSetHighlight(nArgStartPos, nArgStartPos + sArgument.length());

    m_isFunctionContext = true;
}


/////////////////////////////////////////////////
/// \brief Update the assigned procedure viewer.
///
/// \return void
///
/// This member function updates the procedure viewer,
/// if it was registered in this editor
/////////////////////////////////////////////////
void NumeReEditor::UpdateProcedureViewer()
{
    if (m_procedureViewer && m_fileType == FILE_NPRC)
    {
        m_procedureViewer->updateProcedureList(m_search->getProceduresInFile());
    }
}


/////////////////////////////////////////////////
/// \brief Find the function, whose braces the
/// cursor is currently located in.
///
/// \param nStartingBrace int&
/// \return string
///
/////////////////////////////////////////////////
string NumeReEditor::GetCurrentFunctionContext(int& nStartingBrace)
{
    int nCurrentLineStart = this->PositionFromLine(this->GetCurrentLine());
    int nCurrentPos = this->GetCurrentPos();

    // Find the outermost brace for the current function call
    for (int i = nCurrentPos; i > nCurrentLineStart; i--)
    {
        if (this->GetCharAt(i) == '('
                && (this->BraceMatch(i) >= nCurrentPos || this->BraceMatch(i) == -1) // either no brace (yet) or the brace further right
                && (this->GetStyleAt(i - 1) == wxSTC_NSCR_FUNCTION
                    || this->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES
                    || this->GetStyleAt(i - 1) == wxSTC_NSCR_METHOD))
        {
            nStartingBrace = i;

            // Get the word in front of the brace and return it
            if (this->GetStyleAt(i - 1) == wxSTC_NSCR_PROCEDURES)
                return m_search->FindMarkedProcedure(i - 1).ToStdString();
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


/////////////////////////////////////////////////
/// \brief Finds the current argument below the cursor.
///
/// \param sCallTip const string&
/// \param nStartingBrace int
/// \param nArgStartPos int&
/// \return string
///
/// This member function identifies the current
/// active argument of a function or a procedure,
/// on which the cursor is currently located.
/////////////////////////////////////////////////
string NumeReEditor::GetCurrentArgument(const string& sCallTip, int nStartingBrace, int& nArgStartPos)
{
    int nCurrentPos = this->GetCurrentPos();
    int nCurrentArg = 0;
    size_t nParensPos = 0;
    char currentChar;

    // Do nothing, if no parenthesis is found
    if (sCallTip.find('(') == string::npos)
        return "";

    nParensPos = sCallTip.find('(');

    // If this is a method call of a table,
    // advance the position to the method
    // parenthesis, if it is available
    if (sCallTip.find("().") != string::npos)
    {
        if (sCallTip.find('(', sCallTip.find("().") + 3) == string::npos)
            return "";

        nParensPos = sCallTip.find('(', sCallTip.find("().") + 3);
    }

    // Extract the argument list
    string sArgList = sCallTip.substr(nParensPos);
    sArgList.erase(getMatchingParenthesis(sArgList));

    // Find the n-th argument in the editor
    for (int i = nStartingBrace + 1; i < nCurrentPos && i < this->GetLineEndPosition(this->GetCurrentLine()); i++)
    {
        // Ignore comments and strings
        if (isStyleType(StyleType::STYLE_STRING, i)
            || isStyleType(StyleType::STYLE_COMMENT_LINE, i)
            || isStyleType(StyleType::STYLE_COMMENT_BLOCK, i))
            continue;

        currentChar = this->GetCharAt(i);

        // Increment the argument count, if a comma
        // has been found
        if (currentChar == ',')
            nCurrentArg++;

        // Jump over parentheses and braces
        if ((currentChar == '(' || currentChar == '[' || currentChar == '{')
                && this->BraceMatch(i) != -1)
            i = this->BraceMatch(i);
    }

    size_t nQuotationMarks = 0;

    // Find the corresponding argument in the argument list
    for (size_t i = 1; i < sArgList.length(); i++)
    {
        // If this is the current argument and we're after
        // an opening parenthesis or a comma
        if (!(nQuotationMarks % 2) && !nCurrentArg && (sArgList[i - 1] == '(' || sArgList[i - 1] == ','))
        {
            nArgStartPos = i + nParensPos;
            string sArgument = sArgList.substr(i);

            // If the argument still contains commas,
            // extract them here. Probably it is necessary
            // to move the position to the right
            if (sArgument.find(',') != string::npos)
            {
                sArgument = getNextArgument(sArgument, false);
                nArgStartPos = nParensPos + sArgList.find(sArgument, i);
            }

            // return the extracted argument
            return sArgument;
        }

        // Count and consider quotation marks
        if (sArgList[i] == '"' && sArgList[i-1] != '\\')
            nQuotationMarks++;

        // If a parenthesis or a brace was found,
        // jump over it
        if (!(nQuotationMarks % 2) && (sArgList[i] == '(' || sArgList[i] == '{'))
            i += getMatchingParenthesis(sArgList.substr(i));

        // If a comma was found,
        // decrement the argument count
        if (!(nQuotationMarks % 2) && sArgList[i] == ',')
            nCurrentArg--;
    }

    // check, if last argument was an ellipsis. We then keep showing this
    // argument, if the user is already beyond the last argument
    if (sArgList.ends_with("..."))
    {
        nArgStartPos = nParensPos + sArgList.length()-3;
        return "...";
    }

    // Return nothing
    return "";
}


/////////////////////////////////////////////////
/// \brief Show the dwelling calltip, if the
/// cursor is on an element providing such an
/// information.
///
/// \param charpos int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::ShowDwellingCallTip(int charpos)
{
    int startPosition = WordStartPosition(charpos, true);
    int endPosition = WordEndPosition(charpos, true);
    static NumeRe::CallTipProvider _provider = *m_terminal->getProvider();

    wxString selection = this->GetTextRange(startPosition, endPosition);
    NumeRe::CallTip _cTip;

    if (GetStyleAt(charpos) == wxSTC_NSCR_FUNCTION)
        _cTip = _provider.getFunction(selection.ToStdString());
    else if (GetStyleAt(charpos) == wxSTC_NSCR_COMMAND
             || GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURE_COMMANDS)
    {
        // Is it a block?
        int id = getBlockID(selection);

        if (id != wxNOT_FOUND)
        {
            if (CallTipActive() && m_nCallTipStart == startPosition)
                return;

            AdvCallTipCancel();

            size_t lastpos = 0;
            size_t nLength = 0;
            size_t lastpos2 = 0;

            // Get the block definition
            SyntaxBlockDefinition blockDef = vBlockDefs[id];

            // Construct the tooltip
            std::string sBlock = addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_"
                                                                              + toUpperCase(blockDef.startWord)
                                                                              + "_*"), lastpos)) + "\n  [...]\n";

            if (selection != blockDef.startWord)
                nLength = sBlock.length() + countUmlauts(sBlock);

            // Include middle words
            if (blockDef.middleWord1.length())
            {
                sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_"
                                                                       + toUpperCase(blockDef.middleWord1)
                                                                       + "_*"), lastpos2)) + "\n  [...]\n";

                if (selection != blockDef.startWord && selection != blockDef.middleWord1)
                    nLength = sBlock.length() + countUmlauts(sBlock);
            }

            // Include middle words
            if (blockDef.middleWord2.length())
            {
                sBlock += addLinebreaks(_guilang.get("PARSERFUNCS_LISTCMD_CMD_"
                                                     + toUpperCase(blockDef.middleWord2) + "_*"))
                                                     + "\n  [...]\n";

                if (selection != blockDef.startWord && selection != blockDef.middleWord1 && selection != blockDef.middleWord2)
                    nLength = sBlock.length() + countUmlauts(sBlock);
            }

            // Add the last word
            sBlock += addLinebreaks(realignLangString(_guilang.get("PARSERFUNCS_LISTCMD_CMD_"
                                                                   + toUpperCase(blockDef.endWord)
                                                                   + "_*"), lastpos));

            // Display the tooltip and highlight the corresponding positions
            AdvCallTipShow(startPosition, sBlock);
            CallTipSetHighlight(nLength, selection.length() + nLength);
            return;
        }
        else
            _cTip = _provider.getCommand(selection.ToStdString());
    }
    else if (GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES)
    {
        if (GetCharAt(charpos) != '$')
            startPosition--;

        // If we are already showing this tooltip or the function context, do nothing
        if (CallTipActive() && m_nCallTipStart == startPosition)
            return;

        AdvCallTipCancel();

        wxString proc = m_search->FindMarkedProcedure(charpos);

        if (!proc.length())
            return;

        wxString procdef = m_search->FindProcedureDefinition();
        wxString flags = "";

        if (!procdef.length())
            procdef = m_clickedProcedure + "(...)";

        size_t nSepPos = std::string::npos;

        if (procdef.find("::") != std::string::npos)
            nSepPos = procdef.find("::");
        else if (procdef.find(" -> ") != std::string::npos)
            nSepPos = procdef.find(" -> ");
        else if (procdef.find('\n') != std::string::npos)
            nSepPos = procdef.find('\n');

        if (nSepPos != std::string::npos)
        {
            flags = procdef.substr(nSepPos);
            procdef.erase(nSepPos);
        }

        if (flags.find('\n') != string::npos)
            AdvCallTipShow(startPosition, procdef + flags);
        else
            AdvCallTipShow(startPosition, procdef + flags + "\n    " + _guilang.get("GUI_EDITOR_CALLTIP_PROC2"));

        CallTipSetHighlight(0, procdef.length());
        return;
    }
    else if (GetStyleAt(charpos) == wxSTC_NSCR_OPTION)
        _cTip = _provider.getOption(selection.ToStdString());
    else if (GetStyleAt(charpos) == wxSTC_NSCR_METHOD)
        _cTip = _provider.getMethod(selection.ToStdString());
    else if (GetStyleAt(charpos) == wxSTC_NSCR_PREDEFS)
        _cTip = _provider.getPredef(selection.ToStdString());
    else if (GetStyleAt(charpos) == wxSTC_NSCR_CONSTANTS)
        _cTip = _provider.getConstant(selection.ToStdString());

    if (_cTip.sDefinition.length())
    {
        if (CallTipActive() && m_nCallTipStart == startPosition)
            return;
        else
            AdvCallTipCancel();

        AdvCallTipShow(startPosition, _cTip.sDefinition + (_cTip.sDocumentation.length() ? "\n" + _cTip.sDocumentation : ""));
        CallTipSetHighlight(_cTip.nStart, _cTip.nEnd);
    }
}



/////////////////////////////////////////////////
/// \brief Returns the starting position of the
/// currently displayed calltip.
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::CallTipStartPos()
{
    return m_nCallTipStart;
}


/////////////////////////////////////////////////
/// \brief A more advanced calltip display routine.
///
/// \param pos int
/// \param definition const wxString&
/// \return void
///
/// Only updates the calltip, if the contents are
/// different.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Simply closes the calltip and resets
/// its associated variables.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::AdvCallTipCancel()
{
    m_nCallTipStart = 0;
    m_sCallTipContent.clear();
    m_isFunctionContext = false;
    CallTipCancel();
}


/////////////////////////////////////////////////
/// \brief Checks key input events, before they
/// are typed into the editor.
///
/// \param event wxKeyEvent&
/// \return void
///
/// This member function checks the key input events
/// before(!) they are typed into the editor. We catch
/// here parentheses and quotation marks in the case
/// that we have
/// \li either a selection (we then surround the selection)
/// \li or a already matching partner of the parenthesis (we
/// then jump over the matching partner)
/////////////////////////////////////////////////
void NumeReEditor::OnKeyDn(wxKeyEvent& event)
{
    m_currSelection.clear();

    // Check the parentheses in the case of selections
    // and matching partners
    if (event.GetKeyCode() != WXK_SHIFT
        && event.GetKeyCode() != WXK_CAPITAL
        && event.GetKeyCode() != WXK_END
        && event.GetKeyCode() != WXK_HOME
        && event.GetKeyCode() != WXK_LEFT
        && event.GetKeyCode() != WXK_RIGHT
        && event.GetKeyCode() != WXK_UP
        && event.GetKeyCode() != WXK_DOWN)
    {
        if (HasSelection())
            m_currSelection = GetSelectedText();
        else
        {
            m_nextChar = GetCharAt(GetCurrentPos());

            if (!(isStyleType(STYLE_STRING, GetCurrentPos()) && m_nextChar == '"') && !isStyleType(STYLE_OPERATOR, GetCurrentPos()))
                m_nextChar = 0;
        }
    }

    // If the autocompletion dialog is open, we want the HOME and END
    // keys to automatically close it. Otherwise they are used
    // to navigate in the autocompletion list
    if (AutoCompActive()
        && m_options->isEnabled(SETTING_B_HOMEENDCANCELS)
        && (event.GetKeyCode() == WXK_END || event.GetKeyCode() == WXK_HOME))
        AutoCompCancel();

    // Pass the control to the internal OnKeyDown event
    // handler, which will insert the correct character
    OnKeyDown(event);

    // Apply the brace match indicator, if we have only
    // one or no selection
    if (this->GetSelections() <= 1)
        MakeBraceCheck();

    // Apply the block match indicator
    MakeBlockCheck();

    // Clear the double click occurence highlighter,
    // if the user doesn't press Ctrl or Shift.
    if (!event.ControlDown() && !event.ShiftDown())
        ClearDblClkIndicator();
}


/////////////////////////////////////////////////
/// \brief Called, when the user releases a key.
///
/// \param event wxKeyEvent&
/// \return void
///
/// Performs highlighting and asynchronous actions,
/// which are time-consuming tasks.
/////////////////////////////////////////////////
void NumeReEditor::OnKeyRel(wxKeyEvent& event)
{
    if (this->GetSelections() <= 1)
        MakeBraceCheck();

    m_nLastReleasedKey = (event.AltDown() || event.ControlDown()) ? WXK_CONTROL : event.GetKeyCode();

    MakeBlockCheck();
    event.Skip();
    CallAfter(NumeReEditor::AsynchActions);
}


/////////////////////////////////////////////////
/// \brief Called, when the user releases the mouse key.
///
/// \param event wxMouseEvent&
/// \return void
///
/// Performs highlighting and asynchronous actions,
/// which are time-consuming tasks.
/////////////////////////////////////////////////
void NumeReEditor::OnMouseUp(wxMouseEvent& event)
{
    m_nLastReleasedKey = 0;
    MakeBraceCheck();
    MakeBlockCheck();
    CallAfter(NumeReEditor::AsynchActions);
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the user presses the left
/// mouse key.
///
/// \param event wxMouseEvent&
/// \return void
///
/// Performs highlighting and removes the double-
/// click indicator.
/////////////////////////////////////////////////
void NumeReEditor::OnMouseDn(wxMouseEvent& event)
{
    if (!event.ControlDown())
        ClearDblClkIndicator();

    MakeBraceCheck();
    MakeBlockCheck();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the user double clicks.
///
/// \param event wxMouseEvent&
/// \return void
///
/// Searches automatically for all occurences of
/// a string in the document and highlightes them
/////////////////////////////////////////////////
void NumeReEditor::OnMouseDblClk(wxMouseEvent& event)
{
    int charpos = PositionFromPoint(event.GetPosition());
    int startPosition = WordStartPosition(charpos, true);
    int endPosition = WordEndPosition(charpos, true);
    wxString selection = this->GetTextRange(startPosition, endPosition);

    // Ensure the user has selected a word
    if (!selection.length())
    {
        event.Skip();
        return;
    }

    // Handle the control key and add the current selection
    // to the already existing one
    if (event.ControlDown() && this->HasSelection())
        this->AddSelection(endPosition, startPosition);
    else
        this->SetSelection(startPosition, endPosition);

    m_dblclkString = selection;
    long int maxpos = GetLastPosition();

    // Update the Indicators
    SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
    IndicatorClearRange(0, maxpos);

    int nPos = 0;
    int nCurr = 0;
    int nLength = endPosition - startPosition;
    vector<int> vSelectionList;

    // Find all occurences and store them in a vector
    while ((nPos = FindText(nCurr, maxpos, selection, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != wxNOT_FOUND)
    {
        vSelectionList.push_back(nPos);
        nCurr = nPos + nLength;
    }

    // Highlight the occurences
    for (size_t i = 0; i < vSelectionList.size(); i++)
        this->IndicatorFillRange(vSelectionList[i], nLength);

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the mouse leaves the editor
/// screen, but the user keeps the mouse pressed.
///
/// \param event wxMouseCaptureLostEvent&
/// \return void
///
/// The capturing of the mouse is ended and the
/// editor window is refreshed
/////////////////////////////////////////////////
void NumeReEditor::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
    if (GetCapture() == this)
    {
        ReleaseMouse();
        Refresh();
    }
}


/////////////////////////////////////////////////
/// \brief Called, when the mouse enters the editor window.
///
/// \param event wxMouseEvent&
/// \return void
///
/// Focuses the editor, so that the user can start
/// typing directly.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Called, when the mouse leaves the editor window.
///
/// \param event wxMouseEvent&
/// \return void
///
/// Cancels all open call tips, if there are any.
/////////////////////////////////////////////////
void NumeReEditor::OnLeave(wxMouseEvent& event)
{
   // if (this->CallTipActive())
   //     this->AdvCallTipCancel();

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the editor loses focus.
///
/// \param event wxFocusEvent&
/// \return void
///
/// Cancels all open call tips, if there are any.
/////////////////////////////////////////////////
void NumeReEditor::OnLoseFocus(wxFocusEvent& event)
{
    if (CallTipActive())
        AdvCallTipCancel();

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the mouse dwells for some time.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/// This function displays the correct call tip
/// depending upon the syntax element below the
/// mouse pointer.
///
/////////////////////////////////////////////////
void NumeReEditor::OnMouseDwell(wxStyledTextEvent& event)
{
    if ((m_fileType != FILE_NSCR && m_fileType != FILE_NPRC)
        || m_PopUpActive
        || m_isFunctionContext
        || AutoCompActive()
        || !HasFocus())
        return;

    ShowDwellingCallTip(event.GetPosition());
}


/////////////////////////////////////////////////
/// \brief Called, when the editor idles, i.e. the
/// user is not using it.
///
/// \param event wxIdleEvent&
/// \return void
///
/// Fires the analyzer timer, which will run for
/// 750 msec and then fire another event. If the
/// timer is started again in between, it is
/// reset without firing the timer event.
/////////////////////////////////////////////////
void NumeReEditor::OnIdle(wxIdleEvent& event)
{
    if (!m_modificationHappened)
        return;

    m_modificationHappened = false;
    m_analyzerTimer->StartOnce(750);
}


/////////////////////////////////////////////////
/// \brief This event handler fires, once the
/// analyzer timer is finished and calls the time
/// consuming analysis tasks.
///
/// \param event wxTimerEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnAnalyzerTimer(wxTimerEvent& event)
{
    AsynchEvaluations();
}


/////////////////////////////////////////////////
/// \brief Called, when the editor reaches the
/// latest save point.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/// The save point is reached by using undo and redo
/// actions. The editor will mark all remaining modifications
/// as saved.
/////////////////////////////////////////////////
void NumeReEditor::OnSavePointReached(wxStyledTextEvent& event)
{
    markSaved();
    m_bSetUnsaved = false;
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Called, when the editor leaves the
/// latest save point.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/// The save point is left by undo and redo actions
/// and actual modifications. The editor will be
/// marked as containing unsaved parts.
/////////////////////////////////////////////////
void NumeReEditor::OnSavePointLeft(wxStyledTextEvent& event)
{
    SetUnsaved();
}


/////////////////////////////////////////////////
/// \brief This event handler fires, if the user
/// selects an entry in the autocompletion list
/// or if he presses e.g. TAB.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnAutoCompletion(wxStyledTextEvent& event)
{
    // Is the autocompleted text a function or
    // a control flow statement?
    if (event.GetText()[event.GetText().length()-1] == '(')
    {
        // Ensure that there's actually a need for a new
        // parenthesis and insert a closed pair
        if (m_options->getSetting(SETTING_B_BRACEAUTOCOMP).active()
            && (GetCharAt(GetCurrentPos()) != '(' || BraceMatch(GetCurrentPos()) == wxSTC_INVALID_POSITION))
            InsertText(GetCurrentPos(), "()");

        // Replace the current text with the function's
        // name without the parenthesis
        Replace(event.GetPosition(), GetCurrentPos(), event.GetText().substr(0, event.GetText().length()-1));

        // Jump into the parenthesis
        GotoPos(event.GetPosition() + event.GetText().length() - !m_options->getSetting(SETTING_B_BRACEAUTOCOMP).active());
        AutoCompCancel();
    }
    else if (event.GetText()[event.GetText().length()-1] == '{')
    {
        // Ensure that there's actually a need for a new
        // parenthesis and insert a closed pair
        if (m_options->getSetting(SETTING_B_BRACEAUTOCOMP).active()
            && (GetCharAt(GetCurrentPos()) != '{' || BraceMatch(GetCurrentPos()) == wxSTC_INVALID_POSITION))
            InsertText(GetCurrentPos(), "{}");

        // Replace the current text with the function's
        // name without the parenthesis
        Replace(event.GetPosition(), GetCurrentPos(), event.GetText().substr(0, event.GetText().length()-1));

        // Jump into the parenthesis
        GotoPos(event.GetPosition() + event.GetText().length() - !m_options->getSetting(SETTING_B_BRACEAUTOCOMP).active());
        AutoCompCancel();
    }
    else if (m_options->getSetting(SETTING_B_BLOCKAUTOCOMP).active() && isBlockStart(event.GetText(), true) != wxNOT_FOUND)
    {
        // Get the autocompletion block and find
        // the pipe position
        wxString sAutoComp = getBlockAutoCompletion(event.GetText());
        size_t pos = sAutoComp.find('|');

        // Remove the pipe position
        sAutoComp.erase(pos, 1);

        // Replace the current text with the
        // complete autocompletion text
        Replace(event.GetPosition(), GetCurrentPos(), sAutoComp);

        // Copy the line indentation for the second statement,
        // if needed
        if (sAutoComp.find("\r\n") != std::string::npos)
            SetLineIndentation(LineFromPosition(event.GetPosition())+1, GetLineIndentation(LineFromPosition(event.GetPosition())));

        // Goto the pipe position
        GotoPos(event.GetPosition() + pos);
        AutoCompCancel();
    }
}


/////////////////////////////////////////////////
/// \brief Toggles a line comment.
///
/// \return void
///
/// This function comments lines, which are not
/// commented and uncomments lines, which are
/// commented.
///
/////////////////////////////////////////////////
void NumeReEditor::ToggleCommentLine()
{
    if (m_fileType == FILE_NONSOURCE)
        return;

    int nFirstLine = 0;
    int nLastLine = 0;
    int nSelectionStart = -1;
    int nSelectionEnd = 0;
    int nCommentStyle = 0;
    int nCommentCharSequenceLen = 0;
    wxString sLineCommentCharSequence;

    // Determine the parameters for line comments
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        sLineCommentCharSequence = "## ";
        nCommentStyle = wxSTC_NSCR_COMMENT_LINE;
    }
    else if (m_fileType == FILE_TEXSOURCE)
    {
        sLineCommentCharSequence = "% ";
        nCommentStyle = wxSTC_TEX_DEFAULT;
    }
    else if (m_fileType == FILE_DATAFILES)
    {
        sLineCommentCharSequence = "# ";
        nCommentStyle = wxSTC_MATLAB_COMMENT;
    }
    else if (m_fileType == FILE_MATLAB)
    {
        sLineCommentCharSequence = "% ";
        nCommentStyle = wxSTC_MATLAB_COMMENT;
    }
    else if (m_fileType == FILE_CPP)
    {
        sLineCommentCharSequence = "// ";
        nCommentStyle = wxSTC_C_COMMENTLINE;
    }

    nCommentCharSequenceLen = sLineCommentCharSequence.length();

    // Get start and end of the commenting
    if (HasSelection())
    {
        nSelectionStart = GetSelectionStart();
        nSelectionEnd = GetSelectionEnd();
        nFirstLine = LineFromPosition(nSelectionStart);
        nLastLine = LineFromPosition(nSelectionEnd);
    }
    else
    {
        nFirstLine = GetCurrentLine();
        nLastLine = nFirstLine;
    }

    BeginUndoAction();

    // Go through the code section and toggle every comment line
    for (int i = nFirstLine; i <= nLastLine; i++)
    {
        int position = PositionFromLine(i);

        while (GetCharAt(position) == ' ' || GetCharAt(position) == '\t')
            position++;

        int style = GetStyleAt(position);

        // TeX file syntax highlighting requires a special handling
        if ((m_fileType == FILE_TEXSOURCE && GetStyleAt(position + 1) == wxSTC_TEX_DEFAULT && GetCharAt(position) == '%')
            || (m_fileType != FILE_TEXSOURCE && style == nCommentStyle))
        {
            // Remove the line comment characters
            if (GetCharAt(position + nCommentCharSequenceLen-1) == ' ')
            {
                // With the separating whitespace
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + nCommentCharSequenceLen)
                    nSelectionStart -= nCommentCharSequenceLen;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;

                DeleteRange(position, nCommentCharSequenceLen);
                nSelectionEnd -= nCommentCharSequenceLen;
            }
            else
            {
                // Without the separating whitespace
                if (i == nFirstLine && nSelectionStart >= 0 && nSelectionStart >= position + nCommentCharSequenceLen-1)
                    nSelectionStart -= nCommentCharSequenceLen-1;
                else if (i == nFirstLine && nSelectionStart >= 0)
                    nSelectionStart = position;

                DeleteRange(position, nCommentCharSequenceLen-1);
                nSelectionEnd -= nCommentCharSequenceLen-1;
            }
        }
        else if ((m_fileType == FILE_TEXSOURCE && (GetStyleAt(position + 1) != wxSTC_TEX_DEFAULT || GetCharAt(position) != '%'))
                 || (m_fileType != FILE_TEXSOURCE && style != nCommentStyle))
        {
            // Add line comment characters
            InsertText(PositionFromLine(i), sLineCommentCharSequence);

            if (nSelectionStart >= 0)
            {
                nSelectionStart += nCommentCharSequenceLen;
                nSelectionEnd += nCommentCharSequenceLen;
            }
        }
    }

    if (nSelectionStart >= 0)
        SetSelection(nSelectionStart, nSelectionEnd);

    EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Toggles block comments in a selection.
///
/// \return void
///
/// This function comments selections, which are
/// not commented and uncomments selections, which
/// are commented. This function uses the block
/// comment style for this feature.
///
/////////////////////////////////////////////////
void NumeReEditor::ToggleCommentSelection()
{
    if (m_fileType == FILE_NONSOURCE)
        return;

    // Only works, if the user selected some code
    if (!HasSelection())
        return;

    int nFirstPosition = GetSelectionStart();
    int nLastPosition = GetSelectionEnd();
    int nSelectionStart = nFirstPosition;
    int nSelectionEnd = nLastPosition;
    int style = GetStyleAt(nFirstPosition);
    int nCommentStyle = wxSTC_NSCR_COMMENT_BLOCK;
    wxString sBlockCommentStart = "#* ";
    wxString sBlockCommentEnd = " *#";

    // Some code segments do not have block comments
    if (m_fileType != FILE_NSCR
        && m_fileType != FILE_NPRC
        && m_fileType != FILE_XML
        && m_fileType != FILE_CPP)
    {
        ToggleCommentLine();
        return;
    }

    // Change characters and style for C++ or XML files
    if (m_fileType == FILE_CPP)
    {
        sBlockCommentEnd = " */";
        sBlockCommentStart = "/* ";
        nCommentStyle = wxSTC_C_COMMENT;
    }
    else if (m_fileType == FILE_XML)
    {
        sBlockCommentEnd = " -->";
        sBlockCommentStart = "<!-- ";
        nCommentStyle = wxSTC_H_COMMENT;
    }

    BeginUndoAction();

    if (style == nCommentStyle)
    {
        // Position before
        while (nFirstPosition && GetStyleAt(nFirstPosition - 1) == nCommentStyle)
            nFirstPosition--;

        if (GetStyleAt(nLastPosition) != nCommentStyle)
            nLastPosition = nFirstPosition;

        // Position after
        while (nLastPosition < GetLastPosition() && GetStyleAt(nLastPosition) == nCommentStyle)
            nLastPosition++;

        // Remove the trailing block comment sequence
        if (GetTextRange(nLastPosition - sBlockCommentEnd.length(), nLastPosition) == sBlockCommentEnd)
        {
            if (nSelectionEnd > nLastPosition - (int)sBlockCommentEnd.length())
                nSelectionEnd -= sBlockCommentEnd.length();

            DeleteRange(nLastPosition - sBlockCommentEnd.length(), sBlockCommentEnd.length());
        }
        else
        {
            if (nSelectionEnd > nLastPosition - (int)sBlockCommentEnd.length() + 1)
                nSelectionEnd -= sBlockCommentEnd.length() - 1;

            DeleteRange(nFirstPosition - sBlockCommentEnd.length() + 1, sBlockCommentEnd.length() - 1);
        }

        // Remove the leading block comment sequence
        if (GetTextRange(nFirstPosition, nFirstPosition + sBlockCommentStart.length()) == sBlockCommentStart)
        {
            if (nFirstPosition != nSelectionStart)
                nSelectionStart -= sBlockCommentStart.length();

            DeleteRange(nFirstPosition, sBlockCommentStart.length());
            nSelectionEnd -= sBlockCommentStart.length();
        }
        else
        {
            if (nFirstPosition != nSelectionStart)
                nSelectionStart -= sBlockCommentStart.length() - 1;

            DeleteRange(nFirstPosition, sBlockCommentStart.length() - 1);
            nSelectionEnd -= sBlockCommentStart.length() - 1;
        }
    }
    else
    {
        // Embed the selection into a block comment
        InsertText(nFirstPosition, sBlockCommentStart);
        InsertText(nLastPosition + sBlockCommentStart.length(), sBlockCommentEnd);
        nSelectionEnd += sBlockCommentEnd.length();
        nSelectionStart += sBlockCommentStart.length();
    }

    EndUndoAction();
    SetSelection(nSelectionStart, nSelectionEnd);
}


/////////////////////////////////////////////////
/// \brief Folds the code completely.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::FoldAll()
{
    for (int i = GetLineCount() - 1; i >= 0; i--)
    {
        if (GetFoldLevel(i) & wxSTC_FOLDLEVELHEADERFLAG && GetFoldExpanded(i))
            ToggleFold(i);
    }
}


/////////////////////////////////////////////////
/// \brief Unfolds every folded section.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::UnfoldAll()
{
    for (int i = 0; i < GetLineCount(); i++)
    {
        if (GetFoldLevel(i) & wxSTC_FOLDLEVELHEADERFLAG && !GetFoldExpanded(i))
            ToggleFold(i);
    }
}


/////////////////////////////////////////////////
/// \brief Jumps the cursor to the next bookmark.
///
/// \param down bool
/// \return void
///
/// This function uses bookmark markers and section
/// markers as targets for its jumps.
/////////////////////////////////////////////////
void NumeReEditor::JumpToBookmark(bool down)
{
    int nCurrentLine = GetCurrentLine();

    // Leave the line, if it contains already one of both markers
    if (MarkerOnLine(nCurrentLine, MARKER_BOOKMARK) || MarkerOnLine(nCurrentLine, MARKER_SECTION))
    {
        if (down)
            nCurrentLine++;
        else
            nCurrentLine--;
    }

    int nMarker;
    int nMarkerMask = (1 << MARKER_BOOKMARK) | (1 << MARKER_SECTION);

    // Find the next marker
    if (down)
        nMarker = MarkerNext(nCurrentLine, nMarkerMask);
    else
        nMarker = MarkerPrevious(nCurrentLine, nMarkerMask);

    // Wrap around the search, if nothing was found
    if (nMarker == -1)
    {
        if (down)
            nMarker = MarkerNext(0, nMarkerMask);
        else
            nMarker = MarkerPrevious(LineFromPosition(GetLastPosition()), nMarkerMask);
    }

    // Go to the marker, if a marker was found
    if (nMarker != -1)
        GotoLine(nMarker);
}


/////////////////////////////////////////////////
/// \brief Returns the line positions of the bookmarks.
///
/// \return vector<int>
///
/// This member function returns a vector containing the list
/// of available bookmarks in the current file.
/////////////////////////////////////////////////
vector<int> NumeReEditor::getBookmarks()
{
    vector<int> vBookmarks;

    // Find all bookmark markers in the document
    for (int i = 0; i < GetLineCount(); i++)
    {
        if (MarkerOnLine(i, MARKER_BOOKMARK))
            vBookmarks.push_back(i);
    }

    return vBookmarks;
}


/////////////////////////////////////////////////
/// \brief Set the bookmarks in the editor.
///
/// \param vBookmarks const vector<int>&
/// \return void
///
/// This member function overrides all bookmarks in the current
/// file with the passed list of bookmarks.
/////////////////////////////////////////////////
void NumeReEditor::setBookmarks(const vector<int>& vBookmarks)
{
    // Remove all available bookmark markers
    MarkerDeleteAll(MARKER_BOOKMARK);

    // Set the list of bookmarks
    for (size_t i = 0; i < vBookmarks.size(); i++)
    {
        MarkerAdd(vBookmarks[i], MARKER_BOOKMARK);
    }
}


/////////////////////////////////////////////////
/// \brief Removes whitespaces in the document.
///
/// \param nType int Type of removal
/// \return void
///
/// The type of removal is selected using nType.
/// RM_WS_FRONT removes leading, RM_WS_BACK removes
/// trailing and RM_WS_BOTH removes leading and
/// trailing whitespaces from each line.
/////////////////////////////////////////////////
void NumeReEditor::removeWhiteSpaces(int nType)
{
    int nFirstline = 0;
    int nLastLine = GetLineCount() - 1;

    // If the user selected something, use the
    // selection as begin and end lines
    if (HasSelection())
    {
        nFirstline = LineFromPosition(GetSelectionStart());
        nLastLine = LineFromPosition(GetSelectionEnd());
    }

    BeginUndoAction();

    // Go through each line and remove the white
    // spaces depending on the selected type
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

    EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Toggles a bookmark marker on the current line.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::toggleBookmark()
{
    int nLine = GetCurrentLine();

    if (MarkerOnLine(nLine, MARKER_BOOKMARK))
        MarkerDelete(nLine, MARKER_BOOKMARK);
    else
        MarkerAdd(nLine, MARKER_BOOKMARK);
}


/////////////////////////////////////////////////
/// \brief Removes all bookmark markers from the
/// current document.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::clearBookmarks()
{
    this->MarkerDeleteAll(MARKER_BOOKMARK);
}


/////////////////////////////////////////////////
/// \brief Sorts the selected lines alphabetically.
///
/// \param ascending bool
/// \return void
///
/// This function sorts the user selected lines
/// alphabetically in either ascending or descending
/// order. The sorting algorithm is case-insensitive.
/////////////////////////////////////////////////
void NumeReEditor::sortSelection(bool ascending)
{
    int nFirstline = 0;
    int nLastLine = GetLineCount() - 1;
    map<string, int> mSortMap;
    vector<wxString> vSortVector;
    string sCurrentLine;

    // Use the selection, if it has any
    if (HasSelection())
    {
        nFirstline = LineFromPosition(GetSelectionStart());
        nLastLine = LineFromPosition(GetSelectionEnd());
    }

    BeginUndoAction();

    // Copy the contents between starting and ending
    // line into a vector omitting the trailing line
    // ending characters. The contents are also converted
    // into a lowercase standard string and stored into a map
    for (int i = nFirstline; i <= nLastLine; i++)
    {
        // Store the line in the vector
        vSortVector.push_back(this->GetLine(i));

        if (vSortVector[i - nFirstline].find_first_of("\r\n") != string::npos)
            vSortVector[i - nFirstline].erase(vSortVector[i - nFirstline].find_first_of("\r\n"));

        // Transform it to a lowercase standard string
        sCurrentLine = toLowerCase(vSortVector[i - nFirstline].ToStdString());
        StripSpaces(sCurrentLine);

        if (!sCurrentLine.length())
            sCurrentLine = " " + toString(i + 256);

        if (mSortMap.find(sCurrentLine) != mSortMap.end())
            sCurrentLine += "\n" + toString(i + 256); // need a value smaller than space therefore the \n

        // Store it in a map
        mSortMap[sCurrentLine] = i - nFirstline;
    }

    // Use the map auto-sorting feature to refill the section
    // with the sorted lines
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

    EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Apply the function heuristics to
/// detect the current symbol's type.
///
/// \param func const std::string&
/// \return char
///
/////////////////////////////////////////////////
static char applyFunctionHeuristics(const std::string& func)
{
    // Check the following special cases
    if (func == "time" || func == "clock" || func == "to_time")
        return 't';

    // Find the key for the corresponding language string
    std::string sKey = _guilang.getKey("PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(func) + "_[*]");
    std::string sRetVal = _guilang.get(sKey);
    sRetVal.erase(0, sRetVal.find_first_not_of(' ', sRetVal.find(')')+1));
    sRetVal.erase(sRetVal.find_first_of(" -"));

    // Use the documented return value to determine the type
    // of the variable
    if (sRetVal == "VAL" || sRetVal == "{VAL}" || sRetVal == "MAT" || sRetVal == "VEC")
        return 'f';
    else if (sRetVal == "LOG" || sRetVal == "{LOG}")
        return 'b';
    else if (sRetVal == "STR" || sRetVal == "{STR}")
        return 's';
    else if (sRetVal == "ARG" || sRetVal == "{ARG}")
        return 0; // Not detectable here

    return 'f';
}


/////////////////////////////////////////////////
/// \brief Apply the command heuristics to
/// detect the current symbol's type.
///
/// \param command const std::string&
/// \return char
///
/////////////////////////////////////////////////
static char applyCommandHeuristics(const std::string& command)
{
    // Find the key for the corresponding language string
    std::string sKey = _guilang.getKey("PARSERFUNCS_LISTCMD_CMD_" + toUpperCase(command) + "_[*]");
    std::string sRetVal = _guilang.get(sKey);

    if (sRetVal.find("->") == std::string::npos)
        return 0; // s.th.s is clearly wrong here

    sRetVal.erase(0, sRetVal.find_first_not_of(' ', sRetVal.find("->")+2));
    sRetVal.erase(sRetVal.find_first_of(" -"));

    // Use the documented return value to determine the type
    // of the variable
    if (sRetVal == "VAL" || sRetVal == "{VAL}" || sRetVal == "MAT" || sRetVal == "VEC")
        return 'f';
    else if (sRetVal == "LOG" || sRetVal == "{LOG}")
        return 'b';
    else if (sRetVal == "STR" || sRetVal == "{STR}")
        return 's';
    else if (sRetVal == "ARG" || sRetVal == "{ARG}")
        return 0; // Not detectable here

    return 'f';
}


/////////////////////////////////////////////////
/// \brief Apply the value heuristics to detect
/// the current symbol's type.
///
/// \param val std::string
/// \return char
///
/////////////////////////////////////////////////
static char applyValueHeuristics(std::string val)
{
    val = getNextArgument(val);

    if (val.find("##") != std::string::npos)
        val.erase(val.find("##"));

    // We most probably have additional closing
    // parentheses
    if (!validateParenthesisNumber(val))
    {
        for (size_t i = 0; i < val.size(); i++)
        {
            if (val[i] == '(' || val[i] == '[' || val[i] == '{')
            {
                size_t pos = getMatchingParenthesis(StringView(val, i));

                // S.th. is wrong
                if (pos == std::string::npos)
                    return 0;

                i += pos;
                continue;
            }

            // These have to be unmatched
            if (val[i] == ')' || val[i] == ']' || val[i] == '}')
            {
                val.erase(i);
                break;
            }
        }
    }

    StripSpaces(val);

    if (val.back() == ';')
        val.pop_back();

    if (val == "true" || val == "false")
        return 'b';
    else if (val.find_first_not_of("0123456789^, ") == std::string::npos)
    {
        if (val == "0") // 0 is too common
            return 0;

        return 'n';
    }
    else if (val.find_first_of("/^'") != std::string::npos)
        return 'f';
    else if (val.length() > 2 && val[0] == '_' && std::islower(val[1]) && std::isupper(val[2]))
        return val[1];
    else if (val.length() > 1 && std::islower(val[0]) && std::isupper(val[1]))
        return val[0];

    return 'f';
}


/////////////////////////////////////////////////
/// \brief Fixes the symbol's type depending on
/// either the declaration point or the first
/// assigned value.
///
/// \param pos int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::fixSymbolName(int pos)
{
    if (m_fileType != FILE_NSCR && m_fileType != FILE_NPRC/* && m_fileType != FILE_MATLAB*/)
        return;

    if (!isStyleType(STYLE_IDENTIFIER, pos))
        return;

    // Get context
    std::pair<int, int> context = getCurrentContext(LineFromPosition(pos));

    // Get the symbol's name below the cursor
    std::string sSymbol = GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true)).ToStdString();
    std::string sOldName = sSymbol;

    // Find all occurences
    std::vector<int> vMatches = m_search->FindAll(sSymbol, GetStyleAt(pos),
                                                  PositionFromLine(context.first), GetLineEndPosition(context.second), false);

    // Examine the first occurence
    wxString lineContent = GetLine(LineFromPosition(vMatches.front())).Strip(wxString::both);

    // this character will contain the type of
    // this symbol once detected
    char type = 'f';

    // Is it a variable definition?
    if (lineContent.StartsWith("str "))
        type = 's';
    else
    {
        // In all other cases, we need to examine the first assigned
        // value
        for (int match : vMatches)
        {
            match += sSymbol.length();
            int lineStartPosition = PositionFromLine(LineFromPosition(match));
            lineContent = GetLine(LineFromPosition(match)).Strip(wxString::both);

            // This match is probably in a vector brace
            if (lineContent.rfind('{', match-lineStartPosition) != std::string::npos
                && lineContent.find('}', match-lineStartPosition) != std::string::npos)
            {
                int pos = match;

                while (GetCharAt(pos) != '{')
                    pos--;

                pos = BraceMatch(pos);

                // If the position is farther than the match, then
                // the match is part of a vector brace. We'll search
                // after the brace
                if (pos > match)
                    match = pos+1;
            }

            // Increment and decrement operators
            if (GetTextRange(match, match+2) == "++" || GetTextRange(match, match+2) == "--")
            {
                type = 'n';
                break;
            }

            // Find the next character, which is no whitespace and not the initial
            // character of a definition
            while (GetCharAt(match) == ' ' || GetCharAt(match) == '\t' || GetCharAt(match) == ':')
                match++;

            // Is it an assignment operator?
            if (GetCharAt(match) == '=' && GetCharAt(match+1) != '=')
            {
                // Move one char forward
                match++;

                // Find the value
                while (GetCharAt(match) == ' ' || GetCharAt(match) == '\t')
                    match++;

                // Determine the type of the value. We'll default to
                // float
                if (isStyleType(STYLE_STRING, match) || GetStyleAt(match) == wxSTC_NSCR_STRING_PARSER)
                    type = 's';
                else if (GetStyleAt(match) == wxSTC_NSCR_CONSTANTS)
                    type = 'f';
                else if (isStyleType(STYLE_FUNCTION, match))
                {
                    char funcType = applyFunctionHeuristics(GetTextRange(match, WordEndPosition(match, true)).ToStdString());

                    if (!funcType)
                        continue;

                    type = funcType;
                }
                else if (isStyleType(STYLE_COMMAND, match))
                {
                    char commandType = applyCommandHeuristics(GetTextRange(match, WordEndPosition(match, true)).ToStdString());

                    if (!commandType)
                        continue;

                    type = commandType;
                }
                else
                {
                    char valtype = applyValueHeuristics(GetTextRange(match, GetLineEndPosition(LineFromPosition(match))).ToStdString());

                    if (!valtype)
                        continue;

                    type = valtype;
                }

                // We have our information
                break;
            }
        }
    }

    // Now create the fixed variable name
    CodeAnalyzer::changeVariableType(sSymbol, type);

    if (sSymbol != sOldName)
    {
        int ret = wxMessageBox(_guilang.get("GUI_EDITOR_FIX_TYPE_MESSAGE", sOldName, sSymbol),
                               _guilang.get("GUI_EDITOR_FIX_TYPE_TITLE"),
                               wxCENTER | wxYES_NO,
                               this);

        if (ret == wxNO)
            return;

        BeginUndoAction();
        // Now replace it everywhere
        ReplaceMatches(vMatches, sOldName, sSymbol);
        EndUndoAction();
    }
}


/////////////////////////////////////////////////
/// \brief Notifies the editor that the duplicated
/// code dialog had been closed.
///
/// \return void
///
/// The notification is done by setting the
/// corresponding pointer to al nullptr.
/////////////////////////////////////////////////
void NumeReEditor::notifyDialogClose()
{
    m_duplicateCode = nullptr;
}


/////////////////////////////////////////////////
/// \brief Changes the editor's font face.
///
/// \param font const wxFont& The new font face
/// \return void
///
/// The font face is changed globally and the
/// syntax highlighing is recalculated. Called
/// after the settings dialog was closed.
/////////////////////////////////////////////////
void NumeReEditor::SetEditorFont(const wxFont& font)
{
    wxFont newFont = font;
    StyleSetFont(wxSTC_STYLE_DEFAULT, newFont);
    StyleClearAll();
    UpdateSyntaxHighlighting(true);
}


/////////////////////////////////////////////////
/// \brief Returns true, if the selected setting
/// is active.
///
/// \param _setting EditorSettings
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::getEditorSetting(EditorSettings _setting)
{
    return m_nEditorSetting & _setting;
}


/////////////////////////////////////////////////
/// \brief Enables or disables an editor setting.
///
/// \param _setting int
/// \return void
///
/// The selected setting is enabled, if it was
/// disabled and vice-versa. All necessary style
/// calculations are applied afterwards.
/////////////////////////////////////////////////
void NumeReEditor::ToggleSettings(int _setting)
{
    SetWhitespaceForeground(true, wxColor(170, 190, 210));
    SetWhitespaceSize(2);

    // Determine, whether the corresponding setting
    // is already enabled
    if (!(m_nEditorSetting & _setting))
    {
        // Enable setting
        m_nEditorSetting |= _setting;

        // Apply the necessary style calculations
        if (_setting & SETTING_WRAPEOL)
        {
            SetWrapMode(wxSTC_WRAP_WORD);
            SetWrapIndentMode(wxSTC_WRAPINDENT_INDENT);
            SetWrapStartIndent(1);
            SetWrapVisualFlags(wxSTC_WRAPVISUALFLAG_END);
            SetWrapVisualFlagsLocation(wxSTC_WRAPVISUALFLAGLOC_END_BY_TEXT);
        }

        if (_setting & SETTING_DISPCTRLCHARS)
        {
            SetViewWhiteSpace(wxSTC_WS_VISIBLEALWAYS);
            SetViewEOL(true);
        }

        if (_setting & SETTING_USESECTIONS)
            markSections(true);
    }
    else
    {
        // Disable setting
        m_nEditorSetting &= ~_setting;

        // Apply the necessary style calculations
        if (_setting == SETTING_WRAPEOL)
            SetWrapMode(wxSTC_WRAP_NONE);
        else if (_setting == SETTING_DISPCTRLCHARS)
        {
            SetViewEOL(false);
            SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
        }
        else if (_setting == SETTING_USETXTADV)
        {
            SetIndicatorCurrent(HIGHLIGHT_STRIKETHROUGH);
            IndicatorClearRange(0, GetLastPosition());
        }
        else if (_setting == SETTING_USESECTIONS)
            MarkerDeleteAll(MARKER_SECTION);
    }

    UpdateSyntaxHighlighting();
    m_analyzer->run();
}


/////////////////////////////////////////////////
/// \brief Finds and highlights the matching brace.
///
/// \param nPos int
/// \return void
///
/// This function searches the matching brace to
/// the one at the selected position and performs
/// the highlighting. If no matching brace is found,
/// the single brace is highlighted correspondingly.
/////////////////////////////////////////////////
void NumeReEditor::getMatchingBrace(int nPos)
{
    int nMatch = BraceMatch(nPos);

    // Search the matching brace
    if (nMatch == wxSTC_INVALID_POSITION)
        BraceBadLight(nPos);
    else
    {
        // If one is found, then highlight the
        // the brace and the room in between
        SetIndicatorCurrent(HIGHLIGHT_MATCHING_BRACE);
        IndicatorClearRange(0, GetLastPosition());

        if (nMatch < nPos)
        {
            BraceHighlight(nMatch, nPos);
            IndicatorFillRange(nMatch + 1, nPos - nMatch - 1);
        }
        else
        {
            BraceHighlight(nPos, nMatch);
            IndicatorFillRange(nPos + 1, nMatch - nPos - 1);
        }
    }

    m_braceIndicatorActive = true;
}


/////////////////////////////////////////////////
/// \brief Finds and highlights the matching flow
/// control statements.
///
/// \param nPos int
/// \return void
///
/// This function searches the matching flow control
/// statement to the one at the selected position
/// and performs the highlighting. If no matching
/// statement or part of it is missing, then the
/// remaining statements are highlighted
/// correspondingly.
/////////////////////////////////////////////////
void NumeReEditor::getMatchingBlock(int nPos)
{
    // Search all flow control statements
    vector<int> vMatch = BlockMatch(nPos);

    if (vMatch.size() == 1 && vMatch[0] == wxSTC_INVALID_POSITION)
        return;

    // Select the correct indicator for available
    // and missing blocks
    if (vMatch.front() == wxSTC_INVALID_POSITION || vMatch.back() == wxSTC_INVALID_POSITION)
        SetIndicatorCurrent(HIGHLIGHT_NOT_MATCHING_BLOCK);
    else
        SetIndicatorCurrent(HIGHLIGHT_MATCHING_BLOCK);

    // Clear the indicators
    IndicatorClearRange(0, GetLastPosition());

    // Highlight all occurences
    for (size_t i = 0; i < vMatch.size(); i++)
    {
        if (vMatch[i] == wxSTC_INVALID_POSITION)
            continue;

        IndicatorFillRange(vMatch[i], WordEndPosition(vMatch[i], true) - vMatch[i]);
    }

    m_blockIndicatorActive = true;
}


/////////////////////////////////////////////////
/// \brief Finds all matching flow control statements.
///
/// \param nPos int
/// \return vector<int>
///
/// Returnes a vector. If first element is invalid,
/// then the word at the position is no command.
/// If the last one is invalid, then there's no
/// matching partner. It returnes more than two
/// elements for "if" and "switch" blocks.
/// If there's no first "if" and if one currently
/// is focusing on an "else...", the first element
/// may be invalid, but more can be returned.
/////////////////////////////////////////////////
vector<int> NumeReEditor::BlockMatch(int nPos)
{
    // Select the correct helper function
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


/////////////////////////////////////////////////
/// \brief Checks, if the selected block is the
/// only block active.
///
/// \param nBlockID int
/// \param vBlocks std::vector<int>
/// \return bool
///
/////////////////////////////////////////////////
static bool isOnlyActiveBlock(int nBlockID, std::vector<int> vBlocks)
{
    for (size_t i = 0; i < vBlocks.size(); i++)
    {
        if ((int)i == nBlockID && vBlocks[i] != 1)
            return false;
        else if ((int)i != nBlockID && vBlocks[i])
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Finds all matching flow control statements
/// for NumeRe command syntax.
///
/// \param nPos int
/// \return vector<int>
///
/// See description of \c BlockMatch
/////////////////////////////////////////////////
vector<int> NumeReEditor::BlockMatchNSCR(int nPos)
{
    vector<int> vBlocks(vBlockDefs.size());

    int nStartPos = WordStartPosition(nPos, true);
    vector<int> vPos;
    wxString startblock;
    wxString endblock;

    int searchForMiddleWords = wxNOT_FOUND;
    int nSearchDir = 1; //direction in which to search for the matching block partner

    if (GetStyleAt(nPos) != wxSTC_NSCR_COMMAND && GetStyleAt(nPos) != wxSTC_NPRC_COMMAND)
    {
        if (nPos && GetStyleAt(nPos - 1) == wxSTC_NSCR_COMMAND)
            nPos--;
        else
        {
            vPos.push_back(wxSTC_INVALID_POSITION);
            return vPos;
        }
    }

    startblock = GetTextRange(WordStartPosition(nPos, true), WordEndPosition(nPos, true));

    int blockMiddle = isBlockMiddle(startblock);

    if (startblock.substr(0, 3) == "end")
    {
        endblock = getBlockStart(startblock);
        nSearchDir = -1;
    }
    else if (blockMiddle != wxNOT_FOUND)
    {
        // search for starting "if"
        // adding 1 to nIf, because we're already inside of an "if"
        vBlocks[blockMiddle]++;

        for (int i = WordEndPosition(nPos, true); i >= 0; i--)
        {
            if (GetStyleAt(i) == wxSTC_NSCR_COMMAND)
            {
                wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

                int block = isBlockStart(currentWord, false);

                // if we iterate upwards, the closing blocks shall increment and the opening blocks decrement the counter
                if (block != wxNOT_FOUND)
                    vBlocks[block]--;
                else if ((block = isBlockEnd(currentWord)) != wxNOT_FOUND)
                    vBlocks[block]++;

                if (block == blockMiddle
                    && !(*std::min_element(vBlocks.begin(), vBlocks.end()))
                    && !(*std::max_element(vBlocks.begin(), vBlocks.end())))
                {
                    nStartPos = WordStartPosition(i, true);
                    break;
                }

                i -= currentWord.length();
            }

            if (*std::min_element(vBlocks.begin(), vBlocks.end()) < 0)
            {
                // There's no matching partner
                // set the first to invalid but do not return
                vPos.push_back(wxSTC_INVALID_POSITION);
                break;
            }
        }

        if (*std::max_element(vBlocks.begin(), vBlocks.end()) > 0)
        {
            // There's no matching partner
            // set the first to invalid but do not return
            vPos.push_back(wxSTC_INVALID_POSITION);
            vBlocks.assign(vBlocks.size(), 0);
            vBlocks[blockMiddle] = 1;
        }
        else
            vBlocks.assign(vBlocks.size(), 0);

        searchForMiddleWords = blockMiddle;
        endblock = getBlockEnd(startblock);
    }
    else
        endblock = getBlockEnd(startblock);

    if (!endblock.length())
    {
        vPos.push_back(wxSTC_INVALID_POSITION);
        return vPos;
    }

    if (hasBlockMiddle(endblock))
        searchForMiddleWords = getBlockID(endblock);

    vPos.push_back(nStartPos);

    if (nSearchDir == -1)
        nStartPos = WordEndPosition(nPos, true);

    for (int i = nStartPos; (i < GetLastPosition() && i >= 0); i += nSearchDir) // iterates down, if nSearchDir == 1, and up of nSearchDir == -1
    {
        if (GetStyleAt(i) == wxSTC_NSCR_COMMAND)
        {
            wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

            int block = isBlockStart(currentWord, false);

            // if we iterate upwards, the closing blocks shall increment and the opening blocks decrement the counter
            if (block != wxNOT_FOUND)
                vBlocks[block] += nSearchDir;
            else if ((block = isBlockEnd(currentWord)) != wxNOT_FOUND)
                vBlocks[block] -= nSearchDir;

            // only in the current if block
            if (searchForMiddleWords != wxNOT_FOUND
                && isBlockMiddle(currentWord) == searchForMiddleWords
                && isOnlyActiveBlock(searchForMiddleWords, vBlocks))
                vPos.push_back(WordStartPosition(i, true));

            if (currentWord == endblock
                && !(*std::min_element(vBlocks.begin(), vBlocks.end()))
                && !(*std::max_element(vBlocks.begin(), vBlocks.end())))
            {
                vPos.push_back(WordStartPosition(i, true));
                break;
            }

            i += nSearchDir * currentWord.length();
        }

        if (*std::min_element(vBlocks.begin(), vBlocks.end()) < 0)
        {
            // There's no matching partner
            vPos.push_back(wxSTC_INVALID_POSITION);
            break;
        }
    }

    if (!vPos.size() || (*std::max_element(vBlocks.begin(), vBlocks.end()) > 0))
        vPos.push_back(wxSTC_INVALID_POSITION);

    return vPos;
}


/////////////////////////////////////////////////
/// \brief Finds all matching flow control statements
/// for MATLAB command syntax.
///
/// \param nPos int
/// \return vector<int>
///
/// See description of \c BlockMatch
/////////////////////////////////////////////////
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

    if (GetStyleAt(nPos) != wxSTC_MATLAB_KEYWORD)
    {
        if (nPos && GetStyleAt(nPos - 1) == wxSTC_MATLAB_KEYWORD)
            nPos--;
        else
        {
            vPos.push_back(wxSTC_INVALID_POSITION);
            return vPos;
        }
    }


    startblock = GetTextRange(WordStartPosition(nPos, true), WordEndPosition(nPos, true));

    if (startblock == "end")
    {
        // search for starting block
        // adding 1 to nBlock, because we're already inside of an "block"
        //nBlock++;
        for (int i = WordStartPosition(nPos, true); i >= 0; i--)
        {
            if (GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
            {
                wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

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
            if (GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
            {
                wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

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
            if (GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
            {
                wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

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
            if (GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
            {
                wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

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
        endblock = "end";
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

    //if (nSearchDir == -1)
    //    nStartPos = WordEndPosition(nPos, true);

    for (int i = nStartPos; (i < GetLastPosition() && i >= 0); i += nSearchDir) // iterates down, if nSearchDir == 1, and up of nSearchDir == -1
    {
        if (GetStyleAt(i) == wxSTC_MATLAB_KEYWORD)
        {
            wxString currentWord = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

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
                vPos.push_back(WordStartPosition(i, true));

            if (bSearchForSwitch && nBlock == 1 // only in the current if block
                    && (currentWord == "case" || currentWord == "otherwise"))
                vPos.push_back(WordStartPosition(i, true));

            if (bSearchForCatch && nBlock == 1 // only in the current if block
                    && currentWord == "catch")
                vPos.push_back(WordStartPosition(i, true));

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


/////////////////////////////////////////////////
/// \brief Marks the editor as modified.
///
/// \return void
///
/////////////////////////////////////////////////
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

    StyleSetBackground(wxSTC_STYLE_DEFAULT, m_options->GetSyntaxStyle(Options::STANDARD).background);

    StyleSetBackground(wxSTC_STYLE_LINENUMBER,  m_options->GetSyntaxStyle(Options::UI_THEME).foreground.ChangeLightness(Options::EDITORMARGIN));
    SetFoldMarginColour(true, m_options->GetSyntaxStyle(Options::UI_THEME).foreground.ChangeLightness(Options::EDITORMARGIN));

    FileFilterType filetype = GetFileType(filename);

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
    if (filetype == FILE_NSCR
        || filetype == FILE_NPRC
        || filetype == FILE_MATLAB
        || filetype == FILE_CPP
        || filetype == FILE_DIFF
        || filetype == FILE_XML
        || filetype == FILE_INI
        || filetype == FILE_TEXSOURCE)
    {
        SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

        SetMarginType(MARGIN_FOLD, wxSTC_MARGIN_SYMBOL);
        SetMarginWidth(MARGIN_FOLD, 13);
        SetMarginMask(MARGIN_FOLD, wxSTC_MASK_FOLDERS);
        SetMarginSensitive(MARGIN_FOLD, true);
        StyleSetBackground(MARGIN_FOLD, wxColor(200, 200, 200) );
        SetMarginSensitive(MARGIN_FOLD, true);

        wxColor grey( 100, 100, 100 );
        MarkerDefine (wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDER, "WHITE");
        MarkerSetBackground (wxSTC_MARKNUM_FOLDER, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDEROPEN, "WHITE");
        MarkerSetBackground (wxSTC_MARKNUM_FOLDEROPEN, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDERSUB, grey);
        MarkerSetBackground (wxSTC_MARKNUM_FOLDERSUB, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDEREND, "WHITE");
        MarkerSetBackground (wxSTC_MARKNUM_FOLDEREND, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDEROPENMID, "WHITE");
        MarkerSetBackground (wxSTC_MARKNUM_FOLDEROPENMID, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDERMIDTAIL, grey);
        MarkerSetBackground (wxSTC_MARKNUM_FOLDERMIDTAIL, grey);

        MarkerDefine (wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER);
        MarkerSetForeground (wxSTC_MARKNUM_FOLDERTAIL, grey);
        MarkerSetBackground (wxSTC_MARKNUM_FOLDERTAIL, grey);

        MarkerEnableHighlight(true);
    }

    if (filetype == FILE_NSCR)
    {
        m_fileType = FILE_NSCR;
        SetLexer(wxSTC_LEX_NSCR);
        SetProperty("fold", "1");

        if (_syntax)
        {
            SetKeyWords(0, _syntax->getCommands());
            SetKeyWords(1, _syntax->getOptions());
            SetKeyWords(2, _syntax->getFunctions());
            SetKeyWords(3, _syntax->getMethods());
            SetKeyWords(4, _syntax->getBlockDefs());
            SetKeyWords(5, _syntax->getConstants());
            SetKeyWords(6, _syntax->getSpecial());
            SetKeyWords(7, _syntax->getOperators());
            SetKeyWords(8, _syntax->getDocKeyWords());
            SetKeyWords(9, _syntax->getNPRCCommands());
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
                case wxSTC_NSCR_DOCCOMMENT_BLOCK:
                case wxSTC_NSCR_DOCCOMMENT_LINE:
                    _style = m_options->GetSyntaxStyle(Options::DOCCOMMENT);
                    break;
                case wxSTC_NSCR_DOCKEYWORD:
                    _style = m_options->GetSyntaxStyle(Options::DOCKEYWORD);
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
                case wxSTC_NSCR_CLUSTER:
                    _style = m_options->GetSyntaxStyle(Options::CLUSTER);
                    break;
                case wxSTC_NSCR_OPERATORS:
                case wxSTC_NSCR_OPERATOR_KEYWORDS:
                    _style = m_options->GetSyntaxStyle(Options::OPERATOR);
                    break;
            }

            StyleSetForeground(i, _style.foreground);

            if (!_style.defaultbackground)
                StyleSetBackground(i, _style.background);
            else
                StyleSetBackground(i, StyleGetBackground(wxSTC_STYLE_DEFAULT));

            StyleSetBold(i, _style.bold);
            StyleSetItalic(i, _style.italics);
            StyleSetUnderline(i, _style.underline);
        }
    }
    else if (filetype == FILE_NPRC)
    {
        m_fileType = FILE_NPRC;
        SetLexer(wxSTC_LEX_NPRC);
        SetProperty("fold", "1");

        if (_syntax)
        {
            SetKeyWords(0, _syntax->getCommands() + _syntax->getNPRCCommands());
            SetKeyWords(1, _syntax->getOptions());
            SetKeyWords(2, _syntax->getFunctions());
            SetKeyWords(3, _syntax->getMethods());
            SetKeyWords(4, _syntax->getBlockDefs());
            SetKeyWords(5, _syntax->getConstants());
            SetKeyWords(6, _syntax->getSpecial());
            SetKeyWords(7, _syntax->getOperators());
            SetKeyWords(8, _syntax->getDocKeyWords());
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
                case wxSTC_NPRC_DOCCOMMENT_BLOCK:
                case wxSTC_NPRC_DOCCOMMENT_LINE:
                    _style = m_options->GetSyntaxStyle(Options::DOCCOMMENT);
                    break;
                case wxSTC_NPRC_DOCKEYWORD:
                    _style = m_options->GetSyntaxStyle(Options::DOCKEYWORD);
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
                case wxSTC_NPRC_CLUSTER:
                    _style = m_options->GetSyntaxStyle(Options::CLUSTER);
                    break;
                case wxSTC_NPRC_OPERATORS:
                case wxSTC_NPRC_OPERATOR_KEYWORDS:
                    _style = m_options->GetSyntaxStyle(Options::OPERATOR);
                    break;
            }

            StyleSetForeground(i, _style.foreground);

            if (!_style.defaultbackground)
                StyleSetBackground(i, _style.background);
            else
                StyleSetBackground(i, StyleGetBackground(wxSTC_STYLE_DEFAULT));

            StyleSetBold(i, _style.bold);
            StyleSetItalic(i, _style.italics);
            StyleSetUnderline(i, _style.underline);
        }
    }
    else if (filetype == FILE_TEXSOURCE)
    {
        SetLexer(wxSTC_LEX_TEX);
        SetProperty("fold", "1");

        StyleSetForeground(wxSTC_TEX_DEFAULT, wxColor(0, 128, 0)); //Comment
        StyleSetForeground(wxSTC_TEX_COMMAND, wxColor(0, 0, 255)); //Command
        StyleSetBold(wxSTC_TEX_COMMAND, true);
        StyleSetUnderline(wxSTC_TEX_COMMAND, false);
        StyleSetForeground(wxSTC_TEX_TEXT, wxColor(0, 0, 0)); // Actual text
        StyleSetForeground(wxSTC_TEX_GROUP, wxColor(0, 128, 0)); // Grouping elements like $ $ or { }
        StyleSetBackground(wxSTC_TEX_GROUP, wxColor(255, 255, 183)); // Grouping elements like $ $ or { }
        StyleSetBold(wxSTC_TEX_GROUP, true);
        StyleSetForeground(wxSTC_TEX_SPECIAL, wxColor(255, 0, 196)); // Parentheses/Brackets
        StyleSetItalic(wxSTC_TEX_SPECIAL, false);
        StyleSetBold(wxSTC_TEX_SPECIAL, true);
        StyleSetForeground(wxSTC_TEX_SYMBOL, wxColor(255, 0, 0)); // Operators
        StyleSetBackground(wxSTC_TEX_SYMBOL, wxColor(255, 255, 255));
        StyleSetBold(wxSTC_TEX_SYMBOL, false);
    }
    else if (filetype == FILE_DATAFILES)
    {
        SetLexer(wxSTC_LEX_OCTAVE);
        StyleSetForeground(wxSTC_MATLAB_COMMENT, wxColor(0, 128, 0));
        StyleSetItalic(wxSTC_MATLAB_COMMENT, false);
        StyleSetForeground(wxSTC_MATLAB_OPERATOR, wxColor(255, 0, 0));
        StyleSetBold(wxSTC_MATLAB_OPERATOR, false);
        StyleSetForeground(wxSTC_MATLAB_NUMBER, wxColor(0, 0, 128));
        StyleSetBackground(wxSTC_MATLAB_NUMBER, wxColor(255, 255, 255));
        StyleSetForeground(wxSTC_MATLAB_IDENTIFIER, wxColor(0, 0, 0));
        StyleSetBold(wxSTC_MATLAB_IDENTIFIER, false);
    }
    else if (filetype == FILE_MATLAB)
    {
        SetLexer(wxSTC_LEX_MATLAB);
        SetProperty("fold", "1");

        if (_syntax)
        {
            SetKeyWords(0, _syntax->getMatlab());
            SetKeyWords(1, _syntax->getMatlabFunctions());
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

            StyleSetForeground(i, _style.foreground);

            if (!_style.defaultbackground)
                StyleSetBackground(i, _style.background);
            else
                StyleSetBackground(i, StyleGetBackground(wxSTC_STYLE_DEFAULT));

            StyleSetBold(i, _style.bold);
            StyleSetItalic(i, _style.italics);
            StyleSetUnderline(i, _style.underline);
        }
    }
    else if (filetype == FILE_CPP)
    {
        SetLexer(wxSTC_LEX_CPP);
        SetProperty("fold", "1");

        if (_syntax)
        {
            SetKeyWords(0, _syntax->getCpp());
            SetKeyWords(1, _syntax->getCppFunctions());
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
                case wxSTC_C_COMMENTDOC:
                case wxSTC_C_COMMENTLINEDOC:
                    _style = m_options->GetSyntaxStyle(Options::DOCCOMMENT);
                    break;
                case wxSTC_C_COMMENTDOCKEYWORD:
                case wxSTC_C_COMMENTDOCKEYWORDERROR:
                    _style = m_options->GetSyntaxStyle(Options::DOCKEYWORD);
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

            StyleSetForeground(i, _style.foreground);

            if (!_style.defaultbackground)
                StyleSetBackground(i, _style.background);
            else
                StyleSetBackground(i, StyleGetBackground(wxSTC_STYLE_DEFAULT));

            StyleSetBold(i, _style.bold);
            StyleSetItalic(i, _style.italics);
            StyleSetUnderline(i, _style.underline);
        }
    }
    else if (filetype == FILE_DIFF)
    {
        SetLexer(wxSTC_LEX_DIFF);
        SetProperty("fold", "1");
        StyleSetForeground(wxSTC_DIFF_ADDED, wxColour(0, 128, 0));
        StyleSetBackground(wxSTC_DIFF_ADDED, wxColour(210, 255, 210));
        StyleSetForeground(wxSTC_DIFF_CHANGED, wxColour(128, 0, 0));
        StyleSetBackground(wxSTC_DIFF_CHANGED, wxColour(255, 210, 210));
        StyleSetForeground(wxSTC_DIFF_DELETED, wxColour(128, 0, 0));
        StyleSetBackground(wxSTC_DIFF_DELETED, wxColour(255, 210, 210));
        StyleSetForeground(wxSTC_DIFF_DEFAULT, *wxBLACK);
        StyleSetBackground(wxSTC_DIFF_DEFAULT, *wxWHITE);
        StyleSetForeground(wxSTC_DIFF_HEADER, *wxBLUE);
        StyleSetBackground(wxSTC_DIFF_HEADER, *wxWHITE);
        StyleSetForeground(wxSTC_DIFF_POSITION, wxColour(255, 128, 0));
        StyleSetBackground(wxSTC_DIFF_POSITION, *wxWHITE);
        StyleSetBold(wxSTC_DIFF_POSITION, true);
    }
    else if (filetype == FILE_XML)
    {
        SetLexer(wxSTC_LEX_XML);
        SetProperty("fold", "1");
        StyleSetForeground(wxSTC_H_DEFAULT, wxColour(0,0,0));
        StyleSetForeground(wxSTC_H_TAG, wxColour(0,0,255));
        StyleSetBold(wxSTC_H_TAG, true);
        StyleSetForeground(wxSTC_H_TAGEND, wxColour(0,0,255));
        StyleSetBold(wxSTC_H_TAGEND, true);
        StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(255,0,0));
        StyleSetBackground(wxSTC_H_ATTRIBUTE, wxColour(255,255,255));
        StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(128,0,255));
        StyleSetBold(wxSTC_H_DOUBLESTRING, true);
        StyleSetForeground(wxSTC_H_SINGLESTRING, wxColour(128,0,255));
        StyleSetBold(wxSTC_H_SINGLESTRING, true);
        StyleSetForeground(wxSTC_H_COMMENT, wxColour(0,128,0));
        StyleSetItalic(wxSTC_H_COMMENT, true);
        StyleSetForeground(wxSTC_H_ENTITY, wxColour(64,0,0));
        StyleSetBackground(wxSTC_H_ENTITY, wxColour(255,255,220));
        StyleSetBold(wxSTC_H_ENTITY, true);
    }
    else if (filetype == FILE_INI)
    {
        SetLexer(wxSTC_LEX_PROPERTIES);
        SetProperty("fold", "1");
        StyleSetForeground(wxSTC_PROPS_DEFAULT, wxColour(0,0,0));
        StyleSetForeground(wxSTC_PROPS_KEY, wxColour(0,0,128));
        StyleSetBold(wxSTC_PROPS_KEY, true);
        StyleSetForeground(wxSTC_PROPS_DEFVAL, wxColour(255,0,255));
        StyleSetForeground(wxSTC_PROPS_ASSIGNMENT, wxColour(255,0,0));
        StyleSetBackground(wxSTC_PROPS_ASSIGNMENT, wxColour(255,255,255));
        StyleSetForeground(wxSTC_PROPS_SECTION, wxColour(128,0,0));
        StyleSetBold(wxSTC_PROPS_SECTION, true);
        StyleSetForeground(wxSTC_PROPS_COMMENT, wxColour(0,128,0));
        StyleSetItalic(wxSTC_PROPS_COMMENT, true);
    }
    else
    {
        if (!getEditorSetting(SETTING_USETXTADV))
        {
            SetLexer(wxSTC_LEX_NULL);
            ClearDocumentStyle();
        }
        else
        {
            SetLexer(wxSTC_LEX_TXTADV);
            StyleSetItalic(wxSTC_TXTADV_DEFAULT, false);
            StyleSetItalic(wxSTC_TXTADV_MODIFIER, true);
            StyleSetForeground(wxSTC_TXTADV_MODIFIER, wxColor(255, 180, 180));
            StyleSetItalic(wxSTC_TXTADV_ITALIC, true);
            StyleSetItalic(wxSTC_TXTADV_BOLD, false);
            StyleSetBold(wxSTC_TXTADV_BOLD, true);
            StyleSetItalic(wxSTC_TXTADV_BOLD_ITALIC, true);
            StyleSetBold(wxSTC_TXTADV_BOLD_ITALIC, true);
            StyleSetUnderline(wxSTC_TXTADV_UNDERLINE, true);
            StyleSetForeground(wxSTC_TXTADV_STRIKETHROUGH, wxColor(140, 140, 140));
            StyleSetItalic(wxSTC_TXTADV_STRIKETHROUGH, true);
            StyleSetUnderline(wxSTC_TXTADV_URL, true);
            StyleSetForeground(wxSTC_TXTADV_URL, wxColor(0, 0, 255));
            StyleSetUnderline(wxSTC_TXTADV_HEAD, true);
            StyleSetBold(wxSTC_TXTADV_HEAD, true);
            StyleSetUnderline(wxSTC_TXTADV_BIGHEAD, true);
            StyleSetBold(wxSTC_TXTADV_BIGHEAD, true);
            StyleSetSize(wxSTC_TXTADV_BIGHEAD, this->StyleGetSize(0) + 1);
            StyleSetCase(wxSTC_TXTADV_BIGHEAD, wxSTC_CASE_UPPER);
        }
    }

    updateDefaultHighlightSettings();
    Colourise(0, -1);
    applyStrikeThrough();
    markLocalVariables(true);
}


/////////////////////////////////////////////////
/// \brief Defines and applies styles to all
/// needed indicators.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::UpdateIndicators()
{
    // Indicator for the matching brace
    IndicatorSetStyle(HIGHLIGHT_MATCHING_BRACE, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetForeground(HIGHLIGHT_MATCHING_BRACE, wxColour(0, 200, 0));

    // Indicator for a matching control
    // flow block
    IndicatorSetStyle(HIGHLIGHT_MATCHING_BLOCK, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT_MATCHING_BLOCK, 100);
    IndicatorSetForeground(HIGHLIGHT_MATCHING_BLOCK, wxColour(0, 220, 0));

    // Indicator for a non-matching
    // control flow block
    IndicatorSetStyle(HIGHLIGHT_NOT_MATCHING_BLOCK, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT_NOT_MATCHING_BLOCK, 128);
    IndicatorSetForeground(HIGHLIGHT_NOT_MATCHING_BLOCK, wxColour(255, 0, 0));

    // Indicator for highlighted occurences
    // after double-clicking
    IndicatorSetStyle(HIGHLIGHT_DBLCLK, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT_DBLCLK, 80);
    IndicatorSetForeground(HIGHLIGHT_DBLCLK, wxColor(0, 255, 0));

    // Indicator for permanently highlighted
    // occurences
    IndicatorSetStyle(HIGHLIGHT, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT, 100);
    IndicatorSetForeground(HIGHLIGHT, wxColor(255, 0, 0));

    // Indicator for strike-through
    IndicatorSetStyle(HIGHLIGHT_STRIKETHROUGH, wxSTC_INDIC_STRIKE);
    IndicatorSetForeground(HIGHLIGHT_STRIKETHROUGH, wxColor(255, 0, 0));

    // Indicator for highlighting local
    // variables
    IndicatorSetStyle(HIGHLIGHT_LOCALVARIABLES, wxSTC_INDIC_DOTS);
    IndicatorSetForeground(HIGHLIGHT_LOCALVARIABLES, wxColour(0, 0, 255));
//    IndicatorSetForeground(HIGHLIGHT_LOCALVARIABLES, wxColour(255, 128, 128));

    // Indicators for highlighting differences in
    // the duplicated code analysis
    IndicatorSetStyle(HIGHLIGHT_DIFFERENCES, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT_DIFFERENCES, 64);
    IndicatorSetForeground(HIGHLIGHT_DIFFERENCES, wxColour(128, 0, 128));

    IndicatorSetStyle(HIGHLIGHT_DIFFERENCE_SOURCE, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(HIGHLIGHT_DIFFERENCE_SOURCE, 64);
    IndicatorSetForeground(HIGHLIGHT_DIFFERENCE_SOURCE, wxColour(0, 128, 128));
}


/////////////////////////////////////////////////
/// \brief Performs all general default syntax
/// highlightings.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::updateDefaultHighlightSettings()
{
    this->CallTipSetForegroundHighlight(*wxBLUE);
    this->SetCaretLineVisible(true);
    this->SetIndentationGuides(wxSTC_IV_LOOKBOTH);

    this->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, m_options->GetSyntaxStyle(Options::STANDARD).foreground);
    this->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, m_options->GetSyntaxStyle(Options::STANDARD).background);

    this->SetCaretForeground(m_options->GetSyntaxStyle(Options::STANDARD).foreground);

    // Use these styles for enabling black mode
//	this->StyleSetBackground(wxSTC_STYLE_LINENUMBER, m_options->GetSyntaxStyle(Options::STANDARD).background);
//	this->StyleSetForeground(wxSTC_STYLE_LINENUMBER, m_options->GetSyntaxStyle(Options::STANDARD).foreground);
//
//	this->SetFoldMarginColour(true, m_options->GetSyntaxStyle(Options::STANDARD).background);
//	this->SetFoldMarginHiColour(true, m_options->GetSyntaxStyle(Options::STANDARD).foreground);

    if (!m_options->GetSyntaxStyle(Options::ACTIVE_LINE).defaultbackground)
        this->SetCaretLineBackground(m_options->GetSyntaxStyle(Options::ACTIVE_LINE).background);
    else
        this->SetCaretLineVisible(false);

    if (m_options->getSetting(SETTING_B_LINELENGTH).active())
    {
        SetEdgeColour(wxColour(210, 210, 255));
        SetEdgeMode(wxSTC_EDGE_LINE);
        SetEdgeColumn(LINELENGTH_COLUMN);
    }
    else
        SetEdgeMode(wxSTC_EDGE_NONE);

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
    this->StyleSetBackground(ANNOTATION_WARN, wxColour(255, 240, 210));
    this->StyleSetForeground(ANNOTATION_WARN, wxColour(200, 120, 0));
    this->StyleSetSize(ANNOTATION_WARN, nAnnotationFontSize);
    this->StyleSetItalic(ANNOTATION_WARN, true);
    this->StyleSetFaceName(ANNOTATION_WARN, "Segoe UI");
    this->StyleSetBackground(ANNOTATION_ERROR, wxColour(255, 225, 225));
    this->StyleSetForeground(ANNOTATION_ERROR, wxColour(170, 0, 0));
    this->StyleSetSize(ANNOTATION_ERROR, nAnnotationFontSize);
    this->StyleSetItalic(ANNOTATION_ERROR, true);
    this->StyleSetFaceName(ANNOTATION_ERROR, "Segoe UI");
}


/////////////////////////////////////////////////
/// \brief Applies the strike-through effect.
///
/// \return void
///
/// This function is only used inf the advanced
/// text mode of the editor.
/////////////////////////////////////////////////
void NumeReEditor::applyStrikeThrough()
{
    if (!getEditorSetting(SETTING_USETXTADV)
            || m_fileType == FILE_NSCR
            || m_fileType == FILE_NPRC
            || m_fileType == FILE_MATLAB
            || m_fileType == FILE_XML
            || m_fileType == FILE_TEXSOURCE
            || m_fileType == FILE_DATAFILES)
        return;

    SetIndicatorCurrent(HIGHLIGHT_STRIKETHROUGH);
    IndicatorClearRange(0, GetLastPosition());

    for (int i = 0; i < GetLastPosition(); i++)
    {
        if (GetStyleAt(i) == wxSTC_TXTADV_STRIKETHROUGH)
        {
            for (int j = i; j < GetLastPosition(); j++)
            {
                if (GetStyleAt(j) == wxSTC_TXTADV_MODIFIER || j == GetLastPosition() - 1)
                {
                    IndicatorFillRange(i, j - i);
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
    if (GetFileNameAndPath().ToStdString().length())
        m_terminal->clearBreakpoints(GetFileNameAndPath().ToStdString());

    m_bLastSavedRemotely = fileIsRemote;
    m_fileNameAndPath = filename;
    SynchronizeBreakpoints();
}


/////////////////////////////////////////////////
/// \brief Returns the FileFilterType which
/// corresponds to the passed file name.
///
/// \param filename const wxString&
/// \return FileFilterType
///
/////////////////////////////////////////////////
FileFilterType NumeReEditor::GetFileType(const wxString& filename)
{
	wxFileName file(filename);

	wxString extension = file.GetExt().Lower();

	FileFilterType fileType = FILE_NONSOURCE;

	if (extension == "nprc")
		fileType = FILE_NPRC;
	else if (extension == "nscr" || extension == "nlyt")
		fileType = FILE_NSCR;
	else if (extension == "ndat" || extension == "dat" || extension == "csv" || extension == "jdx" || extension == "dx" || extension == "jcm")
		fileType = FILE_DATAFILES;
	else if (extension == "tex")
        fileType = FILE_TEXSOURCE;
	else if (extension == "m")
        fileType = FILE_MATLAB;
	else if (extension == "cxx" || extension == "hxx" || extension == "c" || extension == "h" || extension == "cpp" || extension == "hpp")
        fileType = FILE_CPP;
	else if (extension == "diff" || extension == "patch")
        fileType = FILE_DIFF;
	else if (extension == "nhlp" || extension == "xml" || extension == "npkp")
        fileType = FILE_XML;
	else if (extension == "ini" || extension == "cfg" || extension == "conf")
        fileType = FILE_INI;

	return fileType;
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
    return m_fileNameAndPath.GetPath();
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
    m_clickedWord.clear();
    m_clickedProcedure.clear();
    m_clickedInclude.clear();
    m_clickedWordLength = 0;
    m_watchedString.clear();
    vRenameSymbolsChangeLog.clear();

    SetReadOnly(false);
    SetText(wxEmptyString);
    SetSavePoint();
    EmptyUndoBuffer();

    MarkerDeleteAll(MARKER_BREAKPOINT);
    MarkerDeleteAll(MARKER_CONDITIONALBREAKPOINT);
    MarkerDeleteAll(MARKER_FOCUSEDLINE);
    MarkerDeleteAll(MARKER_MODIFIED);
    MarkerDeleteAll(MARKER_SAVED);
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
    const int nINSERTIONPOINT = 16;
    wxString clickedWord = m_search->FindClickedWord();

    // Determine the marker and breakpoint conditions
    bool breakpointOnLine = MarkerOnLine(linenum, MARKER_BREAKPOINT) || MarkerOnLine(linenum, MARKER_CONDITIONALBREAKPOINT);
    bool bookmarkOnLine = MarkerOnLine(linenum, MARKER_BOOKMARK);
    bool breakpointsAllowed = isNumeReFileType();

    // returns a copy of a member variable, which would seem sort of pointless, but
    // GetBreakpoints cleans up any stray marker IDs in the list before returning
    // so we have an accurate count of how many breakpoints there are
    wxArrayInt currentBreakpoints = GetBreakpoints();
    bool canClearBreakpoints = currentBreakpoints.GetCount() > 0;

    // Prepare the context menu
    if (m_popupMenu.FindItem(ID_DEBUG_DISPLAY_SELECTION) != nullptr)
    {
        m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, false);
        m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", "..."));
    }

    if (m_popupMenu.FindItem(ID_FIND_PROCEDURE) != nullptr)
    {
        m_popupMenu.Remove(ID_FIND_PROCEDURE);
    }

    if (m_popupMenu.FindItem(ID_FIND_INCLUDE) != nullptr)
    {
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
        m_refactoringMenu->Enable(ID_FIX_TYPE, false);
        m_popupMenu.Remove(ID_REFACTORING_MENU);
    }

    // Enable menus depending on the marker and breakpint states
    m_popupMenu.Enable(ID_FOLD_CURRENT_BLOCK, isCodeFile());
    m_popupMenu.Enable(ID_HIDE_SELECTION, HasSelection());
    m_popupMenu.Enable(ID_DEBUG_ADD_BREAKPOINT, breakpointsAllowed && !breakpointOnLine);
    m_popupMenu.Enable(ID_DEBUG_EDIT_BREAKPOINT, breakpointsAllowed && breakpointOnLine);
    m_popupMenu.Enable(ID_DEBUG_REMOVE_BREAKPOINT, breakpointsAllowed && breakpointOnLine);
    m_popupMenu.Enable(ID_BOOKMARK_ADD, !bookmarkOnLine);
    m_popupMenu.Enable(ID_BOOKMARK_REMOVE, bookmarkOnLine);
    m_popupMenu.Enable(ID_DEBUG_CLEAR_ALL_BREAKPOINTS, canClearBreakpoints);

    // Enable upper- and lowercase if the user made
    // a selection in advance
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

    // If th user clicked a word or made a selection
    if (clickedWord.length() > 0 || HasSelection())
    {
        if (this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURES)
        {
            // Show "find procedure"
            wxString clickedProc = m_search->FindClickedProcedure();

            if (clickedProc.length())
            {
                m_popupMenu.Insert(nINSERTIONPOINT, m_menuFindProcedure);
                m_menuFindProcedure->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_FINDPROC", clickedProc.ToStdString()));
            }
        }
        else if (this->GetStyleAt(charpos) == wxSTC_NSCR_COMMAND
                || this->GetStyleAt(charpos) == wxSTC_NSCR_PROCEDURE_COMMANDS
                || this->GetStyleAt(charpos) == wxSTC_NSCR_OPTION)
        {
            // Show "help on item"
            m_popupMenu.Insert(nINSERTIONPOINT, m_menuHelpOnSelection);
            m_menuHelpOnSelection->SetItemLabel(_guilang.get("GUI_TREE_PUP_HELPONITEM", clickedWord.ToStdString()));
        }
        else if (this->GetStyleAt(charpos) == wxSTC_NSCR_INCLUDES
                || this->GetStyleAt(charpos) == wxSTC_NPRC_INCLUDES)
        {
            // Show "find included file"
            wxString clickedInclude = m_search->FindClickedInclude();

            if (clickedInclude.length())
            {
                m_popupMenu.Insert(nINSERTIONPOINT, m_menuFindInclude);
                m_menuFindInclude->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_FINDINCLUDE", clickedInclude.ToStdString()));
            }
        }
        else
        {
            // Show the refactoring menu
            m_popupMenu.Insert(nINSERTIONPOINT, m_menuRefactoring);

            if (isStyleType(STYLE_DEFAULT, charpos)
                || isStyleType(STYLE_IDENTIFIER, charpos)
                || isStyleType(STYLE_DATAOBJECT, charpos)
                || isStyleType(STYLE_FUNCTION, charpos))
                m_refactoringMenu->Enable(ID_RENAME_SYMBOLS, true);

            if (isStyleType(STYLE_IDENTIFIER, charpos))
                m_refactoringMenu->Enable(ID_FIX_TYPE, true);

            if (HasSelection())
                m_refactoringMenu->Enable(ID_ABSTRAHIZE_SECTION, true);
        }

        // Enable the "highlight" menu item and add the clicked word
        // to the item text
        m_popupMenu.Enable(ID_DEBUG_DISPLAY_SELECTION, true);
        m_menuShowValue->SetItemLabel(_guilang.get("GUI_MENU_EDITOR_HIGHLIGHT", clickedWord.ToStdString()));

        // Set the boolean flag to correspond to the highlight
        // state of the clicked word
        if (m_clickedWord == m_watchedString && m_clickedWord.length())
            m_menuShowValue->Check(true);
        else
            m_menuShowValue->Check(false);
    }

    // Cancel the call tip, if there's one active
    if (this->CallTipActive())
        this->AdvCallTipCancel();

    PopupMenu(&m_popupMenu, m_lastRightClick);
    m_PopUpActive = false;
}


/////////////////////////////////////////////////
/// \brief Marks the selected line as modified.
///
/// \param nLine int
/// \return void
///
/// The modified bar on the left margin will be
/// shown in yellow.
/////////////////////////////////////////////////
void NumeReEditor::markModified(int nLine)
{
    int nMarkSaved = 1 << (MARKER_SAVED);
    int nMarkModified = 1 << (MARKER_MODIFIED);

    while (this->MarkerGet(nLine) & nMarkSaved)
        this->MarkerDelete(nLine, MARKER_SAVED);

    if (!(this->MarkerGet(nLine) & nMarkModified))
        this->MarkerAdd(nLine, MARKER_MODIFIED);
}


/////////////////////////////////////////////////
/// \brief Marks the complete document as saved.
///
/// \return void
///
/// The modified bar on the left margin will be
/// shown on green, where it had been shown in
/// yellow before.
/////////////////////////////////////////////////
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
    if (!m_bLoadingFile && (event.GetModificationType() & wxSTC_MOD_INSERTTEXT || event.GetModificationType() & wxSTC_MOD_DELETETEXT))
    {
        m_modificationHappened = true;
        int nLine = this->LineFromPosition(event.GetPosition());
        int nLinesAdded = event.GetLinesAdded();

        if (nLinesAdded > 0)
        {
            for (int i = 0; i < nLinesAdded; i++)
                this->markModified(i + nLine);
        }
        else if (nLinesAdded < 0)
            this->markModified(nLine);
        else
            this->markModified(nLine);
    }

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Folds the block, to which the current
/// line belongs.
///
/// \param nLine int
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Adds markers to section headlines.
///
/// \param bForceRefresh bool \c true to clear
/// every marker first
/// \return void
///
/// Adds markers on the sidebar at the headline of
/// every section of NumeRe, MATLAB, LaTeX and text
/// files with advanced text highlighting active.
/// The markers are used as automatic bookmarks,
/// so the user may jump to the markers using the
/// same short key.
/////////////////////////////////////////////////
void NumeReEditor::markSections(bool bForceRefresh)
{
    // Ensure, that sectioning is activated
    if (!getEditorSetting(SETTING_USESECTIONS))
        return;

    if (bForceRefresh)
        this->MarkerDeleteAll(MARKER_SECTION);

    int startline = 0;
    int endline = this->GetLineCount();

    // Find the first and last line, if not the whole
    // editor shall be examined
    if (!bForceRefresh)
    {
        int markermask = (1 << MARKER_SECTION);

        if ((startline = this->MarkerPrevious(this->GetCurrentLine() - 1, markermask)) == -1)
            startline = 0;

        if ((endline = this->MarkerNext(this->GetCurrentLine() + 1, markermask)) == -1)
            endline = this->GetLineCount();
    }

    // NumeRe or MATLAB file?
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB)
    {
        // Search for documentation comment blocks
        // in the file and add markers
        for (int i = startline; i < endline; i++)
        {
            if (isStyleType(STYLE_COMMENT_SECTION_LINE, GetLineIndentPosition(i))
                || isStyleType(STYLE_COMMENT_SECTION_BLOCK, GetLineIndentPosition(i)))
            {
                if (!MarkerOnLine(i, MARKER_SECTION))
                    MarkerAdd(i, MARKER_SECTION);

                while (isStyleType(STYLE_COMMENT_SECTION_LINE, GetLineIndentPosition(i+1))
                       || isStyleType(STYLE_COMMENT_SECTION_BLOCK, GetLineIndentPosition(i+1)))
                {
                    i++;

                    // Only keep the very first section. Should also remove erroneously
                    // placed markers
                    if (MarkerOnLine(i, MARKER_SECTION))
                        MarkerDelete(i, MARKER_SECTION);
                }
            }
            else if (MarkerOnLine(i, MARKER_SECTION))
                MarkerDelete(i, MARKER_SECTION);
        }
    }

    // LaTeX source?
    if (m_fileType == FILE_TEXSOURCE)
    {
        // Search for LaTeX sectioning commands in
        // the examination range and add markers to them
        for (int i = startline; i < endline; i++)
        {
            for (int j = PositionFromLine(i); j < GetLineEndPosition(i) + 1; j++)
            {
                if (GetCharAt(j) == ' ' || GetCharAt(j) == '\t')
                    continue;

                if (GetStyleAt(j) == wxSTC_TEX_COMMAND)
                {
                    int wordstart = WordStartPosition(j, false);
                    int wordend = WordEndPosition(j, false);

                    wxString word = GetTextRange(wordstart, wordend);

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
                        if (!MarkerOnLine(i, MARKER_SECTION))
                            MarkerAdd(i, MARKER_SECTION);
                    }

                    j = wordend;
                }
            }
        }
    }

    // Text file with advanced highlighting?
    if (m_fileType == FILE_NONSOURCE && getEditorSetting(SETTING_USETXTADV))
    {
        // Search for all headlines in the document and
        // add markers to them
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


/////////////////////////////////////////////////
/// \brief This method wraps the detection of
/// local variables.
///
/// \param bForceRefresh bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::markLocalVariables(bool bForceRefresh)
{
    SetIndicatorCurrent(HIGHLIGHT_LOCALVARIABLES);

    // We clean everything, if need to refresh the indicators
    if (bForceRefresh)
        IndicatorClearRange(0, GetLastPosition());

    if (m_fileType != FILE_NPRC || !m_options->GetHighlightLocalVariables())
        return;

    std::pair<int,int> context = getCurrentContext(GetCurrentLine());
    IndicatorClearRange(PositionFromLine(context.first),
                        GetLineEndPosition(context.second) - PositionFromLine(context.first));

    // Run the algorithm for every possible local variable declarator
    markLocalVariableOfType("var", bForceRefresh);
    markLocalVariableOfType("cst", bForceRefresh);
    markLocalVariableOfType("tab", bForceRefresh);
    markLocalVariableOfType("str", bForceRefresh);
}


/////////////////////////////////////////////////
/// \brief This method finds all local variables
/// of the selected type and highlights their
/// definitions.
///
/// \param command const wxString&
/// \param bForceRefresh bool
/// \return void
///
/// If the refresh flag is not set, then the
/// alogrithm will only update the current edited
/// line of the editor.
/////////////////////////////////////////////////
void NumeReEditor::markLocalVariableOfType(const wxString& command, bool bForceRefresh)
{
    std::vector<int> matches;
    std::pair<int, int> context = std::make_pair(0, GetLineCount());

    // Find the occurences of the variable declaration commands
    // in the corresponding scope
    if (!bForceRefresh)
    {
        context = getCurrentContext(GetCurrentLine());
        matches = m_search->FindAll(command,
                                    wxSTC_NPRC_COMMAND,
                                    PositionFromLine(context.first),
                                    GetLineEndPosition(context.second),
                                    false);
    }
    else
        matches = m_search->FindAll(command,
                                    wxSTC_NPRC_COMMAND,
                                    0,
                                    GetLastPosition(),
                                    false);

    // Run through all found occurences and extract the definitions
    // of the local variables
    for (size_t i = 0; i < matches.size(); i++)
    {
        context = getCurrentContext(LineFromPosition(i));
        wxString line = GetTextRange(matches[i]+command.length(), GetLineEndPosition(LineFromPosition(matches[i])));
        int nPos = line.find_first_not_of(' ') + matches[i] + command.length();

        for (int j = nPos; j < GetLineEndPosition(LineFromPosition(matches[i])) + 1; j++)
        {
            char currentChar = GetCharAt(j);

            // If a separator character was found, highlight
            // the current word and find the next candidate
            if (currentChar == ' '
                || currentChar == '='
                || currentChar == ','
                || currentChar == '('
                || currentChar == '{'
                || currentChar == ';'
                || currentChar == '\r'
                || currentChar == '\n')
            {
                std::vector<int> localVars = m_search->FindAll(GetTextRange(nPos, j),
                                                               GetStyleAt(nPos),
                                                               nPos,
                                                               GetLineEndPosition(context.second),
                                                               false);

                for (int n : localVars)
                {
                    IndicatorFillRange(n, j - nPos);
                }

                if (currentChar == ',')
                {
                    // Only a declaration, find the next one
                    while (GetCharAt(j) == ',' || GetCharAt(j) == ' ')
                        j++;
                }
                else if (currentChar == ';')
                    break;
                else
                {
                    // This is also an initialization. Find the
                    // next declaration by jumping over the assigned
                    // value.
                    for (int l = j; l < GetLineEndPosition(LineFromPosition(matches[i])); l++)
                    {
                        if (GetCharAt(l) == ',' && GetStyleAt(l) == wxSTC_NPRC_OPERATORS)
                        {
                            while (GetCharAt(l) == ',' || GetCharAt(l) == ' ')
                                l++;

                            j = l;
                            break;
                        }
                        else if (GetStyleAt(l) == wxSTC_NPRC_OPERATORS && (GetCharAt(l) == '(' || GetCharAt(l) == '{'))
                        {
                            j = -1;
                            l = BraceMatch(l);

                            if (l == -1)
                                break;
                        }
                        else if (l+1 == GetLineEndPosition(LineFromPosition(matches[i])))
                            j = -1;
                    }
                }

                if (j == -1)
                    break;

                nPos = j;
            }
            else if (GetStyleAt(j) == wxSTC_NPRC_OPERATORS && (currentChar == '(' || currentChar == '{'))
            {
                j = BraceMatch(j);

                if (j == -1)
                    break;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This method is a simple helper for
/// AsynchActions to determine, which key should
/// not trigger the autoindention feature.
///
/// \param keycode int
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::isNoAutoIndentionKey(int keycode)
{
    return keycode == WXK_BACK
        || keycode == WXK_DELETE
        || keycode == WXK_TAB
        || keycode == WXK_SHIFT
        || keycode == WXK_CAPITAL
        || keycode == WXK_ALT
        || keycode == WXK_CONTROL
        || keycode == WXK_ESCAPE
        || keycode == WXK_LEFT
        || keycode == WXK_RIGHT
        || keycode == WXK_UP
        || keycode == WXK_DOWN
        || keycode == WXK_HOME
        || keycode == WXK_END;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether a
/// line has been syntactically wrapped around.
///
/// \param line int
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::isWrappedLine(int line)
{
    if (line <= 0)
        return false;

    wxString lastLine = GetLine(line-1);
    lastLine.erase(lastLine.find_last_not_of("\r\n\t ")+1);

    if (m_fileType == FILE_MATLAB)
        return lastLine.EndsWith("...");
    else if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        return lastLine.EndsWith("\\\\");

    return false;
}


/////////////////////////////////////////////////
/// \brief Performs asynchronous actions and is
/// called automatically.
///
/// \return void
///
/// The asynchronous actions include the following
/// items:
/// \li Automatic code indentation
/// \li Display and update of function call tip
/////////////////////////////////////////////////
void NumeReEditor::AsynchActions()
{
    if (!AutoCompActive()
        && getEditorSetting(SETTING_INDENTONTYPE)
        && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB || m_fileType == FILE_CPP)
        && !isNoAutoIndentionKey(m_nLastReleasedKey)
        && !HasSelection())
    {
        int nLine = GetCurrentLine();
        int offset = 0;

        // Probably the current line is also a parent line -> take the parent
        if (GetFoldLevel(nLine) & wxSTC_FOLDLEVELHEADERFLAG)
            offset--;

        // Get parent line
        int nParentline = GetFoldParent(nLine+offset);

        // if not found -> return
        if (nParentline == wxNOT_FOUND)
        {
            nParentline = std::max(nLine-10, 0);

            if (GetFoldParent(nParentline) != wxNOT_FOUND)
                nParentline = GetFoldParent(nParentline);

            // Do not use comments or syntactial wrapped lines
            // to determine the indentation level (if avoidable)
            while (nParentline > 0 && (isStyleType(STYLE_COMMENT, PositionFromLine(nParentline)) || isWrappedLine(nParentline)))
                nParentline--;
        }

        ApplyAutoIndentation(nParentline, nLine + 1);
    }

    HandleFunctionCallTip();
}


/////////////////////////////////////////////////
/// \brief Performs asynchronous evaluations and
/// is called automatically.
///
/// \return void
///
/// The asynchronous evaluations include the
/// following items:
/// \li static code analysis
/// \li update of sectioning markers
/////////////////////////////////////////////////
void NumeReEditor::AsynchEvaluations()
{
    //if (!this->AutoCompActive()
    //    && this->getEditorSetting(SETTING_INDENTONTYPE)
    //    && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB || m_fileType == FILE_CPP)
    //    && !isNoAutoIndentionKey(m_nLastReleasedKey)
    //    && !HasSelection())
    //    ApplyAutoIndentation(0, this->GetCurrentLine() + 1);


    if (getEditorSetting(SETTING_USEANALYZER))
        m_analyzer->run();

    markSections();
    markLocalVariables();
}


/////////////////////////////////////////////////
/// \brief Event handler for starting drag 'n drop.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnStartDrag(wxStyledTextEvent& event)
{
    wxString gtxt = event.GetDragText();
}


/////////////////////////////////////////////////
/// \brief Event handler for stopping drag 'n drop.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnDragOver(wxStyledTextEvent& event)
{
    event.SetDragAllowMove(true); // Necessary for file drag/drop
    event.SetDragResult(wxDragMove);
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Event handler for stopping drag 'n drop.
/// (Actually does nothing)
///
/// \param event wxStyledTextEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnDrop(wxStyledTextEvent& event)
{
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Event handler for moving while performing
/// a drag 'n drop operation.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnMouseMotion(wxMouseEvent& event)
{
    if (m_dragging)
        DoDragOver(event.GetX(), event.GetY(), wxDragMove);

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Jumps to the predefined template caret
/// position.
///
/// \param nStartPos int
/// \return void
///
/// This member function jumps the caret to the predefined
/// caret position (using a pipe "|") in the template and
/// removes the character at the position
/////////////////////////////////////////////////
void NumeReEditor::GotoPipe(int nStartPos)
{
    vector<int> vPos;

    // Try to find the pipe in the file
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
        vPos = m_search->FindAll("|", wxSTC_NSCR_OPERATORS, nStartPos, GetLastPosition(), false);

    // If nothting was found, try to find the pipe
    // in the install section
    if (m_fileType == FILE_NSCR && !vPos.size())
        vPos = m_search->FindAll("|", wxSTC_NSCR_INSTALL, nStartPos, GetLastPosition(), false);

    // Go to the pipe, if it was found, and remove
    // it
    if (vPos.size())
    {
        GotoPos(vPos.front());
        DeleteRange(vPos.front(), 1);
    }
    else
        GotoLine(nStartPos); // fallback solution
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
    int linenum = GetLineForMarkerOperation();
    AddBreakpoint(linenum);
    ResetRightClickLocation();
}


/////////////////////////////////////////////////
/// \brief Edits a breakpoint at the
/// right-clicked location.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnEditBreakpoint(wxCommandEvent& event)
{
    int linenum = GetLineForMarkerOperation();
    EditBreakpoint(linenum);
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
    int numBreakpoints = GetBreakpoints().GetCount();

    for (int i = 0; i < numBreakpoints; i++)
    {
        int markerHandle = m_breakpoints[i];
        this->MarkerDeleteHandle(markerHandle);
    }

    m_terminal->clearBreakpoints(GetFileNameAndPath().ToStdString());
    ResetRightClickLocation();
}


/////////////////////////////////////////////////
/// \brief Adds a bookmark at the selected line.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnAddBookmark(wxCommandEvent& event)
{
    int nLineNumber = GetLineForMarkerOperation();
    this->MarkerAdd(nLineNumber, MARKER_BOOKMARK);
    ResetRightClickLocation();
}


/////////////////////////////////////////////////
/// \brief Removes a bookmark from the selected line.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnRemoveBookmark(wxCommandEvent& event)
{
    int nLineNumber = GetLineForMarkerOperation();
    this->MarkerDelete(nLineNumber, MARKER_BOOKMARK);
}


/////////////////////////////////////////////////
/// \brief Removes all bookmarks from the whole
/// file.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Registers the passed procedure viewer.
///
/// \param viewer ProcedureViewer*
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::registerProcedureViewer(ProcedureViewer* viewer)
{
    m_procedureViewer = viewer;
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
    SetFirstVisibleLine(VisibleFromDocLine(linenumber) - m_options->GetDebuggerFocusLine());
    EnsureLineVisibility(linenumber);

    if (showMarker)
    {
        MarkerDeleteAll(MARKER_FOCUSEDLINE);
        MarkerAdd(linenumber, MARKER_FOCUSEDLINE);
    }
}


/////////////////////////////////////////////////
/// \brief Ensures that the selected line is
/// visible and not hidden bei either a code fold
/// or a explicit line hiding. Does not move the
/// cursor to the selected line.
///
/// \param line int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::EnsureLineVisibility(int line)
{
    EnsureVisible(line);

    // Unhide the lines, if the current line is part
    // of a hidden sectioon
    if (!GetLineVisible(line))
    {
        int nFirstLine = line-1;
        int nLastLine = line+1;

        // Find the first unhidden line
        while (!GetLineVisible(nFirstLine))
            nFirstLine--;

        // Find the last hidden line
        while (!GetLineVisible(nLastLine))
            nLastLine++;

        // Show the lines
        ShowLines(nFirstLine, nLastLine);

        // Delete the markers
        for (int i = nFirstLine; i < nLastLine; i++)
        {
            MarkerDelete(i, MARKER_HIDDEN);
            MarkerDelete(i, MARKER_HIDDEN_MARGIN);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Gets the line belonging to the last
/// right mouse click or the current line of the
/// caret.
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::GetLineForMarkerOperation()
{
    int lineNum = 0;

    if (m_lastRightClick.x < 0 || m_lastRightClick.y < 0)
        lineNum =  GetCurrentLine();
    else
    {
        int charpos = PositionFromPoint(m_lastRightClick);
        lineNum = LineFromPosition(charpos);
    }

    return lineNum;
}


/////////////////////////////////////////////////
/// \brief Invalidates the latest mouse right click
/// position.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::ResetRightClickLocation()
{
    m_lastRightClick.x = -1;
    m_lastRightClick.y = -1;
}


/////////////////////////////////////////////////
/// \brief Wrapper for the static code analyzer.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::AnalyseCode()
{
    m_analyzer->run();
}


/////////////////////////////////////////////////
/// \brief Finds the procedure definition and displays
/// it in the editor.
///
/// \param procedurename const wxString&
/// \return void
///
/// This private member function searches for the procedure
/// definition in the local or a global file (depending on
/// the namespace) opens the procedure and goes to the
/// corresponding position.
/////////////////////////////////////////////////
void NumeReEditor::FindAndOpenProcedure(const wxString& procedurename)
{
    if (!procedurename.length())
        return;

    vector<std::string> vPaths = m_terminal->getPathSettings();
    wxString pathname = procedurename;

    // check whether the procedure is an external procedure
    if (pathname.substr(0, 2) == "$'" && pathname[pathname.length()-1] == '\'')
    {
        if (!fileExists(pathname.substr(2, pathname.length()-3).ToStdString() + ".nprc"))
        {
            int ret = wxMessageBox(_guilang.get("GUI_DLG_PROC_NEXISTS_CREATE", procedurename.ToStdString()), _guilang.get("GUI_DLG_PROC_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);

            if (ret == wxYES)
                m_mainFrame->NewFile(FILE_NPRC, pathname.substr(2, pathname.length()-3));

            return;
        }
    }
    else
    {
        // Validate the procedure name for unwanted characters
        for (size_t i = 0; i < pathname.length(); i++)
        {
            if (!isalnum(pathname[i])
                && pathname[i] != '$'
                && pathname[i] != '_'
                && pathname[i] != '~')
            {
                wxMessageBox(_guilang.get("GUI_DLG_PROC_INVALIDCHARS", procedurename.ToStdString()), _guilang.get("GUI_DLG_PROC_INVALIDCHARS_HEADLINE"), wxCENTER | wxICON_ERROR | wxOK, this);
                return;
            }
        }
    }

    // Resolve the namespaces
    if (pathname.find("$thisfile~") != string::npos)
    {
        // Search for the procedure in the current file
        wxString name = pathname.substr(pathname.rfind('~') + 1);
        wxString procedureline;
        int nminpos = 0;
        int nmaxpos = GetLastPosition();

        // Search the procedure and check, whether it is uncommented
        while (nminpos < nmaxpos && FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) != -1)
        {
            nminpos = FindText(nminpos, nmaxpos, "procedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) + 1;

            // Check for comments
            if (isStyleType(STYLE_COMMENT_LINE, nminpos) || isStyleType(STYLE_COMMENT_BLOCK, nminpos))
                continue;

            procedureline = GetLine(LineFromPosition(nminpos));

            // If the line contains the necessary syntax elements
            // jump to it
            if (procedureline.find("$" + name) != string::npos && procedureline[procedureline.find_first_not_of(' ', procedureline.find("$" + name) + name.length() + 1)] == '(')
            {
                this->SetFocus();
                this->GotoLine(LineFromPosition(nminpos));
                this->SetFirstVisibleLine(VisibleFromDocLine(LineFromPosition(nminpos))-2);
                this->EnsureVisible(LineFromPosition(nminpos));
                return;
            }
        }

        // If it is not found, ask the user for creation
        int ret = wxMessageBox(_guilang.get("GUI_DLG_PROC_NEXISTS_CREATE", procedurename.ToStdString()), _guilang.get("GUI_DLG_PROC_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);

        if (ret != wxYES)
            return;

        // Get the template
        wxString proctemplate = getTemplateContent(name);

        // Add the template after the last line
        int nLastLine = this->GetLineCount();
        this->GotoLine(nLastLine);
        this->AddText("\n");
        this->AddText(proctemplate);

        // Replace the name in the template with the correct name
        // and goto the position
        nLastLine = FindText(this->PositionFromLine(nLastLine), this->GetLastPosition(), "procedure $" + name, wxSTC_FIND_MATCHCASE);
        this->GotoPipe(nLastLine);

        // Update the syntax highlighting and the analyzer state
        UpdateSyntaxHighlighting(true);
        m_analyzer->run();

        return;
    }
    else
        pathname = Procedure::nameSpaceToPath(pathname.ToStdString(), GetFilePath().ToStdString());

    wxArrayString pathnames;
    pathnames.Add(pathname + ".nprc");

    // If the file with this path exists, open it,
    // otherwise ask the user for creation
    if (!fileExists((pathname + ".nprc").ToStdString()))
    {
        int ret = wxMessageBox(_guilang.get("GUI_DLG_PROC_NEXISTS_CREATE", procedurename.ToStdString()), _guilang.get("GUI_DLG_PROC_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);

        if (ret != wxYES)
            return;

        m_mainFrame->NewFile(FILE_NPRC, pathname);
    }
    else
        m_mainFrame->OpenSourceFile(pathnames);
}


/////////////////////////////////////////////////
/// \brief Finds the included script and displays
/// it in the editor.
///
/// \param includename const wxString&
/// \return void
///
/// This private member function searches for the included
/// script as a global file and opens it, if it exists.
/////////////////////////////////////////////////
void NumeReEditor::FindAndOpenInclude(const wxString& includename)
{
    if (!includename.length())
        return;

    wxArrayString pathnames;
    pathnames.Add(includename);

    // If the file exists, open it, otherwise
    // ask the user for creation
    if (!fileExists((includename).ToStdString()))
    {
        int ret = wxMessageBox(_guilang.get("GUI_DLG_SCRIPT_NEXISTS_CREATE", includename.ToStdString()), _guilang.get("GUI_DLG_SCRIPT_NEXISTS_CREATE_HEADLINE"), wxCENTER | wxICON_WARNING | wxYES_NO, this);

        if (ret != wxYES)
            return;

        m_mainFrame->NewFile(FILE_NSCR, includename);
    }
    else
        m_mainFrame->OpenSourceFile(pathnames);
}


/////////////////////////////////////////////////
/// \brief Wrapper for the search controller.
///
/// \return vector<wxString>
///
/////////////////////////////////////////////////
vector<wxString> NumeReEditor::getProceduresInFile()
{
    return m_search->getProceduresInFile();
}


/////////////////////////////////////////////////
/// \brief Replaces the matches with a new symbol.
///
/// \param vMatches const vector<int>&
/// \param sSymbol const wxString&
/// \param sNewSymbol const wxString&
/// \return void
///
/// This member function replaces the found matches
/// with a new symbol.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Asks the user to supply a new name for
/// the symbol at the passed position and replaces
/// all occurences.
///
/// \param nPos int
/// \return void
///
/// This member function replaces the selected symbol
/// with a new name supplied by the user using a
/// text input dialog. This method also supplies the
/// needed user interaction.
/////////////////////////////////////////////////
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

    // Change the position to the start of the selected symbol
    // to correctly identify the style of the selected symbol
    nPos = this->WordStartPosition(nPos, true);

    // Prepare and show the text entry dialog, so that the
    // user may supply a new symbol name
    RenameSymbolsDialog textdialog(this, vRenameSymbolsChangeLog, wxID_ANY, _guilang.get("GUI_DLG_RENAMESYMBOLS"), sCurrentName);
    int retval = textdialog.ShowModal();

    if (retval == wxID_CANCEL)
        return;

    // Get the new symbol name and ensure that it
    // exists
    sNewName = textdialog.GetValue();

    if (!sNewName.length() || (!textdialog.replaceAfterCursor() && !textdialog.replaceBeforeCursor()))
        return;

    vRenameSymbolsChangeLog.push_back(sCurrentName + "\t" + sNewName);

    // The selected symbol is probably part of a procedure. If this is
    // the case, get the start and end position here
    if ((m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB) && !textdialog.replaceInWholeFile())
    {
        // Find the head of the current procedure
        nStartPos = m_search->FindCurrentProcedureHead(nPos);

        // Find the end of the current procedure depending on
        // the located head
        vector<int> vBlock = this->BlockMatch(nStartPos);
        if (vBlock.back() != wxSTC_INVALID_POSITION)
            nEndPos = vBlock.back();
    }

    // Adjust start and end position depending
    // on the flags of the renaming dialog
    if (!textdialog.replaceAfterCursor())
    {
        nEndPos = WordEndPosition(nPos, true);
    }
    else if (!textdialog.replaceBeforeCursor())
    {
        nStartPos = WordStartPosition(nPos, true);
    }

    // Ensure that the new symbol is not already in use
    vector<int> vNewNameOccurences = m_search->FindAll(sNewName, GetStyleAt(nPos), nStartPos, nEndPos);

    // It's possible to rename a standard function with
    // a new name. Check here against custom function
    // names
    if (!vNewNameOccurences.size() && isStyleType(STYLE_FUNCTION, nPos))
        vNewNameOccurences = m_search->FindAll(sNewName, wxSTC_NSCR_CUSTOM_FUNCTION, nStartPos, nEndPos);

    // MATLAB-specific fix, because the MATLAB lexer
    // does know custom defined functions
    if (!vNewNameOccurences.size() && m_fileType == FILE_MATLAB && (isStyleType(STYLE_FUNCTION, nPos) || isStyleType(STYLE_COMMAND, nPos)))
        vNewNameOccurences = m_search->FindAll(sNewName, wxSTC_MATLAB_IDENTIFIER, nStartPos, nEndPos);

    // If the vector size is non-zero, this symbol is
    // already in use
    if (vNewNameOccurences.size())
    {
        // Allow the user to cancel the replacement
        if (wxMessageBox(_guilang.get("GUI_DLG_RENAMESYMBOLS_ALREADYINUSE_WARNING"), _guilang.get("GUI_DLG_RENAMESYMBOLS_ALREADYINUSE"), wxCENTER | wxOK | wxCANCEL | wxICON_EXCLAMATION, this) == wxCANCEL)
            return;
    }

    // Gather all operations into one undo step
    this->BeginUndoAction();

    // Perform the renaming of symbols
    this->ReplaceMatches(m_search->FindAll(sCurrentName, this->GetStyleAt(nPos), nStartPos, nEndPos, textdialog.replaceInComments()), sCurrentName, sNewName);
    this->EndUndoAction();
}


/////////////////////////////////////////////////
/// \brief Extracts the marked selection into a
/// new procedure.
///
/// \return void
///
/// This member function extracts a selected code
/// section into a new procedure. The interface
/// is created depending upon the used variables
/// inside of the selected block.
/////////////////////////////////////////////////
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

    std::list<wxString> lInputTokens;
    std::list<wxString> lOutputTokens;

    std::set<std::string> sArgumentListSet;
    std::set<std::string> sMatlabReturnListSet;


    // If we have a procedure file, consider scoping
    if (m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB)
    {
        nCurrentBlockStart = m_search->FindCurrentProcedureHead(nStartPos);

        std::string sArgumentList = getFunctionArgumentList(LineFromPosition(nCurrentBlockStart)).ToStdString();

        // Split the argument list into single tokens
        while (sArgumentList.length())
        {
            std::string sCurrentArg = getNextArgument(sArgumentList, true);

            // Remove possible default values
            if (sCurrentArg.find_first_of(" =") != std::string::npos)
                sCurrentArg.erase(sCurrentArg.find_first_of(" ="));

            if (sCurrentArg.find('&') != std::string::npos)
                sCurrentArg.erase(sCurrentArg.find('&'), 1);

            StripSpaces(sCurrentArg);

            sArgumentListSet.insert(sCurrentArg);
        }

        // In the MATLAB case, get the return list of
        // the function
        if (m_fileType == FILE_MATLAB)
        {
            std::string sReturnList = getMatlabReturnList(LineFromPosition(nCurrentBlockStart)).ToStdString();

            // Split the return list into single tokens
            while (sReturnList.length())
            {
                sMatlabReturnListSet.insert(getNextArgument(sReturnList, true));
            }
        }

        // Ensure that the end of the current block exists
        std::vector<int> vBlock = BlockMatch(nCurrentBlockStart);

        if (vBlock.back() != wxSTC_INVALID_POSITION && vBlock.back() > nEndPos)
            nCurrentBlockEnd = vBlock.back();

        // Increment the starting line to omit the argument list
        // as variable occurence source. Those will be detected
        // by comparing with the splitted argument list
        nCurrentBlockStart = PositionFromLine(LineFromPosition(nCurrentBlockStart)+1);
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
            std::vector<int> vMatch = m_search->FindAll(sCurrentToken, this->GetStyleAt(i), nCurrentBlockStart, nCurrentBlockEnd);

            if (vMatch.size())
            {
                // Determine, whether the token is used before
                // or afer the current section
                if (vMatch.front() < nStartPos
                    || (sArgumentListSet.size() && sArgumentListSet.find(sCurrentToken.ToStdString()) != sArgumentListSet.end()))
                    lInputTokens.push_back(sCurrentToken);

                if (vMatch.back() > nEndPos
                    && vMatch.front() < nStartPos
                    && IsModifiedInSection(nStartPos, nEndPos, sCurrentToken, vMatch))
                    lOutputTokens.push_back(sCurrentToken);
                else if (vMatch.back() > nEndPos
                         && vMatch.front() >= nStartPos
                         && sArgumentListSet.find(sCurrentToken.ToStdString()) == sArgumentListSet.end())
                    lOutputTokens.push_back(sCurrentToken);
                else if (sMatlabReturnListSet.size()
                         && sMatlabReturnListSet.find(sCurrentToken.ToStdString()) != sMatlabReturnListSet.end()
                         && IsModifiedInSection(nStartPos, nEndPos, sCurrentToken, vMatch))
                    lOutputTokens.push_back(sCurrentToken);
            }

            i += sCurrentToken.length();
        }
        else if (isStyleType(STYLE_FUNCTION, i) && m_fileType == FILE_MATLAB)
        {
            // Matlab specials
            //
            // Get the token name
            wxString sCurrentToken = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

            // Ignore MATLAB structure fields
            if (GetCharAt(WordStartPosition(i, true)-1) == '.')
                continue;

            // Ignore functions, which are not part of any argument list;
            // these are most probably actual functions
            if ((!sMatlabReturnListSet.size() || sMatlabReturnListSet.find(sCurrentToken.ToStdString()) == sMatlabReturnListSet.end())
                && (!sArgumentListSet.size() || sArgumentListSet.find(sCurrentToken.ToStdString()) == sArgumentListSet.end()))
                continue;

            // Find all occurences
            std::vector<int> vMatch = m_search->FindAll(sCurrentToken, this->GetStyleAt(i), nCurrentBlockStart, nCurrentBlockEnd);

            if (vMatch.size())
            {
                // Determine, whether the token is used before
                // or afer the current section
                if (sArgumentListSet.size() && sArgumentListSet.find(sCurrentToken.ToStdString()) != sArgumentListSet.end())
                    lInputTokens.push_back(sCurrentToken);

                if (sMatlabReturnListSet.size() && sMatlabReturnListSet.find(sCurrentToken.ToStdString()) != sMatlabReturnListSet.end() && IsModifiedInSection(nStartPos, nEndPos, sCurrentToken, vMatch))
                    lOutputTokens.push_back(sCurrentToken);
            }

            i += sCurrentToken.length();
        }
        else if (isStyleType(STYLE_DATAOBJECT, i))
        {
            // Tables
            //
            // Get the token name
            wxString sCurrentToken = GetTextRange(WordStartPosition(i, true), WordEndPosition(i, true));

            // Find all occurences
            std::vector<int> vMatch = m_search->FindAll(sCurrentToken, this->GetStyleAt(i), nCurrentBlockStart, nCurrentBlockEnd);

            if (vMatch.size())
            {
                // Determine, whether the token is used before
                // or after the current section
                if (vMatch.front() < nStartPos
                    || (sArgumentListSet.find(sCurrentToken.ToStdString() + "()") != sArgumentListSet.end()
                        && GetStyleAt(i) == wxSTC_NSCR_CUSTOM_FUNCTION)
                    || (sArgumentListSet.find(sCurrentToken.ToStdString() + "{}") != sArgumentListSet.end()
                        && GetStyleAt(i) == wxSTC_NSCR_CLUSTER))
                {
                    if (GetStyleAt(i) == wxSTC_NSCR_CLUSTER)
                        lInputTokens.push_back(sCurrentToken + "{}");
                    else
                        lInputTokens.push_back(sCurrentToken + "()");
                }

                if (vMatch.back() > nEndPos
                    && IsModifiedInSection(nStartPos, nEndPos, sCurrentToken, vMatch))
                {
                    if (GetStyleAt(i) == wxSTC_NSCR_CLUSTER)
                        lOutputTokens.push_back(sCurrentToken + "{}");
                    else
                        lOutputTokens.push_back(sCurrentToken + "()");
                }
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

    // If the output list is larger than one element
    // and we're extracting NumeRe code, we have to ensure
    // that it does not contain a table or a cluster, because
    // this mixtures are not possible
    if (lOutputTokens.size() > 1 && (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC))
    {
        auto iter = lOutputTokens.begin();

        while (iter != lOutputTokens.end())
        {
            if ((*iter).find("()") != std::string::npos
                || (*iter).find("{}") != std::string::npos)
            {
                // We have a table or a cluster as a mixture
                auto inputIter = std::find(lInputTokens.begin(), lInputTokens.end(), *iter);

                if (inputIter != lInputTokens.end())
                    *inputIter += "&";
                else
                    lInputTokens.push_back(*iter + "&");

                iter = lOutputTokens.erase(iter);
                continue;
            }

            ++iter;
        }
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


/////////////////////////////////////////////////
/// \brief Creates a new procedure from the analysed
/// code section.
///
/// \param nStartPos int
/// \param nEndPos int
/// \param sInputList const wxString&
/// \param sOutputList const wxString
/// \return void
///
/// This member function is used to create a new
/// procedure from the analysed code section (Done
/// in \c AbstrahizeSection()). The new procedure
/// will be displayed in a new window.
/////////////////////////////////////////////////
void NumeReEditor::CreateProcedureFromSection(int nStartPos, int nEndPos, const wxString& sInputList, const wxString sOutputList)
{
    // Creata a new window and a new editor
    ViewerFrame* copyFrame = new ViewerFrame(m_mainFrame, _guilang.get("GUI_REFACTORING_COPYWINDOW_HEAD"), wxFRAME_FLOAT_ON_PARENT);
    NumeReEditor* edit = new NumeReEditor(m_mainFrame, m_options, copyFrame, wxID_ANY, _syntax, m_terminal, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
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
        wxString sCodeFragment = GetTextRange(nStartPos, nEndPos);
        size_t nLastChar = sCodeFragment.find_last_not_of("\r\n \t");
        bool hasReturn = false;

        if (nLastChar != std::string::npos)
            hasReturn = sCodeFragment.find("return", sCodeFragment.find_last_of("\r\n", nLastChar)) != std::string::npos;

        edit->AddText(sCodeFragment + "\r\n");

        // Write the output list
        if (sOutputList.length())
        {
            edit->AddText("\t## " + _guilang.get("GUI_REFACTORING_RETURNVALUES") + "\r\n");

            if (sOutputList.find(',') != string::npos)
                edit->AddText("return {" + sOutputList + "};\r\n");
            else
                edit->AddText("return " + sOutputList + ";\r\n");
        }
        else if (!hasReturn)
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
    copyFrame->SetIcon(m_mainFrame->getStandardIcon());
    copyFrame->Show();
    copyFrame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief Determines, whether \c sToken is modified
/// in the section.
///
/// \param nSectionStart int
/// \param nSectionEnd int
/// \param sToken const wxString&
/// \param vMatch const vector<int>&
/// \return bool
///
/// This member function determines, whether a
/// string token, which corresponds to a variable,
/// is semantically modified in the code section
/// (i.e. overwritten). This is used for variables,
/// which are both input and possible output.
/////////////////////////////////////////////////
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

        // Ignore dynamic structure field accesses in MATLAB
        if (isStyleType(STYLE_OPERATOR, vMatch[i]-1)
            && (GetCharAt(vMatch[i]-1) == '.' || (GetCharAt(vMatch[i]-2) == '.' && GetCharAt(vMatch[i]-1) == '(')))
            continue;

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
                // MATLAB struct fix
                while (isStyleType(STYLE_IDENTIFIER, j+1))
                    j++;
            }
            else if (isStyleType(STYLE_OPERATOR, j) && GetCharAt(j) == '.' && isStyleType(STYLE_FUNCTION, j+1))
            {
                // MATLAB struct fix
                while (isStyleType(STYLE_FUNCTION, j+1))
                    j++;
            }
            else if (isStyleType(STYLE_OPERATOR, j) && GetCharAt(j) == '.' && GetCharAt(j+1) == '(')
            {
                // MATLAB struct fix
                // Jump over parentheses
                j = BraceMatch(j+1);
                if (j == wxSTC_INVALID_POSITION)
                    return false;
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


/////////////////////////////////////////////////
/// \brief Gets the argument list of a procedure.
///
/// \param nFunctionStartLine int
/// \return wxString
///
/// This private member function extracts the argument
/// list from procedures, which is necessary
/// for correct function extraction.
/////////////////////////////////////////////////
wxString NumeReEditor::getFunctionArgumentList(int nFunctionStartLine)
{
    // Get the complete line
    wxString sReturn = GetLine(nFunctionStartLine);

    // Ensure that the line contains the keyword "function"
    // or "procedure", respectively
    if ((sReturn.find("function ") == string::npos && m_fileType == FILE_MATLAB)
        || (sReturn.find("procedure ") == string::npos && m_fileType == FILE_NPRC))
        return "";

    // Ensure that the line contains opening and
    // closing parentheses
    if (sReturn.find('(') == string::npos || sReturn.find(')') == string::npos)
        return "";

    // Extract the function argument list
    sReturn.erase(0, sReturn.find('(')+1);
    sReturn.erase(sReturn.rfind(')'));

    // Return the argument list
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief Gets the return list of a MATLAB function.
///
/// \param nMatlabFunctionStartLine int
/// \return wxString
///
/// This private member function extracts the return
/// list from MATLAB functions, which is necessary
/// for correct function extraction.
/////////////////////////////////////////////////
wxString NumeReEditor::getMatlabReturnList(int nMatlabFunctionStartLine)
{
    // Get the complete line
    wxString sReturn = GetLine(nMatlabFunctionStartLine);

    // Ensure that the line contains the keyword "function"
    if (sReturn.find("function") == string::npos)
        return "";

    // Ensure that the line contains an equal sign
    if (sReturn.find('=') == string::npos)
        return "";

    // Remove the keyword part and the function declaration itself
    sReturn.erase(0, sReturn.find("function")+8);
    sReturn.erase(sReturn.find('='));

    // Remove surrounding brackets
    if (sReturn.find('[') != string::npos && sReturn.find(']') != string::npos)
    {
        sReturn.erase(0, sReturn.find('[')+1);
        sReturn.erase(sReturn.rfind(']'));
    }

    // Return the return list
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of a template file.
///
/// \param sFileName const wxString&
/// \return wxString
///
/// The contents of a template file are written to
/// memory and the place holders are filled with
/// target file name and timestamp.
/////////////////////////////////////////////////
wxString NumeReEditor::getTemplateContent(const wxString& sFileName)
{
    wxString template_file, template_type, timestamp;

    template_type = "tmpl_procedure.nlng";
    timestamp = getTimeStamp(false);

    // Get the file's contents
    if (m_terminal->getKernelSettings().useCustomLangFiles() && wxFileExists(m_mainFrame->getProgramFolder() + "\\user\\lang\\" + template_type))
        m_mainFrame->GetFileContents(m_mainFrame->getProgramFolder() + "\\user\\lang\\" + template_type, template_file, template_type);
    else
        m_mainFrame->GetFileContents(m_mainFrame->getProgramFolder() + "\\lang\\" + template_type, template_file, template_type);

    // Replace filenames
    while (template_file.find("%%1%%") != string::npos)
        template_file.replace(template_file.find("%%1%%"), 5, sFileName);

    // Replace timestamps
    while (template_file.find("%%2%%") != string::npos)
        template_file.replace(template_file.find("%%2%%"), 5, timestamp);

    return template_file;
}


/////////////////////////////////////////////////
/// \brief Ensures that the current position
/// follows all rules to be a valid
/// autocompletion match.
///
/// \param nPos int
/// \param findAll bool
/// \param searchMethod bool
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::isValidAutoCompMatch(int nPos, bool findAll, bool searchMethod)
{
    // Ignore self matches
    if (nPos <= GetCurrentPos() && WordEndPosition(nPos+1, true) >= GetCurrentPos())
        return false;

    // Ignore matches not at the starting position
    if (nPos != WordStartPosition(nPos, true))
        return false;

    // Check style conditions
    if (findAll)
        return true;
    else if (!isStyleType(STYLE_COMMENT, nPos)
             && !isStyleType(STYLE_STRING, nPos)
             && (!searchMethod || GetCharAt(nPos-1) == '.'))
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief Generates an autocompletion list based
/// upon the file's contents.
///
/// \param wordstartpos int
/// \param currpos int
/// \param sPreDefList std::string
/// \return wxString
///
/// The function combines the passed predefined
/// list of autocompletioon possibities (aka the
/// actual syntax autocompletion list) with possible
/// completion candidates found in the text of the
/// current file.
/////////////////////////////////////////////////
wxString NumeReEditor::generateAutoCompList(int wordstartpos, int currpos, std::string sPreDefList)
{
    std::string sScopedList;
    bool useSmartSense = m_options->getSetting(SETTING_B_SMARTSENSE).active();
    bool searchMethod = GetCharAt(wordstartpos) == '.';
    wxString wordstart = GetTextRange(searchMethod ? wordstartpos+1 : wordstartpos, currpos);
    std::pair<int, int> context = std::make_pair(0, GetLineCount());

    if (useSmartSense)
        context = getCurrentContext(LineFromPosition(currpos));

    size_t nPos = PositionFromLine(context.first);
    bool findAll = !useSmartSense
                || isStyleType(STYLE_COMMENT, wordstartpos)
                || isStyleType(STYLE_STRING, wordstartpos);

    int searchFlags = wxSTC_FIND_WORDSTART;

    // If we use SmartSense, then we'll using RegEx and also
    // accept leading underscores as possible matches
    if (useSmartSense && !searchMethod)
    {
        wordstart.insert(0, "\\<_*");
        searchFlags = wxSTC_FIND_REGEXP;
    }

    // Find every occurence of the current word start
    // and store the possible completions in the map
    while (wordstart.length() && (nPos = FindText(nPos, GetLineEndPosition(context.second), wordstart, searchFlags)) != std::string::npos)
    {
        if (isValidAutoCompMatch(nPos, findAll, searchMethod))
        {
            std::string sMatch = GetTextRange(nPos, WordEndPosition(nPos + 1, true)).ToStdString();
            std::string sFillUp;

            // Append the needed opening parentheses, if the completed
            // objects are data objects or functions
            if (isStyleType(STYLE_CUSTOMFUNCTION, nPos))
                sFillUp = "(?" + toString((int)NumeReSyntax::SYNTAX_TABLE);
            else if (isStyleType(STYLE_DATAOBJECT, nPos))
                sFillUp = "{?" + toString((int)NumeReSyntax::SYNTAX_CLUSTER);
            else if (isStyleType(STYLE_IDENTIFIER, nPos))
                sFillUp = "?" + toString((int)NumeReSyntax::SYNTAX_STD);

            sScopedList += sMatch + sFillUp + " ";
        }

        nPos++;
    }

    // Get all declares if we do not start on the first line
    if (useSmartSense && context.first != 0 && !searchMethod)
    {
        // Find the declares in front of the current autocompletion part
        std::vector<int> vMatches = m_search->FindAll("declare", wxSTC_NSCR_COMMAND, 0, PositionFromLine(context.first), false);

        for (int pos : vMatches)
        {
            int lineEnd = GetLineEndPosition(LineFromPosition(pos));

            // Find every occurence of the current word start
            // and store the possible completions in the map
            while ((pos = FindText(pos, lineEnd, wordstart, searchFlags)) != wxNOT_FOUND)
            {
                if (isValidAutoCompMatch(pos, findAll, searchMethod))
                {
                    std::string sMatch = GetTextRange(pos, WordEndPosition(pos + 1, true)).ToStdString();
                    std::string sFillUp;

                    // Append the needed opening parentheses, if the completed
                    // objects are data objects or functions
                    if (isStyleType(STYLE_CUSTOMFUNCTION, pos))
                        sFillUp = "(?" + toString((int)NumeReSyntax::SYNTAX_TABLE);
                    else if (isStyleType(STYLE_DATAOBJECT, pos))
                        sFillUp = "{?" + toString((int)NumeReSyntax::SYNTAX_CLUSTER);
                    else if (isStyleType(STYLE_IDENTIFIER, pos))
                        sFillUp = "?" + toString((int)NumeReSyntax::SYNTAX_STD);

                    sScopedList += sMatch + sFillUp + " ";
                }

                pos++;
            }
        }
    }

    // Use the static member function from NumeReSyntax to combine those two lists
    return NumeReSyntax::mergeAutoCompleteLists(sPreDefList, sScopedList);
}


/////////////////////////////////////////////////
/// \brief Highlights all word occurences permanently.
///
/// \param event wxCommandEvent&
/// \return void
///
/// This member function highlights the clicked word
/// permanently or removes the highlighting, if the
/// word was already selected
/////////////////////////////////////////////////
void NumeReEditor::OnDisplayVariable(wxCommandEvent& event)
{
    // Clear the highlighting state and prepare the
    // colors and style for the next highlighting
    long int maxpos = this->GetLastPosition();
    SetIndicatorCurrent(HIGHLIGHT);
    IndicatorClearRange(0, maxpos);

    size_t nPos = 0;
    size_t nCurr = 0;
    vector<size_t> vSelectionList;

    // If the current clicked word is already
    // highlighted, then simply clear out the
    // buffer string and return
    if (m_watchedString == m_clickedWord)
    {
        m_watchedString.clear();
        return;
    }

    // Update the buffer with the new clicked word
    m_watchedString = m_clickedWord;

    // Search for all occurences of the current clicked
    // word in the document and store them
    while ((nPos = this->FindText(nCurr, maxpos, m_clickedWord, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != string::npos)
    {
        vSelectionList.push_back(nPos);
        nCurr = nPos +  m_clickedWordLength;
    }

    // Apply the indicator to all found occurences
    for (size_t i = 0; i < vSelectionList.size(); i++)
    {
        this->IndicatorFillRange(vSelectionList[i], m_clickedWordLength);
    }
}


/////////////////////////////////////////////////
/// \brief Triggers the main frame to show the
/// documentation viewer concerning the selected
/// command.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnHelpOnSelection(wxCommandEvent& event)
{
    m_mainFrame->ShowHelp(m_clickedWord.ToStdString());
}


/////////////////////////////////////////////////
/// \brief Private event handler function for
/// finding the procedure definition.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFindProcedure(wxCommandEvent& event)
{
    if (!m_clickedProcedure.length())
        return;

    FindAndOpenProcedure(m_clickedProcedure);
}


/////////////////////////////////////////////////
/// \brief Global event handler function for
/// finding the procedure definition.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFindProcedureFromMenu()
{
    if (!isNumeReFileType() || GetStyleAt(GetCurrentPos()) != wxSTC_NSCR_PROCEDURES)
        return;

    m_search->FindMarkedProcedure(GetCurrentPos());
    FindAndOpenProcedure(m_clickedProcedure);
}


/////////////////////////////////////////////////
/// \brief Private event handler function for
/// finding the included script.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFindInclude(wxCommandEvent& event)
{
    if (!m_clickedInclude.length())
        return;

    FindAndOpenInclude(m_clickedInclude);
}


/////////////////////////////////////////////////
/// \brief Global event handler function for
/// finding the included script.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFindIncludeFromMenu()
{
    if (!isNumeReFileType() || GetStyleAt(GetCurrentPos()) != wxSTC_NSCR_INCLUDES)
        return;

    m_search->FindMarkedInclude(GetCurrentPos());
    FindAndOpenInclude(m_clickedInclude);
}


/////////////////////////////////////////////////
/// \brief Changes the letters in the selection.
///
/// \param event wxCommandEvent& Using \c GetId
/// method to determine upper or lowercase
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnChangeCase(wxCommandEvent& event)
{
    if (!HasSelection())
        return;

    // Get selection positions
    int nFirstPos = GetSelectionStart();
    int nLastPos = GetSelectionEnd();

    // Change the case
    if (event.GetId() == ID_UPPERCASE)
        Replace(nFirstPos, nLastPos, toUpperCase(GetSelectedText().ToStdString()));
    else
        Replace(nFirstPos, nLastPos, toLowerCase(GetSelectedText().ToStdString()));
}


/////////////////////////////////////////////////
/// \brief Event wrapper for \c FoldCurrentBlock.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFoldCurrentBlock(wxCommandEvent& event)
{
    FoldCurrentBlock(this->LineFromPosition(this->PositionFromPoint(m_lastRightClick)));
}


/////////////////////////////////////////////////
/// \brief Private event handling function for
/// hiding the selection.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnHideSelection(wxCommandEvent& event)
{
    int nFirstLine = LineFromPosition(GetSelectionStart());
    int nLastLine = LineFromPosition(GetSelectionEnd());

    HideLines(nFirstLine, nLastLine);

    MarkerAdd(nFirstLine-1, MARKER_HIDDEN);
    MarkerAdd(nFirstLine-1, MARKER_HIDDEN_MARGIN);
}


/////////////////////////////////////////////////
/// \brief Global event handling function to unhide
/// all lines from the main frame's menu.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnUnhideAllFromMenu()
{
    if (GetAllLinesVisible())
        return;

    ShowLines(0, LineFromPosition(GetLastPosition()));

    MarkerDeleteAll(MARKER_HIDDEN);
    MarkerDeleteAll(MARKER_HIDDEN_MARGIN);
}


/////////////////////////////////////////////////
/// \brief Event wrapper for \c RenameSymbols.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnRenameSymbols(wxCommandEvent& event)
{
    this->RenameSymbols(this->PositionFromPoint(m_lastRightClick));
}


/////////////////////////////////////////////////
/// \brief Global wrapper for \c RenameSymbols.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnRenameSymbolsFromMenu()
{
    int charpos = GetCurrentPos();

    if (this->isStyleType(STYLE_DEFAULT, charpos) || this->isStyleType(STYLE_IDENTIFIER, charpos) || this->isStyleType(STYLE_DATAOBJECT, charpos) || this->isStyleType(STYLE_FUNCTION, charpos))
        this->RenameSymbols(charpos);
}


/////////////////////////////////////////////////
/// \brief Event wrapper for \c fixSymbolName.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFixTypes(wxCommandEvent& event)
{
    fixSymbolName(this->PositionFromPoint(m_lastRightClick));
}


/////////////////////////////////////////////////
/// \brief Private event handler for extracting
/// the selected section.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnAbstrahizeSection(wxCommandEvent& event)
{
    this->AbstrahizeSection();
}


/////////////////////////////////////////////////
/// \brief Global event handler for extracting
/// the selected section.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnAbstrahizeSectionFromMenu()
{
    if (HasSelection())
        this->AbstrahizeSection();
}


/////////////////////////////////////////////////
/// \brief OnMenu event handler.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_MENU_COPY:
            Copy();
            break;
        case ID_MENU_CUT:
            Cut();
            break;
        case ID_MENU_PASTE:
            Paste();
            break;
    }
}


/////////////////////////////////////////////////
/// \brief On MenuEvent handler for the transpose
/// functionality of the editor.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnTranspose()
{
    int nFirstLine = 0;
    int nLastLine = -1;

    if (HasSelection())
    {
        nFirstLine = LineFromPosition(GetSelectionStart());
        nLastLine = LineFromPosition(GetSelectionEnd());

        // Decrement, if first position is selected
        if (GetSelectionEnd() == PositionFromLine(nLastLine))
            nLastLine--;
    }

    Transpose(nFirstLine, nLastLine);
}


/////////////////////////////////////////////////
/// \brief On MenuEvent handler for the extract
/// as HTML functionality of the editor.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnExtractAsHTML()
{
    int nFirstLine = 0;
    int nLastLine = -1;

    if (HasSelection())
    {
        nFirstLine = LineFromPosition(GetSelectionStart());
        nLastLine = LineFromPosition(GetSelectionEnd());

        // Decrement, if first position is selected
        if (GetSelectionEnd() == PositionFromLine(nLastLine))
            nLastLine--;
    }

    wxString sHtml = ExtractAsHTML(nFirstLine, nLastLine);

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(sHtml));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief On MenuEvent handler for the extract
/// as formatted text functionality of the editor.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnExtractFormatted()
{
    int nFirstLine = 0;
    int nLastLine = -1;

    if (HasSelection())
    {
        nFirstLine = LineFromPosition(GetSelectionStart());
        nLastLine = LineFromPosition(GetSelectionEnd());

        // Decrement, if first position is selected
        if (GetSelectionEnd() == PositionFromLine(nLastLine))
            nLastLine--;
    }

    wxString sHtml = ExtractAsHTML(nFirstLine, nLastLine);

    sHtml.Replace("  ", "&nbsp;&nbsp;");
    sHtml.Replace("\r\n", "\n");
    sHtml.Replace("\n\n", "<br>&nbsp;<br>");
    sHtml.Replace("\n", "<br>");

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxHTMLDataObject(sHtml));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief Displays the duplicated code dialog.
///
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::InitDuplicateCode()
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC || m_fileType == FILE_MATLAB || m_fileType == FILE_CPP)
    {
        m_duplicateCode = new DuplicateCodeDialog(this, "NumeRe: " + _guilang.get("GUI_DUPCODE_TITLE") + " [" + this->GetFilenameString() + "]");
        m_duplicateCode->SetIcon(m_mainFrame->getStandardIcon());
        m_duplicateCode->Show();
        m_duplicateCode->SetFocus();
        m_duplicateCode->Refresh();
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Wrapper for \c detectCodeDuplicates.
///
/// \param nDuplicateFlag int
/// \param nNumDuplicatedLines int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::OnFindDuplicateCode(int nDuplicateFlag, int nNumDuplicatedLines)
{
    detectCodeDuplicates(0, this->LineFromPosition(this->GetLastPosition()), nDuplicateFlag, nNumDuplicatedLines);
}


/////////////////////////////////////////////////
/// \brief Thread event handler function for the
/// duplicated code detection functionality.
///
/// \param event wxThreadEvent&
/// \return void
///
/// This function simply updates the gauge in the
/// duplicated code dialog and passes the results,
/// once the detection is finished.
/////////////////////////////////////////////////
void NumeReEditor::OnThreadUpdate(wxThreadEvent& event)
{
    if (m_nProcessValue < 100)
    {
        // Update the gauge
        if (m_duplicateCode && m_duplicateCode->IsShown())
            m_duplicateCode->SetProgress(m_nProcessValue);
    }
    else
    {
        // Update the gauge
        if (m_duplicateCode && m_duplicateCode->IsShown())
            m_duplicateCode->SetProgress(100);
        else
        {
            vDuplicateCodeResults.clear();
            return;
        }

        // Pass the results
        wxCriticalSectionLocker lock(m_editorCS);
        m_duplicateCode->SetResult(vDuplicateCodeResults);
        vDuplicateCodeResults.clear();
    }

}


/////////////////////////////////////////////////
/// \brief Highlights differences between two blocks
/// of code.
///
/// \param nStart1 int
/// \param nEnd1 int
/// \param nStart2 int
/// \param nEnd2 int
/// \param nSelectionLine int
/// \return void
///
/// This function highlights the differences between
/// two blocks of code including the wordwise
/// differences in the lines.
///
/////////////////////////////////////////////////
void NumeReEditor::IndicateDuplicatedLine(int nStart1, int nEnd1, int nStart2, int nEnd2, int nSelectionLine)
{
    MarkerDeleteAll(MARKER_DUPLICATEINDICATOR_ONE);
    MarkerDeleteAll(MARKER_DUPLICATEINDICATOR_TWO);

    SetIndicatorCurrent(HIGHLIGHT_DIFFERENCES);
    IndicatorClearRange(0, GetLastPosition());

    SetIndicatorCurrent(HIGHLIGHT_DIFFERENCE_SOURCE);
    IndicatorClearRange(0, GetLastPosition());

    if (nStart1 == -1 && nStart2 == -1 && nEnd1 == -1 && nEnd2 == -1)
        return;

    // Mark section 1
    for (int i = nStart1; i <= nEnd1; i++)
        MarkerAdd(i, MARKER_DUPLICATEINDICATOR_ONE);

    // Mark section 2
    for (int i = nStart2; i <= nEnd2; i++)
        MarkerAdd(i, MARKER_DUPLICATEINDICATOR_TWO);

    // Determine the wordwise differences
    map<int, int> mDifferences = getDifferences(nStart1, nEnd1, nStart2, nEnd2);

    // Mark the wordwise differences
    for (auto iter = mDifferences.begin(); iter != mDifferences.end(); ++iter)
    {
        if ((iter->first) < 0)
            SetIndicatorCurrent(HIGHLIGHT_DIFFERENCE_SOURCE);
        else
            SetIndicatorCurrent(HIGHLIGHT_DIFFERENCES);

        IndicatorFillRange(abs(iter->first), iter->second);
    }

    ScrollToLine(nSelectionLine);
}


static bool isEqualIgnoreWhitespace(wxString word1, wxString word2)
{
    word1.Trim();
    word2.Trim();

    return word2.Trim(false) == word1.Trim(false);
}


/////////////////////////////////////////////////
/// \brief This function returns true, if a
/// passed word corresponds to a control flow
/// statement block start.
///
/// \param sWord const wxString&
/// \param allowIntermediate bool
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::isBlockStart(const wxString& sWord, bool allowIntermediate)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        int id = getBlockID(sWord);

        if (id != wxNOT_FOUND)
        {
            if (isEqualIgnoreWhitespace(vBlockDefs[id].startWord, sWord)
                || (allowIntermediate && (isEqualIgnoreWhitespace(sWord, vBlockDefs[id].middleWord1) || isEqualIgnoreWhitespace(sWord, vBlockDefs[id].middleWord2))))
                return id;
        }
    }
    else if (m_fileType == FILE_MATLAB)
        return (sWord == "if"
                || sWord == "for"
                || sWord == "do"
                || sWord == "while"
                || sWord == "classdef"
                || sWord == "properties"
                || sWord == "function"
                || sWord == "methods"
                || sWord == "try"
                || sWord == "switch"
                || (allowIntermediate && (sWord == "elseif"
                                          || sWord == "else"
                                          || sWord == "case"
                                          || sWord == "otherwise"
                                          || sWord == "catch"))) ? 1 : wxNOT_FOUND;

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief This function returns true, if a
/// passed word corresponds to a control flow
/// statement block end.
///
/// \param sWord const wxString&
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::isBlockEnd(const wxString& sWord)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        int id = getBlockID(sWord);

        if (id != wxNOT_FOUND)
        {
            if (isEqualIgnoreWhitespace(vBlockDefs[id].endWord, sWord))
                return id;
        }
    }
    else if (m_fileType == FILE_MATLAB)
        return sWord == "end" ? 1 : wxNOT_FOUND;

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief This function returns, whether a word
/// matches to a block middle statement (e.g.
/// elseif).
///
/// \param sWord const wxString&
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::isBlockMiddle(const wxString& sWord)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        int id = getBlockID(sWord);

        if (id != wxNOT_FOUND
            && (isEqualIgnoreWhitespace(vBlockDefs[id].middleWord1, sWord) || isEqualIgnoreWhitespace(vBlockDefs[id].middleWord2, sWord)))
            return id;
    }
    else if (m_fileType == FILE_MATLAB)
        return (sWord == "elseif" || sWord == "else" || sWord == "case" || sWord == "otherwise" || sWord == "catch") ? 1 : wxNOT_FOUND;

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief This function returns, whether a block
/// has block middle statements.
///
/// \param sWord const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::hasBlockMiddle(const wxString& sWord)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        int id = getBlockID(sWord);

        if (id != wxNOT_FOUND && vBlockDefs[id].middleWord1.length())
            return true;
    }
    else if (m_fileType == FILE_MATLAB)
        return sWord == "if"
            || sWord == "try"
            || sWord == "switch"
            || sWord == "elseif"
            || sWord == "else"
            || sWord == "case"
            || sWord == "otherwise"
            || sWord == "catch";

    return false;
}


/////////////////////////////////////////////////
/// \brief This function returns the block ID.
///
/// \param word const wxString&
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::getBlockID(const wxString& word)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        if (!word.length())
            return wxNOT_FOUND;

        for (size_t i = 0; i < vBlockDefs.size(); i++)
        {
            if (isEqualIgnoreWhitespace(vBlockDefs[i].startWord, word)
                || isEqualIgnoreWhitespace(vBlockDefs[i].middleWord1, word)
                || isEqualIgnoreWhitespace(vBlockDefs[i].middleWord2, word)
                || isEqualIgnoreWhitespace(vBlockDefs[i].endWord, word))
                return i;
        }
    }
    else if (m_fileType == FILE_MATLAB)
        return word == "if"
            || word == "for"
            || word == "while"
            || word == "classdef"
            || word == "properties"
            || word == "function"
            || word == "methods"
            || word == "try"
            || word == "switch"
            || word == "end"
            || word == "elseif"
            || word == "else"
            || word == "case"
            || word == "otherwise"
            || word == "catch";

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief This member function returns a the
/// complete autocompletion block for the passed
/// control flow statement block start.
///
/// \param sWord const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReEditor::getBlockAutoCompletion(const wxString& sWord)
{
    if (m_fileType == FILE_NSCR || m_fileType == FILE_NPRC)
    {
        if (sWord == "layout" || sWord == "compose" || sWord == "group")
            return sWord + " |\r\n" + getBlockEnd(sWord);
        else if (sWord == "try")
            return sWord + "|\r\n" + getBlockEnd(sWord); // Had to remove first "\r\n" due to a lack in the insertion algorithm
        else if (sWord == "procedure")
            return sWord + " $|()\r\nendprocedure";
        else if (sWord == "else")
            return sWord + "|";
        else if (sWord == "elseif")
            return sWord + " (|)";
        else if (sWord == "case" || sWord == "catch")
            return sWord + " |:";
        else if (sWord == "default")
            return sWord + ":|";
        else
            return sWord + " (|)\r\n" + getBlockEnd(sWord);
    }
    else if (m_fileType == FILE_MATLAB)
    {
        if (sWord == "else" || sWord == "otherwise")
            return sWord + "|";
        else if (sWord == "elseif" || sWord == "case" || sWord == "catch")
            return sWord + " |";
        else
            return sWord + " |\r\n" + getBlockEnd(sWord);
    }

    return "";
}

/////////////////////////////////////////////////
/// \brief This member function returns the
/// control flow statement block end, which
/// corresponds to the passed word.
///
/// \param sWord const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReEditor::getBlockEnd(const wxString& sWord)
{
    if (m_fileType == FILE_MATLAB)
        return "end";

    int id = isBlockStart(sWord, true);

    if (id != wxNOT_FOUND)
        return vBlockDefs[id].endWord;

    return "";
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// control flow statement block start, which
/// corresponds to the passed word.
///
/// \param sWord const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReEditor::getBlockStart(const wxString& sWord)
{
    if (m_fileType == FILE_MATLAB)
        return "";

    int id = getBlockID(sWord);

    if (id != wxNOT_FOUND && !isEqualIgnoreWhitespace(sWord, vBlockDefs[id].startWord))
        return vBlockDefs[id].startWord;

    return "";
}


/////////////////////////////////////////////////
/// \brief Removes the double-click occurence
/// indicators from the document.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::ClearDblClkIndicator()
{
    if (!m_dblclkString.length())
        return;

    m_dblclkString.clear();

    SetIndicatorCurrent(HIGHLIGHT_DBLCLK);
    long int maxpos = GetLastPosition();
    IndicatorClearRange(0, maxpos);
}


/////////////////////////////////////////////////
/// \brief Event handler called when clicking on the
/// editor margin.
///
/// \param event wxStyledTextEvent&
/// \return void
///
/// This event handler function toggles the folding,
/// if one clicks on the fold margin, and handles
/// the breakpoints or the hidden lines markers.
/////////////////////////////////////////////////
void NumeReEditor::OnMarginClick(wxStyledTextEvent& event)
{
    bool bCanUseBreakPoints = m_fileType == FILE_NSCR || m_fileType == FILE_NPRC;
    int position = event.GetPosition();
    int linenum = LineFromPosition(position);

    if (event.GetMargin() == MARGIN_FOLD)
    {
        // Folding logic
        int levelClick = GetFoldLevel(linenum);

        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
            ToggleFold(linenum);
    }
    else
    {
        // All other markers
        if (MarkerOnLine(linenum, MARKER_HIDDEN))
        {
            // Hidden lines
            int nNextVisibleLine = linenum+1;

            while (!GetLineVisible(nNextVisibleLine))
                nNextVisibleLine++;

            ShowLines(linenum, nNextVisibleLine);

            for (int i = linenum; i < nNextVisibleLine; i++)
            {
                MarkerDelete(i, MARKER_HIDDEN);
                MarkerDelete(i, MARKER_HIDDEN_MARGIN);
            }

        }
        else if ((MarkerOnLine(linenum, MARKER_BREAKPOINT) || MarkerOnLine(linenum, MARKER_CONDITIONALBREAKPOINT)) && bCanUseBreakPoints)
        {
            // Breakpoint
            RemoveBreakpoint(linenum);
        }
        else if (bCanUseBreakPoints)
        {
            // Add breakpoint
            AddBreakpoint(linenum);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Adds a breakpoint to the selected line.
///
/// \param linenum int
/// \return void
///
/// This function checks in advance, whether the
/// selected line is non-empty and not a comment-
/// only line.
/////////////////////////////////////////////////
void NumeReEditor::AddBreakpoint(int linenum)
{
    // Add a breakpoint, if it is allowed in this line
    if (isBreakPointAllowed(linenum))
    {
        // Add the breakpoint marker
        int markerNum = MarkerAdd(linenum, MARKER_BREAKPOINT);

        // Add the breakpoint to the internal
        // logic
        m_breakpoints.Add(markerNum);
        m_terminal->addBreakpoint(GetFileNameAndPath().ToStdString(), linenum, Breakpoint(true));
    }
}


/////////////////////////////////////////////////
/// \brief Edits a breakpoint at the selected
/// line (if any).
///
/// \param linenum int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::EditBreakpoint(int linenum)
{
    if (!isBreakPointAllowed(linenum)
        || (!MarkerOnLine(linenum, MARKER_BREAKPOINT) && !MarkerOnLine(linenum, MARKER_CONDITIONALBREAKPOINT)))
        return;

    // Get the current breakpoint definition
    Breakpoint bp = m_terminal->getBreakpoint(GetFileNameAndPath().ToStdString(), linenum);

    // Get the expression from the user
    wxString newCondition = wxGetTextFromUser(_guilang.get("GUI_MENU_EDITOR_EDITBP_TEXT"),
                                              _guilang.get("GUI_MENU_EDITOR_EDITBP_HEAD"),
                                              bp.m_condition, this);

    if (!newCondition.length())
        return;

    // Create and update the internal breakpoint
    // TODO: we might want to check the expression for requirement fulfillment
    bp = Breakpoint(newCondition.ToStdString());
    m_terminal->addBreakpoint(GetFileNameAndPath().ToStdString(), linenum, bp);

    // Enable the correct type of breakpoint on the margin
    if (MarkerOnLine(linenum, bp.m_isConditional ? MARKER_BREAKPOINT : MARKER_CONDITIONALBREAKPOINT))
    {
        MarkerDelete(linenum, bp.m_isConditional ? MARKER_BREAKPOINT : MARKER_CONDITIONALBREAKPOINT);
        MarkerAdd(linenum, bp.m_isConditional ? MARKER_CONDITIONALBREAKPOINT : MARKER_BREAKPOINT);
    }
}


/////////////////////////////////////////////////
/// \brief Removes a breakpoint from the selected
/// line.
///
/// \param linenum int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::RemoveBreakpoint(int linenum)
{
    // need to remove the marker handle from the array - use
    // LineFromHandle on debug start and clean up then
    MarkerDelete(linenum, MARKER_BREAKPOINT);
    MarkerDelete(linenum, MARKER_CONDITIONALBREAKPOINT);
    m_terminal->removeBreakpoint(GetFileNameAndPath().ToStdString(), linenum);
}


/////////////////////////////////////////////////
/// \brief Synchronizes all breakpoints between
/// editor and kernel.
///
/// \return void
///
/// This member function synchronizes all breakpoints
/// in the current opened file after the file was
/// modified and saved
/////////////////////////////////////////////////
void NumeReEditor::SynchronizeBreakpoints()
{
    // Clear all breakpoints stored internally
    m_terminal->clearBreakpoints(GetFileNameAndPath().ToStdString());
    int line = 0;

    // Re-set the existing breakpoints
    while ((line = MarkerNext(line, 1 << MARKER_BREAKPOINT)) != -1)
    {
        if (isBreakPointAllowed(line))
            m_terminal->addBreakpoint(GetFileNameAndPath().ToStdString(), line, Breakpoint(true));
        else
            MarkerDelete(line, MARKER_BREAKPOINT);

        line++;
    }
}


/////////////////////////////////////////////////
/// \brief Check, whether a breakpoint is allowed
/// in this line.
///
/// \param linenum int
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::isBreakPointAllowed(int linenum)
{
    for (int i = PositionFromLine(linenum); i < GetLineEndPosition(linenum); i++)
    {
        // Check the current character
        if (!isStyleType(STYLE_COMMENT_LINE, i)
            && !isStyleType(STYLE_COMMENT_BLOCK, i)
            && !isStyleType(STYLE_COMMENT_SECTION_LINE, i)
            && !isStyleType(STYLE_COMMENT_SECTION_BLOCK, i)
            && GetCharAt(i) != '\r'
            && GetCharAt(i) != '\n'
            && GetCharAt(i) != ' '
            && GetCharAt(i) != '\t')
        {
            // Add the breakpoint marker
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of documentation blocks in front of
/// procedure heads.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::AddProcedureDocumentation()
{
    if (getFileType() != FILE_NPRC)
        return;

    // Find the possible start position of the current procedure
    int nProcedureHeadPosition = m_search->FindCurrentProcedureHead(GetLineEndPosition(GetCurrentLine()));

    // Ensure that the cursor is in the line of the procedure's
    // head
    if (LineFromPosition(nProcedureHeadPosition) != GetCurrentLine())
        return;

    string sProcedureName = m_search->FindMarkedProcedure(nProcedureHeadPosition + 11, false).ToStdString();
    sProcedureName.erase(sProcedureName.find('('));
    replaceAll(sProcedureName, "_", "\\_");

    string sDocumentation = "##! \\procedure " + sProcedureName.substr(1) + "\r\n##! [Procedure description]";

    // Get the argument list
    string sFunctionArgumentList = getFunctionArgumentList(LineFromPosition(nProcedureHeadPosition)).ToStdString();

    if (sFunctionArgumentList.length())
        sDocumentation += "\r\n##!";

    // Separate the argument list and prepare the template
    while (sFunctionArgumentList.length())
    {
        string sCurArg = getNextArgument(sFunctionArgumentList, true);
        string sDefault;

        // Remove default arguments from the list and store them separately
        if (sCurArg.find('=') != string::npos)
        {
            sDefault = sCurArg.substr(sCurArg.find_first_not_of(" =\t", sCurArg.find('=')));
            sCurArg.erase(sCurArg.find_last_not_of(" =", sCurArg.find('='))+1);
        }

        sDocumentation += "\r\n##! \\param " + sCurArg + "  ";

        // Append default parameters, if they are available
        if (sDefault.length())
            sDocumentation += "(!!=" + sDefault + "!!) ";

        sDocumentation += "[Parameter description]";
    }

    // Insert the text before the procedure head
    InsertText(nProcedureHeadPosition, sDocumentation + "\r\n");
}


/////////////////////////////////////////////////
/// \brief Checks, whether the passed marker is set
/// on the passed line.
///
/// \param linenum int
/// \param nMarker int
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReEditor::MarkerOnLine(int linenum, int nMarker)
{
    int markerLineBitmask = this->MarkerGet(linenum);
    return (markerLineBitmask & (1 << nMarker));
}


/////////////////////////////////////////////////
/// \brief Main thread loop for the duplicated
/// code analysis.
///
/// \return wxThread::ExitCode
///
/////////////////////////////////////////////////
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

    // Go through the selected code
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

        // Search for all possible duplications in the remaining document.
        // We don't have to search before the current line, because we would
        // have found this duplication earlier.
        for (int j = i + currentDuplicateCodeLength; j <= m_nLastLine - currentDuplicateCodeLength; j++)
        {
            dMatch = compareCodeLines(i, j, m_nDuplicateCodeFlag);

            // The match should be at least 75%
            if (dMatch >= 0.75)
            {
                double dComp;

                // Search the following code lines for a continuation
                // of the current match
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
                        // Was the last line of the duplication. Is it long
                        // enough?
                        if (k - nBlankLines > currentDuplicateCodeLength)
                        {
                            // Add the current duplication to the buffer
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


/////////////////////////////////////////////////
/// \brief Starts the duplicated code analysis.
///
/// \param startline int
/// \param endline int
/// \param nDuplicateFlags int
/// \param nNumDuplicatedLines int
/// \return void
///
/// This function starts the secondary thread,
/// which will handle the actual analysis, because
/// it is very time and resource consuming and would
/// block the editor otherwise.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Performs a semantic code comparsion of
/// the two selected lines.
///
/// \param nLine1 int
/// \param nLine2 int
/// \param nDuplicateFlags int
/// \return double
///
/// This function performs a semantic code comparison
/// between two code lines. Will return a double
/// representing a matching percentage. This value
/// is constructed out of a semantic match.
/// If the line lengths differ too much, the
/// analysis is omitted.
/////////////////////////////////////////////////
double NumeReEditor::compareCodeLines(int nLine1, int nLine2, int nDuplicateFlags)
{
    size_t nMatchedCount = 0;

    // Get the code lines transformed into semantic
    // code
    string sSemLine1 = this->getSemanticLine(nLine1, nDuplicateFlags);
    string sSemLine2 = this->getSemanticLine(nLine2, nDuplicateFlags);

    // It is possible that the lines are semantical identical although they may contain different vars
    if (!sSemLine1.length() && sSemLine2.length())
        return -2.0;
    else if (!sSemLine1.length() && !sSemLine2.length())
        return -1.0;
    else if (sSemLine1.length() * 1.5 < sSemLine2.length() || sSemLine1.length() > sSemLine2.length() * 1.5)
        return 0.0;

    // Check the actual match of the semantic code
    for (size_t i = 0; i < sSemLine1.length(); i++)
    {
        if (i >= sSemLine2.length())
            break;

        if (sSemLine1[i] == sSemLine2[i])
            nMatchedCount++;
    }

    return (double)nMatchedCount / (double)max(sSemLine1.length(), sSemLine2.length());
}


/////////////////////////////////////////////////
/// \brief Returns the selected line as semantic
/// code.
///
/// \param nLine int
/// \param nDuplicateFlags int
/// \return string
///
/// If the selected line was already transformed
/// into semantic code, the semantic code is read
/// from a buffer.
/////////////////////////////////////////////////
string NumeReEditor::getSemanticLine(int nLine, int nDuplicateFlags)
{
    if (vParsedSemanticCode[nLine].length())
        return vParsedSemanticCode[nLine];

    // Use the correct parser for the current language
    if (getFileType() == FILE_NSCR || getFileType() == FILE_NPRC)
        return getSemanticLineNSCR(nLine, nDuplicateFlags);
    else if (getFileType() == FILE_MATLAB)
        return getSemanticLineMATLAB(nLine, nDuplicateFlags);
    else if (getFileType() == FILE_CPP)
        return getSemanticLineCPP(nLine, nDuplicateFlags);
    else
        return "";
}


/////////////////////////////////////////////////
/// \brief Returns the selected line as semantic
/// code.
///
/// \param nLine int
/// \param nDuplicateFlags int
/// \return string
///
/// This function parses NumeRe code into semantic
/// code and returns it. The degree of transformation
/// is selected using a bit or in \c nDuplicateFlags:
/// \li 0 = direct comparison
/// \li 1 = use var semanticals
/// \li 2 = use string semanticals
/// \li 4 = use numeric semanticals
/////////////////////////////////////////////////
string NumeReEditor::getSemanticLineNSCR(int nLine, int nDuplicateFlags)
{
    string sSemLine = "";

    for (int i = this->PositionFromLine(nLine); i < this->GetLineEndPosition(nLine); i++)
    {
        if (this->GetCharAt(i) == ' '
                || this->GetCharAt(i) == '\t'
                || this->GetCharAt(i) == '\r'
                || this->GetCharAt(i) == '\n'
                || isStyleType(STYLE_COMMENT_LINE, i)
                || isStyleType(STYLE_COMMENT_BLOCK, i))
            continue;
        else if ((nDuplicateFlags & SEMANTICS_VAR)
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
        else if ((nDuplicateFlags & SEMANTICS_STRING) && this->GetStyleAt(i) == wxSTC_NSCR_STRING)
        {
            // replace string literals with a placeholder
            i++;

            while (this->GetStyleAt(i + 1) == wxSTC_NSCR_STRING)
                i++;

            sSemLine += "STR";
        }
        else if ((nDuplicateFlags & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_NSCR_NUMBERS)
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
        else if ((nDuplicateFlags & SEMANTICS_FUNCTION) && this->GetStyleAt(i) == wxSTC_NSCR_CUSTOM_FUNCTION)
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


/////////////////////////////////////////////////
/// \brief Returns the selected line as semantic
/// code.
///
/// \param nLine int
/// \param nDuplicateFlags int
/// \return string
///
/// This function parses MATLAB code into semantic
/// code and returns it. The degree of transformation
/// is selected using a bit or in \c nDuplicateFlags:
/// \li 0 = direct comparison
/// \li 1 = use var semanticals
/// \li 2 = use string semanticals
/// \li 4 = use numeric semanticals
/////////////////////////////////////////////////
string NumeReEditor::getSemanticLineMATLAB(int nLine, int nDuplicateFlags)
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
        else if ((nDuplicateFlags & SEMANTICS_VAR)
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
        else if ((nDuplicateFlags & SEMANTICS_STRING) && this->GetStyleAt(i) == wxSTC_MATLAB_STRING)
        {
            // replace string literals with a placeholder
            i++;

            while (this->GetStyleAt(i + 1) == wxSTC_MATLAB_STRING)
                i++;

            sSemLine += "STR";
        }
        else if ((nDuplicateFlags & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_MATLAB_NUMBER)
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
        else
            sSemLine += this->GetCharAt(i);

    }

    // Store the result to avoid repeated processing of this line
    vParsedSemanticCode[nLine] = sSemLine;
    return sSemLine;
}


/////////////////////////////////////////////////
/// \brief Returns the selected line as semantic
/// code.
///
/// \param nLine int
/// \param nDuplicateFlags int
/// \return string
///
/// This function parses C++ code into semantic
/// code and returns it. The degree of transformation
/// is selected using a bit or in \c nDuplicateFlags:
/// \li 0 = direct comparison
/// \li 1 = use var semanticals
/// \li 2 = use string semanticals
/// \li 4 = use numeric semanticals
/////////////////////////////////////////////////
string NumeReEditor::getSemanticLineCPP(int nLine, int nDuplicateFlags)
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
        else if ((nDuplicateFlags & SEMANTICS_VAR)
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
        else if ((nDuplicateFlags & SEMANTICS_STRING) && (this->GetStyleAt(i) == wxSTC_C_STRING || this->GetStyleAt(i) == wxSTC_C_CHARACTER))
        {
            // replace string literals with a placeholder
            i++;

            while (this->GetStyleAt(i + 1) == wxSTC_C_STRING || this->GetStyleAt(i + 1) == wxSTC_C_CHARACTER)
                i++;

            sSemLine += "STR";
        }
        else if ((nDuplicateFlags & SEMANTICS_NUM) && this->GetStyleAt(i) == wxSTC_C_NUMBER)
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
        else
            sSemLine += this->GetCharAt(i);

    }

    // Store the result to avoid repeated processing of this line
    vParsedSemanticCode[nLine] = sSemLine;
    return sSemLine;
}


/////////////////////////////////////////////////
/// \brief Returns the actual word-wise differences
/// in the selected lines.
///
/// \param nStart1 int
/// \param nEnd1 int
/// \param nStart2 int
/// \param nEnd2 int
/// \return map<int, int>
///
/// This function examines the selected blocks of
/// code linewise and compares every syntax element
/// using \c getNextToken. If they differ, their
/// positions and lengths are stored in the returned
/// map.
/////////////////////////////////////////////////
map<int, int> NumeReEditor::getDifferences(int nStart1, int nEnd1, int nStart2, int nEnd2)
{
    map<int, int> mDifferences;
    int nLinePos1 = 0;
    int nLinePos2 = 0;
    wxString sToken1;
    wxString sToken2;

    // Compare every line in the selected range
    for (int i = 0; i <= nEnd1 - nStart1; i++)
    {
        nLinePos1 = this->PositionFromLine(nStart1 + i);
        nLinePos2 = this->PositionFromLine(nStart2 + i);

        // Read every token from the lines and compare them
        while (nLinePos1 < this->GetLineEndPosition(nStart1 + i) || nLinePos2 < this->GetLineEndPosition(nStart2 + i))
        {
            sToken1 = getNextToken(nLinePos1);
            sToken2 = getNextToken(nLinePos2);

            // Break, if no tokens are available
            if (!sToken1.length() && !sToken2.length())
                break;

            // Compare the tokens
            if (sToken1.length() && !sToken2.length())
                mDifferences[-nLinePos1] = sToken1.length();
            else if (sToken2.length() && !sToken1.length())
                mDifferences[nLinePos2] = sToken2.length();
            else
            {
                if (sToken1 != sToken2)
                {
                    mDifferences[-nLinePos1] = sToken1.length();
                    mDifferences[nLinePos2] = sToken2.length();
                }
            }

            // Increment the search position
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


/////////////////////////////////////////////////
/// \brief Returns the next syntax token starting
/// from the selected position.
///
/// \param nPos int&
/// \return wxString
///
/////////////////////////////////////////////////
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
        nEndPos++;

    // it is possible that we walked over the last position
    if (nEndPos > nCurrentLineEnd)
        return this->GetTextRange(nPos, nCurrentLineEnd);

    return this->GetTextRange(nPos, nEndPos);
}


/////////////////////////////////////////////////
/// \brief Returns the start and end line of the
/// current code context (i.e. the first and the
/// last line of the current procedure).
///
/// \param line int
/// \return std::pair<int,int>
///
/////////////////////////////////////////////////
std::pair<int,int> NumeReEditor::getCurrentContext(int line)
{
    if (m_fileType != FILE_NPRC
        && m_fileType != FILE_MATLAB
        && m_fileType != FILE_CPP)
        return std::make_pair(0, GetLineCount());

    // Try to find the current procedure header
    while (GetFoldParent(line) != wxNOT_FOUND)
    {
        line = GetFoldParent(line);
    }

    // Try to find the procedure foot
    std::vector<int> vMatch = BlockMatch(GetLineIndentPosition(line));

    if (vMatch.back() == wxNOT_FOUND)
        return std::make_pair(line, GetLineCount());

    return std::make_pair(line, LineFromPosition(vMatch.back()));
}


/////////////////////////////////////////////////
/// \brief This helper function finds the line
/// to start code execution for the run from
/// line command. This includes checking for
/// continued lines and loops.
///
/// \param line int
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::getStartLine(int line)
{
    // Only check for scripts and do not do further checks for line 0
    if (m_fileType != FILE_NSCR || line == 0)
        return 0;

    // If in a loop, find start of loop
    while (GetFoldParent(line - 1) != wxNOT_FOUND)
        line = GetFoldParent(line - 1) + 1;

    // Check if line is continued and if so go to start of line
    while (GetLine(line - 2).find("\\\\") != std::string::npos)
        line--;

    return line;
}


/////////////////////////////////////////////////
/// \brief Wrapper for \c CodeFormatter.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::ApplyAutoIndentation(int nFirstLine, int nLastLine)
{
    m_formatter->IndentCode(nFirstLine, nLastLine);
}


/////////////////////////////////////////////////
/// \brief Determine the syntax style type at the
/// selected position.
///
/// \param _type StyleType
/// \param nPos int
/// \return bool
///
/// This member function summarizes determining,
/// which style type the selected character is
/// using abstracting out the selection of the
/// correct styling language.
/////////////////////////////////////////////////
bool NumeReEditor::isStyleType(StyleType _type, int nPos)
{
    switch (getFileType())
    {
        case FILE_NSCR:
        case FILE_NPRC:
            {
                switch (_type)
                {
                    case STYLE_DEFAULT:
                        return GetStyleAt(nPos) == wxSTC_NSCR_DEFAULT;
                    case STYLE_COMMENT:
                        return GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_LINE
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_LINE
                                || GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_BLOCK
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_BLOCK
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCKEYWORD;
                    case STYLE_COMMENT_LINE:
                        return GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_LINE
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_LINE;
                    case STYLE_COMMENT_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_NSCR_COMMENT_BLOCK
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_BLOCK;
                    case STYLE_COMMENT_SECTION_LINE:
                        return GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_LINE
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCKEYWORD;
                    case STYLE_COMMENT_SECTION_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_NSCR_DOCCOMMENT_BLOCK
                                || GetStyleAt(nPos) == wxSTC_NSCR_DOCKEYWORD;
                    case STYLE_COMMAND:
                        return GetStyleAt(nPos) == wxSTC_NSCR_COMMAND
                               || GetStyleAt(nPos) == wxSTC_NPRC_COMMAND
                               || GetStyleAt(nPos) == wxSTC_NSCR_PROCEDURE_COMMANDS;
                    case STYLE_FUNCTION:
                        return GetStyleAt(nPos) == wxSTC_NSCR_FUNCTION;
                    case STYLE_CUSTOMFUNCTION:
                        return GetStyleAt(nPos) == wxSTC_NSCR_CUSTOM_FUNCTION;
                    case STYLE_OPERATOR:
                        return GetStyleAt(nPos) == wxSTC_NSCR_OPERATORS;
                    case STYLE_PROCEDURE:
                        return GetStyleAt(nPos) == wxSTC_NSCR_PROCEDURES;
                    case STYLE_IDENTIFIER:
                        return GetStyleAt(nPos) == wxSTC_NSCR_IDENTIFIER;
                    case STYLE_DATAOBJECT:
                        return GetStyleAt(nPos) == wxSTC_NSCR_CUSTOM_FUNCTION || GetStyleAt(nPos) == wxSTC_NSCR_CLUSTER;
                    case STYLE_NUMBER:
                        return GetStyleAt(nPos) == wxSTC_NSCR_NUMBERS;
                    case STYLE_STRINGPARSER:
                        return GetStyleAt(nPos) == wxSTC_NSCR_STRING_PARSER;
                    case STYLE_STRING:
                        return GetStyleAt(nPos) == wxSTC_NSCR_STRING;
                }
                break;
            }
        case FILE_MATLAB:
            {
                switch (_type)
                {
                    case STYLE_DEFAULT:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_DEFAULT;
                    case STYLE_COMMENT:
                    case STYLE_COMMENT_LINE:
                    case STYLE_COMMENT_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT;
                    case STYLE_COMMENT_SECTION_LINE:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT && GetTextRange(nPos, nPos + 2) == "%%";
                    case STYLE_COMMENT_SECTION_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_COMMENT && GetTextRange(nPos, nPos + 2) == "%%";
                    case STYLE_COMMAND:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_KEYWORD;
                    case STYLE_FUNCTION:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_FUNCTIONS;
                    case STYLE_CUSTOMFUNCTION:
                        return false;
                    case STYLE_OPERATOR:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_OPERATOR;
                    case STYLE_PROCEDURE:
                        return false;
                    case STYLE_IDENTIFIER:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_IDENTIFIER;
                    case STYLE_DATAOBJECT:
                        return false;
                    case STYLE_NUMBER:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_NUMBER;
                    case STYLE_STRINGPARSER:
                        return false;
                    case STYLE_STRING:
                        return GetStyleAt(nPos) == wxSTC_MATLAB_STRING;
                }
                break;
            }
        case FILE_CPP:
            {
                switch (_type)
                {
                    case STYLE_DEFAULT:
                        return GetStyleAt(nPos) == wxSTC_C_DEFAULT;
                    case STYLE_COMMENT:
                        return GetStyleAt(nPos) == wxSTC_C_COMMENTLINE
                                || GetStyleAt(nPos) == wxSTC_C_COMMENT
                                || GetStyleAt(nPos) == wxSTC_C_COMMENTDOC
                                || GetStyleAt(nPos) == wxSTC_C_COMMENTLINEDOC
                                || GetStyleAt(nPos) == wxSTC_C_COMMENTDOCKEYWORD;
                    case STYLE_COMMENT_LINE:
                        return GetStyleAt(nPos) == wxSTC_C_COMMENTLINE
                                || GetStyleAt(nPos) == wxSTC_C_COMMENTLINEDOC;
                    case STYLE_COMMENT_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_C_COMMENT
                                || GetStyleAt(nPos) == wxSTC_C_COMMENTDOC;
                    case STYLE_COMMENT_SECTION_LINE:
                        return GetStyleAt(nPos) == wxSTC_C_COMMENTLINEDOC
                               || GetStyleAt(nPos) == wxSTC_C_COMMENTDOCKEYWORD;
                    case STYLE_COMMENT_SECTION_BLOCK:
                        return GetStyleAt(nPos) == wxSTC_C_COMMENTDOC
                               || GetStyleAt(nPos) == wxSTC_C_COMMENTDOCKEYWORD;
                    case STYLE_COMMAND:
                        return GetStyleAt(nPos) == wxSTC_C_WORD;
                    case STYLE_FUNCTION:
                        return GetStyleAt(nPos) == wxSTC_C_WORD2;
                    case STYLE_CUSTOMFUNCTION:
                        return false;
                    case STYLE_OPERATOR:
                        return GetStyleAt(nPos) == wxSTC_C_OPERATOR;
                    case STYLE_PROCEDURE:
                        return false;
                    case STYLE_IDENTIFIER:
                        return GetStyleAt(nPos) == wxSTC_C_IDENTIFIER;
                    case STYLE_DATAOBJECT:
                        return false;
                    case STYLE_NUMBER:
                        return GetStyleAt(nPos) == wxSTC_C_NUMBER;
                    case STYLE_STRINGPARSER:
                        return false;
                    case STYLE_STRING:
                        return GetStyleAt(nPos) == wxSTC_C_STRING;
                }
                break;
            }
        default:
            return false;
    }
    return false;
}


/////////////////////////////////////////////////
/// \brief Counts the german umlauts in the current
/// string.
///
/// \param sStr const string&
/// \return int
///
/////////////////////////////////////////////////
int NumeReEditor::countUmlauts(const string& sStr)
{
    static Umlauts _umlauts;
    int nUmlauts = 0;
    for (size_t i = 0; i < sStr.length(); i++)
    {
        if (_umlauts.isUmlaut(sStr[i]))
            nUmlauts++;
    }
    return nUmlauts;
}


/////////////////////////////////////////////////
/// \brief Re-alignes the passed language string
/// to fit into a call tip.
///
/// \param sLine string
/// \param lastpos size_t&
/// \return string
///
/////////////////////////////////////////////////
string NumeReEditor::realignLangString(string sLine, size_t& lastpos)
{
    lastpos = sLine.find(' ');

    if (lastpos == string::npos)
        return sLine;

    // Find the first non-whitespace character
    size_t firstpos = sLine.find_first_not_of(' ', lastpos);

    // Insert separation characters between syntax element
    // and return values
    if (sLine.find(')') < lastpos || sLine.find('.') < lastpos)
        sLine.replace(lastpos, firstpos - lastpos, " -> ");
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


/////////////////////////////////////////////////
/// \brief Adds linebreaks to the call tip language
/// strings.
///
/// \param sLine const string&
/// \param onlyDocumentation bool
/// \return string
///
/// This member function adds linebreaks at the
/// maximal line length of 100 characters. It is
/// used for the tooltips of functions, commands
/// and procedures
/////////////////////////////////////////////////
string NumeReEditor::addLinebreaks(const string& sLine, bool onlyDocumentation /* = false*/)
{
    const size_t nMAXLINE = 100;

    string sReturn = sLine;

    // Remove escaped dollar signs
    while (sReturn.find("\\$") != string::npos)
        sReturn.erase(sReturn.find("\\$"), 1);

    size_t nDescStart = sReturn.find("- ");
    size_t nIndentPos = 4;
    size_t nLastLineBreak = 0;
    size_t addIndent = 0;
    static const std::regex expr("    (\\d+\\.|-) +(?=\\S+)");
    std::smatch match;

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

    for (size_t i = nDescStart; i < sReturn.length(); i++)
    {
        if (sReturn[i] == '\n')
        {
            nLastLineBreak = i;

            std::string sCandidate = sReturn.substr(i+1, 15);

            if (std::regex_search(sCandidate, match, expr) && match.position(0) == 0)
                addIndent = match.length(0)-4;
            else
                addIndent = 0;
        }

        if ((i == nMAXLINE && !nLastLineBreak)
                || (nLastLineBreak && i - nLastLineBreak == nMAXLINE))
        {
            for (int j = i; j >= 0; j--)
            {
                if (sReturn[j] == ' ')
                {
                    sReturn[j] = '\n';
                    sReturn.insert(j + 1, nIndentPos + addIndent, ' ');
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
                    sReturn.insert(j + 2, nIndentPos + addIndent, ' ');
                    nLastLineBreak = j + 1;
                    break;
                }
                else if (sReturn[j] == ',' && j != (int)i && sReturn[j + 1] != ' ')
                {
                    sReturn.insert(j + 1, "\n");
                    sReturn.insert(j + 2, nIndentPos + addIndent, ' ');
                    nLastLineBreak = j + 1;
                    break;
                }
            }
        }
    }

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief Wrapper for \c CodeFormatter.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::ApplyAutoFormat(int nFirstLine, int nLastLine)
{
    m_formatter->FormatCode(nFirstLine, nLastLine);
}


/////////////////////////////////////////////////
/// \brief This method transposes tabular data in
/// between (including) starting and last line.
/// If the current file is a CSV, heuristics are
/// applied to determine the separator character,
/// otherwise all usual whitespace characters are
/// used to sparate the different cells of the
/// tabular data.
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return void
///
/////////////////////////////////////////////////
void NumeReEditor::Transpose(int nFirstLine, int nLastLine)
{
    if (nFirstLine < 0)
		nFirstLine = 0;

	if (nLastLine <= 0 || nLastLine > GetLineCount())
		nLastLine = GetLineCount()-1;

    int nFirstPos = PositionFromLine(nFirstLine);
    int nLastPos = GetLineEndPosition(nLastLine);
    size_t maxLines = 0;

    // Clear all selections
    if (HasSelection())
        ClearSelections();

    std::vector<std::vector<wxString>> vTable;
    wxStringTokenizer tokenizer;
    wxString sDelimiter;

    // Determine the delimiter characters
    if (GetFileName().GetExt() == "csv")
    {
        // For a CSV file, we will search for
        // some distinct combinations to determine,
        // whether a comma or a semicolon is used
        // as separator character
        wxString origSection = GetTextRange(nFirstPos, nLastPos);

        if (origSection.find(';') != string::npos && origSection.find(',') != string::npos)
            sDelimiter = ";\r\n"; // Comma is decimal separator
        else if (origSection.find('.') != string::npos && origSection.find(',') != string::npos)
            sDelimiter = ",\r\n"; // dot is decimal separator
        else
            sDelimiter = ";,\r\n"; // Fallback: separate both
    }
    else
        sDelimiter = "\t \r\n";

    // Tokenize the selected lines into a string table
    for (int i = nFirstLine; i <= nLastLine; i++)
    {
        vTable.push_back(std::vector<wxString>());
        tokenizer.SetString(GetLine(i), sDelimiter);

        while (tokenizer.HasMoreTokens())
            vTable[i-nFirstLine].push_back(tokenizer.GetNextToken());

        // Update the numbers of columns (will be lines),
        // if necessary
        if (vTable[i-nFirstLine].size() > maxLines)
            maxLines = vTable[i-nFirstLine].size();
    }

    wxString finalLayout;

    // Layout the transposed table
    for (size_t i = 0; i < maxLines; i++)
    {
        for (size_t j = 0; j < vTable.size(); j++)
        {
            if (vTable[j].size() > i)
                finalLayout += vTable[j][i];

            if (j+1 < vTable.size())
                finalLayout += sDelimiter[0];
        }

        if (i+1 < maxLines)
            finalLayout += "\r\n";
    }

    // Replace the original section with the
    // transposed table
    Replace(nFirstPos, nLastPos, finalLayout);
}


/////////////////////////////////////////////////
/// \brief Extracts the selected lines as pre-
/// formatted HTML and returns it
///
/// \param nFirstLine int
/// \param nLastLine int
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReEditor::ExtractAsHTML(int nFirstLine, int nLastLine)
{
    wxString sHtml = "<pre style=\"display: block; overflow-x: auto; padding: 0.5em; background-color: rgb(244, 244, 244); color: rgb(0, 0, 0); font-family: consolas, monospace;\">";

    if (nFirstLine < 0)
		nFirstLine = 0;

	if (nLastLine <= 0 || nLastLine > GetLineCount())
		nLastLine = GetLineCount()-1;

    int nFirstPos = PositionFromLine(nFirstLine);
    int nLastPos = GetLineEndPosition(nLastLine);
    int nLastStatePosition = nFirstPos;
    wxTextAttr textAttr;

    for (int i = nFirstPos; i < nLastPos; i++)
    {
        if (GetStyleAt(i) != GetStyleAt(nLastStatePosition) || i+1 == nLastPos)
        {
            int style = GetStyleAt(nLastStatePosition);
            wxString textRange;

            if (i+1 == nLastPos)
                textRange = GetTextRange(nLastStatePosition, i+1);
            else
                textRange = GetTextRange(nLastStatePosition, i);

            // Replace less and greater characters with the
            // HTML entities
            textRange.Replace("<", "&lt;");
            textRange.Replace(">", "&gt;");

            if (textRange.find_first_not_of(" \r\t\n") == std::string::npos)
            {
                sHtml += textRange;
                nLastStatePosition = i;
                continue;
            }

            sHtml += "<span style=\"color: rgb(" + toString(StyleGetForeground(style).Red()) + ", " + toString(StyleGetForeground(style).Green()) + ", " + toString(StyleGetForeground(style).Blue()) + ");";

            if (StyleGetBackground(style) != *wxWHITE)
                sHtml += " background-color: rgb(" + toString(StyleGetBackground(style).Red()) + ", " + toString(StyleGetBackground(style).Green()) + ", " + toString(StyleGetBackground(style).Blue()) + ");";

            if (StyleGetItalic(style))
                sHtml += " font-style: italic;";

            if (StyleGetBold(style))
                sHtml += " font-weight: bold;";

            if (StyleGetUnderline(style))
                sHtml += " text-decoration: underline;";

            sHtml += "\">" + textRange + "</span>";
            nLastStatePosition = i;
        }
    }

    sHtml += "</pre>";

    sHtml.Replace("\t", "    ");

    return sHtml;
}
