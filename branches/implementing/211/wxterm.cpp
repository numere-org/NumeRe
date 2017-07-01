


/*
    taTelnet - A cross-platform telnet program.
    Copyright (c) 2000 Derry Bryson.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Contact Information:

       Technology Associates, Inc.
       Attn:  Derry Bryson
       959 W. 5th Street
       Reno, NV  89503
       USA

       derry@techass.com
*/

#ifdef __GNUG__
#pragma implementation "wxterm.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/timer.h>

#include <ctype.h>

#include "../network/gterm.hpp"
#include "../network/gtelnet.hpp"
#include "wxterm.h"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "../common/globals.hpp"

#include "../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void WinMessageBeep();

#define CURSOR_BLINK_DEFAULT_TIMEOUT	300
#define CURSOR_BLINK_MAX_TIMEOUT	2000

/*
**  Keycode translation tables
*/
wxTerm::TermKeyMap wxTerm::keyMapTable[] =
{
    { WXK_BACK, GTelnet::KEY_BACK },
    { WXK_TAB, GTelnet::KEY_TAB },
    { WXK_RETURN, GTelnet::KEY_RETURN },
    { WXK_ESCAPE, GTelnet::KEY_ESCAPE },
    { WXK_SPACE, GTelnet::KEY_SPACE },
    { WXK_LEFT, GTelnet::KEY_LEFT },
    { WXK_UP, GTelnet::KEY_UP },
    { WXK_RIGHT, GTelnet::KEY_RIGHT },
    { WXK_DOWN, GTelnet::KEY_DOWN },
//  { WXK_DIVIDE, GTelnet::KEY_DIVIDE },
//  { WXK_MULTIPLY, GTelnet::KEY_MULTIPLY },
//  { WXK_SUBTRACT, GTelnet::KEY_SUBTRACT },
//  { WXK_ADD, GTelnet::KEY_ADD },
    { WXK_HOME, GTelnet::KEY_HOME },
    { WXK_END, GTelnet::KEY_END },
    { WXK_PAGEUP, GTelnet::KEY_PAGEUP },
    { WXK_PAGEDOWN, GTelnet::KEY_PAGEDOWN },
    { WXK_INSERT, GTelnet::KEY_INSERT },
    { WXK_DELETE, GTelnet::KEY_DELETE },
    //{ WXK_NEXT, GTelnet::KEY_NEXT },
    //{ WXK_PRIOR, GTelnet::KEY_PRIOR },
//  { WXK_NUMPAD0, GTelnet::KEY_NUMPAD0 },
//  { WXK_NUMPAD1, GTelnet::KEY_NUMPAD1 },
//  { WXK_NUMPAD2, GTelnet::KEY_NUMPAD2 },
//  { WXK_NUMPAD3, GTelnet::KEY_NUMPAD3 },
//  { WXK_NUMPAD4, GTelnet::KEY_NUMPAD4 },
//  { WXK_NUMPAD5, GTelnet::KEY_NUMPAD5 },
//  { WXK_NUMPAD6, GTelnet::KEY_NUMPAD6 },
//  { WXK_NUMPAD7, GTelnet::KEY_NUMPAD7 },
//  { WXK_NUMPAD8, GTelnet::KEY_NUMPAD8 },
//  { WXK_NUMPAD9, GTelnet::KEY_NUMPAD9 },
//  { WXK_DECIMAL, GTelnet::KEY_NUMPAD_DECIMAL },
    { WXK_F1, GTelnet::KEY_F1 },
    { WXK_F2, GTelnet::KEY_F2 },
    { WXK_F3, GTelnet::KEY_F3 },
    { WXK_F4, GTelnet::KEY_F4 },
    { WXK_F5, GTelnet::KEY_F5 },
    { WXK_F6, GTelnet::KEY_F6 },
    { WXK_F7, GTelnet::KEY_F7 },
    { WXK_F8, GTelnet::KEY_F8 },
    { WXK_F9, GTelnet::KEY_F9 },
    { WXK_F10, GTelnet::KEY_F10 },
    { WXK_F11, GTelnet::KEY_F11 },
    { WXK_F12, GTelnet::KEY_F12 },
    { (wxKeyCode)0, GTelnet::KEY_NULL }
};


BEGIN_EVENT_TABLE(wxTerm, wxWindow)
    EVT_PAINT						(wxTerm::OnPaint)
    EVT_CHAR						(wxTerm::OnChar)
    EVT_LEFT_DOWN					(wxTerm::OnLeftDown)
    EVT_MOUSE_CAPTURE_LOST          (wxTerm::OnLoseMouseCapture)
    EVT_LEFT_UP					    (wxTerm::OnLeftUp)
    EVT_MOTION					    (wxTerm::OnMouseMove)
    EVT_ENTER_WINDOW                (wxTerm::OnEnter)
    EVT_TIMER						(-1, wxTerm::OnTimer)

    EVT_KEY_DOWN                    (wxTerm::OnKeyDown)


    EVT_SIZE						(wxTerm::OnSize)
    EVT_SET_FOCUS					(wxTerm::OnGainFocus)
    EVT_KILL_FOCUS				    (wxTerm::OnLoseFocus)
    EVT_CLOSE                       (wxTerm::OnClose)
END_EVENT_TABLE()

wxTerm::wxTerm(wxWindow* parent, wxWindowID id,
               Options* _option,
               const wxPoint& pos,
               int width, int height,
               const wxString& name) :
    wxWindow(parent, id, pos, wxSize(-1, -1), wxWANTS_CHARS, name),
    GTerm(width, height)
{
    Bind(wxEVT_THREAD, &wxTerm::OnThreadUpdate, this);

    m_inUpdateSize = false;
    m_isActive = false;
    m_scrollBarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_X);

    m_init = 1;

    m_curDC = 0;

    m_charsInLine = width;
    m_linesDisplayed = height;

    m_selecting = false;
    m_selx1 = m_sely1 = m_selx2 = m_sely2 = 0;
    m_marking = false;
    m_curX = -1;
    m_curY = -1;
    m_curBlinkRate = CURSOR_BLINK_DEFAULT_TIMEOUT;
    m_timer.SetOwner(this);
    if(m_curBlinkRate)
        m_timer.Start(m_curBlinkRate);

    m_boldStyle = FONT;
    m_options = _option;
    int w, h;

    /*
    **  Determine window size from current font
    */
    wxClientDC
    dc(this);

    if(m_boldStyle != FONT)
        dc.SetFont(m_normalFont);
    else
        dc.SetFont(m_boldFont);
    dc.GetTextExtent("M", &m_charWidth, &h); // EKHL: Changed because Height made no sense
    dc.GetTextExtent("My", &w, &m_charHeight);

    UpdateColors();

    m_width = width;
    m_height = height;

    m_printerFN = 0;
    m_printerName = 0;

    GTerm::m_numCommandLines = -1;



    m_normalFont = GetFont();
    m_underlinedFont = GetFont();
    m_underlinedFont.SetUnderlined(true);
    m_boldFont = GetFont();
    m_boldFont.SetWeight(wxBOLD);
    m_boldUnderlinedFont = m_boldFont;
    m_boldUnderlinedFont.SetUnderlined(true);

    m_bitmap = 0;

    _kernel.StartUp(this);
    StartKernelTask();
    m_KernelStatus = NumeReKernel::NUMERE_DONE;
    m_bCommandAvailable = false;
    m_bTableEditAvailable = false;
    m_bTableEditCanceled = false;
    m_bContinueDebug = false;
    m_sCommandLine = "";
    m_sAnswer = "";
    m_wxParent = nullptr;

    //ResizeTerminal(width, height);


    //SetVirtualSize(m_charWidth * 80, m_charHeight * 100);
    //SetScrollRate(m_charWidth, m_charHeight);

    m_init = 0;

    SetCursor(wxCursor(wxCURSOR_IBEAM));

    wxFont monospacedFont(8, wxMODERN, wxNORMAL, wxNORMAL, false, "Consolas");//10
    SetFont(monospacedFont);

    // 10pt Courier New is 8 pixels wide and 16 pixels high... set up
    // a default client size to match
    SetClientSize(m_charsInLine * 8, m_linesDisplayed * 16);
    UpdateSize();

    {
        wxCriticalSectionLocker lock(m_kernelCS);
        getSyntax()->addPlugins(_kernel.getPluginCommands());
    }
}

