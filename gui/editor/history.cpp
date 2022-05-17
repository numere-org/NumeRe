/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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



#include "../../common/CommonHeaders.h"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"

#include <wx/datetime.h>
#include <wx/stdpaths.h>
#include <vector>
#include <string>

#include "history.hpp"

#define MARGIN_FOLD 3

BEGIN_EVENT_TABLE(NumeReHistory, wxStyledTextCtrl)
	EVT_LEFT_DOWN       (NumeReHistory::OnMouseDn)
	EVT_RIGHT_DOWN		(NumeReHistory::OnRightClick)
	EVT_LEFT_DCLICK		(NumeReHistory::OnMouseDblClk)
	EVT_MOUSE_CAPTURE_LOST(NumeReHistory::OnMouseCaptureLost)
	EVT_STC_MARGINCLICK (-1, NumeReHistory::OnMarginClick)
	EVT_MENU            (ID_DELETE_FROM_HISTORY, NumeReHistory::OnMenuEvent)
	EVT_MENU            (ID_COPY_FROM_HISTORY, NumeReHistory::OnMenuEvent)
END_EVENT_TABLE()


extern Language _guilang;
using namespace std;

/////////////////////////////////////////////////
/// \brief History constructor
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
NumeReHistory::NumeReHistory(NumeReWindow* mframe, Options* options, wxWindow* parent, wxWindowID id, NumeReSyntax* __syntax,
                             NumeReTerminal* __terminal, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
                                 NumeReEditor(mframe, options, parent, id, __syntax, __terminal, pos, size, style, name)
{
	_syntax = __syntax;
	m_mainframe = mframe;
	m_terminal = __terminal;
	m_clickedLine = 0;

    this->SetTabWidth(4);
    this->SetIndent(4);
    this->SetUseTabs(true);
    this->SetReadOnly(true);

    //this->SetMarginWidth(0, 22);
    this->SetMarginWidth(0, 0);
    this->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    this->SetMarginWidth(1,0);

    //wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
    wxFont font;
    font.SetNativeFontInfoUserDesc("Consolas 8 WINDOWS-1252");

    this->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

    this->StyleClearAll();

    this->SetMouseDwellTime(250);
    loadHistory();

	UpdateSyntaxHighlighting();

	SetEdgeMode(wxSTC_EDGE_NONE);
	this->SetCaretLineVisible(false);

	this->SetWrapMode(wxSTC_WRAP_WORD);
    this->SetWrapIndentMode(wxSTC_WRAPINDENT_INDENT);
    this->SetWrapStartIndent(1);
    this->SetWrapVisualFlags(wxSTC_WRAPVISUALFLAG_END);
    this->SetWrapVisualFlagsLocation(wxSTC_WRAPVISUALFLAGLOC_END_BY_TEXT);

    this->GotoPos(this->GetLastPosition());
}

NumeReHistory::~NumeReHistory()
{
}


void NumeReHistory::UpdateSyntaxHighlighting(bool forceUpdate)
{
	this->StyleSetBackground(wxSTC_STYLE_DEFAULT, m_options->GetSyntaxStyle(Options::STANDARD).background);

    this->SetLexer(wxSTC_LEX_NSCR);

    this->SetProperty("fold", "0");
    this->SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_EXPANDED);

    this->SetMarginType(MARGIN_FOLD, wxSTC_MARGIN_SYMBOL);
    this->SetMarginWidth(MARGIN_FOLD, 10);
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
            case wxSTC_NSCR_CLUSTER:
                _style = m_options->GetSyntaxStyle(Options::CLUSTER);
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
	//updateDefaultHighlightSettings();
    this->Colourise(0,-1);
}

void NumeReHistory::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_DELETE_FROM_HISTORY:
            deleteLine();
            break;
        case ID_COPY_FROM_HISTORY:
            copyLine();
            break;
    }
}

void NumeReHistory::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
    if (GetCapture() == this)
    {
        ReleaseMouse();
        Refresh();
    }
}

void NumeReHistory::deleteLine()
{
    this->SetReadOnly(false);
    this->GotoLine(m_clickedLine);
    this->LineDelete();
    applyFoldPoints();
    this->SetReadOnly(true);
}

void NumeReHistory::copyLine()
{
    this->SetReadOnly(false);
    this->SetSelection(PositionFromLine(m_clickedLine), GetLineEndPosition(m_clickedLine));
    this->Copy();
    this->GotoPos(PositionFromLine(m_clickedLine));
    this->SetReadOnly(true);
}

void NumeReHistory::loadHistory()
{
    wxString sFileName = m_mainframe->getProgramFolder() + "\\numere.history";

    this->SetReadOnly(false);
    if (fileExists(sFileName.ToStdString()))
    {
        this->LoadFile(sFileName);
        if (this->GetLineCount() > 10000)
            this->DeleteRange(0, this->PositionFromLine(this->GetLineCount()-99999));
        this->GotoPos(this->GetLastPosition());
    }
    addHeader();
    applyFoldPoints();
    this->SetReadOnly(true);
}

void NumeReHistory::saveHistory()
{
    wxString sFileName = m_mainframe->getProgramFolder() + "\\numere.history";

    // only save the history, if it is necessary
    if (getLastLine().substr(0, 7) == "## --- ")
        return;

    this->SaveFile(sFileName);
}

void NumeReHistory::addHeader()
{
    this->AddText("## --- " + getTimeStamp(false) + " ---\n");
}

void NumeReHistory::applyFoldPoints()
{
    int zerolevel = wxSTC_FOLDLEVELBASE;
    for (int i = 0; i < this->GetLineCount(); i++)
    {
        if (this->GetLine(i).substr(0,6) == "## ---")
        {
            if (!i)
            {
                this->SetFoldLevel(i, zerolevel | wxSTC_FOLDLEVELHEADERFLAG);
            }
            else
            {
                //this->SetFoldLevel(i-1, zerolevel | wxSTC_FOLDLEVELWHITEFLAG);
                this->SetFoldLevel(i, zerolevel | wxSTC_FOLDLEVELHEADERFLAG);
            }
        }
        else
            this->SetFoldLevel(i, zerolevel+1);
    }
}

void NumeReHistory::AddToHistory(const wxString& commandstring)
{
    if (!commandstring.length())
        return;
    this->SetReadOnly(false);
    this->GotoPos(this->GetLastPosition());

    // Get last line
    wxString lastline = getLastLine();

    // return, if last line is equal to the current one
    if (lastline == commandstring.substr(0, lastline.length()) && commandstring.find_first_not_of(' ', lastline.length()) == string::npos)
    {
        this->SetReadOnly(true);
        return;
    }
    // add the new line to the history
    this->AddText(commandstring.substr(0, commandstring.find_last_not_of(' ')+1));
    if (commandstring[commandstring.find_last_not_of(' ')] != '\n')
        this->AddText("\n");
    applyFoldPoints();
    this->SetReadOnly(true);
}


wxString NumeReHistory::getLastLine()
{
    wxString lastline = GetLine(LineFromPosition(GetLastPosition()-1));
    lastline.erase(lastline.find_first_of("\n\r"));

    return lastline;
}


void NumeReHistory::OnMarginClick( wxStyledTextEvent &event )
{
	int position = event.GetPosition();

	int linenum = this->LineFromPosition(position);

    if (event.GetMargin() == MARGIN_FOLD)
    {
        int levelClick = this->GetFoldLevel(linenum);
        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
            this->ToggleFold(linenum);
    }
}

void NumeReHistory::OnMouseDblClk(wxMouseEvent& event)
{
    int clickedLine = LineFromPosition(PositionFromPoint(event.GetPosition()));
    wxString line = this->GetLine(clickedLine);
    if (line.substr(0,6) == "## ---")
    {
        int levelClick = this->GetFoldLevel(clickedLine);
        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
            this->ToggleFold(clickedLine);
        return;
    }
    while (line[line.length()-1] == '\r' || line[line.length()-1] == '\n')
        line.erase(line.length()-1);
    if (line.length())
        m_terminal->pass_command(line.ToStdString(), false);
}

void NumeReHistory::OnMouseDn(wxMouseEvent& event)
{
    int clickedLine = LineFromPosition(PositionFromPoint(event.GetPosition()));
    if (this->GetLine(clickedLine).substr(0,6) == "## ---")
        return;
    this->SetSelection(PositionFromLine(clickedLine), GetLineEndPosition(clickedLine));
    event.Skip();
}

void NumeReHistory::OnRightClick(wxMouseEvent& event)
{
    int clickedLine = LineFromPosition(PositionFromPoint(event.GetPosition()));
    if (this->GetLine(clickedLine).substr(0,6) == "## ---")
        return;
    m_clickedLine = clickedLine;
    wxMenu popupMenu;
    popupMenu.Append(ID_COPY_FROM_HISTORY, _guilang.get("GUI_HISTORY_COPYLINE"));
    popupMenu.Append(ID_DELETE_FROM_HISTORY, _guilang.get("GUI_HISTORY_DELETELINE"));
    wxPoint p = event.GetPosition();
    this->PopupMenu(&popupMenu, p);
}