wxTerm::~wxTerm()
{
    if(m_bitmap)
    {
        m_memDC.SelectObject(wxNullBitmap);
        delete m_bitmap;
    }
}


vector<string> wxTerm::getPathSettings()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    vector<string> vPaths = _kernel.getPathSettings();
    return vPaths;
}

void wxTerm::passEditedTable(const vector<vector<string> >& _sTable)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    NumeReKernel::sTable = _sTable;
    m_bTableEditAvailable = true;
}

string wxTerm::getDocumentation(const string& sCommand)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getDocumentation(sCommand);
}

map<string,string> wxTerm::getPluginLanguageStrings()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getPluginLanguageStrings();
}

map<string,string> wxTerm::getFunctionLanguageStrings()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getFunctionLanguageStrings();
}

Settings wxTerm::getKernelSettings()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    Settings _option(_kernel.getKernelSettings());
    return _option;
}

void wxTerm::setKernelSettings(const Settings& _settings)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    _kernel.setKernelSettings(_settings);
}



void wxTerm::StartKernelTask()
{
    if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
    {
        wxLogError("Could not create kernel thread!");
        return;
    }
    if (GetThread()->Run() != wxTHREAD_NO_ERROR)
    {
        wxLogError("Could not run the kernel thread!");
        return;
    }
}

wxThread::ExitCode wxTerm::Entry()
{
    string sCommand = "";
    bool bCommandAvailable = false;

    //NumeReKernel::toggleTableStatus();
    _kernel.printVersionInfo();
    //NumeReKernel::toggleTableStatus();
    //NumeReKernel::printPreFmt("|\n|<- ");
    while (!GetThread()->TestDestroy())
    {
        Sleep(100);
        // start critical section for reading the command and the boolean
        {
            wxCriticalSectionLocker lock(m_kernelCS);
            bCommandAvailable = m_bCommandAvailable;
            sCommand = m_sCommandLine;
            m_bCommandAvailable = false;
            m_sCommandLine.clear();
            if (!sCommand.length() && bCommandAvailable)
            {
                m_KernelStatus = NumeReKernel::NUMERE_PENDING;
                wxQueueEvent(GetEventHandler(), new wxThreadEvent());
                continue;
            }
            if (bCommandAvailable)
                NumeReKernel::_messenger = _guimessenger;
        }
        if (bCommandAvailable) // A command is available
        {
            m_KernelStatus = _kernel.MainLoop(sCommand);
            if (m_KernelStatus > 0) // solve errors and quits
            {
                wxCriticalSectionLocker lock(m_kernelCS);
                switch (m_KernelStatus)
                {
                    // fallthrough is intended
                    case NumeReKernel::NUMERE_DONE:
                    //case NumeReKernel::NUMERE_CALC_UPDATE:
                    //case NumeReKernel::NUMERE_PRINTLINE:
                    //case NumeReKernel::NUMERE_STATUSBAR_UPDATE:
                        m_sAnswer = _kernel.ReadAnswer();
                        break;
                    default:
                        break;
                        //All others
                }
                bCommandAvailable = false;
            }
            else if (m_KernelStatus == NumeReKernel::NUMERE_QUIT) //quit
            {
                break;
            }
            /*else if (m_KernelStatus == NumeReKernel::NUMERE_ERROR)
            {
                wxCriticalSectionLocker lock(m_kernelCS);
                //m_sAnswer = "ERROR IN NUMERE::KERNEL";
            }*/
            wxQueueEvent(GetEventHandler(), new wxThreadEvent());
        }
        if (time(0)-_kernel.getLastSavedTime() >= _kernel.getAutosaveInterval())
            _kernel.Autosave();
    }
    _kernel.CloseSession();
    m_KernelStatus = NumeReKernel::NUMERE_QUIT;
    wxQueueEvent(GetEventHandler(), new wxThreadEvent());
    return (wxThread::ExitCode)0;
}

void wxTerm::EndKernelTask()
{
    if (GetThread() && GetThread()->IsRunning())
    {
        erase_line();
        GetThread()->Delete();
    }
}

void wxTerm::OnClose(wxCloseEvent& event)
{
    if (GetThread() && GetThread()->IsRunning())
        GetThread()->Wait();
    wxMilliSleep(200);
    Destroy();
}

void wxTerm::OnThreadUpdate(wxThreadEvent& event)
{
    bool Closing = false;
    bool changedSettings = false;
    bool openDoc = false;
    bool done = false;
    bool editTable = false;
    string sFileName = "";
    stringmatrix sTable;
    vector<string> vDebugInfo;
    unsigned int nLineNumber = 0;
    int nFileOpenFlag = 0;
    string sAnswer = "";
    {
        wxCriticalSectionLocker lock(m_kernelCS);
        switch (m_KernelStatus)
        {
            case NumeReKernel::NUMERE_ANSWER_READ:
            case NumeReKernel::NUMERE_PENDING_SPECIAL:
                return;
            // fallthrough is intended
            case NumeReKernel::NUMERE_DONE:
                sAnswer = "|-> " + m_sAnswer + "\n|\n|<- ";
                done = true;
                break;
            case NumeReKernel::NUMERE_ERROR:
            case NumeReKernel::NUMERE_DONE_KEYWORD:
                sAnswer = m_sAnswer + "|\n|<- ";
                done = true;
                break;
            case NumeReKernel::NUMERE_EDIT_FILE:
                sAnswer = m_sAnswer;//+ "|\n|<- ";
                sFileName = _kernel.ReadFileName();
                nLineNumber = _kernel.ReadLineNumber();
                nFileOpenFlag = _kernel.ReadOpenFileFlag();
                break;
            case NumeReKernel::NUMERE_OPEN_DOC:
                sAnswer = m_sAnswer;//+ "|\n|<- ";
                sFileName = _kernel.ReadDoc();
                openDoc = true;
                break;
            case NumeReKernel::NUMERE_SHOW_TABLE:
                sAnswer = m_sAnswer;//+ "|\n|<- ";
                sTable = NumeReKernel::sTable;
                sFileName = NumeReKernel::sTableName;
                break;
            case NumeReKernel::NUMERE_EDIT_TABLE:
                sAnswer = m_sAnswer;//+ "|\n|<- ";
                sTable = NumeReKernel::sTable;
                sFileName = NumeReKernel::sTableName;
                editTable = true;
                break;
             case NumeReKernel::NUMERE_DEBUG_EVENT:
                sAnswer = m_sAnswer;//+ "|\n|<- ";
                m_bContinueDebug = false;
                vDebugInfo = NumeReKernel::vDebugInfos;
                break;
            case NumeReKernel::NUMERE_PENDING:
                sAnswer = "|<- ";
                break;
            case NumeReKernel::NUMERE_QUIT:
                Closing = true;
                break;
            default:
                //All other cases
                sAnswer = m_sAnswer;
        }
        m_sAnswer.clear();
        changedSettings = _kernel.SettingsModified();
        m_KernelStatus = NumeReKernel::NUMERE_ANSWER_READ;
    }
    if (done)
    {
        wxToolBar* tb = m_wxParent->GetToolBar();
		tb->EnableTool(ID_DEBUG_START, true);
		tb->EnableTool(ID_DEBUG_STOP, false);
    }
    if (Closing)
    {
        wxMilliSleep(300);
        m_wxParent->Close();
        return;
    }
    if (sTable.size())
    {
        if (editTable)
            m_wxParent->editTable(sTable, sFileName);
        else
            m_wxParent->openTable(sTable, sFileName);
    }
    else if (vDebugInfo.size())
    {
        m_wxParent->evaluateDebugInfo(vDebugInfo);
    }
    else if (openDoc)
    {
        m_wxParent->openHTML(sFileName);
    }
    else if (sFileName.length())
    {
        if (sFileName.find(".png") != string::npos
            || sFileName.find(".jpg") != string::npos
            || sFileName.find(".jpeg") != string::npos
            || sFileName.find(".gif") != string::npos
            || sFileName.find(".bmp") != string::npos)
        {
            m_wxParent->openImage(wxFileName(sFileName));
        }
        else
        {
            m_wxParent->OpenSourceFile(wxArrayString(1, sFileName), nLineNumber, nFileOpenFlag);
        }
    }
    if (changedSettings)
    {
        m_wxParent->EvaluateOptions();
    }
    ProcessOutput(sAnswer.length(), sAnswer);
    //GetTM()->SetEditable(m_curY, m_curX);
    //ProcessInput(sAnswer.length(), sAnswer);
    Refresh();
    //do something
}


//////////////////////////////////////////////////////////////////////////////
///  public SetBoldStyle
///  Sets the bold style for the terminal
///
///  @param  boldStyle wxTerm::BOLDSTYLE & The style to be used
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SetBoldStyle(wxTerm::BOLDSTYLE boldStyle)
{
    wxColour
    colors[16];

    if(boldStyle == DEFAULT)
        boldStyle = COLOR;

    m_boldStyle = boldStyle;
//  GetDefVTColors(colors, m_boldStyle);
//  SetVTColors(colors);
    Refresh();
}

//////////////////////////////////////////////////////////////////////////////
///  public SetFont
///  Sets the font for the terminal
///
///  @param  font const wxFont & The font to be used
///
///  @return bool Unused (returns true)
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool
wxTerm::SetFont(const wxFont& font)
{
    m_init = 1;

    wxWindow::SetFont(font);
    m_normalFont = font;
    m_underlinedFont = font;
    m_underlinedFont.SetUnderlined(true);
    m_boldFont = GetFont();
    m_boldFont.SetWeight(wxBOLD);
    m_boldUnderlinedFont = m_boldFont;
    m_boldUnderlinedFont.SetUnderlined(true);
    m_init = 0;

    ResizeTerminal(m_width, m_height);
    Refresh();

    return true;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetDfVTColors
///  Gets the colors for a VT100 terminal
///
///  @param  colors wxColour [] The colors that need to be assigned to
///  @param  boldStyle wxTerm::BOLDSTYLE The bold style used in the terminal
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::GetDefColors(wxColour colors[16], wxTerm::BOLDSTYLE boldStyle)
{
    if(boldStyle == DEFAULT)
        boldStyle = m_boldStyle;

    for (size_t i = 0; i <= NumeReSyntax::SYNTAX_METHODS; i++)
    {
        switch (i)
        {
            case 0:
                colors[0] = m_options->GetSyntaxStyle(i).background;
                break;
            case NumeReSyntax::SYNTAX_COMMAND:
                colors[NumeReSyntax::SYNTAX_COMMAND] = m_options->GetSyntaxStyle(Options::COMMAND).foreground;
                break;
            case NumeReSyntax::SYNTAX_OPTION:
                colors[NumeReSyntax::SYNTAX_OPTION] = m_options->GetSyntaxStyle(Options::OPTION).foreground;
                break;
            case NumeReSyntax::SYNTAX_FUNCTION:
                colors[NumeReSyntax::SYNTAX_FUNCTION] = m_options->GetSyntaxStyle(Options::FUNCTION).foreground;
                break;
            case NumeReSyntax::SYNTAX_CONSTANT:
                colors[NumeReSyntax::SYNTAX_CONSTANT] = m_options->GetSyntaxStyle(Options::CONSTANT).foreground;
                break;
            case NumeReSyntax::SYNTAX_SPECIALVAL:
                colors[NumeReSyntax::SYNTAX_SPECIALVAL] = m_options->GetSyntaxStyle(Options::SPECIALVAL).foreground;
                break;
            case NumeReSyntax::SYNTAX_STRING:
                colors[NumeReSyntax::SYNTAX_STRING] = m_options->GetSyntaxStyle(Options::STRING).foreground;
                break;
            case NumeReSyntax::SYNTAX_STD:
                colors[NumeReSyntax::SYNTAX_STD] = m_options->GetSyntaxStyle(Options::CONSOLE_STD).foreground;
                break;
            case NumeReSyntax::SYNTAX_OPERATOR:
                colors[NumeReSyntax::SYNTAX_OPERATOR] = m_options->GetSyntaxStyle(Options::OPERATOR).foreground;
                break;
            case NumeReSyntax::SYNTAX_PROCEDURE:
                colors[NumeReSyntax::SYNTAX_PROCEDURE] = m_options->GetSyntaxStyle(Options::PROCEDURE).foreground;
                break;
            case NumeReSyntax::SYNTAX_NUMBER:
                colors[NumeReSyntax::SYNTAX_NUMBER] = m_options->GetSyntaxStyle(Options::NUMBER).foreground;
                break;
            case NumeReSyntax::SYNTAX_NPRC_COMMAND:
                colors[NumeReSyntax::SYNTAX_NPRC_COMMAND] = m_options->GetSyntaxStyle(Options::PROCEDURE_COMMAND).foreground;
                break;
            case NumeReSyntax::SYNTAX_METHODS:
                colors[NumeReSyntax::SYNTAX_METHODS] = m_options->GetSyntaxStyle(Options::METHODS).foreground;
                break;
        }
    }
    // remaining, free colours
    colors[13] = wxColour(255, 255, 0);
    colors[14] = wxColour(0, 255, 255);
    colors[15] = wxColour(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////
///  public GetVTColors
///  Retrieves a copy of the VT100 colors
///
///  @param  colors wxColour [] An array to be filled with the VT100 colors
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::GetVTColors(wxColour colors[16])
{
    int
    i;

    for(i = 0; i < 16; i++)
        colors[i] = m_vt_colors[i];
}

//////////////////////////////////////////////////////////////////////////////
///  public SetVTColors
///  Sets the VT100 colors
///
///  @param  colors wxColour [] The new colors to be used
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SetVTColors(wxColour colors[16])
{
    int
    i;

    m_init = 1;
    for(i = 0; i < 16; i++)
        m_vt_colors[i] = colors[i];

    if(!(GetMode() & PC))
        SetBackgroundColour(m_vt_colors[7]);

    for(i = 0; i < 16; i++)
        m_vt_colorPens[i] = wxPen(m_vt_colors[i], 1, wxSOLID);
    m_init = 0;

    Refresh();
}

//////////////////////////////////////////////////////////////////////////////
///  public GetDefPCColors
///  Gets the default PC colors
///
///  @param  colors wxColour [] Filled with the colors to be used
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::GetDefPCColors(wxColour colors[16])
{
#if 0
    /*
    **  These colors need tweaking.  I'm sure they are not correct.
    */
    colors[0] = wxColour(0, 0, 0);                             // black
    colors[1] = wxColour(0, 0, 128);                           // blue
    colors[2] = wxColour(0, 128, 0);                           // green
    colors[3] = wxColour(0, 128, 128);                         // cyan
    colors[4] = wxColour(128, 0, 0);                           // red
    colors[5] = wxColour(128, 0, 128);                         // magenta
    colors[6] = wxColour(128, 128, 0);                         // brown
    colors[7] = wxColour(128, 128, 128);                       // white
    colors[8] = wxColour(64, 64, 64);                          // gray
    colors[9] = wxColour(0, 0, 255);                           // lt blue
    colors[10] = wxColour(0, 255, 0);                          // lt green
    colors[11] = wxColour(0, 255, 255);                        // lt cyan
    colors[12] = wxColour(255, 0, 0);                          // lt red
    colors[13] = wxColour(255, 0, 255);                        // lt magenta
    colors[14] = wxColour(255, 255, 0);                        // yellow
    colors[15] = wxColour(255, 255, 255);                      // white
#else
    /*
    **  These are much better
    */
    colors[0] = wxColour(0, 0, 0);                             // black
    colors[1] = wxColour(0, 0, 170);                           // blue
    colors[2] = wxColour(0, 170, 0);                           // green
    colors[3] = wxColour(0, 170, 170);                         // cyan
    colors[4] = wxColour(170, 0, 0);                           // red
    colors[5] = wxColour(170, 0, 170);                         // magenta
    colors[6] = wxColour(170, 170, 0);                         // brown
    colors[7] = wxColour(170, 170, 170);                       // white
#if 0
    colors[8] = wxColour(85, 85, 85);                          // gray
    colors[9] = wxColour(85, 85, 255);                         // lt blue
    colors[10] = wxColour(85, 255, 85);                        // lt green
    colors[11] = wxColour(85, 255, 255);                       // lt cyan
    colors[12] = wxColour(255, 85, 85);                        // lt red
    colors[13] = wxColour(255, 85, 255);                       // lt magenta
    colors[14] = wxColour(255, 255, 85);                       // yellow
    colors[15] = wxColour(255, 255, 255);                      // white
#else
    colors[8] = wxColour(50, 50, 50);                          // gray
    colors[9] = wxColour(0, 0, 255);                         // lt blue
    colors[10] = wxColour(0, 255, 0);                        // lt green
    colors[11] = wxColour(0, 255, 255);                       // lt cyan
    colors[12] = wxColour(255, 0, 0);                        // lt red
    colors[13] = wxColour(255, 0, 255);                       // lt magenta
    colors[14] = wxColour(255, 255, 0);                       // yellow
    colors[15] = wxColour(255, 255, 255);                      // white
#endif
#endif
}

//////////////////////////////////////////////////////////////////////////////
///  public GetPCColors
///  Retrieves the PC colors
///
///  @param  colors wxColour [] Filled with the PC colors
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::GetPCColors(wxColour colors[16])
{
    int
    i;

    for(i = 0; i < 16; i++)
        colors[i] = m_pc_colors[i];
}

//////////////////////////////////////////////////////////////////////////////
///  public SetPCColors
///  Sets the PC colors
///
///  @param  colors wxColour [] The new colors to be used
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SetPCColors(wxColour colors[16])
{
    int
    i;

    m_init = 1;
    for(i = 0; i < 16; i++)
        m_pc_colors[i] = colors[i];

    if(GetMode() & PC)
        SetBackgroundColour(m_pc_colors[7]);

    for(i = 0; i < 16; i++)
        m_pc_colorPens[i] = wxPen(m_pc_colors[i], 1, wxSOLID);
    m_init = 0;

    Refresh();
}

//////////////////////////////////////////////////////////////////////////////
///  public SetCursorBlinkRate
///  Sets how often the cursor blinks
///
///  @param  rate int  How many milliseconds between blinks
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SetCursorBlinkRate(int rate)
{
    if(rate < 0 || rate > CURSOR_BLINK_MAX_TIMEOUT)
        return;

    m_init = 1;
    if(rate != m_curBlinkRate)
    {
        m_curBlinkRate = rate;
        if(!m_curBlinkRate)
            m_timer.Stop();
        else
            m_timer.Start(m_curBlinkRate);
    }
    m_init = 0;
}

void wxTerm::pipe_command()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    string sCommand = "";
    if (GTerm::m_numCommandLines != -1)
    {
        for (int i = GetTM()->AdjustIndex(m_curY)-GTerm::m_numCommandLines-1; i < GetTM()->AdjustIndex(m_curY); i++)
        {
            sCommand += GetTM()->GetLine(i);
            if (sCommand.find_last_not_of(' ') != string::npos)
                sCommand.erase(sCommand.find_last_not_of(' ')+1);
        }
        GTerm::m_numCommandLines = -1;
    }
    else
        sCommand = GetTM()->GetLineAdjusted(m_curY-1);
    if (sCommand.substr(0,3) == "|<-")
        sCommand.erase(0,3);
    else if (sCommand.front() == '|' && sCommand.find('>') != string::npos && sCommand.find_first_not_of("-?IFORWHLPCMSE|") == sCommand.find('>')) // -?IFORWHLPCM
        sCommand.erase(0,sCommand.find('>')+1);
    while (sCommand.length() && sCommand.front() == ' ')
        sCommand.erase(0,1);
    m_sCommandLine = sCommand; // the commandline from tm...
    if (!NumeReKernel::bGettingLine) //only add the line to the history, if the kernel isn't currently fetching a line with NumeReKernel::getline()
        m_wxParent->AddToHistory(sCommand);
    m_bCommandAvailable = true;
}

void wxTerm::pass_command(const string& command)
{
    if (!command.length())
        return;
    if(GTerm::IsScrolledUp())
    {
        GTerm::Scroll(MAXHEIGHT, false);
        GTerm::Update();
        Refresh();
    }
    erase_line();
    wxCriticalSectionLocker lock(m_kernelCS);
    //next_line();
    m_wxParent->AddToHistory(command);
    m_sCommandLine = command;
    m_bCommandAvailable = true;
}

void wxTerm::CancelCalculation()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    //next_line();
    _kernel.CancelCalculation();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnChar
///  Handles user keyboard input and begins processing the server's response
///
///  @param  event wxKeyEvent & The generated key event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnChar(wxKeyEvent& event)
{
    if(!(GetMode() & PC) && event.AltDown())
        event.Skip();
    else
    {
        // if the user is scrolled up and they typed something, scroll
        // all the way to the bottom
        if(GTerm::IsScrolledUp())
        {
            GTerm::Scroll(MAXHEIGHT, false);
            GTerm::Update();
            Refresh();
        }

        int
        rc,
        keyCode = 0,
        len = 0;

        unsigned char
        buf[10];

        if(!keyCode )//&& !(keyCode = MapKeyCode((int)event.GetKeyCode())))
        {
            /*
            **  If the keycode wasn't mapped in the table and it is a special
            **  key, then we just ignore it.
            */
            /*if(event.GetKeyCode() >= WXK_START)
            {
              event.Skip();
              return;
            }*/
            /*
            **  Otherwise, it must just be an ascii character
            */
            keyCode = (int)event.GetKeyCode();
        }
        buf[0] = (char)keyCode;
        if (HasSelection())
            ClearSelection();
        if (keyCode == WXK_RETURN)
        {
            GTerm::cr();
            GTerm::lf();
            //synchronize();
            GetTM()->ChangeEditableState();
            GTerm::update_changes();
            //ProcessInput(4, "|\n|<- ");
            //Refresh();
            pipe_command();
            return;
        }
        else if (keyCode == WXK_BACK)
        {
            GTerm::resetAutoComp();
            if (GTerm::bs())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_TAB)
        {
            GTerm::tab();
            GTerm::update_changes();
            Refresh();
            return;
        }
        else if (keyCode == WXK_LEFT)
        {
            GTerm::resetAutoComp();
            if (GTerm::cursor_left())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_RIGHT)
        {
            GTerm::resetAutoComp();
            if (GTerm::cursor_right())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_UP)
        {
            GTerm::resetAutoComp();
            if (GTerm::cursor_up())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_DOWN)
        {
            GTerm::resetAutoComp();
            if (GTerm::cursor_down())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_HOME)
        {
            GTerm::resetAutoComp();
            if (GTerm::home())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_END)
        {
            GTerm::resetAutoComp();
            if (GTerm::end())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode == WXK_DELETE)
        {
            GTerm::resetAutoComp();
            if (GTerm::del())
            {
                GTerm::update_changes();
                Refresh();
            }
            return;
        }
        else if (keyCode >= WXK_START)
            return;
        GTerm::resetAutoComp();

        rc = 1;
        len = 1;
        if(rc)
        {
            if((GetMode() & NEWLINE) && !(GetMode() & PC) && (buf[len - 1] == 10))
            {
                buf[len - 1] = 13;
                buf[len] = 10;
                len++;
            }

            wxClientDC dc(this);

            m_curDC = &dc;
            GTerm::ProcessInput(len, string((char*)buf));
            //GetTM()->AppendChar(keyCode);
            //GetTM()->AddNewLine(string("<- ")+(char)keyCode);
            //GTerm::Update();
            //wxWindow::Update();
            //wxTerm::DrawText(1,0,0,0,0,1, buf);
            m_curDC = 0;
            //ProcessOutput(len, buf);
            /*if((GetMode() & LOCALECHO) && !(GetMode() & PC))
              ProcessInput(len, buf);*/
        }
        else
            event.Skip();
    }
}

//////////////////////////////////////////////////////////////////////////////
///  private OnKeyDown
///  Appears to be unused
///
///  @param  event wxKeyEvent & The generated key event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnKeyDown(wxKeyEvent& event)
{
    if (!(GetMode() & PC) && event.AltDown())
        event.Skip();
    else if (event.ControlDown() && event.ShiftDown())
    {
        if(event.GetKeyCode() == 'C' || event.GetKeyCode() == 'V')
        {
            if (event.GetKeyCode() == 'C' && HasSelection())
            {
                wxString sSelection = GetSelection();
                if (!sSelection.length())
                    return;
                if (wxTheClipboard->Open())
                {
                    wxTheClipboard->SetData(new wxTextDataObject(sSelection));
                    wxTheClipboard->Close();
                }
            }
            else if (event.GetKeyCode() == 'V')
            {
                if (HasSelection())
                    ClearSelection();
                if (wxTheClipboard->Open())
                {
                    if (wxTheClipboard->IsSupported(wxDF_TEXT))
                    {
                        wxTextDataObject data;
                        wxTheClipboard->GetData(data);
                        wxTerm::ProcessInput(data.GetTextLength(), data.GetText().ToStdString());
                        Refresh();
                    }
                    wxTheClipboard->Close();
                }
            }
            return;
        }
        //event.Skip();
    }
    else
        event.Skip();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnPaint
///  Redraws the terminal widget
///
///  @param  event wxPaintEvent & The generated paint event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC
    dc(this);

    m_curDC = &dc;
    ExposeArea(0, 0, m_width, m_height);
    m_curDC = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  private OnLeftDown
///  Begins selection of terminal text
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();

    ClearSelection();
    m_selx1 = m_selx2 = event.GetX() / m_charWidth;
    m_sely1 = m_sely2 = event.GetY() / m_charHeight;
    m_selecting = true;
    CaptureMouse();
}

void wxTerm::OnLoseMouseCapture(wxMouseCaptureLostEvent& event)
{
    if (GetCapture() == this)
    {
        m_selecting = false;
        ReleaseMouse();
        Refresh();
    }
}
//////////////////////////////////////////////////////////////////////////////
///  private OnLeftUp
///  Ends text selection
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnLeftUp(wxMouseEvent& event)
{
    m_selecting = false;
    if(GetCapture() == this)
    {
        ReleaseMouse();
        Refresh();
    }
    move_cursor_editable_area(m_selx2, m_sely2);
}

//////////////////////////////////////////////////////////////////////////////
///  private OnMouseMove
///  Changes the selection if the mouse button is down
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnMouseMove(wxMouseEvent& event)
{
    if(m_selecting)
    {
        m_selx2 = event.GetX() / m_charWidth;
        if(m_selx2 >= Width())
            m_selx2 = Width() - 1;
        m_sely2 = event.GetY() / m_charHeight;
        if(m_sely2 >= Height())
            m_sely2 = Height() - 1;

        if (event.AltDown())
            MarkSelection(true);
        else
            MarkSelection();
        Refresh();
    }
}


void wxTerm::OnEnter(wxMouseEvent& event)
{
    if (g_findReplace != nullptr && g_findReplace->IsShown())
    {
        event.Skip();
        return;
    }
    this->SetFocus();
    event.Skip();
}

//////////////////////////////////////////////////////////////////////////////
///  public ClearSelection
///  De-selects all selected text
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::ClearSelection()
{
    if (!HasSelection())
        return;
    int
    x,
    y;

    wxClientDC
    *dc = 0;

    m_selx1 = m_sely1 = m_selx2 = m_sely2 = 0;

    if(!m_curDC)
    {
        dc = new wxClientDC(this);
        m_curDC = dc;
    }

    for(y = 0; y < Height(); y++)
        for(x = 0; x < Width(); x++)
            Select(x, y, 0);
    if(dc)
    {
        this->wxWindow::Update();

        m_curDC = 0;
        delete dc;
    }
}

//////////////////////////////////////////////////////////////////////////////
///  private MarkSelection
///  Does _something_ as far as selecting text, but not really sure... used for SelectAll, I think
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::MarkSelection(bool bRectangular)
{
    int
    x,
    y;

    wxClientDC
    *dc = 0;

    m_marking = true;

    if (!m_curDC)
    {
        dc = new wxClientDC(this);
        m_curDC = dc;
    }

    for (y = 0; y < Height(); y++)
    {
        for (x = 0; x < Width(); x++)
            Select(x, y, 0);
    }
    if (bRectangular)
    {
        for (y = min(m_sely1, m_sely2); y <= max(m_sely1, m_sely2); y++)
        {
            for (x = min(m_selx1, m_selx2); x <= max(m_selx1, m_selx2); x++)
            {
                Select(x,y,1);
            }
        }
    }
    else
    {
        if (m_sely1 == m_sely2)
        {
            if (m_selx1 > m_selx2)
            {
                for (x = m_selx2; x <= m_selx1; x++)
                    Select(x, m_sely1, 1);
            }
            else
            {
                for (x = m_selx1; x <= m_selx2; x++)
                    Select(x, m_sely1, 1);
            }
        }
        else if (m_sely1 < m_sely2)
        {
            for (x = m_selx1; x < Width(); x++)
                Select(x, m_sely1, 1);

            for (y = m_sely1 + 1; y < m_sely2; y++)
            {
                for (x = 0; x < Width(); x++)
                    Select(x, y, 1);
            }
            for (x = 0; x <= m_selx2; x++)
                Select(x, m_sely2, 1);
        }
        else
        {
            for (x = 0; x <= m_selx1; x++)
                Select(x, m_sely1, 1);

            for (y = m_sely2 + 1; y < m_sely1; y++)
            {
                for (x = 0; x < Width(); x++)
                    Select(x, y, 1);
            }
            for (x = m_selx2; x < Width(); x++)
                Select(x, m_sely2, 1);
        }
    }
    //Refresh();
    //this->wxWindow::Update();

    if (dc)
    {
        m_curDC = 0;
        delete dc;
    }

    m_marking = false;
}

//////////////////////////////////////////////////////////////////////////////
///  public HasSelection
///  Checks if any text is selected
///
///  @return bool Whether or not there's any text selected
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool
wxTerm::HasSelection()
{
    return(m_selx1 != m_selx2 || m_sely1 != m_sely2);
}

//////////////////////////////////////////////////////////////////////////////
///  public GetSelection
///  Returns the selected text
///
///  @return wxString The selected text
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString
wxTerm::GetSelection()
{
    /*int
    x1,
    y1,
    x2,
    y2;*/

    wxString sel = get_selected_text();

    /*if (m_sely1 <= m_sely2 && m_selx1 <= m_selx2)
    {
        x1 = m_selx1;
        y1 = m_sely1;
        x2 = m_selx2;
        y2 = m_sely2;
    }
    else if (m_sely1 <= m_sely2 && m_selx1 > m_selx2)
    {
        x1 = m_selx2;
        y1 = m_sely1;
        x2 = m_selx1;
        y2 = m_sely2;
    }
    else if (m_sely1 > m_sely2 && m_selx1 <= m_selx2)
    {
        x1 = m_selx1;
        y1 = m_sely2;
        x2 = m_selx2;
        y2 = m_sely1;
    }
    else if (m_sely1 > m_sely2 && m_selx1 > m_selx2)
    {
        x1 = m_selx2;
        y1 = m_sely2;
        x2 = m_selx1;
        y2 = m_sely1;
    }

    while(x1 != x2 || y1 != y2)
    {
        if(GetChar(x1, y1))
            sel.Append(GetChar(x1, y1));

        x1++;
        if(x1 == Width())
        {
            sel.Append('\n');
            x1 = 0;
            y1++;
        }
    }
    if(GetChar(x1, y1))
        sel.Append(GetChar(x1, y1));*/

    return sel;
}

//////////////////////////////////////////////////////////////////////////////
///  public SelectAll
///  Selects the whole terminal
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SelectAll()
{
    m_selx1 = 0;
    m_sely1 = 0;
    m_selx2 = Width() - 1;
    m_sely2 = Height() - 1;
    MarkSelection();
}

/*
**  GTelnet stuff
*/
//////////////////////////////////////////////////////////////////////////////
///  public virtual DrawText
///  Responsible for actually drawing the terminal text on the widget.  This virtual
///  function is called from GTerm::update_changes.
///
///  @param  fg_color int             The index of the foreground color
///  @param  bg_color int             The index of the background color
///  @param  flags    int             Modifiers for drawing the text
///  @param  x        int             The x position in character cells
///  @param  y        int             The y position in character cells
///  @param  len      int             The number of characters to draw
///  @param  string   unsigned char * The characters to draw
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::DrawText(int fg_color, int bg_color, int flags,
                 int x, int y, int len, unsigned char *string)
{
    int
    t;

    /*if(flags & BOLD && m_boldStyle == COLOR)
        fg_color = (fg_color % 8) + 8;*/

    if(flags & SELECTED)
    {
        fg_color = 0;
        bg_color = 15;
    }

    if(flags & INVERSE)
    {
        t = fg_color;
        fg_color = bg_color;
        bg_color = t;
    }

    if(!m_curDC)
        return;

#if defined(__WXGTK__) || defined(__WXMOTIF__)
    int
    i;

    for(i = 0; string[i]; i++)
        string[i] = xCharMap[string[i]];
#endif

    wxString
    str(string, len);

    if(m_boldStyle != FONT)
    {
        if(flags & UNDERLINE)
            m_curDC->SetFont(m_underlinedFont);
        else
            m_curDC->SetFont(m_normalFont);
    }
    else
    {
        if(flags & BOLD)
        {
            if(flags & UNDERLINE)
                m_curDC->SetFont(m_boldUnderlinedFont);
            else
                m_curDC->SetFont(m_boldFont);
        }
        else
        {
            if(flags & UNDERLINE)
                m_curDC->SetFont(m_underlinedFont);
            else
                m_curDC->SetFont(m_normalFont);
        }
    }

    x = x * m_charWidth;
    y = y * m_charHeight;
    m_curDC->SetBackgroundMode(wxSOLID);
    m_curDC->SetTextBackground(m_colors[bg_color]);
    m_curDC->SetTextForeground(m_colors[fg_color]);
    m_curDC->DrawText(str, x, y);
    if(flags & BOLD && m_boldStyle == OVERSTRIKE)
        m_curDC->DrawText(str, x + 1, y);
}

//////////////////////////////////////////////////////////////////////////////
///  private DoDrawCursor
///  Does the actual work of drawing the cursor
///
///  @param  fg_color int            The index of the foreground color
///  @param  bg_color int            The index of the background color
///  @param  flags    int            Modifier flags
///  @param  x        int            The x position of the cursor, in characters
///  @param  y        int            The y position of the cursor, in characters
///  @param  c        unsigned char  The character the cursor is over
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::DoDrawCursor(int fg_color, int bg_color, int flags,
                     int x, int y, unsigned char c)
{
    if(GTerm::IsScrolledUp())
    {
        return;
    }
    int
    t;
    /*flags |= UNDERLINE;*/
    //flags |= BOLD;

    if(flags & BOLD && m_boldStyle == COLOR)
        fg_color = (fg_color % 8) + 8;

    if(flags & INVERSE)
    {
        t = fg_color;
        fg_color = bg_color;
        bg_color = t;
    }

    if(!m_curDC)
        return;

#if defined(__WXGTK__) || defined(__WXMOTIF__)
    c = xCharMap[c];
#endif

    /*if (flags & UNDERLINE && c != ' ')
        c = '_';*/
    wxString
    str((char)c);

    if(m_boldStyle != FONT)
    {
        if(flags & UNDERLINE)
            m_curDC->SetFont(m_underlinedFont);
        else
            m_curDC->SetFont(m_normalFont);
    }
    else
    {
        if(flags & BOLD)
        {
            if(flags & UNDERLINE)
                m_curDC->SetFont(m_boldUnderlinedFont);
            else
                m_curDC->SetFont(m_boldFont);
        }
        else
        {
            if(flags & UNDERLINE)
                m_curDC->SetFont(m_underlinedFont);
            else
                m_curDC->SetFont(m_normalFont);
        }
    }

    x = x * m_charWidth;
    y = y * m_charHeight;
    //if (!(flags & UNDERLINE))
        m_curDC->SetBackgroundMode(wxSOLID);
    m_curDC->SetTextBackground(m_colors[fg_color]);
    m_curDC->SetTextForeground(m_colors[bg_color]);
    m_curDC->DrawText(str, x, y);
    if(flags & BOLD && m_boldStyle == OVERSTRIKE)
        m_curDC->DrawText(str, x + 1, y);
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual DrawCursor
///  Draws the cursor on the terminal widget.  This virtual function is called
///  from GTerm::update_changes.
///
///  @param  fg_color int            The index of the foreground color
///  @param  bg_color int            The index of the background color
///  @param  flags    int            Modifiers for drawing the cursor
///  @param  x        int            The x position in character cells
///  @param  y        int            The y position in character cells
///  @param  c        unsigned char  The character that underlies the cursor
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::DrawCursor(int fg_color, int bg_color, int flags,
                   int x, int y, unsigned char c)
{
    m_curX = x;
    m_curY = y;
    m_curFG = fg_color;
    m_curBG = bg_color,
    m_curFlags = flags;
    m_curChar = c;



    if(m_timer.IsRunning())
        m_timer.Stop();
    DoDrawCursor(fg_color, bg_color, flags, x, y, c);
    if(m_curBlinkRate)
    {
        m_timer.Start(m_curBlinkRate);
        m_curState = 1;
    }
}

//////////////////////////////////////////////////////////////////////////////
///  private OnTimer
///  Blinks the cursor each time it goes off
///
///  @param  event wxTimerEvent & The generated timer event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    wxClientDC
    *dc = 0;

    if(m_init)
        return;

    if(m_curX == -1 || m_curY == -1)
        return;

    if(GetMode() & CURSORINVISIBLE)
    {
        //wxLogDebug("Skipping cursor");
        return;
    }
    //wxLogDebug("Drawing cursor");
    if(!m_curDC)
    {
        dc = new wxClientDC(this);
        m_curDC = dc;
    }

    if(m_curBlinkRate)
    {
        m_curState++;
        if(m_curState & 1 && m_curX != -1 && m_curY != -1)
            DoDrawCursor(m_curBG, m_curFG, m_curFlags, m_curX, m_curY, m_curChar);
        else
            DoDrawCursor(m_curFG, m_curBG, m_curFlags, m_curX, m_curY, m_curChar);
    }

    if(dc)
    {
        delete dc;
        m_curDC = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual MoveChars
///  Moves characters on the screen.  This virtual function is called from
///  GTerm::update_changes.
///
///  @param  sx   int  The starting x position, in character cells
///  @param  sy   int  The starting y position, in character cells
///  @param  dx   int  The number of character cells to shift in the x direction
///  @param  dy   int  The number of character cells to shift in the y direction
///  @param  w    int  The width in characters of the area to be moved
///  @param  h    int  The height in characters of the area to be moved
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::MoveChars(int sx, int sy, int dx, int dy, int w, int h)
{
    if(!m_marking)
        ClearSelection();

    sx = sx * m_charWidth;
    sy = sy * m_charHeight;
    dx = dx * m_charWidth;
    dy = dy * m_charHeight;
    w = w * m_charWidth;
    h = h * m_charHeight;

    wxClientDC dc(this);
    m_curDC = &dc;
    m_memDC.Blit(0, 0, w, h, m_curDC, sx, sy);
    m_curDC->Blit(dx, dy, w, h, &m_memDC, 0, 0);
    m_curDC = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ClearChars
///  Clears a section of characters from the screen.  This virtual function
///  is called from GTerm::update_changes.
///
///  @param  bg_color int  The background color to replace the characters with
///  @param  x        int  The starting x position, in characters
///  @param  y        int  The starting y position, in characters
///  @param  w        int  The width of the area to be cleared, in characters
///  @param  h        int  The height of the area to be cleared, in characters
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::ClearChars(int bg_color, int x, int y, int w, int h)
{
//  if(!m_marking)
//    ClearSelection();

    x = x * m_charWidth;
    y = y * m_charHeight;
    w = w * m_charWidth;
    h = h * m_charHeight;

    bool deleteDC = false;
    if(!m_curDC)
    {
        m_curDC = new wxClientDC(this);
        deleteDC = true;
    }
    m_curDC->SetPen(m_colorPens[bg_color]);
    m_curDC->SetBrush(wxBrush(m_colors[bg_color], wxSOLID));
    m_curDC->DrawRectangle(x, y, w /* + 1*/, h /*+ 1*/);

    if(deleteDC)
    {
        delete m_curDC;
        m_curDC = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ModeChange
///  Changes the drawing mode between VT100 and PC
///
///  @param  state int  The new state
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::ModeChange(int state)
{
    ClearSelection();

    if(state & GTelnet::PC)
    {
        m_colors = m_pc_colors;
        m_colorPens = m_pc_colorPens;
    }
    else
    {
        m_colors = m_vt_colors;
        m_colorPens = m_vt_colorPens;
    }
    GTerm/*lnet*/::ModeChange(state);
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual Bell
///  Rings the system bell
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::Bell()
{
#ifdef __WIN32__
    WinMessageBeep();
#else
    wxBell();
#endif
}

//////////////////////////////////////////////////////////////////////////////
///  public UpdateSize
///  Updates the terminal's size in characters after it has been resized on the screen.
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::UpdateSize()
{
    // prevent any nasty recursion
    if(m_inUpdateSize)
    {
        return;
    }

    m_inUpdateSize = true;
    int charWidth, charHeight;

    wxClientDC* dc = new wxClientDC(this);

    if(!m_curDC)
    {
        m_curDC = dc;
    }

    dc->SetFont(m_normalFont);
    dc->GetTextExtent("M", &charWidth, &charHeight);
    wxSize currentClientSize = GetClientSize();
    int numCharsInLine = currentClientSize.GetX() / charWidth;
    int numLinesShown = currentClientSize.GetY() / charHeight;



    if( (numCharsInLine != m_charsInLine) || (numLinesShown != m_linesDisplayed))
    {
        wxString message;

        // FINALLY!  Finally killed the memory leak!  The problem is that somehow a size event
        // was generating negative numbers for these values, which led to weird things happening.
        if( (numCharsInLine > 0) && (numLinesShown > 0))
        {
            m_charsInLine = numCharsInLine;
            m_linesDisplayed = numLinesShown;
            // tell the GTerm core to resize itself
            ResizeTerminal(numCharsInLine, numLinesShown);
            {
                wxCriticalSectionLocker lock(m_kernelCS);
                _kernel.updateLineLenght(numCharsInLine);
            }
            //UpdateRemoteSize(m_charsInLine, m_linesDisplayed);
            /*
            wxString remoteResizeCommand = wxString::Format("stty rows %d cols %d", m_linesDisplayed, m_charsInLine);
            wxLogDebug("Resizing terminal: %s", remoteResizeCommand);
            wxStringBuffer tempBuffer(remoteResizeCommand, 256);
            SendBack(tempBuffer);
            */
        }
    }

    m_inUpdateSize = false;
    Refresh();
    if(dc)
    {
        delete dc;
        m_curDC = 0;
    }
}

void wxTerm::UpdateColors()
{
    GetDefColors(m_vt_colors);

    m_colors = m_vt_colors;

    SetBackgroundColour(m_colors[0]);

    for (int i = 0; i < 16; i++)
        m_vt_colorPens[i] = wxPen(m_vt_colors[i], 1, wxSOLID);

    m_colorPens = m_vt_colorPens;
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ResizeTerminal
///  <Resizes the terminal to a given number of characters high and wide
///
///  @param  w    int  The new number of characters wide
///  @param  h    int  The new number of characters high
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::ResizeTerminal(int width, int height)
{
    int
    w,
    h;

    ClearSelection();

    /*
    **  Determine window size from current font
    */
    wxClientDC
    dc(this);

    if(m_boldStyle != FONT)
        dc.SetFont(m_normalFont);
    else
        dc.SetFont(m_boldFont);
    dc.GetTextExtent("M", &m_charWidth, &h); // EKHL: Changed because Heigth made no sense
    dc.GetTextExtent("My", &w, &m_charHeight);
    w = width * m_charWidth;
    h = height * m_charHeight;

    /*
    **  Create our bitmap for copying
    */
    if(m_bitmap)
    {
        m_memDC.SelectObject(wxNullBitmap);
        delete m_bitmap;
    }
    m_bitmap = new wxBitmap(w, h);
    m_memDC.SelectObject(*m_bitmap);

    /*
    **  Set window size
    */
#if defined(__WXGTK__) || defined(__WXMOTIF__)
    SetSize(w, h + 4);
#else
    SetSize(w, h);
#endif

    /*
    **  Set terminal size
    */
    GTerm::ResizeTerminal(width, height);
    m_width = width;
    m_height = height;

    /*
    **  Send event
    */
    if(!m_init)
    {
        wxCommandEvent e(wxEVT_COMMAND_TERM_RESIZE, GetId());
        e.SetEventObject(this);
        GetParent()->GetEventHandler()->ProcessEvent(e);
    }
    //GTerm::Update();
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual RequestSizeChange
///  A virtual function, used by GTerm to update the size
///
///  @param  w    int  The new width in characters
///  @param  h    int  The new height in characters
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::RequestSizeChange(int w, int h)
{
    ResizeTerminal(w, h);
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual ProcessInput
///  Processes text received from the server
///
///  @param  len  int             The number of characters received
///  @param  data unsigned char * The received text
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::ProcessInput(int len, const string& sData)
{
    wxClientDC
    dc(this);

    //ClearSelection();
    m_curDC = &dc;
    GTerm::ProcessInput(len, sData);
    m_curDC = 0;
}

void
wxTerm::ProcessOutput(int len, const string& sData)
{
    if (HasSelection())
        ClearSelection();
    wxClientDC
    dc(this);
    m_curDC = &dc;
    GTerm::ProcessOutput(len, sData);
    m_curDC = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  private MapKeyCode
///  Does some sort of character lookup
///
///  @param  keyCode int  The keycode to check
///
///  @return int     The returned keycode
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int
wxTerm::MapKeyCode(int keyCode)
{
    int
    i;

    for(i = 0; keyMapTable[i].keyCode; i++)
        if(keyMapTable[i].keyCode == keyCode)
            return keyMapTable[i].VTKeyCode;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual SelectPrinter
///  Not really sure... certainly not used in anything Chameleon needs
///
///  @param  PrinterName char * The text of the printer name
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::SelectPrinter(char *PrinterName)
{
    if(m_printerFN)
    {
        if(m_printerName[0] == '#')
            fclose(m_printerFN);
        else
#if defined(__WXGTK__) || defined(__WXMOTIF__)
            pclose(m_printerFN);
#endif
#if defined(__WXMSW__)
        fclose(m_printerFN);
#endif

        m_printerFN = 0;
    }

    if(m_printerName)
    {
        free(m_printerName);
        m_printerName = 0;
    }

    if(strlen(PrinterName))
    {
        m_printerName = strdup(PrinterName);
    }
}

//////////////////////////////////////////////////////////////////////////////
///  public virtual PrintChars
///  Prints stuff (to a file?), apparently
///
///  @param  len  int             The number of characters
///  @param  data unsigned char * The text
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
wxTerm::PrintChars(int len, unsigned char *data)
{
    char
    pname[100];

    if(!m_printerFN)
    {
        if(!m_printerName)
            return;

        if(m_printerName[0] == '#')
        {
#if defined(__WXGTK__) || defined(__WXMOTIF__)
            sprintf(pname, "/dev/lp%d", m_printerName[1] - '0');
#endif
#if defined(__WXMSW__)
            sprintf(pname, "lpt%d", m_printerName[1] - '0' + 1);
#endif
            m_printerFN = fopen(pname, "wb");
        }
        else
        {
#if defined(__WXGTK__) || defined(__WXMOTIF__)
            sprintf(pname, "lpr -P%s", m_printerName);
            m_printerFN = popen(pname, "w");
#endif
#if defined(__WXMSW__)
            m_printerFN = fopen(m_printerName, "wb");
#endif
        }
    }

    if(m_printerFN)
    {
        fwrite(data, len, 1, m_printerFN);
    }
}

//////////////////////////////////////////////////////////////////////////////
///  private OnActivate
///  Sets the terminal's active state - determines whether or not to draw the cursor
///
///  @param  event wxActivateEvent & The generated activate event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::OnActivate(wxActivateEvent &event)
{
    m_isActive = event.GetActive();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnGainFocus
///  Enables the cursor
///
///  @param  event wxFocusEvent & The generated focus event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::OnGainFocus(wxFocusEvent &event)
{
    this->clear_mode_flag(CURSORINVISIBLE);
    wxLogDebug("Gained focus");
    GTerm::Update();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnLoseFocus
///  Disables the cursor
///
///  @param  event wxFocusEvent & The generated focus event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::OnLoseFocus(wxFocusEvent &event)
{
    this->set_mode_flag(CURSORINVISIBLE);
    wxLogDebug("Lost focus");
    GTerm::Update();
}

//////////////////////////////////////////////////////////////////////////////
///  public ScrollTerminal
///  Scrolls the terminal text
///
///  @param  numLines int   The number of lines to scroll
///  @param  scrollUp bool  [=true] True to scroll up, false to scroll down
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::ScrollTerminal(int numLines, bool scrollUp /* = true */)
{
    if (GTerm::Scroll(numLines, scrollUp))
        Refresh();
}

//////////////////////////////////////////////////////////////////////////////
///  private OnSize
///  Lets the terminal resize the text whenever the window is resized
///
///  @param  event wxSizeEvent & The generated size event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxTerm::OnSize(wxSizeEvent &event)
{
    UpdateSize();
}

void wxTerm::UpdateRemoteSize(int width, int height)
{

}
